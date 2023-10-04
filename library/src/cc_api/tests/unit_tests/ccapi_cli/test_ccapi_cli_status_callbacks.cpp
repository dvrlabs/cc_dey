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

#define RESPONSE  "my response"

static ccapi_bool_t ccapi_cli_finished_cb_called = CCAPI_FALSE;
static ccapi_bool_t ccapi_cli_finished_expected_output_null;
static char const * ccapi_cli_finished_expected_output;
static ccapi_cli_error_t ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_NONE;

void clean_ccapi_cli_finished_data(void)
{
    ccapi_cli_finished_cb_called = CCAPI_FALSE;
    ccapi_cli_finished_expected_output_null = CCAPI_TRUE;
    ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_NONE;
}

static void test_cli_request_cb(ccapi_transport_t const transport, char const * const command, char const * * const output)
{
    (void)transport;
    (void)command;
    (void)output;

    return;
}

static void test_cli_finished_cb(char * const output, ccapi_cli_error_t cli_error)
{
    if (!ccapi_cli_finished_expected_output_null)
    {
        CHECK_EQUAL(ccapi_cli_finished_expected_output, output);
    }
    
    CHECK_EQUAL(ccapi_cli_finished_expected_cli_error, cli_error);

    ccapi_cli_finished_cb_called = CCAPI_TRUE;

    return;
}

TEST_GROUP(test_ccapi_cli_finished_callback)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_cli_service_t cli_service = {test_cli_request_cb, test_cli_finished_cb};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.cli = &cli_service;

        clean_ccapi_cli_finished_data();

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(cli_service.request, ccapi_data_single_instance->service.cli.user_callback.request);
        CHECK_EQUAL(cli_service.finished, ccapi_data_single_instance->service.cli.user_callback.finished);
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

TEST(test_ccapi_cli_finished_callback, testStatusOK_NoResponse)
{
    connector_request_id_t request;
    connector_sm_cli_status_t ccfsm_cli_status_data;
    connector_callback_status_t status;

    ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)malloc(sizeof *svc_cli);
    svc_cli->transport = connector_transport_udp;
    svc_cli->response_required = CCAPI_TRUE;  
    svc_cli->response_string_info.string = NULL;
    svc_cli->response_string_info.length = 0;
    svc_cli->response_handled_internally = CCAPI_FALSE;
    svc_cli->cli_error = CCAPI_CLI_ERROR_NONE;
    svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;

    ccapi_cli_finished_expected_output_null = CCAPI_TRUE;
    ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_NONE;

    ccfsm_cli_status_data.transport = connector_transport_udp;
    ccfsm_cli_status_data.user_context = svc_cli;
    ccfsm_cli_status_data.status = connector_sm_cli_status_t::connector_sm_cli_status_success;

    request.sm_request = connector_request_id_sm_cli_status;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_status_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_status_data.user_context == svc_cli);
    {
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_finished_cb_called);
}

TEST(test_ccapi_cli_finished_callback, testStatusOK_WithResponse)
{
    connector_request_id_t request;
    connector_sm_cli_status_t ccfsm_cli_status_data;
    connector_callback_status_t status;

    char exp_response[] = RESPONSE;

    ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)malloc(sizeof *svc_cli);
    svc_cli->response_required = CCAPI_TRUE;  
    svc_cli->response_string_info.string = exp_response;
    svc_cli->response_string_info.length = sizeof exp_response;

    svc_cli->response_handled_internally = CCAPI_FALSE;
    svc_cli->cli_error = CCAPI_CLI_ERROR_NONE;
    svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;

    ccapi_cli_finished_expected_output_null = CCAPI_FALSE;
    ccapi_cli_finished_expected_output = exp_response;
    ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_NONE;

    ccfsm_cli_status_data.transport = connector_transport_udp;
    ccfsm_cli_status_data.user_context = svc_cli;
    ccfsm_cli_status_data.status = connector_sm_cli_status_t::connector_sm_cli_status_success;

    request.sm_request = connector_request_id_sm_cli_status;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_status_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_status_data.user_context == svc_cli);
    {
        CHECK_EQUAL(svc_cli->cli_error, CCAPI_CLI_ERROR_NONE);

        CHECK(svc_cli->response_string_info.string != NULL);
        CHECK(svc_cli->response_string_info.length == sizeof exp_response);

    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_finished_cb_called);
}

