/*
* Copyright (c) 2017-2022 Digi International Inc.
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

#include <stdio.h>

void ccapi_receive_thread(void * const argument)
{
    ccapi_data_t * const ccapi_data = argument;

    /* ccapi_data is corrupted, it's likely the implementer made it wrong passing argument to the new thread */
    ASSERT_MSG_GOTO(ccapi_data != NULL, done);

    ccapi_data->thread.receive->status = CCAPI_THREAD_RUNNING;
    while (ccapi_data->thread.receive->status == CCAPI_THREAD_RUNNING)
    {
        ccapi_lock_acquire(ccapi_data->thread.receive->lock);

        if (ccapi_data->thread.receive->status != CCAPI_THREAD_REQUEST_STOP)
        {
            ccapi_svc_receive_t * const svc_receive = ccapi_data->service.receive.svc_receive;

            ASSERT_MSG_GOTO(svc_receive != NULL, done);
            ASSERT_MSG_GOTO(svc_receive->receive_thread_status == CCAPI_RECEIVE_THREAD_DATA_CB_QUEUED, done);
            ASSERT_MSG_GOTO(svc_receive->user_callback.data != NULL, done);

            /* Pass data to the user and get possible response from user */
            svc_receive->receive_error = svc_receive->user_callback.data(svc_receive->target, (ccapi_transport_t)svc_receive->transport,
                                                                   &svc_receive->request_buffer_info,
                                                                   svc_receive->response_required ? &svc_receive->response_buffer_info : NULL);
            /* Check if ccfsm has called status callback cancelling the session while we were waiting for the user */
            if (svc_receive->receive_thread_status == CCAPI_RECEIVE_THREAD_DATA_CB_QUEUED)
            {
                svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_DATA_CB_PROCESSED;
            }
            ccapi_data->service.receive.svc_receive = NULL;
        }
    }
    ASSERT_MSG_GOTO(ccapi_data->thread.receive->status == CCAPI_THREAD_REQUEST_STOP, done);

done:
    ccapi_data->thread.receive->status = CCAPI_THREAD_NOT_STARTED;
    return;
}

static connector_callback_status_t ccapi_process_send_data_request(connector_data_service_send_data_t * const send_ptr)
{
    connector_callback_status_t status = connector_callback_error;

    if (send_ptr != NULL)
    {
        ccapi_svc_send_t * const svc_send = send_ptr->user_context;
        size_t bytes_expected_to_read;

        ASSERT_MSG_GOTO(svc_send != NULL, done);
        bytes_expected_to_read = send_ptr->bytes_available > svc_send->bytes_remaining ? svc_send->bytes_remaining : send_ptr->bytes_available;

        if (!svc_send->sending_file)
        {
            memcpy(send_ptr->buffer, svc_send->next_data, bytes_expected_to_read);
            svc_send->next_data = ((char *)svc_send->next_data) + bytes_expected_to_read;
        }
#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
        else
        {
            size_t bytes_read;

            ccimp_status_t ccimp_status = ccapi_read_file(svc_send->ccapi_data, svc_send->file_handler, send_ptr->buffer, bytes_expected_to_read, &bytes_read);
            if (ccimp_status != CCIMP_STATUS_OK)
            {
                svc_send->request_error = CCAPI_SEND_ERROR_ACCESSING_FILE;
                goto done;
            }

            ASSERT_MSG(bytes_expected_to_read == bytes_read);
        }
#endif

        send_ptr->bytes_used = bytes_expected_to_read;
        svc_send->bytes_remaining -= send_ptr->bytes_used;
        send_ptr->more_data = CCFSM_BOOL(svc_send->bytes_remaining > 0);

        status = connector_callback_continue;
    }
    else
    {
        ccapi_logging_line("ccapi_process_send_data_request: no app data set to send");
    }

done:
    return status;
}

