/*
 * Copyright (c) 2014-2022 Digi International Inc.
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
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

#if (defined CONNECTOR_DATA_POINTS)

#include "connector_data_point_csv_generator.h"

typedef struct
{
    #if (defined CONNECTOR_TRANSPORT_TCP)
    #define DP_FILE_PATH_SIZE   64
    #else
    #define DP_FILE_PATH_SIZE   32
    #endif
    connector_request_data_service_send_t header;
    char file_path[DP_FILE_PATH_SIZE];

    enum
    {
        dp_content_type_binary,
        dp_content_type_csv
    } type;

    union
    {
        struct
        {
            connector_request_data_point_t const * dp_request;
            csv_process_data_t process_data;
        } csv;


        struct
        {
            connector_request_data_point_binary_t const * bp_request;
            uint8_t * current_bp;
            size_t bytes_to_send;
        } binary;

    } data;

} data_point_info_t;

static char const internal_dp4d_path[] = "_DP_PATH_/";
static size_t const internal_dp4d_path_strlen = sizeof internal_dp4d_path - 1;
static char const dp4d_path_prefix[] = "DataPoint/";
static size_t const dp4d_path_prefix_strlen = sizeof dp4d_path_prefix - 1;

static connector_request_data_point_t const * data_point_pending = NULL;
static connector_request_data_point_binary_t const * data_point_binary_pending = NULL;

STATIC connector_status_t dp_initiate_data_point(connector_request_data_point_t const * const dp_ptr)
{
    connector_status_t result = connector_invalid_data;

    ASSERT_GOTO(dp_ptr != NULL, error);

    if (data_point_pending != NULL)
    {
        result = connector_service_busy;
        goto error;
    }

    if (dp_ptr->stream == NULL)
    {
        connector_debug_line("dp_initiate_data_point: NULL data stream");
        goto error;
    }
    else if (dp_ptr->stream->point == NULL)
    {
        connector_debug_line("dp_initiate_data_point: NULL data point");
        goto error;
    }

    data_point_pending = dp_ptr;
    result = connector_success;

error:
    return result;
}

STATIC connector_status_t dp_initiate_data_point_binary(connector_request_data_point_binary_t const * const bp_ptr)
{
    connector_status_t result = connector_invalid_data;

    ASSERT_GOTO(bp_ptr != NULL, error);

    if (data_point_binary_pending != NULL)
    {
        result = connector_service_busy;
        goto error;
    }

    if (bp_ptr->path == NULL)
    {
        connector_debug_line("dp_initiate_data_point_binary: NULL data point path");
        goto error;
    }

    if ((bp_ptr->point == NULL))
    {
        connector_debug_line("dp_initiate_data_point_binary: NULL data point");
        goto error;
    }

    data_point_binary_pending = bp_ptr;
    result = connector_success;

error:
    return result;
}

STATIC connector_status_t dp_callback_status_to_status(connector_callback_status_t const callback_status)
{
    connector_status_t status;

    switch (callback_status)
    {
        case connector_callback_continue:
            status = connector_working;
            break;

        case connector_callback_busy:
            status = connector_pending;
            break;

        default:
            status = connector_abort;
            break;
    }

    return status;
}

STATIC connector_status_t dp_inform_status(connector_data_t * const connector_ptr, connector_request_id_data_point_t request,
                                           connector_transport_t const transport, void * context, connector_session_error_t const error)
{
    connector_status_t result;
    connector_data_point_status_t dp_status;

    dp_status.transport = transport;
    dp_status.user_context = context;
    dp_status.session_error = connector_session_error_none;

    switch (error)
    {
        case connector_session_error_none:
            dp_status.status = connector_data_point_status_complete;
            break;

        case connector_session_error_cancel:
            dp_status.status = connector_data_point_status_cancel;
            break;

        case connector_session_error_timeout:
            dp_status.status = connector_data_point_status_timeout;
            break;

        case connector_session_error_format:
            dp_status.status = connector_data_point_status_invalid_data;
            break;

        default:
            dp_status.status = connector_data_point_status_session_error;
            dp_status.session_error = error;
            break;
    }

    {
        connector_callback_status_t callback_status;
        connector_request_id_t request_id;

        request_id.data_point_request = request;
        callback_status = connector_callback(connector_ptr->callback, connector_class_id_data_point, request_id, &dp_status, connector_ptr->context);
        result = dp_callback_status_to_status(callback_status);
    }

    return result;
}

#if (defined CONNECTOR_SHORT_MESSAGE)
STATIC connector_status_t dp_cancel_session(connector_data_t * const connector_ptr, void const * const session, uint32_t const * const request_id)
{
    connector_status_t status = connector_working;
    connector_bool_t cancel_all = connector_bool(request_id == NULL);

    if (data_point_binary_pending != NULL)
    {
        connector_bool_t const has_request_id = connector_bool(data_point_binary_pending->request_id != NULL);
        connector_bool_t const matching_request = connector_bool(has_request_id && *data_point_binary_pending->request_id == *request_id);

        if (cancel_all || matching_request)
        {
            if (session == NULL)
            {
                status = dp_inform_status(connector_ptr, connector_request_id_data_point_binary_status, data_point_binary_pending->transport, data_point_binary_pending->user_context, connector_session_error_cancel);
                if (status != connector_working)
                  goto done;
            }
            data_point_binary_pending = NULL;
        }
    }

    if (data_point_pending != NULL)
    {
        connector_bool_t const pending_dp_has_request_id = connector_bool(data_point_pending->request_id != NULL);
        connector_bool_t const matching_request = connector_bool(pending_dp_has_request_id && *data_point_pending->request_id == *request_id);

        if (cancel_all || matching_request)
        {
            if (session == NULL)
            {
                status = dp_inform_status(connector_ptr, connector_request_id_data_point_status, data_point_pending->transport, data_point_pending->user_context, connector_session_error_cancel);
                if (status != connector_working)
                  goto done;
            }
            data_point_pending = NULL;
        }
    }
done:
    return status;
}
#endif

STATIC connector_status_t dp_fill_file_path(data_point_info_t * const dp_info, char const * const path, char const * const extension)
{
    connector_status_t result;
    size_t const available_path_bytes = sizeof dp_info->file_path - 1;
    size_t const path_bytes = path == NULL ? 0 : strlen(path);  /* Allow NULL path: User responsible of filling each point stream_id */
    size_t const extension_bytes = strlen(extension);
    size_t const full_path_bytes = internal_dp4d_path_strlen + path_bytes + extension_bytes;

    if (full_path_bytes < available_path_bytes)
    {
        strncpy(dp_info->file_path, internal_dp4d_path, internal_dp4d_path_strlen + 1);
        if (path_bytes)
        {
            strncpy(&dp_info->file_path[internal_dp4d_path_strlen], path, path_bytes + 1);
        }
        strncpy(&dp_info->file_path[internal_dp4d_path_strlen + path_bytes], extension, extension_bytes + 1);
        dp_info->file_path[full_path_bytes] = '\0';
        result = connector_working;
    }
    else
    {
        connector_debug_line("dp_fill_file_path [DataPoint/%s.%s]: file path bytes [%" PRIsize "] exceeds the limit [%" PRIsize "]", path, extension, full_path_bytes, available_path_bytes);
        result = connector_invalid_data;
    }

    return result;
}

