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
#include "ccapi/ccxapi.h"

static ccapi_start_error_t check_params(ccapi_start_t const * const start)
{
    static uint8_t const invalid_device_id[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ccapi_start_error_t error = CCAPI_START_ERROR_NONE;

    if (start->vendor_id == 0x00)
    {
        error = CCAPI_START_ERROR_INVALID_VENDORID;
        goto done;
    }

    if (memcmp(start->device_id, invalid_device_id, sizeof invalid_device_id) == 0)
    {
        error = CCAPI_START_ERROR_INVALID_DEVICEID;
        goto done;
    }

    if (start->device_cloud_url == NULL || start->device_cloud_url[0] == '\0')
    {
        error = CCAPI_START_ERROR_INVALID_URL;
        goto done;
    }

    if (start->device_type == NULL || start->device_type[0] == '\0')
    {
        error = CCAPI_START_ERROR_INVALID_DEVICETYPE;
        goto done;
    }

done:
    return error;
}

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
static void free_filesystem_dir_entry_list(ccapi_data_t * const ccapi_data)
{
    ccapi_fs_virtual_dir_t * dir_entry = ccapi_data->service.file_system.virtual_dir_list;

    do {
        ccapi_fs_virtual_dir_t * const next_dir_entry = dir_entry->next;
        ccapi_free(dir_entry->local_dir);
        ccapi_free(dir_entry->virtual_dir);
        ccapi_free(dir_entry);
        dir_entry = next_dir_entry;
    } while (dir_entry != NULL);
}
#endif

#if (defined CCIMP_DATA_SERVICE_ENABLED)
static void free_receive_target_list(ccapi_data_t * const ccapi_data)
{
    ccimp_status_t ccimp_status;
    ccapi_receive_target_t * target_entry;

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

    target_entry = ccapi_data->service.receive.target_list;

    while (target_entry != NULL)
    {
        ccapi_receive_target_t * const next_target_entry = target_entry->next;

        ccapi_free(target_entry->target);
        ccapi_free(target_entry);
        target_entry = next_target_entry;
    }

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
#endif

static void free_ccapi_data_internal_resources(ccapi_data_t * const ccapi_data)
{
    ASSERT_MSG_GOTO(ccapi_data != NULL, done);

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
    if (ccapi_data->file_system_lock != NULL)
    {
        ccimp_status_t const ccimp_status = ccapi_lock_destroy(ccapi_data->file_system_lock);
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

    if (ccapi_data->config.filesystem_supported)
    {
        if (ccapi_data->service.file_system.virtual_dir_list != NULL)
        {
            free_filesystem_dir_entry_list(ccapi_data);
        }
    }
#endif

#if (defined CONNECTOR_SM_CLI)
    if (ccapi_data->config.cli_supported)
    {
        reset_heap_ptr(&ccapi_data->thread.cli);
    }
#endif

#if (defined CCIMP_DATA_SERVICE_ENABLED)
    if (ccapi_data->config.receive_supported)
    {
        if (ccapi_data->service.receive.target_list != NULL)
        {
            free_receive_target_list(ccapi_data);
        }

        if (ccapi_data->service.receive.receive_lock != NULL)
        {
            ccimp_status_t const ccimp_status = ccapi_lock_destroy(ccapi_data->service.receive.receive_lock);
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
        reset_heap_ptr(&ccapi_data->thread.receive);
    }
#endif

#if (defined CCIMP_RCI_SERVICE_ENABLED)
    if (ccapi_data->config.rci_supported)
    {
        reset_heap_ptr(&ccapi_data->thread.rci);
    }
#endif

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    if (ccapi_data->service.firmware_update.config.target.count && ccapi_data->service.firmware_update.config.target.item != NULL)
    {
        unsigned char target_num;

        for (target_num = 0; target_num < ccapi_data->service.firmware_update.config.target.count; target_num++)
        {
            if (ccapi_data->service.firmware_update.config.target.item[target_num].description != NULL)
            {
                ccapi_free((void *)ccapi_data->service.firmware_update.config.target.item[target_num].description);
            }

            if (ccapi_data->service.firmware_update.config.target.item[target_num].filespec != NULL)
            {
               ccapi_free((void*)ccapi_data->service.firmware_update.config.target.item[target_num].filespec);
            }
        }

        ccapi_free(ccapi_data->service.firmware_update.config.target.item);

        ccapi_data->service.firmware_update.config.target.count = 0;
        ccapi_data->service.firmware_update.config.target.item = NULL;
        reset_heap_ptr(&ccapi_data->thread.firmware);
    }
#endif

    reset_heap_ptr(&ccapi_data->config.device_type);
    reset_heap_ptr(&ccapi_data->config.device_cloud_url);
    reset_heap_ptr(&ccapi_data->thread.connector_run);

    if (ccapi_data->initiate_action_lock != NULL)
    {
        ccimp_status_t const ccimp_status = ccapi_lock_destroy(ccapi_data->initiate_action_lock);
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

done:
    return;
}

#if (defined CCIMP_DEBUG_ENABLED)
static void free_logging_resources(void)
{
    if (--logging_lock_users == 0)
    {
        if (logging_lock != NULL)
        {
            ccimp_status_t const ccimp_status = ccapi_lock_destroy(logging_lock);
            switch (ccimp_status)
            {
                case CCIMP_STATUS_OK:
                    break;
                case CCIMP_STATUS_ERROR:
                case CCIMP_STATUS_BUSY:
                    ASSERT_MSG(ccimp_status == CCIMP_STATUS_OK);
                    break;
            }

            logging_lock = NULL;
        }
    }

    return;
}
#endif

static ccapi_start_error_t check_malloc(void const * const p)
{
    if (p == NULL)
        return CCAPI_START_ERROR_INSUFFICIENT_MEMORY;
    else
        return CCAPI_START_ERROR_NONE;
}


static ccapi_start_error_t ccapi_create_and_start_thread(ccapi_data_t * const ccapi_data, ccapi_thread_info_t * * const thread_info, ccimp_os_thread_start_t thread_start, ccimp_os_thread_type_t thread_type)
{
    ccapi_start_error_t error = CCAPI_START_ERROR_NONE;
    ccapi_thread_info_t * thread_info_aux;

    thread_info_aux = ccapi_malloc(sizeof *thread_info_aux);
    error = check_malloc(thread_info_aux);
    if (error != CCAPI_START_ERROR_NONE)
        goto done;

    thread_info_aux->lock = ccapi_lock_create();
    if (thread_info_aux->lock == NULL)
    {
        error = CCAPI_START_ERROR_LOCK_FAILED;
        goto done;
    }

    thread_info_aux->status = CCAPI_THREAD_REQUEST_START;
    thread_info_aux->ccimp_info.argument = ccapi_data;
    thread_info_aux->ccimp_info.start = thread_start;
    thread_info_aux->ccimp_info.type = thread_type;

    *thread_info = thread_info_aux;

    if (ccimp_os_create_thread(&thread_info_aux->ccimp_info) != CCIMP_STATUS_OK)
    {
        error = CCAPI_START_ERROR_THREAD_FAILED;
        goto done;
    }

    do
    {
        ccimp_os_yield();
    } while (thread_info_aux->status == CCAPI_THREAD_REQUEST_START);

done:
    return error;
}

static void ccapi_stop_thread(ccapi_thread_info_t * thread_info)
{
    ASSERT_MSG_GOTO(thread_info != NULL, done);

    ccapi_logging_line("ccapi_stop_thread: type=%d", thread_info->ccimp_info.type);

    if (thread_info->status == CCAPI_THREAD_RUNNING)
    {
        thread_info->status = CCAPI_THREAD_REQUEST_STOP;
    }

    ASSERT_MSG_GOTO(thread_info->lock != NULL, done);
    ccapi_lock_release(thread_info->lock);

    do {
        ccimp_os_yield();
    } while (thread_info->status != CCAPI_THREAD_NOT_STARTED);

    ccapi_lock_destroy(thread_info->lock);
done:
    return;
}

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
ccapi_fw_request_error_t stub_firmware_request_reject_all(unsigned int const target, char const * const filename, size_t const total_size)
{
    UNUSED_ARGUMENT(target);
    UNUSED_ARGUMENT(filename);
    UNUSED_ARGUMENT(total_size);
    return CCAPI_FW_REQUEST_ERROR_DOWNLOAD_CONFIGURED_TO_REJECT;
}
#endif

/* This function allocates ccapi_data_t so other ccXapi_* functions can use it as a handler */
ccapi_start_error_t ccxapi_start(ccapi_handle_t * const ccapi_handle, ccapi_start_t const * const start)
{
    ccapi_start_error_t error = CCAPI_START_ERROR_NONE;
    ccapi_data_t * ccapi_data = NULL;
#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    ccapi_bool_t stub_fw_update = CCAPI_TRUE;
#endif

#if (defined CCIMP_DEBUG_ENABLED)
    /* Initialize one single time for all connector instances the logging lock object */
    if (logging_lock_users++ == 0)
    {
        ccimp_os_lock_create_t create_data;

        if (ccimp_os_lock_create(&create_data) != CCIMP_STATUS_OK || ccapi_lock_release(create_data.lock) != CCIMP_STATUS_OK)
        {
            error = CCAPI_START_ERROR_LOCK_FAILED;
            goto done;
        }

        ASSERT(logging_lock == NULL);

        logging_lock = create_data.lock;
    }
#endif

    if (ccapi_handle == NULL)
    {
        error = CCAPI_START_ERROR_NULL_PARAMETER;
        goto done;
    }

    if (*ccapi_handle != NULL)
    {
        error = CCAPI_START_ERROR_ALREADY_STARTED;
        goto done;
    }

    ccapi_data = ccapi_malloc(sizeof *ccapi_data);
    *ccapi_handle = (ccapi_handle_t)ccapi_data;

    error = check_malloc(ccapi_data);
    if (error != CCAPI_START_ERROR_NONE)
        goto done;
    ccapi_data->initiate_action_lock = NULL;
    ccapi_data->service.file_system.virtual_dir_list = NULL;
    ccapi_data->file_system_lock = NULL;

    ccapi_data->config.device_type = NULL;
    ccapi_data->config.device_cloud_url = NULL;
    ccapi_data->thread.connector_run = NULL;
    ccapi_data->thread.rci = NULL;
    ccapi_data->thread.receive = NULL;
    ccapi_data->thread.cli = NULL;
    ccapi_data->thread.firmware = NULL;

    ccapi_data->config.firmware_supported = CCAPI_FALSE;
#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    ccapi_data->service.firmware_update.config.target.count = 0;
    ccapi_data->service.firmware_update.config.target.item = NULL;
#endif
    ccapi_data->config.receive_supported = CCAPI_FALSE;
    ccapi_data->config.cli_supported = CCAPI_FALSE;
    ccapi_data->config.sm_supported = CCAPI_FALSE;
    ccapi_data->config.rci_supported = CCAPI_FALSE;
    ccapi_data->config.streaming_cli_supported = CCAPI_FALSE;

    if (start == NULL)
    {
        error = CCAPI_START_ERROR_NULL_PARAMETER;
        goto done;
    }

    error = check_params(start);
    if (error != CCAPI_START_ERROR_NONE)
        goto done;

    ccapi_data->config.vendor_id = start->vendor_id;
    memcpy(ccapi_data->config.device_id, start->device_id, sizeof ccapi_data->config.device_id);

    ccapi_data->config.device_type = ccapi_malloc(strlen(start->device_type) + 1);
    error = check_malloc(ccapi_data->config.device_type);
    if (error != CCAPI_START_ERROR_NONE)
        goto done;
    strcpy(ccapi_data->config.device_type, start->device_type);

    ccapi_data->config.device_cloud_url = ccapi_malloc(strlen(start->device_cloud_url) + 1);
    error = check_malloc(ccapi_data->config.device_cloud_url);
    if (error != CCAPI_START_ERROR_NONE)
        goto done;
    strcpy(ccapi_data->config.device_cloud_url, start->device_cloud_url);

    ccapi_data->config.status_callback = start->status;

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
    if (start->service.file_system != NULL)
    {
        ccapi_data->config.filesystem_supported = CCAPI_TRUE;
        ccapi_data->service.file_system.user_callback.access = start->service.file_system->access;
        ccapi_data->service.file_system.user_callback.changed = start->service.file_system->changed;
        ccapi_data->service.file_system.imp_context = NULL;
        ccapi_data->service.file_system.virtual_dir_list = NULL;
    }
    else
#endif
    {
        ccapi_data->config.filesystem_supported = CCAPI_FALSE;
    }

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    if (start->service.firmware != NULL)
    {
        /* Check required callbacks */
        if (start->service.firmware->callback.data == NULL)
        {
            error = CCAPI_START_ERROR_INVALID_FIRMWARE_DATA_CALLBACK;
            goto done;
        }

        /* If target info is wrong, we won't let CCAPI start */
        if (start->service.firmware->target.item == NULL || start->service.firmware->target.count == 0)
        {
            error = CCAPI_START_ERROR_INVALID_FIRMWARE_INFO;
            goto done;
        }

        {
            size_t const list_size = start->service.firmware->target.count * sizeof *start->service.firmware->target.item;
            unsigned char target_num;
            unsigned char chunk_pool_index;

            ccapi_data->service.firmware_update.config.target.count = start->service.firmware->target.count;

            ccapi_data->service.firmware_update.config.target.item = ccapi_malloc(list_size);
            error = check_malloc(ccapi_data->service.firmware_update.config.target.item);
            if (error != CCAPI_START_ERROR_NONE)
                goto done;

            memcpy(ccapi_data->service.firmware_update.config.target.item, start->service.firmware->target.item, list_size);

            for (target_num = 0; target_num < start->service.firmware->target.count; target_num++)
            {
                size_t const description_size = strlen(start->service.firmware->target.item[target_num].description) + 1;
                size_t const filespec_size = strlen(start->service.firmware->target.item[target_num].filespec) + 1;
                char * description;
                char * filespec;

                description = ccapi_malloc(description_size);
                error = check_malloc(description);
                if (error != CCAPI_START_ERROR_NONE)
                    goto done;
                memcpy(description, start->service.firmware->target.item[target_num].description, description_size);
                ccapi_data->service.firmware_update.config.target.item[target_num].description = description;

                filespec = ccapi_malloc(filespec_size);
                error = check_malloc(filespec);
                if (error != CCAPI_START_ERROR_NONE)
                    goto done;
                memcpy(filespec, start->service.firmware->target.item[target_num].filespec, filespec_size);
                ccapi_data->service.firmware_update.config.target.item[target_num].filespec = filespec;
            }

            ccapi_data->service.firmware_update.config.callback.request = start->service.firmware->callback.request;
            ccapi_data->service.firmware_update.config.callback.data = start->service.firmware->callback.data;
            ccapi_data->service.firmware_update.config.callback.cancel = start->service.firmware->callback.cancel;
            ccapi_data->service.firmware_update.config.callback.reset = start->service.firmware->callback.reset;

            for (chunk_pool_index = 0; chunk_pool_index < ARRAY_SIZE(ccapi_data->service.firmware_update.processing.chunk_pool); chunk_pool_index++)
            {
                ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].data = NULL;
            }

            ccapi_data->service.firmware_update.processing.update_started = CCAPI_FALSE;
            ccapi_data->config.firmware_supported = CCAPI_TRUE;
            stub_fw_update = CCAPI_FALSE;
        }
    }
#if (defined CONNECTOR_RCI_SERVICE)
    else if (start->service.rci != NULL)
    {
        unsigned int const target_count = 1;
        size_t const list_size = target_count * sizeof *ccapi_data->service.firmware_update.config.target.item;
        unsigned char chunk_pool_index;
        uint32_t const rci_target_zero_version = start->service.rci->rci_data->rci_desc->firmware_target_zero_version;

        ccapi_data->service.firmware_update.config.target.count = target_count;
        ccapi_data->service.firmware_update.config.target.item = ccapi_malloc(list_size);
        error = check_malloc(ccapi_data->service.firmware_update.config.target.item);
        if (error != CCAPI_START_ERROR_NONE)
            goto done;

        ccapi_data->service.firmware_update.config.target.item[0].description = NULL;
        ccapi_data->service.firmware_update.config.target.item[0].filespec = NULL;
        ccapi_data->service.firmware_update.config.target.item[0].chunk_size = 0;
        ccapi_data->service.firmware_update.config.target.item[0].maximum_size = 0;

        ccapi_data->service.firmware_update.config.target.item[0].version.major = (rci_target_zero_version & 0xFF000000) >> 24;
        ccapi_data->service.firmware_update.config.target.item[0].version.minor = (rci_target_zero_version & 0x00FF0000) >> 16;
        ccapi_data->service.firmware_update.config.target.item[0].version.revision = (rci_target_zero_version & 0x0000FF00) >> 8;
        ccapi_data->service.firmware_update.config.target.item[0].version.build = (rci_target_zero_version & 0x000000FF);

        ccapi_data->service.firmware_update.config.callback.request = stub_firmware_request_reject_all;
        ccapi_data->service.firmware_update.config.callback.data = NULL;
        ccapi_data->service.firmware_update.config.callback.cancel = NULL;
        ccapi_data->service.firmware_update.config.callback.reset = NULL;

        for (chunk_pool_index = 0; chunk_pool_index < ARRAY_SIZE(ccapi_data->service.firmware_update.processing.chunk_pool); chunk_pool_index++)
        {
            ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].data = NULL;
        }

        ccapi_data->service.firmware_update.processing.update_started = CCAPI_FALSE;
        ccapi_data->config.firmware_supported = CCAPI_TRUE;
        stub_fw_update = CCAPI_TRUE;
    }
#endif
#endif

#if (defined CCIMP_DATA_SERVICE_ENABLED)
    if (start->service.receive != NULL)
    {
        ccapi_data->service.receive.receive_lock = ccapi_lock_create_and_release();
        if (ccapi_data->service.receive.receive_lock == NULL)
        {
            error = CCAPI_START_ERROR_LOCK_FAILED;
            goto done;
        }

        ccapi_data->config.receive_supported = CCAPI_TRUE;
        ccapi_data->service.receive.user_callback.accept = start->service.receive->accept;
        ccapi_data->service.receive.user_callback.data = start->service.receive->data;
        ccapi_data->service.receive.user_callback.status = start->service.receive->status;
        ccapi_data->service.receive.target_list = NULL;
        ccapi_data->service.receive.svc_receive = NULL;
    }
#endif

#if (defined CONNECTOR_SM_CLI)
    if (start->service.cli != NULL)
    {
        /* Check required callbacks */
        if (start->service.cli->request == NULL)
        {
            error = CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK;
            goto done;
        }

        ccapi_data->config.cli_supported = CCAPI_TRUE;
        ccapi_data->service.cli.user_callback.request = start->service.cli->request;
        ccapi_data->service.cli.user_callback.finished = start->service.cli->finished;
        ccapi_data->service.cli.svc_cli = NULL;
    }
#endif

#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)
    if (start->service.sm != NULL)
    {
        ccapi_sm_encryption_t const * encryption = start->service.sm->encryption;

        /* The CCAPI design was meant to allow non-encrypted SM, but problems arose in CCFSM. For now we will require it. */
        if (encryption == NULL)
        {
            error = CCAPI_START_ERROR_INVALID_SM_ENCRYPTION_CALLBACK;
            goto done;
        }

        /* Check required callbacks */
        if (encryption != NULL)
        {
            if ((encryption->load_data == NULL) || (encryption->store_data == NULL) || (encryption->encrypt_gcm == NULL) || (encryption->decrypt_gcm == NULL))
            {
                error = CCAPI_START_ERROR_INVALID_SM_ENCRYPTION_CALLBACK;
                goto done;
            }
        }

		ccapi_data->config.sm_key_distribution_supported = CCAPI_TRUE;
        ccapi_data->config.sm_supported = CCAPI_TRUE;
        ccapi_data->service.sm.user_callback = *start->service.sm;
    }
#endif

#if (defined CONNECTOR_RCI_SERVICE)
    if (start->service.rci != NULL)
    {
        if (start->service.rci->rci_data == NULL)
        {
            error = CCAPI_START_ERROR_INVALID_RCI_REQUEST_CALLBACK;
            goto done;
        }

        ccapi_data->config.rci_supported = CCAPI_TRUE;
        ccapi_data->service.rci.rci_data = start->service.rci->rci_data;
        ccapi_data->service.rci.rci_thread_status = CCAPI_RCI_THREAD_IDLE;
        ccapi_data->service.rci.callback.type = ccapi_callback_type_none;
    }
#endif

#if (defined CCIMP_STREAMING_CLI_SERVICE_ENABLED)
    if (start->service.streaming_cli != NULL)
    {
        ccapi_data->config.streaming_cli_supported = CCAPI_TRUE;
        ccapi_data->service.streaming_cli.user_callback = *start->service.streaming_cli;
    }
#endif

    ccapi_data->connector_handle = connector_init(ccapi_connector_callback, ccapi_data);
    error = check_malloc(ccapi_data->connector_handle);
    if (error != CCAPI_START_ERROR_NONE)
        goto done;

    ccapi_data->transport_tcp.connected = CCAPI_FALSE;
    ccapi_data->transport_tcp.info = NULL;
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
    ccapi_data->transport_udp.started = CCAPI_FALSE;
    ccapi_data->transport_udp.info = NULL;
#endif
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
    ccapi_data->transport_sms.started = CCAPI_FALSE;
    ccapi_data->transport_sms.info = NULL;
#endif

    error = ccapi_create_and_start_thread(ccapi_data, &ccapi_data->thread.connector_run, ccapi_connector_run_thread, CCIMP_THREAD_FSM);
    if (error != CCAPI_START_ERROR_NONE)
    {
        goto done;
    }

#if (defined CCIMP_RCI_SERVICE_ENABLED)
    if (ccapi_data->config.rci_supported)
    {
        error = ccapi_create_and_start_thread(ccapi_data, &ccapi_data->thread.rci, ccapi_rci_thread, CCIMP_THREAD_RCI);
        if (error != CCAPI_START_ERROR_NONE)
        {
            goto done;
        }
    }
#endif

#if (defined CCIMP_DATA_SERVICE_ENABLED)
    if (ccapi_data->config.receive_supported)
    {
        error = ccapi_create_and_start_thread(ccapi_data, &ccapi_data->thread.receive, ccapi_receive_thread, CCIMP_THREAD_RECEIVE);
        if (error != CCAPI_START_ERROR_NONE)
        {
            goto done;
        }
    }
#endif

#if (defined CONNECTOR_SM_CLI)
    if (ccapi_data->config.cli_supported)
    {
        error = ccapi_create_and_start_thread(ccapi_data, &ccapi_data->thread.cli, ccapi_cli_thread, CCIMP_THREAD_CLI);
        if (error != CCAPI_START_ERROR_NONE)
        {
            goto done;
        }
    }
#endif

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    if (ccapi_data->config.firmware_supported && !stub_fw_update)
    {
        error = ccapi_create_and_start_thread(ccapi_data, &ccapi_data->thread.firmware, ccapi_firmware_thread, CCIMP_THREAD_FIRMWARE);
        if (error != CCAPI_START_ERROR_NONE)
        {
            goto done;
        }
    }
#endif

    ccapi_data->initiate_action_lock = ccapi_lock_create_and_release();
    if (ccapi_data->initiate_action_lock == NULL)
    {
        error = CCAPI_START_ERROR_LOCK_FAILED;
        goto done;
    }

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
    ccapi_data->file_system_lock = ccapi_lock_create_and_release();
    if (ccapi_data->file_system_lock == NULL)
    {
        error = CCAPI_START_ERROR_LOCK_FAILED;
        goto done;
    }
#endif

done:
    switch (error)
    {
        case CCAPI_START_ERROR_NONE:
            break;
        case CCAPI_START_ERROR_NULL_PARAMETER:
        case CCAPI_START_ERROR_INVALID_VENDORID:
        case CCAPI_START_ERROR_INVALID_DEVICEID:
        case CCAPI_START_ERROR_INVALID_URL:
        case CCAPI_START_ERROR_INVALID_DEVICETYPE:
        case CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK:
        case CCAPI_START_ERROR_INVALID_RCI_REQUEST_CALLBACK:
        case CCAPI_START_ERROR_INVALID_FIRMWARE_INFO:
        case CCAPI_START_ERROR_INVALID_FIRMWARE_DATA_CALLBACK:
        case CCAPI_START_ERROR_INVALID_SM_ENCRYPTION_CALLBACK:
        case CCAPI_START_ERROR_INSUFFICIENT_MEMORY:
        case CCAPI_START_ERROR_THREAD_FAILED:
        case CCAPI_START_ERROR_LOCK_FAILED:
        case CCAPI_START_ERROR_ALREADY_STARTED:
            if (ccapi_data != NULL)
            {
                free_ccapi_data_internal_resources(ccapi_data);
                ccapi_free(ccapi_data);
            }
#if (defined CCIMP_DEBUG_ENABLED)
            free_logging_resources();
#endif
            break;
    }

    ccapi_logging_line("ccapi_start ret %d", error);

    return error;
}

static ccapi_transport_stop_t ccapi_stop_to_ccapi_transport_stop(ccapi_stop_t const stop_behavior)
{
    ccapi_transport_stop_t transport_stop_behavior = INVALID_ENUM(ccapi_transport_stop_t);

    switch (stop_behavior)
    {
        case CCAPI_STOP_GRACEFULLY:
            transport_stop_behavior = CCAPI_TRANSPORT_STOP_GRACEFULLY;
            break;
        case CCAPI_STOP_IMMEDIATELY:
            transport_stop_behavior = CCAPI_TRANSPORT_STOP_IMMEDIATELY;
            break;
    }
    return transport_stop_behavior;
}

void ccxapi_asynchronous_stop(ccapi_data_t * const ccapi_data)
{
    if (ccapi_data->transport_tcp.info != NULL)
    {
        free_transport_tcp_info(ccapi_data->transport_tcp.info);
    }

#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
    if (ccapi_data->transport_udp.info != NULL)
    {
        free_transport_udp_info(ccapi_data->transport_udp.info);
    }
#endif

#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
    if (ccapi_data->transport_sms.info != NULL)
    {
        free_transport_sms_info(ccapi_data->transport_sms.info);
    }
#endif

#if (defined CCIMP_RCI_SERVICE_ENABLED)
    if (ccapi_data->config.rci_supported)
    {
        ccapi_stop_thread(ccapi_data->thread.rci);
    }
#endif

#if (defined CCIMP_DATA_SERVICE_ENABLED)
    if (ccapi_data->config.receive_supported)
    {
        ccapi_stop_thread(ccapi_data->thread.receive);
    }
#endif

#if (defined CONNECTOR_SM_CLI)
    if (ccapi_data->config.cli_supported)
    {
        ccapi_stop_thread(ccapi_data->thread.cli);
    }
#endif

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    if (ccapi_data->config.firmware_supported)
    {
        /* Do not attempt to stop the thread if firmware service is the stub version for RCI. */
        if (ccapi_data->thread.firmware != NULL)
        {
            ccapi_stop_thread(ccapi_data->thread.firmware);
        }
    }
#endif

    free_ccapi_data_internal_resources(ccapi_data);
    ccapi_free(ccapi_data);
#if (defined CCIMP_DEBUG_ENABLED)
    free_logging_resources();
#endif
}

ccapi_stop_error_t ccxapi_stop(ccapi_handle_t const ccapi_handle, ccapi_stop_t const behavior)
{
    ccapi_stop_error_t error = CCAPI_STOP_ERROR_NOT_STARTED;
    ccapi_data_t * const ccapi_data = (ccapi_data_t *)ccapi_handle;

    if (!CCAPI_RUNNING(ccapi_data))
    {
        goto done;
    }

    if (ccapi_data->transport_tcp.connected)
    {
        ccapi_tcp_stop_t tcp_stop;
        ccapi_tcp_stop_error_t tcp_stop_error;

        tcp_stop.behavior = ccapi_stop_to_ccapi_transport_stop(behavior);
        tcp_stop_error = ccxapi_stop_transport_tcp(ccapi_handle, &tcp_stop);
        switch(tcp_stop_error)
        {
            case CCAPI_TCP_STOP_ERROR_NONE:
                break;
            case CCAPI_TCP_STOP_ERROR_CCFSM:
                ccapi_logging_line("ccapi_stop: failed to stop TCP transport!");
                break;
            case CCAPI_TCP_STOP_ERROR_NOT_STARTED:
                ASSERT_MSG(tcp_stop_error != CCAPI_TCP_STOP_ERROR_NOT_STARTED);
                break;
        }
    }
    if (ccapi_data->transport_tcp.info != NULL)
    {
        free_transport_tcp_info(ccapi_data->transport_tcp.info);
    }

#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
    if (ccapi_data->transport_udp.started)
    {
        ccapi_udp_stop_t udp_stop;
        ccapi_udp_stop_error_t udp_stop_error;

        udp_stop.behavior = ccapi_stop_to_ccapi_transport_stop(behavior);
        udp_stop_error = ccxapi_stop_transport_udp(ccapi_handle, &udp_stop);
        switch(udp_stop_error)
        {
            case CCAPI_UDP_STOP_ERROR_NONE:
                break;
            case CCAPI_UDP_STOP_ERROR_CCFSM:
                ccapi_logging_line("ccapi_stop: failed to stop UDP transport!");
                break;
            case CCAPI_UDP_STOP_ERROR_NOT_STARTED:
                ASSERT_MSG(udp_stop_error != CCAPI_UDP_STOP_ERROR_NONE);
        }
    }
    if (ccapi_data->transport_udp.info != NULL)
    {
        free_transport_udp_info(ccapi_data->transport_udp.info);
    }
#endif

#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
    if (ccapi_data->transport_sms.started)
    {
        ccapi_sms_stop_t sms_stop;
        ccapi_sms_stop_error_t sms_stop_error;

        sms_stop.behavior = ccapi_stop_to_ccapi_transport_stop(behavior);
        sms_stop_error = ccxapi_stop_transport_sms(ccapi_handle, &sms_stop);
        switch(sms_stop_error)
        {
            case CCAPI_SMS_STOP_ERROR_NONE:
                break;
            case CCAPI_SMS_STOP_ERROR_CCFSM:
                ccapi_logging_line("ccapi_stop: failed to stop SMS transport!");
                break;
            case CCAPI_SMS_STOP_ERROR_NOT_STARTED:
                ASSERT_MSG(sms_stop_error != CCAPI_SMS_STOP_ERROR_NONE);
        }
    }
    if (ccapi_data->transport_sms.info != NULL)
    {
        free_transport_sms_info(ccapi_data->transport_sms.info);
    }
#endif

    {
        connector_status_t ccfsm_status;

        for (;;)
        {
            ccfsm_status = connector_initiate_action_secure(ccapi_data, connector_initiate_terminate, NULL);
            if (ccfsm_status != connector_service_busy)
            {
                break;
            }
            ccimp_os_yield();
        }

        switch(ccfsm_status)
        {
            case connector_success:
                error = CCAPI_STOP_ERROR_NONE;
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
                ASSERT_MSG_GOTO(ccfsm_status != connector_success, done);
                break;
        }
    }

    do {
        ccimp_os_yield();
    } while (ccapi_data->thread.connector_run->status != CCAPI_THREAD_NOT_STARTED);

    ccapi_stop_thread(ccapi_data->thread.connector_run);

#if (defined CCIMP_RCI_SERVICE_ENABLED)
    if (ccapi_data->config.rci_supported)
    {
        ccapi_stop_thread(ccapi_data->thread.rci);
    }
#endif

#if (defined CCIMP_DATA_SERVICE_ENABLED)
    if (ccapi_data->config.receive_supported)
    {
        ccapi_stop_thread(ccapi_data->thread.receive);
    }
#endif

#if (defined CONNECTOR_SM_CLI)
    if (ccapi_data->config.cli_supported)
    {
        ccapi_stop_thread(ccapi_data->thread.cli);
    }
#endif

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
    if (ccapi_data->config.firmware_supported)
    {
        /* Do not attempt to stop the thread if firmware service is the stub version for RCI. */
        if (ccapi_data->thread.firmware != NULL)
        {
            ccapi_stop_thread(ccapi_data->thread.firmware);
        }
    }
#endif

done:
    switch (error)
    {
        case CCAPI_STOP_ERROR_NONE:
            free_ccapi_data_internal_resources(ccapi_data);
            ccapi_free(ccapi_data);
#if (defined CCIMP_DEBUG_ENABLED)
            free_logging_resources();
#endif
            break;
        case CCAPI_STOP_ERROR_NOT_STARTED:
            break;
    }

    return error;
}

ccapi_start_error_t ccapi_start(ccapi_start_t const * const start)
{
    ccapi_start_error_t error;

    error = ccxapi_start((ccapi_handle_t *)&ccapi_data_single_instance, start);

    switch (error)
    {
        case CCAPI_START_ERROR_NONE:
            break;
        case CCAPI_START_ERROR_NULL_PARAMETER:
        case CCAPI_START_ERROR_INVALID_VENDORID:
        case CCAPI_START_ERROR_INVALID_DEVICEID:
        case CCAPI_START_ERROR_INVALID_URL:
        case CCAPI_START_ERROR_INVALID_DEVICETYPE:
        case CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK:
        case CCAPI_START_ERROR_INVALID_RCI_REQUEST_CALLBACK:
        case CCAPI_START_ERROR_INVALID_FIRMWARE_INFO:
        case CCAPI_START_ERROR_INVALID_FIRMWARE_DATA_CALLBACK:
        case CCAPI_START_ERROR_INVALID_SM_ENCRYPTION_CALLBACK:
        case CCAPI_START_ERROR_INSUFFICIENT_MEMORY:
        case CCAPI_START_ERROR_THREAD_FAILED:
        case CCAPI_START_ERROR_LOCK_FAILED:
        case CCAPI_START_ERROR_ALREADY_STARTED:
            ccapi_data_single_instance = NULL;
            break;
    }

    return error;
}

ccapi_stop_error_t ccapi_stop(ccapi_stop_t const behavior)
{
    ccapi_stop_error_t error;

    error = ccxapi_stop((ccapi_handle_t)ccapi_data_single_instance, behavior);
    switch (error)
    {
        case CCAPI_STOP_ERROR_NONE:
            ccapi_data_single_instance = NULL;
            break;
        case CCAPI_STOP_ERROR_NOT_STARTED:
            break;
    }

    return error;
}
