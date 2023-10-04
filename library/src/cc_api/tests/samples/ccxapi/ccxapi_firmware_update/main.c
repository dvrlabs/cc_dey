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
#include "ccapi/ccxapi.h"

#define DEVICE_TYPE_STRING      "Device type"
#define DEVICE_CLOUD_URL_STRING "devicecloud.digi.com"

static ccapi_firmware_target_t firmware_list_devA[] = {
       /* version   description    filespec             maximum_size       chunk_size */
        {{1,0,0,0}, "Bootloader",  ".*\\.[bB][iI][nN]", 1 * 1024 * 1024,   1 * 1024 },  /* any *.bin files */
    };
static uint8_t firmware_count_devA = (sizeof(firmware_list_devA)/sizeof(firmware_list_devA[0]));

static ccapi_firmware_target_t firmware_list_devB[] = {
       /* version   description    filespec             maximum_size       chunk_size */
        {{0,0,1,0}, "Kernel",      ".*\\.a",            128 * 1024 * 1024, 128 * 1024 }   /* any *.a files */
    };
static uint8_t firmware_count_devB = (sizeof(firmware_list_devB)/sizeof(firmware_list_devB[0]));

FILE * fp_devA = NULL;

static ccapi_fw_request_error_t app_fw_request_cb_devA(unsigned int const target, char const * const filename, size_t const total_size)
{
    (void)target;
    (void)total_size;

    fp_devA = fopen(filename, "wb+");
    if (fp_devA == NULL)
    {
        printf("Unable to create %s file\n", filename );
        return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
    }

    return CCAPI_FW_REQUEST_ERROR_NONE;
}

static ccapi_fw_data_error_t app_fw_data_cb_devA(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    unsigned long const prog_time_ms = firmware_list_devA[target].chunk_size / 1024 * 1000 * 1000 / 32;  /* Simulate a 32 KByte/second flash programming time */

    printf("app_fw_data_cb_devA: offset= 0x%08" PRIx32 " length= %" PRIsize " last_chunk= %d\n", offset, size, last_chunk);

    fwrite(data, 1, size, fp_devA);
    
    usleep(prog_time_ms);

    if (last_chunk)
	{
        if (fp_devA != NULL)
            fclose(fp_devA);
    }

    return CCAPI_FW_DATA_ERROR_NONE;
}

FILE * fp_devB = NULL;

static ccapi_fw_request_error_t app_fw_request_cb_devB(unsigned int const target, char const * const filename, size_t const total_size)
{
    (void)target;
    (void)total_size;

    fp_devB = fopen(filename, "wb+");
    if (fp_devB == NULL)
    {
        printf("Unable to create %s file\n", filename );
        return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
    }

    return CCAPI_FW_REQUEST_ERROR_NONE;
}

static ccapi_fw_data_error_t app_fw_data_cb_devB(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    unsigned long const prog_time_ms = firmware_list_devB[target].chunk_size / 1024 * 1000 * 1000 / 32;  /* Simulate a 32 KByte/second flash programming time */

    printf("app_fw_data_cb_devB: offset= 0x%" PRIx32 "length= %" PRIsize " last_chunk= %d\n", offset, size, last_chunk);

    fwrite(data, 1, size, fp_devB);
    
    usleep(prog_time_ms);

    if (last_chunk)
	{
        if (fp_devB != NULL)
            fclose(fp_devB);
    }

    return CCAPI_FW_DATA_ERROR_NONE;
}

void fill_start_structure_with_good_parameters(ccapi_start_t * start)
{
    char const * const device_cloud_url = DEVICE_CLOUD_URL_STRING;
    char const * const device_type = DEVICE_TYPE_STRING;
    start->vendor_id = 0x0300009F; /* Set vendor_id or ccapi_init_error_invalid_vendorid will be returned instead */
    start->device_cloud_url = device_cloud_url;
    start->device_type = device_type;

    start->service.cli = NULL;
    start->service.sm = NULL;
    start->service.receive = NULL;
    start->service.file_system = NULL;
    start->service.firmware = NULL;
    start->service.rci = NULL;
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

int main (void)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t start_error;
    ccapi_tcp_start_error_t tcp_start_error;
    ccapi_tcp_info_t tcp_info = {{0}};

    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */

    ccapi_handle_t ccapi_handle_devA = NULL;
    uint8_t device_id_devA[] = {0x00, 0x08, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x9F, 0xEC, 0x46, 0x19, 0x0E}; /* Auto-provisioned */
    ccapi_fw_service_t fw_service_devA = {
                                        { firmware_count_devA, firmware_list_devA }, 
                                        { app_fw_request_cb_devA, app_fw_data_cb_devA, NULL, NULL }
                                    };

    ccapi_handle_t ccapi_handle_devB = NULL;
    uint8_t device_id_devB[] = {0x00, 0x08, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x9F, 0x78, 0x44, 0x33, 0x37}; /* Auto-provisioned */
    ccapi_fw_service_t fw_service_devB = {
                                        { firmware_count_devB, firmware_list_devB }, 
                                        { app_fw_request_cb_devB, app_fw_data_cb_devB, NULL, NULL }
                                    };

    fill_start_structure_with_good_parameters(&start);
    start.service.firmware = &fw_service_devA;

    /* Start devA */
    memcpy(start.device_id, device_id_devA, sizeof start.device_id);

    start_error = ccxapi_start(&ccapi_handle_devA, &start);
    if (start_error == CCAPI_START_ERROR_NONE)
    {
        printf("ccxapi_start success for devA (handle=%p)\n", (void *)ccapi_handle_devA);
    }
    else
    {
        printf("ccxapi_start error %d for devA\n", start_error);
        goto done;
    }

    /* Start devB */
    memcpy(start.device_id, device_id_devB, sizeof start.device_id);
    start.service.firmware = &fw_service_devB;

    start_error = ccxapi_start(&ccapi_handle_devB, &start);
    if (start_error == CCAPI_START_ERROR_NONE)
    {
        printf("ccxapi_start success for devB (handle=%p)\n", (void *)ccapi_handle_devB);
    }
    else
    {
        printf("ccxapi_start error %d for devB\n", start_error);
        goto done;
    }

    tcp_info.connection.type = CCAPI_CONNECTION_LAN;
    memcpy(tcp_info.connection.ip.address.ipv4, ipv4, sizeof tcp_info.connection.ip.address.ipv4);
    memcpy(tcp_info.connection.info.lan.mac_address, mac, sizeof tcp_info.connection.info.lan.mac_address);

    tcp_info.callback.close = ccapi_tcp_close_cb;
    tcp_info.callback.keepalive = NULL;

    /* Start devA tcp transport*/
    tcp_start_error = ccxapi_start_transport_tcp(ccapi_handle_devA, &tcp_info);
    if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
    {
        printf("ccxapi_start_transport_tcp success for handle %p \n", (void *)ccapi_handle_devA);
    }
    else
    {
        printf("ccxapi_start_transport_tcp failed with error %d for handle %p\n", tcp_start_error, (void *)ccapi_handle_devA);
        goto done;
    }

    /* Start devB tcp transport*/
    tcp_start_error = ccxapi_start_transport_tcp(ccapi_handle_devB, &tcp_info);
    if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
    {
        printf("ccapi_start_transport_tcp success for handle %p \n", (void *)ccapi_handle_devB);
    }
    else
    {
        printf("ccapi_start_transport_tcp failed with error %d for handle %p\n", tcp_start_error, (void *)ccapi_handle_devB);
        goto done;
    }

    printf("Waiting for ever\n");
    for(;;)
        sleep(10);

done:
    return 0;
}
