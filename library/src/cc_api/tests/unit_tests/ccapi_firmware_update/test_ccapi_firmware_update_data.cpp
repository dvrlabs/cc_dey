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
#include <unistd.h>
#include "../../../source/ccapi_definitions.h" /* To get CCAPI_CHUNK_POOL_SIZE */

#define TEST_CHUNK_SIZE 16

static ccapi_firmware_target_t firmware_list[] = {
       /* version   description           filespec                    maximum_size                               chunk_size     */
        {{0,0,1,0}, "Test",        ".*\\.test",        TEST_CHUNK_SIZE * (CCAPI_CHUNK_POOL_SIZE + 1),         TEST_CHUNK_SIZE         }   /* any *.test files */
    };
static uint8_t firmware_count = ARRAY_SIZE(firmware_list);

#define TEST_TARGET 0

#define DATA  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, \
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, \
                0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, \
                0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f }

#define MAX_CALLBACK_CALLS CCAPI_CHUNK_POOL_SIZE + 1

static unsigned int ccapi_firmware_data_expected_target[MAX_CALLBACK_CALLS];
static uint32_t ccapi_firmware_data_expected_offset[MAX_CALLBACK_CALLS];
static void const * ccapi_firmware_data_expected_data[MAX_CALLBACK_CALLS];
static size_t ccapi_firmware_data_expected_size[MAX_CALLBACK_CALLS];
static ccapi_bool_t ccapi_firmware_data_expected_last_chunk[MAX_CALLBACK_CALLS];
static ccapi_fw_data_error_t ccapi_firmware_data_retval[MAX_CALLBACK_CALLS];
static ccapi_bool_t ccapi_firmware_data_lock_cb[MAX_CALLBACK_CALLS];

static uint8_t ccapi_firmware_data_cb_called;

static ccapi_fw_data_error_t test_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    CHECK_EQUAL(ccapi_firmware_data_expected_target[ccapi_firmware_data_cb_called], target);
    CHECK_EQUAL(ccapi_firmware_data_expected_offset[ccapi_firmware_data_cb_called], offset);
	
#if 0
    {
        unsigned int i;
        uint8_t * pData = (uint8_t*)data;
        for (i = 0; i < size; i++)
        {
            printf("0x%x,", pData[i]);
        }
        printf("\r\n");
    }
#endif

    CHECK_EQUAL(ccapi_firmware_data_expected_size[ccapi_firmware_data_cb_called], size);
    CHECK_EQUAL(0, memcmp(data, ccapi_firmware_data_expected_data[ccapi_firmware_data_cb_called], size));


    CHECK_EQUAL(ccapi_firmware_data_expected_last_chunk[ccapi_firmware_data_cb_called], last_chunk);

    while (ccapi_firmware_data_lock_cb[ccapi_firmware_data_cb_called])
    {
        sched_yield();
    }

    return ccapi_firmware_data_retval[ccapi_firmware_data_cb_called++];
}

TEST_GROUP(test_ccapi_fw_data_callback)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_fw_service_t fw_service = {
                                            {
                                                firmware_count,
                                                firmware_list
                                            }, 
                                            {
                                                NULL, 
                                                test_fw_data_cb, 
                                                NULL
                                            }
                                        };

        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.firmware = &fw_service;

        {
            unsigned int i;
            for (i = 0; i < MAX_CALLBACK_CALLS; i++)
            { 
                ccapi_firmware_data_expected_target[i] = (unsigned int)-1;
                ccapi_firmware_data_expected_offset[i] = 0;
                ccapi_firmware_data_expected_data[i] = NULL;
                ccapi_firmware_data_expected_size[i] = 0;
                ccapi_firmware_data_expected_last_chunk[i] = CCAPI_FALSE;

                ccapi_firmware_data_retval[i] = CCAPI_FW_DATA_ERROR_INVALID_DATA;
                ccapi_firmware_data_lock_cb[i] = CCAPI_FALSE;
                ccapi_firmware_data_cb_called = 0;
            }
        }

        error = ccapi_start(&start);

        CHECK(error == CCAPI_START_ERROR_NONE);

        ccapi_data_single_instance->service.firmware_update.processing.target = TEST_TARGET; 
    }

    void teardown()
    {
        ccapi_stop_error_t stop_error;

        Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
        CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
        CHECK(ccapi_data_single_instance == NULL);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_fw_data_callback, testDataStartNotCalled)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;

    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0x10000;
    connector_firmware_download_data.image.data = NULL;
    connector_firmware_download_data.image.bytes_used = 1024;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = NULL;
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);

    WAIT_FOR_ASSERT();
    ASSERT_IF_NOT_HIT_DO ("ccapi_data->service.firmware_update.processing.update_started", "source/ccapi_firmware_update_handler.c", "ccapi_process_firmware_update_data", 
                                                     FAIL_TEST("'update_started' not hitted"));

    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
}

