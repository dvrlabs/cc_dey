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

extern "C" {
#include "ccapi_rci_functions.h"
#include "rci_config.h"
}

static ccapi_firmware_target_t firmware_list[] = {
       /* version   description    filespec             maximum_size       chunk_size */
        {{1,0,0,0}, "Bootloader",  ".*\\.[bB][iI][nN]", 1 * 1024 * 1024,   128 * 1024 },  /* any *.bin files */
        {{0,0,1,0}, "Kernel",      ".*\\.a",            128 * 1024 * 1024, 128 * 1024 }   /* any *.a files */
    };
static uint8_t const firmware_count = (sizeof(firmware_list)/sizeof(firmware_list[0]));

static ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk)
{
    (void)target;
    (void)offset;
    (void)data;
    (void)size;
    (void)last_chunk;

    return CCAPI_FW_DATA_ERROR_NONE;
}

extern ccapi_rci_data_t const ccapi_rci_data;

static ccapi_fw_service_t  fw_service = {
                                        {
                                            firmware_count,
                                            firmware_list
                                        },
                                        {
                                            NULL,
                                            app_fw_data_cb,
                                            NULL
                                        }
                                    };

static ccapi_rci_service_t rci_service;

ccapi_bool_t ccapi_rci_lock_cb = CCAPI_FALSE;

TEST_GROUP(test_ccapi_rci)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;

        rci_service.rci_data = &ccapi_rci_data;

        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.firmware = &fw_service;
        start.service.rci = &rci_service;

        error = ccapi_start(&start);

        CHECK(error == CCAPI_START_ERROR_NONE);

        ccapi_rci_lock_cb = CCAPI_FALSE;
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

void th_rci_checkExpectations(char const * const function_name, ccapi_rci_info_t const * const rci_info);
char const * th_rci_last_function_called(void);
void th_rci_returnValues(unsigned int const retval, ccapi_rci_info_t const * const rci_info);
void const * th_rci_get_value_ptr(void);

void set_remote_config_defaults(connector_remote_config_t * const remote_config)
{
    remote_config->action = connector_remote_action_query;
    remote_config->attribute.source = rci_query_setting_attribute_source_current;
    remote_config->attribute.compare_to = rci_query_setting_attribute_compare_to_none;
    remote_config->error_id = 0;

    remote_config->element.id = 0;
    remote_config->element.value = NULL;

    remote_config->group.type = connector_remote_group_setting;
    remote_config->group.id = 0;
    remote_config->group.index = 0;

    remote_config->response.compare_matches = connector_false;
    remote_config->response.element_value = NULL;
    remote_config->response.error_hint = NULL;
    remote_config->user_context = NULL;
}

void set_rci_info_defaults(ccapi_rci_info_t * const rci_info)
{
    rci_info->action = CCAPI_RCI_ACTION_QUERY;
    rci_info->error_hint = NULL;
    rci_info->user_context = NULL;
    rci_info->group.instance = 0;
    rci_info->group.type = CCAPI_RCI_GROUP_SETTING;
    rci_info->query_setting.matches = CCAPI_FALSE;
    rci_info->query_setting.attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    rci_info->query_setting.attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;
}

static connector_callback_status_t call_ccapi_connector_callback(connector_class_id_t const class_id, connector_request_id_t const request_id, void * const data, void * const context)
{
#define BUSY_LOOPS 1000
    connector_callback_status_t status;
    ccapi_bool_t const ccapi_rci_lock_cb_config = ccapi_rci_lock_cb;
    unsigned int i = 0;

    do
    {      
        status = ccapi_connector_callback(class_id, request_id, data, context);
        if (i++ == BUSY_LOOPS)
        {
            ccapi_rci_lock_cb = CCAPI_FALSE;
        }
    } while (status == connector_callback_busy);
    CHECK_EQUAL(connector_callback_continue, status);

    if (ccapi_rci_lock_cb_config)
    {
        CHECK(i > BUSY_LOOPS);
        ccapi_rci_lock_cb = CCAPI_TRUE;
    }

    return status;
}

