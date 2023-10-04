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

#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)

#if (defined CONNECTOR_SM_CLI)

#include <stdio.h>

void ccapi_cli_thread(void * const argument)
{
    ccapi_data_t * const ccapi_data = argument;

    /* ccapi_data is corrupted, it's likely the implementer made it wrong passing argument to the new thread */
    ASSERT_MSG_GOTO(ccapi_data != NULL, done);

    ccapi_data->thread.cli->status = CCAPI_THREAD_RUNNING;
    while (ccapi_data->thread.cli->status == CCAPI_THREAD_RUNNING)
    {
        ccapi_lock_acquire(ccapi_data->thread.cli->lock);

        if (ccapi_data->thread.cli->status != CCAPI_THREAD_REQUEST_STOP)
        {
            ccapi_svc_cli_t * const svc_cli = ccapi_data->service.cli.svc_cli;

            ASSERT_MSG_GOTO(svc_cli != NULL, done);

            ASSERT_MSG_GOTO(svc_cli->cli_thread_status == CCAPI_CLI_THREAD_REQUEST_CB_QUEUED, done);
            ASSERT_MSG_GOTO(ccapi_data->service.cli.user_callback.request != NULL, done);

            /* Pass data to the user and get possible response from user */
            ccapi_data->service.cli.user_callback.request(svc_cli->transport,
                                                               svc_cli->request_string_info.string,
                                                               svc_cli->response_required ? (char const * *)&svc_cli->response_string_info.string : NULL);

            /* Check if ccfsm has called status callback cancelling the session while we were waiting for the user */
            if (svc_cli->cli_thread_status == CCAPI_CLI_THREAD_REQUEST_CB_QUEUED)
            {
                svc_cli->cli_thread_status = CCAPI_CLI_THREAD_REQUEST_CB_PROCESSED;
            }
            ccapi_data->service.cli.svc_cli = NULL;
        }

    }
    ASSERT_MSG_GOTO(ccapi_data->thread.cli->status == CCAPI_THREAD_REQUEST_STOP, done);

done:
    ccapi_data->thread.cli->status = CCAPI_THREAD_NOT_STARTED;
    return;
}

static ccapi_bool_t valid_cli_malloc(void * * ptr, size_t size, ccapi_cli_error_t * const error)
{
    ccapi_bool_t success;

    *ptr = ccapi_malloc(size);

    success = CCAPI_BOOL(*ptr != NULL);

    if (!success)
    {
        ccapi_logging_line("valid_cli_malloc: error ccimp_os_realloc for %d bytes", size);
        *error = CCAPI_CLI_ERROR_INSUFFICIENT_MEMORY;
    }

    return success;
}

