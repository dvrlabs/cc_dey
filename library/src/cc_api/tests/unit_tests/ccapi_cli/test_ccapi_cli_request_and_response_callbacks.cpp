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

#include "test_helper_functions.h"

#define TEST_COMMAND "help"
#define TEST_COMMAND_SIZE (sizeof(TEST_COMMAND))

static ccapi_transport_t ccapi_cli_request_expected_transport;
static char const * ccapi_cli_request_expected_command;
static ccapi_bool_t ccapi_cli_request_expected_output_null;
static char const * ccapi_cli_request_desired_output;
static ccapi_bool_t ccapi_cli_request_cb_called;
static ccapi_bool_t ccapi_cli_request_lock_cb[2] = {CCAPI_FALSE, CCAPI_FALSE};
static unsigned int ccapi_cli_request_lock_index = 0;

static void test_cli_request_cb(ccapi_transport_t const transport, char const * const command, char const * * const output)
{
    CHECK_EQUAL(ccapi_cli_request_expected_transport, transport);
    STRCMP_EQUAL(ccapi_cli_request_expected_command, command);
    CHECK(ccapi_cli_request_expected_output_null ? output == NULL : output != NULL);

    if (output != NULL)
    {
        *output = ccapi_cli_request_desired_output;
    }

    ccapi_cli_request_cb_called = CCAPI_TRUE;

    while (ccapi_cli_request_lock_cb[ccapi_cli_request_lock_index])
    {
        sched_yield();
    }
    ccapi_cli_request_lock_index++;

    return;
}

TEST_GROUP(test_ccapi_cli_request_callback_NoCliSupport)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);

        ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
        ccapi_cli_request_expected_command = NULL;
        ccapi_cli_request_expected_output_null = CCAPI_TRUE;
        ccapi_cli_request_cb_called = CCAPI_FALSE;
        ccapi_cli_request_lock_cb[0] = CCAPI_FALSE;
        ccapi_cli_request_lock_cb[1] = CCAPI_FALSE;
        ccapi_cli_request_lock_index = 0;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_cli_request_callback_NoCliSupport, testNoCliSupport)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_callback_status_t status;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_COMMAND;
    ccfsm_cli_request.bytes_used = TEST_COMMAND_SIZE;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NO_CLI_SUPPORT);
    }

    CHECK_EQUAL(CCAPI_FALSE, ccapi_cli_request_cb_called);
}

TEST_GROUP(test_ccapi_cli_request_callback)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_cli_service_t cli_service = {test_cli_request_cb, NULL};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.cli = &cli_service;

        ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
        ccapi_cli_request_expected_command = NULL;
        ccapi_cli_request_expected_output_null = CCAPI_TRUE;
        ccapi_cli_request_desired_output = NULL;
        ccapi_cli_request_cb_called = CCAPI_FALSE;
        ccapi_cli_request_lock_cb[0] = CCAPI_FALSE;
        ccapi_cli_request_lock_cb[1] = CCAPI_FALSE;
        ccapi_cli_request_lock_index = 0;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(cli_service.request, ccapi_data_single_instance->service.cli.user_callback.request);
    }

    void teardown()
    {
        ccapi_stop_error_t stop_error;

        Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
        CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
        CHECK(ccapi_data_single_instance == NULL);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_cli_request_callback, testRequestEmpty)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_callback_status_t status;

    #define REQUEST_EMPTY ""

    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_cli_request_expected_command = REQUEST_EMPTY;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = REQUEST_EMPTY;
    ccfsm_cli_request.bytes_used = sizeof(REQUEST_EMPTY);
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);
}

/* Cloud does not allow to send a NULL request. test it does not harm anyway */
TEST(test_ccapi_cli_request_callback, testRequestNULL)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_callback_status_t status;

    #define TARGET_NULL NULL

    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_cli_request_expected_command = TARGET_NULL;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TARGET_NULL;
    ccfsm_cli_request.bytes_used = 0;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_cli_request.user_context == NULL);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_cli_request_cb_called);
}

TEST(test_ccapi_cli_request_callback, testOneRequest)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_callback_status_t status;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_COMMAND;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_COMMAND;
    ccfsm_cli_request.bytes_used = TEST_COMMAND_SIZE;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);
}

TEST(test_ccapi_cli_request_callback, testOneRequestBusy)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_callback_status_t status;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_COMMAND;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_COMMAND;
    ccfsm_cli_request.bytes_used = TEST_COMMAND_SIZE;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;

    ccapi_cli_request_lock_cb[0] = CCAPI_TRUE;

    request.sm_request = connector_request_id_sm_cli_request;
    {
        unsigned int i = 0;
        do
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
            i++;
            if (i == 1000)
            {
                ccapi_cli_request_lock_cb[0] = CCAPI_FALSE;
            }
        } while ( status == connector_callback_busy);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(i > 1000);
    }

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);
}

