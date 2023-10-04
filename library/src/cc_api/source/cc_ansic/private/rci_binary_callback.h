/*
 * Copyright (c) 2018 Digi International Inc.
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

#if defined(__GNUC__) && __GNUC__ >= 7
 #define FALL_THROUGH __attribute__ ((fallthrough));
#else
 #define FALL_THROUGH /* fall through */
#endif /* __GNUC__ >= 7 */

#define state_call(rci, value)  ((rci)->parser.state = (value))

STATIC connector_bool_t is_set_command(connector_remote_action_t const action)
{
    return connector_bool(action == connector_remote_action_set);
}

STATIC void rci_error(rci_t * const rci, unsigned int const id, char const * const description, char const * const hint)
{
    rci->shared.callback_data.error_id = id;
    rci->shared.callback_data.response.error_hint = hint;

    rci->error.description = description;
}

#if defined RCI_PARSER_USES_ERROR_DESCRIPTIONS
#define get_rci_global_error(rci, id)   rci->service_data->connector_ptr->rci_data->error_table[(id) - connector_fatal_protocol_error_FIRST]
static char const * get_rci_group_error(rci_t * const rci, unsigned int const id)
{
    connector_group_t const * const group = get_current_group(rci);
    unsigned int const index = id - (rci->service_data->connector_ptr->rci_data->global_error_count + 1);

    ASSERT(id >= rci->service_data->connector_ptr->rci_data->global_error_count + 1);
    ASSERT(index < group->errors.count);

    return group->errors.description[index];
}
#else
#define get_rci_global_error(rci, id)   ((void) (rci), (void) (id), NULL)
#define get_rci_group_error(rci, id)    ((void) (rci), (void) (id), NULL)
#endif

STATIC void rci_global_error(rci_t * const rci, unsigned int const id, char const * const hint)
{
    char const * const description = get_rci_global_error(rci, id);

    rci_error(rci, id, description, hint);
}

STATIC void rci_group_error(rci_t * const rci, unsigned int const id, char const * const hint)
{
    if (id < (rci->service_data->connector_ptr->rci_data->global_error_count + 1))
    {
        rci_global_error(rci, id, hint);
    }
    else
    {
        char const * const description = get_rci_group_error(rci, id);

        rci_error(rci, id, description, hint);
    }
}

STATIC connector_bool_t pending_rci_callback(rci_t * const rci)
{
    connector_bool_t const pending = connector_bool(rci->callback.status == connector_callback_busy);

    return pending;
}

STATIC void prepare_group_info(rci_t * const rci, connector_collection_type_t const collection_type)
{
    rci->shared.callback_data.group.collection_type = collection_type;
    ASSERT(have_group_id(rci));
#if (defined RCI_PARSER_USES_LIST)
    rci->shared.callback_data.list.depth = 0;
#endif
    rci->shared.callback_data.group.id = get_group_id(rci);
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
    rci->shared.callback_data.group.name = get_current_group(rci)->collection.name;
#endif
}

#if (defined RCI_PARSER_USES_LIST)
STATIC void prepare_list_info(rci_t * const rci, connector_collection_type_t const collection_type)
{
    ASSERT(have_group_id(rci));
    ASSERT(get_list_depth(rci) > 0 && get_list_depth(rci) <= RCI_LIST_MAX_DEPTH);

    rci->shared.callback_data.list.depth = get_list_depth(rci);
    {
        unsigned int const index = get_list_depth(rci) - 1;
        rci->shared.callback_data.list.level[index].collection_type = collection_type;
        rci->shared.callback_data.list.level[index].id = rci->shared.list.level[index].id;
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
        {
            connector_collection_t const * const list = get_current_collection_info(rci);
            rci->shared.callback_data.list.level[index].name = list->name;
        }
#endif
    }
}
#endif

