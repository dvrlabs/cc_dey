/*
 * Copyright (c) 2014 Digi International Inc.
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

STATIC void rci_output_error_id(rci_t * const rci)
{

    connector_remote_config_t const * const remote_config = &rci->shared.callback_data;
    uint32_t value;

    if (rci->error.command_error)
    {
        #define BINARY_RCI_COMMAND_LOWER_ERROR_ID_MASK  UINT32_C(0x7FF)
        #define BINARY_RCI_COMMAND_UPPER_ERROR_ID_MASK   (~UINT32_C(0x1FFF))
        /* Command Error ID is [:13][10:0] */
        value = (remote_config->error_id & BINARY_RCI_COMMAND_LOWER_ERROR_ID_MASK);
        value |= ((remote_config->error_id << UINT32_C(2)) & BINARY_RCI_COMMAND_UPPER_ERROR_ID_MASK);
    }
    else
    {
        /* Error Id is [:13][10:7][5:0] */
        #define BINARY_RCI_LOWER_ERROR_ID_MASK UINT32_C(0x3F)
        #define BINARY_RCI_MIDDLE_ERROR_ID_MASK UINT32_C(0x780)
        #define BINARY_RCI_UPPER_ERROR_ID_MASK  ()~UINT32_C(0x1FFF))

        value = remote_config->error_id & BINARY_RCI_LOWER_ERROR_ID_MASK;
        value |= (remote_config->error_id << 1) & BINARY_RCI_MIDDLE_ERROR_ID_MASK;
        value |= (remote_config->error_id << 3) & BINARY_RCI_MIDDLE_ERROR_ID_MASK;

    }

    value |= BINARY_RCI_ERROR_INDICATOR_BIT;

    {
        connector_bool_t const overflow = rci_output_uint32(rci, value);

        if (!overflow)
            set_rci_error_state(rci, rci_error_state_description);
    }
}

STATIC void rci_output_error_description(rci_t * const rci)
{
#if defined RCI_PARSER_USES_ERROR_DESCRIPTIONS
    char const * const description = rci->error.description+1;
    size_t const length = (size_t)*rci->error.description;
#else
    char * description = NULL;
    size_t const length = 0;
#endif

    connector_bool_t const overflow = rci_output_string(rci, description,  length);

    if (!overflow)
        set_rci_error_state(rci, rci_error_state_hint);
}

STATIC void rci_output_error_hint(rci_t * const rci)
{
    connector_remote_config_t const * const remote_config = &rci->shared.callback_data;
    size_t const description_length = (remote_config->response.error_hint == NULL) ? 0 : strlen(remote_config->response.error_hint);
    connector_bool_t const overflow = rci_output_string(rci, remote_config->response.error_hint,  description_length);

    if (!overflow)
    {
        set_rci_error_state(rci, rci_error_state_callback);
    }
}

#define is_fatal_protocol_error(err)    ((err) < connector_protocol_error_FIRST)

STATIC void rci_generate_error(rci_t * const rci)
{
    rci_buffer_t * const output = &rci->buffer.output;

    if (pending_rci_callback(rci))
    {
        if (!rci_callback(rci))
            goto done;
    }

    if (rci_buffer_remaining(output) != 0)
    {
        switch (rci->error.state)
        {
            case rci_error_state_id:
                rci_output_error_id(rci);
                break;

            case rci_error_state_description:
                rci_output_error_description(rci);
                break;

            case rci_error_state_hint:
                rci_output_error_hint(rci);
                break;

            case rci_error_state_callback:
            {
                connector_request_id_remote_config_t const remote_config_request = rci->callback.request.remote_config_request;
                connector_remote_config_t * const remote_config = &rci->shared.callback_data;

                switch (remote_config_request)
                {
                    case connector_request_id_remote_config_session_start:
                        trigger_rci_callback(rci, connector_request_id_remote_config_session_end);
                        break;
                    case connector_request_id_remote_config_action_start:
                        trigger_rci_callback(rci, connector_request_id_remote_config_action_end);
                        break;
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
                    case connector_request_id_remote_config_group_instances_set:
                    case connector_request_id_remote_config_group_instances_lock:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_unlock);
                        }
                        else
                        {
                            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_COLLECTION, connector_true);
                            rci->output.skip_depth = 0;
                            set_rci_error_state(rci, rci_error_state_id);
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
                    case connector_request_id_remote_config_group_instance_remove:
#endif
#endif
                    case connector_request_id_remote_config_group_start:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            if (remote_config_request == connector_request_id_remote_config_group_start)
                                trigger_rci_callback(rci, connector_request_id_remote_config_group_end);
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
                            else
                                trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_unlock);
#endif
                        }
                        else
                        {
                            connector_bool_t const overflow = rci_output_terminator(rci);
                            if (overflow) goto done;

                            rci->output.skip_depth = 0;
                            set_rci_error_state(rci, rci_error_state_id);
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
                    case connector_request_id_remote_config_element_process:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
#if (defined RCI_PARSER_USES_LIST)
                            if (get_list_depth(rci) > 0)
                                trigger_rci_callback(rci, connector_request_id_remote_config_list_end);
                            else
#endif
                                trigger_rci_callback(rci, connector_request_id_remote_config_group_end);
                        }
                        else
                        {
                            set_rci_error_state(rci, rci_error_state_id);
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
#if (defined RCI_LEGACY_COMMANDS)
                    case connector_request_id_remote_config_do_command:
                    case connector_request_id_remote_config_reboot:
                    case connector_request_id_remote_config_set_factory_def:
#endif
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            trigger_rci_callback(rci, connector_request_id_remote_config_action_end);
                        }
                        else
                        {
                            set_rci_error_state(rci, rci_error_state_id);
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
                    case connector_request_id_remote_config_group_instances_unlock:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            trigger_rci_callback(rci, connector_request_id_remote_config_action_end);
                        }
                        else
                        {
                            set_rci_error_state(rci, rci_error_state_id);
                            remote_config->error_id = 0;
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
#endif
                    case connector_request_id_remote_config_group_end:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
                            connector_collection_type_t const collection_type = get_group_collection_type(rci);
#endif
                            connector_bool_t const overflow = rci_output_terminator(rci);
                            if (overflow) goto done;
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY) && (defined RCI_PARSER_USES_VARIBLE_DICT)
                            if (collection_type == connector_collection_type_variable_array || collection_type == connector_collection_type_variable_dictionary)
#elif (defined RCI_PARSER_USES_VARIABLE_ARRAY)
                            if (collection_type == connector_collection_type_variable_array)
#else
                            if (collection_type == connector_collection_type_variable_variable_dictionary)
#endif
                                trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_unlock);
                            else
