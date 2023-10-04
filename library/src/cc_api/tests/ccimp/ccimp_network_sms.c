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
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)

#ifdef UNIT_TEST
#define ccimp_network_sms_open       ccimp_network_sms_open_real
#define ccimp_network_sms_send       ccimp_network_sms_send_real
#define ccimp_network_sms_receive    ccimp_network_sms_receive_real
#define ccimp_network_sms_close      ccimp_network_sms_close_real
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


#define GW_PORT		9999		    /* Port number is hardcoded in sms_proxy.py running in ConnectPort X4 Gateway */
#define GW_ADDR_IN  "10.80.1.213"   /* Gateway running sms_proxy.py in India office */
#define GW_ADDR_ES  "10.101.2.21"   /* Gateway running sms_proxy.py in Spain office */
#define GW_ADDR_US  "10.9.16.86"    /* Gateway running sms_proxy.py in US office */

#define GW_ADDR     GW_ADDR_US

static int sms_dns_resolve_name(char const * const domain_name, in_addr_t * const ip_addr)
{
    int ret = -1;
    struct addrinfo *res_list;
    struct addrinfo *res;

   {
        struct addrinfo hint = {0};
        int error;

        hint.ai_socktype = SOCK_STREAM;
        hint.ai_family   = AF_INET;
        error = getaddrinfo(domain_name, NULL, &hint, &res_list);
        if (error != 0)
        {
            printf("sms_dns_resolve_name: DNS resolution failed for [%s]\n", domain_name);
            goto done;
        }
    }

    /* loop over all returned results and look for a IPv4 address */
    for (res = res_list; res; res = res->ai_next)
    {
        if (res->ai_family == PF_INET)
        {
            struct sockaddr_in * const sa = (struct sockaddr_in *)((void *) res->ai_addr);
            struct in_addr const ipv4_addr = sa->sin_addr;

            *ip_addr = ipv4_addr.s_addr;
            printf("sms_dns_resolve_name: ip address = [%s]\n", inet_ntoa(ipv4_addr));
            ret = 0;
            break;
        }
    }

    freeaddrinfo(res_list);

done:
    return ret;
}


static int sms_create_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int buffer_size;

    if (fd >= 0)
    {
        int enabled = 1;

        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)) < 0)
        {
        	printf("open_socket: setsockopt SO_KEEPALIVE failed, errno %d\n", errno);
        }

        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled)) < 0)
        {
        	printf("open_socket: setsockopt TCP_NODELAY failed, errno %d\n", errno);
        }

        /* Adjust Send and Receive buffer size to SMS message max characters */
        buffer_size = 160;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0)
        {
        	printf("open_socket: setsockopt SO_SNDBUF failed, errno %d\n", errno);
        }

        buffer_size = 160;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0)
        {
        	printf("open_socket: setsockopt SO_SNDBUF failed, errno %d\n", errno);
        }

        if (ioctl(fd, FIONBIO, &enabled) < 0)
        {
            printf("ioctl: FIONBIO failed, errno %d\n", errno);
            close(fd);
            fd = -1;
        }
    }
    else
    {
    	printf("Could not open sms socket, errno %d\n", errno);
    }

    return fd;
}

static ccimp_status_t sms_connect(int const fd, in_addr_t const ip_addr)
{

    struct sockaddr_in sin = {0};
    ccimp_status_t status = CCIMP_STATUS_OK;

    memcpy(&sin.sin_addr, &ip_addr, sizeof sin.sin_addr);
    sin.sin_port   = htons(GW_PORT);
    sin.sin_family = AF_INET;

    printf("sms_connect: fd %d\n", fd);


    if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        int const err = errno;
        switch (err)
        {
        case EINTR:
        case EAGAIN:
        case EINPROGRESS:
            status = CCIMP_STATUS_BUSY;
            break;

        default:
            printf("sms_connect: connect() failed, fd %d, errno %d\n", fd, err);
            status = CCIMP_STATUS_ERROR;
        }
    }

    return status;
}

