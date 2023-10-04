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
#include <unistd.h>
#include "ccapi/ccapi.h"

#define DEVICE_TYPE             "Device type"
#define DEVICE_CLOUD_URL        "devicecloud.digi.com"
#define VENDOR_ID               0x030000DB

const uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF};
const uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01};

static ccapi_firmware_target_t firmware_list[] = {
       /* version   description    filespec             maximum_size       chunk_size */
        {{1,0,0,0}, "Bootloader",  ".*\\.[bB][iI][nN]", 1 * 1024 * 1024,   1 * 1024 },  /* any *.bin files */
        {{0,0,1,0}, "Kernel",      ".*\\.a",            128 * 1024 * 1024, 128 * 1024 }   /* any *.a files */
    };
static uint8_t firmware_count = (sizeof(firmware_list)/sizeof(firmware_list[0]));

static ccapi_bool_t stop = CCAPI_FALSE;

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

FILE * fp = NULL;

static ccapi_fw_request_error_t app_fw_request_cb(unsigned int const target, char const * const filename, size_t const total_size)
{
    (void)target;
    (void)total_size;

    fp = fopen(filename, "wb+");
    if (fp == NULL)
    {
        printf("Unable to create %s file\n", filename );
        return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
    }

    return CCAPI_FW_REQUEST_ERROR_NONE;
}

static ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    size_t const max_bytes_to_print = 4;
    size_t const bytes_to_print = (size > max_bytes_to_print) ? max_bytes_to_print : size;
    unsigned long const prog_time_ms = firmware_list[target].chunk_size / 1024 * 1000 * 1000 / 32;  /* Simulate a 32 KByte/second flash programming time */
    size_t i;

    printf("app_fw_data_cb: offset = 0x%" PRIx32 "\n", offset);

    if (offset == 0)
        printf("prog_time_ms=%ld\n", prog_time_ms);

    printf("data = ");
    for (i=0; i < bytes_to_print; i++)
    {
        printf("0x%02X ", ((uint8_t*)data)[i]);
    }
    printf("...\n");

    printf("length = %" PRIsize " last_chunk=%d\n", size, last_chunk);

    fwrite(data, 1, size, fp);
    
    usleep(prog_time_ms);

    if (last_chunk)
	{
        if (fp != NULL)
            fclose(fp);
    }

    printf("done\n");
    return CCAPI_FW_DATA_ERROR_NONE;
}

static void app_fw_cancel_cb(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason)
{

    printf("app_fw_cancel_cb for target='%d'. cancel_reason='%d'\n", target, cancel_reason);

    if (fp != NULL)
        fclose(fp);

    return;
}

static void app_fw_reset_cb(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * version)
{
    printf("app_fw_reset_cb for target='%d'. Current version='%d.%d.%d.%d'\n", target, version->major, version->minor, version->revision, version->build);

    switch (target)
    {
        case 0:
        {
            version->build++;

            *system_reset = CCAPI_FALSE;

            stop = CCAPI_TRUE;

            break;
        }
        case 1:
        {
            *system_reset = CCAPI_TRUE;

            break;
        }
    }
}

static ccapi_start_error_t app_start_ccapi(void)
{
    ccapi_start_t start = {0};
    ccapi_fw_service_t fw_service = {
                                        {
                                            firmware_count,
                                            firmware_list
                                        }, 
                                        {
                                            app_fw_request_cb, 
                                            app_fw_data_cb, 
                                            app_fw_cancel_cb,
                                            app_fw_reset_cb
                                        }
                                    };

    ccapi_start_error_t start_error;

    get_device_id_from_mac(start.device_id, mac);
    start.vendor_id = VENDOR_ID;
    start.device_cloud_url = DEVICE_CLOUD_URL;
    start.device_type = DEVICE_TYPE;

    start.service.firmware = &fw_service;

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

static ccapi_tcp_start_error_t app_start_tcp_transport(void)
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

static ccapi_bool_t check_stop(void)
{
    ccapi_stop_error_t stop_error = CCAPI_STOP_ERROR_NONE;

    if (stop == CCAPI_TRUE)
    {
        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);

        if (stop_error == CCAPI_STOP_ERROR_NONE)
        {
            printf("ccapi_stop success\n");
        }
        else
        {
            printf("ccapi_stop error %d\n", stop_error);
        }
    }

    return stop;
}

int main (void)
{
    if (app_start_ccapi() != CCAPI_START_ERROR_NONE)
    {
        goto done;
    }

    if (app_start_tcp_transport() != CCAPI_TCP_START_ERROR_NONE)
    {
        goto done;
    }

    printf("Waiting for ever\n");
    do
    {
        sleep(1);
    } while (check_stop() != CCAPI_TRUE);

done:
    return 0;
}