STATIC connector_status_t dp_send_message(connector_data_t * const connector_ptr, data_point_info_t * const dp_info,
                                          connector_transport_t const transport, connector_bool_t const response_needed, 
                                          uint32_t * request_id, unsigned long timeout_in_seconds)
{
    connector_status_t result;

    dp_info->header.transport = transport;
    dp_info->header.user_context = dp_info;
    dp_info->header.path = dp_info->file_path;
    dp_info->header.response_required = response_needed;
    dp_info->header.timeout_in_seconds = timeout_in_seconds;
    dp_info->header.content_type = NULL;
    dp_info->header.option = connector_data_service_send_option_overwrite;
    dp_info->header.request_id = request_id;

    result = connector_initiate_action(connector_ptr, connector_initiate_send_data, &dp_info->header);
    switch (result)
    {
        case connector_init_error:
        case connector_unavailable:
        case connector_service_busy:
            result = connector_pending;
            break;

        case connector_success:
            result = connector_working;
            goto done;

        default:
            connector_debug_line("dp_send_message: connector_initiate_action failed [%d]", result);
            break;
    }

done:
    return result;
}

STATIC void * dp_create_dp_info(connector_data_t * const connector_ptr, connector_status_t * result)
{
    void * ptr;

    *result = malloc_data_buffer(connector_ptr, sizeof(data_point_info_t), named_buffer_id(data_point_block), &ptr);
    if (*result != connector_working)
    {
        connector_debug_line("dp_create_dp_info: failed to malloc [%d]", *result);
        ptr = NULL;
    }

    return ptr;
}

