/*
* Copyright (c) 2017, 2018 Digi International Inc.
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

#define CCAPI_CONST_PROTECTION_UNLOCK

#include "ccapi_definitions.h"

#if (defined CCIMP_RCI_SERVICE_ENABLED)

#define connector_rci_error_none 0

#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
#define CLEAR_GROUP_NAME(rci_info) (rci_info)->group.name = NULL
#define CLEAR_LIST_NAME(rci_info, index) (rci_info)->list.data[index].name = NULL
#define COPY_GROUP_NAME(rci_info, remote_config) (rci_info)->group.name = (remote_config)->group.name
#define COPY_LIST_NAME(rci_info, remote_config, index) (rci_info)->list.data[index].name = (remote_config)->list.level[index].name
#else
#define CLEAR_GROUP_NAME(rci_info)
#define CLEAR_LIST_NAME(rci_info, index)
#define COPY_GROUP_NAME(rci_info, remote_config)
#define COPY_LIST_NAME(rci_info, remote_config, index)
#endif

#if (defined RCI_PARSER_USES_ELEMENT_NAMES)
#define CLEAR_ELEMENT_NAME(rci_info) (rci_info)->element.name = NULL
#define COPY_ELEMENT_NAME(rci_info, remote_config) (rci_info)->element.name = (remote_config)->element.name
#else
#define CLEAR_ELEMENT_NAME(rci_info)
#define COPY_ELEMENT_NAME(rci_info, remote_config)
#endif

static ccapi_response_item_t * ccapi_response_item_from_connector_response_item(connector_response_item_t const * const src)
{
    ccapi_response_item_t const dst;
    connector_response_item_t const * const s = src;
    ccapi_response_item_t const * const d = &dst;

    ASSERT(sizeof *s == sizeof *d);
    ASSERT(sizeof s->count == sizeof d->count);
    ASSERT(sizeof s->dictionary == sizeof d->dictionary);
    ASSERT(offsetof(connector_response_item_t, count) == offsetof(ccapi_response_item_t, count));
    ASSERT(offsetof(connector_response_item_t, dictionary) == offsetof(ccapi_response_item_t, dictionary));

    return (ccapi_response_item_t * const) src;
}

static ccapi_element_value_t * ccapi_element_value_from_connector_element_value(connector_element_value_t const * const src)
{
    ccapi_element_value_t const dst;
    connector_element_value_t const * const s = src;
    ccapi_element_value_t const * const d = &dst;

    ASSERT(sizeof *s == sizeof *d);

    return (ccapi_element_value_t * const) src;
}

static void clear_group_item(ccapi_rci_info_t * const rci_info)
{
    switch (rci_info->group.collection_type)
    {
    case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
        rci_info->group.item.index = 0;
        break;

    case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
        rci_info->group.item.key = NULL;
    }
}

static void clear_group_info(ccapi_rci_info_t * const rci_info)
{
    rci_info->group.id = 0;
    clear_group_item(rci_info);
    CLEAR_GROUP_NAME(rci_info);
}

static void clear_list_item(ccapi_rci_info_t * const rci_info, unsigned int const index)
{
    switch (rci_info->list.data[index].collection_type)
    {
    case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
        rci_info->list.data[index].item.index = 0;
        break;

    case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
        rci_info->list.data[index].item.key = NULL;
    }
}

static void clear_list_info(ccapi_rci_info_t * const rci_info, unsigned int const index)
{
    rci_info->list.data[index].id = 0;
    clear_list_item(rci_info, index);
    CLEAR_LIST_NAME(rci_info, index);
}

static void clear_all_list_info(ccapi_rci_info_t * const rci_info)
{
    unsigned int index;

    rci_info->list.depth = 0;
    for (index = 0; index < RCI_LIST_MAX_DEPTH; index++)
    {
        clear_list_info(rci_info, index);
    }
}

static void clear_element_info(ccapi_rci_info_t * const rci_info)
{
    rci_info->element.id = 0;
    CLEAR_ELEMENT_NAME(rci_info);
}

static ccapi_rci_query_setting_attribute_compare_to_t connector_to_ccapi_compare_to_attribute(rci_query_setting_attribute_compare_to_t const compare_to)
{
    ccapi_rci_query_setting_attribute_compare_to_t retval = INVALID_ENUM(ccapi_rci_query_setting_attribute_compare_to_t);

    switch (compare_to)
    {
        case rci_query_setting_attribute_compare_to_none:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
            break;
        case rci_query_setting_attribute_compare_to_current:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_CURRENT;
            break;
        case rci_query_setting_attribute_compare_to_stored:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_STORED;
            break;
        case rci_query_setting_attribute_compare_to_defaults:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_DEFAULTS;
            break;
    }

    return retval;
}

static ccapi_rci_query_setting_attribute_source_t connector_to_ccapi_source_attribute(rci_query_setting_attribute_source_t const source)
{
    ccapi_rci_query_setting_attribute_source_t retval = INVALID_ENUM(ccapi_rci_query_setting_attribute_source_t);

    switch (source)
    {
        case rci_query_setting_attribute_source_current:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;
            break;
        case rci_query_setting_attribute_source_stored:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_STORED;
            break;
        case rci_query_setting_attribute_source_defaults:
            retval = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS;
            break;
    }

    return retval;
}

#if (defined RCI_ENUMS_AS_STRINGS)

static connector_element_t const * get_ccfsm_element_enum_element(connector_remote_config_data_t const * const rci_internal_data,
    connector_remote_group_type_t const ccfsm_group_type, unsigned int const group_id, unsigned int const element_id, connector_remote_list_t const list)
{
    connector_group_t const group = rci_internal_data->group_table[ccfsm_group_type].groups[group_id];
    connector_collection_t const * c_collection =  &group.collection;
    unsigned int i;

    for (i = 0; i < list.depth; i++)
    {
        connector_item_t const * const c_item = &c_collection->item.data[list.level[i].id];
        c_collection = c_item->data.collection;
    }

    connector_element_t const * const element = c_collection->item.data[element_id].data.element;
    ASSERT(group_id < rci_internal_data->group_table[ccfsm_group_type].count);

    return element;
}

static char const * enum_to_string(connector_element_enum_t const * const element_enum_info, unsigned int enum_id)
{
    return element_enum_info[enum_id].name;
}

static int string_to_enum(connector_element_enum_t const * const element_enum_info, unsigned int const enum_count, char const * const enum_string)
{
    int enum_id = -1;
    unsigned int i;

    ASSERT(enum_string != NULL);
    for (i = 0; i < enum_count; i++)
    {
        if (strcmp(element_enum_info[i].name, enum_string) == 0)
        {
            enum_id = i;
            break;
        }
    }

    return enum_id;
}

static void queue_enum_callback(ccapi_data_t * const ccapi_data, connector_remote_config_t const * const remote_config)
{
    connector_remote_config_data_t const * const rci_desc = ccapi_data->service.rci.rci_data->rci_desc;
    unsigned int const group_id = remote_config->group.id;
    unsigned int const element_id = remote_config->element.id;
    connector_remote_group_type_t const group_type = remote_config->group.type;

    connector_element_t const * enum_element = get_ccfsm_element_enum_element(rci_desc, group_type, group_id, element_id, remote_config->list);

    ccapi_data->service.rci.callback.enum_data.array = enum_element->enums.data;
    ccapi_data->service.rci.callback.enum_data.element_count = enum_element->enums.count;
}
#endif

void ccapi_rci_thread(void * const argument)
{
    ccapi_data_t * const ccapi_data = argument;

    /* ccapi_data is corrupted, it's likely the implementer made it wrong passing argument to the new thread */
    ASSERT_MSG_GOTO(ccapi_data != NULL, done);

    ccapi_data->thread.rci->status = CCAPI_THREAD_RUNNING;
    while (ccapi_data->thread.rci->status == CCAPI_THREAD_RUNNING)
    {
        ccapi_lock_acquire(ccapi_data->thread.rci->lock);

        if (ccapi_data->thread.rci->status != CCAPI_THREAD_REQUEST_STOP)
        {
            ccapi_rci_info_t * const rci_info = &ccapi_data->service.rci.rci_info;

            ASSERT_MSG_GOTO(ccapi_data->service.rci.rci_thread_status == CCAPI_RCI_THREAD_CB_QUEUED, done);

            /* Pass data to the user */
            switch (ccapi_data->service.rci.callback.type)
            {
                case ccapi_callback_type_none:
                {
                    ASSERT_MSG_GOTO(ccapi_data->service.rci.callback.type != ccapi_callback_type_none, done);
                    break;
                }

                case ccapi_callback_type_base:
                {
                    ccapi_rci_function_base_t const function = ccapi_data->service.rci.callback.as.base.function;

                    ASSERT_MSG_GOTO(function != NULL, done);

                    ccapi_data->service.rci.callback.error = function(rci_info);
                    break;
                }

                case ccapi_callback_type_lock:
                {
                    ccapi_rci_function_lock_t const function = ccapi_data->service.rci.callback.as.lock.function;
                    ccapi_response_item_t * const item = ccapi_data->service.rci.callback.as.lock.item;

                    ASSERT_MSG_GOTO(function != NULL, done);
                    ASSERT_MSG_GOTO(item != NULL, done);

                    ccapi_data->service.rci.callback.error = function(rci_info, item);
                    break;
                }

                case ccapi_callback_type_element:
                {
                    ccapi_rci_function_element_t const function = ccapi_data->service.rci.callback.as.element.function;

                    ASSERT_MSG_GOTO(function != NULL, done);

#if (defined RCI_ENUMS_AS_STRINGS)
                    connector_element_enum_t const * const enum_array = ccapi_data->service.rci.callback.enum_data.array;

                    if (enum_array != NULL)
                    {
                        unsigned int const enum_element_count = ccapi_data->service.rci.callback.enum_data.element_count;

                        if (rci_info->action == CCAPI_RCI_ACTION_QUERY)
                        {
                            ccapi_element_value_t element_value = { 0 };
                            int enum_id;
                            unsigned int * const actual_value = &ccapi_data->service.rci.callback.as.element.value->enum_value;

                            ASSERT_MSG_GOTO(actual_value != NULL, done);

                            ccapi_data->service.rci.callback.error = function(rci_info, &element_value);
                            if (ccapi_data->service.rci.callback.error == connector_rci_error_none)
                            {
                                enum_id = string_to_enum(enum_array, enum_element_count, element_value.string_value);
                                if (enum_id < 0)
                                {
                                    ccapi_data->service.rci.callback.error = connector_protocol_error_bad_value;
                                }
                                *actual_value = (unsigned) enum_id;
                            }
                        }
                        else
                        {
                            ccapi_element_value_t element_value;
                            ccapi_element_value_t * const reference_value = ccapi_data->service.rci.callback.as.element.value;

                            ASSERT_MSG_GOTO(reference_value != NULL, done);
                            {
                                unsigned int const actual_value = reference_value->enum_value;

                                ASSERT(actual_value < enum_element_count);
                                element_value.string_value = enum_to_string(enum_array, actual_value);
                                ccapi_data->service.rci.callback.error = function(rci_info, &element_value);
                            }
                        }
                    }
                    else
#endif
                    {
                        ccapi_element_value_t * const value = ccapi_data->service.rci.callback.as.element.value;

                        ASSERT_MSG_GOTO(value != NULL, done);

                        ccapi_data->service.rci.callback.error = function(rci_info, value);
                    }
                    break;
                }
                
                case ccapi_callback_type_transform:
                {
                    ccapi_rci_function_transform_t const function = ccapi_data->service.rci.callback.as.transform.function;
                    char const ** const transformed = ccapi_data->service.rci.callback.as.transform.transformed;

                    ASSERT_MSG_GOTO(function != NULL, done);

#if (defined RCI_ENUMS_AS_STRINGS)
                    connector_element_enum_t const * const enum_array = ccapi_data->service.rci.callback.enum_data.array;

                    if (enum_array != NULL)
                    {
                        ccapi_element_value_t element_value;
                        ccapi_element_value_t * const reference_value = ccapi_data->service.rci.callback.as.transform.value;
                        unsigned int const enum_element_count = ccapi_data->service.rci.callback.enum_data.element_count;

                        ASSERT_MSG_GOTO(reference_value != NULL, done);
                        {
                            unsigned int const actual_value = reference_value->enum_value;

                            ASSERT(actual_value < enum_element_count);
                            element_value.string_value = enum_to_string(enum_array, actual_value);
                            ccapi_data->service.rci.callback.error = function(rci_info, &element_value, transformed);
                        }
                    }
                    else
#endif
                    {
                        ccapi_element_value_t * const value = ccapi_data->service.rci.callback.as.transform.value;

                        ASSERT_MSG_GOTO(value != NULL, done);

                        ccapi_data->service.rci.callback.error = function(rci_info, value, transformed);
                    }
                    break;
                }
            }

            /* Check if ccfsm has called cancel callback while we were waiting for the user */
            if (ccapi_data->service.rci.rci_thread_status == CCAPI_RCI_THREAD_CB_QUEUED)
            {
                ccapi_data->service.rci.rci_thread_status = CCAPI_RCI_THREAD_CB_PROCESSED;
            }
        }
    }
    ASSERT_MSG_GOTO(ccapi_data->thread.rci->status == CCAPI_THREAD_REQUEST_STOP, done);

