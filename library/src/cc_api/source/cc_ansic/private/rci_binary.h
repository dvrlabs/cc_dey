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

#include "rci_binary_support.h"
#include "rci_binary_debug.h"
#include "rci_binary_util.h"
#include "rci_binary_buffer.h"
#include "rci_binary_string.h"
#include "rci_binary_group.h"
#include "rci_binary_list.h"
#include "rci_binary_element.h"
#include "rci_binary_callback.h"
#include "rci_binary_output.h"
#include "rci_binary_input.h"
#include "rci_binary_traverse.h"
#include "rci_binary_error.h"

STATIC connector_bool_t rci_action_session_start(rci_t * const rci, rci_service_data_t * service_data)
{
    ASSERT(rci->service_data == NULL);
    rci->service_data = service_data;
    ASSERT(rci->service_data != NULL);

    rci_set_buffer(&rci->buffer.input, &rci->service_data->input);
    rci_set_buffer(&rci->buffer.output, &rci->service_data->output);
#if defined RCI_DEBUG
    memset(rci->service_data->output.data, 0, rci->service_data->output.bytes);
#endif

    rci->input.destination = rci_buffer_position(&rci->buffer.input);
    reset_input_content(rci);
    rci->shared.attribute_count = 0;
    rci->shared.attributes_processed = 0;

    rci->output.content.data = NULL;
    rci->output.content.length = 0;
    rci->output.skip_depth = INVALID_DEPTH;

    invalidate_group_id(rci);
    invalidate_group_instance(rci);
#if (defined RCI_PARSER_USES_DICT)
    rci->shared.group.info.keys.list = NULL;
    rci->shared.group.info.keys.key_store[0] = '\0';
    rci->shared.group.info.keys.count = INVALID_INDEX;
#endif
#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
    rci->shared.group.lock = INVALID_ID;
#endif

#if (defined RCI_PARSER_USES_LIST)
    {
        int i;
        for (i = 0; i < RCI_LIST_MAX_DEPTH; i++)
        {
            rci->shared.list.level[i].id = INVALID_ID;
            rci->shared.list.level[i].info.instance = INVALID_INDEX;
#if (defined RCI_PARSER_USES_DICT)
            rci->shared.list.level[i].info.keys.list = NULL;
            rci->shared.list.level[i].info.keys.key_store[0] = '\0';
            rci->shared.list.level[i].info.keys.count = INVALID_INDEX;
#endif
#if (defined RCI_PARSER_USES_VARIABLE_LIST)
            rci->shared.list.level[i].lock = INVALID_ID;
#endif
        }
    }
    set_list_depth(rci, 0);
    set_query_depth(rci, 0);
#endif

    invalidate_element_id(rci);

    rci->shared.callback_data.response.element_value = &rci->shared.value;

    rci->status = rci_status_busy;
    rci->error.command_error = connector_false;
    rci->shared.flag = 0;

    trigger_rci_callback(rci, connector_request_id_remote_config_session_start);
    set_rci_input_state(rci, rci_input_state_command_id);
    state_call(rci, rci_parser_state_input);

#if (!defined CONNECTOR_NO_MALLOC)
    {
        size_t const rci_input_start_size = sizeof "FF:FF:FF:FF:FF:FF";
        connector_data_t * const connector_ptr = rci->service_data->connector_ptr;
        connector_status_t const connector_status = malloc_data(connector_ptr, rci_input_start_size, (void **)&rci->input.storage);

        switch (connector_status)
        {
            case connector_working:
                break;
            default:
                ASSERT(connector_status == connector_working);
                break;
        }
        rci->input.storage_len = rci_input_start_size;
    }
#endif

    set_rci_error_state(rci, rci_error_state_id);

    return connector_true;
}