static ccimp_status_t sms_connect_complete(int const fd)
{
	ccimp_status_t status = CCIMP_STATUS_BUSY;
    struct timeval timeout = {0};
    fd_set read_set, write_set;
    int rc;

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);
    write_set = read_set;

    rc = select(fd+1, &read_set, &write_set, NULL, &timeout);
    if (rc < 0)
    {
        if (errno != EINTR) {
            printf("sms_connect_complete: select on fd %d returned %d, errno %d\n", fd, rc, errno);
            status = CCIMP_STATUS_ERROR;
        }
    }
    else
    /* Check whether the socket is now writable (connection succeeded). */
    if (rc > 0 && FD_ISSET(fd, &write_set))
    {
        /* We expect "socket writable" when the connection succeeds. */
        /* If we also got a "socket readable" we have an error. */
        if (FD_ISSET(fd, &read_set))
        {
            printf("connect_complete: FD_ISSET for read, fd %d\n", fd);
            status = CCIMP_STATUS_ERROR;

        }
        else
        {
            status = CCIMP_STATUS_OK;
        }
    }
    return status;
}

static ccimp_status_t config_server_phone_number(int const fd, const char * device_cloud_phone)
{
	ccimp_status_t status;
	static char const phone_number_prefix[] = "phone-number=";
	char *str_to_send = NULL;
	int ccode;

	if (strlen(device_cloud_phone) == 0)
	{
		printf("config_server_phone_number: device_cloud_phone not yet configured\n");
		return CCIMP_STATUS_ERROR;
	}

	str_to_send = malloc(sizeof(phone_number_prefix)+strlen(device_cloud_phone));

	if (str_to_send == NULL)
	{
		printf("config_server_phone_number: malloc failed\n");
		return CCIMP_STATUS_ERROR;
	}

	strcpy(str_to_send,phone_number_prefix);

	strcat(str_to_send, device_cloud_phone);

	printf("config_server_phone_number: %s\n", str_to_send);

	ccode = write(fd, str_to_send, strlen(str_to_send));
    if (ccode >= 0)
    	status = CCIMP_STATUS_OK;
    else
    	status = CCIMP_STATUS_ERROR;

    free (str_to_send);

    usleep(1000000);	/* Let the proxy digest previous command */

	return(status);
}

ccimp_status_t ccimp_network_sms_open(ccimp_network_open_t * const data)
{
#define CONNECT_TIMEOUT 30

    static ccimp_os_system_up_time_t connect_time;
    int * pfd = NULL;
    struct sockaddr_in interface_addr;
    socklen_t interface_addr_len;

    ccimp_status_t status = CCIMP_STATUS_ERROR;

    if (data->handle == NULL)
    {
        pfd = (int *)malloc(sizeof(int));
        *pfd = -1;
        data->handle = pfd;
    }
    else
    {
        pfd = data->handle;
    }

    if (*pfd == -1)
    {
        in_addr_t ip_addr;

        status = sms_dns_resolve_name(GW_ADDR, &ip_addr);
        if (status != CCIMP_STATUS_OK)
        {
            printf("ccimp_network_sms_open: Can't resolve DNS for %s\n", GW_ADDR);
            goto done;
        }

        *pfd = sms_create_socket();
        if (*pfd == -1)
        {
            status = CCIMP_STATUS_ERROR;
            goto done;
        }

        ccimp_os_get_system_time(&connect_time);
        status = sms_connect(*pfd, ip_addr);
        if (status != CCIMP_STATUS_OK)
            goto error;
    }

    /* Get socket info of connected interface */
    interface_addr_len = sizeof(interface_addr);
    if (getsockname(*pfd, (struct sockaddr *)&interface_addr, &interface_addr_len))
    {
        printf("network_connect: getsockname error, errno %d\n", errno);
        goto done;
    }

    status = sms_connect_complete(*pfd);
    if (status == CCIMP_STATUS_OK)
    {
         printf("ccimp_network_sms_open: connected to %s\n", GW_ADDR);

         status = config_server_phone_number(*pfd, data->device_cloud.phone);

         goto done;
    }

error:
    if (status == CCIMP_STATUS_BUSY)
    {
    	ccimp_os_system_up_time_t elapsed_time;

        ccimp_os_get_system_time(&elapsed_time);
        elapsed_time.sys_uptime -= connect_time.sys_uptime;

        if (elapsed_time.sys_uptime >= CONNECT_TIMEOUT)
        {
            printf("ccimp_network_sms_open: failed to connect withing 30 seconds\n");
            status = CCIMP_STATUS_ERROR;
        }
    }

    if (status == CCIMP_STATUS_ERROR)
    {
        printf("ccimp_network_sms_open: failed to connect to %s\n", GW_ADDR);

        if (pfd != NULL)
        {
            if (*pfd >= 0)
            {
                close(*pfd);
                *pfd = -1;
            }
            free(pfd);
        }
    }

done:
    return status;
}

