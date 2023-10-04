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

#if (defined CCIMP_DATA_SERVICE_ENABLED)

typedef struct
{
    connector_request_data_service_send_t header;
    ccapi_svc_send_t svc_send;
} ccapi_send_t;

static ccapi_bool_t valid_malloc(void * * const ptr, size_t const size, ccapi_send_error_t * const error)
{
    ccapi_bool_t success;

    *ptr = ccapi_malloc(size);

    success = CCAPI_BOOL(*ptr != NULL);

    if (!success)
    {
        *error = CCAPI_SEND_ERROR_INSUFFICIENT_MEMORY;
    }

    return success;
}

static ccimp_status_t ccapi_send_lock_acquire(ccapi_send_t const * const send_info, unsigned long const timeout_ms)
{
    ccimp_os_lock_acquire_t acquire_data;
    ccimp_status_t status = CCIMP_STATUS_ERROR;

    ASSERT_MSG_GOTO(send_info->svc_send.send_lock != NULL, done);

    acquire_data.lock = send_info->svc_send.send_lock;
    acquire_data.timeout_ms= timeout_ms;

    status = ccimp_os_lock_acquire(&acquire_data);

    if (status == CCIMP_STATUS_OK && acquire_data.acquired != CCAPI_TRUE)
        status = CCIMP_STATUS_ERROR;

done:
    return status;
}

static ccapi_send_error_t checkargs_send_common(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type)
{
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;
    ccapi_bool_t const * p_transport_started = NULL;

    if (!CCAPI_RUNNING(ccapi_data))
    {
        ccapi_logging_line("checkargs_send_common: CCAPI not started");

        error = CCAPI_SEND_ERROR_CCAPI_NOT_RUNNING;
        goto done;
    }

    switch (transport)
    {
        case CCAPI_TRANSPORT_TCP:
            p_transport_started = &ccapi_data->transport_tcp.connected;
            break;
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
        case CCAPI_TRANSPORT_UDP:
            p_transport_started = &ccapi_data->transport_udp.started;
            break;
#endif
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
        case CCAPI_TRANSPORT_SMS:
            p_transport_started = &ccapi_data->transport_sms.started;
            break;
#endif
    }

    if (p_transport_started == NULL || !*p_transport_started)
    {
        ccapi_logging_line("checkargs_send_common: Transport not started");

        error = CCAPI_SEND_ERROR_TRANSPORT_NOT_STARTED;
        goto done;
    }

    if (cloud_path == NULL || *cloud_path == '\0')
    {
        ccapi_logging_line("checkargs_send_common: Invalid cloud_path");

        error = CCAPI_SEND_ERROR_INVALID_CLOUD_PATH;
        goto done;
    }

    if (content_type != NULL && (*content_type == '\0' || strlen(content_type) > UCHAR_MAX))
    {
        ccapi_logging_line("checkargs_send_common: Invalid content_type");

        error = CCAPI_SEND_ERROR_INVALID_CONTENT_TYPE;
        goto done;
    }

done:
    return error;
}

static ccapi_send_error_t checkargs_send_data(void const * const data, size_t const bytes)
{
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;

    if (data == NULL || bytes == 0)
    {
        ccapi_logging_line("checkargs_send_data: Invalid data");

        error = CCAPI_SEND_ERROR_INVALID_DATA;
        goto done;
    }

done:
    return error;
}

static ccapi_send_error_t checkargs_send_file(char const * const local_path)
{
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;

    if (local_path == NULL || *local_path == '\0')
    {
        ccapi_logging_line("checkargs_send_file: Invalid local_path");

        error = CCAPI_SEND_ERROR_INVALID_LOCAL_PATH;
        goto done;
    }

done:
    return error;
}

static ccapi_send_error_t checkargs_send_with_reply(unsigned long const timeout, ccapi_string_info_t * const hint)
{
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;

    /* No check needed for timeout */
    UNUSED_ARGUMENT(timeout);

    if (hint != NULL && (hint->string == NULL || hint->length == 0))
    {
        ccapi_logging_line("checkargs_send_with_reply: Invalid hint pointer");

        error = CCAPI_SEND_ERROR_INVALID_HINT_POINTER;
        goto done;
    }

done:
    return error;
}

