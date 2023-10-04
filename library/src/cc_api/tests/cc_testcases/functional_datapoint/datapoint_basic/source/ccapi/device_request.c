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
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "ccapi/ccapi.h"
#include "device_request.h"

/* Import the test functions to execute in the test cases list */
extern void * app_start_test_case_datapoints_loop(void * args);


/* Test Case Thread pointer */
pthread_t test_case_thread;

/* Auxiliar functions to manage the payload */
int split (char *str, char c, char ***arr);



/* Test Cases functions -----------------------------*/


// void test_fs_virtual_dirs_1( ccapi_buffer_info_t const * const request_buffer_info )
void test_datapoint_send_datastream_with_datapoints( char * sentence )
{
    printf("TEST CASE: test_datapoint_send_datastream_with_datapoints '%s'\n",sentence);

    /* Split the payload into the arguments */
    char **arguments = NULL;
    char delimiter = ';';
    int numberOfArguments = 0;
    numberOfArguments = split(sentence, delimiter, &arguments);


    /* If the target is Data point, we know that there are 5 elements in the string */
    unsigned int numberOfLoops = atoi(arguments[0]); /* Obtain an Integer from a char[] */
    unsigned int numberStreams = atoi(arguments[1]); /* Obtain an Integer from a char[] */
    unsigned int numberPointsPerStream = atoi(arguments[2]); /* Obtain an Integer from a char[] */
    char const * valueType = arguments[3];
    char const * streamIdentifier = arguments[4];


    /* Execute the TEST_CASE in a new thread **********************/
    /* Create internal structure to save the test arguments */
    test_thread_arguments_t * test_arguments = malloc(sizeof(test_thread_arguments_t));
    test_arguments->numberOfLoops = numberOfLoops;
    test_arguments->numberPointsPerStream = numberPointsPerStream;
    test_arguments->numberStreams = numberStreams;
    /* Clone the value type */
    test_arguments->valueType = malloc( (sizeof(char) * strlen(valueType)) + 1);
    sprintf(test_arguments->valueType, "%s", valueType);
    /* Clone the stream identifier */
    test_arguments->streamIdentifier = malloc( (sizeof(char) * strlen(streamIdentifier)) + 1);
    sprintf(test_arguments->streamIdentifier, "%s", streamIdentifier);



    printf("numberOfArguments: %d\n",numberOfArguments);
    printf("numberOfLoops: %d\n", numberOfLoops);
    printf("numberStreams: %d\n", numberStreams);
    printf("numberPointsPerStream: %d\n", numberPointsPerStream);
    printf("valueType: %s\n", valueType);
    printf("streamIdentifier: %s\n", streamIdentifier);

    /* Create a new thread to execute the test case with the arguments */
    int ccode = pthread_create(&test_case_thread, NULL, app_start_test_case_datapoints_loop, (void *)test_arguments);
    if (ccode != 0)
    {
        printf("thread_create() app_start_test_case_datapoints_loop on data_point.c %d\n", ccode);
    }


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
    if (  strncmp(target, "test_datapoint_send_datastream_with_datapoints", strlen(target)) == 0 )
    {
        test_datapoint_send_datastream_with_datapoints( sentence );
    }
    else if(strcmp(target, "test_datapoint_kill_test_thread") == 0)
    {
        /* Sending the signal 0, we verify if the thread is alive */
        if ( pthread_kill( test_case_thread, 0) == 0)
        {/* Thread is alive */

            /* Sending the signal for the thread */
            int ccode = pthread_kill( test_case_thread, SIGUSR1); /* SIGKILL */
            if (ccode != 0)
            {
                printf("ERROR: thread_kill() test_datapoint_kill_test_thread %d\n", ccode);

                if ( ccode == ESRCH )
                {
                    printf("ERROR 'ESRCH': No thread could be found corresponding to that specified by the given thread ID.\n");
                }
                else if ( ccode == EINVAL )
                {
                    printf("ERROR 'EINVAL': The value of the sig argument is an invalid or unsupported signal number.\n");
                }
            }
            else
            {
                printf("thread_kill() test_datapoint_kill_test_thread successfully killed %d\n", ccode);
            }
        }
        else
        { /* Thread is dead */
            printf("thread_kill() active test thread is not alive \n");
        }
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


/**
 *  splits str on delim and dynamically allocates an array of pointers.
 *
 *  On error -1 is returned, check errno
 *  On success size of array is returned, which may be 0 on an empty string
 *  or 1 if no delim was found.
 */
int split (char *str, char c, char ***arr)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
        exit(1);

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
        {
            (*arr)[i] = (char*) malloc( sizeof(char) * token_len + 1 );
            if ((*arr)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len + 1 );
    if ((*arr)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
        if (*p != c && *p != '\0')
        {
            *t = *p;
            t++;
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }

    return count;
}