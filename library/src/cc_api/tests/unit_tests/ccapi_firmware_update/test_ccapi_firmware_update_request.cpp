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
        {{0,0,1,0}, "Kernel",      ".*\\.a",            128 * 1024 * 1024, 128 * 1024 },  /* any *.a files */
        {{0,0,1,0}, "TestDefVals", ".*\\.k",            0,                 0 }   /* any *.a files */
    };
static uint8_t firmware_count = ARRAY_SIZE(firmware_list);

static ccapi_fw_data_error_t test_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    (void)target;
    (void)offset;
    (void)data;
    (void)size;
    (void)last_chunk;

    return CCAPI_FW_DATA_ERROR_NONE;
}

static unsigned int ccapi_firmware_start_expected_target;
static char const * ccapi_firmware_start_expected_filename;
static size_t ccapi_firmware_start_expected_total_size;
static ccapi_fw_request_error_t ccapi_firmware_start_retval;
static ccapi_bool_t ccapi_firmware_start_cb_called;

static ccapi_fw_request_error_t test_fw_request_cb(unsigned int const target, char const * const filename, size_t const total_size)
{
    CHECK_EQUAL(ccapi_firmware_start_expected_target, target);
    STRCMP_EQUAL(ccapi_firmware_start_expected_filename, filename);
    CHECK_EQUAL(ccapi_firmware_start_expected_total_size, total_size);
    ccapi_firmware_start_cb_called = CCAPI_TRUE;
    return ccapi_firmware_start_retval;
}

TEST_GROUP(test_ccapi_fw_start_no_callback)
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

TEST(test_ccapi_fw_start_no_callback, testStartBadTarget)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = firmware_count;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);
}

TEST(test_ccapi_fw_start_no_callback, testStartAlreadyStarted)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_success);

    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_encountered_error);
}

TEST(test_ccapi_fw_start_no_callback, testStartBadSize)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size + 1;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_invalid_size);
}

TEST(test_ccapi_fw_start_no_callback, testStartMaxSize0)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 2;
    connector_firmware_download_start.code_size = firmware_list[2].maximum_size + 1;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
}

TEST(test_ccapi_fw_start_no_callback, testStartChunkSize0)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 2;
    connector_firmware_download_start.code_size = firmware_list[2].maximum_size;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_success);

    CHECK_EQUAL(ccapi_data_single_instance->service.firmware_update.config.target.item[2].chunk_size, 1024);
}

TEST(test_ccapi_fw_start_no_callback, testStartOk_nocallback)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
}

TEST_GROUP(test_ccapi_fw_start_callback)
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
                                                test_fw_request_cb, 
                                                test_fw_data_cb, 
                                                NULL
                                            }
                                        };

        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.firmware = &fw_service;

        ccapi_firmware_start_expected_target = (unsigned int)-1;
        ccapi_firmware_start_expected_filename = "";
        ccapi_firmware_start_expected_total_size = 0;
        ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_DENIED;
        ccapi_firmware_start_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);

        CHECK(error == CCAPI_START_ERROR_NONE);
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

#define TEST_FILENAME ((char*)"test.bin")

TEST(test_ccapi_fw_start_callback, testStart_download_denied)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_DENIED;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_denied);
}

TEST(test_ccapi_fw_start_callback, testStart_download_invalid_size)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_SIZE;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_invalid_size);
}

TEST(test_ccapi_fw_start_callback, testStart_download_invalid_version)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_VERSION;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_invalid_version);
}

TEST(test_ccapi_fw_start_callback, testStart_download_unauthenticated)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_UNAUTHENTICATED;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_unauthenticated);
}

TEST(test_ccapi_fw_start_callback, testStart_download_not_allowed)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_NOT_ALLOWED;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_not_allowed);
}

TEST(test_ccapi_fw_start_callback, testStart_download_configured_to_reject)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_CONFIGURED_TO_REJECT;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_download_configured_to_reject);
}

TEST(test_ccapi_fw_start_callback, testStart_encountered_error)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_encountered_error);
}

TEST(test_ccapi_fw_start_callback, testStartOk)
{
    connector_request_id_t request;
    connector_firmware_download_start_t connector_firmware_download_start;
    connector_callback_status_t status;

    connector_firmware_download_start.target_number = 0;
    connector_firmware_download_start.filename = TEST_FILENAME;
    connector_firmware_download_start.code_size = firmware_list[0].maximum_size;

    ccapi_firmware_start_expected_target = connector_firmware_download_start.target_number;
    ccapi_firmware_start_expected_filename = connector_firmware_download_start.filename;
    ccapi_firmware_start_expected_total_size = connector_firmware_download_start.code_size;
    ccapi_firmware_start_retval = CCAPI_FW_REQUEST_ERROR_NONE;

    request.firmware_request = connector_request_id_firmware_download_start;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_download_start, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_start_cb_called);

    CHECK(connector_firmware_download_start.status == connector_firmware_status_success);
}