STATIC connector_bool_t rci_action_session_active(rci_t * const rci)
{
    connector_bool_t success = connector_true;

    switch (rci->status)
    {
        case rci_status_error:
        case rci_status_complete:
        {
            rci->status = rci_status_internal_error;
        }
        /* fall through */
        case rci_status_internal_error:
        {
            success = connector_false;
            break;
        }

        case rci_status_busy:
        {
            break;
        }

        case rci_status_more_input:
        {
            connector_debug_print_buffer("rci_binary more request", rci->service_data->input.data, rci->service_data->input.bytes);
            rci_set_buffer(&rci->buffer.input, &rci->service_data->input);
            if (!destination_in_storage(rci))
            {
                rci->input.destination = rci_buffer_position(&rci->buffer.input);
                reset_input_content(rci);
            }
            rci->status = rci_status_busy;
            break;
        }

        case rci_status_flush_output:
        {
            rci_set_buffer(&rci->buffer.output, &rci->service_data->output);
#if defined RCI_DEBUG
            memset(rci->service_data->output.data, 0, rci->service_data->output.bytes);
#endif
            rci->status = rci_status_busy;
            break;
        }
    }

    return success;
}

STATIC connector_bool_t rci_action_session_lost(rci_t * const rci, rci_service_data_t * service_data)
{
    if (rci->service_data == NULL) {
        rci->service_data = service_data;
    }

    trigger_rci_callback(rci, connector_request_id_remote_config_session_cancel);
    {
        connector_bool_t const success = rci_callback(rci);
        ASSERT(success); UNUSED_VARIABLE(success);
    }

    rci->status = rci_status_complete;

    return connector_false;
}


STATIC rci_status_t rci_binary(connector_data_t * const connector_ptr, rci_session_t const action, rci_service_data_t * service_data)
{
#if (defined CONNECTOR_NO_MALLOC)
    static rci_t static_rci_internal_data;
    rci_t * rci_internal_data = &static_rci_internal_data;

    UNUSED_PARAMETER(connector_ptr);
#else
    rci_t * rci_internal_data = connector_ptr->rci_internal_data;
#endif

    {
        connector_bool_t success = connector_false;

        switch (action)
        {
        case rci_session_start:
            if (rci_internal_data == NULL)
            {
#if (!defined CONNECTOR_NO_MALLOC)
                connector_status_t const connector_status = malloc_data(connector_ptr, sizeof *rci_internal_data, (void **)&rci_internal_data);

                switch (connector_status)
                {
                    case connector_working:
                        rci_internal_data->service_data = NULL;
                        connector_ptr->rci_internal_data = rci_internal_data;
                        break;
                    default:
                        ASSERT(connector_status == connector_working);
                        break;
                }
#endif
            }

            ASSERT(rci_internal_data != NULL);
            success = rci_action_session_start(rci_internal_data, service_data);
            break;

        case rci_session_active:
            ASSERT(rci_internal_data != NULL);
            success = rci_action_session_active(rci_internal_data);
            break;

        case rci_session_lost:
            ASSERT(rci_internal_data != NULL);
            success = rci_action_session_lost(rci_internal_data, service_data);
            break;
        }

        if (!success) goto done;
    }

    if (pending_rci_callback(rci_internal_data))
    {
        connector_remote_config_t * const remote_config = &rci_internal_data->shared.callback_data;

        if (!rci_callback(rci_internal_data))
            goto done;

        if (remote_config->error_id != connector_success)
        {
            rci_group_error(rci_internal_data, remote_config->error_id, remote_config->response.error_hint);
            goto done;
        }
    }

    switch (rci_internal_data->parser.state)
    {
    case rci_parser_state_input:
        rci_parse_input(rci_internal_data);
        break;

    case rci_parser_state_output:
        rci_generate_output(rci_internal_data);
        break;

    case rci_parser_state_traverse:
        rci_traverse_data(rci_internal_data);
        break;

    case rci_parser_state_error:
        rci_generate_error(rci_internal_data);
        break;
    }

done:

    switch (rci_internal_data->status)
    {
    case rci_status_busy:
        break;
    case rci_status_more_input:
#ifdef RCI_DEBUG
        connector_debug_line("Need more input");
#endif
        break;
    case rci_status_flush_output:
        rci_internal_data->service_data->output.bytes = rci_buffer_used(&rci_internal_data->buffer.output);
        break;
    case rci_status_complete:
        rci_internal_data->service_data->output.bytes = rci_buffer_used(&rci_internal_data->buffer.output);
        rci_internal_data->service_data = NULL;
        break;
    case rci_status_internal_error:
    case rci_status_error:
        break;
    }

    return rci_internal_data->status;
}
