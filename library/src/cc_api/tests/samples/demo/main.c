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
#include <time.h>
#include "ccapi/ccapi.h"
#include "device_id_utils.h"
#include "simulated_tank.h"

#if !(defined UNUSED_ARGUMENT)
#define UNUSED_ARGUMENT(a)  (void)(a)
#endif

#define DEVICE_TYPE             "Simulated Tank (CCAPI)"
#define DEVICE_CLOUD_URL        "devicecloud.digi.com"
#define IPV4_ADDRESS_STRING     "192.168.1.2"

//#error "Define your Device MAC Address and your account's Vendor ID and comment this error line"
#define MAC_ADDRESS_STRING      "00049D:ABCDFF"
#define VENDOR_ID               0x030000BD

#define SAMPLE_DATA_PATH        "test/ccapi_tank_demo.txt"
#define SAMPLE_DATA_CONTENT     "CCAPI tank simulation sample file"

#define DATA_POINTS_PER_UPLOAD  10
#define SECONDS_BETWEEN_DPS     1

static void tank_valves_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    char const * valveIN_token = NULL;
    char const * valveOUT_token = NULL;
    size_t const buffer_size = 64;

    response_buffer_info->buffer = malloc(buffer_size);

    assert(transport == CCAPI_TRANSPORT_TCP);

    printf("\t *** Received '%s' target from Device Cloud\n", target);
    printf("\tAllocated response buffer at %p\n", response_buffer_info->buffer);

    valveIN_token = strstr(request_buffer_info->buffer, "valveIN=");
    valveOUT_token = strstr(request_buffer_info->buffer, "valveOUT=");

    if (valveIN_token != NULL)
    {
        char const * const value = valveIN_token + strlen("valveIN=");

        if (*value == '0')
        {
            set_valveIN_status(VALVE_OPENED);
            response_buffer_info->length = snprintf(response_buffer_info->buffer, buffer_size, "ValveIN: Opened");
        }
        else if (*value == '1')
        {
            set_valveIN_status(VALVE_CLOSED);
            response_buffer_info->length = snprintf(response_buffer_info->buffer, buffer_size, "ValveIN: Closed");
        }
        else
        {
            response_buffer_info->length = snprintf(response_buffer_info->buffer, buffer_size, "valveIN: Invalid value, status not changed!!!");
        }
    }

    if (valveOUT_token != NULL)
    {
        char const * const value = valveOUT_token + strlen("valveOUT=");

        if (*value == '0')
        {
            set_valveOUT_status(VALVE_OPENED);
            response_buffer_info->length = snprintf(response_buffer_info->buffer, buffer_size, "ValveOUT: Opened");
        }
        else if (*value == '1')
        {
            set_valveOUT_status(VALVE_CLOSED);
            response_buffer_info->length = snprintf(response_buffer_info->buffer, buffer_size, "ValveOUT: Closed");
        }
        else
        {
            response_buffer_info->length = snprintf(response_buffer_info->buffer, buffer_size, "valveOUT: Invalid value, status not changed!!!");
        }
    }

    printf("\tSending response: %s\n", (char *)response_buffer_info->buffer);

    return;
}

static void tank_valves_status_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error)
{
    UNUSED_ARGUMENT(target);
    UNUSED_ARGUMENT(transport);
    UNUSED_ARGUMENT(receive_error);

    printf("\t *** Received receive service status callback\n");

    if (response_buffer_info != NULL)
    {
        printf("\tFreeing response buffer at %p\n", response_buffer_info->buffer);
        free(response_buffer_info->buffer);
    }
}