static connector_callback_status_t ccapi_process_cli_request(connector_sm_cli_request_t * const cli_request_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_svc_cli_t * svc_cli = NULL;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(cli_request_ptr->buffer != NULL && cli_request_ptr->bytes_used, done);

    if (cli_request_ptr->user_context == NULL)
    {
        ccapi_cli_error_t cli_error;

        if (!valid_cli_malloc((void**)&svc_cli, sizeof *svc_cli, &cli_error))
        {
            /* We didn't manage to create a user_context. ccfsm will call response and status callbacks without it */
            ASSERT_MSG_GOTO(svc_cli != NULL, done);
        }

        cli_request_ptr->user_context = svc_cli;

        svc_cli->transport = cli_request_ptr->transport;
        svc_cli->cli_thread_status = CCAPI_CLI_THREAD_IDLE;
        svc_cli->request_string_info.string = NULL;
        svc_cli->request_string_info.length = 0;
        svc_cli->response_string_info.string = NULL;
        svc_cli->response_string_info.length = 0;
        svc_cli->response_handled_internally = CCAPI_FALSE;
        svc_cli->response_processing.string = NULL;
        svc_cli->response_processing.length = 0;
        svc_cli->response_required = CCAPI_BOOL(cli_request_ptr->response_required);
        svc_cli->more_data = CCAPI_BOOL(cli_request_ptr->more_data);
        svc_cli->cli_error = CCAPI_CLI_ERROR_NONE;
    }
    else
    {
        svc_cli = cli_request_ptr->user_context;

        svc_cli->more_data = CCAPI_BOOL(cli_request_ptr->more_data);
    }

    if (!ccapi_data->config.cli_supported)
    {
        ccapi_logging_line("ccapi_process_cli_request. cli service not stated");

        svc_cli->cli_error = CCAPI_CLI_ERROR_NO_CLI_SUPPORT;
        goto done;
    }

    switch (svc_cli->cli_thread_status)
    {
        case CCAPI_CLI_THREAD_IDLE:
        {
            ccapi_logging_line("ccapi_process_cli_request. cli_thread_status=CCAPI_CLI_THREAD_IDLE");

            {
                ccimp_os_realloc_t ccimp_realloc_data;

                ccimp_realloc_data.new_size = svc_cli->request_string_info.length + cli_request_ptr->bytes_used; /* buffer is already null terminated */

                ccimp_realloc_data.old_size = svc_cli->request_string_info.length;
                ccimp_realloc_data.ptr = svc_cli->request_string_info.string;
                if (ccimp_os_realloc(&ccimp_realloc_data) != CCIMP_STATUS_OK)
                {
                    ccapi_logging_line("ccapi_process_cli_request: error ccimp_os_realloc for %d bytes", ccimp_realloc_data.new_size);

                    svc_cli->cli_error = CCAPI_CLI_ERROR_INSUFFICIENT_MEMORY;
                    svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;
                    goto done;
                }
                svc_cli->request_string_info.string = ccimp_realloc_data.ptr;

                {
                    uint8_t * const dest_addr = (uint8_t *)svc_cli->request_string_info.string + svc_cli->request_string_info.length;
                    memcpy(dest_addr, cli_request_ptr->buffer, cli_request_ptr->bytes_used);
                }
                svc_cli->request_string_info.length += cli_request_ptr->bytes_used;
            }

            if (!cli_request_ptr->more_data)
            {
                svc_cli->cli_thread_status = CCAPI_CLI_THREAD_REQUEST_CB_READY;

                ccapi_logging_line("ccapi_process_cli_request. cli_thread_status=CCAPI_CLI_THREAD_REQUEST_CB_READY");

                connector_status = connector_callback_busy;
            }
            else
            {
                connector_status = connector_callback_continue;
            }

            break;
        }
        case CCAPI_CLI_THREAD_REQUEST_CB_READY:
        {
            if (ccapi_data->service.cli.svc_cli == NULL)
            {
                svc_cli->cli_thread_status = CCAPI_CLI_THREAD_REQUEST_CB_QUEUED;

                ccapi_data->service.cli.svc_cli = svc_cli;

                ccapi_logging_line("ccapi_process_cli_request. cli_thread_status=CCAPI_CLI_THREAD_REQUEST_CB_READY->CCAPI_CLI_THREAD_REQUEST_CB_QUEUED");

                ccapi_lock_release(ccapi_data->thread.cli->lock);
            }

            connector_status = connector_callback_busy;
            break;
        }
        case CCAPI_CLI_THREAD_REQUEST_CB_QUEUED:
        {
            connector_status = connector_callback_busy;
            break;
        }
        case CCAPI_CLI_THREAD_REQUEST_CB_PROCESSED:
        {
            ccapi_logging_line("ccapi_process_cli_request. cli_thread_status=CCAPI_CLI_THREAD_REQUEST_CB_PROCESSED");
            ccapi_free(svc_cli->request_string_info.string);

            if (svc_cli->response_required && svc_cli->response_string_info.string != NULL)
            {
                svc_cli->response_string_info.length = strlen(svc_cli->response_string_info.string);

                if (svc_cli->response_string_info.length != 0)
                {
                    svc_cli->response_string_info.length++; /* Add the null terminator */

                    memcpy(&svc_cli->response_processing, &svc_cli->response_string_info, sizeof svc_cli->response_processing);
                }
            }

            svc_cli->cli_thread_status = CCAPI_CLI_THREAD_FREE;

            connector_status = connector_callback_continue;

            break;
        }
        case CCAPI_CLI_THREAD_FREE:
            ASSERT(svc_cli->cli_thread_status != CCAPI_CLI_THREAD_FREE);
            break;
    }

done:
    return connector_status;
}

