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

static ccapi_transport_t ccapi_sm_request_connect_expected_transport;
static ccapi_bool_t ccapi_sm_request_connect_cb_called;

static void test_sm_request_connect_cb(ccapi_transport_t const transport)
{
    CHECK_EQUAL(ccapi_sm_request_connect_expected_transport, transport);

    ccapi_sm_request_connect_cb_called = CCAPI_TRUE;

    return;
}

static ccapi_transport_t ccapi_sm_ping_request_expected_transport;
static ccapi_bool_t ccapi_sm_ping_request_expected_response_required;
static ccapi_bool_t ccapi_sm_ping_request_cb_called;

static void test_sm_ping_request_cb(ccapi_transport_t const transport, ccapi_bool_t const response_required)
{
    CHECK_EQUAL(ccapi_sm_ping_request_expected_transport, transport);
    CHECK_EQUAL(ccapi_sm_ping_request_expected_response_required, response_required);

    ccapi_sm_ping_request_cb_called = CCAPI_TRUE;

    return;
}

static ccapi_transport_t ccapi_sm_unsequenced_response_expected_transport;
static uint32_t ccapi_sm_unsequenced_response_expected_id;
static void const * ccapi_sm_unsequenced_response_expected_data;
static size_t ccapi_sm_unsequenced_response_expected_bytes_used;
static ccapi_bool_t ccapi_sm_unsequenced_response_expected_error;
static ccapi_bool_t ccapi_sm_unsequenced_response_cb_called;

static void test_sm_unsequenced_response_cb(ccapi_transport_t const transport, uint32_t const id, void const * const data, size_t const bytes_used, ccapi_bool_t error)
{
    CHECK_EQUAL(ccapi_sm_unsequenced_response_expected_transport, transport);
    CHECK_EQUAL(ccapi_sm_unsequenced_response_expected_id, id);
    CHECK_EQUAL(ccapi_sm_unsequenced_response_expected_data, data);
    CHECK_EQUAL(ccapi_sm_unsequenced_response_expected_bytes_used, bytes_used);
    CHECK_EQUAL(ccapi_sm_unsequenced_response_expected_error, error);

    ccapi_sm_unsequenced_response_cb_called = CCAPI_TRUE;

    return;
}

static ccapi_transport_t ccapi_sm_pending_data_expected_transport;
static ccapi_bool_t ccapi_sm_pending_data_cb_called;

static void test_sm_pending_data_cb(ccapi_transport_t const transport)
{
    CHECK_EQUAL(ccapi_sm_pending_data_expected_transport, transport);

    ccapi_sm_pending_data_cb_called = CCAPI_TRUE;

    return;
}

static ccapi_transport_t ccapi_sm_phone_provisioning_expected_transport;
static char const * ccapi_sm_phone_provisioning_expected_phone_number;
static char const * ccapi_sm_phone_provisioning_expected_service_id;
static ccapi_bool_t ccapi_sm_phone_provisioning_expected_response_required;
static ccapi_bool_t ccapi_sm_phone_provisioning_cb_called;

static void test_sm_phone_provisioning_cb(ccapi_transport_t const transport, char const * const phone_number, char const * const service_id, ccapi_bool_t const response_required)
{
    CHECK_EQUAL(ccapi_sm_phone_provisioning_expected_transport, transport);
    CHECK_EQUAL(ccapi_sm_phone_provisioning_expected_phone_number, phone_number);
    CHECK_EQUAL(ccapi_sm_phone_provisioning_expected_service_id, service_id);
    CHECK_EQUAL(ccapi_sm_phone_provisioning_expected_response_required, response_required);

    ccapi_sm_phone_provisioning_cb_called = CCAPI_TRUE;

    return;
}