#define TEST_2_COMMAND "1234567890"
#define TEST_2_COMMAND_SIZE (sizeof(TEST_2_COMMAND))
#define TEST_2_COMMAND_A "12345"
#define TEST_2_COMMAND_B "67890"

TEST(test_ccapi_cli_request_callback, testTwoRequests)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_callback_status_t status;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_2_COMMAND;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_2_COMMAND_A;
    ccfsm_cli_request.bytes_used = strlen(TEST_2_COMMAND_A);
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_true;

    request.sm_request = connector_request_id_sm_cli_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_FALSE, ccapi_cli_request_cb_called);

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = ccfsm_cli_request.user_context;
    ccfsm_cli_request.buffer = TEST_2_COMMAND_B;
    ccfsm_cli_request.bytes_used = strlen(TEST_2_COMMAND_B) + 1;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);


}

TEST(test_ccapi_cli_request_callback, testTwoConcurrentRequests)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_sm_cli_request_t ccfsm_cli_request2;
    connector_callback_status_t status;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_2_COMMAND_A;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_2_COMMAND_A;
    ccfsm_cli_request.bytes_used = strlen(TEST_2_COMMAND_A) + 1;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;
    ccfsm_cli_request2.transport = connector_transport_udp;
    ccfsm_cli_request2.user_context = NULL;
    ccfsm_cli_request2.buffer = TEST_2_COMMAND_B;
    ccfsm_cli_request2.bytes_used = strlen(TEST_2_COMMAND_B) + 1;
    ccfsm_cli_request2.response_required = connector_false;
    ccfsm_cli_request2.more_data = connector_false;

    ccapi_cli_request_lock_cb[0] = CCAPI_TRUE;
    ccapi_cli_request_lock_cb[1] = CCAPI_TRUE;

    request.sm_request = connector_request_id_sm_cli_request;
    {
        unsigned int i = 0;
        for (i=0 ; i < 1000 ; i++)
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
        }
        CHECK_EQUAL(connector_callback_busy, status);

        for (i=0 ; i < 1000 ; i++)
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request2, ccapi_data_single_instance);
        }
        CHECK_EQUAL(connector_callback_busy, status);

        ccapi_cli_request_lock_cb[0] = CCAPI_FALSE;

        do
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
        } while ( status == connector_callback_busy);
        CHECK_EQUAL(connector_callback_continue, status);
    }


    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);

    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_2_COMMAND_B;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccapi_cli_request_lock_cb[1] = CCAPI_FALSE;

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request2, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request2.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request2.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);
}

TEST(test_ccapi_cli_request_callback, testTwoConcurrentRequestsNotProccesedInOrder)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_sm_cli_request_t ccfsm_cli_request2;
    connector_callback_status_t status;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_2_COMMAND_A;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_2_COMMAND_A;
    ccfsm_cli_request.bytes_used = strlen(TEST_2_COMMAND_A) + 1;
    ccfsm_cli_request.response_required = connector_false;
    ccfsm_cli_request.more_data = connector_false;
    ccfsm_cli_request2.transport = connector_transport_udp;
    ccfsm_cli_request2.user_context = NULL;
    ccfsm_cli_request2.buffer = TEST_2_COMMAND_B;
    ccfsm_cli_request2.bytes_used = strlen(TEST_2_COMMAND_B) + 1;
    ccfsm_cli_request2.response_required = connector_false;
    ccfsm_cli_request2.more_data = connector_false;

    ccapi_cli_request_lock_cb[0] = CCAPI_TRUE;
    ccapi_cli_request_lock_cb[1] = CCAPI_TRUE;

    request.sm_request = connector_request_id_sm_cli_request;
    {
        do
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
            sched_yield();
        } while (!ccapi_cli_request_cb_called);
        CHECK_EQUAL(connector_callback_busy, status);

        do
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request2, ccapi_data_single_instance);
            sched_yield();
        } while (!ccapi_cli_request_cb_called);
        CHECK_EQUAL(connector_callback_busy, status);

        CHECK_EQUAL(ccfsm_cli_request.user_context, ccapi_data_single_instance->service.cli.svc_cli);
        CHECK_EQUAL(CCAPI_CLI_THREAD_REQUEST_CB_QUEUED, ccapi_data_single_instance->service.cli.svc_cli->cli_thread_status);

        CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);

        ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
        ccapi_cli_request_expected_command = TEST_2_COMMAND_B;
        ccapi_cli_request_expected_output_null = CCAPI_TRUE;
        ccapi_cli_request_cb_called = CCAPI_FALSE;

        ccapi_cli_request_lock_cb[0] = CCAPI_FALSE;

        do
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request2, ccapi_data_single_instance);
            sched_yield();
        } while (!ccapi_cli_request_cb_called);
        CHECK_EQUAL(connector_callback_busy, status);

        CHECK_EQUAL(ccfsm_cli_request2.user_context, ccapi_data_single_instance->service.cli.svc_cli);
        CHECK_EQUAL(CCAPI_CLI_THREAD_REQUEST_CB_QUEUED, ccapi_data_single_instance->service.cli.svc_cli->cli_thread_status);

        ccapi_cli_request_lock_cb[1] = CCAPI_FALSE;

        do
        {      
            status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request2, ccapi_data_single_instance);
            sched_yield();
        } while ( status == connector_callback_busy);
        CHECK_EQUAL(connector_callback_continue, status);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);
    CHECK(ccapi_data_single_instance->service.cli.svc_cli == NULL);

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);
}