STATIC void trigger_rci_callback(rci_t * const rci, connector_request_id_remote_config_t const remote_config_request)
{
    switch (remote_config_request)
    {
    case connector_request_id_remote_config_session_cancel:
        break;

    case connector_request_id_remote_config_session_start:
        rci->shared.callback_data.attribute.source = rci_query_setting_attribute_source_current;
        rci->shared.callback_data.attribute.compare_to = rci_query_setting_attribute_compare_to_none;
        rci->shared.callback_data.attribute.embed_transformed_values = connector_false;
#if (defined RCI_LEGACY_COMMANDS)
        rci->shared.callback_data.attribute.target = NULL;
#endif
        break;
    case connector_request_id_remote_config_session_end:
        break;
    case connector_request_id_remote_config_action_start:
        switch (rci->command.command_id)
        {
            case rci_command_query_setting:
            {
                unsigned int i;
                for (i=0; i < rci->shared.attribute_count; i++)
                {
                    switch (rci->command.attribute[i].id.query_setting)
                    {
                        case rci_query_setting_attribute_id_source:
                            rci->shared.callback_data.attribute.source = rci->command.attribute[i].value.enum_val;
                            break;
                        case rci_query_setting_attribute_id_compare_to:
                            rci->shared.callback_data.attribute.compare_to = rci->command.attribute[i].value.enum_val;
                            break;
                        case rci_query_setting_attribute_id_count:
                            ASSERT_GOTO(0, done);
                            break;
                    }
                }
                rci->shared.callback_data.element.value = NULL;
                rci->shared.callback_data.response.element_value = &rci->shared.value;
                break;
            }
            case rci_command_set_setting:
                /* Only one possible attribute for now so just check first attribute if found */
                if (rci->shared.attribute_count == 1 &&
                    rci->command.attribute[0].id.set_setting == rci_set_setting_attribute_id_embed_transformed_values) {
                    rci->shared.callback_data.attribute.embed_transformed_values = connector_bool(rci->command.attribute[0].value.enum_val == 1);
                }
                rci->shared.callback_data.element.value = &rci->shared.value;
                rci->shared.callback_data.response.element_value = &rci->shared.transformed_value;
                break;
#if (defined RCI_LEGACY_COMMANDS)
            case rci_command_do_command:
            {
                unsigned int i;
                for (i=0; i < rci->shared.attribute_count; i++)
                {
                    switch (rci->command.attribute[i].id.do_command)
                    {
                        case rci_do_command_attribute_id_target:
                            rci->shared.callback_data.attribute.target = rci->command.attribute[i].value.string_val;
                            break;
                        case rci_do_command_attribute_id_count:
                            ASSERT_GOTO(0, done);
                            break;
                    }
                }
                break;
            }
#endif
            case rci_command_query_state:
            case rci_command_set_state:
            case rci_command_query_descriptor:
#if (defined RCI_LEGACY_COMMANDS)
            case rci_command_reboot:
            case rci_command_set_factory_default:
#endif
                break;
        }
        break;
    case connector_request_id_remote_config_action_end:
        break;

#if (defined RCI_PARSER_USES_VARIABLE_DICT)
    case connector_request_id_remote_config_group_instance_remove:
        ASSERT(get_group_collection_type(rci) == connector_collection_type_variable_dictionary);
        ASSERT(rci->shared.group.info.instance == 0);
        rci->shared.callback_data.group.collection_type = connector_collection_type_variable_dictionary;
        rci->shared.callback_data.group.item.key = rci->shared.group.info.keys.key_store;
        break;
#endif

    case connector_request_id_remote_config_group_start:
    {
        connector_collection_type_t const collection_type = get_group_collection_type(rci);
        unsigned int const instance = get_group_instance(rci);

        switch (collection_type)
        {
            case connector_collection_type_fixed_array:
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
            case connector_collection_type_variable_array:
#endif
                rci->shared.callback_data.group.item.index = instance;
                break;
#if (defined RCI_PARSER_USES_DICT)
            case connector_collection_type_fixed_dictionary:
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
            case connector_collection_type_variable_dictionary:
#endif
                if (instance == 0)
                {
                    rci->shared.callback_data.group.item.key = rci->shared.group.info.keys.key_store;
                }
                else
                {
                    rci->shared.callback_data.group.item.key = rci->shared.group.info.keys.list[instance - 1];
                }
                break;
#endif
        }
        prepare_group_info(rci, collection_type);
        break;
    }

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
    case connector_request_id_remote_config_group_instances_lock:
    {
        connector_collection_type_t const collection_type = get_group_collection_type(rci);
        prepare_group_info(rci, collection_type);
        break;
    }

    case connector_request_id_remote_config_group_instances_unlock:
#if (defined RCI_PARSER_USES_LIST)
        rci->shared.callback_data.list.depth = 0;
#endif
        break;

    case connector_request_id_remote_config_group_instances_set:
    {
        connector_collection_type_t const collection_type = get_group_collection_type(rci);
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
        if (collection_type == connector_collection_type_variable_array)
        {
            rci->shared.callback_data.group.item.count = rci->shared.group.info.keys.count;
            break;
        }
#endif
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
        if (collection_type == connector_collection_type_variable_dictionary)
        {
            rci->shared.callback_data.group.item.dictionary.entries = rci->shared.group.info.keys.count;
            rci->shared.callback_data.group.item.dictionary.keys = rci->shared.group.info.keys.list;
            break;
        }
#endif
        ASSERT(0);
        break;
    }
#endif

    case connector_request_id_remote_config_group_end:
        ASSERT(have_group_id(rci));
        ASSERT(have_group_instance(rci));
#if (defined RCI_PARSER_USES_LIST)
        rci->shared.callback_data.list.depth = 0;
#endif
        break;

#if (defined RCI_PARSER_USES_LIST)
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
    case connector_request_id_remote_config_list_instance_remove:
    {
        unsigned int const index = get_list_depth(rci) - 1;
        ASSERT(get_current_collection_type(rci) == connector_collection_type_variable_dictionary);
        ASSERT(get_current_list_instance(rci) == 0);
        rci->shared.callback_data.list.level[index].collection_type = connector_collection_type_variable_dictionary;
        rci->shared.callback_data.list.level[index].item.key = rci->shared.list.level[index].info.keys.key_store;
        break;
    }
#endif

    case connector_request_id_remote_config_list_start:
    {
        unsigned int const index = get_list_depth(rci) - 1;
        connector_collection_type_t const collection_type = get_current_collection_type(rci);
        unsigned int const instance = rci->shared.list.level[index].info.instance;

        switch (collection_type)
        {
            case connector_collection_type_fixed_array:
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
            case connector_collection_type_variable_array:
#endif
                rci->shared.callback_data.list.level[index].item.index = instance;
                break;
#if (defined RCI_PARSER_USES_DICT)
            case connector_collection_type_fixed_dictionary:
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
            case connector_collection_type_variable_dictionary:
#endif
                if (instance ==  0)
                {
                    rci->shared.callback_data.list.level[index].item.key = rci->shared.list.level[index].info.keys.key_store;
                }
                else
                {
                    rci->shared.callback_data.list.level[index].item.key = rci->shared.list.level[index].info.keys.list[instance - 1];
                }
                break;
#endif
        }
        prepare_list_info(rci, collection_type);
        break;
    }

    case connector_request_id_remote_config_list_end:
        ASSERT(have_current_list_instance(rci));
        ASSERT(have_current_list_id(rci));
        rci->shared.callback_data.list.depth = get_list_depth(rci);
        break;

#if (defined RCI_PARSER_USES_VARIABLE_LIST)
    case connector_request_id_remote_config_list_instances_lock:
    {
        connector_collection_type_t const collection_type = get_current_collection_type(rci);
        prepare_list_info(rci, collection_type);
        break;
    }

    case connector_request_id_remote_config_list_instances_unlock:
        rci->shared.callback_data.list.depth = get_list_depth(rci);
        break;

    case connector_request_id_remote_config_list_instances_set:
    {
        unsigned int index = get_list_depth(rci) - 1;
        connector_collection_type_t const collection_type = get_current_collection_type(rci);
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
        if (collection_type == connector_collection_type_variable_array)
#endif
        {
            rci->shared.callback_data.list.level[index].item.count = rci->shared.list.level[index].info.keys.count;
            break;
        }
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
        if (collection_type == connector_collection_type_variable_dictionary)
        {
            rci->shared.callback_data.list.level[index].item.dictionary.entries = rci->shared.list.level[index].info.keys.count;
            rci->shared.callback_data.list.level[index].item.dictionary.keys = rci->shared.list.level[index].info.keys.list;
            break;
        }
#endif
        ASSERT(0);
        break;
    }
#endif

#endif
    case connector_request_id_remote_config_element_process:
        ASSERT(have_group_id(rci));
        ASSERT(have_group_instance(rci));
        ASSERT(have_element_id(rci));

#if (defined RCI_PARSER_USES_LIST)
        rci->shared.callback_data.list.depth = get_list_depth(rci);
#endif
        rci->shared.callback_data.element.id = get_element_id(rci);
        {
            connector_item_t const * const element = get_current_element(rci);

            rci->shared.callback_data.element.type = element->type;
#if (defined RCI_PARSER_USES_ELEMENT_NAMES)
            rci->shared.callback_data.element.name = element->data.element->name;
#endif
        }
        rci->shared.transformed_value.string_value = NULL;
        break;

#if (defined RCI_LEGACY_COMMANDS)
    case connector_request_id_remote_config_do_command:
        /* Provide request */
        rci->shared.callback_data.element.value = &rci->shared.value;
        /* Provide response pointer */
        rci->shared.callback_data.response.element_value = &rci->command.do_command.response_value;
        rci->shared.callback_data.response.element_value->string_value = NULL;
        goto done;
        break;

    case connector_request_id_remote_config_reboot:
    case connector_request_id_remote_config_set_factory_def:
        goto done;
        break;
#endif
    }

done:
    rci->callback.request.remote_config_request = remote_config_request;
    rci->callback.status = connector_callback_busy;
}

