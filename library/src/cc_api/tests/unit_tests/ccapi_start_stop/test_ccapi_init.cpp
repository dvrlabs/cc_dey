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

TEST_GROUP(test_ccapi_init)
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

TEST(test_ccapi_init, testParamNULL)
{
    ccapi_start_error_t error;
    ccapi_start_t * start = NULL;

    error = ccapi_start(start);
    CHECK(error == CCAPI_START_ERROR_NULL_PARAMETER);
}

TEST(test_ccapi_init, testVendorIdZero)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    start.vendor_id = 0;
    error = ccapi_start(&start);

    CHECK_EQUAL(error, CCAPI_START_ERROR_INVALID_VENDORID);
}

TEST(test_ccapi_init, testInvalidDeviceId)
{
    uint8_t device_id[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    memcpy(start.device_id, device_id, sizeof start.device_id);
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INVALID_DEVICEID);
}

TEST(test_ccapi_init, testNullDeviceCloudURL)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    start.device_cloud_url = NULL;
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INVALID_URL);
}

TEST(test_ccapi_init, testInvalidDeviceCloudURL)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    start.device_cloud_url = "";
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INVALID_URL);
}

TEST(test_ccapi_init, testNullDeviceType)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    start.device_type = NULL;
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INVALID_DEVICETYPE);
}

TEST(test_ccapi_init, testInvalidDeviceType)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    th_fill_start_structure_with_good_parameters(&start);
    start.device_type = "";
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INVALID_DEVICETYPE);
}

TEST(test_ccapi_init, testNoMemory)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    void * malloc_for_ccapi_data = NULL;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_free_notExpected();
    th_fill_start_structure_with_good_parameters(&start);

    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INSUFFICIENT_MEMORY);
}

TEST(test_ccapi_init, testDeviceTypeNoMemory)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    void * malloc_for_ccapi_data = malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = NULL;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);

    /* No need to call free on malloc_for_device_type because the malloc failed */
    Mock_ccimp_os_free_expectAndReturn(malloc_for_ccapi_data, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INSUFFICIENT_MEMORY);
}

TEST(test_ccapi_init, testDeviceCloudURLNoMemory)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    void * malloc_for_ccapi_data = malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = malloc(sizeof TH_DEVICE_TYPE_STRING);
    void * malloc_for_device_cloud_url = NULL;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_CLOUD_URL_STRING), malloc_for_device_cloud_url);

    /* No need to call free on malloc_for_device_cloud_url because the malloc failed */
    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_ccapi_data, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INSUFFICIENT_MEMORY);
}

TEST(test_ccapi_init, testConnectorInitNoMemory)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    connector_handle_t handle = NULL;

    Mock_connector_init_expectAndReturn(ccapi_connector_callback, handle, ccapi_data_single_instance);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INSUFFICIENT_MEMORY);
}

TEST(test_ccapi_init, testStartOk)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    void * malloc_for_ccapi_data = malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = malloc(sizeof TH_DEVICE_TYPE_STRING);
    void * malloc_for_device_cloud_url = malloc(sizeof TH_DEVICE_CLOUD_URL_STRING);
    static ccimp_os_create_thread_info_t mem_for_thread_connector_run;
    ccimp_os_create_thread_info_t expected_create_thread_connector_run;

    connector_handle_t handle = &handle; /* Not-NULL */

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_CLOUD_URL_STRING), malloc_for_device_cloud_url);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_thread_info_t), (void*)&mem_for_thread_connector_run);
    Mock_ccimp_os_free_notExpected();

    Mock_connector_init_expectAndReturn(ccapi_connector_callback, handle, ccapi_data_single_instance);

    expected_create_thread_connector_run.argument = malloc_for_ccapi_data;
    expected_create_thread_connector_run.type = CCIMP_THREAD_FSM;
    /* expected_create_thread_connector_run.start */
    Mock_ccimp_os_create_thread_expectAndReturn(&expected_create_thread_connector_run, MOCK_THREAD_ENABLED_NORMAL, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);
    CHECK(error == CCAPI_START_ERROR_NONE);

    CHECK(start.vendor_id == ccapi_data_single_instance->config.vendor_id);
    CHECK(memcmp(start.device_id, ccapi_data_single_instance->config.device_id, sizeof start.device_id) == 0);
    STRCMP_EQUAL(start.device_type, ccapi_data_single_instance->config.device_type);
    STRCMP_EQUAL(start.device_cloud_url, ccapi_data_single_instance->config.device_cloud_url);
    CHECK(ccapi_data_single_instance->thread.connector_run->status == CCAPI_THREAD_RUNNING);
    CHECK(ccapi_data_single_instance->transport_tcp.info == NULL);
    CHECK(ccapi_data_single_instance->transport_udp.info == NULL);
    CHECK(ccapi_data_single_instance->transport_sms.info == NULL);
}