ccimp_status_t ccimp_network_sms_send(ccimp_network_send_t * const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
    int * const fd = data->handle;
    static ccimp_os_system_up_time_t last_message_time = { 0 };
    int ccode;
    ccimp_os_system_up_time_t present_time;

    if (last_message_time.sys_uptime != 0)
    {
    	ccimp_os_get_system_time(&present_time);
        if (last_message_time.sys_uptime == present_time.sys_uptime)
        {
            return CCIMP_STATUS_BUSY;
        }
    }

    ccode= write(*fd, data->buffer,data->bytes_available);
    if (ccode >= 0)
    {
        data->bytes_used = (size_t)ccode;

        ccimp_os_get_system_time(&last_message_time);

        printf("ccimp_network_sms_send: send %d bytes\n", ccode);
        if ((data->bytes_available == 160) && (((char*)data->buffer)[159] == '@'))
            printf("ccimp_network_sms_send: Sending '@' !!!!!\n");
    }
    else
    {
        int const err = errno;
        if (err == EAGAIN)
        {
            status = CCIMP_STATUS_BUSY;
        }
        else
        {
            status = CCIMP_STATUS_ERROR;
            printf("ccimp_network_sms_send: send() failed, errno %d\n", err);
        }
    }

    return status;
}

ccimp_status_t ccimp_network_sms_receive(ccimp_network_receive_t * const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;

    int * const fd = data->handle;

    int ccode = read(*fd, data->buffer, data->bytes_available);
    if (ccode > 0)
    {
        data->bytes_used = (size_t)ccode;
        printf("ccimp_network_sms_receive: recv %d bytes\n", ccode);
    }
    else
    if (ccode == 0)
    {
        /* EOF on input: the connection was closed. */
        printf("network_receive: EOF on socket\n");
        errno = ECONNRESET;
        status = CCIMP_STATUS_ERROR;
    }
    else
    {
        int const err = errno;
        /* An error of some sort occurred: handle it appropriately. */
        if (err == EAGAIN)
        {
            status = CCIMP_STATUS_BUSY;
        }
        else
        {
            printf("ccimp_network_sms_receive: recv() failed, errno %d\n", err);
            /* if not timeout (no data) return an error */
            status = CCIMP_STATUS_ERROR;
        }
    }

    return status;
}

ccimp_status_t ccimp_network_sms_close(ccimp_network_close_t * const data)
{
	ccimp_status_t status = CCIMP_STATUS_OK;
    int * const fd = data->handle;

    if (close(*fd) < 0)
    {
        printf("network_sms_close: close() failed, fd %d, errno %d\n", *fd, errno);
        status = CCIMP_STATUS_ERROR;
    }
    else
       printf("network_sms_close: fd %d\n", *fd);

    free(fd);
    /* Let the proxy close the socket and listen for a new one.
       Consider that it has a 0 backlog (handles 1 single connection)
     */
    usleep(1000000);
    return status;
}


#endif