TEST_GROUP(test_ccapi_sm_callback_NoSmSupport)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);

        ccapi_sm_request_connect_cb_called = CCAPI_FALSE;
        ccapi_sm_ping_request_cb_called = CCAPI_FALSE;
        ccapi_sm_unsequenced_response_cb_called = CCAPI_FALSE;
        ccapi_sm_pending_data_cb_called = CCAPI_FALSE;
        ccapi_sm_phone_provisioning_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_sm_callback_NoSmSupport, testNoRequestConnectCallback)
{
    connector_request_id_t request;
    connector_sm_request_connect_t ccfsm_sm_request_connect;
    connector_callback_status_t status;

    ccfsm_sm_request_connect.transport = connector_transport_udp;

    request.sm_request = connector_request_id_sm_request_connect;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_request_connect, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_sm_request_connect_cb_called);
}

TEST(test_ccapi_sm_callback_NoSmSupport, testNoPingRequestCallback)
{
    connector_request_id_t request;
    connector_sm_receive_ping_request_t ccfsm_sm_ping_request;
    connector_callback_status_t status;

    ccfsm_sm_ping_request.transport = connector_transport_udp;

    request.sm_request = connector_request_id_sm_ping_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_ping_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_sm_ping_request_cb_called);
}

TEST(test_ccapi_sm_callback_NoSmSupport, testNoOpaqueResponseCallback)
{
    connector_request_id_t request;
    connector_sm_opaque_response_t ccfsm_sm_unsequenced_response;
    connector_callback_status_t status;

    ccfsm_sm_unsequenced_response.transport = connector_transport_udp;

    request.sm_request = connector_request_id_sm_ping_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_unsequenced_response, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_sm_ping_request_cb_called);
}

TEST(test_ccapi_sm_callback_NoSmSupport, testNoMoreDataCallback)
{
    connector_request_id_t request;
    connector_sm_more_data_t ccfsm_sm_more_data;
    connector_callback_status_t status;

    ccfsm_sm_more_data.transport = connector_transport_udp;

    request.sm_request = connector_request_id_sm_more_data;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_more_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_sm_pending_data_cb_called);
}

TEST(test_ccapi_sm_callback_NoSmSupport, testNoConfigRequest)
{
    connector_request_id_t request;
    connector_sm_receive_config_request_t ccfsm_sm_config_request;
    connector_callback_status_t status;

    ccfsm_sm_config_request.transport = connector_transport_udp;

    request.sm_request = connector_request_id_sm_config_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_config_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_FALSE, ccapi_sm_phone_provisioning_cb_called);
}

TEST_GROUP(test_ccapi_sm_callback)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_sm_service_t sm_service = {test_sm_request_connect_cb, test_sm_ping_request_cb, test_sm_unsequenced_response_cb, test_sm_pending_data_cb, test_sm_phone_provisioning_cb};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.sm = &sm_service;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(sm_service.request_connect, ccapi_data_single_instance->service.sm.user_callback.request_connect);
        CHECK_EQUAL(sm_service.ping_request, ccapi_data_single_instance->service.sm.user_callback.ping_request);
        CHECK_EQUAL(sm_service.unsequenced_response, ccapi_data_single_instance->service.sm.user_callback.unsequenced_response);
        CHECK_EQUAL(sm_service.pending_data, ccapi_data_single_instance->service.sm.user_callback.pending_data);
        CHECK_EQUAL(sm_service.phone_provisioning, ccapi_data_single_instance->service.sm.user_callback.phone_provisioning);
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

TEST(test_ccapi_sm_callback, testRequestConnect)
{
    connector_request_id_t request;
    connector_sm_request_connect_t ccfsm_sm_request_connect;
    connector_callback_status_t status;

    ccapi_sm_request_connect_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_sm_request_connect_cb_called = CCAPI_FALSE;

    ccfsm_sm_request_connect.transport = connector_transport_udp;
    ccfsm_sm_request_connect.allow = connector_false;

    request.sm_request = connector_request_id_sm_request_connect;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_request_connect, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_sm_request_connect_cb_called);
}

TEST(test_ccapi_sm_callback, testPingRequestResponseRequired)
{
    connector_request_id_t request;
    connector_sm_receive_ping_request_t ccfsm_sm_ping_request;
    connector_callback_status_t status;

    ccapi_sm_ping_request_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_sm_ping_request_expected_response_required = CCAPI_TRUE;
    ccapi_sm_ping_request_cb_called = CCAPI_FALSE;

    ccfsm_sm_ping_request.transport = connector_transport_udp;
    ccfsm_sm_ping_request.response_required = connector_true;

    request.sm_request = connector_request_id_sm_ping_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_ping_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_sm_ping_request_cb_called);
}