STATIC connector_status_t dp_process_csv(connector_data_t * const connector_ptr, connector_request_data_point_t const * const dp_ptr)
{
    connector_status_t result = connector_idle;
    data_point_info_t * const dp_info = dp_create_dp_info(connector_ptr, &result);

    if (dp_info == NULL)
    {
        goto done;
    }

    dp_info->type = dp_content_type_csv;
    dp_info->data.csv.dp_request = dp_ptr;
    dp_info->data.csv.process_data.current_csv_field = csv_data;
    dp_info->data.csv.process_data.current_data_stream = dp_info->data.csv.dp_request->stream;
    dp_info->data.csv.process_data.current_data_point = dp_info->data.csv.process_data.current_data_stream->point;
    dp_info->data.csv.process_data.data.init = connector_false;

    result = dp_fill_file_path(dp_info, NULL, ".csv");
    if (result != connector_working)
    {
        goto error;
    }

    result = dp_send_message(connector_ptr, dp_info, dp_ptr->transport, dp_ptr->response_required, dp_ptr->request_id, dp_ptr->timeout_in_seconds);
    if (result == connector_working)
    {
        goto done;
    }

error:
    if (result != connector_pending)
    {
        result = dp_inform_status(connector_ptr, connector_request_id_data_point_status, dp_ptr->transport, dp_ptr->user_context, connector_session_error_format);
    }


    if (free_data_buffer(connector_ptr, named_buffer_id(data_point_block), dp_info) != connector_working)
    {
        result = connector_abort;
    }

done:
    return result;
}

STATIC connector_status_t dp_process_binary(connector_data_t * const connector_ptr, connector_request_data_point_binary_t const * const bp_ptr)
{
    connector_status_t result = connector_idle;
    data_point_info_t * const dp_info = dp_create_dp_info(connector_ptr, &result);

    if (dp_info == NULL) goto done;

    dp_info->type = dp_content_type_binary;
    dp_info->data.binary.bp_request = bp_ptr;
    dp_info->data.binary.current_bp = bp_ptr->point;
    dp_info->data.binary.bytes_to_send = bp_ptr->bytes_used;

    result = dp_fill_file_path(dp_info, bp_ptr->path, ".bin");
    if (result != connector_working) goto error;
    result = dp_send_message(connector_ptr, dp_info, bp_ptr->transport, bp_ptr->response_required, bp_ptr->request_id, bp_ptr->timeout_in_seconds);
    if (result == connector_working) goto done;

error:
    if (result != connector_pending)
        result = dp_inform_status(connector_ptr, connector_request_id_data_point_binary_status, bp_ptr->transport,
                                  bp_ptr->user_context, connector_session_error_format);

    if (free_data_buffer(connector_ptr, named_buffer_id(data_point_block), dp_info) != connector_working)
        result = connector_abort;

done:
    return result;
}

