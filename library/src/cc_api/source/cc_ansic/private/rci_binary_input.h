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

#include "connector_stringify_tools.h"

#if defined RCI_PARSER_USES_ERROR_DESCRIPTIONS
static char const rci_set_empty_group_hint[] = "Empty group";
static char const rci_set_empty_element_hint[] = "Empty element";
static char const rci_error_descriptor_mismatch_hint[] = "Mismatch configurations";
static char const rci_error_content_size_hint[] = "Maximum content size exceeded";
#else
#define rci_set_empty_group_hint            RCI_NO_HINT
#define rci_set_empty_element_hint          RCI_NO_HINT
#define rci_error_descriptor_mismatch_hint  RCI_NO_HINT
#define rci_error_content_size_hint         RCI_NO_HINT
#endif

STATIC connector_bool_t destination_in_storage(rci_t const * const rci)
{
    uint8_t const * const storage_begin = rci->input.storage;
#if (defined CONNECTOR_NO_MALLOC)
    uint8_t const * const storage_end = storage_begin + sizeof rci->input.storage;
#else
    uint8_t const * const storage_end = storage_begin + rci->input.storage_len;
#endif
    return ptr_in_range(rci->input.destination, storage_begin, storage_end);
}

STATIC size_t get_bytes_to_follow(uint8_t opcode)
{
    size_t bytes_to_read = 0;

    if (opcode & BINARY_RCI_SIZE_ALTERNATE_FLAG)
    {
        rci_size_modifier_t size_mask = BINARY_RCI_SIZE_MODIFIER(opcode);
        switch (size_mask)
        {
        case binary_rci_one_follow_byte:
            bytes_to_read = 1;
            break;
        case binary_rci_multi_follow_byte:
            switch (BINARY_RCI_MULTI_FOLLOW_BYTES(opcode))
            {
                case binary_rci_two_follow_byte:
                    bytes_to_read = 2;
                    break;
                case binary_rci_four_follow_byte:
                    bytes_to_read = 4;
                    break;
                case binary_rci_eight_follow_byte:
                    bytes_to_read = 8;
                    break;
            }
            break;
        case binary_rci_special_value:
            bytes_to_read = 0;
            break;
        default:
            ASSERT(connector_false);
        }
    }
    return bytes_to_read;
}

STATIC void reset_input_content(rci_t * const rci)
{
    rci->shared.content.data = rci->input.destination;
    rci->shared.content.length = 0;
}

#define BINARY_RCI_ONE_BYTE_LIMIT_MASK  UINT32_C(0x7F)
#define BINARY_RCI_HI_BYTE_MASK         UINT32_C(0x1F)

STATIC size_t get_modifier_ber(rci_t * const rci, uint32_t * const value)
{
    uint8_t * const rci_ber = rci->shared.content.data;
    uint8_t const modifier_ber = message_load_u8(rci_ber, value);
    size_t const bytes_to_follow = get_bytes_to_follow(modifier_ber) + 1;
    size_t bytes_read = 0;

    rci->shared.content.length += 1;
    if (rci->shared.content.length < bytes_to_follow)
    {
        goto done;
    }

    switch (bytes_to_follow)
    {
        case record_bytes(rci_ber):
           *value =  (modifier_ber & BINARY_RCI_SIZE_ALTERNATE_FLAG) ? modifier_ber : (modifier_ber & BINARY_RCI_ONE_BYTE_LIMIT_MASK);
            break;
        case record_bytes(rci_ber_u8):
        {
            uint8_t * const rci_ber_u8 = rci_ber;
            /* mask of the 1st byte for data value */
            *value = (modifier_ber & BINARY_RCI_HI_BYTE_MASK) << 8;
            *value |= message_load_u8(rci_ber_u8, value);
            break;
        }
        case record_bytes(rci_ber_u16):
        {
            uint8_t * const rci_ber_u16 = rci_ber;
            *value = message_load_be16(rci_ber_u16, value);
            break;
        }
        case record_bytes(rci_ber_u32):
        {
            uint8_t * const rci_ber_u32 = rci_ber;
            *value = message_load_be32(rci_ber_u32, value);
            break;
        }
        default:
            ASSERT(bytes_to_follow == 1);
            /* NONUM or TRM value */
            *value = modifier_ber;
            break;
    }
    bytes_read = bytes_to_follow;

done:
    return bytes_read;
}

STATIC connector_bool_t get_uint32(rci_t * const rci, uint32_t * const value)
{
    size_t const bytes = get_modifier_ber(rci, value);

    if (bytes > 0)
        reset_input_content(rci);

    return connector_bool(bytes > 0);
}

STATIC connector_bool_t get_string_of_len(rci_t * const rci, char const * * string, uint32_t const length, size_t const offset)
{
    connector_bool_t got_string = connector_false;
    size_t const new_size = offset + length + sizeof "";
    size_t const bytes = rci->shared.content.length;
#if defined CONNECTOR_DEBUG
    size_t const size_max = SIZE_MAX;

    (void)size_max;
    ASSERT(length <= size_max);
#endif
#if (defined CONNECTOR_NO_MALLOC)
    if (new_size > sizeof rci->input.storage)
    {
        connector_debug_line("Maximum content size exceeded while getting  a string - wanted %u, had %u", length, CONNECTOR_NO_MALLOC_RCI_MAXIMUM_CONTENT_LENGTH);
        rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_content_size_hint, rci_output_state_field_id);
        goto done;
    }
#else
    if (new_size > rci->input.storage_len)
    {
        size_t const old_size = rci->input.storage_len;
        connector_data_t * const connector_ptr = rci->service_data->connector_ptr;
        connector_status_t const connector_status = realloc_data(connector_ptr, old_size, new_size, (void **)&rci->input.storage);

        switch (connector_status)
        {
            case connector_working:
                rci->input.storage_len = new_size;
                break;
            default:
                connector_debug_line("Not enough memory for string, wanted %" PRIsize, new_size);
                goto done;
        }
    }
#endif
    if (bytes == (offset + length))
    {
        char * data = (char *)(rci->shared.content.data + offset);

        if (!destination_in_storage(rci))
        {
            memcpy(rci->input.storage, data, length);
            data = (char *)rci->input.storage;
        }

        data[length] = nul;
        *string =  data;

        reset_input_content(rci);
        got_string = connector_true;
    }

done:
    return got_string;
}

STATIC connector_bool_t get_string(rci_t * const rci, char const * * string, size_t * const length)
{
    connector_bool_t got_string = connector_false;
    uint32_t value;
    size_t const ber_bytes = get_modifier_ber(rci, &value);

    if (ber_bytes > 0)
    {
        *length = value;
        got_string = get_string_of_len(rci, string, value, ber_bytes);
    }

    return got_string;
}

#if defined RCI_PARSER_USES_IPV4
STATIC connector_bool_t get_ip_address(rci_t * const rci, uint32_t * const ip_addr, size_t const expected_length)
{
    connector_bool_t got_ip = connector_false;
    uint32_t length;
    size_t const ber_bytes = get_modifier_ber(rci, &length);

#if (!defined CONNECTOR_DEBUG)
    UNUSED_PARAMETER(expected_length);
#endif

    ASSERT_GOTO(ber_bytes == 1, done);
    ASSERT_GOTO(length == expected_length, done);

    if (rci->shared.content.length == expected_length + ber_bytes)
    {
        *ip_addr = LoadBE32(rci->shared.content.data + ber_bytes);

        reset_input_content(rci);
        got_ip = connector_true;
    }

done:
    return got_ip;
}
#endif

