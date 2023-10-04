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

#define TEST_TARGET "my_target"

static ccapi_bool_t generic_receive_accept_cb_called = CCAPI_FALSE;

static ccapi_bool_t generic_receive_accept_cb(char const * const target, ccapi_transport_t const transport)
{
    (void)target;
    (void)transport;
    generic_receive_accept_cb_called = CCAPI_TRUE;
    return CCAPI_TRUE;
}

static void generic_receive_data_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    (void)target;
    (void)transport;
    (void)request_buffer_info;
    (void)response_buffer_info;

    return;
}

static void generic_receive_status_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error)
{

    (void)target;
    (void)transport;
    (void)response_buffer_info;
    (void)receive_error;

    return;
}

static void specific_receive_data_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    (void)target;
    (void)transport;
    (void)request_buffer_info;
    (void)response_buffer_info;

    return;
}

static void specific_receive_status_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error)
{

    (void)target;
    (void)transport;
    (void)response_buffer_info;
    (void)receive_error;

    return;
}

TEST_GROUP(test_ccapi_receive_add_target_no_CCAPI)
{
    void setup()
    {
        Mock_create_all();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_receive_add_target_no_CCAPI, testCcapiNotStarted)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target(TEST_TARGET, specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_CCAPI_NOT_RUNNING, error);

    error = ccapi_receive_remove_target(TEST_TARGET);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_CCAPI_NOT_RUNNING, error);
}

TEST_GROUP(test_ccapi_receive_add_target_no_receive_service)
{
    void setup()
    {
        Mock_create_all();
        th_start_ccapi();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_receive_add_target_no_receive_service, testCcapiNoReceiveSupport)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target(TEST_TARGET, specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT, error);

    error = ccapi_receive_remove_target(TEST_TARGET);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT, error);
}

TEST_GROUP(test_ccapi_receive_add_target)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_receive_service_t receive_service = {generic_receive_accept_cb, generic_receive_data_cb, generic_receive_status_cb};
        Mock_create_all();

        generic_receive_accept_cb_called = CCAPI_FALSE;

        th_fill_start_structure_with_good_parameters(&start);
        start.service.receive = &receive_service;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(receive_service.accept, ccapi_data_single_instance->service.receive.user_callback.accept);
        CHECK_EQUAL(receive_service.data, ccapi_data_single_instance->service.receive.user_callback.data);
        CHECK_EQUAL(receive_service.status, ccapi_data_single_instance->service.receive.user_callback.status);
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

TEST(test_ccapi_receive_add_target, testTargetNull)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target(NULL, specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_INVALID_TARGET, error);

    error = ccapi_receive_remove_target(NULL);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_INVALID_TARGET, error);
}

TEST(test_ccapi_receive_add_target, testDataCallbackNull)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target(TEST_TARGET, NULL, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_INVALID_DATA_CB, error);
}

TEST(test_ccapi_receive_add_target, testStatusCallbackNull)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target(TEST_TARGET, specific_receive_data_cb, NULL, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);
}

TEST(test_ccapi_receive_add_target, testTargetEmpty)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target("", specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);

    error = ccapi_receive_remove_target("");
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);
}

TEST(test_ccapi_receive_add_target, testTargetAlreadyAdded)
{
    ccapi_receive_error_t error;

    error = ccapi_receive_add_target(TEST_TARGET, specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);

    error = ccapi_receive_add_target(TEST_TARGET, specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_TARGET_ALREADY_ADDED, error);

    error = ccapi_receive_add_target("other_target", specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);

    error = ccapi_receive_remove_target(TEST_TARGET);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);

    error = ccapi_receive_remove_target(TEST_TARGET);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_TARGET_NOT_ADDED, error);
}

TEST(test_ccapi_receive_add_target, testTargetAdded)
{
    ccapi_receive_error_t error;

    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    error = ccapi_receive_add_target(TEST_TARGET, specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_NONE);
        CHECK(svc_receive->user_callback.data == specific_receive_data_cb);
        CHECK(svc_receive->user_callback.status == specific_receive_status_cb);
    }

    CHECK_EQUAL(CCAPI_FALSE, generic_receive_accept_cb_called);
}

TEST(test_ccapi_receive_add_target, testTargetNotAdded)
{
    ccapi_receive_error_t error;

    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    error = ccapi_receive_add_target("other_target", specific_receive_data_cb, specific_receive_status_cb, CCAPI_RECEIVE_NO_LIMIT);
    CHECK_EQUAL(CCAPI_RECEIVE_ERROR_NONE, error);

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_NONE);
        CHECK(svc_receive->user_callback.data == generic_receive_data_cb);
        CHECK(svc_receive->user_callback.status == generic_receive_status_cb);
    }

    CHECK_EQUAL(CCAPI_TRUE, generic_receive_accept_cb_called);
}
