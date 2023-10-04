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

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)

static void free_and_stop_service(ccapi_data_t * const ccapi_data)
{
    unsigned char chunk_pool_index;

    for (chunk_pool_index = 0; chunk_pool_index < ARRAY_SIZE(ccapi_data->service.firmware_update.processing.chunk_pool); chunk_pool_index++)
    {
        if (ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].data != NULL)
        {
            ccapi_free(ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].data);
            ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].data = NULL;
        }
    }

    ccapi_data->service.firmware_update.processing.update_started = CCAPI_FALSE;
}

void ccapi_firmware_thread(void * const argument)
{
    ccapi_data_t * const ccapi_data = argument;

    /* ccapi_data is corrupted, it's likely the implementer made it wrong passing argument to the new thread */
    ASSERT_MSG_GOTO(ccapi_data != NULL, done);

    ASSERT_MSG_GOTO(ccapi_data->service.firmware_update.config.callback.data != NULL, done);

    ASSERT_MSG_GOTO(ccapi_data->thread.firmware->lock != NULL, done);

    ccapi_data->thread.firmware->status = CCAPI_THREAD_RUNNING;
    while (ccapi_data->thread.firmware->status == CCAPI_THREAD_RUNNING)
    {
        ccapi_lock_acquire(ccapi_data->thread.firmware->lock);

        if (ccapi_data->thread.firmware->status != CCAPI_THREAD_REQUEST_STOP)
        {
            ccapi_fw_data_error_t ccapi_fw_data_error;
            ccapi_fw_chunk_info * const chunk_pool = ccapi_data->service.firmware_update.processing.chunk_pool;
            ccapi_fw_chunk_info * const chunk_pool_tail = &chunk_pool[ccapi_data->service.firmware_update.processing.chunk_pool_tail];

            ASSERT_MSG(chunk_pool_tail->in_use == CCAPI_TRUE);

            ccapi_logging_line("+ccapi_firmware_thread: processing pool=%d, offset=0x%x, size=%d, last=%d", ccapi_data->service.firmware_update.processing.chunk_pool_tail, chunk_pool_tail->offset, chunk_pool_tail->size, chunk_pool_tail->last);
            ccapi_fw_data_error = ccapi_data->service.firmware_update.config.callback.data(ccapi_data->service.firmware_update.processing.target,
                                                                                                         chunk_pool_tail->offset, chunk_pool_tail->data, chunk_pool_tail->size, chunk_pool_tail->last);
            switch (ccapi_fw_data_error)
            {
                case CCAPI_FW_DATA_ERROR_NONE:
                    break;    
                case CCAPI_FW_DATA_ERROR_INVALID_DATA:
                    ccapi_data->service.firmware_update.processing.data_error = ccapi_fw_data_error;
                    break;    
            }

            chunk_pool_tail->in_use = CCAPI_FALSE;
            ccapi_data->service.firmware_update.processing.chunk_pool_tail++;
            if (ccapi_data->service.firmware_update.processing.chunk_pool_tail == CCAPI_CHUNK_POOL_SIZE)
            {
                ccapi_data->service.firmware_update.processing.chunk_pool_tail = 0;
            }

            ccapi_logging_line("-ccapi_firmware_thread");
        }
    }
    ASSERT_MSG_GOTO(ccapi_data->thread.firmware->status == CCAPI_THREAD_REQUEST_STOP, done);

done:
    ccapi_data->thread.firmware->status = CCAPI_THREAD_NOT_STARTED;
    return;
}

