/*
Copyright 2017-2019, Digi International Inc.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "ccapi_definitions.h"
#include "ccapi/ccapi_transport_tcp.h"

static ccapi_bool_t valid_keepalives(ccapi_tcp_info_t const * const tcp_start, ccapi_tcp_start_error_t * const error)
{
    ccapi_bool_t success = CCAPI_TRUE;

    if (tcp_start->keepalives.rx != 0)
    {
        if (tcp_start->keepalives.rx > CCAPI_KEEPALIVES_RX_MAX || tcp_start->keepalives.rx < CCAPI_KEEPALIVES_RX_MIN)
        {
            success = CCAPI_FALSE;
            goto done;
        }
    }

    if (tcp_start->keepalives.tx != 0)
    {
        if (tcp_start->keepalives.tx > CCAPI_KEEPALIVES_TX_MAX || tcp_start->keepalives.tx < CCAPI_KEEPALIVES_TX_MIN)
        {
            success = CCAPI_FALSE;
            goto done;
        }
    }

    if (tcp_start->keepalives.wait_count != 0)
    {
        if (tcp_start->keepalives.wait_count > CCAPI_KEEPALIVES_WCNT_MAX || tcp_start->keepalives.wait_count < CCAPI_KEEPALIVES_WCNT_MIN)
        {
            success = CCAPI_FALSE;
            goto done;
        }
    }

done:
    switch(success)
    {
        case CCAPI_FALSE:
        {
            ccapi_logging_line("ccxapi_start_transport_tcp: invalid keepalive configuration");
            *error = CCAPI_TCP_START_ERROR_KEEPALIVES;
            break;
        }
        case CCAPI_TRUE:
        {
            break;
        }
    }

    return success;
}

static ccapi_bool_t valid_connection(ccapi_tcp_info_t const * const tcp_start, ccapi_tcp_start_error_t * const error)
{
    ccapi_bool_t success = CCAPI_TRUE;

    switch (tcp_start->connection.ip.type)
    {
        case CCAPI_IPV4:
        {
            static uint8_t const invalid_ipv4[] = {0x00, 0x00, 0x00, 0x00};

            if (memcmp(tcp_start->connection.ip.address.ipv4, invalid_ipv4, sizeof tcp_start->connection.ip.address.ipv4) == 0)
            {
                ccapi_logging_line("ccxapi_start_transport_tcp: invalid IPv4");
                *error = CCAPI_TCP_START_ERROR_IP;
                success = CCAPI_FALSE;
                goto done;
            }
            break;
        }
        case CCAPI_IPV6:
        {
            static uint8_t const invalid_ipv6[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            if (memcmp(tcp_start->connection.ip.address.ipv6, invalid_ipv6, sizeof tcp_start->connection.ip.address.ipv6) == 0)
            {
                ccapi_logging_line("ccxapi_start_transport_tcp: invalid IPv6");
                *error = CCAPI_TCP_START_ERROR_IP;
                success = CCAPI_FALSE;
                goto done;
            }
            break;
        }
    }

    switch (tcp_start->connection.type)
    {
        case CCAPI_CONNECTION_LAN:
        {
            static uint8_t const invalid_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            if (memcmp(invalid_mac, tcp_start->connection.info.lan.mac_address, sizeof invalid_mac) == 0)
            {
                success = CCAPI_FALSE;
                *error = CCAPI_TCP_START_ERROR_INVALID_MAC;
                goto done;
            }
            break;
        }
        case CCAPI_CONNECTION_WAN:
        {
            if (tcp_start->connection.info.wan.phone_number == NULL)
            {
                ccapi_logging_line("ccxapi_start_transport_tcp: invalid Phone number");
                *error = CCAPI_TCP_START_ERROR_PHONE;
                success = CCAPI_FALSE;
                goto done;
            }
        }
            break;
    }
done:
    return success;
}

static ccapi_bool_t valid_malloc(void * ptr, ccapi_tcp_start_error_t * const error)
{
    if (ptr == NULL)
    {
        *error = CCAPI_TCP_START_ERROR_INSUFFICIENT_MEMORY;
        return CCAPI_FALSE;
    }
    else
    {
        return CCAPI_TRUE;
    }
}

static ccapi_bool_t copy_wan_info(ccapi_tcp_info_t * const dest, ccapi_tcp_info_t const * const source, ccapi_tcp_start_error_t * const error)
{
    ccapi_bool_t success = CCAPI_TRUE;

    if (source->connection.info.wan.phone_number != NULL)
    {
        dest->connection.info.wan.phone_number = ccapi_malloc(strlen(source->connection.info.wan.phone_number) + 1);
        if (!valid_malloc(dest->connection.info.wan.phone_number, error))
        {
            success = CCAPI_FALSE;
            goto done;
        }
        strcpy(dest->connection.info.wan.phone_number, source->connection.info.wan.phone_number);
    }

done:
    return success;
}

static ccapi_bool_t copy_ccapi_tcp_info_t_structure(ccapi_tcp_info_t * const dest, ccapi_tcp_info_t const * const source, ccapi_tcp_start_error_t * const error)
{
    ccapi_bool_t success = CCAPI_TRUE;
    *dest = *source; /* Strings and pointers to buffer need to be copied manually to allocated spaces. */

    if (dest->keepalives.rx == 0)
    {
        dest->keepalives.rx = CCAPI_KEEPALIVES_RX_DEFAULT;
    }

    if (dest->keepalives.tx == 0)
    {
        dest->keepalives.tx = CCAPI_KEEPALIVES_TX_DEFAULT;
    }

    if (dest->keepalives.wait_count == 0)
    {
        dest->keepalives.wait_count = CCAPI_KEEPALIVES_WCNT_DEFAULT;
    }

    if (source->connection.password != NULL)
    {
        dest->connection.password = ccapi_malloc(strlen(source->connection.password) + 1);
        if (!valid_malloc(dest->connection.password, error))
        {
            success = CCAPI_FALSE;
            goto done;
        }
        strcpy(dest->connection.password, source->connection.password);
    }

    if (source->connection.type == CCAPI_CONNECTION_WAN)
    {
        success = copy_wan_info(dest, source, error);
    }
    /* No buffers to copy if CCAPI_CONNECTION_LAN */

