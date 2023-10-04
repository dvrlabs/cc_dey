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
        {{1,0,0,0}, (char*)"Bootloader",  (char*)".*\\.[bB][iI][nN]", 1 * 1024 * 1024,   128 * 1024 },  /* any *.bin files */
        {{0,0,1,0}, (char*)"Kernel",      (char*)".*\\.a",            128 * 1024 * 1024, 128 * 1024 }   /* any *.a files */
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

static unsigned int ccapi_firmware_reset_expected_target;
static uint8_t ccapi_firmware_reset_expected_version_major;
static uint8_t ccapi_firmware_reset_expected_version_minor;
static uint8_t ccapi_firmware_reset_expected_version_revision;
static uint8_t ccapi_firmware_reset_expected_version_build;
static uint8_t ccapi_firmware_reset_version_major_retval;
static uint8_t ccapi_firmware_reset_version_minor_retval;
static uint8_t ccapi_firmware_reset_version_revision_retval;
static uint8_t ccapi_firmware_reset_version_build_retval;
static ccapi_bool_t ccapi_firmware_reset_retval;
static ccapi_bool_t ccapi_firmware_reset_cb_called;

static void test_fw_reset_cb(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * new_version)
{
    CHECK_EQUAL(ccapi_firmware_reset_expected_target, target);
    CHECK_EQUAL(ccapi_firmware_reset_expected_version_major, new_version->major);
    CHECK_EQUAL(ccapi_firmware_reset_expected_version_minor, new_version->minor);
    CHECK_EQUAL(ccapi_firmware_reset_expected_version_revision, new_version->revision);
    CHECK_EQUAL(ccapi_firmware_reset_expected_version_build, new_version->build);

    *system_reset = ccapi_firmware_reset_retval;

    new_version->major = ccapi_firmware_reset_version_major_retval;
    new_version->minor = ccapi_firmware_reset_version_minor_retval;
    new_version->revision = ccapi_firmware_reset_version_revision_retval;
    new_version->build = ccapi_firmware_reset_version_build_retval;

    ccapi_firmware_reset_cb_called = CCAPI_TRUE;

    return;
}

TEST_GROUP(test_ccapi_fw_reset_no_callback)
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
                                                NULL,
                                                NULL
                                            }
                                        };

        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.firmware = &fw_service;

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

TEST(test_ccapi_fw_reset_no_callback, testResetBadTarget)
{
    connector_request_id_t request;
    connector_firmware_reset_t connector_firmware_reset;
    connector_callback_status_t status;

    connector_firmware_reset.target_number = firmware_count;

    request.firmware_request = connector_request_id_firmware_target_reset;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_reset, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_error, status);
}

TEST(test_ccapi_fw_reset_no_callback, testResetOk_nocallback)
{
    connector_request_id_t request;
    connector_firmware_reset_t connector_firmware_reset;
    connector_callback_status_t status;

    connector_firmware_reset.target_number = TEST_TARGET;

    Mock_ccimp_hal_reset_expectAndReturn((void*)CCIMP_STATUS_OK);

    request.firmware_request = connector_request_id_firmware_target_reset;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_reset, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);
}

TEST_GROUP(test_ccapi_fw_reset_callback)
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
                                                NULL,
                                                test_fw_reset_cb,
                                            }
                                        };

        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.firmware = &fw_service;

        ccapi_firmware_reset_expected_target = (unsigned int)-1;
        ccapi_firmware_reset_cb_called = CCAPI_FALSE;

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

TEST(test_ccapi_fw_reset_callback, testReset_versions)
{
    connector_request_id_t request;
    connector_firmware_reset_t connector_firmware_reset;
    connector_callback_status_t status;
    const unsigned int test_target = TEST_TARGET;

    connector_firmware_reset.target_number = test_target;

    ccapi_firmware_reset_expected_target = test_target;
    ccapi_firmware_reset_expected_version_major = firmware_list[test_target].version.major;
    ccapi_firmware_reset_expected_version_minor = firmware_list[test_target].version.minor;
    ccapi_firmware_reset_expected_version_revision = firmware_list[test_target].version.revision;
    ccapi_firmware_reset_expected_version_build = firmware_list[test_target].version.build;

    ccapi_firmware_reset_version_major_retval = ccapi_firmware_reset_expected_version_major + 1;
    ccapi_firmware_reset_version_minor_retval = ccapi_firmware_reset_expected_version_minor + 1;
    ccapi_firmware_reset_version_revision_retval = ccapi_firmware_reset_expected_version_revision + 1;
    ccapi_firmware_reset_version_build_retval = ccapi_firmware_reset_expected_version_build + 1;

    ccapi_firmware_reset_retval = CCAPI_FALSE;

    request.firmware_request = connector_request_id_firmware_target_reset;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_reset, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_reset_cb_called);

    CHECK_EQUAL(ccapi_firmware_reset_version_major_retval, ccapi_data_single_instance->service.firmware_update.config.target.item[test_target].version.major);
    CHECK_EQUAL(ccapi_firmware_reset_version_minor_retval, ccapi_data_single_instance->service.firmware_update.config.target.item[test_target].version.minor);
    CHECK_EQUAL(ccapi_firmware_reset_version_revision_retval, ccapi_data_single_instance->service.firmware_update.config.target.item[test_target].version.revision);
    CHECK_EQUAL(ccapi_firmware_reset_version_build_retval, ccapi_data_single_instance->service.firmware_update.config.target.item[test_target].version.build);

    /* Enable the mock to check that ccimp_reset() is NOT called */
    mock("ccimp_hal_reset").setData("behavior", MOCK_RESET_ENABLED);
}

TEST(test_ccapi_fw_reset_callback, testReset_resetTrue)
{
    connector_request_id_t request;
    connector_firmware_reset_t connector_firmware_reset;
    connector_callback_status_t status;
    const unsigned int test_target = TEST_TARGET;

    connector_firmware_reset.target_number = test_target;

    ccapi_firmware_reset_expected_target = test_target;
    ccapi_firmware_reset_expected_version_major = firmware_list[test_target].version.major;
    ccapi_firmware_reset_expected_version_minor = firmware_list[test_target].version.minor;
    ccapi_firmware_reset_expected_version_revision = firmware_list[test_target].version.revision;
    ccapi_firmware_reset_expected_version_build = firmware_list[test_target].version.build;

    ccapi_firmware_reset_retval = CCAPI_TRUE;

    Mock_ccimp_hal_reset_expectAndReturn((void*)CCIMP_STATUS_OK);

    request.firmware_request = connector_request_id_firmware_target_reset;
    status = ccapi_connector_callback(connector_class_id_firmware, request, &connector_firmware_reset, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_firmware_reset_cb_called);
}