#if defined RCI_PARSER_USES_MAC_ADDR
STATIC connector_bool_t get_mac_address(rci_t * const rci, uint8_t * const mac_addr, size_t const expected_length)
{
    connector_bool_t got_mac = connector_false;
    uint32_t length;
    size_t const ber_bytes = get_modifier_ber(rci, &length);

#if (!defined CONNECTOR_DEBUG)
    UNUSED_PARAMETER(expected_length);
#endif

    ASSERT_GOTO(ber_bytes == 1, done);
    ASSERT_GOTO(length == expected_length, done);

    if (rci->shared.content.length == expected_length + ber_bytes)
    {
        uint8_t const * const data = rci->shared.content.data + ber_bytes;
        memcpy(mac_addr, data, length);

        reset_input_content(rci);
        got_mac = connector_true;
    }

done:
    return got_mac;
}
#endif

STATIC connector_bool_t decode_attribute(rci_t * const rci, rci_attribute_info_t * attribute_info)
{
#define BINARY_RCI_ATTRIBUTE_LOW_MASK  0x1F
#define BINARY_RCI_ATTRIBUTE_HIGH_MASK 0x7F80
#define BINARY_RCI_ATTRIBUTE_TYPE_NORMAL_COUNT_MASK 0x0F

    connector_bool_t got_attribute = connector_false;
    uint32_t attribute_value;
    size_t bytes = get_modifier_ber(rci, &attribute_value);

    if (bytes > 0)
    {
        attribute_info->type = attribute_value & BINARY_RCI_ATTRIBUTE_TYPE_MASK;
        switch (attribute_info->type)
        {
            case BINARY_RCI_ATTRIBUTE_TYPE_INDEX:
#if (defined RCI_PARSER_USES_DICT)
            case BINARY_RCI_ATTRIBUTE_TYPE_NAME:
#endif
            case BINARY_RCI_ATTRIBUTE_TYPE_COUNT:
            {
                unsigned int value;
                if (attribute_value & (~BINARY_RCI_ATTRIBUTE_LOW_MASK))
                {
                    /* attribute is wrapped around the "attribute type" bits (bits 5 and 6)
                     *
                     * bit |15| 14 13 12 11 10 9 8 7 | 6 5 | 4 3 2 1 0 |
                     *     | 1|   index_high         | 0 1 | index_low |
                     */
                    uint16_t index_low, index_high;

                    index_low = attribute_value & BINARY_RCI_ATTRIBUTE_LOW_MASK;
                    index_high = (attribute_value & BINARY_RCI_ATTRIBUTE_HIGH_MASK) >> 2;
                    value = index_high | index_low;
                }
                else
                {
                    /* attribute output
                     * bit |7 | 6 5 | 4 3 2 1 0|
                     *     |x | 0 1 | - index -|
                     */
                    value = attribute_value & BINARY_RCI_ATTRIBUTE_LOW_MASK;
                }
#if (defined RCI_PARSER_USES_DICT)
                if (attribute_info->type == BINARY_RCI_ATTRIBUTE_TYPE_INDEX || attribute_info->type == BINARY_RCI_ATTRIBUTE_TYPE_COUNT)
#endif
                {
                    attribute_info->value.index = value;
#ifdef RCI_DEBUG
                    connector_debug_line("decode_attribute: index = %d", attribute_info->value.index);
#endif
                    got_attribute = connector_true;
                }
#if (defined RCI_PARSER_USES_DICT)
                else if (value > 0)
                {
                    if (get_string_of_len(rci, &attribute_info->value.name.data, value, bytes))
                    {
                        attribute_info->value.name.length = value;
#ifdef RCI_DEBUG
                        connector_debug_line("decode_attribute: index = %s", attribute_info->value.name.data);
#endif
                        got_attribute = connector_true;
                    }
                }
                else
                {
                    attribute_info->value.name.length = 0;
                    got_attribute = connector_true;
                }
#endif
                break;
            }
            case BINARY_RCI_ATTRIBUTE_TYPE_NORMAL:
            {
                /* attribute output
                 * bit |7 | 6 5 | 4 3 2 1 0|
                 *     |x | 0 0 | x - cnt -|
                 */

                attribute_info->value.count = attribute_value & BINARY_RCI_ATTRIBUTE_TYPE_NORMAL_COUNT_MASK;
#ifdef RCI_DEBUG
                connector_debug_line("decode_attribute: count = %d", attribute_info->value.count);
#endif
                got_attribute = connector_true;
                break;
            }
        }
    }

    if (got_attribute)
    {
        reset_input_content(rci);
    }

    return got_attribute;
}

STATIC connector_bool_t has_rci_atribute(unsigned int data)
{
    return connector_bool((data & BINARY_RCI_ATTRIBUTE_BIT) == BINARY_RCI_ATTRIBUTE_BIT);
}

STATIC connector_bool_t has_rci_error(rci_t * const rci, unsigned int data)
{

    connector_bool_t const hasError = connector_bool((data & BINARY_RCI_ERROR_INDICATOR_BIT) == BINARY_RCI_ERROR_INDICATOR_BIT);

    UNUSED_PARAMETER(rci);

    if (hasError)
    {
        connector_debug_line("has_rci_error: unexpected error set");
    }
    return hasError;
}

STATIC connector_bool_t has_rci_terminated(unsigned int data)
{
    return connector_bool(data == BINARY_RCI_TERMINATOR);
}

STATIC connector_bool_t has_rci_no_value(unsigned int data)
{
    return connector_bool(data == BINARY_RCI_NO_VALUE);
}

STATIC void process_rci_command(rci_t * const rci)
{
#define BINARY_RCI_COMMAND_MASK   0x3F

    uint32_t command;

    {
        connector_remote_config_t const * const remote_config = &rci->shared.callback_data;

        if (remote_config->error_id != connector_success)
        {
            set_rci_output_state(rci, rci_output_state_field_id);
            state_call(rci, rci_parser_state_output);
            goto done;
        }
    }

    if (get_uint32(rci, &command))
    {

        connector_bool_t const has_attribute = has_rci_atribute(command);

        ASSERT_GOTO(!has_rci_error(rci, command), done);

        command &= BINARY_RCI_COMMAND_MASK;

        rci->command.command_id = (rci_command_t)command;
        rci->shared.attribute_count = 0;

        switch (command)
        {
            case rci_command_set_setting:
            case rci_command_query_setting:
                rci->shared.callback_data.group.type = connector_remote_group_setting;
                break;
            case rci_command_set_state:
            case rci_command_query_state:
                rci->shared.callback_data.group.type = connector_remote_group_state;
                break;
        }

        switch (command)
        {
            case rci_command_set_setting:
            case rci_command_set_state:
                rci->shared.callback_data.action = connector_remote_action_set;
                break;
            case rci_command_query_setting:
            case rci_command_query_state:
                rci->shared.callback_data.action = connector_remote_action_query;
                break;
#if (defined RCI_LEGACY_COMMANDS)
            case rci_command_do_command:
                rci->shared.callback_data.action = connector_remote_action_do_command;
                break;
            case rci_command_reboot:
                rci->shared.callback_data.action = connector_remote_action_reboot;
                break;
            case rci_command_set_factory_default:
                rci->shared.callback_data.action = connector_remote_action_set_factory_def;
                break;
#endif
            default:
                /* unsupported command.
                 * Just go to error state for returning error message.
                 */
                connector_debug_line("unsupported rci command: %d\n", command);
                rci_global_error(rci, connector_fatal_protocol_error_bad_command, RCI_NO_HINT);
                set_rci_command_error(rci);
                state_call(rci, rci_parser_state_error);
                goto done;
        }

        if (has_attribute)
        {
            set_rci_input_state(rci, rci_input_state_command_attribute);
        }
        else
        {
#if (defined RCI_LEGACY_COMMANDS)
            if (rci->command.command_id == rci_command_do_command)
            {
                set_rci_input_state(rci, rci_input_state_do_command_payload);
            }
            else
#endif
            {
                set_rci_input_state(rci, rci_input_state_group_id);

                set_rci_traverse_state(rci, rci_traverse_state_command_id);
                state_call(rci, rci_parser_state_traverse);
            }
        }
    }
done:
    return;
}