done:
    return success;
}

void free_transport_tcp_info(ccapi_tcp_info_t * const tcp_info)
{
    if (tcp_info->connection.password != NULL)
    {
        ccapi_free(tcp_info->connection.password);
    }

    if (tcp_info->connection.type == CCAPI_CONNECTION_WAN && tcp_info->connection.info.wan.phone_number != NULL)
    {
        ccapi_free(tcp_info->connection.info.wan.phone_number);
    }
    ccapi_free(tcp_info);
}

ccapi_tcp_start_error_t ccxapi_start_transport_tcp(ccapi_data_t * const ccapi_data, ccapi_tcp_info_t const * const tcp_start)
{
    ccapi_tcp_start_error_t error = CCAPI_TCP_START_ERROR_NONE;

    if (!CCAPI_RUNNING(ccapi_data))
    {
        ccapi_logging_line("ccxapi_start_transport_tcp: CCAPI not started");

        error = CCAPI_TCP_START_ERROR_CCAPI_STOPPED;
        goto done;
    }

    if (ccapi_data->transport_tcp.connected)
    {
        error = CCAPI_TCP_START_ERROR_ALREADY_STARTED;
        goto done;
    }

    if (tcp_start == NULL)
    {
        ccapi_logging_line("ccxapi_start_transport_tcp: invalid argument");
        error = CCAPI_TCP_START_ERROR_NULL_POINTER;
        goto done;
    }

    if (ccapi_data->transport_tcp.info)
    {
        free_transport_tcp_info(ccapi_data->transport_tcp.info);
    }
    ccapi_data->transport_tcp.info = ccapi_malloc(sizeof *ccapi_data->transport_tcp.info);
    if (!valid_malloc(ccapi_data->transport_tcp.info, &error))
    {
        goto done;
    }

    ccapi_data->transport_tcp.info->connection.password = NULL;


    if (tcp_start->connection.type == CCAPI_CONNECTION_WAN)
    {
        ccapi_data->transport_tcp.info->connection.info.wan.phone_number = NULL;
    }

    if (!valid_keepalives(tcp_start, &error))
    {
        goto done;
    }

    if (!valid_connection(tcp_start, &error))
    {
        goto done;
    }

    if (copy_ccapi_tcp_info_t_structure(ccapi_data->transport_tcp.info, tcp_start, &error) != CCAPI_TRUE)
    {
        goto done;
    }

    ccapi_data->transport_tcp.connected = CCAPI_FALSE;

    {
        connector_transport_t const transport = connector_transport_tcp;
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
                error = CCAPI_TCP_START_ERROR_INIT;
                ASSERT_MSG_GOTO(ccfsm_status == connector_success, done);
                break;
        }
    }

    {
        ccapi_bool_t const wait_forever = CCAPI_BOOL(tcp_start->connection.start_timeout == CCAPI_TCP_START_WAIT_FOREVER);

        if (wait_forever)
        {
            do {
                ccimp_os_yield();
            } while (!ccapi_data->transport_tcp.connected);
        }
        else
        {
            ccimp_os_system_up_time_t time_start;
            ccimp_os_system_up_time_t end_time;
            unsigned long const jitter = 1;

            ccimp_os_get_system_time(&time_start);
            end_time.sys_uptime = time_start.sys_uptime + tcp_start->connection.start_timeout + jitter;
            do {
                ccimp_os_system_up_time_t system_uptime;

                ccimp_os_yield();
                ccimp_os_get_system_time(&system_uptime);
                if (system_uptime.sys_uptime > end_time.sys_uptime)
                {
                    error = CCAPI_TCP_START_ERROR_TIMEOUT;
                    goto done;
                }
            } while (!ccapi_data->transport_tcp.connected);
        }
    }
