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

TEST_GROUP(test_ccapi_dp_ccfsm_callbacks)
{
    void setup()
    {
        Mock_create_all();
        th_start_ccapi();
        th_start_tcp_lan_ipv4();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_dp_ccfsm_callbacks, testCCFSMResponseCallbackErrorMapping)
{
    connector_request_id_t request_id;
    connector_data_point_response_t dp_respose;
    connector_callback_status_t connector_status;
    ccapi_dp_transaction_info_t transaction_info;

    transaction_info.lock = ccapi_lock_create();
    dp_respose.hint = NULL;
    dp_respose.transport = connector_transport_tcp;
    dp_respose.user_context = &transaction_info;
    request_id.data_point_request = connector_request_id_data_point_response;

    dp_respose.response = connector_data_point_response_t::connector_data_point_response_success;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_respose, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, transaction_info.response_error);

    dp_respose.response = connector_data_point_response_t::connector_data_point_response_cloud_error;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_respose, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_RESPONSE_CLOUD_ERROR, transaction_info.response_error);

    dp_respose.response = connector_data_point_response_t::connector_data_point_response_bad_request;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_respose, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_RESPONSE_BAD_REQUEST, transaction_info.response_error);

    dp_respose.response = connector_data_point_response_t::connector_data_point_response_unavailable;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_respose, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_RESPONSE_UNAVAILABLE, transaction_info.response_error);
}

TEST(test_ccapi_dp_ccfsm_callbacks, testCCFSMStatusCallbackReleaseFailed)
{
    connector_request_id_t request_id;
    connector_data_point_status_t dp_status;
    connector_callback_status_t connector_status;
    ccapi_dp_transaction_info_t transaction_info;

    transaction_info.lock = ccapi_lock_create();
    dp_status.transport = connector_transport_tcp;
    dp_status.status = connector_data_point_status_t::connector_data_point_status_complete;
    dp_status.session_error = connector_session_error_none;
    dp_status.user_context = &transaction_info;
    request_id.data_point_request = connector_request_id_data_point_status;

    Mock_ccimp_os_lock_release_return(CCIMP_STATUS_ERROR);

    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_status, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, connector_status);
}

TEST(test_ccapi_dp_ccfsm_callbacks, testCCFSMStatusCallbackErrorMapping)
{
    connector_request_id_t request_id;
    connector_data_point_status_t dp_status;
    connector_callback_status_t connector_status;
    ccapi_dp_transaction_info_t transaction_info;

    transaction_info.lock = ccapi_lock_create();

    dp_status.transport = connector_transport_tcp;
    dp_status.session_error = connector_session_error_none;
    dp_status.user_context = &transaction_info;
    request_id.data_point_request = connector_request_id_data_point_status;

    dp_status.status = connector_data_point_status_t::connector_data_point_status_complete;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_status, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, transaction_info.status);

    dp_status.status = connector_data_point_status_t::connector_data_point_status_cancel;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_status, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_CANCEL, transaction_info.status);

    dp_status.status = connector_data_point_status_t::connector_data_point_status_invalid_data;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_status, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_INVALID_DATA, transaction_info.status);

    dp_status.status = connector_data_point_status_t::connector_data_point_status_session_error;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_status, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_SESSION_ERROR, transaction_info.status);

    dp_status.status = connector_data_point_status_t::connector_data_point_status_timeout;
    connector_status = ccapi_connector_callback(connector_class_id_data_point, request_id, &dp_status, (void *)ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_DP_ERROR_STATUS_TIMEOUT, transaction_info.status);
}