STATIC void process_command_attribute(rci_t * const rci)
{
    rci_attribute_info_t attr;

    if (decode_attribute(rci, &attr))
    {
        switch (attr.type)
        {
            case BINARY_RCI_ATTRIBUTE_TYPE_INDEX:
                /* We don't support command attribute; so just ignore it. */
                set_rci_input_state(rci, rci_input_state_group_id);

                set_rci_traverse_state(rci, rci_traverse_state_command_id);
                state_call(rci, rci_parser_state_traverse);
                break;
#if (defined RCI_PARSER_USES_DICT)
            case BINARY_RCI_ATTRIBUTE_TYPE_NAME:
                ASSERT(connector_false);
                break;
#endif

            case BINARY_RCI_ATTRIBUTE_TYPE_NORMAL:

                rci->shared.attribute_count = attr.value.index;
                rci->shared.attributes_processed = 0;

                ASSERT(rci->shared.attribute_count > 0);

                switch (rci->shared.callback_data.action)
                {
                    case connector_remote_action_query:
                        ASSERT_GOTO(rci->shared.callback_data.group.type == connector_remote_group_setting, done);
                        ASSERT_GOTO(rci->shared.attribute_count <= rci_query_setting_attribute_id_count, done);
                        break;
                    case connector_remote_action_set:
                        ASSERT_GOTO(rci->shared.callback_data.group.type == connector_remote_group_setting, done);
                        ASSERT_GOTO(rci->shared.attribute_count <= rci_set_setting_attribute_id_count, done);
                        break;
#if (defined RCI_LEGACY_COMMANDS)
                    case connector_remote_action_do_command:
                        ASSERT_GOTO(rci->shared.attribute_count <= rci_do_command_attribute_id_count, done);
                        break;
                    case connector_remote_action_reboot:
                    case connector_remote_action_set_factory_def:
                        ASSERT_GOTO(0, done);
                        break;
#endif
                }

                set_rci_input_state(rci, rci_input_state_command_normal_attribute_id);
                break;
        }
    }

done:
    return;
}

STATIC void process_command_normal_attribute_id(rci_t * const rci)
{
    uint32_t attribute_id;

    if (get_uint32(rci, &attribute_id))
    {
#if (defined RCI_DEBUG)
        connector_debug_line("attribute_id=%d", attribute_id);
#endif

        rci->command.attribute[rci->shared.attributes_processed].id.val = attribute_id;

        set_rci_input_state(rci, rci_input_state_command_normal_attribute_value);
    }
}

STATIC connector_bool_t process_command_enum_attribute_value(rci_t * const rci)
{
    uint32_t attribute_value;

    if(!get_uint32(rci, &attribute_value))
        return connector_false;

    rci->command.attribute[rci->shared.attributes_processed].value.enum_val = attribute_value;
    rci->command.attribute[rci->shared.attributes_processed].type = attribute_type_enum;
    return connector_true;
}

STATIC void process_command_normal_attribute_value(rci_t * const rci)
{
    switch (rci->command.command_id)
    {
        case rci_command_query_setting:
        {
            if (rci->command.attribute[rci->shared.attributes_processed].id.query_setting >= rci_query_setting_attribute_id_count)
                ASSERT_GOTO(0, done);
            if (!process_command_enum_attribute_value(rci))
                goto done;
            break;
        }
        case rci_command_set_setting:
        {
            if (rci->command.attribute[rci->shared.attributes_processed].id.set_setting >= rci_set_setting_attribute_id_count)
                ASSERT_GOTO(0, done);
            if (!process_command_enum_attribute_value(rci))
                goto done;
            break;
        }
#if (defined RCI_LEGACY_COMMANDS)
        case rci_command_do_command:
        {
            const char * attribute_value;
            size_t attribute_value_len;

            if (!get_string(rci, &attribute_value, &attribute_value_len))
                goto done;

#if (defined RCI_DEBUG)
            connector_debug_line("attribute_len=%d", attribute_value_len);
            connector_debug_line("attribute='%.*s'", attribute_value_len, attribute_value);
#endif

            switch (rci->command.attribute[rci->shared.attributes_processed].id.do_command)
            {
                case rci_do_command_attribute_id_target:
                    ASSERT(attribute_value_len <= RCI_COMMANDS_ATTRIBUTE_MAX_LEN);
                    memcpy(rci->command.attribute[rci->shared.attributes_processed].value.string_val, attribute_value, attribute_value_len);
                    rci->command.attribute[rci->shared.attributes_processed].value.string_val[attribute_value_len] = '\0';
                    break;
                case rci_do_command_attribute_id_count:
                    ASSERT_GOTO(0, done);
                    break;
            }
            rci->command.attribute[rci->shared.attributes_processed].type = attribute_type_string;
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
           ASSERT_GOTO(0, done);
           break;
    }

    rci->shared.attributes_processed++;

    if (rci->shared.attributes_processed == rci->shared.attribute_count)
    {
#if (defined RCI_LEGACY_COMMANDS)
        if (rci->command.command_id == rci_command_do_command)
        {
            set_rci_input_state(rci, rci_input_state_do_command_payload);
        }
        else
#endif
        {
            set_rci_input_state(rci, rci_input_state_group_id);

            set_rci_traverse_state(rci, rci_traverse_state_command_id);
            state_call(rci, rci_parser_state_traverse);
        }
    }
    else
    {
        set_rci_input_state(rci, rci_input_state_command_normal_attribute_id);
    }

done:
    return;
}

STATIC void start_group(rci_t * const rci)
{
#if (defined RCI_PARSER_USES_LIST)
    set_query_depth(rci, 0);
#endif

    if (!have_group_instance(rci))
    {
        if (rci->shared.callback_data.action == connector_remote_action_set)
        {
#if (defined RCI_PARSER_USES_DICT)
            connector_collection_type_t collection_type = get_group_collection_type(rci);
            if (is_dictionary(collection_type) == connector_true)
            {
                set_group_instance(rci, 0);
                rci->shared.group.info.keys.key_store[0] = '\0';
            }
            else
#endif
            {
                set_group_instance(rci, 1);
            }
        }
        else
        {
            set_should_traverse_all_group_instances(rci, connector_true);
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_INPUT, connector_true);
        }
    }

    if (should_remove_instance(rci))
    {
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_INPUT, connector_true);
    }

    set_rci_traverse_state(rci, rci_traverse_state_group_count);
    state_call(rci, rci_parser_state_traverse);
}