#if !(defined CONNECTOR_NO_MALLOC)
STATIC connector_status_t free_rci_internal_data(connector_data_t * const connector_ptr)
{
    connector_status_t status = connector_working;

    if (connector_ptr->rci_internal_data != NULL)
    {
        if (connector_ptr->rci_internal_data->input.storage != NULL)
        {
            status = free_data(connector_ptr, connector_ptr->rci_internal_data->input.storage);
            ASSERT_GOTO(status == connector_working, done);
        }
        status = free_data(connector_ptr, connector_ptr->rci_internal_data);
        ASSERT_GOTO(status == connector_working, done);

        connector_ptr->rci_internal_data = NULL;
    }

done:
    return status;
}
#endif

STATIC unsigned int check_instance(rci_t * const rci)
{
    connector_request_id_remote_config_t const remote_config_request = rci->callback.request.remote_config_request;
    rci_collection_info_t const * info;
    connector_collection_type_t collection_type;

    switch (remote_config_request)
    {
        case connector_request_id_remote_config_group_start:
#if (defined RCI_PARSER_USES_VARIABLE_GROUP) && (defined RCI_PARSER_USES_VARIABLE_DICT)
        case connector_request_id_remote_config_group_instance_remove:
#endif
            info = &rci->shared.group.info;
            collection_type = get_group_collection_type(rci);
            break;
#if (defined RCI_PARSER_USES_LIST)
        case connector_request_id_remote_config_list_start:
#if (defined RCI_PARSER_USES_VARIABLE_LIST) && (defined RCI_PARSER_USES_VARIABLE_DICT)
        case connector_request_id_remote_config_list_instance_remove:
#endif
            info = &rci->shared.list.level[rci->shared.callback_data.list.depth - 1].info;
            collection_type = get_current_collection_type(rci);
            break;
#endif
        default:
            return connector_success;
            break;
    }

    switch (collection_type)
    {
        case connector_collection_type_fixed_array:
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
        case connector_collection_type_variable_array:
#endif
            return (info->instance > info->keys.count) ? connector_protocol_error_invalid_index : connector_success;
            break;
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
        case connector_collection_type_variable_dictionary:
        {
#if (defined RCI_PARSER_USES_VARIABLE_GROUP) && (defined RCI_PARSER_USES_VARIABLE_LIST)
            connector_bool_t const is_remove_request = (remote_config_request == connector_request_id_remote_config_group_instance_remove) ||
                                                        (remote_config_request == connector_request_id_remote_config_list_instance_remove);
#elif (defined RCI_PARSER_USES_VARIABLE_GROUP)
            connector_bool_t const is_remove_request = (remote_config_request == connector_request_id_remote_config_group_instance_remove);
#elif (defined RCI_PARSER_USES_VARIABLE_LIST)
            connector_bool_t const is_remove_request = (remote_config_request == connector_request_id_remote_config_list_instance_remove);
#endif


            if (is_set_command(rci->shared.callback_data.action) && !is_remove_request)
            {
                return (info->keys.key_store[0] == '\0') ? connector_protocol_error_missing_name : connector_success;
            }
        }
        /* intentional fall-through */
        case connector_collection_type_fixed_dictionary:
            if (info->instance != 0)
            {
                return connector_success;
            }
            else if (info->keys.key_store[0] == '\0')
            {
                return connector_protocol_error_missing_name;
            }
            else
            {
                unsigned int i;
                for (i = 0; i < info->keys.count; i++)
                {
                    if (strcmp(info->keys.list[i], info->keys.key_store) == 0)
                    {
                        return connector_success;
                    }
                }
                return connector_protocol_error_invalid_name;
            }
            break;
#endif
    }

    return connector_success;
}

