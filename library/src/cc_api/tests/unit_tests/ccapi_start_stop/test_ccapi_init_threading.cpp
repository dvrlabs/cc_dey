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

TEST_GROUP(test_ccapi_init_threading)
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

TEST(test_ccapi_init_threading, testInitErrorThreadNullPointer)
{
    ccapi_start_t start = {0};
    void * malloc_for_ccapi_data = malloc(sizeof (ccapi_data_t));
    void * malloc_for_device_type = malloc(sizeof TH_DEVICE_TYPE_STRING);
    void * malloc_for_device_cloud_url = malloc(sizeof TH_DEVICE_CLOUD_URL_STRING);
    static ccimp_os_create_thread_info_t mem_for_thread_connector_run;
    ccimp_os_create_thread_info_t expected_create_thread_connector_run;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof(ccapi_data_t), malloc_for_ccapi_data);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_TYPE_STRING), malloc_for_device_type);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof(TH_DEVICE_CLOUD_URL_STRING), malloc_for_device_cloud_url);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_thread_info_t), (void*)&mem_for_thread_connector_run);
    Mock_ccimp_os_free_notExpected();

    /* corrupt the argument created by the handle */

    expected_create_thread_connector_run.argument = malloc_for_ccapi_data;
    expected_create_thread_connector_run.type = CCIMP_THREAD_FSM;
    Mock_ccimp_os_create_thread_expectAndReturn(&expected_create_thread_connector_run, MOCK_THREAD_ENABLED_ARGUMENT_NULL, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    {
        /* call ccapi_start in a sepatare thread as it won't return */
        pthread_t const aux_thread = th_aux_ccapi_start(&start);

        WAIT_FOR_ASSERT();
        ASSERT_IF_NOT_HIT_DO ("ccapi_data != NULL", "source/ccapi.c", "ccapi_connector_run_thread", 
                                                          FAIL_TEST("'ccapi_data != NULL' not hitted"));

        th_stop_aux_thread(aux_thread);

        {
            ccapi_data_t * ccapi_data = (ccapi_data_t *)malloc_for_ccapi_data;

            /* The connector won't start but has been allocated. Manually clean it */
            assert(ccapi_data->connector_handle != NULL);
            mock_connector_api_info_free(ccapi_data->connector_handle);
        }
    }
    free(malloc_for_device_cloud_url);
    free(malloc_for_device_type);
    free(malloc_for_ccapi_data);
}

/* This test makes layer1 run thread return connector_init_error.
*/
TEST(test_ccapi_init_threading, testInitErrorRunRetConnectorInitError)
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
    Mock_ccimp_os_create_thread_expectAndReturn(&expected_create_thread_connector_run, MOCK_THREAD_ENABLED_NORMAL, CCIMP_STATUS_OK);

    th_fill_start_structure_with_good_parameters(&start);
    error = ccapi_start(&start);
    CHECK(error == CCAPI_START_ERROR_NONE);

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(handle); 
        mock_info->connector_run_retval = connector_init_error;
    }

    WAIT_FOR_ASSERT();
    ASSERT_IF_NOT_HIT_DO ("status != connector_init_error", "source/ccapi.c", "ccapi_connector_run_thread", 
                                                     FAIL_TEST("'status != connector_init_error' not hitted"));

    CHECK(ccapi_data_single_instance->thread.connector_run->status == CCAPI_THREAD_RUNNING);

    free(malloc_for_device_cloud_url);
    free(malloc_for_device_type);
    free(malloc_for_ccapi_data);
}