STATIC void process_group_id(rci_t * const rci)
{
    uint32_t group_id;
    size_t const group_length = get_modifier_ber(rci, &group_id);

    rci->shared.attributes_processed = 0;

    if (group_length == 0)
    {
        goto done;
    }

    reset_input_content(rci);

    ASSERT_GOTO(!has_rci_error(rci, group_id), done);

    if (has_rci_terminated(group_id) && group_length == 1)
    {
#if (defined RCI_LEGACY_COMMANDS)
        switch (rci->command.command_id)
        {
            case rci_command_do_command:
            case rci_command_reboot:
            case rci_command_set_factory_default:
            {
                set_rci_input_state(rci, rci_input_state_command_id);

                set_rci_traverse_state(rci, rci_traverse_state_group_end);
                state_call(rci, rci_parser_state_traverse);
                goto done;
            }
            default:
                break;
        }
#endif

        if (have_group_id(rci))
        {
            /* not 1st group */
            set_rci_input_state(rci, rci_input_state_command_id);

            set_rci_traverse_state(rci, rci_traverse_state_group_end);
            state_call(rci, rci_parser_state_traverse);
            goto done;
        }
        else
        {
            /* Get all groups if no group has been requested before */
            switch (rci->shared.callback_data.action)
            {
                case connector_remote_action_query:
                    set_should_traverse_all_groups(rci, connector_true);
                    set_rci_traverse_state(rci, rci_traverse_state_all_groups);
                    state_call(rci, rci_parser_state_traverse);
                    break;
                case connector_remote_action_set:
                    connector_debug_line("process_group_id: got set command with no group id specified");
                    rci_set_output_error(rci, connector_fatal_protocol_error_bad_command, rci_set_empty_group_hint, rci_output_state_group_id);
                    break;
#if (defined RCI_LEGACY_COMMANDS)
                case connector_remote_action_do_command:
                case connector_remote_action_reboot:
                case connector_remote_action_set_factory_def:
                    ASSERT_GOTO(0, done);
#endif
            }
            goto done;
        }
    }
    else
    {
        set_group_id(rci, decode_group_id(group_id));

        if (!have_group_id(rci))
        {
            connector_debug_line("process_group_id: unrecognized group (mismatch of descriptors) group id = %d", decode_group_id(group_id));
            rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_group_id);
            ASSERT(connector_false);
            goto done;
        }

        if (has_rci_atribute(group_id))
        {
            set_rci_input_state(rci, rci_input_state_group_attribute);
            goto done;
        }

        set_rci_input_state(rci, rci_input_state_field_id);
    }

    start_group(rci);
    connector_debug_line("process_group_id: group id = %d", get_group_id(rci));

done:
    return;
}

STATIC void handle_index_attribute(rci_t * const rci, uint32_t index, connector_bool_t is_group)
{
    if (is_group)
    {
        set_group_instance(rci, index);
    }
#if (defined RCI_PARSER_USES_LIST)
    else
    {
        set_current_list_instance(rci, index);
    }
#endif
}

#if (defined RCI_PARSER_USES_DICT)
STATIC void handle_name_attribute(rci_t * const rci, char const * const name, size_t const name_len, connector_bool_t is_group)
{
    if (name_len > RCI_DICT_MAX_KEY_LENGTH)
    {
#if (defined RCI_PARSER_USES_LIST)
        rci_output_state_t state = get_list_depth(rci) > 0 ? rci_output_state_field_id : rci_output_state_group_id;
#else
        rci_output_state_t state = rci_output_state_group_id;
#endif
        rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_content_size_hint, state);
    }
    else
    {
        char * target;
        if (is_group)
        {
            target = rci->shared.group.info.keys.key_store;
            set_group_instance(rci, 0);
        }
#if (defined RCI_PARSER_USES_LIST)
        else
        {
            target = rci->shared.list.level[get_list_depth(rci) - 1].info.keys.key_store;
            set_current_list_instance(rci, 0);
        }
#endif
        if (name_len > 0)
        {
            memcpy(target, name, name_len);
        }
        target[name_len] = '\0';
    }
}

STATIC connector_bool_t get_name_attribute(rci_t * const rci, connector_bool_t is_group)
{
    connector_bool_t got_attribute = connector_false;
    const char * attribute_value;
    size_t attribute_value_len;

    if (get_string(rci, &attribute_value, &attribute_value_len))
    {
#if (defined RCI_DEBUG)
        connector_debug_line("get_name_attribute: name = %s\n", attribute_value);
#endif
        handle_name_attribute(rci, attribute_value, attribute_value_len, is_group);
        got_attribute = connector_true;
    }

    return got_attribute;
}
#endif

STATIC void process_group_attribute(rci_t * const rci)
{
    rci_attribute_info_t attr;

    if (decode_attribute(rci, &attr))
    {
        switch (attr.type)
        {
            case BINARY_RCI_ATTRIBUTE_TYPE_INDEX:
                handle_index_attribute(rci, attr.value.index, connector_true);
                break;
#if (defined RCI_PARSER_USES_DICT)
            case BINARY_RCI_ATTRIBUTE_TYPE_NAME:
                handle_name_attribute(rci, attr.value.name.data, attr.value.name.length, connector_true);
                break;
#endif
            case BINARY_RCI_ATTRIBUTE_TYPE_NORMAL:
                rci->shared.attribute_count = attr.value.count;
                set_rci_input_state(rci, rci_input_state_group_normal_attribute_id);
                return;
                break;
            case BINARY_RCI_ATTRIBUTE_TYPE_COUNT:
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_true);
                rci->shared.group.info.keys.count = attr.value.count;
                break;
        }

        set_rci_input_state(rci, rci_input_state_field_id);
        start_group(rci);
    }

    return;
}

#if (defined RCI_PARSER_USES_LIST)
STATIC void start_list(rci_t * const rci)
{
    invalidate_element_id(rci);
    set_rci_input_state(rci, rci_input_state_field_id);

    if (should_skip_input(rci))
    {
        if (destination_in_storage(rci))
        {
            rci->input.destination = rci->buffer.input.current;
            reset_input_content(rci);
        }
        return;
    }

    set_query_depth(rci, get_list_depth(rci));

    if (!have_current_list_instance(rci))
    {
        if (rci->shared.callback_data.action == connector_remote_action_set)
        {
#if (defined RCI_PARSER_USES_DICT)
            connector_collection_type_t collection_type = get_current_collection_type(rci);
            if (is_dictionary(collection_type) == connector_true)
            {
                set_current_list_instance(rci, 0);
                rci->shared.list.level[get_list_depth(rci) - 1].info.keys.key_store[0] = '\0';
            }
            else
#endif
            {
                set_current_list_instance(rci, 1);
            }
        }
        else
        {
            set_should_traverse_all_list_instances(rci, connector_true);
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_INPUT, connector_true);
        }
    }

    if (should_remove_instance(rci))
    {
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SKIP_INPUT, connector_true);
    }

    set_rci_traverse_state(rci, rci_traverse_state_list_count);
    state_call(rci, rci_parser_state_traverse);
}
#endif

