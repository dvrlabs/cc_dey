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

TEST_GROUP(test_ccapi_sms_start)
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


TEST(test_ccapi_sms_start, testConnectorInitiateActionOK)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    sms_start.callback.close = ccapi_sms_close_cb;

    connector_transport_t connector_transport = connector_transport_sms;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_NONE, error);
}

TEST(test_ccapi_sms_start, testSMSAlreadyStarted)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    sms_start.callback.close = ccapi_sms_close_cb;

    connector_transport_t connector_transport = connector_transport_sms;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_NONE, error);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_ALREADY_STARTED, error);
}

TEST(test_ccapi_sms_start, testConnectorInitiateActionInitError1)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    connector_transport_t connector_transport = connector_transport_sms;
    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_init_error);

    th_expect_malloc(sizeof (ccapi_sms_info_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof phone_number, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof service_id, TH_MALLOC_RETURN_NORMAL, true);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INIT, error);

    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->thread.connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance, CCIMP_STATUS_OK);
}

TEST(test_ccapi_sms_start, testConnectorInitiateActionInitError2)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    connector_transport_t connector_transport = connector_transport_sms;
    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_invalid_data);
    th_expect_malloc(sizeof (ccapi_sms_info_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof phone_number, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof service_id, TH_MALLOC_RETURN_NORMAL, true);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INIT, error);

    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->thread.connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance, CCIMP_STATUS_OK);
}

TEST(test_ccapi_sms_start, testConnectorInitiateActionInitError3)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    connector_transport_t connector_transport = connector_transport_sms;
    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_service_busy);
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_abort);
    th_expect_malloc(sizeof (ccapi_sms_info_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof phone_number, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof service_id, TH_MALLOC_RETURN_NORMAL, true);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INIT, error);

    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->config.device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance->thread.connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(ccapi_data_single_instance, CCIMP_STATUS_OK);
}

TEST(test_ccapi_sms_start, testConnectorInitiateActionUnknownError)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    connector_transport_t connector_transport = connector_transport_sms;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_abort);
    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_INIT, error);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_data_single_instance->transport_sms.started);
}


TEST(test_ccapi_sms_start, testSMSConnectionTimeout)
{
    ccapi_sms_start_error_t error;
    ccapi_sms_info_t sms_start = {{0}};
    char phone_number[] = "+54-3644-421921";
    char service_id[] = "";

    sms_start.cloud_config.service_id = service_id;
    sms_start.cloud_config.phone_number = phone_number;
    connector_transport_t connector_transport = connector_transport_sms;

    sms_start.start_timeout = 10;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    Mock_ccimp_os_get_system_time_return(0); /* Start time */
    Mock_ccimp_os_get_system_time_return(5);
    Mock_ccimp_os_get_system_time_return(10);
    Mock_ccimp_os_get_system_time_return(15);

    error = ccapi_start_transport_sms(&sms_start);
    CHECK_EQUAL(CCAPI_SMS_START_ERROR_TIMEOUT, error);
}
