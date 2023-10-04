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

#define STREAM_ID   "stream_id"


static void th_prepare_ccfsm_datapoint_response_and_status(ccapi_dp_collection_t * const dp_collection, connector_transport_t transport, connector_data_point_response_t * ccfsm_response, connector_data_point_status_t * ccfsm_status)
{
    static connector_request_data_point_t ccfsm_request;

    void * malloc_for_transaction = th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NORMAL, true);

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    ccfsm_response->hint = NULL;
    ccfsm_response->transport = transport;
    ccfsm_response->user_context = malloc_for_transaction;

    ccfsm_status->transport = transport;
    ccfsm_status->user_context = malloc_for_transaction;
    mock_info->connector_initiate_data_point.ccfsm_response = ccfsm_response;
    mock_info->connector_initiate_data_point.ccfsm_status = ccfsm_status;

    ccfsm_request.request_id = NULL;
    ccfsm_request.response_required = connector_false;
    ccfsm_request.timeout_in_seconds = OS_LOCK_ACQUIRE_INFINITE;
    ccfsm_request.transport = transport;
    ccfsm_request.user_context = malloc_for_transaction;
    ccfsm_request.stream = dp_collection->ccapi_data_stream_list->ccfsm_data_stream;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_data_point, &ccfsm_request, connector_success);
}

TEST_GROUP(test_ccapi_dp_send_collection)
{
    ccapi_dp_collection_handle_t dp_collection;

    void setup()
    {
        ccapi_dp_error_t dp_error;
        Mock_create_all();

        dp_error = ccapi_dp_create_collection(&dp_collection);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, STREAM_ID, CCAPI_DP_KEY_DATA_INT32);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_error = ccapi_dp_add(dp_collection, STREAM_ID, 0);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
        dp_error = ccapi_dp_add(dp_collection, STREAM_ID, 1);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        th_check_collection_dp_count(dp_collection, 2);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_dp_send_collection, testSendCollectionInvalidArgument)
{
    ccapi_dp_error_t dp_error;

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_CCAPI_NOT_RUNNING, dp_error);

    th_start_ccapi();
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_TRANSPORT_NOT_STARTED, dp_error);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_UDP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_TRANSPORT_NOT_STARTED, dp_error);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_SMS, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_TRANSPORT_NOT_STARTED, dp_error);
}