static void tank_demo_loop(void)
{
    ccapi_dp_collection_handle_t dp_collection;
    ccapi_timestamp_t timestamp;

    timestamp.epoch.milliseconds = 0;

    ccapi_dp_create_collection(&dp_collection);
    ccapi_dp_add_data_stream_to_collection(dp_collection, "tank/temperature", "double ts_epoch");
    ccapi_dp_add_data_stream_to_collection(dp_collection, "tank/level", "int32 ts_epoch");
    ccapi_dp_add_data_stream_to_collection(dp_collection, "tank/valveIN", "int32");
    ccapi_dp_add_data_stream_to_collection(dp_collection, "tank/valveOUT", "int32");

    for(;;)
    {
        int i;

        for (i = 0; i < DATA_POINTS_PER_UPLOAD; i++)
        {
            timestamp.epoch.seconds = time(NULL);

            update_simulated_tank_status();

            ccapi_dp_add(dp_collection, "tank/temperature", get_tank_temperature(), &timestamp);
            ccapi_dp_add(dp_collection, "tank/level", get_tank_fill_percentage(), &timestamp);
            ccapi_dp_add(dp_collection, "tank/valveIN", get_valveIN_status());
            ccapi_dp_add(dp_collection, "tank/valveOUT", get_valveOUT_status());

            sleep(SECONDS_BETWEEN_DPS);
        }

        printf("\tSending Data Point collection\n");
        print_tank_status();

        ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    }
}

int main (void)
{
    uint8_t mac_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t ipv4[] = {0x00, 0x00, 0x00, 0x00};
    ccapi_filesystem_service_t filesystem_service;
    ccapi_receive_service_t receive_service;
    ccapi_start_t start = {0};
    ccapi_start_error_t start_error;
    ccapi_tcp_start_error_t tcp_start_error;
    ccapi_tcp_info_t tcp_info = {{0}};
    ccapi_receive_error_t receive_error;

    get_mac_from_string(mac_address, MAC_ADDRESS_STRING);
    get_ipv4_from_string(ipv4, IPV4_ADDRESS_STRING);

    filesystem_service.access = NULL;
    filesystem_service.changed = NULL;

    receive_service.accept = NULL;
    receive_service.data = NULL;
    receive_service.status = NULL;

    start.device_cloud_url = DEVICE_CLOUD_URL;
    get_device_id_from_mac(start.device_id, mac_address);

    start.device_type = DEVICE_TYPE;
    start.service.file_system = &filesystem_service;
    start.service.receive = &receive_service;
    start.status = NULL;
    start.vendor_id = VENDOR_ID;

    printf("\tStarting CCAPI\n");
    start_error = ccapi_start(&start);

    if (start_error == CCAPI_START_ERROR_NONE)
    {
        printf("\tccapi_start success\n");
    }
    else
    {
        printf("\tccapi_start error %d\n", start_error);
        goto done;
    }

    memcpy(tcp_info.connection.ip.address.ipv4, ipv4, sizeof tcp_info.connection.ip.address.ipv4);
    memcpy(tcp_info.connection.info.lan.mac_address, mac_address, sizeof tcp_info.connection.info.lan.mac_address);
    tcp_info.connection.type = CCAPI_CONNECTION_LAN;
    tcp_info.callback.close = NULL;
    tcp_info.callback.keepalive = NULL;

    printf("\tStarting TCP transport\n");
    tcp_start_error = ccapi_start_transport_tcp(&tcp_info);
    if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
    {
        printf("\tccapi_start_transport_tcp success\n");
    }
    else
    {
        printf("\tccapi_start_transport_tcp failed with error %d\n", tcp_start_error);
        goto done;
    }

    printf("\tRegistering Receive Service target\n");
    receive_error = ccapi_receive_add_target("tank", tank_valves_cb, tank_valves_status_cb, 100);
    if (receive_error == CCAPI_RECEIVE_ERROR_NONE)
    {
        printf("\tccapi_receive_add_target success\n");
    }
    else
    {
        printf("\tccapi_receive_add_target failed with error %d\n", receive_error);
    }

    printf("\tUploading sample data to %s\n", SAMPLE_DATA_PATH);
    ccapi_send_data(CCAPI_TRANSPORT_TCP, SAMPLE_DATA_PATH, "text/plain", SAMPLE_DATA_CONTENT, (sizeof SAMPLE_DATA_CONTENT) - 1, CCAPI_SEND_BEHAVIOR_OVERWRITE);

    printf("\tEntering application's infinite loop\n");
    tank_demo_loop();

done:
    return 0;
}
