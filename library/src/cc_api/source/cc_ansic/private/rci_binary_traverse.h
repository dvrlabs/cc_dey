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

STATIC void traverse_rci_command(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_action_start);

    set_rci_output_state(rci, rci_output_state_command_id);
    state_call(rci, rci_parser_state_output);
}

STATIC void traverse_group_count(rci_t * const rci)
{
    connector_collection_type_t collection_type = get_group_collection_type(rci);

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
    if (is_variable(collection_type) == connector_true)
    {
        if (!have_group_lock(rci))
        {
            if (collection_type == connector_collection_type_variable_array)
            {
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_OUTPUT_COUNT, connector_true);
            }
            trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_lock);
            goto done;
        }
        if (get_group_id(rci) != get_group_lock(rci))
        {
            trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_unlock);
            goto done;
        }
        if (should_set_count(rci))
        {
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_OUTPUT_COUNT, connector_true);
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_false);
            trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_set);
            goto done;
        }
        if (should_remove_instance(rci))
        {
            trigger_rci_callback(rci, connector_request_id_remote_config_group_instance_remove);
            set_rci_traverse_state(rci, rci_traverse_state_none);
            set_rci_output_state(rci, rci_output_state_group_id);
            state_call(rci, rci_parser_state_output);
            goto done;
        }
    }
    else
    {
        if (have_group_lock(rci))
        {
            trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_unlock);
            goto done;
        }
#endif
        {
            connector_group_t const * const group = get_current_group(rci);

            if (collection_type == connector_collection_type_fixed_array)
            {
                rci->shared.group.info.keys.count = group->collection.capacity.instances;
            }
#if (defined RCI_PARSER_USES_DICT)
            else
            {
                rci->shared.group.info.keys.count = group->collection.capacity.dictionary.entries;
                rci->shared.group.info.keys.list = group->collection.capacity.dictionary.keys;
            }
#endif
        }
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
    }
#endif

    if (should_traverse_all_group_instances(rci))
    {
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
        if (collection_type == connector_collection_type_variable_dictionary)
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_OUTPUT_COUNT, connector_true);
#endif

        if (get_group_count(rci) == 0)
        {
            if (should_traverse_all_groups(rci))
            {
                set_rci_traverse_state(rci, rci_traverse_state_all_groups);
            }
            else
            {
                set_rci_traverse_state(rci, rci_traverse_state_none);
            }
            set_rci_output_state(rci, rci_output_state_group_id);
            state_call(rci, rci_parser_state_output);
        }
        else
        {
            set_rci_traverse_state(rci, rci_traverse_state_all_group_instances);
        }
    }
    else
    {
        set_rci_traverse_state(rci, rci_traverse_state_none);
    }

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
done:
#endif
    return;
}

STATIC void traverse_group_id(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_group_start);

    set_rci_output_state(rci, rci_output_state_group_id);
    state_call(rci, rci_parser_state_output);
}

#if (defined RCI_PARSER_USES_LIST)
STATIC connector_bool_t finish_all_list_instances(rci_t * const rci)
{
    set_element_id(rci, get_current_list_id(rci));
    if (get_list_depth(rci) > get_query_depth(rci))
    {
        invalidate_current_list_id(rci);
        invalidate_current_list_instance(rci);
        decrement_list_depth(rci);
        set_rci_traverse_state(rci, rci_traverse_state_all_elements);
    }
    else
    {
        set_should_traverse_all_list_instances(rci, connector_false);
        return connector_true;
    }
    return connector_false;
}

STATIC void traverse_list_id(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_list_start);
    set_rci_output_state(rci, rci_output_state_list_id);
    state_call(rci, rci_parser_state_output);
}

