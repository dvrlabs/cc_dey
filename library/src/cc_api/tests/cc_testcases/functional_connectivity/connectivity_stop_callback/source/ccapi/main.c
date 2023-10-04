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
#include "helper/helper_library.h"
#include "device_request.h"


/******** Modified for testing ********/
/* Global flag to force the reconnection */
// extern connector_bool_t reconnect_transport = connector_false;
/******** End of Modifications for testing ********/


int main (void)
{
    /* Start Initialization ---------------- */
    /* Initialize settings structure */
    ccapi_start_t start = {0};
    /* Initialize error structure */
    ccapi_start_error_t start_error = CCAPI_START_ERROR_NONE;

    /* TCP Transport ----------------------- */
    /* Initialize settings structure for TCP transport */
    ccapi_tcp_info_t tcp_info = {{0}};
    /* Initialize error structure for TCP transport */
    ccapi_tcp_start_error_t tcp_start_error;







    /* ----- STEP 1: Start the Cloud Connector ----- */
    /* Fill the settings structure with valid parameters */
    fill_device_settings(&start);

    /* Fill File system */
    fill_filesystem_service(&start);

    /* Fill Device request service */
    fill_device_request_service(&start);

    /* Launch the connector instance */
    start_error = ccapi_start(&start);

    if (start_error == CCAPI_START_ERROR_NONE)
    {
        printf("ccapi_start success\n");
    }
    else
    {
        printf("ccapi_start error %d\n", start_error);
        goto error;
    }





    /* ----- STEP 2: Start the TCP transport ----- */
    /* Fill the connection settings for the transport */
    fill_tcp_transport_settings(&tcp_info);

    /* Launch the TCP transport */
    tcp_start_error = ccapi_start_transport_tcp(&tcp_info);
    if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
    {
        printf("ccapi_start_transport_tcp success\n");
    }
    else
    {
        printf("ccapi_start_transport_tcp failed with error %d\n", tcp_start_error);
        goto error;
    }





    /* LOOP TO MAINTAIN THE CONNECTION */
    printf("Waiting for ever...\n");
    for(;;)
    {

        switch( device_request_start_test_action )
        {
            case TEST_01_START_FLAG:
                printf("TEST: test_01_stop_connector_wait_sessions_complete_for_transport_all\n");

                /* Disable flag */
                device_request_start_test_action = TEST_DO_NOTHING_FLAG;

                /* STOP the UDP transport */
                printf("main.c: call to ccapi_stop_transport_udp()...\n");
                ccapi_udp_stop_t udp_stop;
                ccapi_udp_stop_error_t udp_stop_error;

                udp_stop.behavior = CCAPI_TRANSPORT_STOP_GRACEFULLY;
                udp_stop_error = ccapi_stop_transport_udp(&udp_stop);

                if (udp_stop_error != CCAPI_UDP_STOP_ERROR_NOT_STARTED)
                {
                    assert("Error stopping the transport_udp!!!");
                    goto error;
                }
                else
                {
                    printf("ccapi_stop_transport_udp returns CCAPI_UDP_STOP_ERROR_NOT_STARTED\n");
                }


                /* STOP the SMS transport */
                printf("main.c: call to ccapi_stop_transport_sms()...\n");
                ccapi_sms_stop_t sms_stop;
                ccapi_sms_stop_error_t sms_stop_error;

                sms_stop.behavior = CCAPI_TRANSPORT_STOP_GRACEFULLY;
                sms_stop_error = ccapi_stop_transport_sms(&sms_stop);
                if (sms_stop_error != CCAPI_SMS_STOP_ERROR_NOT_STARTED)
                {
                    assert("Error stopping the transport_sms!!!");
                    goto error;
                }
                else
                {
                    printf("ccapi_stop_transport_sms returns CCAPI_SMS_STOP_ERROR_NOT_STARTED\n");
                }

                break;

            case TEST_02_START_FLAG:
                printf("TEST: test_02_stop_connector_wait_sessions_complete_for_transport_tcp\n");

                /* Disable flag */
                device_request_start_test_action = TEST_DO_NOTHING_FLAG;


                printf("main.c: call to ccapi_stop_transport_tcp(CCAPI_TRANSPORT_STOP_GRACEFULLY)...\n");

                ccapi_tcp_stop_t ccapi_tcp_stop_info;
                ccapi_tcp_stop_error_t tcp_stop_error;

                ccapi_tcp_stop_info.behavior = CCAPI_TRANSPORT_STOP_GRACEFULLY;
                tcp_stop_error = ccapi_stop_transport_tcp(&ccapi_tcp_stop_info);


                if (tcp_stop_error != CCAPI_TCP_STOP_ERROR_NONE)
                {
                    assert("Error stopping the transport_tcp!!!");
                    goto error;
                }
                else
                {
                    printf("ccapi_stop_transport_tcp returns CCAPI_TCP_STOP_ERROR_NONE\n");
                }


                printf("main.c: waiting 5 seconds after start the tcp transport again...\n");
                sleep(5);

                /* Activate flag to launch the TCP transport */
                flag_start_tcp_transport = connector_true;

                break;

            case TEST_04_START_FLAG:
                printf("TEST: test_04_stop_connector_stop_immediately_for_transport_tcp\n");

                /* Disable flag */
                device_request_start_test_action = TEST_DO_NOTHING_FLAG;


                printf("main.c: waiting 5 seconds after start the tcp transport again...\n");
                sleep(5);

                /* Activate flag to launch the TCP transport */
                flag_start_tcp_transport = connector_true;

                break;

            case TEST_06_START_FLAG:
                printf("TEST: test_06_connector_terminate\n");

                /* Disable flag */
                device_request_start_test_action = TEST_DO_NOTHING_FLAG;

                ccapi_stop_error_t error = ccapi_stop(CCAPI_STOP_GRACEFULLY);
                if (error != CCAPI_STOP_ERROR_NONE)
                {
                    assert("Error stopping the ccapi!!!");
                    exit(1);
                }
                else
                {
                    printf("ccapi_stop returns CCAPI_STOP_ERROR_NONE\n");
                }

                break;

            default:
//                 printf("main.c: Waiting for next action....\n");
                break;

        }


        if(flag_start_tcp_transport)
        {
            printf("main.c: relaunch the tcp transport for the next test...\n");

            /* Initialize settings structure for TCP transport */
            ccapi_tcp_info_t tcp_info = {{0}};
            /* Initialize error structure for TCP transport */
            ccapi_tcp_start_error_t tcp_start_error;

            /* Fill the connection settings for the transport */
            fill_tcp_transport_settings(&tcp_info);

            /* Launch the TCP transport */
            tcp_start_error = ccapi_start_transport_tcp(&tcp_info);
            if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
            {
                printf("ccapi_start_transport_tcp success\n");
                /* Disable flag */
                flag_start_tcp_transport = connector_false;
            }
            else
            {
                printf("ccapi_start_transport_tcp failed with error %d\n", tcp_start_error);
                goto error;
            }

        }


        /* This sleep is needed to run the sample because without it does not work */
        sleep(1);

    }


    printf("ccapi: exit successfully");
    return 0;


error:
    printf("ccapi: exit with errors!!!!");
    return 1;
}
