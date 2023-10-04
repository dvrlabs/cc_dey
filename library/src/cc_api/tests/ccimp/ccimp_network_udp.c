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
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)

#ifdef UNIT_TEST
#define ccimp_network_udp_open       ccimp_network_udp_open_real
#define ccimp_network_udp_send       ccimp_network_udp_send_real
#define ccimp_network_udp_receive    ccimp_network_udp_receive_real
#define ccimp_network_udp_close      ccimp_network_udp_close_real
#endif

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "dns_helper.h"


static int udp_create_socket(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd >= 0)
    {
        int enabled = 1;

        if (ioctl(fd, FIONBIO, &enabled) < 0)
        {
            printf("ioctl: FIONBIO failed, errno %d\n", errno);
            close(fd);
            fd = -1;
        }
    }
    else
    {
        printf("Could not open udp socket, errno %d\n", errno);
    }

    return fd;
}

static ccimp_status_t udp_connect(int const fd, in_addr_t const ip_addr)
{

    struct sockaddr_in sin = {0};
    ccimp_status_t status = CCIMP_STATUS_OK;

    memcpy(&sin.sin_addr, &ip_addr, sizeof sin.sin_addr);
    sin.sin_port   = htons(CCIMP_UDP_PORT);
    sin.sin_family = AF_INET;

    printf("app_udp_connect: fd %d\n", fd);

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
            printf("udp_connect: connect() failed, fd %d, errno %d\n", fd, err);
            status = CCIMP_STATUS_ERROR;
        }
    }

    return status;
}

ccimp_status_t ccimp_network_udp_open(ccimp_network_open_t * const data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    in_addr_t ip_addr;
    static int fd = -1;

    data->handle =&fd;
    status= dns_resolve(data->device_cloud.url, &ip_addr);
    if (status!=CCIMP_STATUS_OK)
    {
        printf("Can't resolve DNS for %s\n",data->device_cloud.url);
        return status;
    }
    if (fd==-1)
    {
        fd=udp_create_socket();
        if (fd==-1)
        {
            return CCIMP_STATUS_ERROR;
        }
        status= udp_connect(fd, ip_addr);
        if ( status != CCIMP_STATUS_OK)
        {
            data->handle = NULL;
            return status;
        }
    }
    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_network_udp_send(ccimp_network_send_t * const data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int * const fd = data->handle;

    int ccode = write(*fd, data->buffer,data->bytes_available);
    if (ccode >= 0)
        data->bytes_used = (size_t)ccode;
    else
    {
        if (errno == EAGAIN)
        {
            status = CCIMP_STATUS_BUSY;
        }
        else
        {
            printf("ccimp_network_udp_send: send() failed, errno %d\n", errno);
            status = CCIMP_STATUS_ERROR;
        }
    }
    return status;
}

ccimp_status_t ccimp_network_udp_receive(ccimp_network_receive_t * const data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int * const fd = data->handle;

    int ccode = read(*fd, data->buffer, data->bytes_available);
    if (ccode < 0)
    {
        if (errno == EAGAIN)
        {
            status = CCIMP_STATUS_BUSY;
            goto done;
        }
        else
        {
            printf("ccimp_network_udp_receive: recv() failed, errno %d\n", errno);
            status = CCIMP_STATUS_ERROR;
            goto done;
        }
    }
    data->bytes_used = (size_t)ccode;
done:
    return status;
}

ccimp_status_t ccimp_network_udp_close(ccimp_network_close_t * const data)
{
    int * const fd = data->handle;
    ccimp_status_t status = CCIMP_STATUS_OK;

    if (close(*fd) < 0)
    {
        printf("An error occurred when closing socket. Errno: %d\n", errno);
        status = CCIMP_STATUS_ERROR;
    }
    *fd = -1;
    return status;
}

#endif