TEST(test_ccapi_sm_callback, testPingRequestResponseNotRequired)
{
    connector_request_id_t request;
    connector_sm_receive_ping_request_t ccfsm_sm_ping_request;
    connector_callback_status_t status;

    ccapi_sm_ping_request_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_sm_ping_request_expected_response_required = CCAPI_FALSE;
    ccapi_sm_ping_request_cb_called = CCAPI_FALSE;

    ccfsm_sm_ping_request.transport = connector_transport_udp;
    ccfsm_sm_ping_request.response_required = connector_false;

    request.sm_request = connector_request_id_sm_ping_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_ping_request, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_sm_ping_request_cb_called);
}

TEST(test_ccapi_sm_callback, testPingRequestOpaqueResponse)
{
    connector_request_id_t request;
    connector_sm_opaque_response_t ccfsm_sm_unsequenced_response;
    connector_callback_status_t status;

    ccapi_sm_unsequenced_response_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_sm_unsequenced_response_expected_id = 123;
    ccapi_sm_unsequenced_response_expected_data = "OPAQUE_TEST_DATA";
    ccapi_sm_unsequenced_response_expected_bytes_used = strlen((char *)ccapi_sm_unsequenced_response_expected_data);
    ccapi_sm_unsequenced_response_expected_error = CCAPI_FALSE;
    ccapi_sm_unsequenced_response_cb_called = CCAPI_FALSE;

    ccfsm_sm_unsequenced_response.transport = connector_transport_udp;
    ccfsm_sm_unsequenced_response.id = 123;
    ccfsm_sm_unsequenced_response.data = "OPAQUE_TEST_DATA";
    ccfsm_sm_unsequenced_response.bytes_used = strlen((char *)ccfsm_sm_unsequenced_response.data);
    ccfsm_sm_unsequenced_response.error = connector_false;

    request.sm_request = connector_request_id_sm_opaque_response;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_unsequenced_response, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_sm_unsequenced_response_cb_called);
}

TEST(test_ccapi_sm_callback, testMoreData)
{
    connector_request_id_t request;
    connector_sm_more_data_t ccfsm_sm_more_data;
    connector_callback_status_t status;

    ccapi_sm_pending_data_expected_transport = CCAPI_TRANSPORT_UDP;    
    ccapi_sm_pending_data_cb_called = CCAPI_FALSE;

    ccfsm_sm_more_data.transport = connector_transport_udp;

    request.sm_request = connector_request_id_sm_more_data;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_more_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_sm_pending_data_cb_called);
}

TEST(test_ccapi_sm_callback, testConfigrequest)
{
    connector_request_id_t request;
    connector_sm_receive_config_request_t ccfsm_sm_connector_sm_receive_config_request_t;
    connector_callback_status_t status;

    ccapi_sm_phone_provisioning_expected_transport = CCAPI_TRANSPORT_UDP;
    ccapi_sm_phone_provisioning_expected_phone_number = "0123456789";
    ccapi_sm_phone_provisioning_expected_service_id = "idgp";
    ccapi_sm_phone_provisioning_expected_response_required = CCAPI_FALSE;    
    ccapi_sm_phone_provisioning_cb_called = CCAPI_FALSE;

    ccfsm_sm_connector_sm_receive_config_request_t.transport = connector_transport_udp;
    ccfsm_sm_connector_sm_receive_config_request_t.phone_number = (char *)"0123456789";
    ccfsm_sm_connector_sm_receive_config_request_t.service_id = (char *)"idgp";
    ccfsm_sm_connector_sm_receive_config_request_t.response_required = connector_false;

    request.sm_request = connector_request_id_sm_config_request;
    status = ccapi_connector_callback(connector_class_id_short_message, request, &ccfsm_sm_connector_sm_receive_config_request_t, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_sm_phone_provisioning_cb_called);
}
