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

ccapi_data_t * ccapi_data_single_instance = NULL;

void * ccapi_malloc(size_t const size)
{
    ccimp_os_malloc_t malloc_info;
    ccimp_status_t status;

    malloc_info.size = size;
    status = ccimp_os_malloc(&malloc_info);

    switch (status)
    {
        case CCIMP_STATUS_OK:
            return malloc_info.ptr;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            break;
    }
    return NULL;
}

ccimp_status_t ccapi_free(void * const ptr)
{
    ccimp_os_free_t free_info;
    ccimp_status_t ccimp_status = CCIMP_STATUS_OK;

    if (ptr == NULL)
    {
        goto done;
    }

    free_info.ptr = ptr;
    ccimp_status = ccimp_os_free(&free_info);

done:
    return ccimp_status;
}


void * ccapi_lock_create(void)
{
    ccimp_os_lock_create_t create_data;
    ccimp_status_t const ccimp_status = ccimp_os_lock_create(&create_data);

    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            ccapi_logging_line("ccapi_lock_create() failed!");
            create_data.lock = NULL;
            break;
    }

    return create_data.lock;
}

void * ccapi_lock_create_and_release(void)
{
    ccimp_status_t ccimp_status;
    void * lock = ccapi_lock_create();

    if (lock == NULL)
    {
        goto done;
    }

    ccimp_status = ccapi_lock_release(lock);
    switch(ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            lock = NULL;
            goto done;
    }

done:
    return lock;
}

ccimp_status_t ccapi_lock_acquire(void * const lock)
{
    ccimp_os_lock_acquire_t acquire_data;
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    ASSERT_MSG_GOTO(lock != NULL, done);
    acquire_data.lock = lock;
    acquire_data.timeout_ms = OS_LOCK_ACQUIRE_INFINITE;

    ccimp_status = ccimp_os_lock_acquire(&acquire_data);
    if (ccimp_status == CCIMP_STATUS_OK && acquire_data.acquired != CCAPI_TRUE)
    {
        ccimp_status = CCIMP_STATUS_ERROR;
    }

done:
    return ccimp_status;
}

ccimp_status_t ccapi_lock_release(void * const lock)
{
    ccimp_os_lock_release_t release_data;

    release_data.lock = lock;

    return ccimp_os_lock_release(&release_data);
}

ccimp_status_t ccapi_lock_destroy(void * const lock)
{
    ccimp_os_lock_destroy_t destroy_data;

    destroy_data.lock = lock;

    return ccimp_os_lock_destroy(&destroy_data);
}

connector_transport_t ccapi_to_connector_transport(ccapi_transport_t const ccapi_transport)
{
    connector_transport_t connector_transport = connector_transport_all;

    switch(ccapi_transport)
    {
        case CCAPI_TRANSPORT_TCP:
            connector_transport = connector_transport_tcp;
            break;
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
        case CCAPI_TRANSPORT_UDP:
            connector_transport = connector_transport_udp;
            break;
#endif
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
        case CCAPI_TRANSPORT_SMS:
            connector_transport = connector_transport_sms;
            break;
#endif
    }

    return connector_transport;
}

static connector_stop_condition_t ccapi_to_connector_stop(ccapi_transport_stop_t const ccapi_stop)
{
    connector_stop_condition_t stop_condition = INVALID_ENUM(connector_stop_condition_t);

    switch(ccapi_stop)
    {
        case CCAPI_TRANSPORT_STOP_GRACEFULLY:
            stop_condition = connector_wait_sessions_complete;
            break;
        case CCAPI_TRANSPORT_STOP_IMMEDIATELY:
            stop_condition = connector_stop_immediately;
            break;
    }

    return stop_condition;
}

connector_status_t ccapi_initiate_transport_stop(ccapi_data_t * const ccapi_data, ccapi_transport_t const transport, ccapi_transport_stop_t const behavior)
{
    connector_status_t ccfsm_status;
    connector_initiate_stop_request_t stop_data;

    stop_data.transport = ccapi_to_connector_transport(transport);;
    stop_data.user_context = NULL;
    stop_data.condition = ccapi_to_connector_stop(behavior);

    for (;;)
    {
        ccfsm_status = connector_initiate_action_secure(ccapi_data, connector_initiate_transport_stop, &stop_data);
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
            ASSERT_MSG_GOTO(ccfsm_status != connector_success, done);
            break;
    }

done:
    return ccfsm_status;
}

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)

ccimp_status_t ccapi_open_file(ccapi_data_t * const ccapi_data, char const * const local_path, int const flags, ccimp_fs_file_handle_t * const file_handler)
{
    ccimp_fs_file_open_t ccimp_fs_file_open_data = {0};
    ccapi_bool_t loop_done = CCAPI_FALSE;
    ccimp_status_t ccimp_status;

    ccimp_status = ccapi_lock_acquire(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            goto done;
    }

    ccimp_fs_file_open_data.path = local_path;
    ccimp_fs_file_open_data.flags = flags;
    ccimp_fs_file_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_file_open_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_fs_file_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;

    do {
        ccimp_status = ccimp_fs_file_open(&ccimp_fs_file_open_data);
        switch (ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_ERROR:
                ccapi_data->service.file_system.imp_context = ccimp_fs_file_open_data.imp_context;
                loop_done = CCAPI_TRUE;
                break;
            case CCIMP_STATUS_BUSY:
                break;
        }
        ccimp_os_yield();
    } while (!loop_done);

    ASSERT_MSG(ccapi_lock_release(ccapi_data->file_system_lock) == CCIMP_STATUS_OK);

    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            goto done;
    }

    *file_handler = ccimp_fs_file_open_data.handle;

done:
    return ccimp_status;
}

