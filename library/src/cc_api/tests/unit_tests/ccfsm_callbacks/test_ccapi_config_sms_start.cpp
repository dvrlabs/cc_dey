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

TEST_GROUP(test_ccapi_config_sms_start)
{
    void setup()
    {

        Mock_create_all();

        th_start_ccapi();
        th_start_sms();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);
        Mock_destroy_all();
    }
};

TEST(test_ccapi_config_sms_start,testMaxSessions)
{
    connector_request_id_t request;
    connector_config_sm_max_sessions_t max_s = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_sm_sms_max_sessions;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &max_s, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(max_s.max_sessions,ccapi_data_single_instance->transport_sms.info->limit.max_sessions);

}

TEST(test_ccapi_config_sms_start,testRxTimeout)
{
    connector_request_id_t request;
    connector_config_sm_rx_timeout_t rx_t = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_sm_sms_rx_timeout;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &rx_t, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(rx_t.rx_timeout,ccapi_data_single_instance->transport_sms.info->limit.rx_timeout);

}

TEST(test_ccapi_config_sms_start,testGetPhoneNumber)
{
    connector_request_id_t request;
    connector_config_pointer_string_t phone = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_get_device_cloud_phone;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &phone, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(strlen(ccapi_data_single_instance->transport_sms.info->cloud_config.phone_number), phone.length);
    STRCMP_EQUAL(ccapi_data_single_instance->transport_sms.info->cloud_config.phone_number, phone.string);

    STRCMP_EQUAL("+54-3644-421921", phone.string);
}

TEST(test_ccapi_config_sms_start,testSetPhoneNumber)
{
    connector_request_id_t request;
    char phone_number[] = "+33-3333-333333-4";
    connector_config_pointer_string_t new_phone = { phone_number, sizeof phone_number - 1};
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_set_device_cloud_phone;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &new_phone, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(strlen(ccapi_data_single_instance->transport_sms.info->cloud_config.phone_number), new_phone.length);
    STRCMP_EQUAL(ccapi_data_single_instance->transport_sms.info->cloud_config.phone_number, new_phone.string);
}

TEST(test_ccapi_config_sms_start,testServiceID)
{
    connector_request_id_t request;
    connector_config_pointer_string_t id = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_device_cloud_service_id;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &id, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(strlen(ccapi_data_single_instance->transport_sms.info->cloud_config.service_id), id.length);
    STRCMP_EQUAL(ccapi_data_single_instance->transport_sms.info->cloud_config.service_id, id.string);

    STRCMP_EQUAL("", id.string);
}