static void fill_internal_error(ccapi_svc_cli_t * svc_cli)
{
#define ERROR_MESSAGE "CCAPI Error %d (%s)"

        char const * cli_error_str = NULL;
        size_t cli_error_str_len = 0;

        switch (svc_cli->cli_error)
        {
#define ENUM_TO_CASE_ERR(name)  case name:  cli_error_str = #name; cli_error_str_len = sizeof #name - 1; break
            ENUM_TO_CASE_ERR(CCAPI_CLI_ERROR_NO_CLI_SUPPORT);
            ENUM_TO_CASE_ERR(CCAPI_CLI_ERROR_INSUFFICIENT_MEMORY);
#undef ENUM_TO_CASE_ERR

            case CCAPI_CLI_ERROR_NONE:
            case CCAPI_CLI_ERROR_STATUS_CANCEL:
            case CCAPI_CLI_ERROR_STATUS_ERROR:
            {
                static char const this_cli_error_str[] = "Unexpected error";
                cli_error_str = (char *)this_cli_error_str;
                cli_error_str_len = sizeof this_cli_error_str - 1;
                break;
            }
        }

        cli_error_str_len += sizeof ERROR_MESSAGE;

        if (!valid_cli_malloc((void**)&svc_cli->response_string_info.string, cli_error_str_len, &svc_cli->cli_error))
        {
              return;
        }
        svc_cli->response_string_info.length = sprintf(svc_cli->response_string_info.string, ERROR_MESSAGE,
                                                                        svc_cli->cli_error, cli_error_str);

        ccapi_logging_line("fill_internal_error: Providing response in buffer at %p: %s",
                                    svc_cli->response_string_info.string, (char*)svc_cli->response_string_info.string);
}

static connector_callback_status_t ccapi_process_cli_response(connector_sm_cli_response_t * const cli_response_ptr)
{
    ccapi_svc_cli_t * const svc_cli = cli_response_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_cli != NULL, done);

    ccapi_logging_line("ccapi_process_cli_response");

    if (!svc_cli->response_required)
    {
        goto done;
    }

    /* We initialize the response buffer for internal errors just once */
    if (svc_cli->cli_error != CCAPI_CLI_ERROR_NONE && !svc_cli->response_handled_internally)
    {
        svc_cli->response_handled_internally = CCAPI_TRUE;

        fill_internal_error(svc_cli);
        memcpy(&svc_cli->response_processing, &svc_cli->response_string_info, sizeof svc_cli->response_string_info);
    }

    {
        size_t const bytes_to_send = svc_cli->response_processing.length > cli_response_ptr->bytes_available ?
                                                 cli_response_ptr->bytes_available : svc_cli->response_processing.length;

        memcpy(cli_response_ptr->buffer, svc_cli->response_processing.string, bytes_to_send);
        svc_cli->response_processing.string = ((char *)svc_cli->response_processing.string) + bytes_to_send;

        cli_response_ptr->bytes_used = bytes_to_send;
        svc_cli->response_processing.length -= cli_response_ptr->bytes_used;
        cli_response_ptr->more_data = CCFSM_BOOL(svc_cli->response_processing.length > 0);
    }

    connector_status = connector_callback_continue;

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_cli_status(connector_sm_cli_status_t const * const status_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_svc_cli_t * const svc_cli = status_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_cli != NULL, done);

    ccapi_logging_line("ccapi_process_cli_status: ccapi_cli_error= %d,  status: %d", svc_cli->cli_error, status_ptr->status);

    /* Prior reported errors by ccapi have priority over the ones reported by the cloud */
    if (svc_cli->cli_error == CCAPI_CLI_ERROR_NONE)
    {
        switch (status_ptr->status)
        {
            case connector_sm_cli_status_success:
                svc_cli->cli_error = CCAPI_CLI_ERROR_NONE;
                break;
            case connector_sm_cli_status_cancel:
                svc_cli->cli_error = CCAPI_CLI_ERROR_STATUS_CANCEL;
                break;
            case connector_sm_cli_status_error:
                svc_cli->cli_error = CCAPI_CLI_ERROR_STATUS_ERROR;
                break;
        }
    }

    /* Call the user so he can free allocated response memory and handle errors  */
    if (ccapi_data->config.cli_supported && ccapi_data->service.cli.user_callback.finished != NULL)
    {
       ccapi_bool_t const should_user_free_response_buffer = !svc_cli->response_handled_internally && svc_cli->response_required && svc_cli->response_string_info.string != NULL;
       ccapi_data->service.cli.user_callback.finished(should_user_free_response_buffer ? svc_cli->response_string_info.string : NULL, svc_cli->cli_error);
    }

    /* Free resources */
    if (svc_cli->response_handled_internally == CCAPI_TRUE)
    {
        ccapi_logging_line("ccapi_process_cli_status: Freeing response buffer at %p", svc_cli->response_string_info.string);
        ccapi_free(svc_cli->response_string_info.string);
    }
    ccapi_free(svc_cli);

    connector_status = connector_callback_continue;

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_cli_response_length(connector_sm_cli_response_length_t * const length_ptr)
{
    ccapi_svc_cli_t const * const svc_cli = length_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_cli != NULL, done);

    length_ptr->total_bytes = svc_cli->response_processing.length;

    connector_status = connector_callback_continue;

