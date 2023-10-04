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

TEST_GROUP(test_ccapi_config)
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

TEST(test_ccapi_config, testDeviceID)
{
    connector_request_id_t request;
    connector_config_pointer_data_t device_id = {NULL, sizeof ccapi_data_single_instance->config.device_id};


    request.config_request = connector_request_id_config_device_id;
    ccapi_connector_callback(connector_class_id_config, request, &device_id, ccapi_data_single_instance);
    CHECK_EQUAL(device_id.data, ccapi_data_single_instance->config.device_id);
}

TEST(test_ccapi_config, testCloudURL)
{
    connector_request_id_t request;
    connector_config_pointer_string_t device_cloud_url = {0};

    request.config_request = connector_request_id_config_device_cloud_url;
    ccapi_connector_callback(connector_class_id_config, request, &device_cloud_url, ccapi_data_single_instance);
    STRCMP_EQUAL(device_cloud_url.string, ccapi_data_single_instance->config.device_cloud_url);
    CHECK(strlen(ccapi_data_single_instance->config.device_cloud_url) == device_cloud_url.length);
}

TEST(test_ccapi_config, testVendorID)
{
    connector_request_id_t request;
    connector_config_vendor_id_t vendor_id = {0};

    request.config_request = connector_request_id_config_vendor_id;
    ccapi_connector_callback(connector_class_id_config, request, &vendor_id, ccapi_data_single_instance);
    CHECK(vendor_id.id == ccapi_data_single_instance->config.vendor_id);
}

TEST(test_ccapi_config, testDeviceType)
{
    connector_request_id_t request;
    connector_config_pointer_string_t device_type = {0};

    request.config_request = connector_request_id_config_device_type;
    ccapi_connector_callback(connector_class_id_config, request, &device_type, ccapi_data_single_instance);
    STRCMP_EQUAL(device_type.string, ccapi_data_single_instance->config.device_type);
    CHECK(strlen(ccapi_data_single_instance->config.device_type) == device_type.length);
}

TEST(test_ccapi_config, testFirmwareSupport)
{
    connector_request_id_t request;
    connector_config_supported_t firmware_supported = {connector_true}; /* Set to the opposite to test that it actually worked */

    request.config_request = connector_request_id_config_firmware_facility;
    ccapi_connector_callback(connector_class_id_config, request, &firmware_supported, ccapi_data_single_instance);
    CHECK(firmware_supported.supported == connector_false);
}

TEST(test_ccapi_config, testFileSystemSupport)
{
    connector_request_id_t request;
    connector_config_supported_t filesystem_supported = {connector_true}; /* Set to the opposite to test that it actually worked */

    request.config_request = connector_request_id_config_file_system;
    ccapi_connector_callback(connector_class_id_config, request, &filesystem_supported, ccapi_data_single_instance);
    CHECK(filesystem_supported.supported == connector_false);
}

TEST(test_ccapi_config, testRCISupport)
{
    connector_request_id_t request;
    connector_config_supported_t rci_supported = {connector_true}; /* Set to the opposite to test that it actually worked */

    request.config_request = connector_request_id_config_remote_configuration;
    ccapi_connector_callback(connector_class_id_config, request, &rci_supported, ccapi_data_single_instance);
    CHECK(rci_supported.supported == connector_false);
}

TEST(test_ccapi_config, testDataServiceSupport)
{
    connector_request_id_t request;
    connector_config_supported_t dataservice_supported = {connector_false}; /* Set to the opposite to test that it actually worked */

    request.config_request = connector_request_id_config_data_service;
    ccapi_connector_callback(connector_class_id_config, request, &dataservice_supported, ccapi_data_single_instance);
    CHECK(dataservice_supported.supported == connector_true);
}