STATIC void traverse_list_count(rci_t * const rci)
{
    if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_FIRST_ELEMENT))
    {
        set_element_id(rci, get_current_list_id(rci));
        decrement_list_depth(rci);
        if (get_list_depth(rci) == 0)
            traverse_group_id(rci);
        else
            traverse_list_id(rci);

        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_FIRST_ELEMENT, connector_false);
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_RESTORE_DEPTH, connector_true);
    }
    else
    {
        connector_collection_type_t collection_type;

        if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_RESTORE_DEPTH))
        {
            increment_list_depth(rci);
            invalidate_element_id(rci);
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_RESTORE_DEPTH, connector_false);
        }

        collection_type = get_current_collection_type(rci);

#if (defined RCI_PARSER_USES_VARIABLE_LIST)
        if (is_variable(collection_type) == connector_true)
        {
            if (!have_current_list_lock(rci))
            {
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
                if (collection_type == connector_collection_type_variable_array)
                {
                    SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_OUTPUT_COUNT, connector_true);
                }
#endif
                trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_lock);
                goto done;
            }
            if (get_current_list_id(rci) != get_current_list_lock(rci))
            {
                trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_unlock);
                goto done;
            }
            if (should_set_count(rci))
            {
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_OUTPUT_COUNT, connector_true);
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_false);
                trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_set);
                goto done;
            }
            if (should_remove_instance(rci))
            {
                trigger_rci_callback(rci, connector_request_id_remote_config_list_instance_remove);
                set_rci_traverse_state(rci, rci_traverse_state_none);
                set_rci_output_state(rci, rci_output_state_list_id);
                state_call(rci, rci_parser_state_output);
                goto done;
            }
        }
        else
        {
            if (have_current_list_lock(rci))
            {
                trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_unlock);
                goto done;
            }
#endif
            {
                connector_collection_t const * const info = get_current_collection_info(rci);

                if (collection_type == connector_collection_type_fixed_array)
                {
                    set_current_list_count(rci, info->capacity.instances);
                }
#if (defined RCI_PARSER_USES_DICT)
                else
                {
                    set_current_list_count(rci, info->capacity.dictionary.entries);
                    set_current_list_key_list(rci, info->capacity.dictionary.keys);
                }
#endif
            }
#if (defined RCI_PARSER_USES_VARIABLE_LIST)
        }
#endif

        if (should_traverse_all_list_instances(rci) || get_list_depth(rci) > get_query_depth(rci))
        {
            if (collection_type == connector_collection_type_variable_dictionary)
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_OUTPUT_COUNT, connector_true);

            if (get_current_list_count(rci) == 0)
            {
                set_rci_traverse_state(rci, rci_traverse_state_list_instances_done);
                set_rci_output_state(rci, rci_output_state_list_id);
                   state_call(rci, rci_parser_state_output);
            }
            else
            {
                set_rci_traverse_state(rci, rci_traverse_state_all_list_instances);
            }
        }
        else
        {
            set_rci_traverse_state(rci, rci_traverse_state_none);
        }
    }

#if (defined RCI_PARSER_USES_VARIABLE_LIST)
done:
#endif
    return;
}

STATIC connector_bool_t release_last_list_lock(rci_t * const rci)
{
    connector_bool_t need_unlock = connector_false;

    if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_RESTORE_DEPTH))
    {
        decrement_list_depth(rci);
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_RESTORE_DEPTH, connector_false);
        goto done;
    }

#if (defined RCI_PARSER_USES_VARIABLE_LIST)
    if (get_list_depth(rci) == RCI_LIST_MAX_DEPTH) goto done;

    increment_list_depth(rci);
    if (have_current_list_lock(rci))
    {
        trigger_rci_callback(rci, connector_request_id_remote_config_list_instances_unlock);
        need_unlock = connector_true;
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_RESTORE_DEPTH, connector_true);
    }
    else
    {
        decrement_list_depth(rci);
    }
#endif

done:
    return need_unlock;
}
#endif

