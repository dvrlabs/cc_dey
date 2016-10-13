/*
 * ccimp_network_tcp.c
 *
 * Copyright (C) 2016 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi.
 * International Inc. By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered. This is an unpublished copyrighted work of Digi International
 * Inc. Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to restrictions
 * set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
 * Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and
 * (2) of the Commercial Computer Software - Restricted Rights at 48 CFR
 * 52.227-19, as applicable.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 *
 * Description: Cloud Connector network implementation.
 *
 */

#include "ccimp/ccimp_network.h"
#include "ccimp/ccimp_os.h"

#if (defined UNIT_TEST)
#define ccimp_network_tcp_open       ccimp_network_tcp_open_real
#define ccimp_network_tcp_send       ccimp_network_tcp_send_real
#define ccimp_network_tcp_receive    ccimp_network_tcp_receive_real
#define ccimp_network_tcp_close      ccimp_network_tcp_close_real
#endif

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "dns_helper.h"
#include "utils.h"

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static int app_tcp_create_socket(void);
static ccimp_status_t app_tcp_connect(int const fd, in_addr_t const ip_addr);
static ccimp_status_t app_is_tcp_connect_complete(int const fd);

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
ccimp_status_t ccimp_network_tcp_close(ccimp_network_close_t * const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
	int * const fd = data->handle;

	if (close(*fd) < 0)
		log_error("network_tcp_close(): close() failed, fd %d, errno %d", *fd, errno);
	else
		log_error("network_tcp_close(): fd %d", *fd);

	*fd = -1;
	free(fd);
	return status;
}

ccimp_status_t ccimp_network_tcp_receive(ccimp_network_receive_t * const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
	int * const fd = data->handle;

	int ccode = read(*fd, data->buffer, data->bytes_available);
	if (ccode > 0) {
		data->bytes_used = (size_t) ccode;
	} else if (ccode == 0) {
		/* EOF on input: the connection was closed. */
		log_debug("ccimp_network_tcp_receive(): network_receive: EOF on socket");
		errno = ECONNRESET;
		status = CCIMP_STATUS_ERROR;
	} else {
		int const err = errno;
		/* An error of some sort occurred: handle it appropriately. */
		if (err == EAGAIN) {
			status = CCIMP_STATUS_BUSY;
		} else {
			log_error("ccimp_network_tcp_receive(): network_receive: recv() failed, errno %d", err);
			/* if not timeout (no data) return an error */
			dns_cache_invalidate();
			status = CCIMP_STATUS_ERROR;
		}
	}
	return status;
}

ccimp_status_t ccimp_network_tcp_send(ccimp_network_send_t * const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
	int * const fd = data->handle;

	int ccode = write(*fd, data->buffer, data->bytes_available);
	if (ccode >= 0) {
		data->bytes_used = (size_t) ccode;
	} else {
		int const err = errno;
		if (err == EAGAIN) {
			status = CCIMP_STATUS_BUSY;
		} else {
			status = CCIMP_STATUS_ERROR;
			log_error("app_network_tcp_send(): send() failed, errno %d", err);
			dns_cache_invalidate();
		}
	}

	return status;
}

