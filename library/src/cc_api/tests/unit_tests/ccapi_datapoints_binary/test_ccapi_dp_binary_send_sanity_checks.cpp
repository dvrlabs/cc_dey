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
#define DATA  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, \
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f }
#define LOCAL_PATH   "./test_ccapi_dp_binary_send_data_common_sanity_checks.txt"

/* This group doesn't call ccapi_start/stop functions */
TEST_GROUP(ccapi_datapoint_binary_with_no_ccapi) 
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

TEST(ccapi_datapoint_binary_with_no_ccapi, testCcapiNotStarted)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_CCAPI_NOT_RUNNING, error);
}

/* This group doesn't call ccapi_transport_tcp_start function */
TEST_GROUP(ccapi_datapoint_binary_with_no_transport_start) 
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

TEST(ccapi_datapoint_binary_with_no_transport_start, testTransportNotStarted)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED, error);
}

TEST_GROUP(test_ccapi_datapoint_binary_common_sanity_checks)
{
    static ccapi_dp_b_error_t error;

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

TEST(test_ccapi_datapoint_binary_common_sanity_checks, testNullStreamId)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, NULL, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_STREAM_ID, error);
}

TEST(test_ccapi_datapoint_binary_common_sanity_checks, testEmptyStreamId)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, "", data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_STREAM_ID, error);
}

TEST(test_ccapi_datapoint_binary_common_sanity_checks, testUdpTransportNotStarted)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_UDP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED, error);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_UDP, STREAM_ID, data, sizeof data, 0, NULL);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED, error);
}

TEST(test_ccapi_datapoint_binary_common_sanity_checks, testSmsTransportNotStarted)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_SMS, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED, error);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_SMS, STREAM_ID, data, sizeof data, 0, NULL);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED, error);
}

TEST_GROUP(test_ccapi_dp_binary_send_data_sanity_checks)
{
    static ccapi_dp_b_error_t error;

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

TEST(test_ccapi_dp_binary_send_data_sanity_checks, testInvalidData)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, NULL, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_DATA, error);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_TCP, STREAM_ID, NULL, sizeof data, 0, NULL);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_DATA, error);
}

TEST(test_ccapi_dp_binary_send_data_sanity_checks, testInvalidBytes)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, 0);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_DATA, error);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_TCP, STREAM_ID, data, 0, 0, NULL);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_DATA, error);
}

TEST(test_ccapi_dp_binary_send_data_sanity_checks, testOK)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    error = ccapi_dp_binary_send_data(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data, 0, NULL);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);
}

TEST_GROUP(test_dp_send_binary_with_reply_sanity_checks)
{
    static ccapi_dp_b_error_t error;

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

TEST(test_dp_send_binary_with_reply_sanity_checks, testInvalidHintString)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    ccapi_string_info_t hint;

    hint.length = 10;
    hint.string = NULL;

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data, 0, &hint);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_HINT_POINTER, error);
}

TEST(test_dp_send_binary_with_reply_sanity_checks, testInvalidHintLenght)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    ccapi_string_info_t hint;

    hint.length = 0;
    hint.string = (char*)malloc(10);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data, 0, &hint);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_HINT_POINTER, error);
}

TEST(test_dp_send_binary_with_reply_sanity_checks, testOK)
{
    ccapi_dp_b_error_t error;
    char const data[] = DATA;

    ccapi_string_info_t hint;

    hint.length = 10;
    hint.string = (char*)malloc(hint.length);

    error = ccapi_dp_binary_send_data_with_reply(CCAPI_TRANSPORT_TCP, STREAM_ID, data, sizeof data, 0, &hint);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);
}

TEST_GROUP(test_ccapi_dp_binary_send_file_sanity_checks)
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

TEST(test_ccapi_dp_binary_send_file_sanity_checks, testNullLocalPath)
{
    ccapi_dp_b_error_t error;

    error = ccapi_dp_binary_send_file(CCAPI_TRANSPORT_TCP, NULL, STREAM_ID);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_LOCAL_PATH, error);
}

TEST(test_ccapi_dp_binary_send_file_sanity_checks, testEmptyLocalPath)
{
    ccapi_dp_b_error_t error;

    error = ccapi_dp_binary_send_file(CCAPI_TRANSPORT_TCP, "", STREAM_ID);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_INVALID_LOCAL_PATH, error);
}

TEST(test_ccapi_dp_binary_send_file_sanity_checks, testLocalPathIsNotFile)
{
    ccapi_dp_b_error_t error;

    error = ccapi_dp_binary_send_file(CCAPI_TRANSPORT_TCP, "/kkk", STREAM_ID);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NOT_A_FILE, error);
}

TEST(test_ccapi_dp_binary_send_file_sanity_checks, testOK)
{
    ccapi_dp_b_error_t error;

    char const data[] = DATA;    

    create_test_file(LOCAL_PATH, data, sizeof data);

    error = ccapi_dp_binary_send_file(CCAPI_TRANSPORT_TCP, LOCAL_PATH, STREAM_ID);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    error = ccapi_dp_binary_send_file_with_reply(CCAPI_TRANSPORT_TCP, LOCAL_PATH, STREAM_ID, 0, NULL);
    CHECK_EQUAL(CCAPI_DP_B_ERROR_NONE, error);

    destroy_test_file(LOCAL_PATH);
}

