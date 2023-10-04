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

using namespace std;

static const uint8_t device_id1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x9D, 0xFF, 0xFF, 0xAB, 0xCD, 0xEF};
static const uint8_t device_id2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x9D, 0xFF, 0xFF, 0xAB, 0xCD, 0x44};

TEST_GROUP(test_ccxapi)
{
    void setup()
    {
        Mock_create_all();

        logging_lock_users = 0;
        logging_lock = NULL;        
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccxapi, testNULLHandle)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    error = ccxapi_start(NULL, &start);

    CHECK_EQUAL(error, CCAPI_START_ERROR_NULL_PARAMETER);
}

TEST(test_ccxapi, testStartTwiceSameHandlerFails)
{
    ccapi_start_error_t start_error;
    ccapi_start_t start = {0};
    ccapi_handle_t ccapi_handle = NULL;

    th_fill_start_structure_with_good_parameters(&start);

    start_error = ccxapi_start(&ccapi_handle, &start);

    CHECK_EQUAL(start_error, CCAPI_START_ERROR_NONE);

    CHECK_EQUAL(1, logging_lock_users);
    CHECK(logging_lock != NULL);

    start_error = ccxapi_start(&ccapi_handle, &start);

    CHECK_EQUAL(start_error, CCAPI_START_ERROR_ALREADY_STARTED);

    CHECK_EQUAL(1, logging_lock_users);
    CHECK(logging_lock != NULL);
}