static ccapi_send_error_t setup_send_common(ccapi_data_t * const ccapi_data, ccapi_send_t * const send_info, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t const behavior)
{
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;
    ccimp_os_lock_create_t create_data;

    if (ccimp_os_lock_create(&create_data) != CCIMP_STATUS_OK)
    {
        error = CCAPI_SEND_ERROR_LOCK_FAILED;
        goto done;
    }

    send_info->svc_send.send_lock = create_data.lock;

    send_info->header.transport = ccapi_to_connector_transport(transport);
    send_info->header.request_id = NULL;

    switch (behavior)
    {
        case CCAPI_SEND_BEHAVIOR_APPEND:
            send_info->header.option = connector_data_service_send_option_append;
            break;
        case CCAPI_SEND_BEHAVIOR_OVERWRITE:
            send_info->header.option = connector_data_service_send_option_overwrite;
            break;
    }

    send_info->svc_send.ccapi_data = ccapi_data;
    send_info->svc_send.request_error = CCAPI_SEND_ERROR_NONE;
    send_info->svc_send.response_error = CCAPI_SEND_ERROR_NONE;
    send_info->svc_send.status_error = CCAPI_SEND_ERROR_NONE;
    send_info->header.path = cloud_path;
    send_info->header.content_type = content_type;
    send_info->header.user_context = &send_info->svc_send;

done:
    return error;
}

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
static ccapi_send_error_t setup_send_file(ccapi_data_t * const ccapi_data, ccapi_send_t * const send_info, char const * const local_path)
{
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;

    {
        ccimp_fs_stat_t fs_status;

        switch (ccapi_get_dir_entry_status(ccapi_data, local_path, &fs_status))
        {
            case CCIMP_STATUS_OK:
                break;
            case CCIMP_STATUS_BUSY:
            case CCIMP_STATUS_ERROR:
                error = CCAPI_SEND_ERROR_NOT_A_FILE;
                goto done;
        }

        switch (fs_status.type)
        {
            case CCIMP_FS_DIR_ENTRY_FILE:
                send_info->svc_send.bytes_remaining = fs_status.file_size;
                break;
            case CCIMP_FS_DIR_ENTRY_DIR:
            case CCIMP_FS_DIR_ENTRY_UNKNOWN:
                error = CCAPI_SEND_ERROR_NOT_A_FILE;
                goto done;
                break;
        }
    }

    {
        ccimp_fs_file_handle_t file_handler;
        ccimp_status_t const ccimp_status = ccapi_open_file(ccapi_data, local_path, CCIMP_FILE_O_RDONLY, &file_handler);

        if (ccimp_status != CCIMP_STATUS_OK)
        {
            error = CCAPI_SEND_ERROR_ACCESSING_FILE;
            goto done;
        }

        send_info->svc_send.file_handler = file_handler;
    }

    ccapi_logging_line("file_size=%d, file_handler=%p", send_info->svc_send.bytes_remaining, send_info->svc_send.file_handler);

    send_info->svc_send.sending_file = CCAPI_TRUE;
    send_info->svc_send.next_data = NULL;

done:
    return error;
}
#endif

static void setup_send_data(ccapi_send_t * const send_info, void const * const data, size_t const bytes)
{
    send_info->svc_send.next_data = (void *)data;
    send_info->svc_send.bytes_remaining = bytes;
    send_info->svc_send.sending_file = CCAPI_FALSE;
}