#define TEST_OUTPUT "this is my response string"

TEST(test_ccapi_cli_request_callback, testOneResponse)
{
    connector_request_id_t request;
    connector_callback_status_t status;

    connector_sm_cli_request_t ccfsm_cli_request;

    char const exp_response[] = TEST_OUTPUT;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_COMMAND;
    ccapi_cli_request_expected_output_null = CCAPI_FALSE;
    ccapi_cli_request_desired_output = exp_response;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_COMMAND;
    ccfsm_cli_request.bytes_used = TEST_COMMAND_SIZE;
    ccfsm_cli_request.response_required = connector_true;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_TRUE);

        CHECK(svc_cli->request_string_info.string != NULL);
        CHECK(svc_cli->request_string_info.length == TEST_COMMAND_SIZE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);

    {
        connector_sm_cli_response_t ccfsm_cli_response;

        #define MAX_RESPONSE_SIZE 100
        char response[MAX_RESPONSE_SIZE];

        ccfsm_cli_response.transport = connector_transport_udp;
        ccfsm_cli_response.user_context = ccfsm_cli_request.user_context;
        ccfsm_cli_response.buffer = response;
        ccfsm_cli_response.bytes_available = MAX_RESPONSE_SIZE;
        ccfsm_cli_response.bytes_used = 0;
        ccfsm_cli_response.more_data = connector_false;

        request.sm_request = connector_request_id_sm_cli_response;
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_response, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(ccfsm_cli_request.user_context != NULL);

        {
            ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_response.user_context;
            CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
            CHECK_EQUAL(svc_cli->response_required, CCAPI_TRUE);

            CHECK(svc_cli->response_string_info.string != NULL);
            CHECK(svc_cli->response_string_info.length == sizeof exp_response);
            CHECK(svc_cli->response_processing.length == 0);

            STRCMP_EQUAL(exp_response, response);
            CHECK(ccfsm_cli_response.bytes_used == sizeof exp_response);
            CHECK(ccfsm_cli_response.more_data == connector_false);
        }

        CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);
    }
}

