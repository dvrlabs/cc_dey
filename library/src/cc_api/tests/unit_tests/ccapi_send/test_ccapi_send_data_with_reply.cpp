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

#define CLOUD_PATH   "test/test.txt"
#define CONTENT_TYPE "text/plain"
#define DATA  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, \
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f }

TEST_GROUP(test_ccapi_send_data_with_reply)
{
    static ccapi_send_error_t error;

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

TEST(test_ccapi_send_data_with_reply, testTimeoutOkNoHint)
{
    ccapi_send_error_t error;

    connector_request_data_service_send_t header;
    char const data[] = DATA;
    unsigned long timeout = 10;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    header.transport = connector_transport_tcp;
    header.option = connector_request_data_service_send_t::connector_data_service_send_option_overwrite;
    header.path  = CLOUD_PATH;
    header.content_type = CONTENT_TYPE;
    header.response_required = connector_true;
    header.timeout_in_seconds = timeout;
    header.request_id = NULL;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_send_data, &header, connector_success);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, timeout, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    CHECK(0 == memcmp(data, mock_info->connector_initiate_send_data_info.out.data, sizeof data));
}

TEST(test_ccapi_send_data_with_reply, testHint)
{
    ccapi_send_error_t error;

    connector_request_data_service_send_t header;
    char const data[] = DATA;
    char const hint_check[] = "hello man";
    unsigned long timeout = CCAPI_SEND_WAIT_FOREVER;

    ccapi_string_info_t hint;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    header.transport = connector_transport_tcp;
    header.option = connector_request_data_service_send_t::connector_data_service_send_option_overwrite;
    header.path  = CLOUD_PATH;
    header.content_type = CONTENT_TYPE;
    header.response_required = connector_true;
    header.timeout_in_seconds = timeout;
    header.request_id = NULL;

    hint.length = 10;
    hint.string = (char*)malloc(hint.length);

    mock_info->connector_initiate_send_data_info.in.hint = hint_check;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_send_data, &header, connector_success);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, timeout, &hint);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    CHECK(0 == memcmp(data, mock_info->connector_initiate_send_data_info.out.data, sizeof data));

    CHECK(0 == strcmp(hint.string, hint_check));
}

/* This test compared with 'testHint': Forces ccfsm to call response with a hint but the user has not provided a hint buffer */
TEST(test_ccapi_send_data_with_reply, testHintCanBeNull)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    char const hint_check[] = "hello man";

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    mock_info->connector_initiate_send_data_info.in.hint = hint_check;

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

}

TEST(test_ccapi_send_data_with_reply, testHintNoEnoughtRoom)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    char const hint_check[] = "testHintNoEnoughtRoom";

    ccapi_string_info_t hint;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    hint.length = 10;
    hint.string = (char*)malloc(hint.length);

    mock_info->connector_initiate_send_data_info.in.hint = hint_check;

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, &hint);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    CHECK(0 == memcmp(hint.string, hint_check,  hint.length - 1));
    CHECK('\0' == hint.string[hint.length - 1]);
}

TEST(test_ccapi_send_data_with_reply, testHintJustEnoughtRoom)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    char const hint_check[] = "testHintJustEnoughtRoom";

    ccapi_string_info_t hint;

    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);

    hint.length = strlen(hint_check) + 1;
    hint.string = (char*)malloc(hint.length);

    mock_info->connector_initiate_send_data_info.in.hint = hint_check;

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, &hint);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    CHECK(0 == memcmp(hint.string, hint_check,  hint.length - 1));
    CHECK('\0' == hint.string[hint.length - 1]);
    CHECK(0 == strcmp(hint.string, hint_check));
}

TEST(test_ccapi_send_data_with_reply, testSEND_ERROR_RESPONSE_BAD_REQUEST)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.response = connector_data_service_send_response_t::connector_data_service_send_response_bad_request;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_RESPONSE_BAD_REQUEST, error);
}

TEST(test_ccapi_send_data_with_reply, testSEND_ERROR_RESPONSE_UNAVAILABLE)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.response = connector_data_service_send_response_t::connector_data_service_send_response_unavailable;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_RESPONSE_UNAVAILABLE, error);
}

TEST(test_ccapi_send_data_with_reply, testSEND_ERROR_RESPONSE_CLOUD_ERROR)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.response = connector_data_service_send_response_t::connector_data_service_send_response_cloud_error;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_RESPONSE_CLOUD_ERROR, error);
}

TEST(test_ccapi_send_data_with_reply, testResponseErrorHasPriorityOverStatusError)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.response = connector_data_service_send_response_t::connector_data_service_send_response_cloud_error;
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_timeout;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_RESPONSE_CLOUD_ERROR, error);
}


TEST(test_ccapi_send_data_with_reply, testSEND_ERROR_STATUS_CANCEL)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_cancel;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_STATUS_CANCEL, error);
}

TEST(test_ccapi_send_data_with_reply, testSEND_ERROR_STATUS_TIMEOUT)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_timeout;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_STATUS_TIMEOUT, error);
}

TEST(test_ccapi_send_data_with_reply, testSEND_ERROR_STATUS_SESSION_ERROR)
{
    ccapi_send_error_t error;
    char const data[] = DATA;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_session_error;
    }

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, data, sizeof data, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_STATUS_SESSION_ERROR, error);
}