static connector_callback_status_t ccapi_process_send_data_response(connector_data_service_send_response_t const * const resp_ptr)
{
    ccapi_svc_send_t * const svc_send = resp_ptr->user_context;

    ASSERT_MSG_GOTO(svc_send != NULL, done);

    /* TODO: we could have a flag in svc_send where to check if user wants a response or not to skip this callback */

    ccapi_logging_line("Received %s response from Device Cloud", resp_ptr->response == connector_data_service_send_response_success ? "success" : "error");

    switch (resp_ptr->response)
    {
        case connector_data_service_send_response_success:
            svc_send->response_error = CCAPI_SEND_ERROR_NONE;
            break;
        case connector_data_service_send_response_bad_request:
            svc_send->response_error = CCAPI_SEND_ERROR_RESPONSE_BAD_REQUEST;
            break;
        case connector_data_service_send_response_unavailable:
            svc_send->response_error = CCAPI_SEND_ERROR_RESPONSE_UNAVAILABLE;
            break;
        case connector_data_service_send_response_cloud_error:
            svc_send->response_error = CCAPI_SEND_ERROR_RESPONSE_CLOUD_ERROR;
            break;
        case connector_data_service_send_response_COUNT:
            ASSERT_MSG_GOTO(resp_ptr->response != connector_data_service_send_response_COUNT, done);
            break;
    }

    if (resp_ptr->hint != NULL)
    {
        ccapi_logging_line("Device Cloud response hint %s", resp_ptr->hint);
        if (svc_send->hint != NULL)
        {
            size_t const max_hint_length = svc_send->hint->length - 1;

            strncpy(svc_send->hint->string, resp_ptr->hint, max_hint_length);
            svc_send->hint->string[max_hint_length] = '\0';
        }
    }

done:
    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_send_data_status(connector_data_service_status_t const * const status_ptr)
{
    ccapi_svc_send_t * const svc_send = status_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_send != NULL, done);

    ccapi_logging_line("ccapi_process_send_data_status: %d", status_ptr->status);

    switch (status_ptr->status)
    {
        case connector_data_service_status_complete:
            svc_send->status_error = CCAPI_SEND_ERROR_NONE;
            break;
        case connector_data_service_status_cancel:
            svc_send->status_error = CCAPI_SEND_ERROR_STATUS_CANCEL;
            break;
        case connector_data_service_status_timeout:
            svc_send->status_error = CCAPI_SEND_ERROR_STATUS_TIMEOUT;
            break;
        case connector_data_service_status_session_error:
            svc_send->status_error = CCAPI_SEND_ERROR_STATUS_SESSION_ERROR;
            ccapi_logging_line("ccapi_process_send_data_status: session_error=%d", status_ptr->session_error);
            break;
        case connector_data_service_status_COUNT:
            ASSERT_MSG_GOTO(status_ptr->status != connector_data_service_status_COUNT, done);
            break;
    }

    ASSERT_MSG_GOTO(svc_send->send_lock != NULL, done);

    switch (ccapi_lock_release(svc_send->send_lock))
    {
        case CCIMP_STATUS_OK:
            connector_status = connector_callback_continue;
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            connector_status = connector_callback_error;
            break;
    }

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_send_data_length(connector_data_service_length_t * const length_ptr)
{
    ccapi_svc_send_t * const svc_send = length_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_send != NULL, done);

    length_ptr->total_bytes = svc_send->bytes_remaining;

    ccapi_logging_line("ccapi_process_send_data_length: %d", length_ptr->total_bytes);

    connector_status = connector_callback_continue;

done:
    return connector_status;
}

static ccapi_bool_t valid_receive_malloc(void * * ptr, size_t size, ccapi_receive_error_t * const error)
{
    ccapi_bool_t success;

    *ptr = ccapi_malloc(size);

    success = CCAPI_BOOL(*ptr != NULL);

    if (!success)
    {
        *error = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
    }

    return success;
}