done:
    switch (error)
    {
        case CCAPI_TCP_START_ERROR_NONE:
        case CCAPI_TCP_START_ERROR_ALREADY_STARTED:
        case CCAPI_TCP_START_ERROR_CCAPI_STOPPED:
        case CCAPI_TCP_START_ERROR_NULL_POINTER:
            break;
        case CCAPI_TCP_START_ERROR_INSUFFICIENT_MEMORY:
        case CCAPI_TCP_START_ERROR_KEEPALIVES:
        case CCAPI_TCP_START_ERROR_IP:
        case CCAPI_TCP_START_ERROR_INVALID_MAC:
        case CCAPI_TCP_START_ERROR_PHONE:
        case CCAPI_TCP_START_ERROR_INIT:
        case CCAPI_TCP_START_ERROR_TIMEOUT:
            if (ccapi_data->transport_tcp.info != NULL)
            {
                free_transport_tcp_info(ccapi_data->transport_tcp.info);
                ccapi_data->transport_tcp.info = NULL;
            }
            break;
    }

    return error;
}

ccapi_tcp_stop_error_t ccxapi_stop_transport_tcp(ccapi_data_t * const ccapi_data, ccapi_tcp_stop_t const * const tcp_stop)
{
    ccapi_tcp_stop_error_t error = CCAPI_TCP_STOP_ERROR_NONE;
    connector_status_t connector_status;

    if (!CCAPI_RUNNING(ccapi_data) || !ccapi_data->transport_tcp.connected)
    {
        error = CCAPI_TCP_STOP_ERROR_NOT_STARTED;
        goto done;
    }

    connector_status = ccapi_initiate_transport_stop(ccapi_data, CCAPI_TRANSPORT_TCP, tcp_stop->behavior);
    if (connector_status != connector_success)
    {
        error = CCAPI_TCP_STOP_ERROR_CCFSM;
        goto done;
    }

    do {
        ccimp_os_yield();
    } while (ccapi_data->transport_tcp.connected);

    ASSERT_MSG_GOTO(ccapi_data->transport_tcp.info != NULL, done);
    free_transport_tcp_info(ccapi_data->transport_tcp.info);
    ccapi_data->transport_tcp.info = NULL;
done:
    return error;
}


ccapi_tcp_start_error_t ccapi_start_transport_tcp(ccapi_tcp_info_t const * const tcp_start)
{
    return ccxapi_start_transport_tcp(ccapi_data_single_instance, tcp_start);
}

ccapi_tcp_stop_error_t ccapi_stop_transport_tcp(ccapi_tcp_stop_t const * const tcp_stop)
{
    return ccxapi_stop_transport_tcp(ccapi_data_single_instance, tcp_stop);
}
