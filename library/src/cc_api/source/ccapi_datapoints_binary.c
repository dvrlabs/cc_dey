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

#define CCAPI_CONST_PROTECTION_UNLOCK

#include "ccapi_definitions.h"

#if (defined CCIMP_DATA_SERVICE_ENABLED) && (defined CCIMP_DATA_POINTS_ENABLED)

#include "ccapi/ccxapi.h"

static ccapi_dp_b_error_t add_dp_prefix_sufix(char const * const stream_id, char * * dp_path)
{
    static char const dp_prefix[] = "DataPoint/";
    static char const dp_sufix[] = ".bin";
    size_t const dp_prefix_length = sizeof dp_prefix - 1;
    size_t const dp_sufix_length = sizeof dp_sufix - 1;
    ccapi_dp_b_error_t dp_b_error = CCAPI_DP_B_ERROR_NONE;
    size_t stream_id_length;
    char * tmp_dp_path;

    if (stream_id == NULL || *stream_id == '\0')
    {
        ccapi_logging_line("add_dp_prefix_sufix: Invalid stream_id");

        dp_b_error = CCAPI_DP_B_ERROR_INVALID_STREAM_ID;
        goto done;
    }

    stream_id_length = strlen(stream_id);

    tmp_dp_path = ccapi_malloc(dp_prefix_length + stream_id_length + dp_sufix_length + 1);
    if (tmp_dp_path == NULL)
    {
        dp_b_error = CCAPI_DP_B_ERROR_INSUFFICIENT_MEMORY;
        goto done;
    }

    memcpy(&tmp_dp_path[0], dp_prefix, dp_prefix_length);
    memcpy(&tmp_dp_path[dp_prefix_length], stream_id, stream_id_length);
    memcpy(&tmp_dp_path[dp_prefix_length + stream_id_length], dp_sufix, dp_sufix_length + 1);

    *dp_path = tmp_dp_path;
done:
    return dp_b_error;
}

static ccapi_dp_b_error_t send_data_error_to_dp_binary_error(ccapi_send_error_t send_data_error)
{
    ccapi_dp_b_error_t dp_b_error = INVALID_ENUM(ccapi_dp_b_error_t);

    switch (send_data_error)
    {
        case CCAPI_SEND_ERROR_NONE:
            dp_b_error = CCAPI_DP_B_ERROR_NONE;
            break;
        case CCAPI_SEND_ERROR_CCAPI_NOT_RUNNING:
            dp_b_error = CCAPI_DP_B_ERROR_CCAPI_NOT_RUNNING;
            break;
        case CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED:
            dp_b_error = CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED;
            break;
        case CCAPI_SEND_ERROR_FILESYSTEM_NOT_SUPPORTED:
            dp_b_error = CCAPI_DP_B_ERROR_FILESYSTEM_NOT_SUPPORTED;
            break;
        case CCAPI_SEND_ERROR_INVALID_DATA:
            dp_b_error = CCAPI_DP_B_ERROR_INVALID_DATA;
            break;
        case CCAPI_SEND_ERROR_INVALID_LOCAL_PATH:
            dp_b_error = CCAPI_DP_B_ERROR_INVALID_LOCAL_PATH;
            break;
        case CCAPI_SEND_ERROR_NOT_A_FILE:
            dp_b_error = CCAPI_DP_B_ERROR_NOT_A_FILE;
            break;
        case CCAPI_SEND_ERROR_ACCESSING_FILE:
            dp_b_error = CCAPI_DP_B_ERROR_ACCESSING_FILE;
            break;
        case CCAPI_SEND_ERROR_INVALID_HINT_POINTER:
            dp_b_error = CCAPI_DP_B_ERROR_INVALID_HINT_POINTER;
            break;
        case CCAPI_SEND_ERROR_INSUFFICIENT_MEMORY:
            dp_b_error = CCAPI_DP_B_ERROR_INSUFFICIENT_MEMORY;
            break;
        case CCAPI_SEND_ERROR_LOCK_FAILED:
            dp_b_error = CCAPI_DP_B_ERROR_LOCK_FAILED;
            break;
        case CCAPI_SEND_ERROR_INITIATE_ACTION_FAILED:
            dp_b_error = CCAPI_DP_B_ERROR_INITIATE_ACTION_FAILED;
            break;
        case CCAPI_SEND_ERROR_STATUS_CANCEL:
            dp_b_error = CCAPI_DP_B_ERROR_STATUS_CANCEL;
            break;
        case CCAPI_SEND_ERROR_STATUS_TIMEOUT:
            dp_b_error = CCAPI_DP_B_ERROR_STATUS_TIMEOUT;
            break;
        case CCAPI_SEND_ERROR_STATUS_SESSION_ERROR:
            dp_b_error = CCAPI_DP_B_ERROR_STATUS_SESSION_ERROR;
            break;
        case CCAPI_SEND_ERROR_RESPONSE_BAD_REQUEST:
            dp_b_error = CCAPI_DP_B_ERROR_RESPONSE_BAD_REQUEST;
            break;
        case CCAPI_SEND_ERROR_RESPONSE_UNAVAILABLE:
            dp_b_error = CCAPI_DP_B_ERROR_RESPONSE_UNAVAILABLE;
            break;
        case CCAPI_SEND_ERROR_RESPONSE_CLOUD_ERROR:
            dp_b_error = CCAPI_DP_B_ERROR_RESPONSE_CLOUD_ERROR;
            break;
        /* These errors should never happen */
        case CCAPI_SEND_ERROR_INVALID_CLOUD_PATH:
        case CCAPI_SEND_ERROR_INVALID_CONTENT_TYPE:
            dp_b_error = CCAPI_DP_B_ERROR_RESPONSE_CLOUD_ERROR;
            ASSERT_MSG_GOTO(send_data_error != CCAPI_SEND_ERROR_NONE, done);
            break;
    }

done:
    return dp_b_error;
}

