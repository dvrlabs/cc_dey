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

#define DEVICE_TYPE_STRING      "Device type"
#define DEVICE_CLOUD_URL_STRING "devicecloud.digi.com"

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

int main (void)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t start_error = CCAPI_START_ERROR_NONE;

    ccapi_send_error_t send_error;
    char hint_string[100] = "";
    ccapi_string_info_t hint_string_info;

    fill_start_structure_with_good_parameters(&start);

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

    {
        ccapi_tcp_start_error_t tcp_start_error;
        ccapi_tcp_info_t tcp_info = {{0}};
        uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
        uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */

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
    }

    #define SEND_DATA_TCP         "CCAPI send_data_tcp() sample\n"
    send_error = ccapi_send_data(CCAPI_TRANSPORT_TCP, "send_data_tcp.txt", "text/plain", SEND_DATA_TCP, strlen(SEND_DATA_TCP), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data for tcp success\n");
    }
    else
    {
        printf("ccapi_send_data for tcp failed with error %d\n", send_error);
    }

    hint_string_info.string = hint_string;
    hint_string_info.length = sizeof(hint_string);

    #define SEND_DATA_TCP_WITH_REPLY         "CCAPI send_data_tcp_with_reply() sample\n"
    send_error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, "send_data_tcp_with_reply.txt", "text/plain", SEND_DATA_TCP_WITH_REPLY, strlen(SEND_DATA_TCP_WITH_REPLY), CCAPI_SEND_BEHAVIOR_OVERWRITE, CCAPI_SEND_WAIT_FOREVER, &hint_string_info);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data_with_reply for tcp success\n");
    }
    else
    {
        printf("ccapi_send_data_with_reply for tcp failed with error %d\n", send_error);

        if (strlen(hint_string_info.string) != 0)
            printf("ccapi_send_data_with_reply for tcp hint %s\n", hint_string_info.string);
    }

    {
        ccapi_udp_start_error_t udp_start_error;
        ccapi_udp_info_t udp_info = {{0}};

        udp_info.start_timeout = CCAPI_UDP_START_WAIT_FOREVER;
        udp_info.limit.max_sessions = 1;
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

    #define SEND_DATA_UDP         "CCAPI send_data_udp() sample\n"
    send_error = ccapi_send_data(CCAPI_TRANSPORT_UDP, "send_data_udp.txt", "text/plain", SEND_DATA_UDP, strlen(SEND_DATA_UDP), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data for udp success\n");
    }
    else
    {
        printf("ccapi_send_data for udp failed with error %d\n", send_error);
    }

    #define UDP_TX_TIMEOUT 10 /* Wait 10 seconds to complete udp send operation. Optional: CCAPI_SEND_WAIT_FOREVER */
    hint_string_info.string = hint_string;
    hint_string_info.length = sizeof(hint_string);

    #define SEND_DATA_UDP_WITH_REPLY         "CCAPI send_data_udp_with_reply() sample\n"
    send_error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_UDP, "send_data_udp_with_reply.txt", "text/plain", SEND_DATA_UDP_WITH_REPLY, strlen(SEND_DATA_UDP_WITH_REPLY), CCAPI_SEND_BEHAVIOR_OVERWRITE, UDP_TX_TIMEOUT, &hint_string_info);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data_with_reply for udp success\n");
    }
    else
    {
        printf("ccapi_send_data_with_reply for udp failed with error %d\n", send_error);

        if (strlen(hint_string_info.string) != 0)
            printf("ccapi_send_data_with_reply for udp hint %s\n", hint_string_info.string);
    }

#if 0 /* SMS */
    {
        ccapi_sms_start_error_t sms_start_error;
        ccapi_sms_info_t sms_info = {{0}};

        sms_info.start_timeout = CCAPI_TCP_START_WAIT_FOREVER;
        sms_info.limit.max_sessions = 1;
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

    #define SEND_DATA_SMS         "CCAPI send_data_sms() sample\n"
    send_error = ccapi_send_data(CCAPI_TRANSPORT_SMS, "send_data_sms.txt", "text/plain", SEND_DATA_SMS, strlen(SEND_DATA_SMS), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data for sms success\n");
    }
    else
    {
        printf("ccapi_send_data for sms failed with error %d\n", send_error);
    }

    #define SMS_TX_TIMEOUT 60 /* Wait 60 seconds to complete sms send operation. Optional: CCAPI_SEND_WAIT_FOREVER */
    hint_string_info.string = hint_string;
    hint_string_info.length = sizeof(hint_string);

    #define SEND_DATA_SMS_WITH_REPLY         "CCAPI send_data_sms_with_reply() sample\n"
    send_error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_SMS, "send_data_sms_with_reply.txt", "text/plain", SEND_DATA_SMS_WITH_REPLY, strlen(SEND_DATA_SMS_WITH_REPLY), CCAPI_SEND_BEHAVIOR_OVERWRITE, SMS_TX_TIMEOUT, &hint_string_info);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data_with_reply for sms success\n");
    }
    else
    {
        printf("ccapi_send_data_with_reply for sms failed with error %d\n", send_error);

        if (strlen(hint_string_info.string) != 0)
            printf("ccapi_send_data_with_reply for sms hint %s\n", hint_string_info.string);
    }
#endif

done:
    return 0;
}