TEST(test_ccapi_fw_data_callback, testDataBadInitialOffset) /* TODO: Do one with bad intermediate offset */
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size; /* second chunk_size block */
    connector_firmware_download_data.image.data = NULL;
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_invalid_offset);

    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
}

TEST(test_ccapi_fw_data_callback, testDataTotalsizeEqualsChunksize)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
}

TEST(test_ccapi_fw_data_callback, testDataTotalsizeEqualsChunksizeBusy)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
    }
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
}

TEST(test_ccapi_fw_data_callback, testDataTwoBlocksThatMachChuncks)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block match first chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_cb_called = 0;

    /* second data block match second chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
    while ( status == connector_callback_busy);
#else
    while (0);
#endif
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
}

TEST(test_ccapi_fw_data_callback, testDataTwoBlocksThatMachChuncksBusy)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block match first chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
    }
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_cb_called = 0;

    /* second data block match second chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
    while ( status == connector_callback_busy);
#else
    while (0);
#endif
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
    }
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);

    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_cb_called = 0;
}

TEST(test_ccapi_fw_data_callback, testErrorInvalidData)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block match first chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_INVALID_DATA;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    /* The error reported in one chunk will be recognised one or more chunks later by the connector, when the firmware thread process it */
    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0 || ccapi_data_single_instance->service.firmware_update.processing.chunk_pool[0].in_use == CCAPI_TRUE);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);

    CHECK(ccapi_data_single_instance->service.firmware_update.processing.data_error == CCAPI_FW_DATA_ERROR_INVALID_DATA);

    ccapi_firmware_data_cb_called = 0;

    /* second data block match second chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
    while ( status == connector_callback_busy);
#else
    while (0);
#endif
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_invalid_data);
}

TEST(test_ccapi_fw_data_callback, testDataFourBlocksSmallerThanChuncks)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block is half a chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    /* second data block is half a chunk. We should get a callback */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;

    /* Third data block is half a chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    request.firmware_request = connector_request_id_firmware_download_data;
    do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
    while ( status == connector_callback_busy);
#else
    while (0);
#endif
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    /* second data block is half a chunk. We should get a callback */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
}

TEST(test_ccapi_fw_data_callback, testDataFourBlocksSmallerThanChuncksBusy)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block is half a chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    /* second data block is half a chunk. We should get a callback */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
    }
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);

    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;

    /* Third data block is half a chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    request.firmware_request = connector_request_id_firmware_download_data;
    do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
    while ( status == connector_callback_busy);
#else
    while (0);
#endif
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    /* Fourth data block is half a chunk. We should get a callback */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
    }
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;
}

TEST(test_ccapi_fw_data_callback, testDataThreeBlocksThatDoNotMatchChunckBoundaries)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block is half a chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    /* second data block is a chunk. We should get a callback and catch half of the data */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;

#if (CCAPI_CHUNK_POOL_SIZE == 1)
    /* The ccapi_connector_callback returns busy until the user callback is processed as there are no more chunk pool */
    do
    {
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
#else
    /* We need two calls: first one completes first chunk and queues it (returns busy) and second call partilly fill second chunk */
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_busy, status);
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
#endif
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;

    /* Third data block is half a chunk. We should get the second callback */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;
}

TEST(test_ccapi_fw_data_callback, testDataThreeBlocksThatDoNotMatchChunckBoundariesBusy)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    /* First data block is half a chunk */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    /* second data block is a chunk. We should get a callback and catch half of the data */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;