static ccapi_send_error_t perform_send(ccapi_data_t * const ccapi_data, ccapi_send_t * const send_info)
{
    connector_status_t ccfsm_status;
    ccapi_send_error_t error = CCAPI_SEND_ERROR_NONE;

    for (;;)
    {
        ccfsm_status = connector_initiate_action_secure(ccapi_data, connector_initiate_send_data, &send_info->header);
        if (ccfsm_status != connector_service_busy)
        {
            break;
        }
        ccimp_os_yield();
    }

    switch(ccfsm_status)
    {
        case connector_success:
            break;
        case connector_init_error:
        case connector_invalid_data_size:
        case connector_invalid_data_range:
        case connector_invalid_data:
        case connector_keepalive_error:
        case connector_bad_version:
        case connector_device_terminated:
        case connector_service_busy:
        case connector_invalid_response:
        case connector_no_resource:
        case connector_unavailable:
        case connector_idle:
        case connector_working:
        case connector_pending:
        case connector_active:
        case connector_abort:
        case connector_device_error:
        case connector_exceed_timeout:
        case connector_invalid_payload_packet:
        case connector_open_error:
            ccapi_logging_line("perform_send: ccfsm error %d", ccfsm_status);
            error = CCAPI_SEND_ERROR_INITIATE_ACTION_FAILED;
            goto done;
            break;
    }

    {
        ccimp_status_t const result = ccapi_send_lock_acquire(send_info, OS_LOCK_ACQUIRE_INFINITE);

        if (result != CCIMP_STATUS_OK)
        {
            ccapi_logging_line("perform_send: lock_acquire failed");
            error = CCAPI_SEND_ERROR_LOCK_FAILED;
        }
    }
done:
    return error;
}

static void finish_send(ccapi_data_t * const ccapi_data, ccapi_send_t * const send_info)
{
    /* Free resources */
    if (send_info != NULL)
    {
        if (send_info->svc_send.send_lock != NULL)
        {
            ccimp_status_t const ccimp_status = ccapi_lock_destroy(send_info->svc_send.send_lock);
            switch (ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    break;
                case CCIMP_STATUS_ERROR:
                case CCIMP_STATUS_BUSY:
                    ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
                    break;
            }
        }

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
        if (send_info->svc_send.sending_file)
        {
            ccimp_status_t const ccimp_status = ccapi_close_file(ccapi_data, send_info->svc_send.file_handler);
            switch (ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    break;
                case CCIMP_STATUS_BUSY:
                case CCIMP_STATUS_ERROR:
                    ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
                    break;
            }

        }
#else
        UNUSED_ARGUMENT(ccapi_data);
#endif

        ccapi_free(send_info);
    }
}

ccapi_send_error_t ccxapi_send_data_common(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t const bytes, ccapi_send_behavior_t const behavior, ccapi_bool_t const with_reply, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    ccapi_send_error_t error;
    ccapi_send_t * send_info = NULL;

    error = checkargs_send_common(ccapi_data, transport, cloud_path, content_type);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    error = checkargs_send_data(data, bytes);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    error = checkargs_send_with_reply(timeout, hint);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    if (!valid_malloc((void**)&send_info, sizeof *send_info, &error))
    {
        goto done;
    }

    error = setup_send_common(ccapi_data, send_info, transport, cloud_path, content_type, behavior);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    setup_send_data(send_info, data, bytes);

    send_info->svc_send.hint = hint;
    send_info->header.response_required = CCAPI_BOOL_TO_CONNECTOR_BOOL(with_reply);
    send_info->header.timeout_in_seconds = timeout;

    error = perform_send(ccapi_data, send_info);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    if (send_info->svc_send.request_error != CCAPI_SEND_ERROR_NONE)
    {
        error = send_info->svc_send.request_error;
    }
    else if (with_reply && send_info->svc_send.response_error != CCAPI_SEND_ERROR_NONE)
    {
        /* tcp transport will update response_error even if not requested but we ignore it if with_reply is false */
        error = send_info->svc_send.response_error;
    }
    else
    {
        error = send_info->svc_send.status_error;
    }

done:
    finish_send(ccapi_data, send_info);

    return error;
}

