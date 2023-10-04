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
#include "helper/helper_library.h"




#define DATA         "CCAPI send data sample\n"

void start_send_data_action(void)
{
    /* Send Data ----------------------- */
    /* Initialize error structure for send data*/
    ccapi_send_error_t send_error;
    /* Initialize data buffer to send */
    char const data[] = DATA;
    /* Initialize info structure for returned info */
    ccapi_string_info_t hint_string_info;
    /* Initialize string buffer to save the server response */
    char hint_string[100] = "";
    /* Fill the string buffer in the structure */
    hint_string_info.string = hint_string;
    hint_string_info.length = sizeof(hint_string);


    send_error = ccapi_send_data(CCAPI_TRANSPORT_TCP, "test/ccapi_send_data.txt", "text/plain", data, strlen(data), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data success\n");
    }
    else
    {
        printf("ccapi_send_data failed with error %d\n", send_error);
    }



    send_error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, "test/ccapi_send_data_with_reply.txt", "text/plain", data, strlen(data), CCAPI_SEND_BEHAVIOR_OVERWRITE, CCAPI_SEND_WAIT_FOREVER, &hint_string_info);
    if (send_error == CCAPI_SEND_ERROR_NONE)
    {
        printf("ccapi_send_data_with_reply success\n");
    }
    else
    {
        printf("ccapi_send_data_with_reply failed with error %d\n", send_error);

        if (strlen(hint_string_info.string) != 0)
            printf("ccapi_send_data_with_reply hint %s\n", hint_string_info.string);
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




    /* ----- STEP 3: Send Data over the TCP transport ----- */
    start_send_data_action();




    /* LOOP TO MAINTAIN THE CONNECTION */
    printf("Waiting for ever...\n");
    for(;;);


    printf("ccapi: exit successfully");
    return 0;


error:
    printf("ccapi: exit with errors!!!!");
    return 1;
}
