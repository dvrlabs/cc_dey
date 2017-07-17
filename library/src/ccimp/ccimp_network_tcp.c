/*
 * Copyright (c) 2017 Digi International Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
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
#if (defined APP_SSL)
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include "dns_helper.h"
#include "cc_logging.h"

/*------------------------------------------------------------------------------
                 D A T A    T Y P E S    D E F I N I T I O N S
------------------------------------------------------------------------------*/
#if (defined APP_SSL)
typedef struct
{
	int *sfd;
	SSL_CTX *ctx;
	SSL *ssl;
} app_ssl_t;
#endif

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static int app_tcp_create_socket(void);
static ccimp_status_t app_tcp_connect(int const fd, in_addr_t const ip_addr);
static ccimp_status_t app_is_tcp_connect_complete(int const fd);
#if (defined APP_SSL)
static int get_user_passwd(char *buf, int size, int rwflag, void *password);
static int app_load_certificate_and_key(SSL_CTX *const ctx);
static void app_free_ssl_info(app_ssl_t *const ssl_ptr);
static int app_verify_device_cloud_certificate(SSL *const ssl);
static int app_ssl_connect(app_ssl_t *const ssl_ptr);
#endif

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
ccimp_status_t ccimp_network_tcp_close(ccimp_network_close_t *const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
	int *fd = NULL;
#if (defined APP_SSL)
	app_ssl_t *const ssl_ptr = data->handle;
	fd = ssl_ptr->sfd;
#else
	fd = data->handle;
#endif

	if (close(*fd) < 0)
		log_error("ccimp_network_tcp_close(): close() failed, fd %d, errno %d", *fd, errno);

#if (defined APP_SSL)
	/* send close notify to peer */
	if (SSL_shutdown(ssl_ptr->ssl) == 0)
		SSL_shutdown(ssl_ptr->ssl);  /* wait for peer's close notify */

	app_free_ssl_info(ssl_ptr);
#endif

	*fd = -1;
	free(fd);
	return status;
}

