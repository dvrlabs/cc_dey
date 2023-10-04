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
#include <string.h>
#include <assert.h>
#include "ccapi/ccapi.h"
#include "device_request.h"


/* Initialize flag to comunicate with the main.c */
test_flag_start_action_t device_request_start_test_action = TEST_DO_NOTHING_FLAG;
char * test_data_buffer = NULL;

/* Test Cases functions -----------------------------*/

void test_put_request_send_option_overwrite( char * sentence )
{
    printf("TEST CASE: put_request in override mode\n");

    size_t bytes_used = strlen(sentence);

    test_data_buffer = realloc(test_data_buffer, bytes_used+1);
    if (test_data_buffer == NULL)
    {
        printf("Error with realloc() for test buffer!!!!!!!!");
    }
    
    /* let's copy the payload to test case buffer */
    memcpy(test_data_buffer, sentence, bytes_used);
    test_data_buffer[bytes_used] = '\0';

    device_request_start_test_action = TEST_PUT_REQUEST_SEND_OPTION_OVERWRITE_FLAG;

}


void test_put_request_send_option_append( char * sentence )
{
    printf("TEST CASE: put_request in append mode\n");

    size_t bytes_used = strlen(sentence);

    test_data_buffer = realloc(test_data_buffer, bytes_used+1);
    if (test_data_buffer == NULL)
    {
        printf("Error with realloc() for test buffer!!!!!!!!");
    }

    /* let's copy the payload to test case buffer */
    memcpy(test_data_buffer, sentence, bytes_used);
    test_data_buffer[bytes_used] = '\0';

    device_request_start_test_action = TEST_PUT_REQUEST_SEND_OPTION_APPEND_FLAG;


}




/* Device request functions -----------------------------*/

#define DESIRED_MAX_RESPONSE_SIZE 400


/* This function is called for accept or reject a specific target, if it is not defined all targets will be accepted */
ccapi_bool_t ccapi_device_request_accept_callback(char const * const target, ccapi_transport_t const transport)
{
    ccapi_bool_t accept_target = CCAPI_TRUE;

    printf("ccapi_device_request_accept_callback: target = '%s'. transport = %d\n", target, transport);

    /* If we don't accept this target */
    /* accept_target = CCAPI_FALSE; */

    return accept_target;
}


/* This function is called only if the target was accepted by the previous callback */
void ccapi_device_request_data_callback(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    printf("ccapi_device_request_data_callback: target = '%s'. transport = %d.\n", target, transport);

    /* Print data */
    {
        size_t i;
        printf("Data received: '");
        for (i=0 ; i < request_buffer_info->length ; i++)
        {
            printf("%c", ((char*)request_buffer_info->buffer)[i]);
        }
        printf("'\n");
        //printf("\nccapi_device_request_data_callback received total %d bytes\n", (int)request_buffer_info->length);
    }

    /* Create a string with the data */
    char sentence[request_buffer_info->length + 1];
    size_t const length = request_buffer_info->length;
    memcpy(sentence, request_buffer_info->buffer, length);
    sentence[length] = '\0';


    /* Execute the selected action for this target */
    if (  strncmp(target, "test_put_request_send_option_overwrite", strlen(target)) == 0 )
    {
        test_put_request_send_option_overwrite( sentence );
    }
    else if (  strncmp(target, "test_put_request_send_option_append", strlen(target)) == 0 )
    {
        test_put_request_send_option_append( sentence );
    }
    else
    {
        printf("ERROR: Unknown target '%s'\n", target);
    }





    /* Provide response */
    if (response_buffer_info != NULL)
    {
        response_buffer_info->buffer = malloc(DESIRED_MAX_RESPONSE_SIZE);
        printf("ccapi_device_request_data_callback: Providing response in buffer at %p\n", response_buffer_info->buffer);

        response_buffer_info->length = sprintf(response_buffer_info->buffer, "Request successfully processed");
    }

    return;
}



/* This function is called with the response from the cloud after send the response to the request. */
void ccapi_device_request_status_callback(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error)
{
    printf("ccapi_device_request_status_callback: target = '%s'. transport = %d. Error = %d\n", target, transport, receive_error);

    if (response_buffer_info != NULL)
    {
        printf("Freeing response buffer at %p\n", response_buffer_info->buffer);
        free(response_buffer_info->buffer);
    }

    (void)response_buffer_info;
}



void fill_device_request_service(ccapi_start_t * start)
{
    static ccapi_receive_service_t receive_service;

    /* Fill the connection callbacks for the transport */
    receive_service.accept = ccapi_device_request_accept_callback;
    receive_service.data = ccapi_device_request_data_callback;
    receive_service.status = ccapi_device_request_status_callback;


    /* Set the Filesystem service */
    start->service.receive = &receive_service;
}