ccimp_status_t ccapi_read_file(ccapi_data_t * const ccapi_data, ccimp_fs_file_handle_t const file_handler, void * const data, size_t const bytes_available, size_t * const bytes_used)
{
    ccimp_fs_file_read_t ccimp_fs_file_read_data = {0};
    ccapi_bool_t loop_done = CCAPI_FALSE;
    ccimp_status_t ccimp_status;

    *bytes_used = 0;

    ccimp_status = ccapi_lock_acquire(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            goto done;
    }

    ccimp_fs_file_read_data.handle = file_handler;
    ccimp_fs_file_read_data.buffer = data;
    ccimp_fs_file_read_data.bytes_available = bytes_available;
    ccimp_fs_file_read_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_file_read_data.imp_context = ccapi_data->service.file_system.imp_context;

    do {
        ccimp_status = ccimp_fs_file_read(&ccimp_fs_file_read_data);
        switch (ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_ERROR:
                ccapi_data->service.file_system.imp_context = ccimp_fs_file_read_data.imp_context;
                loop_done = CCAPI_TRUE;
                break;
            case CCIMP_STATUS_BUSY:
                break;
        }
        ccimp_os_yield();
    } while (!loop_done);

    ASSERT_MSG(ccapi_lock_release(ccapi_data->file_system_lock) == CCIMP_STATUS_OK);

    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            goto done;
            break;
    }

    *bytes_used = ccimp_fs_file_read_data.bytes_used;

done:
    return ccimp_status;
}

ccimp_status_t ccapi_close_file(ccapi_data_t * const ccapi_data, ccimp_fs_file_handle_t const file_handler)
{
    ccimp_fs_file_close_t ccimp_fs_file_close_data = {0};
    ccapi_bool_t loop_done = CCAPI_FALSE;
    ccimp_status_t ccimp_status;

    ccimp_status = ccapi_lock_acquire(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            goto done;
    }

    ccimp_fs_file_close_data.handle = file_handler;
    ccimp_fs_file_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_file_close_data.imp_context = ccapi_data->service.file_system.imp_context;

    do {
        ccimp_status = ccimp_fs_file_close(&ccimp_fs_file_close_data);
        switch (ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_ERROR:
                ccapi_data->service.file_system.imp_context = ccimp_fs_file_close_data.imp_context;
                loop_done = CCAPI_TRUE;
                break;
            case CCIMP_STATUS_BUSY:
                break;
        }
        ccimp_os_yield();
    } while (!loop_done);

    ASSERT_MSG(ccapi_lock_release(ccapi_data->file_system_lock) == CCIMP_STATUS_OK);

done:
    return ccimp_status;
}

ccimp_status_t ccapi_get_dir_entry_status(ccapi_data_t * const ccapi_data, char const * const local_path, ccimp_fs_stat_t * const fs_status)
{
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data = {0};
    ccapi_bool_t loop_done = CCAPI_FALSE;
    ccimp_status_t ccimp_status;

    ccimp_status = ccapi_lock_acquire(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            goto done;
    }

    ccimp_fs_dir_entry_status_data.path = local_path;
    ccimp_fs_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_dir_entry_status_data.imp_context = ccapi_data->service.file_system.imp_context;

    do {
        ccimp_status = ccimp_fs_dir_entry_status(&ccimp_fs_dir_entry_status_data);
        switch (ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_ERROR:
                ccapi_data->service.file_system.imp_context = ccimp_fs_dir_entry_status_data.imp_context;
                loop_done = CCAPI_TRUE;
                break;
            case CCIMP_STATUS_BUSY:
                break;
        }
        ccimp_os_yield();
    } while (!loop_done);

    ASSERT_MSG(ccapi_lock_release(ccapi_data->file_system_lock) == CCIMP_STATUS_OK);

    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            goto done;
    }

    memcpy(fs_status, &ccimp_fs_dir_entry_status_data.status, sizeof *fs_status);

done:
    return ccimp_status;
}
#endif

connector_status_t connector_initiate_action_secure(ccapi_data_t * const ccapi_data, connector_initiate_request_t const request, void const * const request_data)
{
    connector_status_t ccfsm_status;
    ccimp_status_t ccimp_status;

    ccimp_status = ccapi_lock_acquire(ccapi_data->initiate_action_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            ccfsm_status = connector_abort;
            goto done;
    }

    ccfsm_status = connector_initiate_action(ccapi_data->connector_handle, request, request_data);

    ccimp_status = ccapi_lock_release(ccapi_data->initiate_action_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            ccfsm_status = connector_abort;
            goto done;
    }

    ccapi_lock_release(ccapi_data->thread.connector_run->lock);

done:
    ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
    return ccfsm_status;
}

char * ccapi_strdup(char const * const string)
{
    size_t const string_size = strlen(string) + 1;
    char * const dup_string = ccapi_malloc(string_size);

    if (dup_string != NULL)
    {
        memcpy(dup_string, string, string_size);
    }

    return dup_string;
}

void ccapi_connector_run_thread(void * const argument)
{
    ccapi_data_t * const ccapi_data = argument;

    /* ccapi_data is corrupted, it's likely the implementer made it wrong passing argument to the new thread */
    ASSERT_MSG_GOTO(ccapi_data != NULL, done);

    ccapi_data->thread.connector_run->status = CCAPI_THREAD_RUNNING;
    while (ccapi_data->thread.connector_run->status == CCAPI_THREAD_RUNNING)
    {
        connector_status_t const status = connector_run(ccapi_data->connector_handle);

        ASSERT_MSG_GOTO(status != connector_init_error, done);

        switch (status)
        {
            case connector_device_terminated:
                ccapi_data->thread.connector_run->status = CCAPI_THREAD_REQUEST_STOP;
                break;
            case connector_abort:
                if (ccapi_data->config.status_callback != NULL)
                {
                    ccapi_status_info_t status_info;
                    status_info.stop_cause = CCAPI_STOP_CCFSM_ERROR;
                    ccapi_data->config.status_callback(&status_info);
                }
                ccxapi_asynchronous_stop(ccapi_data);
                goto done;
                break;
            case connector_success:
            case connector_init_error:
            case connector_invalid_data_size:
            case connector_invalid_data_range:
            case connector_invalid_data:
            case connector_keepalive_error:
            case connector_bad_version:
            case connector_service_busy:
            case connector_invalid_response:
            case connector_no_resource:
            case connector_unavailable:
            case connector_idle:
            case connector_working:
            case connector_pending:
            case connector_active:
            case connector_device_error:
            case connector_exceed_timeout:
            case connector_invalid_payload_packet:
            case connector_open_error:
                break;
        }
    }

    ASSERT_MSG_GOTO(ccapi_data->thread.connector_run->status == CCAPI_THREAD_REQUEST_STOP, done);

    ccapi_data->thread.connector_run->status = CCAPI_THREAD_NOT_STARTED;
done:
    return;
}

