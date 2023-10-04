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

typedef struct
{
    connector_sm_send_ping_request_t header;
    ccapi_svc_ping_t svc_ping;
} ccapi_ping_t;

static ccapi_bool_t valid_malloc(void * * const ptr, size_t const size, ccapi_ping_error_t * const error)
{
    ccapi_bool_t success;

    *ptr = ccapi_malloc(size);

    success = CCAPI_BOOL(*ptr != NULL);

    if (!success)
    {
        *error = CCAPI_PING_ERROR_INSUFFICIENT_MEMORY;
    }

    return success;
}

static ccimp_status_t ccapi_send_ping_lock_acquire(ccapi_ping_t const * const ping_info, unsigned long const timeout_ms)
{
    ccimp_os_lock_acquire_t acquire_data;
    ccimp_status_t status = CCIMP_STATUS_ERROR;

    ASSERT_MSG_GOTO(ping_info->svc_ping.ping_lock != NULL, done);

    acquire_data.lock = ping_info->svc_ping.ping_lock;
    acquire_data.timeout_ms= timeout_ms;

    status = ccimp_os_lock_acquire(&acquire_data);

    if (status == CCIMP_STATUS_OK && acquire_data.acquired != CCAPI_TRUE)
        status = CCIMP_STATUS_ERROR;

done:
    return status;
}

static ccapi_ping_error_t checkargs_send_ping(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport)
{
    ccapi_ping_error_t error = CCAPI_PING_ERROR_NONE;
    ccapi_bool_t const * p_transport_started = NULL;

    if (!CCAPI_RUNNING(ccapi_data))
    {
        ccapi_logging_line("checkargs_send_ping: CCAPI not started");

        error = CCAPI_PING_ERROR_CCAPI_NOT_RUNNING;
        goto done;
    }

    switch (transport)
    {
        case CCAPI_TRANSPORT_TCP:
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

    if (p_transport_started == NULL)
    {
        ccapi_logging_line("checkargs_send_ping: Transport not valid");

        error = CCAPI_PING_ERROR_TRANSPORT_NOT_VALID;
        goto done;
    }

    if (!*p_transport_started)
    {
        ccapi_logging_line("checkargs_send_ping: Transport not started");

        error = CCAPI_PING_ERROR_TRANSPORT_NOT_STARTED;
        goto done;
    }

done:
    return error;
}

static ccapi_ping_error_t setup_send_ping(ccapi_ping_t * const ping_info, ccapi_transport_t const transport)
{
    ccapi_ping_error_t error = CCAPI_PING_ERROR_NONE;
    ccimp_os_lock_create_t create_data;

    if (ccimp_os_lock_create(&create_data) != CCIMP_STATUS_OK)
    {
        error = CCAPI_PING_ERROR_LOCK_FAILED;
        goto done;
    }

    ping_info->svc_ping.ping_lock = create_data.lock;

    ping_info->header.transport = ccapi_to_connector_transport(transport);
    ping_info->header.request_id = NULL;

    ping_info->svc_ping.response_error = CCAPI_PING_ERROR_NONE;
    ping_info->header.user_context = &ping_info->svc_ping;

done:
    return error;
}

static ccapi_ping_error_t perform_send_ping(ccapi_data_t * const ccapi_data, ccapi_ping_t * const ping_info)
{
    connector_status_t ccfsm_status;
    ccapi_ping_error_t error = CCAPI_PING_ERROR_NONE;

    for (;;)
    {
        ccfsm_status = connector_initiate_action_secure(ccapi_data, connector_initiate_ping_request, &ping_info->header);
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
            ccapi_logging_line("perform_send_ping: ccfsm error %d", ccfsm_status);
            error = CCAPI_PING_ERROR_INITIATE_ACTION_FAILED;
            goto done;
    }
    {
        ccimp_status_t const result = ccapi_send_ping_lock_acquire(ping_info, OS_LOCK_ACQUIRE_INFINITE);

        if (result != CCIMP_STATUS_OK)
        {
            ccapi_logging_line("perform_send_ping: lock_acquire failed");
            error = CCAPI_PING_ERROR_LOCK_FAILED;
        }
    }
done:
    return error;
}

static void finish_send_ping(ccapi_ping_t * const ping_info)
{
    /* Free resources */
    if (ping_info != NULL)
    {
        if (ping_info->svc_ping.ping_lock != NULL)
        {
            ccimp_status_t const ccimp_status = ccapi_lock_destroy(ping_info->svc_ping.ping_lock);
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

        ccapi_free(ping_info);
    }
}

ccapi_ping_error_t ccxapi_send_ping_common(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, ccapi_bool_t const with_reply, unsigned long const timeout)
{
    ccapi_ping_error_t error;
    ccapi_ping_t * ping_info = NULL;

    error = checkargs_send_ping(ccapi_data, transport);
    if (error != CCAPI_PING_ERROR_NONE)
    {
        goto done;
    }

    if (!valid_malloc((void**)&ping_info, sizeof *ping_info, &error))
    {
        goto done;
    }

    error = setup_send_ping(ping_info, transport);
    if (error != CCAPI_PING_ERROR_NONE)
    {
        goto done;
    }

    ping_info->header.response_required = CCAPI_BOOL_TO_CONNECTOR_BOOL(with_reply);
    ping_info->header.timeout_in_seconds = timeout;

    error = perform_send_ping(ccapi_data, ping_info);
    if (error != CCAPI_PING_ERROR_NONE)
    {
        goto done;
    }

    error = ping_info->svc_ping.response_error;

done:
    finish_send_ping(ping_info);

    return error;
}

ccapi_ping_error_t ccxapi_send_ping(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport)
{
    return ccxapi_send_ping_common(ccapi_data, transport, CCAPI_FALSE, CCAPI_SEND_PING_WAIT_FOREVER);
}

ccapi_ping_error_t ccapi_send_ping(ccapi_transport_t const transport)
{
    return ccxapi_send_ping_common(ccapi_data_single_instance, transport, CCAPI_FALSE, CCAPI_SEND_PING_WAIT_FOREVER);
}

ccapi_ping_error_t ccxapi_send_ping_with_reply(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, unsigned long const timeout)
{
    return ccxapi_send_ping_common(ccapi_data, transport, CCAPI_TRUE, timeout);
}

ccapi_ping_error_t ccapi_send_ping_with_reply(ccapi_transport_t const transport, unsigned long const timeout)
{
    return ccxapi_send_ping_common(ccapi_data_single_instance, transport, CCAPI_TRUE, timeout);
}
#endif
