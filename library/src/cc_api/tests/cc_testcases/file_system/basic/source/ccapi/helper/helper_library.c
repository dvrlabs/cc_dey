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

#include <stdio.h>
#include "ccapi/ccapi.h"
#include "helper_library.h"


/* Basic Initialization functions ------------------------ */
void fill_device_settings(ccapi_start_t * start)
{
    /* Configure settings */
    uint8_t device_id[] = DEVICE_ID;
    char const * const device_cloud_url = DEVICE_CLOUD_URL_STRING;
    char const * const device_type = DEVICE_TYPE_STRING;
    start->vendor_id = DEVICE_CLOUD_VENDOR_ID;
    memcpy(start->device_id, device_id, sizeof start->device_id);
    start->device_cloud_url = device_cloud_url;
    start->device_type = device_type;

    /* Configure services */
    start->service.cli = NULL;
    start->service.sm = NULL;
    start->service.receive = NULL;
    start->service.file_system = NULL;
    start->service.firmware = NULL;
    start->service.rci = NULL;
}





/* Filesystem functions -----------------------------*/
ccapi_fs_access_t ccapi_filesystem_access_callback(char const * const local_path, ccapi_fs_request_t const request)
{
    #define INIT_CSTR(str)  { str, sizeof str - 1 }
    typedef struct { char const * str; size_t chars; } cstr_t;

    static const cstr_t stay_out = INIT_CSTR("/etc");

    printf("  %s path %s request ", __FUNCTION__, local_path);
    switch (request)
    {
        case CCAPI_FS_REQUEST_LIST:
            printf("CCAPI_FILESYSTEM_REQUEST_LIST");
            break;
        case CCAPI_FS_REQUEST_READ:
            printf("CCAPI_FILESYSTEM_REQUEST_READ");
            break;
        case CCAPI_FS_REQUEST_READWRITE:
            printf("CCAPI_FILESYSTEM_REQUEST_READWRITE");
            break;
        case CCAPI_FS_REQUEST_WRITE:
            printf("CCAPI_FILESYSTEM_REQUEST_WRITE");
            break;
        case CCAPI_FS_REQUEST_REMOVE:
            printf("CCAPI_FILESYSTEM_REQUEST_REMOVE");
            break;
        case CCAPI_FS_REQUEST_UNKNOWN:
            printf("CCAPI_FILESYSTEM_REQUEST_UNKNOWN");
            break;
    }

    if (strncmp(local_path, stay_out.str, stay_out.chars) == 0)
    {
        printf(" CCAPI_FILESYSTEM_ACCESS_DENY\n");
        return CCAPI_FS_ACCESS_DENY;
    }
    else
    {
        printf(" CCAPI_FILESYSTEM_ACCESS_ALLOW\n");
        return CCAPI_FS_ACCESS_ALLOW;
    }
}

void ccapi_filesystem_changed_callback(char const * const local_path, ccapi_fs_changed_t const request)
{
    printf("  %s path %s changed ", __FUNCTION__, local_path);
    switch (request)
    {
        case CCAPI_FS_CHANGED_MODIFIED:
            printf("CCAPI_FILESYSTEM_CHANGED_MODIFIED");
            break;
        case CCAPI_FS_CHANGED_REMOVED:
            printf("CCAPI_FILESYSTEM_CHANGED_REMOVED");
            break;
    }
    printf("\n");
}


void fill_filesystem_service(ccapi_start_t * start)
{
    ccapi_filesystem_service_t filesystem_service;

    /* Fill the connection callbacks for the transport */
    filesystem_service.access = ccapi_filesystem_access_callback;
    filesystem_service.changed = ccapi_filesystem_changed_callback;

    /* Set the Filesystem service */
    start->service.file_system = &filesystem_service;
}





/* TCP Transport functions ------------------------ */
ccapi_bool_t ccapi_tcp_close_callback(ccapi_tcp_close_cause_t cause)
{
    ccapi_bool_t reconnect;
    switch (cause)
    {
        case CCAPI_TCP_CLOSE_DISCONNECTED:
            printf("ccapi_tcp_close_callback cause CCAPI_TCP_CLOSE_DISCONNECTED\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_REDIRECTED:
            printf("ccapi_tcp_close_callback cause CCAPI_TCP_CLOSE_REDIRECTED\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_NO_KEEPALIVE:
            printf("ccapi_tcp_close_callback cause CCAPI_TCP_CLOSE_NO_KEEPALIVE\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_DATA_ERROR:
            printf("ccapi_tcp_close_callback cause CCAPI_TCP_CLOSE_DATA_ERROR\n");
            reconnect = CCAPI_TRUE;
            break;
    }
    return reconnect;
}


void fill_tcp_transport_settings(ccapi_tcp_info_t * tcp_info)
{

    uint8_t ipv4[] = DEVICE_IP;
    uint8_t mac[] = DEVICE_MAC_ADDRESS;

    /* Fill the TCP transport settings */
    tcp_info->connection.type = CCAPI_CONNECTION_LAN;
    memcpy(tcp_info->connection.ip.address.ipv4, ipv4, sizeof tcp_info->connection.ip.address.ipv4);
    memcpy(tcp_info->connection.info.lan.mac_address, mac, sizeof tcp_info->connection.info.lan.mac_address);

    /* Fill the TCP keepalive info */
    tcp_info->keepalives.tx = 75;
    tcp_info->keepalives.rx = 30;
    tcp_info->keepalives.wait_count = 2;

    /* Fill the connection callbacks for the transport */
    tcp_info->callback.close = ccapi_tcp_close_callback;
    tcp_info->callback.keepalive = NULL;

}
