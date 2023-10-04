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
#define DATA         "CCAPI send data sample\n"
#define LOCAL_PATH   "./test_ccapi_send_common_sanity_checks.txt"

/* This group doesn't call ccapi_start/stop functions */
TEST_GROUP(ccapi_send_with_no_ccapi) 
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

TEST(ccapi_send_with_no_ccapi, testCcapiNotStarted)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_CCAPI_NOT_RUNNING, error);
}

/* This group doesn't call ccapi_transport_tcp_start function */
TEST_GROUP(ccapi_send_with_no_transport_start) 
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

TEST(ccapi_send_with_no_transport_start, testTransportNotStarted)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED, error);
}

TEST_GROUP(test_ccapi_send_common_sanity_checks)
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

TEST(test_ccapi_send_common_sanity_checks, testNullCloudPath)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, NULL, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_CLOUD_PATH, error);
}

TEST(test_ccapi_send_common_sanity_checks, testEmptyCloudPath)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, "", CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_CLOUD_PATH, error);
}

TEST(test_ccapi_send_common_sanity_checks, testNullContentTypeIsValid)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, NULL, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);
}

TEST(test_ccapi_send_common_sanity_checks, testEmptyContentType)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, "", DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_CONTENT_TYPE, error);
}

TEST(test_ccapi_send_common_sanity_checks, testLimitContentType)
{
    ccapi_send_error_t error;
    char * content_type;

    #define TEST_SIZE_FAIL (UCHAR_MAX+1)
    content_type = (char *)malloc(TEST_SIZE_FAIL+1);
    memset(content_type, 'A', TEST_SIZE_FAIL);
    content_type[TEST_SIZE_FAIL] = '\0';

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, content_type, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_CONTENT_TYPE, error);

    free(content_type);


    #define TEST_SIZE_OK (UCHAR_MAX)
    content_type = (char *)malloc(TEST_SIZE_OK+1);
    memset(content_type, 'B', TEST_SIZE_OK);
    content_type[TEST_SIZE_OK] = '\0';

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, content_type, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    free(content_type);
}

TEST(test_ccapi_send_common_sanity_checks, testUdpTransportNotStarted)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_UDP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED, error);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_UDP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED, error);
}

TEST(test_ccapi_send_common_sanity_checks, testSmsTransportNotStarted)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_SMS, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED, error);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_SMS, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED, error);
}


TEST_GROUP(test_ccapi_send_data_sanity_checks)
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

TEST(test_ccapi_send_data_sanity_checks, testInvalidData)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, NULL, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_DATA, error);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, NULL, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_DATA, error);
}

TEST(test_ccapi_send_data_sanity_checks, testInvalidBytes)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, 0, CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_DATA, error);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, 0, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_DATA, error);
}

TEST(test_ccapi_send_data_sanity_checks, testOK)
{
    ccapi_send_error_t error;

    error = ccapi_send_data(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);
}

TEST_GROUP(test_ccapi_send_with_reply_sanity_checks)
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

TEST(test_ccapi_send_with_reply_sanity_checks, testInvalidHintString)
{
    ccapi_send_error_t error;

    ccapi_string_info_t hint;

    hint.length = 10;
    hint.string = NULL;

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, &hint);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_HINT_POINTER, error);
}

TEST(test_ccapi_send_with_reply_sanity_checks, testInvalidHintLenght)
{
    ccapi_send_error_t error;

    ccapi_string_info_t hint;

    hint.length = 0;
    hint.string = (char*)malloc(10);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, &hint);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_HINT_POINTER, error);
}

TEST(test_ccapi_send_with_reply_sanity_checks, testOK)
{
    ccapi_send_error_t error;

    ccapi_string_info_t hint;

    hint.length = 10;
    hint.string = (char*)malloc(hint.length);

    error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP, CLOUD_PATH, CONTENT_TYPE, DATA, strlen(DATA), CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, &hint);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);
}

TEST_GROUP(test_ccapi_send_file_sanity_checks)
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

TEST(test_ccapi_send_file_sanity_checks, testNullLocalPath)
{
    ccapi_send_error_t error;

    error = ccapi_send_file(CCAPI_TRANSPORT_TCP, NULL, CLOUD_PATH, CONTENT_TYPE, CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_LOCAL_PATH, error);
}

TEST(test_ccapi_send_file_sanity_checks, testEmptyLocalPath)
{
    ccapi_send_error_t error;

    error = ccapi_send_file(CCAPI_TRANSPORT_TCP, "", CLOUD_PATH, CONTENT_TYPE, CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_INVALID_LOCAL_PATH, error);
}

TEST(test_ccapi_send_file_sanity_checks, testLocalPathIsNotFile)
{
    ccapi_send_error_t error;

    error = ccapi_send_file(CCAPI_TRANSPORT_TCP, "/kkk", CLOUD_PATH, CONTENT_TYPE, CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NOT_A_FILE, error);
}

TEST(test_ccapi_send_file_sanity_checks, testOK)
{
    ccapi_send_error_t error;

    create_test_file(LOCAL_PATH, DATA, strlen(DATA));

    error = ccapi_send_file(CCAPI_TRANSPORT_TCP, LOCAL_PATH, CLOUD_PATH, CONTENT_TYPE, CCAPI_SEND_BEHAVIOR_OVERWRITE);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    error = ccapi_send_file_with_reply(CCAPI_TRANSPORT_TCP, LOCAL_PATH, CLOUD_PATH, CONTENT_TYPE, CCAPI_SEND_BEHAVIOR_OVERWRITE, 0, NULL);
    CHECK_EQUAL(CCAPI_SEND_ERROR_NONE, error);

    destroy_test_file(LOCAL_PATH);
}