TEST(test_ccapi_dp_send_collection, testSendEmptyCollection)
{
    ccapi_dp_error_t dp_error;
    ccapi_dp_collection_handle_t empty_collection;

    dp_error = ccapi_dp_create_collection(&empty_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, empty_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionNoMemory4Transaction)
{
    ccapi_dp_error_t dp_error;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NULL, false);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_INSUFFICIENT_MEMORY, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionCreateSyncrFailed)
{
    ccapi_dp_error_t dp_error;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NORMAL, true);
    Mock_ccimp_os_lock_create_return(CCIMP_STATUS_ERROR);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_LOCK_FAILED, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionCCFSMFailure)
{
    ccapi_dp_error_t dp_error;
    connector_request_data_point_t ccfsm_request;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    void * malloc_for_transaction = th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NORMAL, true);

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    ccfsm_response.user_context = malloc_for_transaction;
    ccfsm_status.user_context = malloc_for_transaction;

    mock_info->connector_initiate_data_point.ccfsm_response = &ccfsm_response;
    mock_info->connector_initiate_data_point.ccfsm_status = &ccfsm_status;

    ccfsm_request.request_id = NULL;
    ccfsm_request.response_required = connector_false;
    ccfsm_request.timeout_in_seconds = OS_LOCK_ACQUIRE_INFINITE;
    ccfsm_request.transport = connector_transport_tcp;
    ccfsm_request.user_context = malloc_for_transaction;
    ccfsm_request.stream = dp_collection->ccapi_data_stream_list->ccfsm_data_stream;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_data_point, &ccfsm_request, connector_unavailable);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_INITIATE_ACTION_FAILED, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPOk)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_complete;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);

    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point->next, CCIMP_STATUS_OK);

    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
    th_check_collection_dp_count(dp_collection, 0);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPResponseCloudError)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_cloud_error;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_complete;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_RESPONSE_CLOUD_ERROR, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPResponseErrorUnavailable)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_unavailable;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_cancel;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_RESPONSE_UNAVAILABLE, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPResponseErrorBadRequest)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_bad_request;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_cancel;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_RESPONSE_BAD_REQUEST, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPStatusCancel)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_cancel;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_CANCEL, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPStatusInvalidData)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_invalid_data;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_INVALID_DATA, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPStatusTimeout)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_timeout;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_TIMEOUT, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionTCPStatusSessionError)
{
    ccapi_dp_error_t dp_error;
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_session_error;

    th_prepare_ccfsm_datapoint_response_and_status(dp_collection, connector_transport_tcp, &ccfsm_response, &ccfsm_status);
    dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_SESSION_ERROR, dp_error);
    th_check_collection_dp_count(dp_collection, 2);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionWithReplyTCPOk)
{
    ccapi_dp_error_t dp_error;
    connector_request_data_point_t ccfsm_request;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    void * malloc_for_transaction = th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NORMAL, true);

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    ccfsm_response.hint = "Cloud Connector for Embedded Hint";
    ccfsm_response.transport = connector_transport_tcp;
    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_response.user_context = malloc_for_transaction;

    ccfsm_status.transport = connector_transport_tcp;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_complete;
    ccfsm_status.user_context = malloc_for_transaction;
    mock_info->connector_initiate_data_point.ccfsm_response = &ccfsm_response;
    mock_info->connector_initiate_data_point.ccfsm_status = &ccfsm_status;

    ccfsm_request.request_id = NULL;
    ccfsm_request.response_required = connector_true;
    ccfsm_request.timeout_in_seconds = 30;
    ccfsm_request.transport = connector_transport_tcp;
    ccfsm_request.user_context = malloc_for_transaction;
    ccfsm_request.stream = dp_collection->ccapi_data_stream_list->ccfsm_data_stream;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_data_point, &ccfsm_request, connector_success);

    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point->next, CCIMP_STATUS_OK);

    ccapi_string_info_t hint;
    char buffer[128];

    hint.string = buffer;
    hint.length = sizeof buffer;

    dp_error = ccapi_dp_send_collection_with_reply(CCAPI_TRANSPORT_TCP, dp_collection, 30, &hint);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
    STRCMP_EQUAL(ccfsm_response.hint, hint.string);
    CHECK_EQUAL(strlen(ccfsm_response.hint), hint.length);
}

TEST(test_ccapi_dp_send_collection, testSendCollectionWithReplyTCPOkHintShort)
{
    ccapi_dp_error_t dp_error;
    connector_request_data_point_t ccfsm_request;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    void * malloc_for_transaction = th_expect_malloc(sizeof (ccapi_dp_transaction_info_t), TH_MALLOC_RETURN_NORMAL, true);

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
    connector_data_point_response_t ccfsm_response;
    connector_data_point_status_t ccfsm_status;

    ccfsm_response.hint = "123456789ABCDEF";
    ccfsm_response.transport = connector_transport_tcp;
    ccfsm_response.response = connector_data_point_response_t::connector_data_point_response_success;
    ccfsm_response.user_context = malloc_for_transaction;

    ccfsm_status.transport = connector_transport_tcp;
    ccfsm_status.status = connector_data_point_status_t::connector_data_point_status_complete;
    ccfsm_status.user_context = malloc_for_transaction;
    mock_info->connector_initiate_data_point.ccfsm_response = &ccfsm_response;
    mock_info->connector_initiate_data_point.ccfsm_status = &ccfsm_status;

    ccfsm_request.request_id = NULL;
    ccfsm_request.response_required = connector_true;
    ccfsm_request.timeout_in_seconds = 30;
    ccfsm_request.transport = connector_transport_tcp;
    ccfsm_request.user_context = malloc_for_transaction;
    ccfsm_request.stream = dp_collection->ccapi_data_stream_list->ccfsm_data_stream;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_data_point, &ccfsm_request, connector_success);

    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point->next, CCIMP_STATUS_OK);

    ccapi_string_info_t hint;
    char buffer[128];
    hint.string = buffer;
    hint.length = strlen(ccfsm_response.hint) + 1 - 5;
    char const * const expected_hint = "123456789A";

    dp_error = ccapi_dp_send_collection_with_reply(CCAPI_TRANSPORT_TCP, dp_collection, 30, &hint);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
    STRCMP_EQUAL(expected_hint, hint.string);
}
