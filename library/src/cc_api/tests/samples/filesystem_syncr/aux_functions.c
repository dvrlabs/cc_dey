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

#define DEVICE_TYPE             "Device type"
#define DEVICE_CLOUD_URL        "devicecloud.digi.com"
#define VENDOR_ID               0x030000DB

const uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF};
const uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01};

static void get_device_id_from_mac(uint8_t * const device_id, uint8_t const * const mac_addr)
{
    memset(device_id, 0x00, 16);
    device_id[8] = mac_addr[0];
    device_id[9] = mac_addr[1];
    device_id[10] = mac_addr[2];
    device_id[11] = 0xFF;
    device_id[12] = 0xFF;
    device_id[13] = mac_addr[3];
    device_id[14] = mac_addr[4];
    device_id[15] = mac_addr[5];
}

#define INIT_CSTR(str)  { str, sizeof str - 1 }
typedef struct { char const * str; size_t chars; } cstr_t;

static ccapi_bool_t ccapi_tcp_close_cb(ccapi_tcp_close_cause_t cause)
{
    ccapi_bool_t reconnect;
    switch (cause)
    {
        case CCAPI_TCP_CLOSE_DISCONNECTED:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_DISCONNECTED\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_REDIRECTED:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_REDIRECTED\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_NO_KEEPALIVE:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_NO_KEEPALIVE\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_DATA_ERROR:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_DATA_ERROR\n");
            reconnect = CCAPI_TRUE;
            break;
    }
    return reconnect;
}

static ccapi_fs_access_t ccapi_fs_access_cb(char const * const local_path, ccapi_fs_request_t const request)
{
    static const cstr_t stay_out = INIT_CSTR("/etc");

    printf("  %s path %s request ", __FUNCTION__, local_path);
    switch (request)
    {
        case CCAPI_FS_REQUEST_LIST:
            printf("CCAPI_FS_REQUEST_LIST");
            break;
        case CCAPI_FS_REQUEST_READ:
            printf("CCAPI_FS_REQUEST_READ");
            break;
        case CCAPI_FS_REQUEST_READWRITE:
            printf("CCAPI_FS_REQUEST_READWRITE");
            break;
        case CCAPI_FS_REQUEST_WRITE:
            printf("CCAPI_FS_REQUEST_WRITE");
            break;
        case CCAPI_FS_REQUEST_REMOVE:
            printf("CCAPI_FS_REQUEST_REMOVE");
            break;
        case CCAPI_FS_REQUEST_UNKNOWN:
            printf("CCAPI_FS_REQUEST_UNKNOWN");
            break;
    }

    if (strncmp(local_path, stay_out.str, stay_out.chars) == 0)
    {
        printf(" CCAPI_FS_ACCESS_DENY\n");
        return CCAPI_FS_ACCESS_DENY;
    }
    else
    {
        printf(" CCAPI_FS_ACCESS_ALLOW\n");
        return CCAPI_FS_ACCESS_ALLOW;
    }
}

static void ccapi_fs_changed_cb(char const * const local_path, ccapi_fs_changed_t const request)
{
    printf("  %s path %s changed ", __FUNCTION__, local_path);
    switch (request)
    {
        case CCAPI_FS_CHANGED_MODIFIED:
            printf("CCAPI_FS_CHANGED_MODIFIED");
            break;
        case CCAPI_FS_CHANGED_REMOVED:
            printf("CCAPI_FS_CHANGED_REMOVED");
            break;
    }
    printf("\n");
}

ccapi_start_error_t app_start_ccapi(void)
{
    ccapi_start_t start = {0};
    ccapi_filesystem_service_t fs_service;
    ccapi_start_error_t start_error;

    get_device_id_from_mac(start.device_id, mac);
    start.vendor_id = VENDOR_ID;
    start.device_cloud_url = DEVICE_CLOUD_URL;
    start.device_type = DEVICE_TYPE;

    fs_service.access = ccapi_fs_access_cb;
    fs_service.changed = ccapi_fs_changed_cb;
    start.service.file_system = &fs_service;
    start_error = ccapi_start(&start);
    if (start_error == CCAPI_START_ERROR_NONE)
    {
        printf("ccapi_start success\n");
    }
    else
    {
        printf("ccapi_start error %d\n", start_error);
        goto done;
    }

done:
    return start_error;
}

ccapi_tcp_start_error_t app_start_tcp_transport(void)
{
    ccapi_tcp_start_error_t tcp_start_error;
    ccapi_tcp_info_t tcp_info = {{0}};

    tcp_info.connection.type = CCAPI_CONNECTION_LAN;
    memcpy(tcp_info.connection.ip.address.ipv4, ipv4, sizeof tcp_info.connection.ip.address.ipv4);
    memcpy(tcp_info.connection.info.lan.mac_address, mac, sizeof tcp_info.connection.info.lan.mac_address);

    tcp_info.callback.close = ccapi_tcp_close_cb;
    tcp_info.callback.keepalive = NULL;
    tcp_start_error = ccapi_start_transport_tcp(&tcp_info);
    if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
    {
        printf("ccapi_start_transport_tcp success\n");
    }
    else
    {
        printf("ccapi_start_transport_tcp failed with error %d\n", tcp_start_error);
        goto done;
    }
done:
    return tcp_start_error;
}