connector_callback_status_t connector_callback_status_from_ccimp_status(ccimp_status_t const ccimp_status)
{
    connector_callback_status_t callback_status = connector_callback_abort;

    switch (ccimp_status)
    {
        case CCIMP_STATUS_ERROR:
            callback_status = connector_callback_error;
            break;
        case CCIMP_STATUS_OK:
            callback_status = connector_callback_continue;
            break;
        case CCIMP_STATUS_BUSY:
            callback_status = connector_callback_busy;
            break;
    }

    return callback_status;
}

connector_callback_status_t ccapi_config_handler(connector_request_id_config_t const config_request, void * const data, ccapi_data_t const * const ccapi_data)
{
    connector_callback_status_t status;

    ccapi_logging_line(TMP_INFO_PREFIX "ccapi_config_handler: config_request %d", config_request);

    switch (config_request)
    {
        case connector_request_id_config_device_id:
        {
            connector_config_pointer_data_t * const ccfsm_device_id = data;
            uint8_t * const ccapi_device_id = (uint8_t *)ccapi_data->config.device_id;

            ccfsm_device_id->data = ccapi_device_id;
            break;
        }
        case connector_request_id_config_device_cloud_url:
        {
            connector_config_pointer_string_t * const device_cloud = data;

            device_cloud->string = ccapi_data->config.device_cloud_url;
            device_cloud->length = strlen(ccapi_data->config.device_cloud_url);
            break;
        }
        case connector_request_id_config_vendor_id:
        {
            connector_config_vendor_id_t * const vendor_id = data;

            vendor_id->id = ccapi_data->config.vendor_id;
            break;
        }
        case connector_request_id_config_device_type:
        {
            connector_config_pointer_string_t * const device_type = data;

            device_type->string = ccapi_data->config.device_type;
            device_type->length = strlen(ccapi_data->config.device_type);
            break;
        }
        case connector_request_id_config_firmware_facility:
        {
            connector_config_supported_t * const firmware_supported = data;

            firmware_supported->supported = CCAPI_BOOL_TO_CONNECTOR_BOOL(ccapi_data->config.firmware_supported);
            break;
        }
        case connector_request_id_config_file_system:
        {
#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
            connector_config_supported_t * const filesystem_supported = data;

            filesystem_supported->supported = CCAPI_BOOL_TO_CONNECTOR_BOOL(ccapi_data->config.filesystem_supported);
#endif
            break;
        }
        case connector_request_id_config_remote_configuration:
        {
            connector_config_supported_t * const rci_supported = data;

            rci_supported->supported = CCAPI_BOOL_TO_CONNECTOR_BOOL(ccapi_data->config.rci_supported);
            break;
        }
        case connector_request_id_config_data_service:
        {
            connector_config_supported_t * const data_service_supported = data;

            data_service_supported->supported = connector_true;
            break;
        }
        case connector_request_id_config_streaming_cli:
        {
            connector_config_supported_t * const streaming_message_service_supported = data;

            streaming_message_service_supported->supported = CCAPI_BOOL_TO_CONNECTOR_BOOL(ccapi_data->config.streaming_cli_supported);
            break;
        }
#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)
		case connector_request_id_config_sm_key_distribution:
		{
			connector_config_supported_t * const sm_key_distribution_service_supported = data;

			sm_key_distribution_service_supported->supported = CCAPI_BOOL_TO_CONNECTOR_BOOL(ccapi_data->config.sm_key_distribution_supported);

			break;
		}
#endif
        case connector_request_id_config_connection_type:
        {
            connector_config_connection_type_t * const connection_type = data;

            switch (ccapi_data->transport_tcp.info->connection.type)
            {
                case CCAPI_CONNECTION_LAN:
                    connection_type->type = connector_connection_type_lan;
                    break;
                case CCAPI_CONNECTION_WAN:
                    connection_type->type = connector_connection_type_wan;
                    break;
            }
            break;
        }
        case connector_request_id_config_mac_addr:
        {
            connector_config_pointer_data_t * const mac_addr = data;

            mac_addr->data = ccapi_data->transport_tcp.info->connection.info.lan.mac_address;
            break;
        }
        case connector_request_id_config_ip_addr:
        {
            connector_config_ip_address_t * const ip_addr = data;

            switch(ccapi_data->transport_tcp.info->connection.ip.type)
            {
                case CCAPI_IPV4:
                {
                    ip_addr->ip_address_type = connector_ip_address_ipv4;
                    ip_addr->address = ccapi_data->transport_tcp.info->connection.ip.address.ipv4;
                    break;
                }
                case CCAPI_IPV6:
                {
                    ip_addr->ip_address_type = connector_ip_address_ipv6;
                    ip_addr->address = ccapi_data->transport_tcp.info->connection.ip.address.ipv6;
                    break;
                }
            }
            break;
        }
        case connector_request_id_config_identity_verification:
        {
            connector_config_identity_verification_t * const id_verification = data;
            int const has_password = ccapi_data->transport_tcp.info->connection.password != NULL;

            id_verification->type = has_password ? connector_identity_verification_password : connector_identity_verification_simple;
            break;
        }
        case connector_request_id_config_password:
        {
            connector_config_pointer_string_t * const password = data;

            password->string = ccapi_data->transport_tcp.info->connection.password;
            password->length = strlen(password->string);
            break;
        }
        case connector_request_id_config_max_transaction:
        {
            connector_config_max_transaction_t * const max_transaction = data;

            max_transaction->count = ccapi_data->transport_tcp.info->connection.max_transactions;
            break;
        }
        case connector_request_id_config_rx_keepalive:
        {
            connector_config_keepalive_t * const rx_keepalives = data;

            rx_keepalives->interval_in_seconds = ccapi_data->transport_tcp.info->keepalives.rx;
            break;
        }
        case connector_request_id_config_tx_keepalive:
        {
            connector_config_keepalive_t * const tx_keepalives = data;

            tx_keepalives->interval_in_seconds = ccapi_data->transport_tcp.info->keepalives.tx;
            break;
        }
        case connector_request_id_config_wait_count:
        {
            connector_config_wait_count_t * const wc_keepalives = data;

            wc_keepalives->count = ccapi_data->transport_tcp.info->keepalives.wait_count;
            break;
        }
        case connector_request_id_config_link_speed:
        {
            connector_config_link_speed_t * const link_speed = data;

            link_speed->speed = ccapi_data->transport_tcp.info->connection.info.wan.link_speed;
            break;
        }
        case connector_request_id_config_phone_number:
        {
            connector_config_pointer_string_t * const phone_number = data;

            phone_number->string = ccapi_data->transport_tcp.info->connection.info.wan.phone_number;
            phone_number->length = strlen(ccapi_data->transport_tcp.info->connection.info.wan.phone_number);
            break;
        }
        case connector_request_id_config_sm_udp_max_sessions:
        {
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
            connector_config_sm_max_sessions_t * const max_session = data;

            max_session->max_sessions = ccapi_data->transport_udp.info->limit.max_sessions;
#endif
            break;
        }
        case connector_request_id_config_sm_udp_rx_timeout:
        {
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
            connector_config_sm_rx_timeout_t * const rx_timeout = data;

            rx_timeout->rx_timeout = ccapi_data->transport_udp.info->limit.rx_timeout;
#endif
            break;
        }
        case connector_request_id_config_sm_sms_max_sessions:
        {
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
            connector_config_sm_max_sessions_t * const max_session = data;

            max_session->max_sessions = ccapi_data->transport_sms.info->limit.max_sessions;
#endif
            break;
        }
        case connector_request_id_config_sm_sms_rx_timeout:
        {
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
            connector_config_sm_rx_timeout_t * const rx_timeout = data;

            rx_timeout->rx_timeout = ccapi_data->transport_sms.info->limit.rx_timeout;
#endif
            break;
        }
        case connector_request_id_config_get_device_cloud_phone:
        {
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
            connector_config_pointer_string_t * const device_cloud_phone = data;

            device_cloud_phone->string = ccapi_data->transport_sms.info->cloud_config.phone_number;
            device_cloud_phone->length = strlen(ccapi_data->transport_sms.info->cloud_config.phone_number);
#endif
            break;
        }
        case connector_request_id_config_set_device_cloud_phone:
        {
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
            connector_config_pointer_string_t const * const device_cloud_phone = data;

            ASSERT_MSG_GOTO(device_cloud_phone->string != NULL, done);
            ASSERT_MSG_GOTO(device_cloud_phone->length == strlen(device_cloud_phone->string), done);

            if (ccapi_data->transport_sms.info->cloud_config.phone_number != NULL)
            {
                ccapi_free((void *) ccapi_data->transport_sms.info->cloud_config.phone_number);
            }

            ccapi_data->transport_sms.info->cloud_config.phone_number = ccapi_strdup(device_cloud_phone->string);
            if (ccapi_data->transport_sms.info->cloud_config.phone_number == NULL)
            {
                status = connector_callback_error;
                goto done;
            }
#endif
            break;
        }
        case connector_request_id_config_device_cloud_service_id:
        {
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
            connector_config_pointer_string_t * const service_id = data;

            service_id->string = ccapi_data->transport_sms.info->cloud_config.service_id;
            service_id->length = strlen(ccapi_data->transport_sms.info->cloud_config.service_id);
#endif
            break;
        }
        case connector_request_id_config_rci_descriptor_data:
        {
#if (defined CCIMP_RCI_SERVICE_ENABLED)
            connector_config_rci_descriptor_data_t * const rci_descriptor_data = data;

            rci_descriptor_data->rci_data = ccapi_data->service.rci.rci_data->rci_desc;
#endif
            break;
        }
        case connector_request_id_config_error_status:
        {
            connector_config_error_status_t const * const ccfsm_error_status = data;
            connector_class_id_t const class_id = ccfsm_error_status->class_id;
            int const request_id = ccfsm_error_status->request_id.int_value;
            connector_status_t const ccfsm_status = ccfsm_error_status->status;

            ccapi_logging_line("connector_request_id_config_error_status: class ID %d, request ID %d, status %d", class_id, request_id, ccfsm_status);
            break;
        }
        case connector_request_id_config_set_device_id:
        case connector_request_id_config_device_id_method:
        case connector_request_id_config_network_tcp:
        case connector_request_id_config_network_udp:
        case connector_request_id_config_network_sms:
        case connector_request_id_config_sm_udp_max_rx_segments:
        case connector_request_id_config_sm_sms_max_rx_segments:
            ASSERT_MSG_GOTO(CCAPI_FALSE, done);
            break;
    }
    status = connector_callback_continue;
done:
    return status;
}