ccapi_send_error_t ccxapi_send_file_common(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t const behavior, ccapi_bool_t const with_reply, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    ccapi_send_error_t error;
    ccapi_send_t * send_info = NULL;

#if !(defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
    UNUSED_ARGUMENT(transport);
    UNUSED_ARGUMENT(local_path);
    UNUSED_ARGUMENT(cloud_path);
    UNUSED_ARGUMENT(content_type);
    UNUSED_ARGUMENT(behavior);
    UNUSED_ARGUMENT(with_reply);
    UNUSED_ARGUMENT(timeout);
    UNUSED_ARGUMENT(hint);

    error = CCAPI_SEND_ERROR_FILESYSTEM_NOT_SUPPORTED;
    goto done;
#else
    error = checkargs_send_common(ccapi_data, transport, cloud_path, content_type);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    error = checkargs_send_file(local_path);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    error = checkargs_send_with_reply(timeout, hint);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    if (!valid_malloc((void**)&send_info, sizeof *send_info, &error))
    {
        goto done;
    }

    error = setup_send_common(ccapi_data, send_info, transport, cloud_path, content_type, behavior);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    error = setup_send_file(ccapi_data, send_info, local_path);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    send_info->svc_send.hint = hint;
    send_info->header.response_required = CCAPI_BOOL_TO_CONNECTOR_BOOL(with_reply);
    send_info->header.timeout_in_seconds = timeout;

    error = perform_send(ccapi_data, send_info);
    if (error != CCAPI_SEND_ERROR_NONE)
    {
        goto done;
    }

    if (send_info->svc_send.request_error != CCAPI_SEND_ERROR_NONE)
    {
        error = send_info->svc_send.request_error;
    }
    else if (with_reply && send_info->svc_send.response_error != CCAPI_SEND_ERROR_NONE)
    {
        /* tcp transport will update response_error even if not requested but we ignore it if with_reply is false */
        error = send_info->svc_send.response_error;
    }
    else
    {
        error = send_info->svc_send.status_error;
    }
#endif

done:
    finish_send(ccapi_data, send_info);

    return error;
}


ccapi_send_error_t ccxapi_send_data(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t const bytes, ccapi_send_behavior_t const behavior)
{
    return ccxapi_send_data_common(ccapi_data, transport, cloud_path, content_type, data, bytes, behavior, CCAPI_FALSE, CCAPI_SEND_WAIT_FOREVER, NULL);
}

ccapi_send_error_t ccapi_send_data(ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t const bytes, ccapi_send_behavior_t const behavior)
{
    return ccxapi_send_data_common(ccapi_data_single_instance, transport, cloud_path, content_type, data, bytes, behavior, CCAPI_FALSE, CCAPI_SEND_WAIT_FOREVER, NULL);
}

ccapi_send_error_t ccxapi_send_data_with_reply(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t const bytes, ccapi_send_behavior_t const behavior, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    return ccxapi_send_data_common(ccapi_data, transport, cloud_path, content_type, data, bytes, behavior, CCAPI_TRUE, timeout, hint);
}

ccapi_send_error_t ccapi_send_data_with_reply(ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t const bytes, ccapi_send_behavior_t const behavior, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    return ccxapi_send_data_common(ccapi_data_single_instance, transport, cloud_path, content_type, data, bytes, behavior, CCAPI_TRUE, timeout, hint);
}

ccapi_send_error_t ccxapi_send_file(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t const behavior)
{
    return ccxapi_send_file_common(ccapi_data, transport, local_path, cloud_path, content_type, behavior, CCAPI_FALSE, CCAPI_SEND_WAIT_FOREVER, NULL);
}

ccapi_send_error_t ccapi_send_file(ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t const behavior)
{
    return ccxapi_send_file_common(ccapi_data_single_instance, transport, local_path, cloud_path, content_type, behavior, CCAPI_FALSE, CCAPI_SEND_WAIT_FOREVER, NULL);
}

ccapi_send_error_t ccxapi_send_file_with_reply(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t const behavior, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    return ccxapi_send_file_common(ccapi_data, transport, local_path, cloud_path, content_type, behavior, CCAPI_TRUE, timeout, hint);
}

ccapi_send_error_t ccapi_send_file_with_reply(ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t const behavior, unsigned long const timeout, ccapi_string_info_t * const hint)
{
    return ccxapi_send_file_common(ccapi_data_single_instance, transport, local_path, cloud_path, content_type, behavior, CCAPI_TRUE, timeout, hint);
}
#endif