#if (CCAPI_CHUNK_POOL_SIZE == 1)
    /* The ccapi_connector_callback returns busy until the user callback is processed as there are no more chunk pool */
    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_busy, status);
        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
    }
    
    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;

    do
    {
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    } while ( status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);    
#else
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_busy, status);
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;
#endif

    /* Third data block is half a chunk. We should get the second callback */
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2;
    connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size + firmware_list[TEST_TARGET].chunk_size / 2];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size / 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    for (i = 0 ; i < 1000 ; i++)
    {
        sched_yield();
    }
    CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

    ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == 0);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;
}

TEST(test_ccapi_fw_data_callback, testOneBlockIsTwoChunks)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size * 2;

    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    ccapi_firmware_data_expected_target[1] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[1] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[1] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[1] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[1] = CCAPI_TRUE;
    ccapi_firmware_data_retval[1] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
#if (CCAPI_CHUNK_POOL_SIZE == 1)
    do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    while ( status == connector_callback_busy);
#else
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_busy, status);
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#endif
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called < 2);
    CHECK_EQUAL(2, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;
}

/*TODO: Very dependent on CCAPI_CHUNK_POOL_SIZE */
IGNORE_TEST(test_ccapi_fw_data_callback, testOneBlockIsTwoChunksBusy)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.offset = 0;
    connector_firmware_download_data.image.data = &data[0];
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size * 2;

    /* First call, one callback we return busy */
    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_busy, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;


    /* second call, one callback we return ok, second we return busy */
    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = 0;
    ccapi_firmware_data_expected_data[0] = &data[0];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    ccapi_firmware_data_expected_target[1] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[1] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[1] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[1] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[1] = CCAPI_TRUE;
    ccapi_firmware_data_retval[1] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_busy, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(2, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;

    /* third call, we return ok */
    ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
    ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
    ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
    ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_data;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    ccapi_firmware_data_cb_called = 0;
}

