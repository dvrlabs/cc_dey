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


/* Test Cases functions -----------------------------*/

void show_virtual_dir_error_and_stop(ccapi_fs_error_t error)
{

    /* This is an unexpected error */
    printf("show_virtual_dir_error_and_stop: Unexpected error caught ");
    if ( error == CCAPI_FS_ERROR_NOT_A_DIR )
    {
        printf("CCAPI_FS_ERROR_NOT_A_DIR\n");
    }
    else if ( error == CCAPI_FS_ERROR_CCAPI_STOPPED )
    {
        printf("CCAPI_FS_ERROR_CCAPI_STOPPED\n");
    }
    else if  ( error == CCAPI_FS_ERROR_NO_FS_SUPPORT )
    {
        printf("CCAPI_FS_ERROR_NO_FS_SUPPORT\n");
    }
    else if  ( error == CCAPI_FS_ERROR_INVALID_PATH )
    {
        printf("CCAPI_FS_ERROR_INVALID_PATH\n");
    }
    else if  ( error == CCAPI_FS_ERROR_INSUFFICIENT_MEMORY )
    {
        printf("CCAPI_FS_ERROR_INSUFFICIENT_MEMORY\n");
    }
    else if  ( error == CCAPI_FS_ERROR_NOT_MAPPED )
    {
        printf("CCAPI_FS_ERROR_NOT_MAPPED\n");
    }
    else if  ( error == CCAPI_FS_ERROR_ALREADY_MAPPED )
    {
        printf("CCAPI_FS_ERROR_ALREADY_MAPPED\n");
    }
    else if  ( error == CCAPI_FS_ERROR_LOCK_FAILED )
    {
        printf("CCAPI_FS_ERROR_LOCK_FAILED\n");
    }
    else if  ( error == CCAPI_FS_ERROR_NONE )
    {
        printf("CCAPI_FS_ERROR_NONE\n");
    }
    else
    {
        printf("UNKNOWN ERROR!!!\n");
    }

    /* Break the execution */
    assert(0);

}

// void test_fs_virtual_dirs_1( ccapi_buffer_info_t const * const request_buffer_info )
void test_01_add_virtual_directory_nonexistent_path( char * sentence )
{
    printf("TEST CASE: Add virtual directory for a nonexistent path '%s'\n",sentence);

    /* Add virtual directory with a real path passed by device request */
    ccapi_fs_error_t error = ccapi_fs_add_virtual_dir("Test path1", sentence);


    if ( error == CCAPI_FS_ERROR_NOT_A_DIR )
    {
        printf("test_01_add_virtual_directory_nonexistent_path: Error caught CCAPI_FS_ERROR_NOT_A_DIR\n");
    }
    else
    {
        /* This is an unexpected error */
        show_virtual_dir_error_and_stop(error);
    }

}



void test_02_add_virtual_directory_existent_path( char * sentence )
{
    printf("TEST CASE: Add virtual directory in a valid path '%s'\n",sentence);

    /* Add virtual directory with a real path passed by device request */
    ccapi_fs_error_t error = ccapi_fs_add_virtual_dir("Test path1", sentence);


    if ( error == CCAPI_FS_ERROR_NONE )
    {
        printf("test_02_add_virtual_directory_existent_path: Error caught CCAPI_FS_ERROR_NONE\n");
    }
    else
    {
        /* This is an unexpected error */
        show_virtual_dir_error_and_stop(error);
    }
}



void test_03_add_virtual_directory_already_mapped( char * sentence )
{
    printf("TEST CASE: Add virtual directory in an already mapped path '%s'\n",sentence);

    /* Add virtual directory with a real path passed by device request */
    ccapi_fs_error_t error = ccapi_fs_add_virtual_dir("Test path1", sentence);


    if ( error == CCAPI_FS_ERROR_ALREADY_MAPPED )
    {
        printf("test_03_add_virtual_directory_already_mapped: Error caught CCAPI_FS_ERROR_ALREADY_MAPPED\n");
    }
    else
    {
        /* This is an unexpected error */
        show_virtual_dir_error_and_stop(error);
    }
}


void test_04_remove_existent_virtual_directory( char * sentence )
{
    printf("TEST CASE: remove virtual directory that has been added before '%s'\n",sentence);

    /* Add virtual directory with a real path passed by device request */
    ccapi_fs_error_t error = ccapi_fs_remove_virtual_dir("Test path1");


    if ( error == CCAPI_FS_ERROR_NONE )
    {
        printf("test_04_remove_existent_virtual_directory: Error caught CCAPI_FS_ERROR_NONE\n");
    }
    else
    {
        /* This is an unexpected error */
        show_virtual_dir_error_and_stop(error);
    }
}


void test_05_remove_nonexistent_virtual_directory( char * sentence )
{
    printf("TEST CASE: remove virtual directory that has not been added before '%s'\n",sentence);

    /* Add virtual directory with a real path passed by device request */
    ccapi_fs_error_t error = ccapi_fs_remove_virtual_dir("Test path1");


    if ( error == CCAPI_FS_ERROR_NOT_MAPPED )
    {
        printf("test_05_remove_nonexistent_virtual_directory: Error caught CCAPI_FS_ERROR_NOT_MAPPED\n");
    }
    else
    {
        /* This is an unexpected error */
        show_virtual_dir_error_and_stop(error);
    }
}


void test_06_send_file_to_virtual_directory( char * sentence )
{
    printf("TEST CASE: send file to virtual directory '%s'\n",sentence);

    /* Add virtual directory with a real path passed by device request */
    ccapi_fs_error_t error = ccapi_fs_add_virtual_dir("Test_06_virtual_path", sentence);


    if ( error == CCAPI_FS_ERROR_NONE )
    {
        printf("test_06_send_file_to_virtual_directory: Error caught CCAPI_FS_ERROR_NONE\n");
    }
    else
    {
        /* This is an unexpected error */
        show_virtual_dir_error_and_stop(error);
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
    if (  strncmp(target, "test_01_add_virtual_directory_nonexistent_path", strlen(target)) == 0 )
    {
//         test_fs_virtual_dirs_1( request_buffer_info );
        test_01_add_virtual_directory_nonexistent_path( sentence );
    }
    else if (  strncmp(target, "test_02_add_virtual_directory_existent_path", strlen(target)) == 0 )
    {
        test_02_add_virtual_directory_existent_path( sentence );
    }
    else if (  strncmp(target, "test_03_add_virtual_directory_already_mapped", strlen(target)) == 0 )
    {
        test_03_add_virtual_directory_already_mapped( sentence );
    }
    else if (  strncmp(target, "test_04_remove_existent_virtual_directory", strlen(target)) == 0 )
    {
        test_04_remove_existent_virtual_directory( sentence );
    }
    else if (  strncmp(target, "test_05_remove_nonexistent_virtual_directory", strlen(target)) == 0 )
    {
        test_05_remove_nonexistent_virtual_directory( sentence );
    }
    else if (  strncmp(target, "test_06_send_file_to_virtual_directory", strlen(target)) == 0 )
    {
        test_06_send_file_to_virtual_directory( sentence );
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
