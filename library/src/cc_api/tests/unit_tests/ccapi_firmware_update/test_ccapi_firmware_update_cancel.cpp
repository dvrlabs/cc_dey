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

static ccapi_firmware_target_t firmware_list[] = {
       /* version   description           filespec                    maximum_size       chunk_size */
        {{1,0,0,0}, "Bootloader",  ".*\\.[bB][iI][nN]", 1 * 1024 * 1024,   128 * 1024 },  /* any *.bin files */
        {{0,0,1,0}, "Kernel",      ".*\\.a",            128 * 1024 * 1024, 128 * 1024 }   /* any *.a files */
    };
static uint8_t firmware_count = ARRAY_SIZE(firmware_list);

#define TEST_TARGET 0

static ccapi_fw_data_error_t test_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    (void)target;
    (void)offset;
    (void)data;
    (void)size;
    (void)last_chunk;

    return CCAPI_FW_DATA_ERROR_NONE;
}

static unsigned int ccapi_firmware_cancel_expected_target;
static ccapi_fw_cancel_error_t ccapi_firmware_cancel_expected_cancel_reason;
static ccapi_bool_t ccapi_firmware_cancel_cb_called;

static void test_fw_cancel_cb(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason)
{
    CHECK_EQUAL(ccapi_firmware_cancel_expected_target, target);
    CHECK_EQUAL(ccapi_firmware_cancel_expected_cancel_reason, cancel_reason);
    ccapi_firmware_cancel_cb_called = CCAPI_TRUE;
    return;
}

TEST_GROUP(test_ccapi_fw_abort_no_callback)
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

        error = ccapi_start(&start);

        CHECK(error == CCAPI_START_ERROR_NONE);

        {
            connector_request_id_t request;
            connector_callback_status_t status;
            connector_firmware_download_start_t connector_firmware_download_start;

            connector_firmware_download_start.target_number = TEST_TARGET;
            connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size;

            request.firmware_request = connector_request_id_firmware_download_start;
            status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
            CHECK_EQUAL(connector_callback_continue, status);
            CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
        }
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

TEST(test_ccapi_fw_abort_no_callback, testAbortBadTarget)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = firmware_count;
    connector_firmware_download_abort.status = connector_firmware_status_user_abort;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);
}

TEST(test_ccapi_fw_abort_no_callback, testAbortOk_nocallback)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = TEST_TARGET;
    connector_firmware_download_abort.status = connector_firmware_status_user_abort;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
}

TEST_GROUP(test_ccapi_fw_abort_callback)
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
                                                test_fw_cancel_cb
                                            }
                                        };

        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.firmware = &fw_service;

        ccapi_firmware_cancel_expected_target = (unsigned int)-1;
        ccapi_firmware_cancel_expected_cancel_reason = CCAPI_FW_CANCEL_USER_ABORT;
        ccapi_firmware_cancel_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);

        CHECK(error == CCAPI_START_ERROR_NONE);

        {
            connector_request_id_t request;
            connector_callback_status_t status;
            connector_firmware_download_start_t connector_firmware_download_start;

            connector_firmware_download_start.target_number = TEST_TARGET;
            connector_firmware_download_start.code_size = firmware_list[TEST_TARGET].chunk_size;

            request.firmware_request = connector_request_id_firmware_download_start;
            status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
            CHECK_EQUAL(connector_callback_continue, status);
            CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
        }
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

TEST(test_ccapi_fw_abort_callback, testAbort_user_abort)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = TEST_TARGET;
    connector_firmware_download_abort.status = connector_firmware_status_user_abort;

    ccapi_firmware_cancel_expected_target = connector_firmware_download_abort.target_number;
    ccapi_firmware_cancel_expected_cancel_reason = CCAPI_FW_CANCEL_USER_ABORT;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_cancel_cb_called);
}

TEST(test_ccapi_fw_abort_callback, testAbort_device_error)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = TEST_TARGET;
    connector_firmware_download_abort.status = connector_firmware_status_device_error;

    ccapi_firmware_cancel_expected_target = connector_firmware_download_abort.target_number;
    ccapi_firmware_cancel_expected_cancel_reason = CCAPI_FW_CANCEL_DEVICE_ERROR;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_cancel_cb_called);
}

TEST(test_ccapi_fw_abort_callback, testAbort_invalid_offset)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = TEST_TARGET;
    connector_firmware_download_abort.status = connector_firmware_status_invalid_offset;

    ccapi_firmware_cancel_expected_target = connector_firmware_download_abort.target_number;
    ccapi_firmware_cancel_expected_cancel_reason = CCAPI_FW_CANCEL_INVALID_OFFSET;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_cancel_cb_called);
}

TEST(test_ccapi_fw_abort_callback, testAbort_invalid_data)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = TEST_TARGET;
    connector_firmware_download_abort.status = connector_firmware_status_invalid_data;

    ccapi_firmware_cancel_expected_target = connector_firmware_download_abort.target_number;
    ccapi_firmware_cancel_expected_cancel_reason = CCAPI_FW_CANCEL_INVALID_DATA;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_cancel_cb_called);
}

TEST(test_ccapi_fw_abort_callback, testAbort_hardware_error)
{
    connector_request_id_t request;
    connector_firmware_download_abort_t connector_firmware_download_abort;
    connector_callback_status_t status;

    connector_firmware_download_abort.target_number = TEST_TARGET;
    connector_firmware_download_abort.status = connector_firmware_status_hardware_error;

    ccapi_firmware_cancel_expected_target = connector_firmware_download_abort.target_number;
    ccapi_firmware_cancel_expected_cancel_reason = CCAPI_FW_CANCEL_HARDWARE_ERROR;

    request.firmware_request = connector_request_id_firmware_download_abort;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_abort, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_cancel_cb_called);
}
