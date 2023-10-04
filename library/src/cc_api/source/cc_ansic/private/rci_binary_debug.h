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

#if !(defined RCI_DEBUG)

#define rci_input_state_t_as_string(value)      NULL
#define rci_output_state_t_as_string(value)     NULL
#define rci_traverse_state_t_as_string(value)   NULL

#else

#define enum_to_case(name)  case name: result = #name; break

static char const * rci_input_state_t_as_string(rci_input_state_t const value)
{
    char const * result=NULL;
    switch (value)
    {
        enum_to_case(rci_input_state_command_id);
        enum_to_case(rci_input_state_command_attribute);
        enum_to_case(rci_input_state_command_normal_attribute_id);
        enum_to_case(rci_input_state_command_normal_attribute_value);
        enum_to_case(rci_input_state_group_id);
        enum_to_case(rci_input_state_group_attribute);
        enum_to_case(rci_input_state_field_id);
        enum_to_case(rci_input_state_group_normal_attribute_id);
        enum_to_case(rci_input_state_group_normal_attribute_value);
#if (defined RCI_PARSER_USES_LIST)
        enum_to_case(rci_input_state_list_normal_attribute_id);
        enum_to_case(rci_input_state_list_normal_attribute_value);
        enum_to_case(rci_input_state_list_attribute);
#endif
        enum_to_case(rci_input_state_field_type);
        enum_to_case(rci_input_state_field_no_value);
        enum_to_case(rci_input_state_field_value);
#if (defined RCI_LEGACY_COMMANDS)
        enum_to_case(rci_input_state_do_command_payload);
#endif
        enum_to_case(rci_input_state_done);
    }
    return result;
}

static char const * rci_output_state_t_as_string(rci_output_state_t const value)
{
    char const * result=NULL;
    switch (value)
    {
        enum_to_case(rci_output_state_command_id);
        enum_to_case(rci_output_state_command_normal_attribute_count);
        enum_to_case(rci_output_state_command_normal_attribute_id);
        enum_to_case(rci_output_state_command_normal_attribute_value);
        enum_to_case(rci_output_state_group_id);
        enum_to_case(rci_output_state_collection_attribute);
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
        enum_to_case(rci_output_state_collection_count_id);
        enum_to_case(rci_output_state_collection_count_value);
#endif
        enum_to_case(rci_output_state_collection_specifier_id);
        enum_to_case(rci_output_state_collection_specifier_value);
#if (defined RCI_PARSER_USES_DICT)
        enum_to_case(rci_output_state_collection_name_string);
#endif
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
        enum_to_case(rci_output_state_complete_attribute_id);
        enum_to_case(rci_output_state_complete_attribute_value);
        enum_to_case(rci_output_state_remove_attribute_id);
        enum_to_case(rci_output_state_remove_attribute_value);
#endif
#if (defined RCI_PARSER_USES_LIST)
        enum_to_case(rci_output_state_list_id);
#endif
        enum_to_case(rci_output_state_field_id);
        enum_to_case(rci_output_state_field_value);
        enum_to_case(rci_output_state_field_terminator);
        enum_to_case(rci_output_state_group_terminator);
#if (defined RCI_LEGACY_COMMANDS)
        enum_to_case(rci_output_state_do_command_payload);
#endif
        enum_to_case(rci_output_state_response_done);
        enum_to_case(rci_output_state_done);
    }
    return result;
}

static char const * rci_traverse_state_t_as_string(rci_traverse_state_t const value)
{
    char const * result=NULL;
    switch (value)
    {
        enum_to_case(rci_traverse_state_none);
        enum_to_case(rci_traverse_state_command_id);
        enum_to_case(rci_traverse_state_group_id);
        enum_to_case(rci_traverse_state_element_id);
        enum_to_case(rci_traverse_state_element_end);
        enum_to_case(rci_traverse_state_all_groups);
        enum_to_case(rci_traverse_state_group_count);
        enum_to_case(rci_traverse_state_all_group_instances);
#if (defined RCI_PARSER_USES_LIST)
        enum_to_case(rci_traverse_state_list_count);
        enum_to_case(rci_traverse_state_all_list_instances);
        enum_to_case(rci_traverse_state_list_id);
        enum_to_case(rci_traverse_state_list_instances_done);
#endif
        enum_to_case(rci_traverse_state_all_elements);
        enum_to_case(rci_traverse_state_elements_done);
        enum_to_case(rci_traverse_state_group_end);
#if (defined RCI_LEGACY_COMMANDS)
        enum_to_case(rci_traverse_state_command_do_command);
        enum_to_case(rci_traverse_state_command_reboot);
        enum_to_case(rci_traverse_state_command_set_factory_default);
#endif
    }
    return result;
}

#endif