ccapi_dp_b_error_t ccxapi_dp_binary_send_data(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes)
{
    ccapi_send_error_t send_data_error;
    ccapi_dp_b_error_t dp_b_error;
    char * dp_path;

    dp_b_error = add_dp_prefix_sufix(stream_id, &dp_path);
    if (dp_b_error != CCAPI_DP_B_ERROR_NONE)
    {
        goto done;
    }

    send_data_error = ccxapi_send_data(ccapi_handle, transport, dp_path, NULL, data, bytes, CCAPI_SEND_BEHAVIOR_OVERWRITE);

    dp_b_error = send_data_error_to_dp_binary_error(send_data_error);

    ccapi_free(dp_path);

done:
    return dp_b_error;
}

ccapi_dp_b_error_t ccxapi_dp_binary_send_data_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    ccapi_send_error_t send_data_error;
    ccapi_dp_b_error_t dp_b_error;
    char * dp_path;

    dp_b_error = add_dp_prefix_sufix(stream_id, &dp_path);
    if (dp_b_error != CCAPI_DP_B_ERROR_NONE)
    {
        goto done;
    }

    send_data_error = ccxapi_send_data_with_reply(ccapi_handle, transport, dp_path, NULL, data, bytes, CCAPI_SEND_BEHAVIOR_OVERWRITE, timeout, hint);

    dp_b_error = send_data_error_to_dp_binary_error(send_data_error);

    ccapi_free(dp_path);

done:
    return dp_b_error;
}

ccapi_dp_b_error_t ccxapi_dp_binary_send_file(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const local_path, char const * const stream_id)
{
    ccapi_send_error_t send_data_error;
    ccapi_dp_b_error_t dp_b_error;
    char * dp_path;

    dp_b_error = add_dp_prefix_sufix(stream_id, &dp_path);
    if (dp_b_error != CCAPI_DP_B_ERROR_NONE)
    {
        goto done;
    }

    send_data_error = ccxapi_send_file(ccapi_handle, transport, local_path, dp_path, NULL, CCAPI_SEND_BEHAVIOR_OVERWRITE);

    dp_b_error = send_data_error_to_dp_binary_error(send_data_error);

    ccapi_free(dp_path);

done:
    return dp_b_error;
}

ccapi_dp_b_error_t ccxapi_dp_binary_send_file_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const local_path, char const * const stream_id, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    ccapi_send_error_t send_data_error;
    ccapi_dp_b_error_t dp_b_error;
    char * dp_path;

    dp_b_error = add_dp_prefix_sufix(stream_id, &dp_path);
    if (dp_b_error != CCAPI_DP_B_ERROR_NONE)
    {
        goto done;
    }

    send_data_error = ccxapi_send_file_with_reply(ccapi_handle, transport, local_path, dp_path, NULL, CCAPI_SEND_BEHAVIOR_OVERWRITE, timeout, hint);

    dp_b_error = send_data_error_to_dp_binary_error(send_data_error);

    ccapi_free(dp_path);

done:
    return dp_b_error;
}

ccapi_dp_b_error_t ccapi_dp_binary_send_data(ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes)
{
    return ccxapi_dp_binary_send_data((ccapi_handle_t)ccapi_data_single_instance, transport, stream_id, data, bytes);
}

ccapi_dp_b_error_t ccapi_dp_binary_send_data_with_reply(ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    return ccxapi_dp_binary_send_data_with_reply((ccapi_handle_t)ccapi_data_single_instance, transport, stream_id, data, bytes, timeout, hint);
}

ccapi_dp_b_error_t ccapi_dp_binary_send_file(ccapi_transport_t const transport, char const * const local_path, char const * const stream_id)
{
    return ccxapi_dp_binary_send_file((ccapi_handle_t)ccapi_data_single_instance, transport, local_path, stream_id);
}

ccapi_dp_b_error_t ccapi_dp_binary_send_file_with_reply(ccapi_transport_t const transport, char const * const local_path, char const * const stream_id, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    return ccxapi_dp_binary_send_file_with_reply((ccapi_handle_t)ccapi_data_single_instance, transport, local_path, stream_id, timeout, hint);
}

#endif