connector_callback_status_t ccapi_os_handler(connector_request_id_os_t os_request, void * const data, ccapi_data_t const * const ccapi_data)
{
    connector_callback_status_t connector_status;
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    UNUSED_ARGUMENT(ccapi_data);
    switch (os_request) {
        case connector_request_id_os_malloc:
        {
            connector_os_malloc_t * const connector_malloc_data = data;
            ccimp_os_malloc_t ccimp_malloc_data;

            ccimp_malloc_data.size = connector_malloc_data->size;
            ccimp_malloc_data.ptr = connector_malloc_data->ptr;

            ccimp_status = ccimp_os_malloc(&ccimp_malloc_data);

            connector_malloc_data->ptr = ccimp_malloc_data.ptr;
            break;
        }

        case connector_request_id_os_free:
        {
            connector_os_free_t * const connector_free_data = data;
            ccimp_os_free_t ccimp_free_data;

            ccimp_free_data.ptr = connector_free_data->ptr;
            ccimp_status = ccimp_os_free(&ccimp_free_data);
            break;
        }

        case connector_request_id_os_yield:
        {
            connector_os_yield_t * connector_yield_data = data;

            if (connector_yield_data->status == connector_idle)
            {
                ccimp_os_lock_acquire_t acquire_data;
                ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

                ASSERT_MSG(ccapi_data->thread.connector_run->lock != NULL);
                acquire_data.lock = ccapi_data->thread.connector_run->lock;
                acquire_data.timeout_ms = CCIMP_IDLE_SLEEP_TIME_MS; /* TODO: could be increased (hopefully until next keep alive must be delivered)
                                                        if transports had a thread to handle connector_request_id_network_receive */

                /* ccapi_logging_line("+connector_run->lock"); */
                ccimp_status = ccimp_os_lock_acquire(&acquire_data);
                switch (ccimp_status)
                {
                    case CCIMP_STATUS_OK:
                        break;
                    case CCIMP_STATUS_BUSY:
                    case CCIMP_STATUS_ERROR:
                        ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
                        break;
                }
                /* ccapi_logging_line("-connector_run->lock: acquired=%d", acquire_data.acquired); */
            }

            ccimp_status = ccimp_os_yield();
            break;
        }

        case connector_request_id_os_system_up_time:
        {
            connector_os_system_up_time_t * const connector_system_uptime = data;
            ccimp_os_system_up_time_t ccimp_system_uptime;

            ccimp_status = ccimp_os_get_system_time(&ccimp_system_uptime);

            connector_system_uptime->sys_uptime = ccimp_system_uptime.sys_uptime;
            break;
        }

        case connector_request_id_os_reboot:
        {
            ccimp_status = ccimp_hal_reset();
            break;
        }

        case connector_request_id_os_realloc:
        {
            connector_os_realloc_t * const connector_realloc_data = data;
            ccimp_os_realloc_t ccimp_realloc_data;

            ccimp_realloc_data.new_size = connector_realloc_data->new_size;
            ccimp_realloc_data.old_size = connector_realloc_data->old_size;
            ccimp_realloc_data.ptr = connector_realloc_data->ptr;
            ccimp_status = ccimp_os_realloc(&ccimp_realloc_data);

            connector_realloc_data->ptr = ccimp_realloc_data.ptr;
            break;
        }
    }

    connector_status = connector_callback_status_from_ccimp_status(ccimp_status);

    return connector_status;
}