done:
    ccapi_data->thread.rci->status = CCAPI_THREAD_NOT_STARTED;
    return;
}

static void clear_callback(ccapi_data_t * const ccapi_data)
{
    ccapi_data->service.rci.callback.type = ccapi_callback_type_none;
    ccapi_data->service.rci.callback.error = connector_rci_error_none;
#if (defined RCI_ENUMS_AS_STRINGS)
    ccapi_data->service.rci.callback.enum_data.array = NULL;
    ccapi_data->service.rci.callback.enum_data.element_count = 0;
#endif
}

static void copy_group_item(ccapi_rci_info_t * const rci_info, connector_remote_config_t const * const remote_config)
{
    switch (rci_info->group.collection_type)
    {
    case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
        rci_info->group.item.index = remote_config->group.item.index;
        break;

    case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
        rci_info->group.item.key = remote_config->group.item.key;
    }
}

static void copy_list_item(ccapi_rci_info_t * const rci_info, connector_remote_config_t const * const remote_config, unsigned int index)
{
    switch (rci_info->list.data[index].collection_type)
    {
    case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
        rci_info->list.data[index].item.index = remote_config->list.level[index].item.index;
        break;

    case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
    case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
        rci_info->list.data[index].item.key = remote_config->list.level[index].item.key;
    }
}

