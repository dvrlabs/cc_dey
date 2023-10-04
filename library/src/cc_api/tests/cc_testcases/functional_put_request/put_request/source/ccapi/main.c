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



void show_send_data_error(ccapi_send_error_t error)
{
    switch ( error )
    {
        case CCAPI_SEND_ERROR_NONE:
            printf("main.c: ccapi_send_data was executed without errors\n");
            break;
        case CCAPI_SEND_ERROR_CCAPI_NOT_RUNNING:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_CCAPI_NOT_RUNNING' error\n");
            break;
        case CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED' error\n");
            break;
        case CCAPI_SEND_ERROR_FILESYSTEM_NOT_SUPPORTED:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_FILESYSTEM_NOT_SUPPORTED' error\n");
            break;
        case CCAPI_SEND_ERROR_INVALID_CLOUD_PATH:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INVALID_CLOUD_PATH' error\n");
            break;
        case CCAPI_SEND_ERROR_INVALID_CONTENT_TYPE:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INVALID_CONTENT_TYPE' error\n");
            break;
        case CCAPI_SEND_ERROR_INVALID_DATA:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INVALID_DATA' error\n");
            break;
        case CCAPI_SEND_ERROR_INVALID_LOCAL_PATH:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INVALID_LOCAL_PATH' error\n");
            break;
        case CCAPI_SEND_ERROR_NOT_A_FILE:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_NOT_A_FILE' error\n");
            break;
        case CCAPI_SEND_ERROR_ACCESSING_FILE:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_ACCESSING_FILE' error\n");
            break;
        case CCAPI_SEND_ERROR_INVALID_HINT_POINTER:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INVALID_HINT_POINTER' error\n");
            break;
        case CCAPI_SEND_ERROR_INSUFFICIENT_MEMORY:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INSUFFICIENT_MEMORY' error\n");
            break;
        case CCAPI_SEND_ERROR_LOCK_FAILED:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_LOCK_FAILED' error\n");
            break;
        case CCAPI_SEND_ERROR_INITIATE_ACTION_FAILED:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_INITIATE_ACTION_FAILED' error\n");
            break;
        case CCAPI_SEND_ERROR_STATUS_CANCEL:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_STATUS_CANCEL' error\n");
            break;
        case CCAPI_SEND_ERROR_STATUS_TIMEOUT:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_STATUS_TIMEOUT' error\n");
            break;
        case CCAPI_SEND_ERROR_STATUS_SESSION_ERROR:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_STATUS_SESSION_ERROR' error\n");
            break;
        case CCAPI_SEND_ERROR_RESPONSE_BAD_REQUEST:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_RESPONSE_BAD_REQUEST' error\n");
            break;
        case CCAPI_SEND_ERROR_RESPONSE_UNAVAILABLE:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_RESPONSE_UNAVAILABLE' error\n");
            break;
        case CCAPI_SEND_ERROR_RESPONSE_CLOUD_ERROR:
            printf("main.c: ccapi_send_data throws 'CCAPI_SEND_ERROR_RESPONSE_CLOUD_ERROR' error\n");
            break;
        default:
            //print("default");
            break;
    }

}



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

        char const file_type[] = "text/plain";
        ccapi_send_error_t send_error;

        switch( device_request_start_test_action )
        {
            case TEST_PUT_REQUEST_SEND_OPTION_OVERWRITE_FLAG:
                printf("TEST: test_put_request_send_option_overwrite\n");
                /* Disable test flag */
                device_request_start_test_action = TEST_DO_NOTHING_FLAG;

                /* Initialize send data arguments */
                char const cloud_path_overwrite[] = "test/test_overwrite.txt";

                /* Call to send the data */
                send_error = ccapi_send_data( CCAPI_TRANSPORT_TCP,
                                            cloud_path_overwrite, file_type,
                                            test_data_buffer, strlen(test_data_buffer),
                                            CCAPI_SEND_BEHAVIOR_OVERWRITE);


                if (send_error != CCAPI_SEND_ERROR_NONE)
                {
                    printf("ccapi_send_data failed with error [%d]\n", send_error);
                }
                else
                {
                    printf("Data service for '%s' was successfully\n", cloud_path_overwrite);
                }

                show_send_data_error(send_error);


                break;

            case TEST_PUT_REQUEST_SEND_OPTION_APPEND_FLAG:
                printf("TEST: test_put_request_send_option_append\n");
                /* Disable test flag */
                device_request_start_test_action = TEST_DO_NOTHING_FLAG;

                /* Initialize send data arguments */
                char const cloud_path_append[] = "test/test_append.txt";

                /* Call to send the data */
                send_error = ccapi_send_data( CCAPI_TRANSPORT_TCP,
                                            cloud_path_append, file_type,
                                            test_data_buffer, strlen(test_data_buffer),
                                            CCAPI_SEND_BEHAVIOR_APPEND);


                if (send_error != CCAPI_SEND_ERROR_NONE)
                {
                    printf("ccapi_send_data failed with error [%d]\n", send_error);
                }
                else
                {
                    printf("Data service for '%s' was successfully\n", cloud_path_append);
                }

                show_send_data_error(send_error);


                break;

            default:
                printf("main.c: Waiting for next action....\n");
                break;
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