static ccapi_bool_t ask_user_if_reconnect(connector_close_status_t const close_status, ccapi_data_t const * const ccapi_data)
{
    ccapi_tcp_close_cb_t const close_cb = ccapi_data->transport_tcp.info->callback.close;
    ccapi_bool_t reconnect = CCAPI_FALSE;

    if (close_cb != NULL)
    {
        ccapi_tcp_close_cause_t ccapi_close_cause = INVALID_ENUM(ccapi_tcp_close_cause_t);

        switch (close_status)
        {
            case connector_close_status_cloud_disconnected:
                ccapi_close_cause = CCAPI_TCP_CLOSE_DISCONNECTED;
                break;
            case connector_close_status_cloud_redirected:
                ccapi_close_cause = CCAPI_TCP_CLOSE_REDIRECTED;
                break;
            case connector_close_status_device_error:
                ccapi_close_cause = CCAPI_TCP_CLOSE_DATA_ERROR;
                break;
            case connector_close_status_no_keepalive:
                ccapi_close_cause = CCAPI_TCP_CLOSE_NO_KEEPALIVE;
                break;
            case connector_close_status_device_stopped:
            case connector_close_status_device_terminated:
            case connector_close_status_abort:
                ASSERT_MSG_GOTO(close_status != connector_close_status_cloud_disconnected, done);
                break;
        }
        reconnect = close_cb(ccapi_close_cause);
    }
done:
    return reconnect;
}
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
static ccapi_bool_t ask_user_if_reconnect_udp(connector_close_status_t const close_status, ccapi_data_t const * const ccapi_data)
{
    ccapi_udp_close_cb_t const close_cb = ccapi_data->transport_udp.info->callback.close;
    ccapi_udp_close_cause_t ccapi_close_cause = INVALID_ENUM(ccapi_udp_close_cause_t);
    ccapi_bool_t reconnect = CCAPI_FALSE;

    if (close_cb != NULL)
    {
        switch (close_status)
        {
            case connector_close_status_cloud_disconnected:
                ccapi_close_cause = CCAPI_UDP_CLOSE_DISCONNECTED;
                break;

            case connector_close_status_device_error:
                ccapi_close_cause = CCAPI_UDP_CLOSE_DATA_ERROR;
                break;
            case connector_close_status_no_keepalive:
            case connector_close_status_cloud_redirected:
            case connector_close_status_device_stopped:
            case connector_close_status_device_terminated:
            case connector_close_status_abort:
                ASSERT_MSG_GOTO(close_status == connector_close_status_cloud_disconnected || close_status == connector_close_status_device_error, done);
                break;
        }
        reconnect = close_cb(ccapi_close_cause);
    }
done:
    return reconnect;
}
#endif