static connector_callback_status_t ccapi_process_device_request_target(connector_data_service_receive_target_t * const target_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_svc_receive_t * svc_receive = NULL;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(target_ptr->target != NULL, done);

    ccapi_logging_line("ccapi_process_device_request_target for target = '%s'", target_ptr->target);

    ASSERT_MSG_GOTO(target_ptr->user_context == NULL, done);

    {
        ccapi_receive_error_t receive_error;

        if (!valid_receive_malloc((void**)&svc_receive, sizeof *svc_receive, &receive_error))
        {
            /* We didn't manage to create a user_context. ccfsm will call response and status callbacks without it */
            ASSERT_MSG_GOTO(svc_receive != NULL, done);
        }

        target_ptr->user_context = svc_receive;

        svc_receive->target = NULL;
        svc_receive->transport = target_ptr->transport;
        svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_IDLE;
        svc_receive->user_callback.data = ccapi_data->service.receive.user_callback.data;
        svc_receive->user_callback.status = ccapi_data->service.receive.user_callback.status;
        svc_receive->max_request_size = CCAPI_RECEIVE_NO_LIMIT;
        svc_receive->request_buffer_info.buffer = NULL;
        svc_receive->request_buffer_info.length = 0;
        svc_receive->response_buffer_info.buffer = NULL;
        svc_receive->response_buffer_info.length = 0;
        svc_receive->response_handled_internally = CCAPI_FALSE;
        svc_receive->response_processing.buffer = NULL;
        svc_receive->response_processing.length = 0;
        svc_receive->receive_error = CCAPI_RECEIVE_ERROR_NONE;

        svc_receive->response_required = CCAPI_BOOL(target_ptr->response_required);

        /* CCAPI_RECEIVE_ERROR_CCAPI_STOPPED is not handled here. We assume that if we get a request
           means that ccapi is running. That error will be used in add_receive_target()
         */

        {
            size_t const target_size = strlen(target_ptr->target) + 1;

            if (!valid_receive_malloc((void**)&svc_receive->target, target_size, &svc_receive->receive_error))
            {
                goto done;
            }
            memcpy(svc_receive->target, target_ptr->target, target_size);
        }

        if (!ccapi_data->config.receive_supported)
        {
            svc_receive->receive_error = CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT;
            goto done;
        }

        /* Check if it's a registered target */
        {
            ccapi_receive_target_t const * const added_target = *get_pointer_to_target_entry(ccapi_data, svc_receive->target);

            if (added_target != NULL)
            {
                svc_receive->max_request_size = added_target->max_request_size;
                svc_receive->user_callback.data = added_target->user_callback.data;
                svc_receive->user_callback.status = added_target->user_callback.status;

                connector_status = connector_callback_continue;
                goto done;
            }
        }

        if (svc_receive->user_callback.data == NULL)
        {
            svc_receive->receive_error = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
            goto done;
        }

        /* Ask user if accepts target */
        {
            ccapi_bool_t user_accepts;

            if (ccapi_data->service.receive.user_callback.accept != NULL)
            {
                user_accepts = ccapi_data->service.receive.user_callback.accept(svc_receive->target,
										(ccapi_transport_t)svc_receive->transport);
            }
            else
            {
                /* User didn't provide an accept callback. We accept it always */
                user_accepts = CCAPI_TRUE;
            }

            if (user_accepts)
            {
                connector_status = connector_callback_continue;
            }
            else
            {
                svc_receive->receive_error = CCAPI_RECEIVE_ERROR_USER_REFUSED_TARGET;
            }
        }
    }

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_device_request_data(connector_data_service_receive_data_t * const data_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_svc_receive_t * const svc_receive = data_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_receive != NULL, done);

    if (!ccapi_data->config.receive_supported)
    {
        svc_receive->receive_error = CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT;
        goto done;
    }

    switch (svc_receive->receive_thread_status)
    {
        case CCAPI_RECEIVE_THREAD_IDLE:
        {
            ccapi_logging_line("ccapi_process_device_request_data for target = '%s'. receive_thread_status=CCAPI_RECEIVE_THREAD_IDLE", svc_receive->target);

            {
                ccimp_os_realloc_t ccimp_realloc_data;

                ccimp_realloc_data.new_size = svc_receive->request_buffer_info.length + data_ptr->bytes_used;

                if (svc_receive->max_request_size != CCAPI_RECEIVE_NO_LIMIT && ccimp_realloc_data.new_size > svc_receive->max_request_size)
                {
                    ccapi_logging_line("ccapi_process_device_request_data: request excess max_request_size (%d) for this target", svc_receive->max_request_size);

                    svc_receive->receive_error = CCAPI_RECEIVE_ERROR_REQUEST_TOO_BIG;
                    svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_FREE;
                    goto done;
                }

                ccimp_realloc_data.old_size = svc_receive->request_buffer_info.length;
                ccimp_realloc_data.ptr = svc_receive->request_buffer_info.buffer;
                if (ccimp_os_realloc(&ccimp_realloc_data) != CCIMP_STATUS_OK)
                {
                    ccapi_logging_line("ccapi_process_device_request_data: error ccimp_os_realloc for %d bytes", ccimp_realloc_data.new_size);

                    svc_receive->receive_error = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
                    svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_FREE;
                    goto done;
                }
                svc_receive->request_buffer_info.buffer = ccimp_realloc_data.ptr;

                {
                    uint8_t * const dest_addr = (uint8_t *)svc_receive->request_buffer_info.buffer + svc_receive->request_buffer_info.length;
                    memcpy(dest_addr, data_ptr->buffer, data_ptr->bytes_used);
                }
                svc_receive->request_buffer_info.length += data_ptr->bytes_used;
            }

            if (data_ptr->more_data == connector_false)
            {
                svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_DATA_CB_READY;

                ccapi_logging_line("ccapi_process_device_request_data for target = '%s'. receive_thread_status=CCAPI_RECEIVE_THREAD_DATA_CB_READY", svc_receive->target);

                connector_status = connector_callback_busy;
            }
            else
            {
                connector_status = connector_callback_continue;
            }

            break;
        }

        case CCAPI_RECEIVE_THREAD_DATA_CB_READY:
        {
            ccimp_status_t ccimp_status;

            ccimp_status = ccapi_lock_acquire(ccapi_data->service.receive.receive_lock);
            switch (ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    break;
                case CCIMP_STATUS_ERROR:
                case CCIMP_STATUS_BUSY:
                    ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
                    break;
            }

            if (ccapi_data->service.receive.svc_receive == NULL)
            {
                svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_DATA_CB_QUEUED;

                ccapi_data->service.receive.svc_receive = svc_receive;

                ccimp_status = ccapi_lock_release(ccapi_data->service.receive.receive_lock);
                switch (ccimp_status)
                {
                    case CCIMP_STATUS_OK:
                        break;
                    case CCIMP_STATUS_ERROR:
                    case CCIMP_STATUS_BUSY:
                        ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
                        break;
                }

                ccapi_logging_line("ccapi_process_device_request_data for target = '%s'. receive_thread_status=CCAPI_RECEIVE_THREAD_DATA_CB_READY->CCAPI_RECEIVE_THREAD_DATA_CB_QUEUED", svc_receive->target);

                ccapi_lock_release(ccapi_data->thread.receive->lock);

            }
            else
            {
                ccimp_status = ccapi_lock_release(ccapi_data->service.receive.receive_lock);
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

            connector_status = connector_callback_busy;
            break;
        }
        case CCAPI_RECEIVE_THREAD_DATA_CB_QUEUED:
        {
            connector_status = connector_callback_busy;
            break;
        }
        case CCAPI_RECEIVE_THREAD_DATA_CB_PROCESSED:
        {
            ccapi_logging_line("ccapi_process_device_request_data for target = '%s'. receive_thread_status=CCAPI_RECEIVE_THREAD_DATA_CB_PROCESSED", svc_receive->target);
            ccapi_free(svc_receive->request_buffer_info.buffer);

            if (svc_receive->response_required)
            {
                memcpy(&svc_receive->response_processing, &svc_receive->response_buffer_info, sizeof svc_receive->response_buffer_info);
            }

            svc_receive->receive_thread_status = CCAPI_RECEIVE_THREAD_FREE;

            if (svc_receive->receive_error == CCAPI_RECEIVE_ERROR_NONE)
                connector_status = connector_callback_continue;

            break;
        }
        case CCAPI_RECEIVE_THREAD_FREE:
            break;
    }

done:
    return connector_status;
}

static void fill_internal_error(ccapi_svc_receive_t * svc_receive)
{
#define ERROR_MESSAGE "CCAPI Error %d (%s) while handling target '%s'"

        char const * receive_error_str = NULL;
        size_t receive_error_str_len = 0;

        switch (svc_receive->receive_error)
        {
#define ENUM_TO_CASE_ERR(name)  case name:  receive_error_str = #name; receive_error_str_len = sizeof #name - 1; break
            ENUM_TO_CASE_ERR(CCAPI_RECEIVE_ERROR_CCAPI_NOT_RUNNING);
            ENUM_TO_CASE_ERR(CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT);
            ENUM_TO_CASE_ERR(CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY);
            ENUM_TO_CASE_ERR(CCAPI_RECEIVE_ERROR_INVALID_DATA_CB);
            ENUM_TO_CASE_ERR(CCAPI_RECEIVE_ERROR_USER_REFUSED_TARGET);
            ENUM_TO_CASE_ERR(CCAPI_RECEIVE_ERROR_REQUEST_TOO_BIG);
#undef ENUM_TO_CASE_ERR

            case CCAPI_RECEIVE_ERROR_NONE:
            case CCAPI_RECEIVE_ERROR_INVALID_TARGET:
            case CCAPI_RECEIVE_ERROR_TARGET_NOT_ADDED:
            case CCAPI_RECEIVE_ERROR_TARGET_ALREADY_ADDED:
            case CCAPI_RECEIVE_ERROR_LOCK_FAILED:
            case CCAPI_RECEIVE_ERROR_STATUS_CANCEL:
            case CCAPI_RECEIVE_ERROR_STATUS_TIMEOUT:
            case CCAPI_RECEIVE_ERROR_STATUS_SESSION_ERROR:
            {
                static char const this_receive_error_str[] = "Unexpected error";
                receive_error_str = (char *)this_receive_error_str;
                receive_error_str_len = sizeof this_receive_error_str - 1;
                break;
            }
        }

        receive_error_str_len += sizeof ERROR_MESSAGE + strlen(svc_receive->target);

        if (!valid_receive_malloc(&svc_receive->response_buffer_info.buffer, receive_error_str_len, &svc_receive->receive_error))
        {
              return;
        }
        svc_receive->response_buffer_info.length = sprintf(svc_receive->response_buffer_info.buffer, ERROR_MESSAGE,
                                                                        svc_receive->receive_error, receive_error_str, svc_receive->target);

        ccapi_logging_line("fill_internal_error: Providing response in buffer at %p: %s",
                                    svc_receive->response_buffer_info.buffer, (char*)svc_receive->response_buffer_info.buffer);
}

static connector_callback_status_t ccapi_process_device_request_response(connector_data_service_receive_reply_data_t * const reply_ptr)
{
    ccapi_svc_receive_t * const svc_receive = reply_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_receive != NULL, done);

    ccapi_logging_line("ccapi_process_device_request_response for target = '%s'", svc_receive->target);

    if (!svc_receive->response_required)
    {
        goto done;
    }

    /* We initialize the response buffer for internal errors just once and if there are no a custom response */
    if (svc_receive->receive_error != CCAPI_RECEIVE_ERROR_NONE && !svc_receive->response_handled_internally && svc_receive->response_buffer_info.length == 0)
    {
        svc_receive->response_handled_internally = CCAPI_TRUE;

        fill_internal_error(svc_receive);
        memcpy(&svc_receive->response_processing, &svc_receive->response_buffer_info, sizeof svc_receive->response_buffer_info);
    }

    {
        size_t const bytes_to_send = svc_receive->response_processing.length > reply_ptr->bytes_available ?
                                                 reply_ptr->bytes_available : svc_receive->response_processing.length;

        memcpy(reply_ptr->buffer, svc_receive->response_processing.buffer, bytes_to_send);
        svc_receive->response_processing.buffer = ((char *)svc_receive->response_processing.buffer) + bytes_to_send;

        reply_ptr->bytes_used = bytes_to_send;
        svc_receive->response_processing.length -= reply_ptr->bytes_used;
        reply_ptr->more_data = CCFSM_BOOL(svc_receive->response_processing.length > 0);
    }

    connector_status = connector_callback_continue;

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_device_request_status(connector_data_service_status_t const * const status_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_svc_receive_t * const svc_receive = status_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_receive != NULL, done);

    ccapi_logging_line("ccapi_process_device_request_status for target = '%s'", svc_receive->target);
    ccapi_logging_line("ccapi_process_device_request_status: ccapi_receive_error= %d,  status: %d", svc_receive->receive_error, status_ptr->status);

    /* Prior reported errors by ccapi have priority over the ones reported by the cloud */
    if (svc_receive->receive_error == CCAPI_RECEIVE_ERROR_NONE)
    {
        switch (status_ptr->status)
        {
            case connector_data_service_status_complete:
                svc_receive->receive_error = CCAPI_RECEIVE_ERROR_NONE;
                break;
            case connector_data_service_status_cancel:
                svc_receive->receive_error = CCAPI_RECEIVE_ERROR_STATUS_CANCEL;
                break;
            case connector_data_service_status_timeout:
                svc_receive->receive_error = CCAPI_RECEIVE_ERROR_STATUS_TIMEOUT;
                break;
            case connector_data_service_status_session_error:
                svc_receive->receive_error = CCAPI_RECEIVE_ERROR_STATUS_SESSION_ERROR;
                ccapi_logging_line("ccapi_process_device_request_status: session_error=%d", status_ptr->session_error);
                break;
            case connector_data_service_status_COUNT:
                ASSERT_MSG_GOTO(status_ptr->status != connector_data_service_status_COUNT, done);
                break;
        }
    }

    /* Call the user so he can free allocated response memory and handle errors  */
    if (ccapi_data->config.receive_supported && svc_receive->user_callback.status != NULL)
    {
       ccapi_bool_t const should_user_free_response_buffer = !svc_receive->response_handled_internally && svc_receive->response_required && svc_receive->response_buffer_info.buffer != NULL;
       svc_receive->user_callback.status(svc_receive->target, (ccapi_transport_t)svc_receive->transport,
                           should_user_free_response_buffer ? &svc_receive->response_buffer_info : NULL,
                           svc_receive->receive_error);
    }

    /* Free resources */
    if (svc_receive->target != NULL)
    {
        ccapi_free(svc_receive->target);
    }
    if (svc_receive->response_handled_internally)
    {
        ccapi_logging_line("ccapi_process_device_request_status: Freeing response buffer at %p", svc_receive->response_buffer_info.buffer);
        ccapi_free(svc_receive->response_buffer_info.buffer);
    }
    ccapi_free(svc_receive);

    connector_status = connector_callback_continue;

done:
    return connector_status;
}