STATIC connector_bool_t traverse_element_id(rci_t * const rci)
{
    connector_bool_t done = connector_false;
    connector_item_t const * element;

    if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_FIRST_ELEMENT))
    {
#if (defined RCI_PARSER_USES_LIST)
        if (get_list_depth(rci) == 0)
#endif
            traverse_group_id(rci);
#if (defined RCI_PARSER_USES_LIST)
        else
            traverse_list_id(rci);
#endif

        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_FIRST_ELEMENT, connector_false);
        goto done;
    }

#if (defined RCI_PARSER_USES_LIST)
    if (release_last_list_lock(rci) == connector_true) goto done;
#endif

    element = get_current_element(rci);

#if (defined RCI_PARSER_USES_LIST)
    ASSERT(element->type != connector_element_type_list);
#endif

    if (should_traverse_all_elements(rci)
#if (defined RCI_PARSER_USES_LIST)
        || get_list_depth(rci) > get_query_depth(rci)
#endif
        )
    {
        set_rci_traverse_state(rci, rci_traverse_state_all_elements);
    }
    else
    {
        done = connector_true;
    }

    if ((rci->shared.callback_data.action == connector_remote_action_query) &&
        (element->data.element->access == connector_element_access_write_only))
    {
        goto done;
    }

    trigger_rci_callback(rci, connector_request_id_remote_config_element_process);
    set_rci_output_state(rci, rci_output_state_field_id);
    state_call(rci, rci_parser_state_output);

done:
    return done;
}

#if (defined RCI_LEGACY_COMMANDS)
STATIC void traverse_command_do_command(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_do_command);

    set_rci_output_state(rci, rci_output_state_do_command_payload);
    state_call(rci, rci_parser_state_output);

    return;
}

STATIC void traverse_command_reboot(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_reboot);

    state_call(rci, rci_parser_state_input);

    return;
}

STATIC void traverse_command_set_factory_default(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_set_factory_def);

    state_call(rci, rci_parser_state_input);

    return;
}
#endif

STATIC connector_bool_t finish_all_elements(rci_t * const rci)
{
    connector_bool_t done = connector_false;
#if (defined RCI_PARSER_USES_LIST)
    if (release_last_list_lock(rci) == connector_true) goto done;
#endif

    if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_FIRST_ELEMENT)) {
#if (defined RCI_PARSER_USES_LIST)
        if (get_list_depth(rci) > 0)
        {
            traverse_list_id(rci);
        }
        else
#endif
        {
            traverse_group_id(rci);
        }
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_FIRST_ELEMENT, connector_false);
        goto done;
    }

#if (defined RCI_PARSER_USES_LIST)
    if (get_list_depth(rci) > 0)
    {
        trigger_rci_callback(rci, connector_request_id_remote_config_list_end);
        set_element_id(rci, get_current_list_id(rci));
        /* traverse_all_list_instances() needs the list info so don't invalidate it if we will be transitioning to that state */
        if (get_list_depth(rci) < get_query_depth(rci) || (get_list_depth(rci) == get_query_depth(rci) && !should_traverse_all_list_instances(rci)))
        {
            invalidate_current_list_id(rci);
            invalidate_current_list_instance(rci);
            decrement_list_depth(rci);
        }
    }
    else
#endif
    {
        trigger_rci_callback(rci, connector_request_id_remote_config_group_end);
        invalidate_element_id(rci);
    }

#if (defined RCI_PARSER_USES_LIST)
    if (get_list_depth(rci) == 0)
    {
#endif
        if (should_traverse_all_group_instances(rci))
        {
            set_rci_traverse_state(rci, rci_traverse_state_all_group_instances);
        }
        else
        {
            done = connector_true;
        }
#if (defined RCI_PARSER_USES_LIST)
    }
    else if (get_list_depth(rci) >= get_query_depth(rci))
    {
        if (should_traverse_all_list_instances(rci) || get_list_depth(rci) > get_query_depth(rci))
            set_rci_traverse_state(rci, rci_traverse_state_all_list_instances);
        else
            done = connector_true;
    }
    else
    {
        done = connector_true;
    }
