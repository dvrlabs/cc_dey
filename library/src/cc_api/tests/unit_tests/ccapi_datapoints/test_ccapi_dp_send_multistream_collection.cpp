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

#define STREAM_1    "FirstStream"
#define STREAM_2    "SecondStream"

TEST_GROUP(test_ccapi_dp_send_multi_stream_collection)
{
    ccapi_dp_collection_handle_t dp_collection;

    void setup()
    {
        ccapi_dp_error_t dp_error;
        Mock_create_all();

        dp_error = ccapi_dp_create_collection(&dp_collection);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, STREAM_1, CCAPI_DP_KEY_DATA_INT32);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, STREAM_2, CCAPI_DP_KEY_DATA_INT64);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_error = ccapi_dp_add(dp_collection, STREAM_1, 0);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
        dp_error = ccapi_dp_add(dp_collection, STREAM_1, 1);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_error = ccapi_dp_add(dp_collection, STREAM_2, -1);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
        dp_error = ccapi_dp_add(dp_collection, STREAM_2, -2);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
        th_check_collection_dp_count(dp_collection, 4);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_dp_send_multi_stream_collection, testSendMultiStreamCollectionTCPOk)
{
    ccapi_dp_error_t dp_error;
    connector_request_data_point_t ccfsm_request;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    void * malloc_for_transaction = th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NORMAL, true);

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    ccfsm_response.hint = NULL;
    ccfsm_response.transport = connector_transport_tcp;
    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_response.user_context = malloc_for_transaction;

    ccfsm_status.transport = connector_transport_tcp;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_complete;
    ccfsm_status.user_context = malloc_for_transaction;
    mock_info->connector_initiate_data_point.ccfsm_response = &ccfsm_response;
    mock_info->connector_initiate_data_point.ccfsm_status = &ccfsm_status;

    ccfsm_request.request_id = NULL;
    ccfsm_request.response_required = connector_false;
    ccfsm_request.timeout_in_seconds = OS_LOCK_ACQUIRE_INFINITE;
    ccfsm_request.transport = connector_transport_tcp;
    ccfsm_request.user_context = malloc_for_transaction;
    ccfsm_request.stream = dp_collection->ccapi_data_stream_list->ccfsm_data_stream;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_data_point, &ccfsm_request, connector_success);

    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point->next, CCIMP_STATUS_OK);

    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->next->ccfsm_data_stream->point, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->next->ccfsm_data_stream->point->next, CCIMP_STATUS_OK);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream != NULL);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->next != NULL);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->next->point == NULL);
    th_check_collection_dp_count(dp_collection, 0);
}