STATIC connector_status_t dp_process_request(connector_data_t * const connector_ptr, connector_transport_t const transport)
{
    connector_status_t result = connector_idle;

    if (connector_ptr->process_csv)
    {
        if ((data_point_pending != NULL) && (data_point_pending->transport == transport))
        {
            result = dp_process_csv(connector_ptr, data_point_pending);
            if (result != connector_pending)
            {
                connector_ptr->process_csv = connector_false;
                data_point_pending = NULL;
                goto done;
            }
        }
    }
    else
    {
        connector_ptr->process_csv = connector_true;
    }

    if ((data_point_binary_pending != NULL) && (data_point_binary_pending->transport == transport))
    {
        result = dp_process_binary(connector_ptr, data_point_binary_pending);
        if (result != connector_pending)
            data_point_binary_pending = NULL;
    }

done:
    return result;
}

STATIC connector_callback_status_t dp_handle_data_callback(connector_data_service_send_data_t * const data_ptr)
{
    connector_callback_status_t status = connector_callback_abort;
    data_point_info_t * const dp_info = data_ptr->user_context;

    ASSERT_GOTO(dp_info != NULL, error);
    switch (dp_info->type)
    {
        case dp_content_type_binary:
            if (dp_info->data.binary.bytes_to_send > data_ptr->bytes_available)
            {
                data_ptr->bytes_used = data_ptr->bytes_available;
                data_ptr->more_data = connector_true;
            }
            else
            {
                data_ptr->bytes_used = dp_info->data.binary.bytes_to_send;
                data_ptr->more_data = connector_false;
            }

            memcpy(data_ptr->buffer, dp_info->data.binary.current_bp, data_ptr->bytes_used);
            dp_info->data.binary.current_bp += data_ptr->bytes_used;
            dp_info->data.binary.bytes_to_send -= data_ptr->bytes_used;
            break;

        case dp_content_type_csv:
        {
            buffer_info_t buffer_info;

            buffer_info.buffer = (char *)data_ptr->buffer;
            buffer_info.bytes_available = data_ptr->bytes_available;
            buffer_info.bytes_written = 0;
            data_ptr->bytes_used = dp_generate_csv(&dp_info->data.csv.process_data, &buffer_info);
            data_ptr->more_data = connector_bool(dp_info->data.csv.process_data.current_data_point != NULL);
            break;
        }
    }

    status = connector_callback_continue;

error:
    return status;
}

STATIC connector_callback_status_t dp_handle_response_callback(connector_data_t * const connector_ptr, connector_data_service_send_response_t * const data_ptr)
{
    connector_callback_status_t callback_status = connector_callback_abort;
    data_point_info_t * const dp_info = data_ptr->user_context;
    connector_request_id_t request_id;
    connector_data_point_response_t user_data;

    ASSERT_GOTO(dp_info != NULL, error);
    switch (dp_info->type)
    {
        case dp_content_type_binary:
            user_data.user_context = dp_info->data.binary.bp_request->user_context;
            request_id.data_point_request = connector_request_id_data_point_binary_response;
            break;

        case dp_content_type_csv:
            user_data.user_context = dp_info->data.csv.dp_request->user_context;
            request_id.data_point_request = connector_request_id_data_point_response;
            break;
    }

    user_data.transport = data_ptr->transport;
    user_data.hint = data_ptr->hint;
    switch (data_ptr->response)
    {
        case connector_data_service_send_response_success:
            user_data.response = connector_data_point_response_success;
            break;

        case connector_data_service_send_response_bad_request:
            user_data.response = connector_data_point_response_bad_request;
            break;

        case connector_data_service_send_response_unavailable:
            user_data.response = connector_data_point_response_unavailable;
            break;

        case connector_data_service_send_response_cloud_error:
            user_data.response = connector_data_point_response_cloud_error;
            break;
        default:
            ASSERT(connector_false);
    }

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_data_point, request_id, &user_data, connector_ptr->context);
    if (callback_status == connector_callback_busy) goto error;