#endif

done:
    return done;
}

STATIC void traverse_element_end(rci_t * const rci)
{
    set_rci_traverse_state(rci, rci_traverse_state_elements_done);
    set_rci_output_state(rci, rci_output_state_field_terminator);
    state_call(rci, rci_parser_state_output);

    return;
}

STATIC connector_bool_t traverse_group_end(rci_t * const rci)
{
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
    if (have_group_lock(rci))
    {
        trigger_rci_callback(rci, connector_request_id_remote_config_group_instances_unlock);
        return connector_false;
    }
    else
#endif
    {
        trigger_rci_callback(rci, connector_request_id_remote_config_action_end);
        set_rci_output_state(rci, rci_output_state_group_terminator);
        state_call(rci, rci_parser_state_output);
        invalidate_group_id(rci);
        invalidate_group_instance(rci);
        return connector_true;
    }
}

#if (defined RCI_PARSER_USES_LIST)
STATIC connector_bool_t traverse_all_list_instances(rci_t * const rci)
{
    connector_bool_t done = connector_false;

    if (!have_current_list_instance(rci))
    {
        set_current_list_instance(rci, 1);
    }
    else
    {
        increment_current_list_instance(rci);
    }

    if (get_current_list_instance(rci) > get_current_list_count(rci))
    {
        done = finish_all_list_instances(rci);
        goto done;
    }

    invalidate_element_id(rci);
    set_rci_traverse_state(rci, rci_traverse_state_all_elements);

done:
    return done;
}
#endif

STATIC void traverse_element(rci_t * const rci)
{
#if (defined RCI_PARSER_USES_LIST)
    connector_item_t const * const element = get_current_element(rci);
    if (element->type == connector_element_type_list)
    {
        increment_list_depth(rci);
        set_current_list_id(rci, get_element_id(rci));
        invalidate_current_list_instance(rci);
        invalidate_element_id(rci);
        set_rci_traverse_state(rci, rci_traverse_state_list_count);
    }
    else
#endif
    {
        set_rci_traverse_state(rci, rci_traverse_state_element_id);
    }
}

STATIC void traverse_all_elements(rci_t * const rci)
{
    if (!have_element_id(rci))
    {
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_FIRST_ELEMENT, connector_true);
        set_element_id(rci, 0);
    }
    else
    {
        connector_collection_t const * const info = get_current_collection_info(rci);
        unsigned int const id = get_element_id(rci) + 1;

        if (id < info->item.count)
        {
            set_element_id(rci, id);
        }
        else
        {
#if (defined RCI_PARSER_USES_LIST)
            if (get_list_depth(rci) == get_query_depth(rci))
#endif
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_ALL_ELEMENTS, connector_false);

            invalidate_element_id(rci);
            set_rci_traverse_state(rci, rci_traverse_state_element_end);

            goto done;
        }
    }

    traverse_element(rci);

done:
    return;
}

STATIC connector_bool_t traverse_all_group_instances(rci_t * const rci)
{
    connector_bool_t done = connector_false;

    if (!have_group_instance(rci))
    {
        set_group_instance(rci, 1);
    }
    else
    {
        /* next instances */
        increment_group_instance(rci);
    }

    if (get_group_instance(rci) > rci->shared.group.info.keys.count)
    {
        if (should_traverse_all_groups(rci))
        {
            set_rci_traverse_state(rci, rci_traverse_state_all_groups);
        }
        else
        {
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_ALL_GROUP_INSTANCES, connector_false);
            done = connector_true;
        }
        goto done;
    }

    invalidate_element_id(rci);
    set_rci_traverse_state(rci, rci_traverse_state_all_elements);

done:
    return done;
}