TEST(test_ccxapi, testStartOneInstance)
{
    ccapi_start_t start1 = {0};
    ccapi_start_error_t start1_error = CCAPI_START_ERROR_NONE;
    ccapi_stop_error_t stop1_error = CCAPI_STOP_ERROR_NONE;
    ccapi_handle_t ccapi_handle1 = NULL;

    th_fill_start_structure_with_good_parameters(&start1);

    CHECK(ccapi_data_single_instance == NULL);
    start1_error = ccxapi_start(&ccapi_handle1, &start1);
    CHECK(start1_error == CCAPI_START_ERROR_NONE);
    CHECK(ccapi_handle1 != NULL);
    CHECK(ccapi_data_single_instance == NULL);

    CHECK_EQUAL(1, logging_lock_users);
    CHECK(logging_lock != NULL);

    Mock_connector_initiate_action_expectAndReturn(((ccapi_data_t *)ccapi_handle1)->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop1_error = ccxapi_stop(ccapi_handle1, CCAPI_STOP_IMMEDIATELY);
    CHECK(stop1_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);

    CHECK_EQUAL(0, logging_lock_users);
    CHECK(logging_lock == NULL);
}

TEST(test_ccxapi, testStartTwoInstances)
{
    ccapi_start_t start1 = {0};
    ccapi_start_error_t start1_error = CCAPI_START_ERROR_NONE;
    ccapi_stop_error_t stop1_error = CCAPI_STOP_ERROR_NONE;
    ccapi_handle_t ccapi_handle1 = NULL;

    ccapi_start_t start2 = {0};
    ccapi_start_error_t start2_error = CCAPI_START_ERROR_NONE;
    ccapi_stop_error_t stop2_error = CCAPI_STOP_ERROR_NONE;
    ccapi_handle_t ccapi_handle2 = NULL;

    th_fill_start_structure_with_good_parameters(&start1);
    memcpy(start1.device_id, device_id1, sizeof start1.device_id);

    CHECK(ccapi_data_single_instance == NULL);

    start1_error = ccxapi_start(&ccapi_handle1, &start1);
    CHECK(start1_error == CCAPI_START_ERROR_NONE);
    CHECK(ccapi_handle1 != NULL);
    CHECK(ccapi_data_single_instance == NULL);

    th_fill_start_structure_with_good_parameters(&start2);
    memcpy(start2.device_id, device_id2, sizeof start2.device_id);

    CHECK(ccapi_data_single_instance == NULL);
    start2_error = ccxapi_start(&ccapi_handle2, &start2);
    CHECK(start2_error == CCAPI_START_ERROR_NONE);
    CHECK(ccapi_handle2 != NULL);
    CHECK(ccapi_data_single_instance == NULL);

    CHECK(ccapi_handle1 != ccapi_handle2);

    CHECK_EQUAL(2, logging_lock_users);
    CHECK(logging_lock != NULL);

    Mock_connector_initiate_action_expectAndReturn(((ccapi_data_t *)ccapi_handle1)->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop1_error = ccxapi_stop(ccapi_handle1, CCAPI_STOP_IMMEDIATELY);
    CHECK(stop1_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);

    CHECK_EQUAL(1, logging_lock_users);
    CHECK(logging_lock != NULL);

    Mock_connector_initiate_action_expectAndReturn(((ccapi_data_t *)ccapi_handle2)->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop2_error = ccxapi_stop(ccapi_handle2, CCAPI_STOP_IMMEDIATELY);
    CHECK(stop2_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);

    CHECK_EQUAL(0, logging_lock_users);
    CHECK(logging_lock == NULL);
}

TEST_GROUP(test_ccxapi_api)
{
    ccapi_handle_t ccapi_handle;

    void setup()
    {
        ccapi_handle = NULL;

        Mock_create_all();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccxapi_api, testStartStopApi)
{
    ccapi_handle_t ccapi_local_handle = NULL;
    ccapi_data_t * ccapi_data;
    ccapi_start_error_t start_error;
    ccapi_stop_error_t stop_error;

    ccapi_start_t start = { 0 };

    th_fill_start_structure_with_good_parameters(&start);
    start_error = ccxapi_start(&ccapi_local_handle, &start);
    CHECK_EQUAL(CCAPI_START_ERROR_NONE, start_error);

    ccapi_data = (ccapi_data_t *)ccapi_local_handle;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data->connector_handle, connector_initiate_terminate, NULL, connector_success);
    stop_error = ccxapi_stop(ccapi_local_handle, CCAPI_STOP_GRACEFULLY);
    CHECK_EQUAL(CCAPI_STOP_ERROR_NONE, stop_error);
}

TEST(test_ccxapi_api, testTransportTcpApi)
{
    ccapi_tcp_start_error_t start_error;
    ccapi_tcp_stop_error_t stop_error;

    ccapi_tcp_info_t tcp_start;
    ccapi_tcp_stop_t tcp_stop;

    start_error = ccxapi_start_transport_tcp(ccapi_handle, &tcp_start);
    stop_error = ccxapi_stop_transport_tcp(ccapi_handle, &tcp_stop);

    UNUSED_ARGUMENT(start_error);
    UNUSED_ARGUMENT(stop_error);
}

TEST(test_ccxapi_api, testTransportUdpApi)
{
    ccapi_udp_start_error_t start_error;
    ccapi_udp_stop_error_t stop_error;

    ccapi_udp_info_t udp_start;
    ccapi_udp_stop_t udp_stop;

    start_error = ccxapi_start_transport_udp(ccapi_handle, &udp_start);
    stop_error = ccxapi_stop_transport_udp(ccapi_handle, &udp_stop);

    UNUSED_ARGUMENT(start_error);
    UNUSED_ARGUMENT(stop_error);
}

TEST(test_ccxapi_api, testTransportSmsApi)
{
    ccapi_sms_start_error_t start_error;
    ccapi_sms_stop_error_t stop_error;

    ccapi_sms_info_t sms_start;
    ccapi_sms_stop_t sms_stop;

    start_error = ccxapi_start_transport_sms(ccapi_handle, &sms_start);
    stop_error = ccxapi_stop_transport_sms(ccapi_handle, &sms_stop);

    UNUSED_ARGUMENT(start_error);
    UNUSED_ARGUMENT(stop_error);
}

TEST(test_ccxapi_api, testSendApi)
{
    ccapi_send_error_t send_error;
    ccapi_string_info_t hint;

    send_error = ccxapi_send_data(ccapi_handle, CCAPI_TRANSPORT_TCP, "cloud path", "content type", "data", sizeof("data"), CCAPI_SEND_BEHAVIOR_APPEND);
    send_error = ccxapi_send_data_with_reply(ccapi_handle, CCAPI_TRANSPORT_TCP, "cloud path", "content type", "data", sizeof("data"), CCAPI_SEND_BEHAVIOR_APPEND, CCAPI_SEND_WAIT_FOREVER, &hint);
    send_error = ccxapi_send_file(ccapi_handle, CCAPI_TRANSPORT_TCP, "local path", "cloud path", "content type", CCAPI_SEND_BEHAVIOR_APPEND);
    send_error = ccxapi_send_file_with_reply(ccapi_handle, CCAPI_TRANSPORT_TCP, "local path", "cloud path", "content type", CCAPI_SEND_BEHAVIOR_APPEND, CCAPI_SEND_WAIT_FOREVER, &hint);

    UNUSED_ARGUMENT(send_error);
}

TEST(test_ccxapi_api, testReceiveApi)
{
    ccapi_receive_error_t receive_error;

    receive_error = ccxapi_receive_add_target(ccapi_handle, "target", (ccapi_receive_data_cb_t)NULL, (ccapi_receive_status_cb_t)NULL, CCAPI_RECEIVE_NO_LIMIT);
    receive_error = ccxapi_receive_remove_target(ccapi_handle, "target");

    UNUSED_ARGUMENT(receive_error);
}

TEST(test_ccxapi_api, testFilesystemApi)
{
    ccapi_fs_error_t fs_error;

    fs_error = ccxapi_fs_add_virtual_dir(ccapi_handle, "virtual_dir", "actual_dir");
    fs_error = ccxapi_fs_remove_virtual_dir(ccapi_handle, "virtual_dir");

    UNUSED_ARGUMENT(fs_error);
}

TEST(test_ccxapi_api, testDataPointBinaryApi)
{
    ccapi_dp_b_error_t dp_b_error;
    ccapi_string_info_t hint;

    dp_b_error = ccxapi_dp_binary_send_data(ccapi_handle, CCAPI_TRANSPORT_TCP, "stream_id", "data", sizeof("data"));
    dp_b_error = ccxapi_dp_binary_send_data_with_reply(ccapi_handle, CCAPI_TRANSPORT_TCP, "stream_id", "data", sizeof("data"), CCAPI_DP_B_WAIT_FOREVER, &hint);
    dp_b_error = ccxapi_dp_binary_send_file(ccapi_handle, CCAPI_TRANSPORT_TCP, "local_path", "stream_id");
    dp_b_error = ccxapi_dp_binary_send_file_with_reply(ccapi_handle, CCAPI_TRANSPORT_TCP, "local_path", "stream_id", CCAPI_DP_B_WAIT_FOREVER, &hint);

    UNUSED_ARGUMENT(dp_b_error);
}

TEST(test_ccxapi_api, testDataPointApi)
{
    ccapi_dp_error_t dp_error;
    ccapi_string_info_t hint;
    ccapi_dp_collection_t dp_collection;

    dp_error = ccxapi_dp_send_collection(ccapi_handle, CCAPI_TRANSPORT_TCP, &dp_collection);
    dp_error = ccxapi_dp_send_collection_with_reply(ccapi_handle, CCAPI_TRANSPORT_TCP, &dp_collection, CCAPI_DP_WAIT_FOREVER, &hint);

    UNUSED_ARGUMENT(dp_error);
}

TEST(test_ccxapi_api, testPingApi)
{
    ccapi_ping_error_t ping_error;

    ping_error = ccxapi_send_ping(ccapi_handle, CCAPI_TRANSPORT_TCP);
    ping_error = ccxapi_send_ping_with_reply(ccapi_handle, CCAPI_TRANSPORT_TCP, CCAPI_SEND_PING_WAIT_FOREVER);

    UNUSED_ARGUMENT(ping_error);
}