STATIC connector_bool_t rci_callback(rci_t * const rci)
{
    connector_bool_t callback_complete;
    connector_remote_config_t * const remote_config = &rci->shared.callback_data;
    connector_remote_config_cancel_t remote_cancel;
    void * callback_data = NULL;
    connector_request_id_remote_config_t const remote_config_request = rci->callback.request.remote_config_request;
    unsigned int const error = check_instance(rci);

    if (remote_config_request == connector_request_id_remote_config_session_cancel)
    {
        remote_cancel.user_context = remote_config->user_context;
        callback_data =  &remote_cancel;
    }
#if (defined RCI_LEGACY_COMMANDS)
    else if (remote_config_request == connector_request_id_remote_config_do_command ||
             remote_config_request == connector_request_id_remote_config_reboot ||
             remote_config_request == connector_request_id_remote_config_set_factory_def)
    {
        remote_config->error_id = connector_success;
        remote_config->response.compare_matches = connector_false;
        callback_data = remote_config;
    }
#endif
    else
    {

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)

#define group_start_is_valid(rci)   (group_is_dynamic(rci) == connector_false || RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_SKIP_COLLECTION) == connector_false)
#define group_callback_is_valid(request, rci)   ((request) == connector_request_id_remote_config_group_instances_lock || \
    ((request) == connector_request_id_remote_config_group_start && group_start_is_valid(rci)))