static void session_start(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    set_remote_config_defaults(remote_config);
    set_rci_info_defaults(&ccapi_data_single_instance->service.rci.rci_info);
    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;
    rci_info_to_return = expected_rci_info;

    rci_info_to_return.user_context = remote_config;
    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_session_start;

    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

static void action_start(connector_remote_config_t * const remote_config, ccapi_rci_action_t const action, ccapi_rci_group_type_t const type, ccapi_rci_query_setting_attributes_t const * const query_setting_attributes, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.action = action;
    expected_rci_info.group.type = type;
    expected_rci_info.query_setting.attributes = *query_setting_attributes;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

    rci_info_to_return = expected_rci_info;

    remote_config->error_id = 0;
    switch (action)
    {
        case CCAPI_RCI_ACTION_QUERY:
            remote_config->action = connector_remote_action_query;
            break;
        case CCAPI_RCI_ACTION_SET:
            remote_config->action = connector_remote_action_set;
            break;
        case CCAPI_RCI_ACTION_DO_COMMAND:
            remote_config->action = connector_remote_action_do_command;
            break;
        case CCAPI_RCI_ACTION_REBOOT:
            remote_config->action = connector_remote_action_reboot;
            break;
        case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
            remote_config->action = connector_remote_action_set_factory_def;
            break;
    }

    switch (type)
    {
        case CCAPI_RCI_GROUP_SETTING:
            remote_config->group.type = connector_remote_group_setting;
            break;
        case CCAPI_RCI_GROUP_STATE:
            remote_config->group.type = connector_remote_group_state;
            break;
    }

    switch (query_setting_attributes->compare_to)
    {
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE:
            remote_config->attribute.compare_to = rci_query_setting_attribute_compare_to_none;
            break;
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_CURRENT:
            remote_config->attribute.compare_to = rci_query_setting_attribute_compare_to_current;
            break;
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_STORED:
            remote_config->attribute.compare_to = rci_query_setting_attribute_compare_to_stored;
            break;
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_DEFAULTS:
            remote_config->attribute.compare_to = rci_query_setting_attribute_compare_to_defaults;
            break;
    }

    switch (query_setting_attributes->source)
    {
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT:
            remote_config->attribute.source = rci_query_setting_attribute_source_current;
            break;
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS:
            remote_config->attribute.source = rci_query_setting_attribute_source_defaults;
            break;
        case CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_STORED:
            remote_config->attribute.source = rci_query_setting_attribute_source_stored;
            break;
    }

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_action_start;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

static void group_start(connector_remote_config_t * const remote_config, unsigned int const group_id, unsigned int const group_index, ccapi_bool_t compare_matches, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.query_setting.matches = CCAPI_FALSE;
    expected_rci_info.group.instance = group_index;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

    rci_info_to_return = expected_rci_info;
    rci_info_to_return.query_setting.matches = compare_matches;

    remote_config->group.id = group_id;
    remote_config->group.index = group_index;
    remote_config->error_id = 0;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_group_start;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
    CHECK_EQUAL(CCAPI_BOOL_TO_CONNECTOR_BOOL(compare_matches), remote_config->response.compare_matches);
}

static ccapi_rci_element_type_t connector_rci_element_type_to_ccapi(connector_element_value_type_t const ccfsm_type)
{
    ccapi_rci_element_type_t ccapi_type;
    switch (ccfsm_type)
    {
#if defined RCI_PARSER_USES_STRING
        case connector_element_type_string:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_STRING;
            break;
#endif
#if defined RCI_PARSER_USES_MULTILINE_STRING
        case connector_element_type_multiline_string:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_MULTILINE_STRING;
            break;
#endif
#if defined RCI_PARSER_USES_PASSWORD
        case connector_element_type_password:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_PASSWORD;
            break;
#endif
#if defined RCI_PARSER_USES_FQDNV4
        case connector_element_type_fqdnv4:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_FQDNV4;
            break;
#endif
#if defined RCI_PARSER_USES_FQDNV6
        case connector_element_type_fqdnv6:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_FQDNV6;
            break;
#endif
#if defined RCI_PARSER_USES_DATETIME
        case connector_element_type_datetime:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_DATETIME;
            break;
#endif
#if defined RCI_PARSER_USES_IPV4
        case connector_element_type_ipv4:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_IPV4;
            break;
#endif
#if defined RCI_PARSER_USES_MAC_ADDR
        case connector_element_type_mac_addr:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_MAC;
            break;
#endif
#if defined RCI_PARSER_USES_INT32
        case connector_element_type_int32:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_INT32;
            break;
#endif
#if defined RCI_PARSER_USES_UINT32
        case connector_element_type_uint32:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_UINT32;
            break;
#endif
#if defined RCI_PARSER_USES_HEX32
        case connector_element_type_hex32:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_HEX32;
            break;
#endif
#if defined RCI_PARSER_USES_0X_HEX32
        case connector_element_type_0x_hex32:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_0X32;
            break;
#endif
#if defined RCI_PARSER_USES_FLOAT
        case connector_element_type_float:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_FLOAT;
            break;
#endif
#if defined RCI_PARSER_USES_ENUM
        case connector_element_type_enum:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_ENUM;
            break;
#endif
#if defined RCI_PARSER_USES_ON_OFF
        case connector_element_type_on_off:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_ON_OFF;
            break;
#endif
#if defined RCI_PARSER_USES_BOOLEAN
        case connector_element_type_boolean:
            ccapi_type = CCAPI_RCI_ELEMENT_TYPE_BOOL;
            break;
#endif
    }
    return ccapi_type;
}

static void group_process_query(connector_remote_config_t * const remote_config, unsigned int const element_id, connector_element_value_type_t const element_type, connector_element_value_t const expected_value, ccapi_bool_t compare_matches, char const * const expected_function, unsigned int const error, char const * const error_hint)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;
    connector_element_value_t response_value;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.query_setting.matches = CCAPI_FALSE;
    expected_rci_info.element.type = connector_rci_element_type_to_ccapi(element_type);

    rci_info_to_return = expected_rci_info;
    rci_info_to_return.query_setting.matches = compare_matches;
    rci_info_to_return.error_hint = error_hint;

    remote_config->element.type = element_type;
    remote_config->element.id = element_id;
    remote_config->element.type = element_type;
    remote_config->response.element_value = &response_value;
    remote_config->error_id = 0;
    remote_config->element.value = NULL;


    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_group_process;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
    CHECK_EQUAL(CCAPI_BOOL_TO_CONNECTOR_BOOL(compare_matches), remote_config->response.compare_matches);
    STRCMP_EQUAL(remote_config->response.error_hint, error_hint);
    switch (element_type)
    {
        case connector_element_type_string:
        case connector_element_type_multiline_string:
        case connector_element_type_password:
        case connector_element_type_ipv4:
        case connector_element_type_fqdnv4:
        case connector_element_type_fqdnv6:
        case connector_element_type_mac_addr:
        case connector_element_type_datetime:
            STRCMP_EQUAL(expected_value.string_value, response_value.string_value);
            break;
        case connector_element_type_int32:
            CHECK_EQUAL(expected_value.signed_integer_value, response_value.signed_integer_value);
            break;
        case connector_element_type_uint32:
        case connector_element_type_hex32:
        case connector_element_type_0x_hex32:
            CHECK_EQUAL(expected_value.unsigned_integer_value, response_value.unsigned_integer_value);
            break;
        case connector_element_type_float:
            CHECK_EQUAL(expected_value.float_value, response_value.float_value);
            break;
        case connector_element_type_enum:
            CHECK_EQUAL(expected_value.enum_value, response_value.enum_value);
            break;
        case connector_element_type_on_off:
            CHECK_EQUAL(expected_value.on_off_value, response_value.on_off_value);
            break;
        case connector_element_type_boolean:
            CHECK_EQUAL(expected_value.boolean_value, response_value.boolean_value);
            break;
    }
}

static void group_process_set(connector_remote_config_t * const remote_config, unsigned int const element_id, connector_element_value_type_t const element_type, connector_element_value_t const set_value, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;
    connector_element_value_t element_value;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.query_setting.matches = CCAPI_FALSE;
    expected_rci_info.element.type = connector_rci_element_type_to_ccapi(element_type);

    rci_info_to_return = expected_rci_info;
    rci_info_to_return.query_setting.matches = CCAPI_FALSE;

    remote_config->element.type = element_type;
    remote_config->element.id = element_id;
    remote_config->element.type = element_type;
    remote_config->response.element_value = NULL;
    remote_config->error_id = 0;
    remote_config->element.value = &element_value;

    *remote_config->element.value = set_value;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_group_process;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
    CHECK_EQUAL(connector_false, remote_config->response.compare_matches);
    switch (element_type)
    {
        case connector_element_type_string:
        case connector_element_type_multiline_string:
        case connector_element_type_password:
        case connector_element_type_ipv4:
        case connector_element_type_fqdnv4:
        case connector_element_type_fqdnv6:
        case connector_element_type_mac_addr:
        case connector_element_type_datetime:
        {
            char const * const string_ptr = (char *)th_rci_get_value_ptr();
            STRCMP_EQUAL(set_value.string_value, string_ptr);
            break;
        }
        case connector_element_type_int32:
        {
            int32_t const * const int_ptr = (int32_t *)th_rci_get_value_ptr();
            CHECK_EQUAL(set_value.signed_integer_value, *int_ptr);
            break;
        }
        case connector_element_type_uint32:
        case connector_element_type_hex32:
        case connector_element_type_0x_hex32:
        {
            uint32_t const * const uint_ptr = (uint32_t *)th_rci_get_value_ptr();
            CHECK_EQUAL(set_value.unsigned_integer_value, *uint_ptr);
            break;
        }
        case connector_element_type_float:
        {
            float const * const float_ptr = (float *)th_rci_get_value_ptr();
            CHECK_EQUAL(set_value.float_value, *float_ptr);
            break;
        }
        case connector_element_type_enum:
        {
            unsigned int const * const enum_ptr = (unsigned int *)th_rci_get_value_ptr();
            CHECK_EQUAL(set_value.enum_value, *enum_ptr);
            break;
        }
            break;
        case connector_element_type_on_off:
        {
            connector_on_off_t const * const on_off_ptr = (connector_on_off_t *)th_rci_get_value_ptr();
            CHECK_EQUAL(set_value.on_off_value, *on_off_ptr);
            break;
        }
        case connector_element_type_boolean:
        {
            connector_bool_t const * const bool_ptr = (connector_bool_t *)th_rci_get_value_ptr();
            CHECK_EQUAL(set_value.boolean_value, *bool_ptr);
            break;
        }
    }
}

static void group_end(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    rci_info_to_return = expected_rci_info;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;
    remote_config->error_id = 0;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_group_end;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

static void do_command(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;
    connector_element_value_t set_value;
    connector_element_value_t response_value;

    remote_config->element.value = &set_value;
    remote_config->response.element_value = &response_value;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.do_command.target = "file_system";
    expected_rci_info.do_command.request = "<ls dir=\"/WEB/python\"/>";
    expected_rci_info.do_command.response = &remote_config->response.element_value->string_value;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

    rci_info_to_return = expected_rci_info;
    *rci_info_to_return.do_command.response = "My response";

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_do_command;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
    STRCMP_EQUAL("My response", *ccapi_data_single_instance->service.rci.rci_info.do_command.response);
}

static void reboot(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.do_command.target = NULL;
    expected_rci_info.do_command.request = NULL;
    expected_rci_info.do_command.response = NULL;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

    rci_info_to_return = expected_rci_info;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_reboot;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

static void set_factory_defaults(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.do_command.target = NULL;
    expected_rci_info.do_command.request = NULL;
    expected_rci_info.do_command.response = NULL;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

    rci_info_to_return = expected_rci_info;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_set_factory_def;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

static void action_end(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

    rci_info_to_return = expected_rci_info;
    remote_config->error_id = 0;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_action_end;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

void session_end(connector_remote_config_t * const remote_config, char const * const expected_function, unsigned int const error)
{
    connector_callback_status_t status;
    connector_request_id_t request;
    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;

    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;
    rci_info_to_return = expected_rci_info;
    remote_config->error_id = 0;

    th_rci_returnValues(error, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_session_end;
    status = call_ccapi_connector_callback(connector_class_id_remote_config, request, remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(error, remote_config->error_id);
    th_rci_checkExpectations(expected_function, &expected_rci_info);
}

void testRCIQuerySettingGroup1(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t expected_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);

    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_LOAD_FAIL, "Error hint");

    expected_value.unsigned_integer_value = 5;
    group_process_query(&remote_config, connector_setting_group_1_el_uint32, connector_element_type_uint32, expected_value, CCAPI_TRUE, "rci_setting_group_1_el_uint32_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.on_off_value = connector_on;
    group_process_query(&remote_config, connector_setting_group_1_el_on_off, connector_element_type_on_off, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_on_off_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.unsigned_integer_value = 0x20101010;
    group_process_query(&remote_config, connector_setting_group_1_el_hex, connector_element_type_hex32, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_hex_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.unsigned_integer_value = 0x20101010;
    group_process_query(&remote_config, connector_setting_group_1_el_0xhex, connector_element_type_0x_hex32, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_0xhex_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.signed_integer_value = -100;
    group_process_query(&remote_config, connector_setting_group_1_el_signed, connector_element_type_int32, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_signed_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.boolean_value = connector_true;
    group_process_query(&remote_config, connector_setting_group_1_el_bool, connector_element_type_boolean, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_bool_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.float_value = 1.2;
    group_process_query(&remote_config, connector_setting_group_1_el_float, connector_element_type_float, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_float_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCIQuerySettingGroup1)
{
    testRCIQuerySettingGroup1();
}

TEST(test_ccapi_rci, testRCIQuerySettingGroup1Busy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCIQuerySettingGroup1();
}

void testRCIQuerySettingGroup3(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t expected_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_3, 0, CCAPI_FALSE, "rci_setting_group_3_start", CCAPI_GLOBAL_ERROR_NONE);

    expected_value.string_value = "String";
    group_process_query(&remote_config, connector_setting_group_3_el_string, connector_element_type_string, expected_value, CCAPI_FALSE, "rci_setting_group_3_el_string_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.string_value = "Multiline\nString";
    group_process_query(&remote_config, connector_setting_group_3_el_multiline, connector_element_type_multiline_string, expected_value, CCAPI_FALSE, "rci_setting_group_3_el_multiline_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    group_end(&remote_config, "rci_setting_group_3_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCIQuerySettingGroup3)
{
    testRCIQuerySettingGroup3();
}

TEST(test_ccapi_rci, testRCIQuerySettingGroup3Busy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCIQuerySettingGroup3();
}

void testRCIQueryStateGroup2(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t expected_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_STATE, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_state_group_2, 0, CCAPI_FALSE, "rci_state_group_2_start", CCAPI_GLOBAL_ERROR_NONE);

    expected_value.string_value = "192.168.1.1";
    group_process_query(&remote_config, connector_state_group_2_el_ip, connector_element_type_ipv4, expected_value, CCAPI_FALSE, "rci_state_group_2_el_ip_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.string_value = "www.digi.com";
    group_process_query(&remote_config, connector_state_group_2_el_fqdnv4, connector_element_type_fqdnv4, expected_value, CCAPI_FALSE, "rci_state_group_2_el_fqdnv4_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.string_value = "2001:0db8:85a3:0042:1000:8a2e:0370:7334";
    group_process_query(&remote_config, connector_state_group_2_el_fqdnv6, connector_element_type_fqdnv6, expected_value, CCAPI_FALSE, "rci_state_group_2_el_fqdnv6_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.string_value = "00:04:9D:AB:CD:EF";
    group_process_query(&remote_config, connector_state_group_2_el_mac, connector_element_type_mac_addr, expected_value, CCAPI_FALSE, "rci_state_group_2_el_mac_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    expected_value.string_value = "2002-05-30T09:30:10-0600";
    group_process_query(&remote_config, connector_state_group_2_el_datetime, connector_element_type_datetime, expected_value, CCAPI_FALSE, "rci_state_group_2_el_datetime_get", CCAPI_GLOBAL_ERROR_NONE, NULL);

    group_end(&remote_config, "rci_state_group_2_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCIQueryStateGroup2)
{
    testRCIQueryStateGroup2();
}

TEST(test_ccapi_rci, testRCIQueryStateGroup2Busy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCIQueryStateGroup2();
}

void testRCISetSettingGroup3(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t set_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    action_start(&remote_config, CCAPI_RCI_ACTION_SET, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_3, 0, CCAPI_FALSE, "rci_setting_group_3_start", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "Some string";
    group_process_set(&remote_config, connector_setting_group_3_el_string, connector_element_type_string, set_value, "rci_setting_group_3_el_string_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "Some\nmultiline\nstring";
    group_process_set(&remote_config, connector_setting_group_3_el_multiline, connector_element_type_multiline_string, set_value, "rci_setting_group_3_el_multiline_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "Password***";
    group_process_set(&remote_config, connector_setting_group_3_el_password, connector_element_type_password, set_value, "rci_setting_group_3_el_password_set", CCAPI_GLOBAL_ERROR_NONE);

    group_end(&remote_config, "rci_setting_group_3_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCISetSettingGroup3)
{
    testRCISetSettingGroup3();
}

TEST(test_ccapi_rci, testRCISetSettingGroup3Busy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCISetSettingGroup3();
}

void testRCISetSettingGroup1(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t set_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    action_start(&remote_config, CCAPI_RCI_ACTION_SET, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);

    set_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_ONE;
    group_process_set(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, set_value, "rci_setting_group_1_el_enum_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.unsigned_integer_value = 123;
    group_process_set(&remote_config, connector_setting_group_1_el_uint32, connector_element_type_uint32, set_value, "rci_setting_group_1_el_uint32_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.on_off_value = connector_off;
    group_process_set(&remote_config, connector_setting_group_1_el_on_off, connector_element_type_on_off, set_value, "rci_setting_group_1_el_on_off_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.unsigned_integer_value = 0x123456;
    group_process_set(&remote_config, connector_setting_group_1_el_hex, connector_element_type_hex32, set_value, "rci_setting_group_1_el_hex_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.unsigned_integer_value = 0x20564;
    group_process_set(&remote_config, connector_setting_group_1_el_0xhex, connector_element_type_0x_hex32, set_value, "rci_setting_group_1_el_0xhex_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.signed_integer_value = -256;
    group_process_set(&remote_config, connector_setting_group_1_el_signed, connector_element_type_int32, set_value, "rci_setting_group_1_el_signed_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.boolean_value = connector_true;
    group_process_set(&remote_config, connector_setting_group_1_el_bool, connector_element_type_boolean, set_value, "rci_setting_group_1_el_bool_set", CCAPI_SETTING_GROUP_1_ERROR_ERROR_1);

    set_value.float_value = -15.26;
    group_process_set(&remote_config, connector_setting_group_1_el_float, connector_element_type_float, set_value, "rci_setting_group_1_el_float_set", CCAPI_GLOBAL_ERROR_NONE);


    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCISetSettingGroup1)
{
    testRCISetSettingGroup1();
}

TEST(test_ccapi_rci, testRCISetSettingGroup1Busy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCISetSettingGroup1();
}

TEST(test_ccapi_rci, testRCIDoCommand)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    action_start(&remote_config, CCAPI_RCI_ACTION_DO_COMMAND, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    do_command(&remote_config, "rci_do_command_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCIReboot)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    action_start(&remote_config, CCAPI_RCI_ACTION_REBOOT, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    reboot(&remote_config, "rci_reboot_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCISetFactoryDefaults)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    action_start(&remote_config, CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    set_factory_defaults(&remote_config, "rci_set_factory_defaults_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

void testRCISetStateGroup2(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t set_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;

    action_start(&remote_config, CCAPI_RCI_ACTION_SET, CCAPI_RCI_GROUP_STATE, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_state_group_2, 0, CCAPI_FALSE, "rci_state_group_2_start", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "192.168.1.1";
    group_process_set(&remote_config, connector_state_group_2_el_ip, connector_element_type_ipv4, set_value, "rci_state_group_2_el_ip_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "www.digi.com";
    group_process_set(&remote_config, connector_state_group_2_el_fqdnv4, connector_element_type_fqdnv4, set_value, "rci_state_group_2_el_fqdnv4_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "2001:b00b:85a3:0042:1000:8a2e:0370:7334";
    group_process_set(&remote_config, connector_state_group_2_el_fqdnv6, connector_element_type_fqdnv6, set_value, "rci_state_group_2_el_fqdnv6_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "12:34:56:78:9A:BC";
    group_process_set(&remote_config, connector_state_group_2_el_mac, connector_element_type_mac_addr, set_value, "rci_state_group_2_el_mac_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "1988-05-30T09:30:10-0600";
    group_process_set(&remote_config, connector_state_group_2_el_datetime, connector_element_type_datetime, set_value, "rci_state_group_2_el_datetime_set", CCAPI_GLOBAL_ERROR_NONE);

    group_end(&remote_config, "rci_state_group_2_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);
    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

TEST(test_ccapi_rci, testRCISetStateGroup2)
{
    testRCISetStateGroup2();
}

TEST(test_ccapi_rci, testRCISetStateGroup2Busy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCISetStateGroup2();
}

void testRCIQuerySettingGroup1AttributesMix(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t expected_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_DEFAULTS;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_TRUE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_CURRENT;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_TRUE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_STORED;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_TRUE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_STORED;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_FALSE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}

void testRCIMultipleCommandsSameSession(void)
{
    connector_remote_config_t remote_config;
    ccapi_rci_query_setting_attributes_t query_setting_attributes;
    connector_element_value_t expected_value;
    connector_element_value_t set_value;

    session_start(&remote_config, "rci_session_start_cb", CCAPI_GLOBAL_ERROR_NONE);

    query_setting_attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_DEFAULTS;
    query_setting_attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS;

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_TRUE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_start(&remote_config, CCAPI_RCI_ACTION_SET, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_3, 0, CCAPI_FALSE, "rci_setting_group_3_start", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "Some string";
    group_process_set(&remote_config, connector_setting_group_3_el_string, connector_element_type_string, set_value, "rci_setting_group_3_el_string_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "Some\nmultiline\nstring";
    group_process_set(&remote_config, connector_setting_group_3_el_multiline, connector_element_type_multiline_string, set_value, "rci_setting_group_3_el_multiline_set", CCAPI_GLOBAL_ERROR_NONE);

    set_value.string_value = "Password***";
    group_process_set(&remote_config, connector_setting_group_3_el_password, connector_element_type_password, set_value, "rci_setting_group_3_el_password_set", CCAPI_GLOBAL_ERROR_NONE);

    group_end(&remote_config, "rci_setting_group_3_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_start(&remote_config, CCAPI_RCI_ACTION_DO_COMMAND, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    do_command(&remote_config, "rci_do_command_cb", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_start(&remote_config, CCAPI_RCI_ACTION_QUERY, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    group_start(&remote_config, connector_setting_group_1, 0, CCAPI_FALSE, "rci_setting_group_1_start", CCAPI_GLOBAL_ERROR_NONE);
    expected_value.enum_value = CCAPI_SETTING_GROUP_1_EL_ENUM_THREE;
    group_process_query(&remote_config, connector_setting_group_1_el_enum, connector_element_type_enum, expected_value, CCAPI_TRUE, "rci_setting_group_1_el_enum_get", CCAPI_GLOBAL_ERROR_NONE, NULL);
    group_end(&remote_config, "rci_setting_group_1_end", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);

    action_start(&remote_config, CCAPI_RCI_ACTION_REBOOT, CCAPI_RCI_GROUP_SETTING, &query_setting_attributes, "rci_action_start_cb", CCAPI_GLOBAL_ERROR_NONE);
    reboot(&remote_config, "rci_reboot_cb", CCAPI_GLOBAL_ERROR_NONE);
    action_end(&remote_config, "rci_action_end_cb", CCAPI_GLOBAL_ERROR_NONE);

    session_end(&remote_config, "rci_session_end_cb", CCAPI_GLOBAL_ERROR_NONE);
}
TEST(test_ccapi_rci, testRCIQuerySettingGroup1AttributesMix)
{
    testRCIQuerySettingGroup1AttributesMix();
}

TEST(test_ccapi_rci, testRCIQuerySettingGroup1AttributesMixBusy)
{
    ccapi_rci_lock_cb = CCAPI_TRUE;
    testRCIQuerySettingGroup1AttributesMix();
}

TEST(test_ccapi_rci, testRCISessionCancel)
{
    connector_remote_config_t remote_config;
    connector_callback_status_t status;
    connector_request_id_t request;

    ccapi_rci_info_t rci_info_to_return;
    ccapi_rci_info_t expected_rci_info;
    char const * const expected_function = "rci_session_start_cb";

    set_remote_config_defaults(&remote_config);
    set_rci_info_defaults(&ccapi_data_single_instance->service.rci.rci_info);
    expected_rci_info = ccapi_data_single_instance->service.rci.rci_info;
    expected_rci_info.element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;
    rci_info_to_return = expected_rci_info;

    th_rci_returnValues(CCAPI_GLOBAL_ERROR_NONE, &rci_info_to_return);
    request.remote_config_request = connector_request_id_remote_config_session_start;

    ccapi_rci_lock_cb = CCAPI_TRUE;

    status = ccapi_connector_callback(connector_class_id_remote_config, request, &remote_config, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_busy, status);

    CHECK(ccapi_data_single_instance->service.rci.rci_thread_status > CCAPI_RCI_THREAD_IDLE);

    /* Wait user locks callback */
    do
    {
        status = ccapi_connector_callback(connector_class_id_remote_config, request, &remote_config, ccapi_data_single_instance);
        sched_yield();
    } while (th_rci_last_function_called() == NULL);
    CHECK_EQUAL(connector_callback_busy, status);
    th_rci_checkExpectations(expected_function, &expected_rci_info);

    CHECK_EQUAL(ccapi_data_single_instance->service.rci.rci_thread_status, CCAPI_RCI_THREAD_CB_QUEUED);

    request.remote_config_request = connector_request_id_remote_config_session_cancel;
    status = ccapi_connector_callback(connector_class_id_remote_config, request, &remote_config, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, status);

    CHECK_EQUAL(ccapi_data_single_instance->service.rci.rci_thread_status, CCAPI_RCI_THREAD_IDLE);

    /* Now release the lock and check status remains idle when session_start callback finish */
    ccapi_data_single_instance->service.rci.queued_callback.error = CCAPI_GLOBAL_ERROR_BAD_COMMAND;
    ccapi_rci_lock_cb = CCAPI_FALSE;
    do
    {
        sched_yield();
        CHECK_EQUAL(ccapi_data_single_instance->service.rci.rci_thread_status, CCAPI_RCI_THREAD_IDLE);
    } while (ccapi_data_single_instance->service.rci.queued_callback.error == CCAPI_GLOBAL_ERROR_BAD_COMMAND);

    CHECK_EQUAL(ccapi_data_single_instance->service.rci.rci_thread_status, CCAPI_RCI_THREAD_IDLE);
}