#endif
                                trigger_rci_callback(rci, connector_request_id_remote_config_action_end);
                        }
                        else
                        {
                            set_rci_error_state(rci, rci_error_state_id);
                            remote_config->error_id = 0;
                            state_call(rci, rci_parser_state_output);
                        }
                        break;
                    case connector_request_id_remote_config_action_end:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            connector_bool_t const overflow = rci_output_terminator(rci);
                            if (overflow) goto done;

                            trigger_rci_callback(rci, connector_request_id_remote_config_session_end);
                        }
                        else
                        {
                            set_rci_error_state(rci, rci_error_state_id);
                            remote_config->error_id = 0;
                            state_call(rci, rci_parser_state_output);
                        }
                        break;
                    case connector_request_id_remote_config_session_cancel:
                        ASSERT(connector_false);
                        break;
                    case connector_request_id_remote_config_session_end:
                        rci->status = rci_status_complete;
                        break;
#if (defined RCI_PARSER_USES_LIST)
#if (defined RCI_PARSER_USES_VARIABLE_LIST)
                    case connector_request_id_remote_config_list_instances_set:
                    case connector_request_id_remote_config_list_instances_lock:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_unlock);
                        }
                        else
                        {
                            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_COLLECTION, connector_true);
                            rci->output.skip_depth = get_list_depth(rci);
                            set_rci_error_state(rci, rci_error_state_id);
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
                    case connector_request_id_remote_config_list_instances_unlock:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            decrement_list_depth(rci);
                            if (get_list_depth(rci) > 0)
                                trigger_rci_callback(rci, connector_request_id_remote_config_list_end);
                            else
                                trigger_rci_callback(rci, connector_request_id_remote_config_group_end);
                        }
                        else
                        {
                            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_COLLECTION, connector_false);
                            set_rci_error_state(rci, rci_error_state_id);
                            remote_config->error_id = 0;
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
#endif
                    case connector_request_id_remote_config_list_end:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            connector_collection_type_t const collection_type = get_current_collection_type(rci);
                            connector_bool_t const overflow = rci_output_terminator(rci);
                            if (overflow) goto done;

                            if (collection_type == connector_collection_type_variable_array || collection_type == connector_collection_type_variable_dictionary)
                            {
                                trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_unlock);
                            }
                            else
                            {
                                decrement_list_depth(rci);
                                if (get_list_depth(rci) > 0)
                                    trigger_rci_callback(rci, connector_request_id_remote_config_list_end);
                                else
                                    trigger_rci_callback(rci, connector_request_id_remote_config_group_end);
                            }
                        }
                        else
                        {
                            set_rci_error_state(rci, rci_error_state_id);
                            remote_config->error_id = 0;
                            state_call(rci, rci_parser_state_output);
                        }
                        break;
#if (defined RCI_PARSER_USES_VARIABLE_LIST) && (defined RCI_PARSER_USES_VARIABLE_DICT)
                    case connector_request_id_remote_config_list_instance_remove:
#endif
                    case connector_request_id_remote_config_list_start:
                        if (is_fatal_protocol_error(remote_config->error_id))
                        {
                            if (remote_config_request == connector_request_id_remote_config_list_start)
                                trigger_rci_callback(rci, connector_request_id_remote_config_list_end);
                            else
                                trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_unlock);
                        }
                        else
                        {
                            rci->output.skip_depth = get_list_depth(rci);
                            set_rci_error_state(rci, rci_error_state_id);
                            state_call(rci, rci_parser_state_traverse);
                        }
                        break;
#endif
                }
                break;
            }
        }
    }

#if defined RCI_DEBUG
    {
        size_t const bytes = rci_buffer_used(&rci->buffer.output);
        if (bytes > 0)
        {
            connector_debug_print_buffer("Response", rci->buffer.output.start, bytes);
        }
    }
#endif

done:
    return;
}