TEST(test_ccapi_fw_data_callback, testDataCompleteBeforeAllDataArrives)
{
    connector_request_id_t request;

    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    const uint8_t BYTES_TO_MAX = 5;
    const uint8_t MISSING_DATA_BYTES = 3;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2 - BYTES_TO_MAX;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    {
        connector_firmware_download_data_t connector_firmware_download_data;

        /* First data block match first chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = 0;
        connector_firmware_download_data.image.data = &data[0];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

        ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
        ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
        ccapi_firmware_data_expected_data[0] = &data[0];
        ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

        request.firmware_request = connector_request_id_firmware_download_data;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);
        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;

        /* second data block less than second chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
        connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;

        request.firmware_request = connector_request_id_firmware_download_data;
        do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
        while ( status == connector_callback_busy);
#else
        while (0);
#endif
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
    }

    {
        connector_firmware_download_complete_t connector_firmware_download_complete;

        connector_firmware_download_complete.target_number = TEST_TARGET;

        request.firmware_request = connector_request_id_firmware_download_complete;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_complete, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_complete.status == connector_firmware_download_not_complete);

        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
    }
}

TEST(test_ccapi_fw_data_callback, testDataCompleteBoundary)
{
    connector_request_id_t request;

    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    const uint8_t BYTES_TO_MAX = 0;
    const uint8_t MISSING_DATA_BYTES = 0;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2 - BYTES_TO_MAX;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    {
        connector_firmware_download_data_t connector_firmware_download_data;

        /* First data block match first chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = 0;
        connector_firmware_download_data.image.data = &data[0];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

        ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
        ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
        ccapi_firmware_data_expected_data[0] = &data[0];
        ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

        request.firmware_request = connector_request_id_firmware_download_data;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);
        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;

        /* second data block match second chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
        connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;

        ccapi_firmware_data_expected_target[0] = TEST_TARGET;
        ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
        ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
        ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

        request.firmware_request = connector_request_id_firmware_download_data;

        do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
        while ( status == connector_callback_busy);
#else
        while (0);
#endif
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);
        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;
    }

    {
        connector_firmware_download_complete_t connector_firmware_download_complete;

        connector_firmware_download_complete.target_number = TEST_TARGET;

        request.firmware_request = connector_request_id_firmware_download_complete;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_complete, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_complete.status == connector_firmware_download_success);

        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
    }
}

TEST(test_ccapi_fw_data_callback, testDataCompleteNotBoundary)
{
    connector_request_id_t request;

    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    const uint8_t BYTES_TO_MAX = 5;
    const uint8_t MISSING_DATA_BYTES = 0;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2 - BYTES_TO_MAX;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    {
        connector_firmware_download_data_t connector_firmware_download_data;

        /* First data block match first chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = 0;
        connector_firmware_download_data.image.data = &data[0];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

        ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
        ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
        ccapi_firmware_data_expected_data[0] = &data[0];
        ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

        request.firmware_request = connector_request_id_firmware_download_data;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);
        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;

        /* second data block less than second chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
        connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;

        ccapi_firmware_data_expected_target[0] = TEST_TARGET;
        ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
        ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
        ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;

        request.firmware_request = connector_request_id_firmware_download_data;
        do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
        while ( status == connector_callback_busy);
#else
        while (0);
#endif
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);
        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;
    }

    {
        connector_firmware_download_complete_t connector_firmware_download_complete;

        connector_firmware_download_complete.target_number = TEST_TARGET;

        request.firmware_request = connector_request_id_firmware_download_complete;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_complete, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_complete.status == connector_firmware_download_success);
        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
    }
}

TEST(test_ccapi_fw_data_callback, testDataCompleteNotBoundaryBusy)
{
    connector_request_id_t request;

    connector_callback_status_t status;
    uint8_t const data[] = DATA;

    const uint8_t BYTES_TO_MAX = 5;
    const uint8_t MISSING_DATA_BYTES = 0;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * 2 - BYTES_TO_MAX;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    {
        connector_firmware_download_data_t connector_firmware_download_data;

        /* First data block match first chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = 0;
        connector_firmware_download_data.image.data = &data[0];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

        ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
        ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
        ccapi_firmware_data_expected_data[0] = &data[0];
        ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_FALSE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
        ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

        request.firmware_request = connector_request_id_firmware_download_data;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

        for (i = 0 ; i < 1000 ; i++)
        {
            sched_yield();
        }
        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

        ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);
        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;

        /* second data block less than second chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = firmware_list[TEST_TARGET].chunk_size;
        connector_firmware_download_data.image.data = &data[firmware_list[TEST_TARGET].chunk_size];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;

        ccapi_firmware_data_expected_target[0] = TEST_TARGET;
        ccapi_firmware_data_expected_offset[0] = firmware_list[TEST_TARGET].chunk_size;
        ccapi_firmware_data_expected_data[0] = &data[firmware_list[TEST_TARGET].chunk_size];
        ccapi_firmware_data_expected_size[0] = firmware_list[TEST_TARGET].chunk_size - BYTES_TO_MAX - MISSING_DATA_BYTES;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
        ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

        request.firmware_request = connector_request_id_firmware_download_data;
        do status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
#if (CCAPI_CHUNK_POOL_SIZE == 1)
        while ( status == connector_callback_busy);
#else
        while (0);
#endif
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

        for (i = 0 ; i < 1000 ; i++)
        {
            sched_yield();
        }
        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);

        ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == 0);

        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
        ccapi_firmware_data_cb_called = 0;
    }

    {
        connector_firmware_download_complete_t connector_firmware_download_complete;

        connector_firmware_download_complete.target_number = TEST_TARGET;

        request.firmware_request = connector_request_id_firmware_download_complete;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_complete, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_complete.status == connector_firmware_download_success);
        CHECK_EQUAL(0, ccapi_firmware_data_cb_called);
    }
}

TEST(test_ccapi_fw_data_callback, testDataCompleteWaitAllPoolsFinish)
{
    connector_request_id_t request;

    connector_callback_status_t status;
    uint8_t const data[] = DATA;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size;

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    {
        connector_firmware_download_data_t connector_firmware_download_data;

        /* First data block match first chunk */
        connector_firmware_download_data.target_number = TEST_TARGET;
        connector_firmware_download_data.image.offset = 0;
        connector_firmware_download_data.image.data = &data[0];
        connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

        ccapi_firmware_data_expected_target[0] = connector_firmware_download_data.target_number;
        ccapi_firmware_data_expected_offset[0] = connector_firmware_download_data.image.offset;
        ccapi_firmware_data_expected_data[0] = &data[0];
        ccapi_firmware_data_expected_size[0] = connector_firmware_download_data.image.bytes_used;
        ccapi_firmware_data_expected_last_chunk[0] = CCAPI_TRUE;
        ccapi_firmware_data_retval[0] = CCAPI_FW_DATA_ERROR_NONE;
        ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

        request.firmware_request = connector_request_id_firmware_download_data;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    }

    {
        connector_firmware_download_complete_t connector_firmware_download_complete;

        connector_firmware_download_complete.target_number = TEST_TARGET;

        request.firmware_request = connector_request_id_firmware_download_complete;
        for (i = 0 ; i < 1000 ; i++)
        {
            status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_complete, ccapi_data_single_instance);
            CHECK_EQUAL(connector_callback_busy, status);
        }

        ccapi_firmware_data_lock_cb[0] = CCAPI_FALSE;

        do
        {
            status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_complete, ccapi_data_single_instance);
        } while ( status == connector_callback_busy);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_complete.status == connector_firmware_download_success);

        CHECK_EQUAL(1, ccapi_firmware_data_cb_called);
    }
}