TEST(test_ccapi_cli_finished_callback, testERROR_NO_RECEIVE_SUPPORT)
{
    connector_request_id_t request;
    connector_sm_cli_status_t ccfsm_cli_status_data;
    connector_callback_status_t status;

    ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)malloc(sizeof *svc_cli);
    svc_cli->response_required = CCAPI_TRUE;  
    svc_cli->response_string_info.string = NULL;
    svc_cli->response_string_info.length = 0;
    svc_cli->response_handled_internally = CCAPI_TRUE;
    svc_cli->cli_error = CCAPI_CLI_ERROR_NO_CLI_SUPPORT;
    svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;

    ccapi_cli_finished_expected_output_null = CCAPI_TRUE;
    ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_NO_CLI_SUPPORT;

    ccfsm_cli_status_data.transport = connector_transport_udp;
    ccfsm_cli_status_data.user_context = svc_cli;
    ccfsm_cli_status_data.status = connector_sm_cli_status_t::connector_sm_cli_status_cancel;

    request.sm_request = connector_request_id_sm_cli_status;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_status_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_status_data.user_context == svc_cli);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_finished_cb_called);
}

TEST(test_ccapi_cli_finished_callback, testERROR_STATUS_CANCEL)
{
    connector_request_id_t request;
    connector_sm_cli_status_t ccfsm_cli_status_data;
    connector_callback_status_t status;

    ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)malloc(sizeof *svc_cli);
    svc_cli->response_required = CCAPI_TRUE;  
    svc_cli->response_string_info.string = NULL;
    svc_cli->response_string_info.length = 0;
    svc_cli->response_handled_internally = CCAPI_FALSE;
    svc_cli->cli_error = CCAPI_CLI_ERROR_NONE;
    svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;

    ccapi_cli_finished_expected_output_null = CCAPI_TRUE;
    ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_STATUS_CANCEL;

    ccfsm_cli_status_data.transport = connector_transport_udp;
    ccfsm_cli_status_data.user_context = svc_cli;
    ccfsm_cli_status_data.status = connector_sm_cli_status_t::connector_sm_cli_status_cancel;

    request.sm_request = connector_request_id_sm_cli_status;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_status_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_status_data.user_context == svc_cli);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_finished_cb_called);
}

TEST(test_ccapi_cli_finished_callback, testERROR_STATUS_ERROR)
{
    connector_request_id_t request;
    connector_sm_cli_status_t ccfsm_cli_status_data;
    connector_callback_status_t status;

    ccapi_svc_cli_t * svc_cli = (ccapi_svc_cli_t *)malloc(sizeof *svc_cli);
    svc_cli->response_required = CCAPI_TRUE;  
    svc_cli->response_string_info.string = NULL;
    svc_cli->response_string_info.length = 0;
    svc_cli->response_handled_internally = CCAPI_FALSE;
    svc_cli->cli_error = CCAPI_CLI_ERROR_NONE;
    svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;

    ccapi_cli_finished_expected_output_null = CCAPI_TRUE;
    ccapi_cli_finished_expected_cli_error = CCAPI_CLI_ERROR_STATUS_ERROR;

    ccfsm_cli_status_data.transport = connector_transport_udp;
    ccfsm_cli_status_data.user_context = svc_cli;
    ccfsm_cli_status_data.status = connector_sm_cli_status_t::connector_sm_cli_status_error;

    request.sm_request = connector_request_id_sm_cli_status;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_cli_status_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_cli_status_data.user_context == svc_cli);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_cli_finished_cb_called);
}