TEST(test_ccapi_init, testStartThreadNoMemory)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    void * malloc_for_ccapi_data= malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = malloc(sizeof TH_DEVICE_TYPE_STRING);
    void * malloc_for_device_cloud_url = malloc(sizeof TH_DEVICE_CLOUD_URL_STRING);
    void * mem_for_thread_connector_run = NULL;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_CLOUD_URL_STRING), malloc_for_device_cloud_url);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_thread_info_t), mem_for_thread_connector_run);

    /* No need to call free on mem_for_thread_connector_run because the malloc failed */
    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_ccapi_data, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);
    CHECK(error == CCAPI_START_ERROR_INSUFFICIENT_MEMORY);

    {
        /* The connector won't start but has been allocated. Manually clean it */
        ccapi_data_t * ccapi_data = (ccapi_data_t *)malloc_for_ccapi_data;
        assert(ccapi_data->connector_handle != NULL);
        mock_connector_api_info_free(ccapi_data->connector_handle);
    }
}

TEST(test_ccapi_init, testStartThreadFail)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    void * malloc_for_ccapi_data= malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = malloc(sizeof TH_DEVICE_TYPE_STRING);
    void * malloc_for_device_cloud_url = malloc(sizeof TH_DEVICE_CLOUD_URL_STRING);
    static ccimp_os_create_thread_info_t mem_for_thread_connector_run;
    ccimp_os_create_thread_info_t expected_create_thread_connector_run;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_CLOUD_URL_STRING), malloc_for_device_cloud_url);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_thread_info_t), (void*)&mem_for_thread_connector_run);

    Mock_ccimp_os_free_expectAndReturn(&mem_for_thread_connector_run, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_cloud_url, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_device_type, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(malloc_for_ccapi_data, CCIMP_STATUS_OK);

    expected_create_thread_connector_run.argument = malloc_for_ccapi_data;
    expected_create_thread_connector_run.type = CCIMP_THREAD_FSM;
    Mock_ccimp_os_create_thread_expectAndReturn(&expected_create_thread_connector_run, MOCK_THREAD_ENABLED_DONT_CREATE_THREAD, CCIMP_STATUS_ERROR);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);
    CHECK(error == CCAPI_START_ERROR_THREAD_FAILED);

    {
        /* The connector won't start but has been allocated. Manually clean it */
        ccapi_data_t * ccapi_data = (ccapi_data_t *)malloc_for_ccapi_data;
        assert(ccapi_data->connector_handle != NULL);
        mock_connector_api_info_free(ccapi_data->connector_handle);
    }
}

TEST(test_ccapi_init, testStartTwiceFails)
{
    ccapi_start_error_t start_error;
    ccapi_start_t start = {0};

    th_fill_start_structure_with_good_parameters(&start);

    start_error = ccapi_start(&start);

    CHECK_EQUAL(start_error, CCAPI_START_ERROR_NONE);

    start_error = ccapi_start(&start);

    CHECK_EQUAL(start_error, CCAPI_START_ERROR_ALREADY_STARTED);
}