connector_callback_status_t ccapi_rci_handler(connector_request_id_remote_config_t const request_id, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t status = connector_callback_error;
    connector_remote_config_t * const remote_config = data;
    ccapi_rci_data_t const * const rci_data = ccapi_data->service.rci.rci_data;
    ccapi_rci_info_t * const rci_info = &ccapi_data->service.rci.rci_info;

    if (request_id == connector_request_id_remote_config_session_cancel)
    {
        clear_callback(ccapi_data);

        ccapi_data->service.rci.rci_thread_status = CCAPI_RCI_THREAD_IDLE;

        status = connector_callback_continue;

        goto done;
    }

    switch (ccapi_data->service.rci.rci_thread_status)
    {
        case CCAPI_RCI_THREAD_IDLE:
        {
            ASSERT(ccapi_data->service.rci.callback.type == ccapi_callback_type_none);
            clear_callback(ccapi_data);

            rci_info->user_context = remote_config->user_context;
            rci_info->error_hint = remote_config->response.error_hint;

            switch (request_id)
            {
                case connector_request_id_remote_config_session_start:
                    if (rci_data->callback.start_session != NULL)
                    {
                        rci_info->action = CCAPI_RCI_ACTION_QUERY;
                        rci_info->error_hint = NULL;
                        rci_info->query_setting.matches = CCAPI_FALSE;
                        rci_info->query_setting.attributes.compare_to = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE;
                        rci_info->query_setting.attributes.source = CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT;
                        rci_info->group.type = CCAPI_RCI_GROUP_SETTING;
                        clear_group_info(rci_info);
                        clear_all_list_info(rci_info);
                        clear_element_info(rci_info);
                        rci_info->do_command.target = NULL;
                        rci_info->do_command.request = NULL;
                        rci_info->do_command.response = NULL;

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.start_session;
                    }
                    break;

                case connector_request_id_remote_config_action_start:
                    if (rci_data->callback.start_action != NULL)
                    {
                        connector_remote_group_type_t const group_type = remote_config->group.type;

                        switch (remote_config->action)
                        {
                            case connector_remote_action_set:
                                rci_info->action = CCAPI_RCI_ACTION_SET;
                                break;
                            case connector_remote_action_query:
                                rci_info->action = CCAPI_RCI_ACTION_QUERY;
                                break;
                            case connector_remote_action_do_command:
                                rci_info->action = CCAPI_RCI_ACTION_DO_COMMAND;
                                break;
                            case connector_remote_action_reboot:
                                rci_info->action = CCAPI_RCI_ACTION_REBOOT;
                                break;
                            case connector_remote_action_set_factory_def:
                                rci_info->action = CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS;
                                break;
                        }

                        switch (group_type)
                        {
                            case connector_remote_group_setting:
                                rci_info->group.type = CCAPI_RCI_GROUP_SETTING;
                                break;
                            case connector_remote_group_state:
                                rci_info->group.type = CCAPI_RCI_GROUP_STATE;
                                break;
                        }

                        if (rci_info->action == CCAPI_RCI_ACTION_QUERY && rci_info->group.type == CCAPI_RCI_GROUP_SETTING)
                        {
                            rci_info->query_setting.attributes.compare_to = connector_to_ccapi_compare_to_attribute(remote_config->attribute.compare_to);
                            rci_info->query_setting.attributes.source = connector_to_ccapi_source_attribute(remote_config->attribute.source);
                            rci_info->query_setting.matches = CCAPI_FALSE;
                        }
                        else if (rci_info->action == CCAPI_RCI_ACTION_SET && rci_info->group.type == CCAPI_RCI_GROUP_SETTING)
                        {
                            rci_info->set_setting.attributes.embed_transformed_values = CCAPI_BOOL(remote_config->attribute.embed_transformed_values == connector_true);
                        }

                        clear_group_info(rci_info);
                        clear_all_list_info(rci_info);
                        clear_element_info(rci_info);

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.start_action;
                    }
                    break;

                case connector_request_id_remote_config_do_command:
                    if (rci_data->callback.do_command != NULL)
                    {
                        ccapi_data->service.rci.rci_info.do_command.target = remote_config->attribute.target;
                        ccapi_data->service.rci.rci_info.do_command.request = remote_config->element.value->string_value;
                        ccapi_data->service.rci.rci_info.do_command.response = &remote_config->response.element_value->string_value;

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.do_command;
                    }
                    break;

                case connector_request_id_remote_config_reboot:
                    if (rci_data->callback.reboot != NULL)
                    {
                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.reboot;
                    }
                    break;

                case connector_request_id_remote_config_set_factory_def:
                    if (rci_data->callback.set_factory_defaults != NULL)
                    {
                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.set_factory_defaults;
                    }
                    break;

                case connector_request_id_remote_config_group_instances_lock:
                    if (rci_data->callback.lock_group_instances != NULL)
                    {
                        rci_info->group.id = remote_config->group.id;
                        rci_info->group.collection_type = (ccapi_rci_collection_type_t)remote_config->group.collection_type;
                        COPY_GROUP_NAME(rci_info, remote_config);
                        clear_all_list_info(rci_info);
                        clear_element_info(rci_info);
                        switch (rci_info->group.collection_type)
                        {
                            case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
                            case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
                                ASSERT(0);
                                break;
                            case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
                            case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
                            {
                                ccapi_data->service.rci.callback.as.lock.item = ccapi_response_item_from_connector_response_item(&remote_config->response.item);
                                break;
                            }
                        }
                        ccapi_data->service.rci.callback.type = ccapi_callback_type_lock;
                        ccapi_data->service.rci.callback.as.lock.function = rci_data->callback.lock_group_instances;
                    }
                    break;

                case connector_request_id_remote_config_group_instances_set:
                    if (rci_data->callback.set_group_instances != NULL)
                    {
                        switch (rci_info->group.collection_type)
                        {
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
                            ASSERT(0);
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
                            rci_info->group.item.count = remote_config->group.item.count;
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
                            rci_info->group.item.dictionary.entries = remote_config->group.item.dictionary.entries;
                            rci_info->group.item.dictionary.keys = remote_config->group.item.dictionary.keys;
                            break;
                        }

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.set_group_instances;
                    }
                    break;

                case connector_request_id_remote_config_group_instance_remove:
                    if (rci_data->callback.remove_group_instance != NULL)
                    {
                        rci_info->group.id = remote_config->group.id;
                        rci_info->group.collection_type = (ccapi_rci_collection_type_t)remote_config->group.collection_type;
                        copy_group_item(rci_info, remote_config);
                        COPY_GROUP_NAME(rci_info, remote_config);
                        clear_all_list_info(rci_info);
                        clear_element_info(rci_info);
                        switch (rci_info->group.collection_type)
                        {
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
                            ASSERT(0);
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
                            ASSERT(0);
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
                            rci_info->group.item.key = remote_config->group.item.key;
                            break;
                        }

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.remove_group_instance;
                    }
                    break;

                case connector_request_id_remote_config_group_instances_unlock:
                    if (rci_data->callback.unlock_group_instances != NULL)
                    {
                        rci_info->group.id = remote_config->group.id;
                        rci_info->group.collection_type = (ccapi_rci_collection_type_t)remote_config->group.collection_type;
                        COPY_GROUP_NAME(rci_info, remote_config);
                        clear_all_list_info(rci_info);
                        clear_element_info(rci_info);

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.unlock_group_instances;
                        break;
                    }
                    break;

                case connector_request_id_remote_config_group_start:
                    if (rci_data->callback.start_group != NULL)
                    {
                        rci_info->group.id = remote_config->group.id;
                        rci_info->group.collection_type = (ccapi_rci_collection_type_t)remote_config->group.collection_type;
                        copy_group_item(rci_info, remote_config);
                        COPY_GROUP_NAME(rci_info, remote_config);
                        clear_all_list_info(rci_info);
                        clear_element_info(rci_info);

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.start_group;
                    }
                    break;

                case connector_request_id_remote_config_list_instances_lock:
                    if (rci_data->callback.lock_list_instances != NULL)
                    {
                        unsigned int const index = remote_config->list.depth - 1;

                        rci_info->list.depth = remote_config->list.depth;
                        rci_info->list.data[index].collection_type = (ccapi_rci_collection_type_t)remote_config->list.level[index].collection_type;
                        COPY_LIST_NAME(rci_info, remote_config, index);
                        clear_element_info(rci_info);

                        switch (rci_info->list.data[index].collection_type)
                        {
                            case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
                            case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
                                ASSERT(0);
                                break;
                            case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
                            case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
                            {
                                ccapi_data->service.rci.callback.as.lock.item = ccapi_response_item_from_connector_response_item(&remote_config->response.item);
                                break;
                            }
                        }

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_lock;
                        ccapi_data->service.rci.callback.as.lock.function = rci_data->callback.lock_list_instances;
                    }
                    break;

                case connector_request_id_remote_config_list_instances_set:
                    if (rci_data->callback.set_list_instances != NULL)
                    {
                        unsigned int const index = remote_config->list.depth - 1;

                        switch (rci_info->list.data[index].collection_type)
                        {
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
                            ASSERT(0);
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
                            rci_info->list.data[index].item.count = remote_config->list.level[index].item.count;
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
                            rci_info->list.data[index].item.dictionary.entries = remote_config->list.level[index].item.dictionary.entries;
                            rci_info->list.data[index].item.dictionary.keys = remote_config->list.level[index].item.dictionary.keys;
                            break;
                        }

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.set_list_instances;
                    }
                    break;

                case connector_request_id_remote_config_list_instance_remove:
                    if (rci_data->callback.remove_list_instance != NULL)
                    {
                        unsigned int const index = remote_config->list.depth - 1;

                        rci_info->list.depth = remote_config->list.depth;
                        rci_info->list.data[index].collection_type = (ccapi_rci_collection_type_t)remote_config->list.level[index].collection_type;
                        copy_list_item(rci_info, remote_config, index);
                        COPY_LIST_NAME(rci_info, remote_config, index);
                        clear_element_info(rci_info);

                        switch (rci_info->list.data[index].collection_type)
                        {
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY:
                        case CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY:
                            ASSERT(0);
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY:
                            ASSERT(0);
                            break;
                        case CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY:
                            rci_info->list.data[index].item.key = remote_config->list.level[index].item.key;
                            break;
                        }

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.remove_list_instance;
                    }
                    break;

                case connector_request_id_remote_config_list_instances_unlock:
                    if (rci_data->callback.unlock_list_instances != NULL)
                    {
                        unsigned int const index = remote_config->list.depth - 1;

                        rci_info->list.depth = remote_config->list.depth;
                        rci_info->list.data[index].collection_type = (ccapi_rci_collection_type_t)remote_config->list.level[index].collection_type;
                        COPY_LIST_NAME(rci_info, remote_config, index);
                        clear_element_info(rci_info);

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.unlock_list_instances;
                    }
                    break;

                case connector_request_id_remote_config_list_start:
                    if (rci_data->callback.start_list != NULL)
                    {
                        unsigned int const index = remote_config->list.depth - 1;

                        rci_info->list.depth = remote_config->list.depth;
                        rci_info->list.data[index].collection_type = (ccapi_rci_collection_type_t)remote_config->list.level[index].collection_type;
                        copy_list_item(rci_info, remote_config, index);
                        COPY_LIST_NAME(rci_info, remote_config, index);
                        clear_element_info(rci_info);

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.start_list;
                    }
                    break;

                case connector_request_id_remote_config_element_process:
#define HAVE_HANDLER(atype, ftype)  ((rci_info->action == atype) && (rci_data->callback.ftype) != NULL)
                    if (HAVE_HANDLER(CCAPI_RCI_ACTION_QUERY, get_element) || HAVE_HANDLER(CCAPI_RCI_ACTION_SET, set_element))
                    {
                        rci_info->list.depth = remote_config->list.depth;
                        rci_info->element.id = remote_config->element.id;
                        COPY_ELEMENT_NAME(rci_info, remote_config);
                        switch (rci_info->action)
                        {
                            case CCAPI_RCI_ACTION_QUERY:
                                ccapi_data->service.rci.callback.as.element.value = ccapi_element_value_from_connector_element_value(remote_config->response.element_value);

                                ccapi_data->service.rci.callback.type = ccapi_callback_type_element;
                                ccapi_data->service.rci.callback.as.element.function = rci_data->callback.get_element;
                                break;
                            case CCAPI_RCI_ACTION_SET:
                                if (rci_info->set_setting.attributes.embed_transformed_values == CCAPI_FALSE)
                                {
                                    ccapi_data->service.rci.callback.as.element.value = ccapi_element_value_from_connector_element_value(remote_config->element.value);

                                    ccapi_data->service.rci.callback.type = ccapi_callback_type_element;
                                    ccapi_data->service.rci.callback.as.element.function = rci_data->callback.set_element;
                                }
                                else
                                {
                                    ccapi_data->service.rci.callback.as.transform.value = ccapi_element_value_from_connector_element_value(remote_config->element.value);
                                    ccapi_data->service.rci.callback.as.transform.transformed = &remote_config->response.element_value->string_value;

                                    ccapi_data->service.rci.callback.type = ccapi_callback_type_transform;
                                    ccapi_data->service.rci.callback.as.transform.function = rci_data->callback.set_and_transform_element;
                                }
                                break;
                            case CCAPI_RCI_ACTION_DO_COMMAND:
                            case CCAPI_RCI_ACTION_REBOOT:
                            case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
                                ASSERT(0);
                                break;
                        }
                        switch (remote_config->element.type)
                        {
                            case connector_element_type_string:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_STRING;
                                break;
                            case connector_element_type_multiline_string:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_MULTILINE_STRING;
                                break;
                            case connector_element_type_password:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_PASSWORD;
                                break;
                            case connector_element_type_fqdnv4:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_FQDNV4;
                                break;
                            case connector_element_type_fqdnv6:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_FQDNV6;
                                break;
                            case connector_element_type_datetime:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_DATETIME;
                                break;
                            case connector_element_type_ipv4:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_IPV4;
                                break;
                            case connector_element_type_mac_addr:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_MAC;
                                break;
                            case connector_element_type_int32:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_INT32;
                                break;
                            case connector_element_type_uint32:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_UINT32;
                                break;
                            case connector_element_type_hex32:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_HEX32;
                                break;
                            case connector_element_type_0x_hex32:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_0X32;
                                break;
                            case connector_element_type_float:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_FLOAT;
                                break;
                            case connector_element_type_enum:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_ENUM;
#if (defined RCI_ENUMS_AS_STRINGS)
                                queue_enum_callback(ccapi_data, remote_config);
#endif
                                break;
                            case connector_element_type_on_off:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_ON_OFF;
                                break;
                            case connector_element_type_boolean:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_BOOL;
                                break;
                            case connector_element_type_ref_enum:
                                rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_REF_ENUM;
                                break;
                            case connector_element_type_list:
                                ASSERT(0);
                                break;
                        }
                    }
                    break;

                case connector_request_id_remote_config_list_end:
                    if (rci_data->callback.end_list != NULL)
                    {
                        rci_info->list.depth = remote_config->list.depth;

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.end_list;
                    }
                    break;

                case connector_request_id_remote_config_group_end:
                    if (rci_data->callback.end_group != NULL)
                    {
                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.end_group;
                    }
                    break;

                case connector_request_id_remote_config_action_end:
                    if (rci_data->callback.end_action != NULL)
                    {
                        rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.end_action;
                    }
                    break;

                case connector_request_id_remote_config_session_end:
                    if (rci_data->callback.end_session != NULL)
                    {
                        rci_info->element.type = CCAPI_RCI_ELEMENT_TYPE_NOT_SET;

                        ccapi_data->service.rci.callback.type = ccapi_callback_type_base;
                        ccapi_data->service.rci.callback.as.base.function = rci_data->callback.end_session;
                    }
                    break;

                case connector_request_id_remote_config_session_cancel:
                    ASSERT(connector_false);
                    break;
            }

            if (ccapi_data->service.rci.callback.type == ccapi_callback_type_none)
            {
                ccapi_data->service.rci.rci_thread_status = CCAPI_RCI_THREAD_CB_PROCESSED;
            }
            else
            {
                ccapi_data->service.rci.rci_thread_status = CCAPI_RCI_THREAD_CB_QUEUED;
                ccapi_lock_release(ccapi_data->thread.rci->lock);
            }

            status = connector_callback_busy;
            break;
        }
        case CCAPI_RCI_THREAD_CB_QUEUED:
        {
            status = connector_callback_busy;

            break;
        }
        case CCAPI_RCI_THREAD_CB_PROCESSED:
        {
            switch (request_id)
            {
                case connector_request_id_remote_config_session_start:
                case connector_request_id_remote_config_action_start:
                    break;
                case connector_request_id_remote_config_do_command:
                case connector_request_id_remote_config_reboot:
                case connector_request_id_remote_config_set_factory_def:
                    break;
                case connector_request_id_remote_config_group_start:
                case connector_request_id_remote_config_list_start:
                {
                    if (rci_info->group.type == CCAPI_RCI_GROUP_SETTING && rci_info->action == CCAPI_RCI_ACTION_QUERY)
                    {
                        remote_config->response.compare_matches = CCAPI_BOOL_TO_CONNECTOR_BOOL(rci_info->query_setting.matches);
                    }
                    break;
                }
                case connector_request_id_remote_config_group_instances_lock:
                case connector_request_id_remote_config_group_instances_set:
                case connector_request_id_remote_config_group_instance_remove:
                case connector_request_id_remote_config_group_instances_unlock:
                    break;

                case connector_request_id_remote_config_list_instances_lock:
                case connector_request_id_remote_config_list_instances_set:
                case connector_request_id_remote_config_list_instance_remove:
                case connector_request_id_remote_config_list_instances_unlock:
                    break;
                case connector_request_id_remote_config_element_process:
                {
                    switch (rci_info->action)
                    {
                        case CCAPI_RCI_ACTION_QUERY:
                        {
                            if (rci_info->group.type == CCAPI_RCI_GROUP_SETTING)
                            {
                                remote_config->response.compare_matches = CCAPI_BOOL_TO_CONNECTOR_BOOL(rci_info->query_setting.matches);
                                rci_info->query_setting.matches = CCAPI_FALSE;
                            }
                            break;
                        }
                        case CCAPI_RCI_ACTION_SET:
                        case CCAPI_RCI_ACTION_DO_COMMAND:
                        case CCAPI_RCI_ACTION_REBOOT:
                        case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
                            break;
                    }
                    break;
                }
                case connector_request_id_remote_config_list_end:
                {
                    unsigned int const index = rci_info->list.depth - 1;

                    clear_list_info(rci_info, index);
                    clear_element_info(rci_info);
                    break;
                }
                case connector_request_id_remote_config_group_end:
                {
                    clear_group_info(rci_info);
                    clear_all_list_info(rci_info);
                    clear_element_info(rci_info);
                    break;
                }
                case connector_request_id_remote_config_action_end:
                    if (rci_info->action == CCAPI_RCI_ACTION_DO_COMMAND)
                    {
                        rci_info->do_command.target = NULL;
                        rci_info->do_command.request = NULL;
                        rci_info->do_command.response = NULL;
                    }
                    break;
                case connector_request_id_remote_config_session_end:
                    break;
                case connector_request_id_remote_config_session_cancel:
                    ASSERT(connector_false);
                    break;
            }

            remote_config->error_id = ccapi_data->service.rci.callback.error;
            remote_config->user_context = rci_info->user_context;
            remote_config->response.error_hint = rci_info->error_hint;

            clear_callback(ccapi_data);

            ccapi_data->service.rci.rci_thread_status = CCAPI_RCI_THREAD_IDLE;

            status = connector_callback_continue;

            break;
        }
    }

done:
    return status;
}

#endif