ccimp_status_t ccimp_network_tcp_open(ccimp_network_open_t * const data)
{
#define APP_CONNECT_TIMEOUT 30

	static unsigned long connect_time;
	int * pfd = NULL;
	struct sockaddr_in interface_addr;
	socklen_t interface_addr_len;

	ccimp_status_t status = CCIMP_STATUS_ERROR;

	if (data->handle == NULL) {
		pfd = (int *) malloc(sizeof *pfd);
		if (pfd == NULL)
			goto error;
		*pfd = -1;
		data->handle = pfd;
	} else {
		pfd = data->handle;
	}

	if (*pfd == -1) {
		in_addr_t ip_addr;
		int const dns_resolve_error = dns_resolve(data->device_cloud.url, &ip_addr);
		if (dns_resolve_error != 0) {
			log_error("app_network_tcp_open(): Can't resolve DNS for %s", data->device_cloud.url);
			status = CCIMP_STATUS_ERROR;
			goto done;
		}

		*pfd = app_tcp_create_socket();
		if (*pfd == -1) {
			status = CCIMP_STATUS_ERROR;
			free(pfd);
			goto done;
		}

		{
			ccimp_os_system_up_time_t uptime;
			ccimp_os_get_system_time(&uptime);
			connect_time = uptime.sys_uptime;
		}
		status = app_tcp_connect(*pfd, ip_addr);
		if (status != CCIMP_STATUS_OK)
			goto error;
	}

	/* Get socket info of connected interface */
	interface_addr_len = sizeof interface_addr;
	if (getsockname(*pfd, (struct sockaddr *) &interface_addr, &interface_addr_len)) {
		log_error("ccimp_network_tcp_open(): getsockname error, errno %d", errno);
		goto done;
	}

	status = app_is_tcp_connect_complete(*pfd);
	if (status == CCIMP_STATUS_OK) {
		log_info("app_network_tcp_open(): connected to %s", data->device_cloud.url);
		goto done;
	}

	if (status == CCIMP_STATUS_BUSY) {
		unsigned long elapsed_time;
		ccimp_os_system_up_time_t uptime;

		ccimp_os_get_system_time(&uptime);
		elapsed_time = uptime.sys_uptime - connect_time;

		if (elapsed_time >= APP_CONNECT_TIMEOUT) {
			log_error("app_network_tcp_open(): failed to connect within 30 seconds");
			status = CCIMP_STATUS_ERROR;
		}
	}

error:
	if (status == CCIMP_STATUS_ERROR) {
		log_error("app_network_tcp_open(): failed to connect to %s", data->device_cloud.url);
		dns_set_redirected(0);

		if (pfd != NULL) {
			if (*pfd >= 0) {
				close(*pfd);
				*pfd = -1;
			}
			free(pfd);
		}
	}

done:
	return status;
}

static int app_tcp_create_socket(void)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd >= 0) {
		int enabled = 1;

		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof enabled) < 0)
			log_error("app_tcp_create_socket(): setsockopt SO_KEEPALIVE failed, errno %d", errno);

		if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof enabled) < 0)
			log_error("app_tcp_create_socket(): setsockopt TCP_NODELAY failed, errno %d", errno);

		if (ioctl(fd, FIONBIO, &enabled) < 0) {
			log_error("app_tcp_create_socket(): ioctl: FIONBIO failed, errno %d", errno);
			close(fd);
			fd = -1;
		}
	} else {
		log_error("Could not open TCP socket, errno %d", errno);
	}

	return fd;
}

static ccimp_status_t app_tcp_connect(int const fd, in_addr_t const ip_addr)
{

	struct sockaddr_in sin = { 0 };
	ccimp_status_t status = CCIMP_STATUS_OK;

	memcpy(&sin.sin_addr, &ip_addr, sizeof sin.sin_addr);
	sin.sin_port = htons(CCIMP_TCP_PORT);
	sin.sin_family = AF_INET;

	log_debug("app_tcp_connect(): fd %d", fd);

	if (connect(fd, (struct sockaddr *) &sin, sizeof sin) < 0) {
		int const err = errno;
		switch (err) {
		case EINTR:
		case EAGAIN:
		case EINPROGRESS:
			status = CCIMP_STATUS_BUSY;
			break;
		default:
			log_error("app_tcp_connect(): connect() failed, fd %d, errno %d", fd, err);
			status = CCIMP_STATUS_ERROR;
		}
	}

	return status;
}

static ccimp_status_t app_is_tcp_connect_complete(int const fd)
{
	ccimp_status_t status = CCIMP_STATUS_BUSY;
	struct timeval timeout = { 0 };
	fd_set read_set, write_set;
	int rc;

	FD_ZERO(&read_set);
	FD_SET(fd, &read_set);
	write_set = read_set;

	rc = select(fd + 1, &read_set, &write_set, NULL, &timeout);
	if (rc < 0) {
		if (errno != EINTR) {
			log_error("app_is_tcp_connect_complete(): select on fd %d returned %d, errno %d", fd, rc, errno);
			status = CCIMP_STATUS_ERROR;
		}
	} else {
		/* Check whether the socket is now writable (connection succeeded). */
		if (rc > 0 && FD_ISSET(fd, &write_set)) {
			/* We expect "socket writable" when the connection succeeds. */
			/* If we also got a "socket readable" we have an error. */
			if (FD_ISSET(fd, &read_set)) {
				log_error("app_is_tcp_connect_complete(): FD_ISSET for read, fd %d", fd);
				status = CCIMP_STATUS_ERROR;
			} else {
				status = CCIMP_STATUS_OK;
			}
		}
	}
	return status;
}