static connector_callback_status_t ccapi_process_firmware_update_request(connector_firmware_download_start_t * const start_ptr, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_error;
    ccapi_fw_request_error_t ccapi_fw_request_error;
    ccapi_firmware_target_t * fw_target_item = NULL;

    ASSERT_MSG_GOTO(start_ptr->target_number < ccapi_data->service.firmware_update.config.target.count, done);

    connector_status = connector_callback_continue;
    start_ptr->status = connector_firmware_status_success;

    ccapi_logging_line("ccapi_process_firmware_update_request for target_number='%d'. code_size='%d'", start_ptr->target_number, start_ptr->code_size);

    if (ccapi_data->service.firmware_update.processing.update_started)
    {
        free_and_stop_service(ccapi_data);

        start_ptr->status = connector_firmware_status_encountered_error;
        goto done;
    }

    fw_target_item = &ccapi_data->service.firmware_update.config.target.item[start_ptr->target_number];

    if (fw_target_item->maximum_size != 0 && 
        fw_target_item->maximum_size < start_ptr->code_size)
    {
        start_ptr->status = connector_firmware_status_download_invalid_size;
        goto done;
    }

    if (ccapi_data->service.firmware_update.config.callback.request != NULL)
    {
        ccapi_fw_request_error = ccapi_data->service.firmware_update.config.callback.request(start_ptr->target_number, start_ptr->filename, start_ptr->code_size);

        switch (ccapi_fw_request_error)
        {
            case CCAPI_FW_REQUEST_ERROR_NONE:
                break;
            case CCAPI_FW_REQUEST_ERROR_DOWNLOAD_DENIED:
                start_ptr->status = connector_firmware_status_download_denied;
                goto done;
                break;
            case CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_SIZE:
                start_ptr->status = connector_firmware_status_download_invalid_size;
                goto done;
                break;
            case CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_VERSION:
                start_ptr->status = connector_firmware_status_download_invalid_version;
                goto done;
                break;
            case CCAPI_FW_REQUEST_ERROR_DOWNLOAD_UNAUTHENTICATED:
                start_ptr->status = connector_firmware_status_download_unauthenticated;
                goto done;
                break;
            case CCAPI_FW_REQUEST_ERROR_DOWNLOAD_NOT_ALLOWED:
                start_ptr->status = connector_firmware_status_download_not_allowed;
                goto done;
                break;
            case CCAPI_FW_REQUEST_ERROR_DOWNLOAD_CONFIGURED_TO_REJECT:
                start_ptr->status = connector_firmware_status_download_configured_to_reject;
                goto done;
                break;
            case CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR:
                start_ptr->status = connector_firmware_status_encountered_error;
                goto done;
                break;
        }
    } 

    if (fw_target_item->chunk_size == 0)
    {
        fw_target_item->chunk_size = 1024;
    }

    {
        ccapi_fw_chunk_info * const chunk_pool = ccapi_data->service.firmware_update.processing.chunk_pool;
        unsigned char chunk_pool_index;

        for (chunk_pool_index = 0; chunk_pool_index < ARRAY_SIZE(ccapi_data->service.firmware_update.processing.chunk_pool); chunk_pool_index++)
        {
            chunk_pool[chunk_pool_index].in_use = CCAPI_FALSE;
            chunk_pool[chunk_pool_index].offset = 0;

            chunk_pool[chunk_pool_index].data = ccapi_malloc(fw_target_item->chunk_size);
            if (chunk_pool[chunk_pool_index].data == NULL)
            {
                start_ptr->status = connector_firmware_status_encountered_error;
                goto done;
            }

            chunk_pool[chunk_pool_index].size = 0;
            chunk_pool[chunk_pool_index].last = CCAPI_FALSE;
        }
    }
    ccapi_data->service.firmware_update.processing.chunk_pool_head = 0;
    ccapi_data->service.firmware_update.processing.chunk_pool_tail = 0;

    ccapi_data->service.firmware_update.processing.target = start_ptr->target_number;
    ccapi_data->service.firmware_update.processing.total_size = start_ptr->code_size;
    ccapi_data->service.firmware_update.processing.tail_offset = 0;
    ccapi_data->service.firmware_update.processing.head_offset = 0;
    ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed = 0;
    ccapi_data->service.firmware_update.processing.data_error = CCAPI_FW_DATA_ERROR_NONE;
    ccapi_data->service.firmware_update.processing.update_started = CCAPI_TRUE;

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_firmware_update_data(connector_firmware_download_data_t * const data_ptr, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_error;

    ASSERT_MSG_GOTO(data_ptr->target_number == ccapi_data->service.firmware_update.processing.target, done);

    ASSERT_MSG_GOTO(ccapi_data->service.firmware_update.processing.update_started, done);

    connector_status = connector_callback_continue;
    data_ptr->status = connector_firmware_status_success;

    if (ccapi_data->service.firmware_update.processing.data_error == CCAPI_FW_DATA_ERROR_INVALID_DATA)
    {
        ccapi_logging_line("Invalid data!");

        data_ptr->status = connector_firmware_status_invalid_data;
        goto done;
    }

    if (data_ptr->image.offset != ccapi_data->service.firmware_update.processing.tail_offset - ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed)
    {
        ccapi_logging_line("Out of order packet: offset 0x%x != 0x%x !", data_ptr->image.offset, ccapi_data->service.firmware_update.processing.tail_offset);

        data_ptr->status = connector_firmware_status_invalid_offset;
        goto done;
    }

    if (ccapi_data->service.firmware_update.processing.chunk_pool[ccapi_data->service.firmware_update.processing.chunk_pool_head].in_use == CCAPI_FALSE)
    {
        uint32_t const chunk_size = ccapi_data->service.firmware_update.config.target.item[data_ptr->target_number].chunk_size;
        uint32_t const tail_offset = ccapi_data->service.firmware_update.processing.tail_offset;
        uint32_t const room_in_chunk = chunk_size - tail_offset % chunk_size;
        uint8_t const * const source_data = data_ptr->image.data + ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed;
        size_t const source_bytes_remaining = data_ptr->image.bytes_used - ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed;
        uint32_t const bytes_to_copy = source_bytes_remaining > room_in_chunk ? room_in_chunk : source_bytes_remaining;
        uint32_t const next_head_offset = tail_offset + bytes_to_copy;
        ccapi_bool_t const last_chunk = CCAPI_BOOL(next_head_offset == ccapi_data->service.firmware_update.processing.total_size);

        ASSERT_MSG(bytes_to_copy != 0);

        /* ccapi_logging_line("ccapi_process_fw_data target_number=%d, offset=0x%x, size=%d", data_ptr->target_number, data_ptr->image.offset, data_ptr->image.bytes_used); */

        if (tail_offset >= ccapi_data->service.firmware_update.processing.head_offset)
        {
            ccapi_fw_chunk_info * const chunk_pool = ccapi_data->service.firmware_update.processing.chunk_pool;
            ccapi_fw_chunk_info * const chunk_pool_head = &chunk_pool[ccapi_data->service.firmware_update.processing.chunk_pool_head];

            memcpy(&chunk_pool_head->data[tail_offset % chunk_size], source_data, bytes_to_copy);

            if (last_chunk || next_head_offset % chunk_size == 0)
            {
                chunk_pool_head->offset = ccapi_data->service.firmware_update.processing.head_offset;
                chunk_pool_head->size = next_head_offset - ccapi_data->service.firmware_update.processing.head_offset;
                ccapi_data->service.firmware_update.processing.head_offset = next_head_offset;
                chunk_pool_head->last = last_chunk;
                ccapi_logging_line("ccapi_process_fw_data queued: pool=%d, offset=0x%x, size=%d, last=%d", ccapi_data->service.firmware_update.processing.chunk_pool_head, chunk_pool_head->offset, chunk_pool_head->size, chunk_pool_head->last);
                chunk_pool_head->in_use = CCAPI_TRUE;
                ccapi_lock_release(ccapi_data->thread.firmware->lock);
                ccapi_data->service.firmware_update.processing.chunk_pool_head++;
                if (ccapi_data->service.firmware_update.processing.chunk_pool_head == CCAPI_CHUNK_POOL_SIZE)
                {
                    ccapi_data->service.firmware_update.processing.chunk_pool_head = 0;
                }
            }

            ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed += bytes_to_copy;
            if (ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed == data_ptr->image.bytes_used)
            {
                ccapi_data->service.firmware_update.processing.ccfsm_bytes_processed = 0;
            }
            else
            {
                connector_status = connector_callback_busy;
            }
        }
        ccapi_data->service.firmware_update.processing.tail_offset = next_head_offset;
    } 
    else
    {
        connector_status = connector_callback_busy;
    }

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_firmware_update_complete(connector_firmware_download_complete_t * const complete_ptr, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_error;
    unsigned char chunk_pool_index;

    ASSERT_MSG_GOTO(complete_ptr->target_number == ccapi_data->service.firmware_update.processing.target, done);

    ASSERT_MSG_GOTO(ccapi_data->service.firmware_update.processing.update_started, done);

    complete_ptr->status = connector_firmware_download_success;

    for (chunk_pool_index = 0; chunk_pool_index < ARRAY_SIZE(ccapi_data->service.firmware_update.processing.chunk_pool); chunk_pool_index++)
    {
        if (ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].in_use)
        {
            connector_status = connector_callback_busy;
            goto done;
        }
    }

    ccapi_logging_line("ccapi_process_firmware_update_complete for target_number='%d'", complete_ptr->target_number);

    if (ccapi_data->service.firmware_update.processing.head_offset != ccapi_data->service.firmware_update.processing.total_size)
    {
        ccapi_logging_line("update_complete arrived before all firmware data arrived!");

        complete_ptr->status = connector_firmware_download_not_complete;
    }

    if (ccapi_data->service.firmware_update.processing.data_error == CCAPI_FW_DATA_ERROR_INVALID_DATA)
    {
        ccapi_logging_line("Invalid data!");
    }
    else
    {
        connector_status = connector_callback_continue;
    }

    free_and_stop_service(ccapi_data);
    
done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_firmware_update_abort(connector_firmware_download_abort_t * const abort_ptr, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_error;
    unsigned char chunk_pool_index;

    ASSERT_MSG_GOTO(abort_ptr->target_number == ccapi_data->service.firmware_update.processing.target, done);

    ASSERT_MSG_GOTO(ccapi_data->service.firmware_update.processing.update_started, done);

    connector_status = connector_callback_continue;

    ccapi_logging_line("ccapi_process_firmware_update_abort for target_number='%d'. status='%d'", abort_ptr->target_number, abort_ptr->status);

    for (chunk_pool_index = 0; chunk_pool_index < ARRAY_SIZE(ccapi_data->service.firmware_update.processing.chunk_pool); chunk_pool_index++)
    {
        if (ccapi_data->service.firmware_update.processing.chunk_pool[chunk_pool_index].in_use)
        {
            connector_status = connector_callback_busy;
            goto done;
        }
    }

    if (ccapi_data->service.firmware_update.config.callback.cancel != NULL)
    {
        ccapi_fw_cancel_error_t cancel_reason = INVALID_ENUM(ccapi_fw_cancel_error_t);

        switch (abort_ptr->status)
        {
            case connector_firmware_status_user_abort:
                cancel_reason = CCAPI_FW_CANCEL_USER_ABORT;
                break;
            case connector_firmware_status_device_error:
                cancel_reason = CCAPI_FW_CANCEL_DEVICE_ERROR;
                break;
            case connector_firmware_status_invalid_offset:
                cancel_reason = CCAPI_FW_CANCEL_INVALID_OFFSET;
                break;
            case connector_firmware_status_invalid_data:
                cancel_reason = CCAPI_FW_CANCEL_INVALID_DATA;
                break;
            case connector_firmware_status_hardware_error:
                cancel_reason = CCAPI_FW_CANCEL_HARDWARE_ERROR;
                break;
            case connector_firmware_status_success:
            case connector_firmware_status_download_denied:
            case connector_firmware_status_download_invalid_size:
            case connector_firmware_status_download_invalid_version:
            case connector_firmware_status_download_unauthenticated:
            case connector_firmware_status_download_not_allowed:
            case connector_firmware_status_download_configured_to_reject:
            case connector_firmware_status_encountered_error:
                connector_status = connector_callback_error;
                ASSERT_MSG_GOTO(abort_ptr->status >= connector_firmware_status_user_abort, done);
        }

        ccapi_data->service.firmware_update.config.callback.cancel(abort_ptr->target_number, cancel_reason);
    }

    free_and_stop_service(ccapi_data);

done:
    return connector_status;
}

static connector_callback_status_t ccapi_process_firmware_update_reset(connector_firmware_reset_t * const reset_ptr, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_error;
    ccapi_bool_t system_reset = CCAPI_TRUE;

    ASSERT_MSG_GOTO(reset_ptr->target_number == ccapi_data->service.firmware_update.processing.target, done);

    connector_status = connector_callback_continue;

    ccapi_logging_line("ccapi_process_firmware_update_reset for target_number='%d'", reset_ptr->target_number);

    if (ccapi_data->service.firmware_update.config.callback.reset != NULL)
    {
        ccapi_firmware_target_version_t * version = &ccapi_data->service.firmware_update.config.target.item[reset_ptr->target_number].version;

        ccapi_data->service.firmware_update.config.callback.reset(reset_ptr->target_number, &system_reset, version);
    }

    if (system_reset)
    {
        ccimp_status_t const ccimp_status = ccimp_hal_reset();

        connector_status = connector_callback_status_from_ccimp_status(ccimp_status);
    }

done:
    return connector_status;
}

connector_callback_status_t ccapi_firmware_service_handler(connector_request_id_firmware_t const firmware_service_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status = connector_callback_error;

    switch (firmware_service_request)
    {
        case connector_request_id_firmware_target_count:
        {
            connector_firmware_count_t * const count_ptr = data;

            ASSERT_MSG_GOTO(ccapi_data->config.firmware_supported, done);

            count_ptr->count = ccapi_data->service.firmware_update.config.target.count;
           
            connector_status = connector_callback_continue;

            break;
        }
        case connector_request_id_firmware_info:
        {
            connector_firmware_info_t * const info_ptr = data;

            ASSERT_MSG_GOTO(ccapi_data->config.firmware_supported, done);

            ASSERT_MSG_GOTO(info_ptr->target_number < ccapi_data->service.firmware_update.config.target.count, done);

            info_ptr->version.major = ccapi_data->service.firmware_update.config.target.item[info_ptr->target_number].version.major;
            info_ptr->version.minor = ccapi_data->service.firmware_update.config.target.item[info_ptr->target_number].version.minor;
            info_ptr->version.revision = ccapi_data->service.firmware_update.config.target.item[info_ptr->target_number].version.revision;
            info_ptr->version.build = ccapi_data->service.firmware_update.config.target.item[info_ptr->target_number].version.build;

            info_ptr->description = ccapi_data->service.firmware_update.config.target.item[info_ptr->target_number].description;
            info_ptr->filespec = ccapi_data->service.firmware_update.config.target.item[info_ptr->target_number].filespec;
           
            connector_status = connector_callback_continue;

            break;
        }
        case connector_request_id_firmware_download_start:
        {
            connector_firmware_download_start_t * const start_ptr = data;

            connector_status = ccapi_process_firmware_update_request(start_ptr, ccapi_data);

            break;
        }
        case connector_request_id_firmware_download_data:
        {
            connector_firmware_download_data_t * const data_ptr = data;

            connector_status = ccapi_process_firmware_update_data(data_ptr, ccapi_data);

            break;
        }
        case connector_request_id_firmware_download_complete:
        {
            connector_firmware_download_complete_t * const complete_ptr = data;

            connector_status = ccapi_process_firmware_update_complete(complete_ptr, ccapi_data);

            break;
        }
        case connector_request_id_firmware_download_abort:
        {
            connector_firmware_download_abort_t * const abort_ptr = data;

            connector_status = ccapi_process_firmware_update_abort(abort_ptr, ccapi_data);

            break;
        }
        case connector_request_id_firmware_target_reset:
        {
            connector_firmware_reset_t * const reset_ptr = data;

            connector_status = ccapi_process_firmware_update_reset(reset_ptr, ccapi_data);

            break;  
        }
    }

    ASSERT_MSG_GOTO(connector_status != connector_callback_unrecognized, done);

done:
    return connector_status;
}
#endif