TEST(test_ccapi_fw_data_callback, testDataFillChunckPool)
{
    connector_request_id_t request;
    connector_firmware_download_data_t connector_firmware_download_data;
    connector_callback_status_t status;
    uint8_t const data[] = DATA;
    unsigned int i;

    {
        connector_firmware_download_start_t connector_firmware_download_start;

        connector_firmware_download_start.target_number = TEST_TARGET;
        connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size * (CCAPI_CHUNK_POOL_SIZE + 1);

        request.firmware_request = connector_request_id_firmware_download_start;
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);
        CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
    }

    ccapi_firmware_data_lock_cb[0] = CCAPI_TRUE;

    request.firmware_request = connector_request_id_firmware_download_data;
    connector_firmware_download_data.target_number = TEST_TARGET;
    connector_firmware_download_data.image.bytes_used = firmware_list[TEST_TARGET].chunk_size;

    /* Send CCAPI_CHUNK_POOL_SIZE blocks matching chunks that should be accepted inmediately */
    for (i = 0; i < CCAPI_CHUNK_POOL_SIZE; i++)
    {
        connector_firmware_download_data.image.offset = i * firmware_list[TEST_TARGET].chunk_size;
        connector_firmware_download_data.image.data = &data[i];

        ccapi_firmware_data_expected_target[i] = TEST_TARGET;
        ccapi_firmware_data_expected_offset[i] = i * firmware_list[TEST_TARGET].chunk_size;
        ccapi_firmware_data_expected_data[i] = &data[i];
        ccapi_firmware_data_expected_size[i] = firmware_list[TEST_TARGET].chunk_size;
        ccapi_firmware_data_expected_last_chunk[i] = CCAPI_FALSE;
        ccapi_firmware_data_retval[i] = CCAPI_FW_DATA_ERROR_NONE;
 
        status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
        CHECK_EQUAL(connector_callback_continue, status);

        CHECK(connector_firmware_download_data.status == connector_firmware_status_success);
    }


    /* Send last block matching chunk that should be returned busy */
    connector_firmware_download_data.image.offset = i * firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[i];

    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_busy, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    for (i = 0; i < CCAPI_CHUNK_POOL_SIZE; i++)
    {
        ccapi_firmware_data_lock_cb[i + 1] = CCAPI_TRUE;
        ccapi_firmware_data_lock_cb[i] = CCAPI_FALSE;

        do
        {
            sched_yield();
        } while (ccapi_firmware_data_cb_called == i);
        CHECK_EQUAL(i + 1, ccapi_firmware_data_cb_called);
    }


    ccapi_firmware_data_expected_target[i] = TEST_TARGET;
    ccapi_firmware_data_expected_offset[i] = i * firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_data[i] = &data[i];
    ccapi_firmware_data_expected_size[i] = firmware_list[TEST_TARGET].chunk_size;
    ccapi_firmware_data_expected_last_chunk[i] = CCAPI_TRUE;
    ccapi_firmware_data_retval[i] = CCAPI_FW_DATA_ERROR_NONE;

    ccapi_firmware_data_lock_cb[i] = CCAPI_FALSE;

    /* Send last block matching chunk that should be returned continue */
    connector_firmware_download_data.image.offset = i * firmware_list[TEST_TARGET].chunk_size;
    connector_firmware_download_data.image.data = &data[i];

    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_data, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_data.status == connector_firmware_status_success);

    do
    {
        sched_yield();
    } while (ccapi_firmware_data_cb_called == i);
    CHECK_EQUAL(i + 1, ccapi_firmware_data_cb_called);
}


