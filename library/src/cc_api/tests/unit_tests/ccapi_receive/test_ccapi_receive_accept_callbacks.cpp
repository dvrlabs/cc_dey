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

static char const * ccapi_receive_accept_expected_target = NULL;
static ccapi_transport_t ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
static ccapi_bool_t ccapi_receive_accept_retval = CCAPI_FALSE;
static ccapi_bool_t ccapi_receive_accept_cb_called = CCAPI_FALSE;

static ccapi_bool_t test_receive_accept_cb(char const * const target, ccapi_transport_t const transport)
{
    STRCMP_EQUAL(ccapi_receive_accept_expected_target, target);
    CHECK_EQUAL(ccapi_receive_accept_expected_transport, transport);
    ccapi_receive_accept_cb_called = CCAPI_TRUE;
    return ccapi_receive_accept_retval;
}

static void test_receive_data_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info)
{
    (void)target;
    (void)transport;
    (void)request_buffer_info;
    (void)response_buffer_info;
}

TEST_GROUP(test_ccapi_receive_accept_callback_NoReceiveSupport)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);

        ccapi_receive_accept_expected_target = NULL;
        ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
        ccapi_receive_accept_retval = CCAPI_FALSE;
        ccapi_receive_accept_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_receive_accept_callback_NoReceiveSupport, testNoReceiveSupport)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT);
        STRCMP_EQUAL(svc_receive->target, TEST_TARGET); 
    }

    CHECK_EQUAL(CCAPI_FALSE, ccapi_receive_accept_cb_called);
}

TEST_GROUP(test_ccapi_receive_accept_callback_MissingAcceptCallback)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_receive_service_t receive_service = {NULL, test_receive_data_cb, NULL};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.receive = &receive_service;

        ccapi_receive_accept_expected_target = NULL;
        ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
        ccapi_receive_accept_retval = CCAPI_FALSE;
        ccapi_receive_accept_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(receive_service.accept, ccapi_data_single_instance->service.receive.user_callback.accept);
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

TEST(test_ccapi_receive_accept_callback_MissingAcceptCallback, testWeAllowNullAcceptCallback)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;
    
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
    }

    CHECK_EQUAL(CCAPI_FALSE, ccapi_receive_accept_cb_called);
}


TEST_GROUP(test_ccapi_receive_accept_callback_MissingCallbacks)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_receive_service_t receive_service = {NULL, NULL, NULL};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.receive = &receive_service;

        ccapi_receive_accept_expected_target = NULL;
        ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
        ccapi_receive_accept_retval = CCAPI_FALSE;
        ccapi_receive_accept_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(receive_service.accept, ccapi_data_single_instance->service.receive.user_callback.accept);
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

TEST(test_ccapi_receive_accept_callback_MissingCallbacks, testERROR_INVALID_DATA_CB)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;
    
    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_INVALID_DATA_CB);
        STRCMP_EQUAL(svc_receive->target, TEST_TARGET); 
    }

    CHECK_EQUAL(CCAPI_FALSE, ccapi_receive_accept_cb_called);
}

TEST_GROUP(test_ccapi_receive_accept_callback)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_receive_service_t receive_service = {test_receive_accept_cb, test_receive_data_cb, NULL};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.receive = &receive_service;

        ccapi_receive_accept_expected_target = NULL;
        ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
        ccapi_receive_accept_retval = CCAPI_FALSE;
        ccapi_receive_accept_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(receive_service.accept, ccapi_data_single_instance->service.receive.user_callback.accept);
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

TEST(test_ccapi_receive_accept_callback, testUserContextNotNull)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    void * fake_context = &fake_context;

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = fake_context;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_receive_target_data.user_context == fake_context);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_receive_accept_cb_called);
}

TEST(test_ccapi_receive_accept_callback, testTargetNotAccepted)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    ccapi_receive_accept_expected_target = TEST_TARGET;
    ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
    ccapi_receive_accept_retval = CCAPI_FALSE;

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_USER_REFUSED_TARGET);
        STRCMP_EQUAL(svc_receive->target, TEST_TARGET); 
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_receive_accept_cb_called);
}

TEST(test_ccapi_receive_accept_callback, testTargetEmpty)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    #define TARGET_EMPTY ""
    
    ccapi_receive_accept_expected_target = TARGET_EMPTY;
    ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
    ccapi_receive_accept_retval = CCAPI_TRUE;

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TARGET_EMPTY;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_NONE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_receive_accept_cb_called);
}

/* Cloud does not allow to send a NULL tatget. test it does not harm anyway */
TEST(test_ccapi_receive_accept_callback, testTargetNULL)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;

    #define TARGET_NULL NULL
    
    ccapi_receive_accept_expected_target = TARGET_NULL;
    ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
    ccapi_receive_accept_retval = CCAPI_TRUE;

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TARGET_NULL;
    ccfsm_receive_target_data.response_required = connector_true;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    CHECK(ccfsm_receive_target_data.user_context == NULL);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_receive_accept_cb_called);
}

TEST(test_ccapi_receive_accept_callback, testOK)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;
    
    ccapi_receive_accept_expected_target = TEST_TARGET;
    ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
    ccapi_receive_accept_retval = CCAPI_TRUE;

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
        CHECK_EQUAL(svc_receive->response_required, CCAPI_TRUE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_receive_accept_cb_called);
}

TEST(test_ccapi_receive_accept_callback, testOK_ResponseNotRequired)
{
    connector_request_id_t request;
    connector_data_service_receive_target_t ccfsm_receive_target_data;
    connector_callback_status_t status;
    
    ccapi_receive_accept_expected_target = TEST_TARGET;
    ccapi_receive_accept_expected_transport = CCAPI_TRANSPORT_TCP;
    ccapi_receive_accept_retval = CCAPI_TRUE;

    ccfsm_receive_target_data.transport = connector_transport_tcp;
    ccfsm_receive_target_data.user_context = NULL;
    ccfsm_receive_target_data.target = TEST_TARGET;
    ccfsm_receive_target_data.response_required = connector_false;

    request.data_service_request = connector_request_id_data_service_receive_target;
    status = ccapi_connector_callback(connector_class_id_data_service, request, &ccfsm_receive_target_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(ccfsm_receive_target_data.user_context != NULL);

    {
        ccapi_svc_receive_t * svc_receive = (ccapi_svc_receive_t *)ccfsm_receive_target_data.user_context;
        CHECK_EQUAL(svc_receive->receive_error, CCAPI_RECEIVE_ERROR_NONE);
        CHECK_EQUAL(svc_receive->response_required, CCAPI_FALSE);
    }

    CHECK_EQUAL(CCAPI_TRUE, ccapi_receive_accept_cb_called);
}