#else

#define group_callback_is_valid(request, rci)   ((request) == connector_request_id_remote_config_group_start)

#endif

#if (defined RCI_PARSER_USES_LIST)
#if (defined RCI_PARSER_USES_VARIABLE_LIST)

#define list_start_is_valid(rci)    (current_list_is_dynamic(rci) == connector_false || RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_SKIP_COLLECTION) == connector_false)
#define list_callback_is_valid(request, rci)    ((request) == connector_request_id_remote_config_list_instances_lock || \
    ((request) == connector_request_id_remote_config_list_start && list_start_is_valid(rci)))

#else

#define list_start_is_valid(rci)    (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_SKIP_COLLECTION) == connector_false)
#define list_callback_is_valid(request, rci)    ((request) == connector_request_id_remote_config_list_start && list_start_is_valid(rci))

#endif

#define list_callback_should_run(remote_config, request, rci)   ((remote_config)->list.depth == (rci)->output.skip_depth && list_callback_is_valid(request, rci))
#define is_valid_start_callback(remote_config, request, rci)    (group_callback_is_valid(request, rci) || list_callback_should_run(remote_config, request, rci))

#else

#define is_valid_start_callback(remote_config, request, rci)    (group_callback_is_valid(request, rci))

#endif

        if (remote_config_request == connector_request_id_remote_config_session_start)
        {
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
                rci->shared.callback_data.group.name = NULL;
#if (defined RCI_PARSER_USES_LIST)
                {
                    int i;
                    for (i = 0; i < RCI_LIST_MAX_DEPTH; i++)
                    {
                           rci->shared.callback_data.list.level[i].name = NULL;
                    }
                }
                /*
                 * should_run_callback() requires depth to be initialised before
                 * session/action start otherwise it does random things with
                 * uninitialised values.  Currently this does not get initialised until
                 * group/list/... start.  Fixing it here seems like a hack but the code
                 * above set the precendent, so just go with it for now,  otherwise we
                 * need a rework of a lot of logic in this file/function.  Not sure
                 * if INVALID_DEPTH or the higher level depth is more appropriate here.
                 * In this scenario I believe they will be the same.
                 */
                rci->shared.callback_data.list.depth = get_list_depth(rci);
#endif
#endif
#if (defined RCI_PARSER_USES_ELEMENT_NAMES)
                rci->shared.callback_data.element.name = NULL;
#endif
        }
        else if (is_valid_start_callback(remote_config, remote_config_request, rci))
        {
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_COLLECTION, connector_false);
            rci->output.skip_depth = INVALID_DEPTH;
        }

        rci->output.element_skip = connector_false;
        remote_config->error_id = connector_success;
        remote_config->response.compare_matches = connector_false;
        remote_config->response.error_hint = NULL;
        callback_data = remote_config;

