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

int main (void)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t start_error = CCAPI_START_ERROR_NONE;

    ccapi_ping_error_t send_error;

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

    send_error = ccapi_send_ping(CCAPI_TRANSPORT_SMS);
    if (send_error == CCAPI_PING_ERROR_NONE)
    {
        printf("ccapi_send_ping for sms success\n");
    }
    else
    {
        printf("ccapi_send_ping for sms failed with error %d\n", send_error);
    }

    #define SMS_TX_TIMEOUT 60 /* Wait 60 seconds to complete sms send operation. Optional: CCAPI_PING_PING_WAIT_FOREVER */
    send_error = ccapi_send_ping_with_reply(CCAPI_TRANSPORT_SMS, SMS_TX_TIMEOUT);
    if (send_error == CCAPI_PING_ERROR_NONE)
    {
        printf("ccapi_send_ping_with_reply for sms success\n");
    }
    else
    {
        printf("ccapi_send_ping_with_reply for sms failed with error %d\n", send_error);
    }
#endif

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

    do
    {
        send_error = ccapi_send_ping(CCAPI_TRANSPORT_UDP);
        if (send_error == CCAPI_PING_ERROR_NONE)
        {
            printf("ccapi_send_ping for udp success\n");
        }
        else
        {
            printf("ccapi_send_ping for udp failed with error %d\n", send_error);
        }

        #define UDP_TX_TIMEOUT 10 /* Wait 10 seconds to complete udp send ping operation. Optional: CCAPI_PING_PING_WAIT_FOREVER */
        send_error = ccapi_send_ping_with_reply(CCAPI_TRANSPORT_UDP, UDP_TX_TIMEOUT);
        if (send_error == CCAPI_PING_ERROR_NONE)
        {
            printf("ccapi_send_ping_with_reply for udp success\n");
        }
        else
        {
            printf("ccapi_send_ping_with_reply for udp failed with error %d\n", send_error);
        }

        sleep(5);
    } while(1);

done:
    return 0;
}