#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
static ccapi_bool_t ask_user_if_reconnect_sms(connector_close_status_t const close_status, ccapi_data_t const * const ccapi_data)
{
    ccapi_sms_close_cb_t const close_cb = ccapi_data->transport_sms.info->callback.close;
    ccapi_sms_close_cause_t ccapi_close_cause = INVALID_ENUM(ccapi_sms_close_cause_t);
    ccapi_bool_t reconnect = CCAPI_FALSE;

    if (close_cb != NULL)
    {
        switch (close_status)
        {
            case connector_close_status_cloud_disconnected:
                ccapi_close_cause = CCAPI_SMS_CLOSE_DISCONNECTED;
                break;

            case connector_close_status_device_error:
                ccapi_close_cause = CCAPI_SMS_CLOSE_DATA_ERROR;
                break;
            case connector_close_status_no_keepalive:
            case connector_close_status_cloud_redirected:
            case connector_close_status_device_stopped:
            case connector_close_status_device_terminated:
            case connector_close_status_abort:
                ASSERT_MSG_GOTO(close_status == connector_close_status_cloud_disconnected || close_status == connector_close_status_device_error, done);
                break;
        }
        reconnect = close_cb(ccapi_close_cause);
    }

done:
    return reconnect;
}
#endif

connector_callback_status_t ccapi_network_handler(connector_request_id_network_t const network_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status;
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    switch (network_request)
    {
        case connector_request_id_network_open:
        {
            connector_network_open_t * const connector_open_data = data;
            ccimp_network_open_t ccimp_open_data;

            ccimp_open_data.device_cloud.url = connector_open_data->device_cloud.url;
            ccimp_open_data.handle = connector_open_data->handle;

            ccimp_status = ccimp_network_tcp_open(&ccimp_open_data);

            connector_open_data->handle = ccimp_open_data.handle;
            break;
        }

        case connector_request_id_network_send:
        {
            connector_network_send_t * const connector_send_data = data;
            ccimp_network_send_t ccimp_send_data;

            ccimp_send_data.buffer = connector_send_data->buffer;
            ccimp_send_data.bytes_available = connector_send_data->bytes_available;
            ccimp_send_data.handle = connector_send_data->handle;
            ccimp_send_data.bytes_used = 0;

            ccimp_status = ccimp_network_tcp_send(&ccimp_send_data);

            connector_send_data->bytes_used = ccimp_send_data.bytes_used;
            break;
        }

        case connector_request_id_network_receive:
        {
            connector_network_receive_t * const connector_receive_data = data;
            ccimp_network_receive_t ccimp_receive_data;

            ccimp_receive_data.buffer = connector_receive_data->buffer;
            ccimp_receive_data.bytes_available = connector_receive_data->bytes_available;
            ccimp_receive_data.handle = connector_receive_data->handle;
            ccimp_receive_data.bytes_used = 0;

            ccimp_status = ccimp_network_tcp_receive(&ccimp_receive_data);

            connector_receive_data->bytes_used = ccimp_receive_data.bytes_used;
            break;
        }

        case connector_request_id_network_close:
        {
            connector_network_close_t * const connector_close_data = data;
            ccimp_network_close_t close_data;
            connector_close_status_t const close_status = connector_close_data->status;

            close_data.handle = connector_close_data->handle;
            ccimp_status = ccimp_network_tcp_close(&close_data);

            switch(ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    ccapi_data->transport_tcp.connected = CCAPI_FALSE;
                    break;
                case CCIMP_STATUS_ERROR:
                case CCIMP_STATUS_BUSY:
                    goto done;
                    break;
            }

            switch (close_status)
            {
                case connector_close_status_cloud_disconnected:
                case connector_close_status_cloud_redirected:
                case connector_close_status_device_error:
                case connector_close_status_no_keepalive:
                {
                    connector_close_data->reconnect = CCAPI_BOOL_TO_CONNECTOR_BOOL(ask_user_if_reconnect(close_status, ccapi_data));
                    break;
                }
                case connector_close_status_device_stopped:
                case connector_close_status_device_terminated:
                case connector_close_status_abort:
                {
                    connector_close_data->reconnect = connector_false;
                    break;
                }
            }
            break;
        }
    }

done:
    connector_status = connector_callback_status_from_ccimp_status(ccimp_status);

    return connector_status;
}

#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
connector_callback_status_t ccapi_network_udp_handler(connector_request_id_network_t const network_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status;
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    switch (network_request)
    {
        case connector_request_id_network_open:
        {
            connector_network_open_t * const connector_open_data = data;
            ccimp_network_open_t ccimp_open_data;

            ccimp_open_data.device_cloud.url = connector_open_data->device_cloud.url;
            ccimp_open_data.handle = connector_open_data->handle;

            ccimp_status = ccimp_network_udp_open(&ccimp_open_data);

            if (ccimp_status == CCIMP_STATUS_OK)
            {
                ccapi_data->transport_udp.started = CCAPI_TRUE;
            }
            connector_open_data->handle = ccimp_open_data.handle;

            break;
        }

        case connector_request_id_network_send:
        {
            connector_network_send_t * const connector_send_data = data;
            ccimp_network_send_t ccimp_send_data;

            ccimp_send_data.buffer = connector_send_data->buffer;
            ccimp_send_data.bytes_available = connector_send_data->bytes_available;
            ccimp_send_data.handle = connector_send_data->handle;
            ccimp_send_data.bytes_used = 0;

            ccimp_status = ccimp_network_udp_send(&ccimp_send_data);

            connector_send_data->bytes_used = ccimp_send_data.bytes_used;

            break;
        }

        case connector_request_id_network_receive:
        {
            connector_network_receive_t * const connector_receive_data = data;
            ccimp_network_receive_t ccimp_receive_data;

            ccimp_receive_data.buffer = connector_receive_data->buffer;
            ccimp_receive_data.bytes_available = connector_receive_data->bytes_available;
            ccimp_receive_data.handle = connector_receive_data->handle;
            ccimp_receive_data.bytes_used = 0;

            ccimp_status = ccimp_network_udp_receive(&ccimp_receive_data);

            connector_receive_data->bytes_used = ccimp_receive_data.bytes_used;
            break;
        }

        case connector_request_id_network_close:
        {
            connector_network_close_t * const connector_close_data = data;
            ccimp_network_close_t close_data;
            connector_close_status_t const close_status = connector_close_data->status;

            close_data.handle = connector_close_data->handle;
            ccimp_status = ccimp_network_udp_close(&close_data);

            switch(ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    ccapi_data->transport_udp.started = CCAPI_FALSE;
                    break;
                case CCIMP_STATUS_ERROR:
                case CCIMP_STATUS_BUSY:
                    goto done;
                    break;
            }

            switch (close_status)
            {
                /* if either Device Cloud or our application cuts the connection, don't reconnect */
                case connector_close_status_device_stopped:
                case connector_close_status_device_terminated:
                case connector_close_status_abort:
                {
                    connector_close_data->reconnect = connector_false;
                    break;
                }
                case connector_close_status_cloud_disconnected:
                case connector_close_status_device_error:
                {
                    connector_close_data->reconnect = CCAPI_BOOL_TO_CONNECTOR_BOOL(ask_user_if_reconnect_udp(close_status, ccapi_data));
                    break;
                }
                case connector_close_status_cloud_redirected:
                case connector_close_status_no_keepalive:
                break;

            }
            break;
        }
    }

done:

    connector_status = connector_callback_status_from_ccimp_status(ccimp_status);

    return connector_status;
}
#endif