STATIC void process_collection_normal_attribute_id(rci_t * const rci, connector_bool_t is_group) /* TODO: Can is_group be replaced with get_list_depth() == 0? */
{
    uint32_t attribute_id;

    if (get_uint32(rci, &attribute_id))
    {
#if (defined RCI_DEBUG)
        connector_debug_line("attribute_id=%d", attribute_id);
#endif
        switch (attribute_id)
        {
            case 0:
            case 1:
            case 2:
                rci->shared.last_attribute_id = attribute_id;
                break;
            default:
                ASSERT(connector_false);
                break;
        }

        if (is_group)
            set_rci_input_state(rci, rci_input_state_group_normal_attribute_value);
#if (defined RCI_PARSER_USES_LIST)
        else
            set_rci_input_state(rci, rci_input_state_list_normal_attribute_value);
#endif
    }
}

STATIC connector_bool_t handle_array_attribute(rci_t * const rci, connector_bool_t is_group)
{
    uint32_t attribute_value;
    connector_bool_t got_attribute = connector_false;

    if (get_uint32(rci, &attribute_value))
    {
        switch (rci->shared.last_attribute_id)
        {
            case rci_array_attribute_index:
                handle_index_attribute(rci, attribute_value, is_group);
                break;
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
            case rci_array_attribute_count:
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_true);
                if (rci->shared.callback_data.action == connector_remote_action_set)
                {
                    if (is_group)
                    {
                        rci->shared.group.info.keys.count = attribute_value;
                    }
#if (defined RCI_PARSER_USES_LIST)
                    else
                    {
                        set_current_list_count(rci, attribute_value);
                    }
#endif
                }
                break;
            case rci_array_attribute_shrink:
                if (rci->shared.callback_data.action == connector_remote_action_set && attribute_value == connector_false)
                    SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_DONT_SHRINK, connector_true);
                break;
#endif
        }
        got_attribute = connector_true;
    }

    return got_attribute;
}

#if (defined RCI_PARSER_USES_DICT)
STATIC connector_bool_t handle_dictionary_attribute(rci_t * const rci, connector_bool_t is_group)
{
    connector_bool_t got_attribute = connector_false;

    if (rci->shared.last_attribute_id == rci_dictionary_attribute_name)
    {
        got_attribute = get_name_attribute(rci, is_group);
    }
    else
    {
        uint32_t attribute_value;
        if (get_uint32(rci, &attribute_value))
        {
            if (rci->shared.callback_data.action == connector_remote_action_set)
            {
                switch (rci->shared.last_attribute_id)
                {
                    case rci_dictionary_attribute_complete:
                        if (attribute_value)
                        {
                            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_true);
                            if (is_group)
                            {
                                rci->shared.group.info.keys.count = 0;
                            }
#if (defined RCI_PARSER_USES_LIST)
                            else
                            {
                                set_current_list_count(rci, 0);
                            }
#endif
                        }
                        break;
                    case rci_dictionary_attribute_remove:
                        ASSERT(!RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_REMOVE));
                        if (attribute_value)
                            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_REMOVE, connector_true);
                        break;
                }
            }
            got_attribute = connector_true;
        }
    }

    return got_attribute;
}
#endif

STATIC void process_collection_normal_attribute_value(rci_t * const rci, connector_bool_t is_group)
{
    connector_bool_t got_attribute = connector_false;

#if (defined RCI_PARSER_USES_LIST)
    connector_collection_type_t collection_type = is_group  ? get_group_collection_type(rci) : get_current_collection_type(rci);
#else
    connector_collection_type_t collection_type = get_group_collection_type(rci);
#endif

    switch (collection_type)
    {
        case connector_collection_type_fixed_array:
            ASSERT(rci->shared.last_attribute_id == rci_array_attribute_index);
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
            /* intentional fall-through */
        case connector_collection_type_variable_array:
#endif
            got_attribute = handle_array_attribute(rci, is_group);
            break;
#if (defined RCI_PARSER_USES_DICT)
        case connector_collection_type_fixed_dictionary:
            ASSERT(rci->shared.last_attribute_id == rci_dictionary_attribute_name);
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
            /* intentional fall-through */
        case connector_collection_type_variable_dictionary:
#endif
            got_attribute = handle_dictionary_attribute(rci, is_group);
            break;
#endif
    }

    if (got_attribute)
    {
        rci->shared.attributes_processed++;

        if (rci->shared.attributes_processed == rci->shared.attribute_count)
        {
            set_rci_input_state(rci, rci_input_state_field_id);

            if (is_group)
            {
                start_group(rci);
            }
#if (defined RCI_PARSER_USES_LIST)
            else if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_TYPE_EXPECTED))
            {
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_TYPE_EXPECTED, connector_false);
                set_rci_input_state(rci, rci_input_state_field_type);
            }
            else
            {
                start_list(rci);
            }
#endif
        }
        else
        {
#if (defined RCI_PARSER_USES_LIST)
            rci_input_state_t const input_state = is_group ? rci_input_state_group_normal_attribute_id : rci_input_state_list_normal_attribute_id;
#else
            rci_input_state_t const input_state = rci_input_state_group_normal_attribute_id;
#endif
            set_rci_input_state(rci, input_state);
        }
    }
}

#if (defined RCI_PARSER_USES_LIST)
STATIC void process_list_attribute(rci_t * const rci)
{
    rci_attribute_info_t attr;

    if (decode_attribute(rci, &attr))
    {
        switch (attr.type)
        {
            case BINARY_RCI_ATTRIBUTE_TYPE_INDEX:
                handle_index_attribute(rci, attr.value.index, connector_false);
                break;
#if (defined RCI_PARSER_USES_DICT)
            case BINARY_RCI_ATTRIBUTE_TYPE_NAME:
                handle_name_attribute(rci, attr.value.name.data, attr.value.name.length, connector_false);
                break;
#endif
            case BINARY_RCI_ATTRIBUTE_TYPE_NORMAL:
                rci->shared.attribute_count = attr.value.count;
                set_rci_input_state(rci, rci_input_state_list_normal_attribute_id);
                return;
                break;
#if (defined RCI_PARSER_USES_VARIABLE_ARRAY)
            case BINARY_RCI_ATTRIBUTE_TYPE_COUNT:
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_SET_COUNT, connector_true);
                set_current_list_count(rci, attr.value.count);
                break;
#endif
        }
        if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_TYPE_EXPECTED))
        {
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_TYPE_EXPECTED, connector_false);
            set_rci_input_state(rci, rci_input_state_field_type);
        }
        else
        {
            start_list(rci);
        }
    }

    return;
}

STATIC void process_list_start(rci_t * const rci, uint32_t value)
{
    if ((value & BINARY_RCI_FIELD_ATTRIBUTE_BIT) == BINARY_RCI_FIELD_ATTRIBUTE_BIT)
    {
        if ((value & BINARY_RCI_FIELD_TYPE_INDICATOR_BIT) == BINARY_RCI_FIELD_TYPE_INDICATOR_BIT)
        {
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_TYPE_EXPECTED, connector_true);
        }

        set_rci_input_state(rci, rci_input_state_list_attribute);
        goto done;
    }

    if ((value & BINARY_RCI_FIELD_TYPE_INDICATOR_BIT) == BINARY_RCI_FIELD_TYPE_INDICATOR_BIT)
    {
        set_rci_input_state(rci, rci_input_state_field_type);
    }
    else
    {
        start_list(rci);
    }