TEST(test_ccapi_cli_request_callback, testTwoResponses)
{
    connector_request_id_t request;
    connector_callback_status_t status;

    connector_sm_cli_request_t ccfsm_cli_request;

    char const exp_response[] = TEST_OUTPUT;
    
    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_COMMAND;
    ccapi_cli_request_expected_output_null = CCAPI_FALSE;
    ccapi_cli_request_desired_output = exp_response;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_COMMAND;
    ccfsm_cli_request.bytes_used = TEST_COMMAND_SIZE;
    ccfsm_cli_request.response_required = connector_true;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    do
    {
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_request.user_context != NULL);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
        CHECK_EQUAL(svc_cli->response_required, CCAPI_TRUE);

        CHECK(svc_cli->request_string_info.string != NULL);
        CHECK(svc_cli->request_string_info.length == TEST_COMMAND_SIZE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_request_cb_called);

    {
        connector_sm_cli_response_t ccfsm_cli_response;

        #define MAX_RESPONSE_SIZE 100
        char response[MAX_RESPONSE_SIZE];

        #define CALL1_SIZE 5
        #define CALL2_SIZE (sizeof exp_response - CALL1_SIZE)

        ccfsm_cli_response.transport = connector_transport_udp;
        ccfsm_cli_response.user_context = ccfsm_cli_request.user_context;
        ccfsm_cli_response.buffer = response;
        ccfsm_cli_response.bytes_available = CALL1_SIZE;
        ccfsm_cli_response.bytes_used = 0;
        ccfsm_cli_response.more_data = connector_false;

        request.sm_request = connector_request_id_sm_cli_response;
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_response, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(ccfsm_cli_request.user_context != NULL);

        {
            ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_response.user_context;
            CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
            CHECK_EQUAL(svc_cli->response_required, CCAPI_TRUE);

            CHECK(svc_cli->response_string_info.string != NULL);
            CHECK(svc_cli->response_string_info.length == sizeof exp_response);
            CHECK(svc_cli->response_processing.length == CALL2_SIZE);

            CHECK(ccfsm_cli_response.bytes_used == CALL1_SIZE);
            CHECK(ccfsm_cli_response.more_data == connector_true);
        }

        ccfsm_cli_response.transport = connector_transport_udp;
        ccfsm_cli_response.user_context = ccfsm_cli_request.user_context;
        ccfsm_cli_response.buffer = response + CALL1_SIZE;
        ccfsm_cli_response.bytes_available = MAX_RESPONSE_SIZE - CALL1_SIZE;
        ccfsm_cli_response.bytes_used = 0;
        ccfsm_cli_response.more_data = connector_false;

        request.sm_request = connector_request_id_sm_cli_response;
        status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_response, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(ccfsm_cli_request.user_context != NULL);

        {
            ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_response.user_context;
            CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
            CHECK_EQUAL(svc_cli->response_required, CCAPI_TRUE);

            CHECK(svc_cli->response_string_info.string != NULL);
            CHECK(svc_cli->response_string_info.length == sizeof exp_response);
            CHECK(svc_cli->response_processing.length == 0);

            STRCMP_EQUAL(exp_response, response);
            CHECK(ccfsm_cli_response.bytes_used == CALL2_SIZE);
            CHECK(ccfsm_cli_response.more_data == connector_false);
        }
    }
}

TEST(test_ccapi_cli_request_callback, testERROR_NO_CLI_SUPPORT)
{
    connector_request_id_t request;
    connector_sm_cli_request_t ccfsm_cli_request;
    connector_sm_cli_response_t ccfsm_cli_response;
    connector_callback_status_t status;

    char const exp_response[] = "CCAPI Error 1 (CCAPI_CLI_ERROR_NO_CLI_SUPPORT)";

    #define MAX_RESPONSE_SIZE 100
    char response[MAX_RESPONSE_SIZE];

    ccapi_data_single_instance->config.cli_supported = CCAPI_FALSE;

    ccapi_cli_request_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_cli_request_expected_command = TEST_COMMAND;
    ccapi_cli_request_expected_output_null = CCAPI_TRUE;
    ccapi_cli_request_desired_output = NULL;
    ccapi_cli_request_cb_called = CCAPI_FALSE;

    ccfsm_cli_request.transport = connector_transport_udp;
    ccfsm_cli_request.user_context = NULL;
    ccfsm_cli_request.buffer = TEST_COMMAND;
    ccfsm_cli_request.bytes_used = TEST_COMMAND_SIZE;
    ccfsm_cli_request.response_required = connector_true;
    ccfsm_cli_request.more_data = connector_false;

    request.sm_request = connector_request_id_sm_cli_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_cli_request_cb_called);

    ccfsm_cli_response.transport = connector_transport_udp;
    ccfsm_cli_response.user_context = ccfsm_cli_request.user_context;
    ccfsm_cli_response.buffer = response;
    ccfsm_cli_response.bytes_available = MAX_RESPONSE_SIZE;
    ccfsm_cli_response.bytes_used = 0;
    ccfsm_cli_response.more_data = connector_true;

    request.sm_request = connector_request_id_sm_cli_response;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_response, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    {
        ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)ccfsm_cli_request.user_context;
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NO_CLI_SUPPORT);

        CHECK(svc_cli->request_string_info.string == NULL);

        CHECK(svc_cli->response_string_info.string != NULL);
        STRCMP_EQUAL((char*)svc_cli->response_string_info.string, (char*)exp_response);
        CHECK_EQUAL(svc_cli->response_string_info.length, sizeof exp_response - 1);

        CHECK(svc_cli->response_processing.length == 0);

        CHECK(ccfsm_cli_response.bytes_used == sizeof exp_response - 1);
        CHECK(ccfsm_cli_response.more_data == connector_false);
    }

    CHECK_EQUAL(CCAPI_FALSE, ccapi_cli_request_cb_called);

    ccapi_data_single_instance->config.cli_supported = CCAPI_TRUE;
}

/* TODO: Add following test if we decide to have a max_request_size field:
         TEST(test_ccapi_receive_data_callback, testRequestTooBig)
*/
