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

#if !(defined UNUSED_ARGUMENT)
#define UNUSED_ARGUMENT(a)  (void)(a)
#endif

#define DESIRED_MAX_RESPONSE_SIZE 400

#define TEST_UDP 1
#define TEST_SMS 0

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

static ccapi_bool_t app_receive_default_accept_cb(char const * const target, ccapi_transport_t const transport)
{
    ccapi_bool_t accept_target = CCAPI_TRUE;

    printf("app_receive_default_accept_cb: target = '%s'. transport = %d\n", target, transport);

    if (transport == CCAPI_TRANSPORT_SMS)
    {
        /* Don't accept requests comming throught SMS for not registered targets */
        accept_target = CCAPI_FALSE;
    }

    return accept_target;
}

static void app_receive_default_data_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    printf("app_receive_default_data_cb: target = '%s'. transport = %d\n", target, transport);

    /* Print request data */
    for (size_t i=0 ; i < request_buffer_info->length ; i++)
    {
            printf("%c", ((char*)request_buffer_info->buffer)[i]);
    }

    /* Provide response to the cloud */
    if (response_buffer_info != NULL)
    {
        response_buffer_info->buffer = malloc(DESIRED_MAX_RESPONSE_SIZE);
        printf("app_receive_default_data_cb: Providing response in buffer at %p\n", response_buffer_info->buffer);

        response_buffer_info->length = sprintf(response_buffer_info->buffer, "Thanks for the info");
    }
    else
    {
        printf("app_receive_default_data_cb: Response not requested by cloud\n");
    }

    return;
}

static void app_get_time_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    size_t max_length;

    printf("app_get_time_cb: transport = %d\n", transport);

    if (transport == CCAPI_TRANSPORT_SMS)
        max_length = 80;
    else
        max_length = DESIRED_MAX_RESPONSE_SIZE;

    response_buffer_info->buffer = malloc(max_length);

    assert(response_buffer_info != NULL);

    if (request_buffer_info->length != 0)
    {
        response_buffer_info->length = sprintf(response_buffer_info->buffer, "Invalid argument, this %s does not take arguments", target);
    }
    else
    {
        time_t t = time(NULL);
        response_buffer_info->length = snprintf(response_buffer_info->buffer, max_length, "time: %s", ctime(&t));
    }

    return;
}

static ccapi_bool_t stop = CCAPI_FALSE;

static void app_stop_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    static char const stop_response[] = "I'll stop";

    UNUSED_ARGUMENT(target);
    UNUSED_ARGUMENT(request_buffer_info);
    UNUSED_ARGUMENT(response_buffer_info);

    printf("app_stop_cb: transport = %d\n", transport);

    response_buffer_info->buffer = (void *)stop_response;
    response_buffer_info->length = strlen(response_buffer_info->buffer);

    stop = CCAPI_TRUE;
}

static void app_receive_default_status_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error)
{
    printf("app_receive_default_status_cb: target = '%s'. transport = %d. Error = %d\n", target, transport, receive_error);

    if (response_buffer_info != NULL)
    {
        printf("app_receive_default_status_cb: Freeing response buffer at %p\n", response_buffer_info->buffer);
        free(response_buffer_info->buffer);
    }
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
    ccapi_start_t start = {0};
    ccapi_start_error_t start_error = CCAPI_START_ERROR_NONE;
    ccapi_receive_service_t receive_service;

    fill_start_structure_with_good_parameters(&start);

    start.service.receive = &receive_service;
    receive_service.accept = app_receive_default_accept_cb;
    receive_service.data = app_receive_default_data_cb;
    receive_service.status = app_receive_default_status_cb;

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

    /* Start TCP transport */
    {
        ccapi_tcp_start_error_t tcp_start_error;
        ccapi_tcp_info_t tcp_info = {{0}};
        uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
        uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */

        tcp_info.connection.type = CCAPI_CONNECTION_LAN;
        tcp_info.connection.max_transactions = 10;
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
    }

#if (TEST_UDP == 1)
    /* Start UDP trasnport */
    {
        ccapi_udp_start_error_t udp_start_error;
        ccapi_udp_info_t udp_info = {{0}};

        udp_info.start_timeout = CCAPI_UDP_START_WAIT_FOREVER;
        udp_info.limit.max_sessions = 10;
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

    {
        ccapi_receive_error_t receive_error;
        
        #define DESIRED_MAX_REQUEST_SIZE 5
        /* A request up to DESIRED_MAX_REQUEST_SIZE will success but app_get_time_cb() will complain that this command doesn't have arguments.
         * A request over DESIRED_MAX_REQUEST_SIZE will make app_get_time_cb() be called with error CCAPI_RECEIVE_ERROR_REQUEST_TOO_BIG 
         */
        receive_error = ccapi_receive_add_target("get_system_time", app_get_time_cb, app_receive_default_status_cb, DESIRED_MAX_REQUEST_SIZE);
        if (receive_error == CCAPI_RECEIVE_ERROR_NONE)
        {
            printf("ccapi_receive_add_target success\n");
        }
        else
        {
            printf("ccapi_receive_add_target failed with error %d\n", receive_error);
        }

        ccapi_receive_add_target("stop", app_stop_cb, NULL, 0);
    }

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
        
        sleep(5);
    } while (check_stop() != CCAPI_TRUE);   
#else
    printf("Endless loop\n");
    do
    {
        sleep(1);
    } while (check_stop() != CCAPI_TRUE);
#endif

done:
    return 0;
}
