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

static ccapi_fw_data_error_t test_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    (void)target;
    (void)offset;
    (void)data;
    (void)size;
    (void)last_chunk;

    return CCAPI_FW_DATA_ERROR_NONE;
}

static void test_cli_request_cb(ccapi_transport_t const transport, char const * const command, char const * * const output)
{
    (void)transport;
    (void)command;
    (void)output;

    return;
}

TEST_GROUP(test_ccapi_init_services)
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

TEST(test_ccapi_init_services, testServicesNotSupported)
{
    th_start_ccapi();
    CHECK(ccapi_data_single_instance->config.cli_supported == CCAPI_FALSE);
    CHECK(ccapi_data_single_instance->config.receive_supported == CCAPI_FALSE);
    CHECK(ccapi_data_single_instance->config.firmware_supported == CCAPI_FALSE);
    CHECK(ccapi_data_single_instance->config.rci_supported == CCAPI_FALSE);
    CHECK(ccapi_data_single_instance->config.filesystem_supported == CCAPI_FALSE);
}

TEST(test_ccapi_init_services, testServicesSupported)
{
    void * pointer = &pointer; /* Not-NULL */
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    ccapi_filesystem_service_t fs_service = {NULL, NULL};
    ccapi_fw_service_t fw_service = {{firmware_count, firmware_list}, {NULL, test_fw_data_cb, NULL}};
    ccapi_receive_service_t receive_service = {NULL, NULL, NULL};
    ccapi_cli_service_t cli_service = {test_cli_request_cb, NULL};
    ccapi_rci_service_t rci_service = {(ccapi_rci_data_t const *)pointer};

    th_fill_start_structure_with_good_parameters(&start);
    start.service.cli = &cli_service;
    start.service.receive = &receive_service;
    start.service.firmware = &fw_service;
    start.service.rci = &rci_service;
    start.service.file_system = &fs_service;

    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_NONE);
    CHECK(ccapi_data_single_instance->config.cli_supported == CCAPI_TRUE);

    CHECK(ccapi_data_single_instance->config.receive_supported == CCAPI_TRUE);
    CHECK_EQUAL(receive_service.accept, ccapi_data_single_instance->service.receive.user_callback.accept);
    CHECK_EQUAL(receive_service.data, ccapi_data_single_instance->service.receive.user_callback.data);
    CHECK_EQUAL(receive_service.status, ccapi_data_single_instance->service.receive.user_callback.status);

    CHECK(ccapi_data_single_instance->config.firmware_supported == CCAPI_TRUE);
    CHECK_EQUAL(fw_service.target.count, ccapi_data_single_instance->service.firmware_update.config.target.count);
    CHECK_EQUAL(fw_service.callback.request, ccapi_data_single_instance->service.firmware_update.config.callback.request);
    CHECK_EQUAL(fw_service.callback.data, ccapi_data_single_instance->service.firmware_update.config.callback.data);
    CHECK_EQUAL(fw_service.callback.cancel, ccapi_data_single_instance->service.firmware_update.config.callback.cancel);

    /* TODO: Check 'maximum_size' and 'chunk_size' as it won't be checked in test_ccapi_fw_init_callback */
    {
        unsigned char target;

        for (target=0 ; target < firmware_count ; target++)
        {
            CHECK(ccapi_data_single_instance->service.firmware_update.config.target.item[target].maximum_size == firmware_list[target].maximum_size);
            CHECK(ccapi_data_single_instance->service.firmware_update.config.target.item[target].chunk_size == firmware_list[target].chunk_size);
        }
    }

    CHECK(ccapi_data_single_instance->config.rci_supported == CCAPI_TRUE);

    CHECK(ccapi_data_single_instance->config.filesystem_supported == CCAPI_TRUE);
    CHECK_EQUAL(fs_service.access, ccapi_data_single_instance->service.file_system.user_callback.access);
    CHECK_EQUAL(fs_service.changed, ccapi_data_single_instance->service.file_system.user_callback.changed);

    CHECK(NULL == ccapi_data_single_instance->service.file_system.imp_context);
    CHECK(NULL == ccapi_data_single_instance->service.file_system.virtual_dir_list);
    CHECK(NULL != ccapi_data_single_instance->file_system_lock);

    {
        ccapi_stop_error_t stop_error;

        Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
        CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
        CHECK(ccapi_data_single_instance == NULL);
    }
}

extern ccapi_rci_data_t const ccapi_rci_data;

TEST(test_ccapi_init_services, testStubFwWithRci)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    ccapi_rci_service_t rci_service;

    th_fill_start_structure_with_good_parameters(&start);
    rci_service.rci_data = &ccapi_rci_data;
    start.service.firmware = NULL;
    start.service.rci = &rci_service;

    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_NONE);

    CHECK(ccapi_data_single_instance->config.firmware_supported == CCAPI_TRUE);
    CHECK_EQUAL(1, ccapi_data_single_instance->service.firmware_update.config.target.count);
    CHECK(ccapi_data_single_instance->service.firmware_update.config.target.item != NULL);
    CHECK(ccapi_data_single_instance->service.firmware_update.config.callback.request != NULL);
    CHECK(ccapi_data_single_instance->service.firmware_update.config.callback.data == NULL);
    CHECK(ccapi_data_single_instance->service.firmware_update.config.callback.cancel == NULL);
    CHECK_EQUAL(0, ccapi_data_single_instance->service.firmware_update.config.target.item[0].chunk_size);
    CHECK_EQUAL(0, ccapi_data_single_instance->service.firmware_update.config.target.item[0].maximum_size);
    CHECK_EQUAL(1, ccapi_data_single_instance->service.firmware_update.config.target.item[0].version.major);
    CHECK_EQUAL(0, ccapi_data_single_instance->service.firmware_update.config.target.item[0].version.minor);
    CHECK_EQUAL(0, ccapi_data_single_instance->service.firmware_update.config.target.item[0].version.revision);
    CHECK_EQUAL(0, ccapi_data_single_instance->service.firmware_update.config.target.item[0].version.build);
    CHECK(ccapi_data_single_instance->service.firmware_update.config.target.item[0].description == NULL);
    CHECK(ccapi_data_single_instance->service.firmware_update.config.target.item[0].filespec == NULL);
    CHECK(ccapi_data_single_instance->thread.firmware == NULL);
    CHECK(ccapi_data_single_instance->config.rci_supported == CCAPI_TRUE);

    {
        ccapi_stop_error_t stop_error;

        Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
        CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
        CHECK(ccapi_data_single_instance == NULL);
    }
}