STATIC void traverse_all_groups(rci_t * const rci)
{
    connector_remote_config_data_t const * const rci_data = rci->service_data->connector_ptr->rci_data;
    connector_remote_group_table_t const * const table = rci_data->group_table + rci->shared.callback_data.group.type;

    if (table->count == 0)
    {
        set_rci_traverse_state(rci, rci_traverse_state_group_end);
        goto done;
    }
    else if (!have_group_id(rci))
    {
        set_group_id(rci, 0);
    }
    else
    {
        unsigned int const id = get_group_id(rci) + 1;
        if (id == table->count)
        {
            /* done all groups */
            set_rci_traverse_state(rci, rci_traverse_state_group_end);
            goto done;
        }
        else
        {
            /* next group */
            increment_group_id(rci);
        }
    }

    invalidate_group_instance(rci);
    set_rci_traverse_state(rci, rci_traverse_state_group_count);

done:
    return;
}

STATIC void rci_traverse_data(rci_t * const rci)
{
    connector_bool_t done_state = connector_true;

#if (defined RCI_DEBUG)
    connector_debug_line("traverse: %s", rci_traverse_state_t_as_string(rci->traverse.state));
#if (defined RCI_PARSER_USES_LIST)
    connector_debug_line("flags: all_groups: %u, all_group_instances: %u, all_list_instances: %u, all_elements: %u, list depth: %u",
                         should_traverse_all_groups(rci), should_traverse_all_group_instances(rci), should_traverse_all_list_instances(rci),
                         should_traverse_all_elements(rci), get_list_depth(rci));
#else
    connector_debug_line("flags: all_groups: %u, all_group_instances: %u", should_traverse_all_groups(rci), should_traverse_all_group_instances(rci));
#endif
#endif

    switch (rci->traverse.state)
    {
        case rci_traverse_state_none:
            if (get_rci_input_state(rci) == rci_input_state_done)
                state_call(rci, rci_parser_state_output);
            else
                state_call(rci, rci_parser_state_input);
            break;

        case rci_traverse_state_command_id:
            traverse_rci_command(rci);
            break;

        case rci_traverse_state_group_count:
            traverse_group_count(rci);
            done_state = connector_false;
            break;

        case rci_traverse_state_all_groups:
            traverse_all_groups(rci);
            done_state = connector_false;
            break;

        case rci_traverse_state_group_id:
            traverse_group_id(rci);
            break;

        case rci_traverse_state_all_group_instances:
            done_state = traverse_all_group_instances(rci);
            break;

#if (defined RCI_PARSER_USES_LIST)
        case rci_traverse_state_list_count:
            traverse_list_count(rci);
            done_state = connector_false;
            break;

        case rci_traverse_state_list_id:
            traverse_list_id(rci);
            break;

        case rci_traverse_state_all_list_instances:
            done_state = traverse_all_list_instances(rci);
            break;

        case rci_traverse_state_list_instances_done:
            done_state = finish_all_list_instances(rci);
            break;
#endif

        case rci_traverse_state_element_id:
            done_state = traverse_element_id(rci);
            break;

        case rci_traverse_state_all_elements:
            traverse_all_elements(rci);
            done_state = connector_false;
            break;

        case rci_traverse_state_element_end:
            traverse_element_end(rci);
            done_state = connector_false;
            break;

        case rci_traverse_state_elements_done:
            done_state = finish_all_elements(rci);
            break;

        case rci_traverse_state_group_end:
            done_state = traverse_group_end(rci);
            break;
#if (defined RCI_LEGACY_COMMANDS)
        case rci_traverse_state_command_do_command:
            traverse_command_do_command(rci);
            break;
        case rci_traverse_state_command_reboot:
            traverse_command_reboot(rci);
            break;
        case rci_traverse_state_command_set_factory_default:
            traverse_command_set_factory_default(rci);
            break;
#endif
    }

    if (done_state)
    {
        set_rci_traverse_state(rci, rci_traverse_state_none);
    }

    return;
}