error:
    return callback_status;
}

STATIC connector_callback_status_t dp_handle_status_callback(connector_data_t * const connector_ptr, connector_data_service_status_t * const data_ptr)
{
    connector_callback_status_t callback_status = connector_callback_abort;
    data_point_info_t * const dp_info = data_ptr->user_context;
    connector_request_id_t request_id;
    connector_data_point_status_t user_data;

    ASSERT_GOTO(dp_info != NULL, error);
    switch (dp_info->type)
    {
        case dp_content_type_binary:
            user_data.user_context = dp_info->data.binary.bp_request->user_context;
            request_id.data_point_request = connector_request_id_data_point_binary_status;
            break;

        case dp_content_type_csv:
            user_data.user_context = dp_info->data.csv.dp_request->user_context;
            request_id.data_point_request = connector_request_id_data_point_status;
            break;
    }

    user_data.transport = data_ptr->transport;
    user_data.session_error = data_ptr->session_error;
    switch (data_ptr->status)
    {
        case connector_data_service_status_complete:
            user_data.status = connector_data_point_status_complete;
            break;

        case connector_data_service_status_cancel:
            user_data.status = connector_data_point_status_cancel;
            break;

        case connector_data_service_status_timeout:
            user_data.status = connector_data_point_status_timeout;
            break;

        case connector_data_service_status_session_error:
            user_data.status = connector_data_point_status_session_error;
            break;

        default:
            user_data.status = connector_data_point_status_session_error;
            break;
    }

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_data_point, request_id, &user_data, connector_ptr->context);
    if (callback_status == connector_callback_busy) goto error;

    if (free_data_buffer(connector_ptr, named_buffer_id(data_point_block), dp_info) != connector_working)
        callback_status = connector_callback_abort;

error:
    return callback_status;
}

#if (defined CONNECTOR_SHORT_MESSAGE)
STATIC connector_callback_status_t dp_handle_length_callback(connector_data_t * const connector_ptr, connector_data_service_length_t * const data_ptr)
{
    connector_callback_status_t status = connector_callback_abort;
    data_point_info_t * const dp_info = data_ptr->user_context;

    ASSERT_GOTO(dp_info != NULL, error);
    UNUSED_PARAMETER(connector_ptr);
    switch (dp_info->type)
    {
        case dp_content_type_binary:
            data_ptr->total_bytes = dp_info->data.binary.bytes_to_send;
            break;

        case dp_content_type_csv:
        {
            buffer_info_t buffer_info;
            csv_process_data_t aux_process_data;

            buffer_info.buffer = NULL;
            buffer_info.bytes_available = SIZE_MAX;
            buffer_info.bytes_written = 0;
            aux_process_data = dp_info->data.csv.process_data;

            data_ptr->total_bytes = dp_generate_csv(&aux_process_data, &buffer_info);
            break;
        }
    }

    status = connector_callback_continue;

error:
    return status;
}
#endif

STATIC connector_callback_status_t dp_handle_callback(connector_data_t * const connector_ptr, connector_request_id_data_service_t const ds_request_id, void * const data)
{
    connector_callback_status_t status;

    switch (ds_request_id)
    {
        case connector_request_id_data_service_send_data:
            status = dp_handle_data_callback(data);
            break;

        case connector_request_id_data_service_send_response:
            status = dp_handle_response_callback(connector_ptr, data);
            break;

        case connector_request_id_data_service_send_status:
            status = dp_handle_status_callback(connector_ptr, data);
            break;

        #if (defined CONNECTOR_SHORT_MESSAGE)
        case connector_request_id_data_service_send_length:
            status = dp_handle_length_callback(connector_ptr, data);
            break;
        #endif

        default:
            status = connector_callback_unrecognized;
            break;
    }

    return status;
}
#endif