done:
    return;
}
#endif

STATIC void process_field_id(rci_t * const rci)
{
    uint32_t value;

    rci->shared.attributes_processed = 0;

    if (!get_uint32(rci, &value))
    {
        goto done;
    }

#if (defined RCI_PARSER_USES_LIST)
    if (RCI_SHARED_FLAG_IS_SET(rci, RCI_SHARED_FLAG_RESTORE_DEPTH))
    {
        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_RESTORE_DEPTH, connector_false);
        set_element_id(rci, get_current_list_id(rci));
        invalidate_current_list_id(rci);
        invalidate_current_list_instance(rci);
        decrement_list_depth(rci);
    }
#endif

    /* Bit 6  - is Field type present indicator
     * Bit 10 - is attributes present indicator
     * Bit 12 - is Error indicator (Should not be set)
     */
    ASSERT_GOTO(!has_rci_error(rci, value), done);

    if (has_rci_terminated(value))
    {
#if (defined RCI_PARSER_USES_LIST)
        if (get_list_depth(rci) > 0)
            set_rci_input_state(rci, rci_input_state_field_id);
        else
#endif
            set_rci_input_state(rci, rci_input_state_group_id);
        if (should_skip_input(rci))
        {
#if (defined RCI_PARSER_USES_LIST)
            if (get_list_depth(rci) == get_query_depth(rci))
#endif
            {
                SET_RCI_SHARED_FLAG(rci,
                                    RCI_SHARED_FLAG_SKIP_INPUT |
                                    RCI_SHARED_FLAG_DONT_SHRINK |
                                    RCI_SHARED_FLAG_REMOVE |
                                    RCI_SHARED_FLAG_FIRST_ELEMENT,
                                    connector_false);
            }

#if (defined RCI_PARSER_USES_LIST)
            if (get_list_depth(rci) > 0)
            {
                set_element_id(rci, get_current_list_id(rci));
                invalidate_current_list_id(rci);
                invalidate_current_list_instance(rci);
                decrement_list_depth(rci);
            }
#endif
        }
        else if (!have_element_id(rci))
        {
            if (rci->shared.callback_data.action == connector_remote_action_query)
            {
                SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_ALL_ELEMENTS, connector_true);
                set_rci_traverse_state(rci, rci_traverse_state_all_elements);
                state_call(rci, rci_parser_state_traverse);
            }
            else
            {
#if (defined RCI_PARSER_USES_VARIABLE_DICT)
                if (get_current_collection_type(rci) == connector_collection_type_variable_dictionary)
                {
                    rci_collection_info_t const * info;
                    assert(!should_remove_instance(rci)); /* If instance removed skip_input should be set instead */
#if (defined RCI_PARSER_USES_LIST)
                    if (get_list_depth(rci) > 0)
                    {
                        info = &rci->shared.list.level[rci->shared.callback_data.list.depth - 1].info;
                    }
                    else
#endif
                    {
                        info = &rci->shared.group.info;
                    }

                    if (info->keys.key_store[0] != '\0')
                    {
                        SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_FIRST_ELEMENT, connector_true);
                        set_rci_traverse_state(rci, rci_traverse_state_elements_done);
                        state_call(rci, rci_parser_state_traverse);
                        goto done;
                    }
                }
#endif
#if (defined RCI_PARSER_USES_LIST)
                if (get_list_depth(rci) > 0)
                {
                    SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_RESTORE_DEPTH, connector_true);
                    set_rci_output_state(rci, rci_output_state_list_id);
                }
                else
#endif
                {
                    set_rci_output_state(rci, rci_output_state_group_id);
                }

                state_call(rci, rci_parser_state_output);
            }
        }
        else
        {
            /* done with all fields */
            invalidate_element_id(rci);

            set_rci_traverse_state(rci, rci_traverse_state_element_end);
            state_call(rci, rci_parser_state_traverse);
        }
        goto done;
    }

    {
        unsigned int const id = decode_element_id(value);

        if (!have_element_id(rci))
        {
            SET_RCI_SHARED_FLAG(rci, RCI_SHARED_FLAG_FIRST_ELEMENT, connector_true);
        }

        set_element_id(rci, id);

        if (!have_element_id(rci))
        {
            connector_debug_line("process_field_id: unrecognized field id (mismatch of descriptors). element id = %d", id);
            rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_field_id);
            goto done;
        }

#if (defined RCI_PARSER_USES_LIST)
        {
            connector_item_t const * const element = get_current_element(rci);
            if (element->type == connector_element_type_list)
            {
                increment_list_depth(rci);
                set_current_list_id(rci, id);
                process_list_start(rci, value);
                goto done;
            }
        }
#endif

        if ((value & BINARY_RCI_FIELD_ATTRIBUTE_BIT) == BINARY_RCI_FIELD_ATTRIBUTE_BIT)
        {
            connector_debug_line("process_field_id: field attribute is not supported");
            rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, RCI_NO_HINT, rci_output_state_field_id);
            goto done;
        }

        if ((value & BINARY_RCI_FIELD_TYPE_INDICATOR_BIT) == BINARY_RCI_FIELD_TYPE_INDICATOR_BIT)
        {
            set_rci_input_state(rci, rci_input_state_field_type);
        }
        else
        {
            set_rci_input_state(rci, rci_input_state_field_no_value);
        }
    }

done:
    return;
}

STATIC void process_field_type(rci_t * const rci)
{
    connector_item_t const * const element = get_current_element(rci);
    connector_bool_t error = connector_false;
    uint32_t type;

    if (get_uint32(rci, &type))
    {
        ASSERT_GOTO(!has_rci_error(rci, type), done);

        switch (type)
        {
            case rci_field_type_none:
                break;

            default:
                if (element->type != type)
                {
                    connector_debug_line("process_field_type: mismatch field type (type %d) != (actual %d)", type, element->type);
                    rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_field_id);
                    error = connector_true;
                }
#if (defined RCI_PARSER_USES_LIST)
                else if (element->type == connector_element_type_list)
                {
                    start_list(rci);
                    goto done;
                }
#endif
                break;
        }
    }

    if (!error)
    {
        set_rci_input_state(rci, rci_input_state_field_no_value);
    }

done:
    return;
}

STATIC size_t uint8_t_array_to_string(char * const buffer, size_t bytes_available, uint8_t const * const array, size_t const array_size, char const separator, unsigned int const base)
{
    int_info_t int_info;
    buffer_info_t buffer_info;
    size_t i;

    buffer_info.buffer = buffer;
    buffer_info.bytes_available = bytes_available;
    buffer_info.bytes_written = 0;

    for (i = 0; i < array_size; i++)
    {
        char const terminator = i < array_size - 1 ? separator : '\0';

        init_int_info(&int_info, array[i], base);
        if (base == 16)
        {
            int_info.figures = 2; /* Write hex with leading zeroes (1 -> "01")*/
        }
        process_integer(&int_info, &buffer_info);
        put_character(terminator, &buffer_info);
    }

    return buffer_info.bytes_written;
}

STATIC void start_element(rci_t * const rci)
{
    if (should_skip_input(rci))
    {
        if (destination_in_storage(rci))
        {
            rci->input.destination = rci->buffer.input.current;
            reset_input_content(rci);
        }
        return;
    }

    set_rci_traverse_state(rci, rci_traverse_state_element_id);
    state_call(rci, rci_parser_state_traverse);
}