#undef group_start_is_valid
#undef list_start_is_valid
#undef group_callback_is_valid
#undef list_callback_is_valid
#undef list_callback_should_run
#undef is_valid_start_callback
    }

    {
#define should_run_end_callback(rci)    (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_SKIP_COLLECTION) == connector_false && \
    RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_SKIP_CLOSE) == connector_false)

#if (defined RCI_PARSER_USES_LIST)

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
#define is_unlock_callback(request) ((request) == connector_request_id_remote_config_list_instances_unlock || \
    (request) == connector_request_id_remote_config_group_instances_unlock)
#else
#define is_unlock_callback(request) ((request) == connector_request_id_remote_config_list_instances_unlock)
#endif

#define is_end_callback(request)    ((request) == connector_request_id_remote_config_list_end || \
    (request) == connector_request_id_remote_config_group_end)
#define is_valid_callback(rci, request) ((should_run_end_callback(rci) && is_end_callback(request)) || is_unlock_callback(request))
#define should_run_callback(remote_config, rci, request)    ((remote_config)->list.depth < (rci)->output.skip_depth || \
    ((remote_config)->list.depth == (rci)->output.skip_depth && is_valid_callback(rci, request)))

#else

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
#define is_valid_callback(rci, request) ((should_run_end_callback(rci) && (request) == connector_request_id_remote_config_group_end) || (request) == connector_request_id_remote_config_group_instances_unlock)
#else
#define is_valid_callback(rci, request) (should_run_end_callback(rci) && (request) == connector_request_id_remote_config_group_end)
#endif
#define should_run_callback(remote_config, rci, request)    ((rci)->output.skip_depth !=0 || is_valid_callback(rci, request))

