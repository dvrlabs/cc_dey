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

TEST_GROUP(test_ccapi_tcp_stop)
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

TEST(test_ccapi_tcp_stop, testCCAPINotStarted)
{
    ccapi_tcp_stop_error_t tcp_stop_error;
    ccapi_tcp_stop_t tcp_stop = {CCAPI_TRANSPORT_STOP_GRACEFULLY};

    tcp_stop_error = ccapi_stop_transport_tcp(&tcp_stop);
    CHECK_EQUAL(CCAPI_TCP_STOP_ERROR_NOT_STARTED, tcp_stop_error);
}

TEST(test_ccapi_tcp_stop, testTCPNotStarted)
{
    ccapi_tcp_stop_error_t tcp_stop_error;
    ccapi_tcp_stop_t tcp_stop = {CCAPI_TRANSPORT_STOP_GRACEFULLY};

    th_start_ccapi();

    tcp_stop_error = ccapi_stop_transport_tcp(&tcp_stop);
    CHECK_EQUAL(CCAPI_TCP_STOP_ERROR_NOT_STARTED, tcp_stop_error);
}

TEST(test_ccapi_tcp_stop, testTCPStopGracefullyOK)
{
    ccapi_tcp_stop_error_t tcp_stop_error;
    ccapi_tcp_stop_t tcp_stop = {CCAPI_TRANSPORT_STOP_GRACEFULLY};

    th_start_ccapi();
    th_start_tcp_wan_ipv4_with_callbacks();

    connector_initiate_stop_request_t stop_data = {connector_transport_tcp, connector_wait_sessions_complete, NULL};
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_stop, &stop_data, connector_success);

    tcp_stop_error = ccapi_stop_transport_tcp(&tcp_stop);
    CHECK_EQUAL(CCAPI_TCP_STOP_ERROR_NONE, tcp_stop_error);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_data_single_instance->transport_tcp.connected);
}

TEST(test_ccapi_tcp_stop, testTCPStopImmediatelyOK)
{
    ccapi_tcp_stop_error_t tcp_stop_error;
    ccapi_tcp_stop_t tcp_stop = {CCAPI_TRANSPORT_STOP_IMMEDIATELY};

    th_start_ccapi();
    th_start_tcp_wan_ipv4_with_callbacks();

    connector_initiate_stop_request_t stop_data = {connector_transport_tcp, connector_stop_immediately, NULL};
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_stop, &stop_data, connector_success);

    tcp_stop_error = ccapi_stop_transport_tcp(&tcp_stop);
    CHECK_EQUAL(CCAPI_TCP_STOP_ERROR_NONE, tcp_stop_error);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_data_single_instance->transport_tcp.connected);
}

TEST(test_ccapi_tcp_stop, testTCPStopCcfsmError)
{
    ccapi_tcp_stop_error_t tcp_stop_error;
    ccapi_tcp_stop_t tcp_stop = {CCAPI_TRANSPORT_STOP_GRACEFULLY};

    th_start_ccapi();
    th_start_tcp_wan_ipv4_with_callbacks();

    connector_initiate_stop_request_t stop_data = {connector_transport_tcp, connector_wait_sessions_complete, NULL};
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_stop, &stop_data, connector_device_error);

    tcp_stop_error = ccapi_stop_transport_tcp(&tcp_stop);
    CHECK_EQUAL(CCAPI_TCP_STOP_ERROR_CCFSM, tcp_stop_error);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_data_single_instance->transport_tcp.connected);
}

TEST(test_ccapi_tcp_stop, testStartStopTCPMemoryLeaks)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    ccimp_os_create_thread_info_t expected_create_thread_connector_run;

    connector_handle_t handle = &handle; /* Not-NULL */
    void * const malloc_for_ccapi_data = th_expect_malloc(sizeof(ccapi_data_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof(TH_DEVICE_TYPE_STRING), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof(TH_DEVICE_CLOUD_URL_STRING), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof (ccapi_thread_info_t), TH_MALLOC_RETURN_NORMAL, true);

    Mock_connector_init_expectAndReturn(ccapi_connector_callback, handle, ccapi_data_single_instance);

    expected_create_thread_connector_run.argument = malloc_for_ccapi_data;
    expected_create_thread_connector_run.type = CCIMP_THREAD_FSM;
    /* expected_create_thread_connector_run.start */
    Mock_ccimp_os_create_thread_expectAndReturn(&expected_create_thread_connector_run, MOCK_THREAD_ENABLED_NORMAL, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);
    CHECK(error == CCAPI_START_ERROR_NONE);

    CHECK(start.vendor_id == ccapi_data_single_instance->config.vendor_id);
    CHECK(memcmp(start.device_id, ccapi_data_single_instance->config.device_id, sizeof start.device_id) == 0);
    STRCMP_EQUAL(start.device_type, ccapi_data_single_instance->config.device_type);
    STRCMP_EQUAL(start.device_cloud_url, ccapi_data_single_instance->config.device_cloud_url);
    CHECK(ccapi_data_single_instance->thread.connector_run->status == CCAPI_THREAD_RUNNING);
    CHECK(ccapi_data_single_instance->transport_tcp.info == NULL);
    CHECK(ccapi_data_single_instance->transport_udp.info == NULL);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);

    ccapi_tcp_start_error_t tcp_start_error;
    ccapi_tcp_info_t tcp_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01};

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);
    tcp_start.callback.close = NULL;
    tcp_start.callback.keepalive = NULL;
    tcp_start.connection.password = (char *)"My password";

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    th_expect_malloc(sizeof *ccapi_data_single_instance->transport_tcp.info, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof phone_number, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof "My password", TH_MALLOC_RETURN_NORMAL, true);

    tcp_start_error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, tcp_start_error);

    connector_initiate_stop_request_t stop_request;
    stop_request.transport = connector_transport_tcp;
    stop_request.condition = connector_wait_sessions_complete;
    stop_request.user_context = NULL;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_stop, &stop_request, connector_success);
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

    ccapi_stop(CCAPI_STOP_GRACEFULLY);
}