done:
    return connector_status;
}
#endif /* (defined CONNECTOR_SM_CLI) */

static connector_callback_status_t ccapi_process_ping_response(connector_sm_ping_response_t const * const response_ptr)
{
    ccapi_svc_ping_t * const svc_ping = response_ptr->user_context;
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(svc_ping != NULL, done);

    ccapi_logging_line("ccapi_process_ping_response: %d", response_ptr->status);

    switch (response_ptr->status)
    {
        case connector_sm_ping_status_success:
        case connector_sm_ping_status_complete:
            svc_ping->response_error = CCAPI_PING_ERROR_NONE;
            break;
        case connector_sm_ping_status_cancel:
            svc_ping->response_error = CCAPI_PING_ERROR_RESPONSE_CANCEL;
            break;
        case connector_sm_ping_status_timeout:
            svc_ping->response_error = CCAPI_PING_ERROR_RESPONSE_TIMEOUT;
            break;
        case connector_sm_ping_status_error:
            svc_ping->response_error = CCAPI_PING_ERROR_RESPONSE_ERROR;
            break;
    }

    ASSERT_MSG_GOTO(svc_ping->ping_lock != NULL, done);

    switch (ccapi_lock_release(svc_ping->ping_lock))
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

static connector_callback_status_t ccapi_process_request_connect(connector_sm_request_connect_t * const request_connect_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("ccapi_process_request_connect: transport %d", request_connect_ptr->transport);

    if (ccapi_data->config.sm_supported && ccapi_data->service.sm.user_callback.request_connect != NULL)
    {
       ccapi_data->service.sm.user_callback.request_connect(request_connect_ptr->transport);
    }

    request_connect_ptr->allow = CCAPI_FALSE;

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_ping_request(connector_sm_receive_ping_request_t const * const ping_request_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("ccapi_process_ping_request: response %s needed", ping_request_ptr->response_required ? "is" : "is not");

    if (ccapi_data->config.sm_supported && ccapi_data->service.sm.user_callback.ping_request != NULL)
    {
        ccapi_data->service.sm.user_callback.ping_request(ping_request_ptr->transport, CCAPI_BOOL(ping_request_ptr->response_required));
    }

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_unsequenced_response(connector_sm_opaque_response_t const * const unsequenced_response_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("Received %" PRIsize " unsequenced bytes on id %d\n", unsequenced_response_ptr->bytes_used, unsequenced_response_ptr->id);

    if (ccapi_data->config.sm_supported && ccapi_data->service.sm.user_callback.unsequenced_response != NULL)
    {
        ccapi_data->service.sm.user_callback.unsequenced_response(unsequenced_response_ptr->transport, unsequenced_response_ptr->id, unsequenced_response_ptr->data, unsequenced_response_ptr->bytes_used, CCAPI_BOOL(unsequenced_response_ptr->error));
    }

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_pending_data(connector_sm_more_data_t const * const pending_data_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("ccapi_process_pending_data: transport %d", pending_data_ptr->transport);

    if (ccapi_data->config.sm_supported && ccapi_data->service.sm.user_callback.pending_data != NULL)
    {
       ccapi_data->service.sm.user_callback.pending_data(pending_data_ptr->transport);
    }

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_phone_provisioning(connector_sm_receive_config_request_t const * const phone_provisioning_ptr, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("ccapi_process_phone_provisioning: response %s needed", phone_provisioning_ptr->response_required ? "is" : "is not");
    ccapi_logging_line("phone-number=%s", phone_provisioning_ptr->phone_number);
    ccapi_logging_line("service-id=%s", phone_provisioning_ptr->service_id);

    if (ccapi_data->config.sm_supported && ccapi_data->service.sm.user_callback.phone_provisioning != NULL)
    {
       ccapi_data->service.sm.user_callback.phone_provisioning(phone_provisioning_ptr->transport, phone_provisioning_ptr->phone_number, phone_provisioning_ptr->service_id, phone_provisioning_ptr->response_required);
    }

    return connector_callback_continue;
}

/* Encryption is currently mandatory for SM, so for now use an simpler check */
#if 1
#define have_encryption(ccapi_data)             ((ccapi_data)->config.sm_supported)
#else
#define have_encryption(ccapi_data)             (((ccapi_data)->config.sm_supported) && ((ccapi_data)->service.sm.user_callback.encryption != NULL))
#endif

#define encryption_callback(ccapi_data, name)   ((ccapi_data)->service.sm.user_callback.encryption->name)

#define HUMAN_CASE(enum)   case enum: return #enum; break
static char const * human_sm_transport(ccapi_sm_transport_t const transport)
{
    switch (transport)
    {
        HUMAN_CASE(connector_sm_transport_sms);
        HUMAN_CASE(connector_sm_transport_satellite);
        HUMAN_CASE(connector_sm_transport_udp);
        HUMAN_CASE(connector_sm_transport_edp);
    }

    return "human_sm_transport UNKNOWN";
}

static char const * human_sm_encryption_data_type(connector_sm_encryption_data_type_t const type)
{
    switch (type)
    {
        HUMAN_CASE(connector_sm_encryption_data_type_current_key);
        HUMAN_CASE(connector_sm_encryption_data_type_previous_key);
        HUMAN_CASE(connector_sm_encryption_data_type_id);
        HUMAN_CASE(connector_sm_encryption_data_type_tracking);
    }

    return "human_sm_encryption_data_type UNKNOWN";
}

static connector_callback_status_t ccapi_process_load_data(connector_sm_encryption_load_data_t * const load_data, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("ccapi_process_load_data: transport=%s, type=%s, bytes-required=%d",
        human_sm_transport(load_data->transport), human_sm_encryption_data_type(load_data->type), load_data->bytes_required);

    if (!have_encryption(ccapi_data))
    {
        return connector_callback_unrecognized;
    }

    if (!encryption_callback(ccapi_data, load_data)(load_data->transport, load_data->type, load_data->data, load_data->bytes_required))
    {
        return connector_callback_error;
    }

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_store_data(connector_sm_encryption_store_data_t const * const store_data, ccapi_data_t * const ccapi_data)
{
    ccapi_logging_line("ccapi_process_store_data: transport=%s, type=%s, bytes-used=%d",
        human_sm_transport(store_data->transport), human_sm_encryption_data_type(store_data->type), store_data->bytes_used);


    if (!have_encryption(ccapi_data))
    {
        return connector_callback_unrecognized;
    }

    if (!encryption_callback(ccapi_data, store_data)(store_data->transport, store_data->type, store_data->data, store_data->bytes_used))
    {
        return connector_callback_error;
    }

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_encrypt_gcm(connector_sm_encrypt_gcm_t * const encrypt_gcm, ccapi_data_t * const ccapi_data)
{
    ccapi_sm_encrypt_gcm_t * const encrypt_gcm_transformed = (ccapi_sm_encrypt_gcm_t *) encrypt_gcm;

    ccapi_logging_line("ccapi_process_encrypt_gcm: message-length=%d", encrypt_gcm_transformed->message.length);

    if (!have_encryption(ccapi_data))
    {
        return connector_callback_unrecognized;
    }

    if (!encryption_callback(ccapi_data, encrypt_gcm)(encrypt_gcm_transformed))
    {
        return connector_callback_error;
    }

    return connector_callback_continue;
}

static connector_callback_status_t ccapi_process_decrypt_gcm(connector_sm_decrypt_gcm_t * const decrypt_gcm, ccapi_data_t * const ccapi_data)
{
    ccapi_sm_decrypt_gcm_t * const decrypt_gcm_transformed = (ccapi_sm_decrypt_gcm_t *) decrypt_gcm;

    ccapi_logging_line("ccapi_process_decrypt_gcm: message-length=%d", decrypt_gcm_transformed->message.length);

    if (!have_encryption(ccapi_data))
    {
        return connector_callback_unrecognized;
    }

    if (!encryption_callback(ccapi_data, decrypt_gcm)(decrypt_gcm_transformed))
    {
        return connector_callback_error;
    }

    return connector_callback_continue;
}

connector_callback_status_t ccapi_sm_service_handler(connector_request_id_sm_t const sm_service_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_unrecognized;

    switch (sm_service_request)
    {
#if (defined CONNECTOR_SM_CLI)
        case connector_request_id_sm_cli_request:
        {
            connector_sm_cli_request_t * const cli_request_ptr = data;

            connector_status = ccapi_process_cli_request(cli_request_ptr, ccapi_data);

            break;
        }
        case connector_request_id_sm_cli_response:
        {
            connector_sm_cli_response_t * const cli_response_ptr = data;

            connector_status = ccapi_process_cli_response(cli_response_ptr);

            break;
        }
        case connector_request_id_sm_cli_status:
        {
            connector_sm_cli_status_t const * const status_ptr = data;

            connector_status = ccapi_process_cli_status(status_ptr, ccapi_data);

            break;
        }
        case connector_request_id_sm_cli_response_length:
        {
            connector_sm_cli_response_length_t * const length_ptr = data;

            connector_status = ccapi_process_cli_response_length(length_ptr);

            break;
        }
#endif

        case connector_request_id_sm_ping_request:
        {
            connector_sm_receive_ping_request_t const * const ping_request_ptr = data;

            connector_status = ccapi_process_ping_request(ping_request_ptr, ccapi_data);
            break;
        }
        case connector_request_id_sm_ping_response:
        {
            connector_sm_ping_response_t const * const ping_response_ptr = data;

            connector_status = ccapi_process_ping_response(ping_response_ptr);

            break;
        }
        case connector_request_id_sm_opaque_response:
        {
            connector_sm_opaque_response_t const * const unsequenced_response = data;

            connector_status = ccapi_process_unsequenced_response(unsequenced_response, ccapi_data);

            break;
        }
        case connector_request_id_sm_more_data:
        {
            connector_sm_more_data_t const * const pending_data_ptr = data;

            connector_status = ccapi_process_pending_data(pending_data_ptr, ccapi_data);

            break;
        }
        case connector_request_id_sm_config_request:
        {
            connector_sm_receive_config_request_t const * const phone_provisioning_ptr = data;

            connector_status = ccapi_process_phone_provisioning(phone_provisioning_ptr, ccapi_data);

            break;
        }
        case connector_request_id_sm_request_connect:
        {
            connector_sm_request_connect_t * const request_connect_ptr = data;

            connector_status = ccapi_process_request_connect(request_connect_ptr, ccapi_data);

            break;
         }

        case connector_request_id_sm_encryption_load_data:
        {
            connector_sm_encryption_load_data_t * const ptr = data;

            connector_status = ccapi_process_load_data(ptr, ccapi_data);
			
			// hack to allow loads to fail with unrecognized
			goto done;

            break;
        }

        case connector_request_id_sm_encryption_store_data:
        {
            connector_sm_encryption_store_data_t const * const ptr = data;

            connector_status = ccapi_process_store_data(ptr, ccapi_data);

            break;
        }

        case connector_request_id_sm_encrypt_gcm:
        {
            connector_sm_encrypt_gcm_t * const ptr = data;

            connector_status = ccapi_process_encrypt_gcm(ptr, ccapi_data);

            break;
        }

        case connector_request_id_sm_decrypt_gcm:
        {
            connector_sm_decrypt_gcm_t * const ptr = data;

            connector_status = ccapi_process_decrypt_gcm(ptr, ccapi_data);

            break;
        }
    }

    ASSERT_MSG_GOTO(connector_status != connector_callback_unrecognized, done);

done:
    return connector_status;
}
#endif
