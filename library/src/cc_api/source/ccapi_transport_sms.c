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

#include "ccapi_definitions.h"
#include "ccapi/ccapi_transport_sms.h"

#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
static ccapi_bool_t valid_malloc(void const * const ptr, ccapi_sms_start_error_t * const error)
{
    if (ptr == NULL)
    {
        *error = CCAPI_SMS_START_ERROR_INSUFFICIENT_MEMORY;
        return CCAPI_FALSE;
    }
    else
    {
        return CCAPI_TRUE;
    }
}

static ccapi_bool_t valid_phone(ccapi_sms_info_t const * const sms_start, ccapi_sms_start_error_t * const error)
{
    ccapi_bool_t success = CCAPI_TRUE;
    static char const * const valid_char = " 0123456789-+#";
    int i = 0;

    if (sms_start->cloud_config.phone_number == NULL || sms_start->cloud_config.phone_number[0] == '\0')
    {
        ccapi_logging_line("ccxapi_start_transport_sms: invalid Phone number");
        *error = CCAPI_SMS_START_ERROR_INVALID_PHONE;
        success = CCAPI_FALSE;
        goto done;
    }

    while (sms_start->cloud_config.phone_number[i] != '\0')
    {
        if(strchr(valid_char, sms_start->cloud_config.phone_number[i]) == NULL)
        {
            ccapi_logging_line("ccxapi_start_transport_sms: invalid Phone number character '%c'",sms_start->cloud_config.phone_number[i]);
            *error = CCAPI_SMS_START_ERROR_INVALID_PHONE;
            success = CCAPI_FALSE;
            goto done;
        }
        i++;
    }

done:
    return success;
}

static ccapi_bool_t valid_service_id(ccapi_sms_info_t const * const sms_start, ccapi_sms_start_error_t * const error)
{
    if (sms_start->cloud_config.service_id == NULL)
    {
        ccapi_logging_line("ccxapi_start_transport_sms: invalid Service Id");
        *error = CCAPI_SMS_START_ERROR_INVALID_SERVICE_ID;
        return CCAPI_FALSE;
    }
    else
    {
        return CCAPI_TRUE;
    }
}

static ccapi_bool_t copy_ccapi_sms_info_t_structure(ccapi_sms_info_t * const dest, ccapi_sms_info_t const * const source , ccapi_sms_start_error_t * const error)
{
    ccapi_bool_t success = CCAPI_TRUE;
    *dest = *source;

    if (dest->limit.max_sessions == 0)
    {
        dest->limit.max_sessions = CCAPI_SM_SMS_MAX_SESSIONS_DEFAULT;
    }

    dest->cloud_config.phone_number = NULL;
    dest->cloud_config.service_id = NULL;

    if (source->cloud_config.phone_number != NULL)
    {
        dest->cloud_config.phone_number = ccapi_strdup(source->cloud_config.phone_number);
        if (!valid_malloc(dest->cloud_config.phone_number, error))
        {
            success = CCAPI_FALSE;
            goto done;
        }
    }
    if (source->cloud_config.service_id != NULL)
    {
        dest->cloud_config.service_id = ccapi_strdup(source->cloud_config.service_id);
        if (!valid_malloc(dest->cloud_config.service_id, error))
        {
            success = CCAPI_FALSE;
            goto done;
        }
    }
done:
    return success;
}

