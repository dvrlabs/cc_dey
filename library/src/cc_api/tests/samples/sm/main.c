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
#include <time.h>
#include <unistd.h>
#include "ccapi/ccapi.h"

#define DEVICE_TYPE_STRING      "Device type"
#define DEVICE_CLOUD_URL_STRING "devicecloud.digi.com"

#define TEST_UDP 1
#define TEST_SMS 0

static ccapi_bool_t start_tcp = CCAPI_FALSE;
static ccapi_bool_t stop_tcp = CCAPI_FALSE;
static int stop_tcp_count = 0;

void fill_start_structure_with_good_parameters(ccapi_start_t * start)
{
    uint8_t device_id[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x9D, 0xFF, 0xFF, 0xAB, 0xCD, 0xEF};
	
    char const * const device_cloud_url = DEVICE_CLOUD_URL_STRING;
    char const * const device_type = DEVICE_TYPE_STRING;
    start->vendor_id = 0x030000DB; /* Set vendor_id or ccapi_init_error_invalid_vendorid will be returned instead */
    memcpy(start->device_id, device_id, sizeof start->device_id);
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

static void app_sm_request_connect_cb(ccapi_transport_t const transport)
{
    printf("app_sm_request_connect_cb: transport = %d\n", transport);

    start_tcp = CCAPI_TRUE;
}

static void app_sm_ping_request_cb(ccapi_transport_t const transport, ccapi_bool_t const response_required)
{
    printf("app_sm_ping_request_cb: transport = %d, response %s needed\n", transport, response_required ? "is" : "is not");
}

static void app_sm_unsequenced_response_cb(ccapi_transport_t const transport, uint32_t const id, void const * const data, size_t const bytes_used, ccapi_bool_t error)
{
    printf("Received %" PRIsize " unsequenced bytes on id %d\n", bytes_used, id);

    (void)transport;
    (void)data;
    (void)error;
}

static void app_sm_pending_data_cb(ccapi_transport_t const transport)
{
    printf("app_sm_pending_data_cb: transport = %d\n", transport);
}

static void app_sm_phone_provisioning_cb(ccapi_transport_t const transport, char const * const phone_number, char const * const service_id, ccapi_bool_t const response_required)
{
    printf("ccapi_process_phone_provisioning: response %s needed\n", response_required ? "is" : "is not");
    printf("phone-number=%s\n", phone_number);
    printf("service-id=%s\n", service_id);

    (void)transport;
}

static ccapi_bool_t check_start_tcp(void)
{
    static ccapi_tcp_info_t tcp_info = {{0}};
    static uint8_t const ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    static uint8_t const mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */

    if (start_tcp == CCAPI_TRUE)
    {
        ccapi_tcp_start_error_t tcp_start_error;

        printf("Calling ccapi_start_transport_tcp\n");

        tcp_info.connection.type = CCAPI_CONNECTION_LAN;
        memcpy(tcp_info.connection.ip.address.ipv4, ipv4, sizeof tcp_info.connection.ip.address.ipv4);
        memcpy(tcp_info.connection.info.lan.mac_address, mac, sizeof tcp_info.connection.info.lan.mac_address);

        tcp_info.callback.close = ccapi_tcp_close_cb;
        tcp_info.callback.keepalive = NULL;

        tcp_start_error = ccapi_start_transport_tcp(&tcp_info);
        if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
        {
            printf("ccapi_start_transport_tcp success\n");

            stop_tcp = CCAPI_TRUE;
            stop_tcp_count = 5;
        }
        else
        {
            printf("ccapi_start_transport_tcp failed with error %d\n", tcp_start_error);
        }

        start_tcp = CCAPI_FALSE;
    }

    return start_tcp;
}

static ccapi_bool_t check_stop_tcp(void)
{
    ccapi_tcp_stop_error_t tcp_stop_error;
    ccapi_tcp_stop_t tcp_stop = {CCAPI_TRANSPORT_STOP_GRACEFULLY};

    if (stop_tcp == CCAPI_FALSE)
        goto done;
    
    stop_tcp_count--;

    printf("check_stop_tcp: stop_tcp_count=%d\n", stop_tcp_count);
    
    if (stop_tcp_count == 0)
    {
        tcp_stop_error = ccapi_stop_transport_tcp(&tcp_stop);
        if (tcp_stop_error == CCAPI_TCP_STOP_ERROR_NONE)
        {
            printf("ccapi_stop_transport_tcp success\n");
        }
        else
        {
            printf("ccapi_stop_transport_tcp failed with error %d\n", tcp_stop_error);
        }

        stop_tcp = CCAPI_FALSE;
    }

done:
    return stop_tcp;
}

int main (void)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t start_error = CCAPI_START_ERROR_NONE;
    ccapi_sm_service_t sm_service;

    fill_start_structure_with_good_parameters(&start);

    start.service.sm = &sm_service;
    sm_service.request_connect = app_sm_request_connect_cb;
    sm_service.ping_request = app_sm_ping_request_cb;
    sm_service.unsequenced_response = app_sm_unsequenced_response_cb;
    sm_service.pending_data = app_sm_pending_data_cb;
    sm_service.phone_provisioning = app_sm_phone_provisioning_cb;

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

#if (TEST_UDP == 1)
    /* Start UDP trasnport */
    {
        ccapi_udp_start_error_t udp_start_error;
        ccapi_udp_info_t udp_info = {{0}};

        udp_info.start_timeout = CCAPI_UDP_START_WAIT_FOREVER;
        udp_info.limit.max_sessions = 20;
        udp_info.limit.rx_timeout = CCAPI_UDP_RX_TIMEOUT_INFINITE;

        udp_info.callback.close = NULL;

        udp_start_error = ccapi_start_transport_udp(&udp_info);
        if (udp_start_error == CCAPI_UDP_START_ERROR_NONE)
        {
            printf("ccapi_start_transport_udp success\n");
        }
        else
        {
            printf("ccapi_start_transport_udp failed with error %d\n", udp_start_error);
            goto done;
        }
    }
#endif

#if (TEST_SMS == 1)
    /* Start SMS trasnport */
    {
        ccapi_sms_start_error_t sms_start_error;
        ccapi_sms_info_t sms_info = {{0}};

        sms_info.start_timeout = CCAPI_TCP_START_WAIT_FOREVER;
        sms_info.limit.max_sessions = 10;
        sms_info.limit.rx_timeout = CCAPI_SMS_RX_TIMEOUT_INFINITE;

        sms_info.callback.close = NULL;

        sms_info.cloud_config.phone_number = "32075";
        sms_info.cloud_config.service_id = "idgp";

        sms_start_error = ccapi_start_transport_sms(&sms_info);
        if (sms_start_error == CCAPI_SMS_START_ERROR_NONE)
        {
            printf("ccapi_start_transport_sms success\n");
        }
        else
        {
            printf("ccapi_start_transport_sms failed with error %d\n", sms_start_error);
            goto done;
        }
    }
#endif

#if (TEST_UDP == 1)
    printf("Send UDP traffic periodically to the cloud so it send us queued requests\n");
    do
    {   
        ccapi_ping_error_t send_error;
        send_error = ccapi_send_ping(CCAPI_TRANSPORT_UDP);
        if (send_error == CCAPI_PING_ERROR_NONE)
        {
            printf("ccapi_send_ping for udp success\n");
        }
        else
        {
            printf("ccapi_send_ping for udp failed with error %d\n", send_error);
        }
        
        check_start_tcp();
        check_stop_tcp();
        sleep(5);
    } while (1);
#else
    printf("Endless loop\n");
    do
    {
        check_start_tcp();
        check_stop_tcp();
        sleep(1);
    } while (1);
#endif

done:
    return 0;
}
