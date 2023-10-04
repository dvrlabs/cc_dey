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


void print_ccapi_start_error(ccapi_start_error_t error)
{
    
    printf("ccapi_start error: ");
    switch (error)
    {
        case CCAPI_START_ERROR_NONE:
            printf("CCAPI_START_ERROR_NONE");
            break;
        case CCAPI_START_ERROR_NULL_PARAMETER:
            printf("CCAPI_START_ERROR_NULL_PARAMETER");
            break;
        case CCAPI_START_ERROR_INVALID_VENDORID:
            printf("CCAPI_START_ERROR_INVALID_VENDORID");
            break;
        case CCAPI_START_ERROR_INVALID_DEVICEID:
            printf("CCAPI_START_ERROR_INVALID_DEVICEID");
            break;
        case CCAPI_START_ERROR_INVALID_URL:
            printf("CCAPI_START_ERROR_INVALID_URL");
            break;
        case CCAPI_START_ERROR_INVALID_DEVICETYPE:
            printf("CCAPI_START_ERROR_INVALID_DEVICETYPE");
            break;
        case CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK:
            printf("CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK");
            break;
        case CCAPI_START_ERROR_INVALID_FIRMWARE_INFO:
            printf("CCAPI_START_ERROR_INVALID_FIRMWARE_INFO");
            break;
        case CCAPI_START_ERROR_INVALID_FIRMWARE_DATA_CALLBACK:
            printf("CCAPI_START_ERROR_INVALID_FIRMWARE_DATA_CALLBACK");
            break;
        case CCAPI_START_ERROR_INSUFFICIENT_MEMORY:
            printf("CCAPI_START_ERROR_INSUFFICIENT_MEMORY");
            break;
        case CCAPI_START_ERROR_THREAD_FAILED:
            printf("CCAPI_START_ERROR_THREAD_FAILED");
            break;
        case CCAPI_START_ERROR_LOCK_FAILED:
            printf("CCAPI_START_ERROR_LOCK_FAILED");
            break;
        case CCAPI_START_ERROR_ALREADY_STARTED:
            printf("CCAPI_START_ERROR_ALREADY_STARTED");
            break;
    }

    printf("!!!!!\n");

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
    static ccapi_filesystem_service_t filesystem_service;

    /* Fill the connection callbacks for the transport */
    filesystem_service.access = ccapi_filesystem_access_callback;
    filesystem_service.changed = ccapi_filesystem_changed_callback;

    /* Set the Filesystem service */
    start->service.file_system = &filesystem_service;
}




/* Firmware functions -----------------------------*/
ccapi_fw_data_error_t ccapi_firmware_data_callback(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    size_t const max_bytes_to_print = 4;
    size_t const bytes_to_print = (size > max_bytes_to_print) ? max_bytes_to_print : size;
    size_t i;

    printf("ccapi_firmware_data_callback: offset = 0x%" PRIx32 "\n", offset);

    printf("target=%d ,data = ", target);
    for (i=0; i < bytes_to_print; i++)
    {
        printf("0x%02X ", ((uint8_t*)data)[i]);
    }
    printf("...\n");

    printf("length = %" PRIsize " last_chunk=%d\n", size, last_chunk);

    printf("done\n");
    return CCAPI_FW_DATA_ERROR_NONE;
}

void ccapi_firmware_cancel_callback(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason)
{

    printf("app_fw_cancel_cb for target='%d'. cancel_reason='%d'\n", target, cancel_reason);
    return;
}


void ccapi_firmware_reset_callback(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * version)
{
    printf("app_fw_reset_cb for target='%d'. Current version='%d.%d.%d.%d'\n", target, version->major, version->minor, version->revision, version->build);

    *system_reset = CCAPI_FALSE;

}



void fill_firmwareupdate_service(ccapi_start_t * start)
{
    static ccapi_firmware_target_t firmware_list[] = {
       /* version   description    filespec             maximum_size       chunk_size */
        {{1,0,0,0}, "Bootloader",  ".*\\.[bB][iI][nN]", 1 * 1024 * 1024,   1 * 1024 },  /* any *.bin files */
        {{0,0,1,0}, "Kernel",      ".*\\.a",            128 * 1024 * 1024, 128 * 1024 }   /* any *.a files */
    };

    static uint8_t firmware_count = (sizeof(firmware_list)/sizeof(firmware_list[0]));


    static ccapi_fw_service_t firmware_service;

    firmware_service.target.item = firmware_list;
    firmware_service.target.count = firmware_count;

    firmware_service.callback.request = NULL;
    firmware_service.callback.data = ccapi_firmware_data_callback;
    firmware_service.callback.cancel = ccapi_firmware_cancel_callback;
    firmware_service.callback.reset = ccapi_firmware_reset_callback;


    /* Set the Filesystem service */
    start->service.firmware = &firmware_service;

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
