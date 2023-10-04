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

/* This group doesn't call ccapi_start/stop functions */
TEST_GROUP(ccapi_sms_start_with_no_ccapi) {};
TEST(ccapi_sms_start_with_no_ccapi, testNotStarted)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{}};
    IGNORE_ALL_LEAKS_IN_TEST(); /* TODO: if CCAPI is not started it detects memory leaks */
    error = ccapi_start_transport_sms(&sms_start);

    CHECK_EQUAL(CCAPI_SMS_START_ERROR_CCAPI_STOPPED, error);
}
TEST_GROUP(test_ccapi_sms_start_sanity_checks)
{
    void setup()
    {
        Mock_create_all();

        th_start_ccapi();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);

        Mock_destroy_all();
    }
};

static ccapi_bool_t ccapi_sms_close_cb(ccapi_sms_close_cause_t cause)
{
    UNUSED_ARGUMENT(cause);
    return CCAPI_TRUE;
}

TEST(test_ccapi_sms_start_sanity_checks, testNullPointer)
{
    ccapi_sms_start_error_t error;

    error = ccapi_start_transport_sms(NULL);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_NULL_POINTER, error);
}

TEST(test_ccapi_sms_start_sanity_checks, testDefaults)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;

    connector_transport_t connector_transport = connector_transport_sms;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_NONE, error);
    CHECK_EQUAL(CCAPI_SM_SMS_MAX_SESSIONS_DEFAULT, ccapi_data_single_instance->transport_sms.info->limit.max_sessions);
}

TEST(test_ccapi_sms_start_sanity_checks, testMaxSessions)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    sms_start.limit.max_sessions = 300;

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_MAX_SESSIONS, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}


TEST(test_ccapi_sms_start_sanity_checks, testSmsInfoNoMemory)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char id[] = "";

    sms_start.cloud_config.service_id = id;
    sms_start.cloud_config.phone_number = phone_number;

    th_expect_malloc(sizeof (ccapi_sms_info_t), TH_MALLOC_RETURN_NULL, false);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INSUFFICIENT_MEMORY, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}

TEST(test_ccapi_sms_start_sanity_checks, testSmsPhoneNoMemory)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char id[] = "";
    char phone_number[] = "+54-3644-421921";

    sms_start.cloud_config.service_id = id;
    sms_start.cloud_config.phone_number = phone_number;

    th_expect_malloc(sizeof (ccapi_sms_info_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof phone_number, TH_MALLOC_RETURN_NULL, false);

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INSUFFICIENT_MEMORY, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_data_single_instance->transport_sms.started);

    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->thread.connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance, CCIMP_STATUS_OK);
}

TEST(test_ccapi_sms_start_sanity_checks, testSmsServiceIdNoMemory)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char id[] = "ID";
    char phone_number[] = "+54-3644-421921";

    sms_start.cloud_config.service_id = id;
    sms_start.cloud_config.phone_number = phone_number;

    th_expect_malloc(sizeof (ccapi_sms_info_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof phone_number, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof id, TH_MALLOC_RETURN_NULL, true);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INSUFFICIENT_MEMORY, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);

    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->thread.connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance, CCIMP_STATUS_OK);
}

TEST(test_ccapi_sms_start_sanity_checks, testInvalidPhoneNULL)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char id[] = "";

    sms_start.cloud_config.phone_number = NULL;
    sms_start.cloud_config.service_id = id;
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INVALID_PHONE, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}

TEST(test_ccapi_sms_start_sanity_checks, testInvalidPhoneEmpty)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "";
    char id[] = "";

    sms_start.cloud_config.phone_number = phone_number;
    sms_start.cloud_config.service_id = id;

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INVALID_PHONE, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}

TEST(test_ccapi_sms_start_sanity_checks, testInvalidPhone)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "#54 5+e";
    char id[] = "";

    sms_start.cloud_config.phone_number = phone_number;
    sms_start.cloud_config.service_id = id;

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INVALID_PHONE, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}

TEST(test_ccapi_sms_start_sanity_checks, testInvalidServiceId)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";

    sms_start.cloud_config.phone_number = phone_number;
    sms_start.cloud_config.service_id = NULL;

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INVALID_SERVICE_ID, error);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}

TEST(test_ccapi_sms_start_sanity_checks, testCallbacksIsCopied)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.limit.max_sessions=140;
    sms_start.limit.rx_timeout=CCAPI_UDP_RX_TIMEOUT_INFINITE;
    sms_start.callback.close=ccapi_sms_close_cb;
    sms_start.cloud_config.phone_number = phone_number;

    connector_transport_t connector_transport = connector_transport_sms;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_NONE, error);
    CHECK_EQUAL(sms_start.callback.close, ccapi_data_single_instance->transport_sms.info->callback.close);


}