STATIC void process_field_value(rci_t * const rci)
{
    connector_item_t const * const element = get_current_element(rci);
    connector_element_value_type_t const type = element->type;

#if (defined RCI_PARSER_USES_ON_OFF) || (defined RCI_PARSER_USES_BOOLEAN)
    connector_bool_t error = connector_false;
#endif

    switch (type)
    {
#if defined RCI_PARSER_USES_STRINGS

#if defined RCI_PARSER_USES_STRING
    case connector_element_type_string:
#endif

#if defined RCI_PARSER_USES_MULTILINE_STRING
    case connector_element_type_multiline_string:
#endif

#if defined RCI_PARSER_USES_PASSWORD
    case connector_element_type_password:
#endif


#if defined RCI_PARSER_USES_FQDNV4
    case connector_element_type_fqdnv4:
#endif

#if defined RCI_PARSER_USES_FQDNV6
    case connector_element_type_fqdnv6:
#endif

#if defined RCI_PARSER_USES_DATETIME
    case connector_element_type_datetime:
#endif

#if defined RCI_PARSER_USES_REF_ENUM
    case connector_element_type_ref_enum:
#endif
        if (!get_string(rci, &rci->shared.value.string_value, &rci->shared.string_value_length))
        {
            goto done;
        }
        break;
#endif


#if defined RCI_PARSER_USES_INT32
    case connector_element_type_int32:
    {
        int32_t value;

        if (!get_uint32(rci, (uint32_t *)&value))
        {
            goto done;
        }
        rci->shared.value.signed_integer_value = value;
        break;
    }
#endif

#if defined RCI_PARSER_USES_IPV4
    case connector_element_type_ipv4:
    {
        uint32_t ip_addr;

        if (!get_ip_address(rci, &ip_addr, sizeof ip_addr))
        {
            goto done;
        }

        {
            char * const data = (char *)rci->input.storage;
            size_t size_written;
            uint8_t ip[4];
#if (defined CONNECTOR_NO_MALLOC)
            size_t const size_avail = sizeof rci->input.storage;
#else
            size_t const size_avail = rci->input.storage_len;
#endif
            ASSERT(size_avail >= sizeof "255.255.255.255");

            ip[0] = BYTE32_3(ip_addr);
            ip[1] = BYTE32_2(ip_addr);
            ip[2] = BYTE32_1(ip_addr);
            ip[3] = BYTE32_0(ip_addr);

            size_written = uint8_t_array_to_string(data, size_avail, ip, ARRAY_SIZE(ip), '.', 10);

            ASSERT(size_written < size_avail);
            ASSERT_GOTO(size_written > 0, done);

            rci->shared.value.string_value = data;

            #undef MAX_IPV4_VALUE
        }
        break;
    }
#endif

#if defined RCI_PARSER_USES_MAC_ADDR
    case connector_element_type_mac_addr:
    {
        uint8_t mac_addr[SIZEOF_MAC_ADDR];

        if (!get_mac_address(rci, mac_addr, sizeof mac_addr))
        {
            goto done;
        }

        {
            char * const data = (char *)rci->input.storage;
            size_t size_written = 0;

#if (defined CONNECTOR_NO_MALLOC)
            size_t const size_avail = sizeof rci->input.storage;
#else
            size_t const size_avail = rci->input.storage_len;
#endif
            ASSERT(size_avail >= sizeof "FF:FF:FF:FF:FF:FF");

            size_written = uint8_t_array_to_string(data, size_avail, mac_addr, ARRAY_SIZE(mac_addr), ':', 16);

            ASSERT(size_written <= size_avail);
            ASSERT_GOTO(size_written > 0, done);

            rci->shared.value.string_value = data;

            #undef MAX_MAC_VALUE
        }
        break;
    }
#endif

#if (defined RCI_PARSER_USES_UNSIGNED_INTEGER)


#if defined RCI_PARSER_USES_UINT32
    case connector_element_type_uint32:
#endif

#if defined RCI_PARSER_USES_HEX32
    case connector_element_type_hex32:
#endif

#if defined RCI_PARSER_USES_0X_HEX32
    case connector_element_type_0x_hex32:
#endif
    {
        uint32_t value;

        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.unsigned_integer_value = value;
        break;
    }
#endif

#if defined RCI_PARSER_USES_FLOAT
    case connector_element_type_float:
    {
        uint32_t value;
        ASSERT(sizeof (float) == sizeof (uint32_t));

        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        memcpy(&rci->shared.value.float_value, &value, sizeof value);
        break;
    }
#endif

#if defined RCI_PARSER_USES_ENUM
    case connector_element_type_enum:
    {
        uint32_t value;
        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.enum_value = value;
        break;
    }
#endif

#if defined RCI_PARSER_USES_ON_OFF
    case connector_element_type_on_off:
    {
        uint32_t value;

        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.on_off_value = (value == 0)? connector_off : connector_on;
        error = connector_bool((value != 0) && (value != 1));
        break;
    }
#endif

#if defined RCI_PARSER_USES_BOOLEAN
    case connector_element_type_boolean:
    {
        uint32_t value;
        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.boolean_value = (value == 0)? connector_false : connector_true;
        error = connector_bool((value != 0) && (value != 1));
        break;
    }
#endif

#if (defined RCI_PARSER_USES_LIST)
    case connector_element_type_list:
        ASSERT(connector_false);
        break;
#endif
    }

#if (defined RCI_PARSER_USES_ON_OFF) || (defined RCI_PARSER_USES_BOOLEAN)
    if (error)
    {
        connector_debug_line("process_field_value: range check error descriptor problem");
        rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_field_id);
    }
    else
#endif
    {
        start_element(rci);
    }
    set_rci_input_state(rci, rci_input_state_field_id);

done:
    return;
}