static connector_callback_status_t ccapi_process_device_request_length(connector_data_service_length_t * const length_ptr)
{
    ccapi_svc_receive_t const * const svc_receive = length_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_receive != NULL, done);

    length_ptr->total_bytes = svc_receive->response_processing.length;

    connector_status = connector_callback_continue;

done:
    return connector_status;
}

connector_callback_status_t ccapi_data_service_handler(connector_request_id_data_service_t const data_service_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_unrecognized;

    switch (data_service_request)
    {
        case connector_request_id_data_service_send_data:
        {
            connector_data_service_send_data_t * const send_ptr = data;

            connector_status = ccapi_process_send_data_request(send_ptr);

            break;
        }
        case connector_request_id_data_service_send_response:
        {
            connector_data_service_send_response_t const * const resp_ptr = data;

            connector_status = ccapi_process_send_data_response(resp_ptr);

            break;
        }
        case connector_request_id_data_service_send_status:
        {
            connector_data_service_status_t const * const status_ptr = data;

            connector_status = ccapi_process_send_data_status(status_ptr);

            break;
        }
        case connector_request_id_data_service_send_length:
        {
            connector_data_service_length_t * const length_ptr = data;

            connector_status = ccapi_process_send_data_length(length_ptr);

            break;
        }
        case connector_request_id_data_service_receive_target:
        {
            connector_data_service_receive_target_t * const target_ptr = data;

            connector_status = ccapi_process_device_request_target(target_ptr, ccapi_data);

            break;
        }
        case connector_request_id_data_service_receive_data:
        {
            connector_data_service_receive_data_t * const data_ptr = data;

            connector_status = ccapi_process_device_request_data(data_ptr, ccapi_data);

            break;
        }
        case connector_request_id_data_service_receive_reply_data:
        {
            connector_data_service_receive_reply_data_t * const reply_ptr = data;

            connector_status = ccapi_process_device_request_response(reply_ptr);

            break;
        }
        case connector_request_id_data_service_receive_status:
        {
            connector_data_service_status_t const * const status_ptr = data;

            connector_status = ccapi_process_device_request_status(status_ptr, ccapi_data);

            break;
        }
        case connector_request_id_data_service_receive_reply_length:
        {
            connector_data_service_length_t * const length_ptr = data;

            connector_status = ccapi_process_device_request_length(length_ptr);

            break;
        }
    }

    ASSERT_MSG(connector_status != connector_callback_unrecognized);

    return connector_status;
}
#endif