ccimp_status_t ccimp_network_tcp_receive(ccimp_network_receive_t *const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
	int read_bytes = 0;
#if (defined APP_SSL)
	app_ssl_t *const ssl_ptr = data->handle;

	if (SSL_pending(ssl_ptr->ssl) == 0) {
		int ready;
		struct timeval timeout;
		fd_set read_set;

		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		FD_ZERO(&read_set);
		FD_SET(*ssl_ptr->sfd, &read_set);

		ready = select(*ssl_ptr->sfd + 1, &read_set, NULL, NULL, &timeout);
		if (ready == 0) {
			status = CCIMP_STATUS_BUSY;
			goto done;
		} else if (ready < 0) {
			log_error("%s", "ccimp_network_tcp_receive(): select failed");
			goto done;
		}
	}
	read_bytes = SSL_read(ssl_ptr->ssl, data->buffer, data->bytes_available);
#else
	int *const fd = data->handle;

	read_bytes = read(*fd, data->buffer, data->bytes_available);
#endif

	if (read_bytes > 0) {
		data->bytes_used = (size_t) read_bytes;
		goto done;
	} else if (read_bytes == 0) {
		/* EOF on input: the connection was closed. */
		log_debug("%s", "ccimp_network_tcp_receive(): network_receive: EOF on socket");
		errno = ECONNRESET;
		status = CCIMP_STATUS_ERROR;
	} else {
		int const err = errno;
		/* An error of some sort occurred: handle it appropriately. */
#if (defined APP_SSL)
		int ssl_error = SSL_get_error(ssl_ptr->ssl, read_bytes);
		if (ssl_error == SSL_ERROR_WANT_READ) {
			status = CCIMP_STATUS_BUSY;
			goto done;
		}
		SSL_set_shutdown(ssl_ptr->ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
#endif
		if (err == EAGAIN) {
			status = CCIMP_STATUS_BUSY;
		} else {
			log_error("ccimp_network_tcp_receive(): network_receive: recv() failed, errno %d", err);
			/* if not timeout (no data) return an error */
			dns_cache_invalidate();
			status = CCIMP_STATUS_ERROR;
		}
	}
done:
	return status;
}

ccimp_status_t ccimp_network_tcp_send(ccimp_network_send_t *const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
	int sent_bytes = 0;
#if (defined APP_SSL)
	app_ssl_t *const ssl_ptr = data->handle;

	sent_bytes = SSL_write(ssl_ptr->ssl, data->buffer, data->bytes_available);
#else
	int *const fd = data->handle;

	sent_bytes = write(*fd, data->buffer, data->bytes_available);
#endif

	if (sent_bytes >= 0) {
		data->bytes_used = (size_t) sent_bytes;
	} else {
		int const err = errno;
		if (err == EAGAIN) {
			status = CCIMP_STATUS_BUSY;
		} else {
			status = CCIMP_STATUS_ERROR;
			log_error("ccimp_network_tcp_send(): send() failed, errno %d", err);
#if (defined APP_SSL)
			SSL_set_shutdown(ssl_ptr->ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
#endif
			dns_cache_invalidate();
		}
	}

	return status;
}

ccimp_status_t ccimp_network_tcp_open(ccimp_network_open_t *const data)
{
#define APP_CONNECT_TIMEOUT 30

	static unsigned long connect_time;
	int *pfd = NULL;
	struct sockaddr_in interface_addr;
	socklen_t interface_addr_len;
#if (defined APP_SSL)
	static app_ssl_t *ssl_info = NULL;
	ssl_info = calloc(1, sizeof(app_ssl_t));
#endif

	ccimp_status_t status = CCIMP_STATUS_ERROR;

	if (data->handle == NULL) {
		pfd = (int *) malloc(sizeof *pfd);
		if (pfd == NULL)
			goto error;
		*pfd = -1;
		data->handle = pfd;
	} else {
#if (defined APP_SSL)
		app_ssl_t *const ssl_ptr = data->handle;
		pfd = ssl_ptr->sfd;
#else
		pfd = data->handle;
#endif
	}

	if (*pfd == -1) {
		in_addr_t ip_addr;
		int const dns_resolve_error = dns_resolve(data->device_cloud.url, &ip_addr);
		if (dns_resolve_error != 0) {
			log_error("ccimp_network_tcp_open(): Can't resolve DNS for %s", data->device_cloud.url);
			status = CCIMP_STATUS_ERROR;
			goto done;
		}

		*pfd = app_tcp_create_socket();
		if (*pfd == -1) {
			status = CCIMP_STATUS_ERROR;
			free(pfd);
			goto done;
		}
#if (defined APP_SSL)
		ssl_info->sfd = pfd;
#endif

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
#if (defined APP_SSL)
		log_debug("%s: openning SSL socket", __func__);
		if (app_ssl_connect(ssl_info)) {
			log_error("%s", "ccimp_network_tcp_open(): ssl connect error");
			status = CCIMP_STATUS_ERROR;
			goto error;
		}
		data->handle = ssl_info;
#endif
		/* make it non-blocking now */
		{
			int enabled = 1;

			if (ioctl(*pfd, FIONBIO, &enabled) < 0)
			{
				log_error("ccimp_network_tcp_open(): ioctl: FIONBIO failed, errno %d\n", errno);
				status = CCIMP_STATUS_ERROR;
				goto error;
			}
		}

		log_info("ccimp_network_tcp_open(): connected to %s", data->device_cloud.url);
		goto done;
	}

	if (status == CCIMP_STATUS_BUSY) {
		unsigned long elapsed_time;
		ccimp_os_system_up_time_t uptime;

		ccimp_os_get_system_time(&uptime);
		elapsed_time = uptime.sys_uptime - connect_time;

		if (elapsed_time >= APP_CONNECT_TIMEOUT) {
			log_error("%s", "ccimp_network_tcp_open(): failed to connect within 30 seconds");
			status = CCIMP_STATUS_ERROR;
		}
	}

error:
	if (status == CCIMP_STATUS_ERROR) {
		log_error("ccimp_network_tcp_open(): failed to connect to %s", data->device_cloud.url);
		dns_set_redirected(0);

		if (pfd != NULL) {
			if (*pfd >= 0) {
				close(*pfd);
				*pfd = -1;
			}
			free(pfd);
		}
#if (defined APP_SSL)
		app_free_ssl_info(ssl_info);
#endif
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
#if (defined APP_SSL)
	sin.sin_port = htons(CCIMP_SSL_PORT);
#else
	sin.sin_port = htons(CCIMP_TCP_PORT);
#endif
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

#if (defined APP_SSL)
	/* wait for 2 seconds to connect */
	timeout.tv_sec = 2;
#endif
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

#if (defined APP_SSL)
#if (defined APP_SSL_CLNT_CERT)
static int get_user_passwd(char * buf, int size, int rwflag, void * password)
{
	char const passwd[] = APP_SSL_CLNT_CERT_PASSWORD;
	int const pwd_bytes = ARRAY_SIZE(passwd) - 1;
	int const copy_bytes = (pwd_bytes < size) ? pwd_bytes : size-1;

	UNUSED_ARGUMENT(rwflag);
	UNUSED_ARGUMENT(password);

	ASSERT_GOTO(copy_bytes >= 0, error);
	memcpy(buf, passwd, copy_bytes);
	buf[copy_bytes] = '\0';

error:
	return copy_bytes;
}
#endif

static int app_load_certificate_and_key(SSL_CTX *const ctx)
{
	int ret = -1;

	ret = SSL_CTX_load_verify_locations(ctx, APP_SSL_CA_CERT_PATH, NULL);
	if (ret != 1) {
		log_error("app_load_certificate_and_key(): Failed to load CA cert %d", ret);
		ERR_print_errors_fp(stderr);
		goto error;
	}

#if (defined APP_SSL_CLNT_CERT)
	SSL_CTX_set_default_passwd_cb(ctx, get_user_passwd);
	ret = SSL_CTX_use_certificate_file(ctx, APP_SSL_CLNT_KEY, SSL_FILETYPE_PEM);
	if (ret != 1) {
		log_error("app_load_certificate_and_key(): SSL_use_certificate_file() Error [%d]", ret);
		goto error;
	}

	ret = SSL_CTX_use_RSAPrivateKey_file(ctx, APP_SSL_CLNT_CERT, SSL_FILETYPE_PEM);
	if (ret != 1) {
		log_error("app_load_certificate_and_key():SSL_use_RSAPrivateKey_file() Error [%d]", ret);
		goto error;
	}
#endif

error:
	return ret;
}

static void app_free_ssl_info(app_ssl_t *const ssl_ptr)
{
	if (ssl_ptr != NULL) {
		SSL_free(ssl_ptr->ssl);
		ssl_ptr->ssl = NULL;

		SSL_CTX_free(ssl_ptr->ctx);
		ssl_ptr->ctx = NULL;

		free(ssl_ptr);
	}
}

static int app_verify_device_cloud_certificate(SSL *const ssl)
{
	int ret = -1;
	X509 *const device_cloud_cert = SSL_get_peer_certificate(ssl);

	if (device_cloud_cert == NULL) {
		log_error("%s", "app_verify_device_cloud_certificate(): No Remote Manager certificate is provided");
		goto done;
	}

	ret = SSL_get_verify_result(ssl);
	if (ret !=  X509_V_OK) {
		log_error("app_verify_device_cloud_certificate(): Remote Manager certificate is invalid %d", ret);
		goto done;
	}

done:
	X509_free(device_cloud_cert);
	return ret;
}

static int app_ssl_connect(app_ssl_t *const ssl_ptr)
{
	int ret = -1;

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ssl_ptr->ctx = SSL_CTX_new(TLSv1_client_method());
	if (ssl_ptr->ctx == NULL) {
		log_error("%s", "app_ssl_connect(): ssl context is null");
		ERR_print_errors_fp(stderr);
		goto error;
	}

	ssl_ptr->ssl = SSL_new(ssl_ptr->ctx);
	if (ssl_ptr->ssl == NULL) {
		log_error("%s", "app_ssl_connect(): error creating new SSL");
		ERR_print_errors_fp(stderr);
		goto error;
	}

	SSL_set_fd(ssl_ptr->ssl, *ssl_ptr->sfd);
	if (app_load_certificate_and_key(ssl_ptr->ctx) != 1) {
		log_error("%s", "app_ssl_connect(): error loading certificate and key");
		goto error;
	}

	SSL_set_options(ssl_ptr->ssl, SSL_OP_ALL);
	if (SSL_connect(ssl_ptr->ssl) <= 0) {
		log_error("app_ssl_connect(): error connecting using SSL %s", strerror(errno));
		ERR_print_errors_fp(stderr);
		goto error;
	}

	if (app_verify_device_cloud_certificate(ssl_ptr->ssl) != X509_V_OK) {
		log_error("%s", "app_ssl_connect(): error verifying Cloud Connector certificate");
		goto error;
	}

	ret = 0;

error:
	return ret;
}
#endif