#endif

        if (error != connector_success)
        {
            if (remote_config_request == connector_request_id_remote_config_group_start
#if (defined RCI_PARSER_USES_LIST)
                || remote_config_request == connector_request_id_remote_config_list_start
#endif
                )
            {
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_CLOSE, connector_true);
                remote_config->error_id = error;
            }
            else if (error == connector_protocol_error_missing_name)
            {
                remote_config->error_id = connector_protocol_error_missing_name;
            }
            rci->callback.status = connector_callback_continue;
        }
        else if (should_run_callback(remote_config, rci, remote_config_request))
        {
            rci->callback.status = connector_callback(rci->service_data->connector_ptr->callback,
                                                      connector_class_id_remote_config,
                                                      rci->callback.request,
                                                      callback_data,
                                                      rci->service_data->connector_ptr->context);
        }
        else
        {
#if (defined RCI_PARSER_USES_LIST)
            if (remote_config->list.depth == rci->output.skip_depth &&
                (remote_config_request == connector_request_id_remote_config_list_end || remote_config_request == connector_request_id_remote_config_group_end))
#else
            if (rci->output.skip_depth == 0 && remote_config_request == connector_request_id_remote_config_group_end)
#endif
            {
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_CLOSE, connector_false);
            }
            rci->callback.status = connector_callback_continue;
        }
    }

    switch (remote_config_request)
    {
#if (defined RCI_LEGACY_COMMANDS)

        case connector_request_id_remote_config_reboot:
        {
            if (rci->callback.status == connector_callback_continue && remote_config->error_id == connector_success)
            {
                connector_request_id_t request_id;
                connector_data_t * const connector_ptr = rci->service_data->connector_ptr;

                request_id.os_request = connector_request_id_os_reboot;
                rci->callback.status = connector_callback(connector_ptr->callback, connector_class_id_operating_system, request_id, NULL, connector_ptr->context);
            }
        }
        FALL_THROUGH
        /* fall through */
#endif
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
        case connector_request_id_remote_config_group_instances_lock:
        case connector_request_id_remote_config_group_instances_set:
        case connector_request_id_remote_config_group_instances_unlock:
#endif
#if (defined RCI_PARSER_USES_VARIABLE_LIST)
        case connector_request_id_remote_config_list_instances_lock:
        case connector_request_id_remote_config_list_instances_set:
        case connector_request_id_remote_config_list_instances_unlock:
#endif
#if (defined RCI_LEGACY_COMMANDS)
        case connector_request_id_remote_config_do_command:
        case connector_request_id_remote_config_set_factory_def:
#endif
            if (remote_config->error_id != connector_success)
            {
                rci_global_error(rci, remote_config->error_id, remote_config->response.error_hint);
                set_rci_command_error(rci);
                state_call(rci, rci_parser_state_error);
            }
            break;
#if (defined RCI_PARSER_USES_VARIABLE_GROUP) && (defined RCI_PARSER_USES_VARIABLE_DICT)
        case connector_request_id_remote_config_group_instance_remove:
#endif
        case connector_request_id_remote_config_session_start:
        case connector_request_id_remote_config_session_end:
        case connector_request_id_remote_config_action_start:
        case connector_request_id_remote_config_action_end:
        case connector_request_id_remote_config_group_start:
        case connector_request_id_remote_config_element_process:
#if (defined RCI_PARSER_USES_LIST)
#if (defined RCI_PARSER_USES_VARIABLE_LIST) && (defined RCI_PARSER_USES_VARIABLE_DICT)
        case connector_request_id_remote_config_list_instance_remove:
#endif
        case connector_request_id_remote_config_list_start:
        case connector_request_id_remote_config_list_end:
#endif
        case connector_request_id_remote_config_group_end:
        case connector_request_id_remote_config_session_cancel:
            break;
    }

    switch (rci->callback.status)
    {
    case connector_callback_abort:
        callback_complete = connector_false;
        rci->status = rci_status_error;
        break;

    case connector_callback_continue:
        callback_complete = connector_true;

        if (remote_config->response.compare_matches)
        {
            ASSERT(rci->shared.callback_data.action == connector_remote_action_query);
            ASSERT(rci->shared.callback_data.group.type == connector_remote_group_setting);
            ASSERT(rci->shared.callback_data.attribute.compare_to != rci_query_setting_attribute_compare_to_none);

            switch (remote_config_request)
            {
                case connector_request_id_remote_config_element_process:
                    rci->output.element_skip = connector_true;
                    break;

                case connector_request_id_remote_config_group_start:
                    rci->output.skip_depth = 0;
                    break;

#if (defined RCI_PARSER_USES_LIST)
                case connector_request_id_remote_config_list_start:
                    rci->output.skip_depth = remote_config->list.depth;
                    break;
#endif
                default:
                    /* Invalid error_id for this callback */
                    ASSERT(0);
                    break;
            }
        }
        break;

    case connector_callback_busy:
        callback_complete = connector_false;
        break;

    default:
        callback_complete = connector_false;
        rci->status = rci_status_error;
        break;
    }

    if (callback_complete)
    {
        if (destination_in_storage(rci))
        {
            rci->input.destination = rci->buffer.input.current;
            reset_input_content(rci);
        }

        switch (remote_config_request)
        {
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
            case connector_request_id_remote_config_group_instances_set:
                if (remote_config->error_id != connector_success || rci->output.skip_depth == 0)
                {
                    rci->shared.group.info.keys.count = 0;
                }
                break;
            case connector_request_id_remote_config_group_instances_lock:
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
                if (remote_config->group.collection_type == connector_collection_type_variable_array)
                {
                    if (!should_set_count(rci) || remote_config->response.item.count == rci->shared.group.info.keys.count ||
                        (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_DONT_SHRINK) && remote_config->response.item.count > rci->shared.group.info.keys.count))
                    {
                        if (remote_config->error_id == connector_success && rci->output.skip_depth != 0)
                            rci->shared.group.info.keys.count = remote_config->response.item.count;
                        else
                            rci->shared.group.info.keys.count = 0;

                        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_false);
                    }
                    SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_DONT_SHRINK, connector_false);
                }
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
                else
