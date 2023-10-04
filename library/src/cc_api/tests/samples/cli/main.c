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

static ccapi_bool_t stop = CCAPI_FALSE;

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

#define FIX_RESPONSE "This is the command echo: "

static void app_cli_request_cb(ccapi_transport_t const transport, char const * const command, char const * * const output)
{
    char * response;
    printf("app_cli_request_cb: transport = %d, command = '%s'\n", transport, command);


    if (!strcmp(command, "stop"))
    {
        stop = CCAPI_TRUE;
    }

    /* Simulate a low processing time */
    sleep(3);

    /* Provide response to the cloud */
    if (output != NULL)
    {
        response = malloc(sizeof(FIX_RESPONSE) + strlen(command) + strlen("''"));
        printf("app_cli_request_cb: Providing response for command = '%s' in buffer at %p\n", command, response);

        sprintf(response, FIX_RESPONSE "'%s'", command);

        *output = response;
    }
    else
    {
        printf("app_cli_request_cb: Response not requested by cloud\n");
    }

    return;
}


static void app_cli_finished_cb(char * const output, ccapi_cli_error_t cli_error)
{
    printf("app_cli_finished_cb: Error = %d\n", cli_error);

    if (output != NULL)
    {
        printf("app_cli_finished_cb: Freeing response buffer at %p\n", output);
        free(output);
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
    ccapi_cli_service_t cli_service;

    fill_start_structure_with_good_parameters(&start);

    start.service.cli = &cli_service;
    cli_service.request = app_cli_request_cb;
    cli_service.finished = app_cli_finished_cb;

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