#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
connector_callback_status_t ccapi_network_sms_handler(connector_request_id_network_t const network_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status;
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    switch (network_request)
    {
        case connector_request_id_network_open:
        {
            connector_network_open_t * const connector_open_data = data;
            ccimp_network_open_t ccimp_open_data;

            ccimp_open_data.device_cloud.phone = connector_open_data->device_cloud.phone;
            ccimp_open_data.handle = connector_open_data->handle;

            ccimp_status = ccimp_network_sms_open(&ccimp_open_data);

            if (ccimp_status == CCIMP_STATUS_OK)
            {
                ccapi_data->transport_sms.started = CCAPI_TRUE;
            }
            connector_open_data->handle = ccimp_open_data.handle;

            break;
        }

        case connector_request_id_network_send:
        {
            connector_network_send_t * const connector_send_data = data;
            ccimp_network_send_t ccimp_send_data;

            ccimp_send_data.buffer = connector_send_data->buffer;
            ccimp_send_data.bytes_available = connector_send_data->bytes_available;
            ccimp_send_data.handle = connector_send_data->handle;
            ccimp_send_data.bytes_used = 0;

            ccimp_status = ccimp_network_sms_send(&ccimp_send_data);

            connector_send_data->bytes_used = ccimp_send_data.bytes_used;

            break;
        }

        case connector_request_id_network_receive:
        {
            connector_network_receive_t * const connector_receive_data = data;
            ccimp_network_receive_t ccimp_receive_data;

            ccimp_receive_data.buffer = connector_receive_data->buffer;
            ccimp_receive_data.bytes_available = connector_receive_data->bytes_available;
            ccimp_receive_data.handle = connector_receive_data->handle;
            ccimp_receive_data.bytes_used = 0;

            ccimp_status = ccimp_network_sms_receive(&ccimp_receive_data);

            connector_receive_data->bytes_used = ccimp_receive_data.bytes_used;
            break;
        }

        case connector_request_id_network_close:
        {
            connector_network_close_t * const connector_close_data = data;
            connector_close_status_t const close_status = connector_close_data->status;
            ccimp_network_close_t close_data;

            close_data.handle = connector_close_data->handle;
            ccimp_status = ccimp_network_sms_close(&close_data);

            switch(ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    ccapi_data->transport_sms.started = CCAPI_FALSE;
                    break;
                case CCIMP_STATUS_ERROR:
                case CCIMP_STATUS_BUSY:
                    goto done;
                    break;
            }

            switch (close_status)
            {
                /* if either Device Cloud or our application cuts the connection, don't reconnect */
                case connector_close_status_device_stopped:
                case connector_close_status_device_terminated:
                case connector_close_status_abort:
                {
                    connector_close_data->reconnect = connector_false;
                    break;
                }
                case connector_close_status_cloud_disconnected:
                case connector_close_status_device_error:
                {
                    connector_close_data->reconnect = CCAPI_BOOL_TO_CONNECTOR_BOOL(ask_user_if_reconnect_sms(close_status, ccapi_data));
                    break;
                }
                case connector_close_status_cloud_redirected:
                case connector_close_status_no_keepalive:
                break;
            }
            break;
        }
    }
done:

    connector_status = connector_callback_status_from_ccimp_status(ccimp_status);

    return connector_status;
}
#endif



connector_callback_status_t ccapi_status_handler(connector_request_id_status_t status_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_continue;

    switch (status_request)
    {
        case connector_request_id_status_tcp:
        {
            connector_status_tcp_event_t const * const tcp_event = data;

            switch(tcp_event->status)
            {
                case connector_tcp_communication_started:
                {
                    ccapi_logging_line("TCP communication started");
                    ccapi_data->transport_tcp.connected = CCAPI_TRUE;
                    break;
                }
                case connector_tcp_keepalive_missed:
                {
                    ccapi_keepalive_status_t const keepalive_status = CCAPI_KEEPALIVE_MISSED;

                    if (ccapi_data->transport_tcp.info->callback.keepalive != NULL)
                    {
                        ccapi_data->transport_tcp.info->callback.keepalive(keepalive_status);
                    }
                    break;
                }
                case connector_tcp_keepalive_restored:
                {
                    ccapi_keepalive_status_t const keepalive_status = CCAPI_KEEPALIVE_RESTORED;

                    if (ccapi_data->transport_tcp.info->callback.keepalive != NULL)
                    {
                        ccapi_data->transport_tcp.info->callback.keepalive(keepalive_status);
                    }
                    break;
                }
            }
            break;
        }
        case connector_request_id_status_stop_completed:
        {
            connector_initiate_stop_request_t const * const stop_request = data;

            switch (stop_request->transport)
            {
                case connector_transport_tcp:
                {
                    ccapi_data->transport_tcp.connected = CCAPI_FALSE;
                    break;
                }
#if (defined CONNECTOR_TRANSPORT_UDP)
                case connector_transport_udp:
                {
                    ccapi_data->transport_udp.started = CCAPI_FALSE;
                    break;
                }
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
                case connector_transport_sms:
                {
                    ccapi_data->transport_sms.started = CCAPI_FALSE;
                    break;
                }
#endif
                case connector_transport_all:
                {
                    ccapi_data->transport_tcp.connected = CCAPI_FALSE;
#if (defined CONNECTOR_TRANSPORT_UDP)
                    ccapi_data->transport_udp.started = CCAPI_FALSE;
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
                    ccapi_data->transport_sms.started = CCAPI_FALSE;
#endif
                    break;
                }
            }
            break;
        }
    }

    return connector_status;
}