STATIC void process_field_no_value(rci_t * const rci)
{
    uint8_t * const rci_ber = rci->shared.content.data;
    uint8_t const modifier_ber = message_load_u8(rci_ber, value);

    if (has_rci_no_value(modifier_ber))
    {
        /* this initializes element.value in case for set setting */
        connector_item_t const * const element = get_current_element(rci);
        connector_element_value_type_t const type = element->type;

        switch (type)
        {
#if defined RCI_PARSER_USES_STRINGS

    #if defined RCI_PARSER_USES_STRING
        case connector_element_type_string:
    #endif

    #if defined RCI_PARSER_USES_MULTILINE_STRING
        case connector_element_type_multiline_string:
    #endif

    #if defined RCI_PARSER_USES_PASSWORD
        case connector_element_type_password:
    #endif


    #if defined RCI_PARSER_USES_FQDNV4
        case connector_element_type_fqdnv4:
    #endif

    #if defined RCI_PARSER_USES_FQDNV6
        case connector_element_type_fqdnv6:
    #endif

    #if defined RCI_PARSER_USES_DATETIME
        case connector_element_type_datetime:
    #endif

    #if defined RCI_PARSER_USES_IPV4
        case connector_element_type_ipv4:
    #endif

    #if defined RCI_PARSER_USES_MAC_ADDR
        case connector_element_type_mac_addr:
    #endif

    #if defined RCI_PARSER_USES_REF_ENUM
        case connector_element_type_ref_enum:
    #endif
            rci->shared.value.string_value = (char *)rci->input.storage;
            rci->input.storage[0] = (uint8_t)nul;
            break;
#endif

#if defined RCI_PARSER_USES_INT32
        case connector_element_type_int32:
            rci->shared.value.signed_integer_value = 0;
            break;
#endif

#if (defined RCI_PARSER_USES_UNSIGNED_INTEGER)


#if defined RCI_PARSER_USES_UINT32
        case connector_element_type_uint32:
#endif

#if defined RCI_PARSER_USES_HEX32
        case connector_element_type_hex32:
#endif

#if defined RCI_PARSER_USES_0X_HEX32
        case connector_element_type_0x_hex32:
#endif
            rci->shared.value.unsigned_integer_value = 0;
            break;
#endif

#if defined RCI_PARSER_USES_FLOAT
        case connector_element_type_float:
            rci->shared.value.float_value = 0;
            break;
#endif

#if defined RCI_PARSER_USES_ENUM
        case connector_element_type_enum:
            rci->shared.value.enum_value = 0;
            break;
#endif

#if defined RCI_PARSER_USES_ON_OFF
        case connector_element_type_on_off:
            rci->shared.value.on_off_value = connector_off;
            break;
#endif

#if defined RCI_PARSER_USES_BOOLEAN
        case connector_element_type_boolean:
            rci->shared.value.boolean_value = connector_false;
            break;
#endif

#if (defined RCI_PARSER_USES_LIST)
        case connector_element_type_list:
            ASSERT(connector_false);
            break;
#endif
        }

        reset_input_content(rci);
        set_rci_input_state(rci, rci_input_state_field_id);
        start_element(rci);
    }
    else
    {
        set_rci_input_state(rci, rci_input_state_field_value);
        process_field_value(rci);
    }

}

#if (defined RCI_LEGACY_COMMANDS)
STATIC void process_do_command_payload(rci_t * const rci)
{
    if (!get_string(rci, &rci->shared.value.string_value, &rci->shared.string_value_length))
    {
        goto done;
    }

    {
        set_rci_traverse_state(rci, rci_traverse_state_command_id);
        state_call(rci, rci_parser_state_traverse);
    }
    set_rci_input_state(rci, rci_input_state_group_id);

done:
    return;
}
#endif

STATIC void rci_parse_input(rci_t * const rci)
{
    rci_buffer_t * const input = &rci->buffer.input;

    while ((rci->parser.state == rci_parser_state_input) && (rci_buffer_remaining(input) != 0))
    {

        if (rci->input.destination != rci_buffer_position(&rci->buffer.input))
        {
            *(rci->input.destination) = rci_buffer_read(input);
        }
        rci->input.destination++;

#if (defined RCI_DEBUG)
        connector_debug_line("input: %s", rci_input_state_t_as_string(rci->input.state));
#endif

        switch (rci->input.state)
        {
            case rci_input_state_command_id:
                process_rci_command(rci);
                break;
            case rci_input_state_command_attribute:
                process_command_attribute(rci);
                break;
            case rci_input_state_command_normal_attribute_id:
                process_command_normal_attribute_id(rci);
                break;
            case rci_input_state_command_normal_attribute_value:
                process_command_normal_attribute_value(rci);
                break;
            case rci_input_state_group_id:
                process_group_id(rci);
                break;
            case rci_input_state_group_attribute:
                process_group_attribute(rci);
                break;
            case rci_input_state_group_normal_attribute_id:
                process_collection_normal_attribute_id(rci, connector_true);
                break;
            case rci_input_state_group_normal_attribute_value:
                process_collection_normal_attribute_value(rci, connector_true);
                break;
#if (defined RCI_PARSER_USES_LIST)
            case rci_input_state_list_attribute:
                process_list_attribute(rci);
                break;
            case rci_input_state_list_normal_attribute_id:
                process_collection_normal_attribute_id(rci, connector_false);
                break;
            case rci_input_state_list_normal_attribute_value:
                process_collection_normal_attribute_value(rci, connector_false);
                break;
#endif
            case rci_input_state_field_id:
                process_field_id(rci);
                break;
            case rci_input_state_field_type:
                process_field_type(rci);
                break;
            case rci_input_state_field_no_value:
                process_field_no_value(rci);
                break;
            case rci_input_state_field_value:
                process_field_value(rci);
                break;
#if (defined RCI_LEGACY_COMMANDS)
            case rci_input_state_do_command_payload:
                process_do_command_payload(rci);
                break;
#endif
            case rci_input_state_done:
                ASSERT(rci->input.state != rci_input_state_done);
                break;
        }

        {
#if (defined CONNECTOR_NO_MALLOC)
            size_t const storage_bytes = sizeof rci->input.storage;
#else
            size_t const storage_bytes = rci->input.storage_len;
#endif
            uint8_t const * const storage_end = rci->input.storage + storage_bytes;

            if (rci->input.destination == storage_end)
            {
                connector_debug_line("Maximum content size exceeded while parsing - wanted %u, had %u", storage_bytes + 1, storage_bytes);
                rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_content_size_hint, rci_output_state_field_id);
                goto done;
            }
        }

        rci_buffer_advance(input, 1);
    }

    if (rci_buffer_remaining(input) == 0)
    {
        if (MsgIsLastData(rci->service_data->input.flags))
        {
            set_rci_input_state(rci, rci_input_state_done);
#if !(defined CONNECTOR_NO_MALLOC)
            {
                if (rci->input.storage != NULL)
                {
                    connector_data_t * const connector_ptr = rci->service_data->connector_ptr;
                    connector_status_t const connector_status = free_data(connector_ptr, rci->input.storage);
                    switch (connector_status)
                    {
                        case connector_working:
                            rci->input.storage = NULL;
                            rci->input.storage_len = 0;
                            break;
                        default:
                            goto done;
                    }
                }
            }
#endif
            state_call(rci, rci_parser_state_traverse);
        }
        else
        {
            uint8_t const * const old_base = rcistr_data(&rci->shared.content);
            uint8_t * const new_base = destination_in_storage(rci) ? rci->input.destination : rci->input.storage;

            if (ptr_in_buffer(old_base, &rci->buffer.input))
            {
                size_t const bytes_wanted = rci->shared.content.length;

                if (bytes_wanted != 0)
                {
#if (defined CONNECTOR_NO_MALLOC)
                    size_t const storage_bytes = sizeof rci->input.storage;
#else
                    size_t const storage_bytes = rci->input.storage_len;
#endif
                    uint8_t const * const storage_end = rci->input.storage + storage_bytes;
                    size_t const bytes_have = (size_t)(storage_end - new_base);

                    if (bytes_wanted >= bytes_have)
                    {
                        connector_debug_line("Maximum content size exceeded while storing - wanted %u, had %u", bytes_wanted, bytes_have);
                        rci_set_output_error(rci, connector_fatal_protocol_error_bad_descriptor, rci_error_content_size_hint, rci_output_state_field_id);
                        goto done;
                    }

                    memcpy(new_base, old_base, bytes_wanted);

                    adjust_rcistr(new_base, old_base, &rci->shared.content);
                    rci->input.destination = new_base + bytes_wanted;
                }
            }

            rci->status = rci_status_more_input;
        }
    }

done:
    return;
}