#endif
#endif
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
                if (!should_set_count(rci))
                {
                    if (remote_config->error_id == connector_success && rci->output.skip_depth != 0)
                    {
                        rci->shared.group.info.keys.count = remote_config->response.item.dictionary.entries;
                        rci->shared.group.info.keys.list = remote_config->response.item.dictionary.keys;
                    }
                    else
                    {
                        rci->shared.group.info.keys.count = 0;
                        rci->shared.group.info.keys.list = NULL;
                    }
                }
#endif
                rci->shared.group.lock = get_group_id(rci);
                break;
            case connector_request_id_remote_config_group_instances_unlock:
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
                rci->shared.callback_data.group.name = NULL;
#endif
                rci->shared.group.lock = INVALID_ID;
                break;
#endif
            case connector_request_id_remote_config_group_end:
                break;
            case connector_request_id_remote_config_element_process:
#if (defined RCI_PARSER_USES_ELEMENT_NAMES)
                rci->shared.callback_data.element.name = NULL;
#endif
                break;
#if (defined RCI_PARSER_USES_LIST)
            case connector_request_id_remote_config_list_instances_set:
                if (remote_config->error_id != connector_success || remote_config->list.depth >= rci->output.skip_depth)
                {
                    set_current_list_count(rci, 0);
                }
                break;
#if (defined RCI_PARSER_USES_VARIABLE_LIST)
            case connector_request_id_remote_config_list_instances_lock:
            {
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
                if (remote_config->list.level[remote_config->list.depth - 1].collection_type == connector_collection_type_variable_array)
                {
                    if (!should_set_count(rci) || remote_config->response.item.count == get_current_list_count(rci) ||
                        (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_DONT_SHRINK) && remote_config->response.item.count > get_current_list_count(rci)))
                    {
                        if (remote_config->error_id == connector_success && remote_config->list.depth < rci->output.skip_depth)
                            set_current_list_count(rci, remote_config->response.item.count);
                        else
                            set_current_list_count(rci, 0);
                        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_false);
                    }
                    SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_DONT_SHRINK, connector_false);
                }
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
                else
#endif
#endif
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
                if (!should_set_count(rci))
                {
                    if (remote_config->error_id == connector_success && remote_config->list.depth < rci->output.skip_depth)
                    {
                        set_current_list_count(rci, remote_config->response.item.dictionary.entries);
                        set_current_list_key_list(rci, remote_config->response.item.dictionary.keys);
                    }
                    else
                    {
                        set_current_list_count(rci, 0);
                        set_current_list_key_list(rci, NULL);
                    }
                }
#endif
                set_current_list_lock(rci, get_current_list_id(rci));
                break;
            }
            case connector_request_id_remote_config_list_instances_unlock:
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
                rci->shared.callback_data.list.level[rci->shared.callback_data.list.depth - 1].name = NULL;
#endif
                invalidate_list_lock(rci, rci->shared.callback_data.list.depth - 1);
                break;
#endif
            case connector_request_id_remote_config_list_start:
            case connector_request_id_remote_config_list_end:
#if (defined RCI_PARSER_USES_VARIABLE_LIST) && (defined RCI_PARSER_USES_VARIABLE_DICT)
            case connector_request_id_remote_config_list_instance_remove:
#endif
#endif
            case connector_request_id_remote_config_session_start:
            case connector_request_id_remote_config_session_end:
            case connector_request_id_remote_config_action_start:
            case connector_request_id_remote_config_action_end:
            case connector_request_id_remote_config_group_start:
            case connector_request_id_remote_config_session_cancel:
#if (defined RCI_PARSER_USES_VARIABLE_GROUP) && (defined RCI_PARSER_USES_VARIABLE_DICT)
            case connector_request_id_remote_config_group_instance_remove:
#endif
#if (defined RCI_LEGACY_COMMANDS)
            case connector_request_id_remote_config_do_command:
            case connector_request_id_remote_config_reboot:
            case connector_request_id_remote_config_set_factory_def:
#endif
                break;
        }
    }

    return callback_complete;
}