#if (defined CCIMP_DATA_POINTS_ENABLED)
connector_callback_status_t ccapi_data_points_handler(connector_request_id_data_point_t const data_point_request, void * const data, ccapi_data_t const * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_continue;

    UNUSED_ARGUMENT(ccapi_data);

    switch (data_point_request)
    {
        case connector_request_id_data_point_response:
        {
            connector_data_point_response_t const * const data_point_response = data;
            ccapi_dp_transaction_info_t * const transaction_info = data_point_response->user_context;

            switch (data_point_response->response)
            {
                case connector_data_point_response_success:
                    transaction_info->response_error = CCAPI_DP_ERROR_NONE;
                    break;
                case connector_data_point_response_bad_request:
                    transaction_info->response_error = CCAPI_DP_ERROR_RESPONSE_BAD_REQUEST;
                    break;
                case connector_data_point_response_unavailable:
                    transaction_info->response_error = CCAPI_DP_ERROR_RESPONSE_UNAVAILABLE;
                    break;
                case connector_data_point_response_cloud_error:
                    transaction_info->response_error = CCAPI_DP_ERROR_RESPONSE_CLOUD_ERROR;
                    break;
            }

            if (data_point_response->hint)
            {
                ccapi_logging_line("data point request response hint: '%s'", data_point_response->hint);
                if (transaction_info->hint != NULL)
                {
                    size_t const max_hint_length = transaction_info->hint->length - 1;

                    transaction_info->hint->length = strlen(data_point_response->hint);
                    strncpy(transaction_info->hint->string, data_point_response->hint, max_hint_length);
                    transaction_info->hint->string[max_hint_length] = '\0';
                }
            }
            break;
        }
        case connector_request_id_data_point_status:
        {
            connector_data_point_status_t const * const data_point_status = data;
            ccapi_dp_transaction_info_t * const transaction_info = data_point_status->user_context;

            switch (data_point_status->status)
            {
                case connector_data_point_status_complete:
                    transaction_info->status = CCAPI_DP_ERROR_NONE;
                    break;
                case connector_data_point_status_cancel:
                    transaction_info->status = CCAPI_DP_ERROR_STATUS_CANCEL;
                    break;
                case connector_data_point_status_timeout:
                    transaction_info->status = CCAPI_DP_ERROR_STATUS_TIMEOUT;
                    break;
                case connector_data_point_status_invalid_data:
                    transaction_info->status = CCAPI_DP_ERROR_STATUS_INVALID_DATA;
                    break;
                case connector_data_point_status_session_error:
                    ccapi_logging_line("Data Points status: session_error = %d\n", data_point_status->session_error);
                    transaction_info->status = CCAPI_DP_ERROR_STATUS_SESSION_ERROR;
                    break;
            }

            switch(ccapi_lock_release(transaction_info->lock))
            {
                case CCIMP_STATUS_OK:
                    break;
                case CCIMP_STATUS_BUSY:
                    connector_status = connector_callback_busy;
                    goto done;
                case CCIMP_STATUS_ERROR:
                    connector_status = connector_callback_error;
                    goto done;
            }
            break;
        }
        case connector_request_id_data_point_binary_response:
        case connector_request_id_data_point_binary_status:
        {
            connector_status = connector_callback_unrecognized;
            goto done;
        }
    }

done:
    return connector_status;
}
#endif

connector_callback_status_t ccapi_connector_callback(connector_class_id_t const class_id, connector_request_id_t const request_id, void * const data, void * const context)
{
    connector_callback_status_t status = connector_callback_error;
    ccapi_data_t * const ccapi_data = context;

    switch (class_id)
    {
        case connector_class_id_config:
            status = ccapi_config_handler(request_id.config_request, data, ccapi_data);
            break;
        case connector_class_id_operating_system:
            status = ccapi_os_handler(request_id.os_request, data, ccapi_data);
            break;
        case connector_class_id_network_tcp:
            status = ccapi_network_handler(request_id.network_request, data, ccapi_data);
            break;
        case connector_class_id_network_udp:
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
            status = ccapi_network_udp_handler(request_id.network_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_network_sms:
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
            status = ccapi_network_sms_handler(request_id.network_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_status:
            status = ccapi_status_handler(request_id.status_request, data, ccapi_data);
            break;
        case connector_class_id_file_system:
#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
            status = ccapi_filesystem_handler(request_id.file_system_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_firmware:
#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
            status = ccapi_firmware_service_handler(request_id.firmware_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_data_service:
#if (defined CCIMP_DATA_SERVICE_ENABLED)
            status = ccapi_data_service_handler(request_id.data_service_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_short_message:
#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)
            status = ccapi_sm_service_handler(request_id.sm_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_data_point:
#if (defined CCIMP_DATA_POINTS_ENABLED)
            status = ccapi_data_points_handler(request_id.data_point_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_remote_config:
#if (defined CCIMP_RCI_SERVICE_ENABLED)
            status = ccapi_rci_handler(request_id.remote_config_request, data, ccapi_data);
#endif
            break;
        case connector_class_id_streaming_cli:
#if (defined CCIMP_STREAMING_CLI_SERVICE_ENABLED)
            status = ccapi_streaming_cli_handler(request_id.streaming_cli_service_request, data, ccapi_data);
#endif
            break;
        default:
            status = connector_callback_unrecognized;
            break;
    }

    ASSERT_MSG_GOTO(status != connector_callback_unrecognized, done);

done:
    return status;
}
