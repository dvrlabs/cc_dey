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

TEST_GROUP(test_ccapi_stop)
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

TEST(test_ccapi_stop, testCcapiNotStarted)
{
    ccapi_stop_error_t stop_error;

    stop_error = ccapi_stop(CCAPI_STOP_GRACEFULLY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NOT_STARTED);

    stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NOT_STARTED);
    CHECK(ccapi_data_single_instance == NULL);
}

TEST(test_ccapi_stop, testCcapiStartedBadly)
{
    ccapi_stop_error_t stop_error;
    ccapi_start_error_t start_error;
    ccapi_start_t start = {0};

    th_fill_start_structure_with_good_parameters(&start);
    start.vendor_id = 0;
    start_error = ccapi_start(&start);

    CHECK_EQUAL(start_error, CCAPI_START_ERROR_INVALID_VENDORID);

    stop_error = ccapi_stop(CCAPI_STOP_GRACEFULLY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NOT_STARTED);
    CHECK(ccapi_data_single_instance == NULL);
}

TEST(test_ccapi_stop, testCcapiStopGracefully)
{
    ccapi_stop_error_t stop_error;

    void * malloc_for_ccapi_data = malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = malloc(sizeof TH_DEVICE_TYPE_STRING);
    void * malloc_for_device_cloud_url = malloc(sizeof TH_DEVICE_CLOUD_URL_STRING);
    static ccimp_os_create_thread_info_t mem_for_thread_connector_run;


    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_CLOUD_URL_STRING), malloc_for_device_cloud_url);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_thread_info_t), (void*)&mem_for_thread_connector_run);

    th_start_ccapi();

    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(&mem_for_thread_connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_ccapi_data, CCIMP_STATUS_OK);

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop_error = ccapi_stop(CCAPI_STOP_GRACEFULLY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);
}

TEST(test_ccapi_stop, testCcapiStopImmediately)
{
    ccapi_stop_error_t stop_error;

    th_start_ccapi();
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);
}

TEST(test_ccapi_stop, testCcapiStopTCPOk)
{
    ccapi_stop_error_t stop_error;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    connector_initiate_stop_request_t stop_data = {connector_transport_tcp, connector_stop_immediately, NULL};
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_stop, &stop_data, connector_success);
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);
}

TEST(test_ccapi_stop, testCcapiStopTCPFail)
{
    ccapi_stop_error_t stop_error;

    th_start_ccapi();
    th_start_tcp_lan_ipv4();

    connector_initiate_stop_request_t stop_data = {connector_transport_tcp, connector_stop_immediately, NULL};
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_stop, &stop_data, connector_device_error);
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

    stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
    CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
    CHECK(ccapi_data_single_instance == NULL);
}

static ccapi_bool_t status_callback_called;

static void status_callback(ccapi_status_info_t * const info)
{
    CHECK_EQUAL(CCAPI_STOP_CCFSM_ERROR, info->stop_cause);
    status_callback_called = CCAPI_TRUE;
}

TEST(test_ccapi_stop, testCcapiStopAsynchronously)
{
    ccapi_start_t start;
    ccapi_start_error_t start_error;

    th_expect_malloc(sizeof (ccapi_data_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof TH_DEVICE_TYPE_STRING, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof TH_DEVICE_CLOUD_URL_STRING, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof (ccapi_thread_info_t), TH_MALLOC_RETURN_NORMAL, true);


    th_fill_start_structure_with_good_parameters(&start);
    status_callback_called = CCAPI_FALSE;
    start.status = status_callback;

    start_error = ccapi_start(&start);
    CHECK_EQUAL(CCAPI_START_ERROR_NONE, start_error);
    CHECK(status_callback == ccapi_data_single_instance->config.status_callback);

    th_expect_malloc(sizeof *ccapi_data_single_instance->transport_tcp.info, TH_MALLOC_RETURN_NORMAL, true);
    th_start_tcp_lan_ipv4();

    th_expect_malloc(sizeof *ccapi_data_single_instance->transport_udp.info, TH_MALLOC_RETURN_NORMAL, true);
    th_start_udp();

    th_expect_malloc(sizeof *ccapi_data_single_instance->transport_sms.info, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof "+54-3644-421921", TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(sizeof "", TH_MALLOC_RETURN_NORMAL, true);
    th_start_sms();

    mock_connector_api_info_t * const mock_connector_api_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
    mock_connector_api_info->connector_run_retval = connector_abort;

    for (;;)
    {
        if (!status_callback_called)
        {
            ccimp_os_yield();
        }
        else
        {
            break;
        }
    }
    wait_for_ccimp_threads();
}
