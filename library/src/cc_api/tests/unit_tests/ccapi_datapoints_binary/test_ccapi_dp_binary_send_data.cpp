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

#define STREAM_ID   "test_stream"
#define CLOUD_PATH   "DataPoint/test_stream.bin"

#define DATA  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, \
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f }

TEST_GROUP(test_ccapi_datapoint_binary_no_reply)
{
    void setup()
    {
        Mock_create_all();

        th_start_ccapi();

        th_start_tcp_lan_ipv4();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_datapoint_binary_no_reply, testDP_B_ERROR_NONE)
{
    ccapi_dp_b_error_t error;

    connector_request_data_service_send_t header;
    char const data[] = DATA;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    header.transport = connector_transport_tcp;
    header.option = connector_request_data_service_send_t::connector_data_service_send_option_overwrite;
    header.content_type = NULL;
    header.path  = CLOUD_PATH;
    header.response_required = connector_false;
    header.timeout_in_seconds = CCAPI_DP_B_WAIT_FOREVER;
    header.request_id = NULL;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_send_data, &header, connector_success);

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    CHECK(0 == memcmp(data, mock_info->connector_initiate_send_data_info.out.data, sizeof data));
}

TEST(test_ccapi_datapoint_binary_no_reply, testChunkSizeEqual)
{
    ccapi_dp_b_error_t error;

    connector_request_data_service_send_t header;
    char const data[] = DATA;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    header.transport = connector_transport_tcp;
    header.option = connector_request_data_service_send_t::connector_data_service_send_option_overwrite;
    header.path  = CLOUD_PATH;
    header.content_type = NULL;
    header.response_required = connector_false;
    header.timeout_in_seconds = CCAPI_DP_B_WAIT_FOREVER;
    header.request_id = NULL;

    mock_info->connector_initiate_send_data_info.in.chunk_size = sizeof data;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_send_data, &header, connector_success);

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    CHECK(0 == memcmp(data, mock_info->connector_initiate_send_data_info.out.data, sizeof data));
}

TEST(test_ccapi_datapoint_binary_no_reply, testChunkSizeSmall)
{
    ccapi_dp_b_error_t error;

    connector_request_data_service_send_t header;
    char const data[] = DATA;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    header.transport = connector_transport_tcp;
    header.option = connector_request_data_service_send_t::connector_data_service_send_option_overwrite;
    header.path  = CLOUD_PATH;
    header.content_type = NULL;
    header.response_required = connector_false;
    header.timeout_in_seconds = CCAPI_DP_B_WAIT_FOREVER;
    header.request_id = NULL;

    mock_info->connector_initiate_send_data_info.in.chunk_size = sizeof data / 4 - 1; /* Don't allocate enough space so data callback is called several times */

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_send_data, &header, connector_success);

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    CHECK(0 == memcmp(data, mock_info->connector_initiate_send_data_info.out.data, sizeof data));
}

TEST(test_ccapi_datapoint_binary_no_reply, testChunkSizeSmallBinary)
{
    ccapi_dp_b_error_t error;

    connector_request_data_service_send_t header;
    #define TEST_SIZE 600
    static uint8_t data[TEST_SIZE];
    unsigned int i;

    for (i=0 ; i < TEST_SIZE ; i++)
        data[i] = i;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    header.transport = connector_transport_tcp;
    header.option = connector_request_data_service_send_t::connector_data_service_send_option_overwrite;
    header.path  = CLOUD_PATH;
    header.content_type = NULL;
    header.response_required = connector_false;
    header.timeout_in_seconds = CCAPI_DP_B_WAIT_FOREVER;
    header.request_id = NULL;

    mock_info->connector_initiate_send_data_info.in.chunk_size = TEST_SIZE / 10 - 3; /* Don't allocate enough space so data callback is called several times */

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_send_data, &header, connector_success);

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, TEST_SIZE);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    CHECK(0 == memcmp(data, mock_info->connector_initiate_send_data_info.out.data, TEST_SIZE));
}

TEST(test_ccapi_datapoint_binary_no_reply, testSEND_ERROR_STATUS_CANCEL)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_cancel;
    }

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_STATUS_CANCEL, error);
}

TEST(test_ccapi_datapoint_binary_no_reply, testDP_B_ERROR_STATUS_TIMEOUT)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_timeout;
    }

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_STATUS_TIMEOUT, error);
}

TEST(test_ccapi_datapoint_binary_no_reply, testDP_B_ERROR_STATUS_SESSION_ERROR)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_session_error;
    }

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_STATUS_SESSION_ERROR, error);
}