ccapi_sms_start_error_t ccxapi_start_transport_sms(ccapi_data_t * const ccapi_data, ccapi_sms_info_t const * const sms_start)
{
    ccapi_sms_start_error_t error = CCAPI_SMS_START_ERROR_NONE;

    if (!CCAPI_RUNNING(ccapi_data))
    {
        ccapi_logging_line("ccxapi_start_transport_sms: CCAPI not started");

        error = CCAPI_SMS_START_ERROR_CCAPI_STOPPED;
        goto done;
    }

    if (ccapi_data->transport_sms.started)
    {
        error = CCAPI_SMS_START_ERROR_ALREADY_STARTED;
        goto done;
    }

    if (sms_start == NULL)
    {
        ccapi_logging_line("ccxapi_start_transport_sms: invalid argument");
        error = CCAPI_SMS_START_ERROR_NULL_POINTER;
        goto done;
    }

    if (sms_start->limit.max_sessions > CCAPI_SM_SMS_MAX_SESSIONS_LIMIT)
    {
        ccapi_logging_line("ccxapi_start_transport_sms: invalid argument MAX SESSIONS");
        error = CCAPI_SMS_START_ERROR_MAX_SESSIONS;
        goto done;
    }
    if (!valid_phone(sms_start, &error))
    {
        goto done;
    }
    if (!valid_service_id(sms_start, &error))
    {
        goto done;
    }

    if (ccapi_data->transport_sms.info)
    {
        free_transport_sms_info(ccapi_data->transport_sms.info);
    }
    ccapi_data->transport_sms.info = ccapi_malloc(sizeof *ccapi_data->transport_sms.info);
    if (!valid_malloc(ccapi_data->transport_sms.info, &error))
    {
        goto done;
    }

    if(!copy_ccapi_sms_info_t_structure(ccapi_data->transport_sms.info, sms_start,&error))
    {
        goto done;
    }

    ccapi_data->transport_sms.started = CCAPI_FALSE;

    {
        connector_transport_t const transport = connector_transport_sms;
        connector_status_t ccfsm_status;

        for (;;)
        {
            ccfsm_status = connector_initiate_action_secure(ccapi_data, connector_initiate_transport_start, &transport);
            if (ccfsm_status != connector_service_busy)
            {
                break;
            }
            ccimp_os_yield();
        }

        switch (ccfsm_status)
        {
            case connector_success:
                break;
            case connector_init_error:
            case connector_invalid_data:
            case connector_service_busy:
            case connector_invalid_data_size:
            case connector_invalid_data_range:
            case connector_keepalive_error:
            case connector_bad_version:
            case connector_device_terminated:
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
                error = CCAPI_SMS_START_ERROR_INIT;
                ASSERT_MSG_GOTO(ccfsm_status == connector_success, done);
                break;
        }
    }
    {
        ccapi_bool_t const wait_forever = CCAPI_BOOL(sms_start->start_timeout == CCAPI_SMS_START_WAIT_FOREVER);

        if (wait_forever)
        {
            do {
                ccimp_os_yield();
            } while (!ccapi_data->transport_sms.started);
        }
        else
        {
            ccimp_os_system_up_time_t time_start;
            ccimp_os_system_up_time_t end_time;
            unsigned long const jitter = 1;

            ccimp_os_get_system_time(&time_start);
            end_time.sys_uptime = time_start.sys_uptime + sms_start->start_timeout + jitter;
            do {
                ccimp_os_system_up_time_t system_uptime;

                ccimp_os_yield();
                ccimp_os_get_system_time(&system_uptime);
                if (system_uptime.sys_uptime > end_time.sys_uptime)
                {
                    error = CCAPI_SMS_START_ERROR_TIMEOUT;
                    goto done;
                }
            } while (!ccapi_data->transport_sms.started);
        }
    }
done:
    switch (error)
    {
        case CCAPI_SMS_START_ERROR_NONE:
        case CCAPI_SMS_START_ERROR_ALREADY_STARTED:
        case CCAPI_SMS_START_ERROR_CCAPI_STOPPED:
        case CCAPI_SMS_START_ERROR_NULL_POINTER:
            break;
        case CCAPI_SMS_START_ERROR_INIT:
        case CCAPI_SMS_START_ERROR_MAX_SESSIONS:
        case CCAPI_SMS_START_ERROR_INVALID_PHONE:
        case CCAPI_SMS_START_ERROR_INVALID_SERVICE_ID:
        case CCAPI_SMS_START_ERROR_INSUFFICIENT_MEMORY:
        case CCAPI_SMS_START_ERROR_TIMEOUT:
            if (ccapi_data->transport_sms.info != NULL)
            {
                if (ccapi_data->transport_sms.info->cloud_config.phone_number != NULL)
                {
                    ccapi_free_const(ccapi_data->transport_sms.info->cloud_config.phone_number);
                }
                if (ccapi_data->transport_sms.info->cloud_config.service_id != NULL)
                {
                    ccapi_free_const(ccapi_data->transport_sms.info->cloud_config.service_id);
                }
                ccapi_free(ccapi_data->transport_sms.info);
                ccapi_data->transport_sms.info = NULL;
            }
            break;
    }
    return error;
}

void free_transport_sms_info(ccapi_sms_info_t * const sms_info)
{
    if (sms_info->cloud_config.phone_number != NULL)
    {
        ccapi_free_const(sms_info->cloud_config.phone_number);
    }
    if (sms_info->cloud_config.service_id != NULL)
    {
        ccapi_free_const(sms_info->cloud_config.service_id);
    }
    ccapi_free(sms_info);
}

ccapi_sms_stop_error_t ccxapi_stop_transport_sms(ccapi_data_t * const ccapi_data, ccapi_sms_stop_t const * const sms_stop)
{
    ccapi_sms_stop_error_t error = CCAPI_SMS_STOP_ERROR_NONE;
    connector_status_t connector_status;

    if (!CCAPI_RUNNING(ccapi_data) || !ccapi_data->transport_sms.started)
    {
        error = CCAPI_SMS_STOP_ERROR_NOT_STARTED;
        goto done;
    }

    connector_status = ccapi_initiate_transport_stop(ccapi_data, CCAPI_TRANSPORT_SMS, sms_stop->behavior);
    if (connector_status != connector_success)
    {
        error = CCAPI_SMS_STOP_ERROR_CCFSM;
        goto done;
    }

    do {
        ccimp_os_yield();
    } while (ccapi_data->transport_sms.started);

    ASSERT_MSG_GOTO(ccapi_data->transport_sms.info != NULL, done);
    free_transport_sms_info(ccapi_data->transport_sms.info);
    ccapi_data->transport_sms.info = NULL;

done:
    return error;
}

ccapi_sms_start_error_t ccapi_start_transport_sms(ccapi_sms_info_t const * const sms_start)
{
    return ccxapi_start_transport_sms(ccapi_data_single_instance, sms_start);
}

ccapi_sms_stop_error_t ccapi_stop_transport_sms(ccapi_sms_stop_t const * const sms_stop)
{
    return ccxapi_stop_transport_sms(ccapi_data_single_instance, sms_stop);
}
#endif
