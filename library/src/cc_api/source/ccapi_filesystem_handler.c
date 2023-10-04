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

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)

typedef struct {
    ccimp_fs_file_handle_t ccimp_handle;
    char * file_path;
    ccapi_fs_request_t request;
} ccapi_fs_file_handle_t;

typedef struct {
    ccapi_fs_virtual_dir_t * dir_entry;
} ccapi_fs_virtual_rootdir_listing_handle_t;

static ccimp_fs_seek_origin_t ccimp_seek_origin_from_ccfsm_seek_origin(connector_file_system_seek_origin_t const ccfsm_origin)
{
    ccimp_fs_seek_origin_t ccimp_fs_seek = CCIMP_SEEK_CUR;

    switch (ccfsm_origin)
    {
        case connector_file_system_seek_cur:
            ccimp_fs_seek = CCIMP_SEEK_CUR;
            break;
        case connector_file_system_seek_end:
            ccimp_fs_seek = CCIMP_SEEK_END;
            break;
        case connector_file_system_seek_set:
            ccimp_fs_seek =  CCIMP_SEEK_SET;
            break;
    }

    return ccimp_fs_seek;
}

static connector_file_system_file_type_t ccfsm_file_system_file_type_from_ccimp_fs_dir_entry_type(ccimp_fs_dir_entry_type_t const ccimp_entry_type)
{
    connector_file_system_file_type_t ccfsm_file_type = connector_file_system_file_type_none;

    switch (ccimp_entry_type)
    {
        case CCIMP_FS_DIR_ENTRY_UNKNOWN:
            ccfsm_file_type = connector_file_system_file_type_none;
            break;
        case CCIMP_FS_DIR_ENTRY_FILE:
            ccfsm_file_type = connector_file_system_file_type_is_reg;
            break;
        case CCIMP_FS_DIR_ENTRY_DIR:
            ccfsm_file_type = connector_file_system_file_type_is_dir;
            break;
    }

    return ccfsm_file_type;
}

static connector_file_system_hash_algorithm_t ccfsm_file_system_hash_algorithm_from_ccimp_fs_hash_alg(ccimp_fs_hash_alg_t const ccimp_hash_alg)
{
    connector_file_system_hash_algorithm_t ccfsm_hash_algorithm = connector_file_system_hash_none;

    switch (ccimp_hash_alg)
    {
        case CCIMP_FS_HASH_NONE:
            ccfsm_hash_algorithm = connector_file_system_hash_none;
            break;
        case CCIMP_FS_HASH_BEST:
            ccfsm_hash_algorithm = connector_file_system_hash_best;
            break;
        case CCIMP_FS_HASH_CRC32:
            ccfsm_hash_algorithm = connector_file_system_hash_crc32;
            break;
        case CCIMP_FS_HASH_MD5:
            ccfsm_hash_algorithm = connector_file_system_hash_md5;
            break;
        case CCIMP_FS_HASH_SHA512:
            ccfsm_hash_algorithm = connector_file_system_hash_sha512;
            break;
        case CCIMP_FS_HASH_SHA3_512:
            ccfsm_hash_algorithm = connector_file_system_hash_sha3_512;
            break;
    }

    return ccfsm_hash_algorithm;
}

static ccimp_fs_hash_alg_t ccimp_fs_hash_alg_from_ccfsm_file_system_hash_algorithm(connector_file_system_hash_algorithm_t const ccimp_hash_alg)
{
    ccimp_fs_hash_alg_t ccimp_hash_algorithm = INVALID_ENUM(ccimp_fs_hash_alg_t);

    switch (ccimp_hash_alg)
    {
        case connector_file_system_hash_none:
            ccimp_hash_algorithm = CCIMP_FS_HASH_NONE;
            break;
        case connector_file_system_hash_best:
            ccimp_hash_algorithm = CCIMP_FS_HASH_BEST;
            break;
        case connector_file_system_hash_crc32:
            ccimp_hash_algorithm = CCIMP_FS_HASH_CRC32;
            break;
        case connector_file_system_hash_md5:
            ccimp_hash_algorithm = CCIMP_FS_HASH_MD5;
            break;
        case connector_file_system_hash_sha512:
            ccimp_hash_algorithm = CCIMP_FS_HASH_SHA512;
            break;
        case connector_file_system_hash_sha3_512:
            ccimp_hash_algorithm = CCIMP_FS_HASH_SHA3_512;
            break;
    }
    
    return ccimp_hash_algorithm;
}

static connector_file_system_error_t ccfsm_file_system_error_status_from_ccimp_fs_error(ccimp_fs_error_t const ccimp_fs_error)
{
    connector_file_system_error_t ccfsm_fs_error = connector_file_system_unspec_error;

    switch(ccimp_fs_error)
    {
        case CCIMP_FS_ERROR_UNKNOWN:
            ccfsm_fs_error = connector_file_system_unspec_error;
            break;
        case CCIMP_FS_ERROR_PATH_NOT_FOUND:
            ccfsm_fs_error = connector_file_system_path_not_found;
            break;
        case CCIMP_FS_ERROR_INSUFFICIENT_SPACE:
            ccfsm_fs_error = connector_file_system_insufficient_storage_space;
            break;
        case CCIMP_FS_ERROR_FORMAT:
            ccfsm_fs_error = connector_file_system_request_format_error;
            break;
        case CCIMP_FS_ERROR_INVALID_PARAMETER:
            ccfsm_fs_error = connector_file_system_invalid_parameter;
            break;
        case CCIMP_FS_ERROR_INSUFFICIENT_MEMORY:
            ccfsm_fs_error = connector_file_system_out_of_memory;
            break;
        case CCIMP_FS_ERROR_PERMISSION_DENIED:
            ccfsm_fs_error = connector_file_system_permission_denied;
            break;
    }
    return ccfsm_fs_error;
}

static ccimp_session_error_status_t ccimp_session_error_from_ccfsm_session_error(connector_session_error_t const ccfsm_session_error)
{
    ccimp_session_error_status_t cccimp_session_error = INVALID_ENUM(ccimp_session_error_status_t);

    switch(ccfsm_session_error)
    {
        case connector_session_error_none:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_NONE;
            break;
        case connector_session_error_fatal:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_FATAL;
            break;
        case connector_session_error_invalid_opcode:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_INVALID_OPCODE;
            break;
        case connector_session_error_format:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_FORMAT;
            break;
        case connector_session_error_session_in_use:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_SESSION_IN_USE;
            break;
        case connector_session_error_unknown_session:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_UNKNOWN_SESSION;
            break;
        case connector_session_error_compression_failure:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_COMPRESSION_FAILURE;
            break;
        case connector_session_error_decompression_failure:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_DECOMPRESSION_FAILURE;
            break;
        case connector_session_error_memory:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_MEMORY_FAILED;
            break;
        case connector_session_error_send:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_SENDING;
            break;
        case connector_session_error_cancel:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_CANCEL;
            break;
        case connector_session_error_busy:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_BUSY;
            break;
        case connector_session_error_ack:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_ACK;
            break;
        case connector_session_error_timeout:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_TIMEOUT;
            break;
        case connector_session_error_no_service:
            cccimp_session_error = CCIMP_FS_SESSION_ERROR_NO_SERVICE;
            break;
        case connector_session_error_count:
            ASSERT(ccfsm_session_error != connector_session_error_count);
            break;
      }

      return cccimp_session_error;
}

static char const * get_path_without_virtual_dir(char const * const full_path)
{
    char const * c;
    char const * const dir_name_start = full_path + 1; /* Skip leading '/' */

    for (c = dir_name_start; *c != '\0' && *c != CCAPI_FS_DIR_SEPARATOR; c++);

    return c;
}

static char const * get_local_path_from_cloud_path(ccapi_data_t * const ccapi_data, char const * const full_path, ccapi_bool_t * const must_free_path)
{
    ccapi_bool_t const virtual_dirs_present = CCAPI_BOOL(ccapi_data->service.file_system.virtual_dir_list != NULL);
    char const * local_path = NULL;
    ccapi_bool_t free_path = CCAPI_FALSE;

    if (!virtual_dirs_present)
    {
        local_path = full_path;
        goto done;
    }

    {
        char const * const path_without_virtual_dir = get_path_without_virtual_dir(full_path);
        char const * const virtual_dir_name_start = full_path + 1;
        size_t const virtual_dir_name_length = path_without_virtual_dir - full_path - sizeof (char);
        ccapi_fs_virtual_dir_t * const dir_entry = *get_pointer_to_dir_entry_from_virtual_dir_name(ccapi_data, virtual_dir_name_start, virtual_dir_name_length);

        if (dir_entry == NULL)
        {
            goto done;
        }

        {
            char * translated_path = NULL;
            size_t path_without_virtual_dir_length = strlen(path_without_virtual_dir);
            translated_path = ccapi_malloc(dir_entry->local_dir_length + path_without_virtual_dir_length + 1);

            ASSERT_MSG_GOTO(translated_path != NULL, done);
            memcpy(translated_path, dir_entry->local_dir, strlen(dir_entry->local_dir));
            memcpy(translated_path + dir_entry->local_dir_length, path_without_virtual_dir, path_without_virtual_dir_length);
            translated_path[dir_entry->local_dir_length + path_without_virtual_dir_length] = '\0';
            local_path = translated_path;
            free_path = CCAPI_TRUE;
        }
    }

done:
    *must_free_path = free_path;

    return local_path;
}

static void free_local_path(char const * const local_path)
{
    ccapi_free((void *)local_path);
}

static void ccapi_fs_set_internal_error(ccapi_fs_internal_error_t const internal_error, connector_filesystem_errnum_t * const p_ccfsm_errnum)
{
    ccapi_fs_error_handle_t * const error_handle = ccapi_malloc(sizeof *error_handle);

    ASSERT_MSG_GOTO(error_handle != NULL, done);
    error_handle->error_is_internal = CCAPI_TRUE;
    error_handle->error.ccapi_error = internal_error;
    *p_ccfsm_errnum = error_handle;
done:
    return;
}

static void ccapi_fs_set_ccimp_error(ccimp_fs_errnum_t const errnum, connector_filesystem_errnum_t * const p_ccfsm_errnum)
{
    ccapi_fs_error_handle_t * const error_handle = ccapi_malloc(sizeof *error_handle);

    ASSERT_MSG_GOTO(error_handle != NULL, done);
    error_handle->error_is_internal = CCAPI_FALSE;
    error_handle->error.ccimp_error = errnum;
    *p_ccfsm_errnum = error_handle;
done:
    return;
}


static ccimp_status_t ccapi_fs_file_open(ccapi_data_t * const ccapi_data, connector_file_system_open_t * const ccfsm_open_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_file_open_t ccimp_open_data;
    ccapi_fs_access_t access;
    ccapi_fs_request_t request = CCAPI_FS_REQUEST_UNKNOWN;
    ccapi_bool_t must_free_local_path;
    char const * const local_path = get_local_path_from_cloud_path(ccapi_data, ccfsm_open_data->path, &must_free_local_path);

    if (local_path == NULL)
    {
        ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_INVALID_PATH, &ccfsm_open_data->errnum);
        goto done;
    }

    ccimp_open_data.path = local_path;
    ccimp_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccimp_open_data.flags = ccfsm_open_data->oflag;
    ccimp_open_data.imp_context = ccapi_data->service.file_system.imp_context;

    if (ccimp_open_data.flags & CCIMP_FILE_O_WRONLY)
    {
        request = CCAPI_FS_REQUEST_WRITE;
    }
    else if (ccimp_open_data.flags & CCIMP_FILE_O_RDWR)
    {
        request = CCAPI_FS_REQUEST_READWRITE;
    }
    else
    {
        request = CCAPI_FS_REQUEST_READ;
    }
    ASSERT_MSG_GOTO(request != CCAPI_FS_REQUEST_UNKNOWN, done);

    if (ccapi_data->service.file_system.user_callback.access != NULL)
    {
        access = ccapi_data->service.file_system.user_callback.access(ccimp_open_data.path, request);
    }
    else
    {
        access = CCAPI_FS_ACCESS_ALLOW;
    }

    switch (access)
    {
        case CCAPI_FS_ACCESS_ALLOW:
            ccimp_status = ccimp_fs_file_open(&ccimp_open_data);
            switch (ccimp_status)
            {
                case CCIMP_STATUS_OK:
                case CCIMP_STATUS_BUSY:
                    break;
                case CCIMP_STATUS_ERROR:
                {
                    ccapi_fs_set_ccimp_error(ccimp_open_data.errnum, &ccfsm_open_data->errnum);
                    break;
                }
            }
            ccapi_data->service.file_system.imp_context = ccimp_open_data.imp_context;
            break;
        case CCAPI_FS_ACCESS_DENY:
            ccimp_status = CCIMP_STATUS_ERROR;
            ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED, &ccfsm_open_data->errnum);
            break;
    }


    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
        {
            ccapi_fs_file_handle_t * const ccapi_fs_handle = ccapi_malloc(sizeof *ccapi_fs_handle);

            ASSERT_MSG_GOTO(ccapi_fs_handle != NULL, done);
            ccapi_fs_handle->file_path = ccapi_strdup(ccimp_open_data.path);
            ASSERT_MSG_GOTO(ccapi_fs_handle->file_path != NULL, done);
            ccapi_fs_handle->ccimp_handle = ccimp_open_data.handle;
            ccapi_fs_handle->request = request;
            ccfsm_open_data->handle = ccapi_fs_handle;
            break;
        }
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
        {
            ccfsm_open_data->handle = CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
            break;
        }
    }

done:
    if (must_free_local_path)
    {
        free_local_path(local_path);
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_file_read(ccapi_data_t * const ccapi_data, connector_file_system_read_t * const ccfsm_read_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_file_read_t ccimp_read_data;
    ccapi_fs_file_handle_t const * const ccapi_fs_handle = (ccapi_fs_file_handle_t *)ccfsm_read_data->handle;

    ccimp_read_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_read_data.bytes_used = 0;
    ccimp_read_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_read_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_read_data.buffer = ccfsm_read_data->buffer;
    ccimp_read_data.bytes_available = ccfsm_read_data->bytes_available;

    ccimp_status = ccimp_fs_file_read(&ccimp_read_data);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
            ccapi_fs_set_ccimp_error(ccimp_read_data.errnum, &ccfsm_read_data->errnum);
            break;
        case CCIMP_STATUS_BUSY:
            break;
    }

    ccapi_data->service.file_system.imp_context = ccimp_read_data.imp_context;
    ccfsm_read_data->bytes_used = ccimp_read_data.bytes_used;

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_file_write(ccapi_data_t * const ccapi_data, connector_file_system_write_t * const ccfsm_write_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_file_write_t ccimp_write_data;
    ccapi_fs_file_handle_t const * const ccapi_fs_handle = (ccapi_fs_file_handle_t *)ccfsm_write_data->handle;

    ccimp_write_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_write_data.bytes_used = 0;
    ccimp_write_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_write_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_write_data.buffer = ccfsm_write_data->buffer;
    ccimp_write_data.bytes_available = ccfsm_write_data->bytes_available;

    ccimp_status = ccimp_fs_file_write(&ccimp_write_data);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
            ccapi_fs_set_ccimp_error(ccimp_write_data.errnum, &ccfsm_write_data->errnum);
            break;
        case CCIMP_STATUS_BUSY:
            break;
    }

    ccapi_data->service.file_system.imp_context = ccimp_write_data.imp_context;
    ccfsm_write_data->bytes_used = ccimp_write_data.bytes_used;

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_file_seek(ccapi_data_t * const ccapi_data, connector_file_system_lseek_t * const ccfsm_seek_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_file_seek_t ccimp_seek_data;
    ccapi_fs_file_handle_t const * const ccapi_fs_handle = (ccapi_fs_file_handle_t *)ccfsm_seek_data->handle;

    ccimp_seek_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_seek_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_seek_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_seek_data.resulting_offset = 0;
    ccimp_seek_data.requested_offset = ccfsm_seek_data->requested_offset;
    ccimp_seek_data.origin = ccimp_seek_origin_from_ccfsm_seek_origin(ccfsm_seek_data->origin);

    ccimp_status = ccimp_fs_file_seek(&ccimp_seek_data);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
            ccapi_fs_set_ccimp_error(ccimp_seek_data.errnum, &ccfsm_seek_data->errnum);
            break;
        case CCIMP_STATUS_BUSY:
            break;
    }

    ccapi_data->service.file_system.imp_context = ccimp_seek_data.imp_context;
    ccfsm_seek_data->resulting_offset = ccimp_seek_data.resulting_offset;

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_file_close(ccapi_data_t * const ccapi_data, connector_file_system_close_t * const ccfsm_close_data)
{
    ccimp_status_t ccimp_status;
    ccimp_fs_file_close_t ccimp_close_data;
    ccapi_fs_file_handle_t const * const ccapi_fs_handle = (ccapi_fs_file_handle_t *)ccfsm_close_data->handle;

    ccimp_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_close_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_close_data.imp_context = ccapi_data->service.file_system.imp_context;

    ccimp_status = ccimp_fs_file_close(&ccimp_close_data);

    if (ccapi_data->service.file_system.user_callback.changed != NULL)
    {
        switch (ccapi_fs_handle->request)
        {
            case CCAPI_FS_REQUEST_READWRITE:
            case CCAPI_FS_REQUEST_WRITE:
                ccapi_data->service.file_system.user_callback.changed(ccapi_fs_handle->file_path, CCAPI_FS_CHANGED_MODIFIED);
                break;
            case CCAPI_FS_REQUEST_LIST:
            case CCAPI_FS_REQUEST_READ:
            case CCAPI_FS_REQUEST_UNKNOWN:
            case CCAPI_FS_REQUEST_REMOVE:
                break;
        }
    }

    switch(ccimp_status)
    {
        case CCIMP_STATUS_ERROR:
            ccapi_fs_set_ccimp_error(ccimp_close_data.errnum, &ccfsm_close_data->errnum);
            /* intentional fall-through */
        case CCIMP_STATUS_OK:
        {
            ccapi_free(ccapi_fs_handle->file_path);
            ccapi_free((void*)ccapi_fs_handle);
            break;
        }
        case CCIMP_STATUS_BUSY:
            break;
    }

    ccapi_data->service.file_system.imp_context = ccimp_close_data.imp_context;

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_file_truncate(ccapi_data_t * const ccapi_data, connector_file_system_truncate_t * const ccfsm_truncate_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_file_truncate_t ccimp_truncate_data;
    ccapi_fs_file_handle_t const * const ccapi_fs_handle = (ccapi_fs_file_handle_t *)ccfsm_truncate_data->handle;

    ccimp_truncate_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_truncate_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_truncate_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_truncate_data.length_in_bytes = ccfsm_truncate_data->length_in_bytes;

    ccimp_status = ccimp_fs_file_truncate(&ccimp_truncate_data);
    switch(ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        {
            ccapi_fs_set_ccimp_error(ccimp_truncate_data.errnum, &ccfsm_truncate_data->errnum);
            break;
        }
        case CCIMP_STATUS_BUSY:
            break;
    }
    ccapi_data->service.file_system.imp_context = ccimp_truncate_data.imp_context;

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_file_remove(ccapi_data_t * const ccapi_data, connector_file_system_remove_t * const ccfsm_remove_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_file_remove_t ccimp_remove_data;
    ccapi_bool_t must_free_local_path;
    ccapi_fs_access_t access;
    char const * const local_path = get_local_path_from_cloud_path(ccapi_data, ccfsm_remove_data->path, &must_free_local_path);

    if (local_path == NULL)
    {
        ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_INVALID_PATH, &ccfsm_remove_data->errnum);
        goto done;
    }

    ccimp_remove_data.path = local_path;
    ccimp_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_remove_data.imp_context = ccapi_data->service.file_system.imp_context;

    if (ccapi_data->service.file_system.user_callback.access != NULL)
    {
        access = ccapi_data->service.file_system.user_callback.access(ccimp_remove_data.path, CCAPI_FS_REQUEST_REMOVE);
    }
    else
    {
        access = CCAPI_FS_ACCESS_ALLOW;
    }

    switch (access)
    {
        case CCAPI_FS_ACCESS_ALLOW:
            ccimp_status = ccimp_fs_file_remove(&ccimp_remove_data);
            switch (ccimp_status)
            {
                case CCIMP_STATUS_OK:
                {
                    if (ccapi_data->service.file_system.user_callback.changed != NULL)
                    {
                        ccapi_data->service.file_system.user_callback.changed(ccimp_remove_data.path, CCAPI_FS_CHANGED_REMOVED);
                    }
                    break;
                }
                case CCIMP_STATUS_ERROR:
                    ccapi_fs_set_ccimp_error(ccimp_remove_data.errnum, &ccfsm_remove_data->errnum);
                    break;
                case CCIMP_STATUS_BUSY:
                    break;
            }

            ccapi_data->service.file_system.imp_context = ccimp_remove_data.imp_context;
            break;
        case CCAPI_FS_ACCESS_DENY:
            ccimp_status = CCIMP_STATUS_ERROR;
            ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED, &ccfsm_remove_data->errnum);
            break;
    }

done:
    if (must_free_local_path)
    {
        free_local_path(local_path);
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_dir_open(ccapi_data_t * const ccapi_data, connector_file_system_opendir_t * const ccfsm_dir_open_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    if (ccfsm_dir_open_data->user_context != NULL)
    {
        ccfsm_dir_open_data->handle = ccfsm_dir_open_data->user_context;
        ccimp_status = CCIMP_STATUS_OK;
    }
    else
    {
        ccimp_fs_dir_open_t ccimp_dir_open_data;
        ccapi_fs_access_t access;
        ccapi_bool_t must_free_local_path;
        char const * const local_path = get_local_path_from_cloud_path(ccapi_data, ccfsm_dir_open_data->path, &must_free_local_path);

        if (local_path == NULL)
        {
            ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_INVALID_PATH, &ccfsm_dir_open_data->errnum);
            goto done;
        }

        ccimp_dir_open_data.path = local_path;
        ccimp_dir_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
        ccimp_dir_open_data.handle = CCIMP_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED;
        ccimp_dir_open_data.imp_context = ccapi_data->service.file_system.imp_context;

        if (ccapi_data->service.file_system.user_callback.access != NULL)
        {
            access = ccapi_data->service.file_system.user_callback.access(ccimp_dir_open_data.path, CCAPI_FS_REQUEST_LIST);
        }
        else
        {
            access = CCAPI_FS_ACCESS_ALLOW;
        }

        switch (access)
        {
            case CCAPI_FS_ACCESS_ALLOW:
                ccimp_status = ccimp_fs_dir_open(&ccimp_dir_open_data);
                switch (ccimp_status)
                {
                    case CCIMP_STATUS_OK:
                        break;
                    case CCIMP_STATUS_BUSY:
                        break;
                    case CCIMP_STATUS_ERROR:
                        ccapi_fs_set_ccimp_error(ccimp_dir_open_data.errnum, &ccfsm_dir_open_data->errnum);
                        break;
                }
                ccapi_data->service.file_system.imp_context = ccimp_dir_open_data.imp_context;
                break;
            case CCAPI_FS_ACCESS_DENY:
                ccimp_status = CCIMP_STATUS_ERROR;
                ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED, &ccfsm_dir_open_data->errnum);
                goto done;
                break;
        }

        ccfsm_dir_open_data->handle = ccimp_dir_open_data.handle;

done:
        if (must_free_local_path)
        {
            free_local_path(local_path);
        }
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_dir_read_entry(ccapi_data_t * const ccapi_data, connector_file_system_readdir_t * const ccfsm_dir_read_entry_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    if (ccfsm_dir_read_entry_data->user_context != NULL)
    {
        ccapi_fs_virtual_rootdir_listing_handle_t * root_dir_listing_handle = ccfsm_dir_read_entry_data->user_context;
        ccapi_fs_virtual_dir_t * dir_entry = root_dir_listing_handle->dir_entry;

        if (dir_entry != NULL)
        {
            size_t const virtual_dir_strsize = strlen(dir_entry->virtual_dir) + 1;
            size_t const mecmpy_size = CCAPI_MIN_OF(virtual_dir_strsize, ccfsm_dir_read_entry_data->bytes_available);

            memcpy(ccfsm_dir_read_entry_data->entry_name, dir_entry->virtual_dir, mecmpy_size);

            root_dir_listing_handle->dir_entry = dir_entry->next;
        }
        else
        {
            strcpy(ccfsm_dir_read_entry_data->entry_name, "");
        }
        ccimp_status = CCIMP_STATUS_OK;
    }
    else
    {
        ccimp_fs_dir_read_entry_t ccimp_dir_read_entry_data;

        ccimp_dir_read_entry_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
        ccimp_dir_read_entry_data.imp_context = ccapi_data->service.file_system.imp_context;
        ccimp_dir_read_entry_data.handle = ccfsm_dir_read_entry_data->handle;
        ccimp_dir_read_entry_data.entry_name = ccfsm_dir_read_entry_data->entry_name;
        ccimp_dir_read_entry_data.bytes_available = ccfsm_dir_read_entry_data->bytes_available;

        ccimp_status = ccimp_fs_dir_read_entry(&ccimp_dir_read_entry_data);
        switch(ccimp_status)
        {
            case CCIMP_STATUS_OK:
                break;
            case CCIMP_STATUS_ERROR:
            {
                ccapi_fs_set_ccimp_error(ccimp_dir_read_entry_data.errnum, &ccfsm_dir_read_entry_data->errnum);
                break;
            }
            case CCIMP_STATUS_BUSY:
                break;
        }
        ccapi_data->service.file_system.imp_context = ccimp_dir_read_entry_data.imp_context;
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_dir_entry_status(ccapi_data_t * const ccapi_data, connector_file_system_stat_dir_entry_t * const ccfsm_dir_entry_status_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    if (ccfsm_dir_entry_status_data->user_context != NULL)
    {
        ccfsm_dir_entry_status_data->statbuf.flags = connector_file_system_file_type_is_dir;
        ccfsm_dir_entry_status_data->statbuf.last_modified = 0;
        ccfsm_dir_entry_status_data->statbuf.file_size = 0;
        ccimp_status = CCIMP_STATUS_OK;
    }
    else
    {
        ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;
        ccapi_bool_t must_free_local_path;
        char const * const local_path = get_local_path_from_cloud_path(ccapi_data, ccfsm_dir_entry_status_data->path, &must_free_local_path);

        if (local_path == NULL)
        {
            ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_INVALID_PATH, &ccfsm_dir_entry_status_data->errnum);
            goto done;
        }

        ccimp_dir_entry_status_data.path = local_path;
        ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
        ccimp_dir_entry_status_data.imp_context = ccapi_data->service.file_system.imp_context;
        ccimp_dir_entry_status_data.status.file_size = 0;
        ccimp_dir_entry_status_data.status.last_modified = 0;
        ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

        ccimp_status = ccimp_fs_dir_entry_status(&ccimp_dir_entry_status_data);
        switch(ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_BUSY:
                break;
            case CCIMP_STATUS_ERROR:
                ccapi_fs_set_ccimp_error(ccimp_dir_entry_status_data.errnum, &ccfsm_dir_entry_status_data->errnum);
                break;
        }

        ccapi_data->service.file_system.imp_context = ccimp_dir_entry_status_data.imp_context;
        ccfsm_dir_entry_status_data->statbuf.file_size = ccimp_dir_entry_status_data.status.file_size;
        ccfsm_dir_entry_status_data->statbuf.last_modified = ccimp_dir_entry_status_data.status.last_modified;
        ccfsm_dir_entry_status_data->statbuf.flags = ccfsm_file_system_file_type_from_ccimp_fs_dir_entry_type(ccimp_dir_entry_status_data.status.type);
done:
        if (must_free_local_path)
        {
            free_local_path(local_path);
        }
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_dir_close(ccapi_data_t * const ccapi_data, connector_file_system_closedir_t * const ccfsm_dir_close_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;

    if (ccfsm_dir_close_data->user_context != NULL)
    {
        ccimp_status = ccapi_free(ccfsm_dir_close_data->user_context);
        ccfsm_dir_close_data->user_context = NULL;
    }
    else
    {
        ccimp_fs_dir_close_t ccimp_dir_close_data;

        ccimp_dir_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
        ccimp_dir_close_data.imp_context = ccapi_data->service.file_system.imp_context;
        ccimp_dir_close_data.handle = ccfsm_dir_close_data->handle;

        ccimp_status = ccimp_fs_dir_close(&ccimp_dir_close_data);
        switch(ccimp_status)
        {
            case CCIMP_STATUS_OK:
                break;
            case CCIMP_STATUS_ERROR:
            {
                ccapi_fs_set_ccimp_error(ccimp_dir_close_data.errnum, &ccfsm_dir_close_data->errnum);
                break;
            }
            case CCIMP_STATUS_BUSY:
                break;
        }
        ccapi_data->service.file_system.imp_context = ccimp_dir_close_data.imp_context;
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_hash_status(ccapi_data_t * const ccapi_data, connector_file_system_stat_t * const ccfsm_hash_status_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccapi_bool_t must_free_local_path = CCAPI_FALSE;
    char const * local_path = NULL;

    if (ccapi_data->service.file_system.virtual_dir_list != NULL && strcmp(ccfsm_hash_status_data->path, CCAPI_FS_ROOT_PATH) == 0)
    {
        ccapi_fs_virtual_rootdir_listing_handle_t * const root_dir_listing_handle = malloc(sizeof *root_dir_listing_handle);

        ASSERT_MSG_GOTO(root_dir_listing_handle != NULL, done);
        root_dir_listing_handle->dir_entry = ccapi_data->service.file_system.virtual_dir_list;
        ccfsm_hash_status_data->user_context = root_dir_listing_handle;
        ccfsm_hash_status_data->hash_algorithm.actual = connector_file_system_hash_none;
        ccfsm_hash_status_data->statbuf.file_size = 0;
        ccfsm_hash_status_data->statbuf.last_modified = 0;
        ccfsm_hash_status_data->statbuf.flags = connector_file_system_file_type_is_dir;
        ccimp_status = CCIMP_STATUS_OK;
    }
    else
    {
        ccimp_fs_get_hash_alg_t ccimp_hash_algorithm_data;
        ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;

        local_path = get_local_path_from_cloud_path(ccapi_data, ccfsm_hash_status_data->path, &must_free_local_path);
        if (local_path == NULL)
        {
            ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_INVALID_PATH, &ccfsm_hash_status_data->errnum);
            goto done;
        }

        ccimp_dir_entry_status_data.path = local_path;
        ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
        ccimp_dir_entry_status_data.imp_context = ccapi_data->service.file_system.imp_context;
        ccimp_dir_entry_status_data.status.file_size = 0;
        ccimp_dir_entry_status_data.status.last_modified = 0;
        ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

        ccimp_status = ccimp_fs_dir_entry_status(&ccimp_dir_entry_status_data);
        switch(ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_BUSY:
                break;
            case CCIMP_STATUS_ERROR:
                ccapi_fs_set_ccimp_error(ccimp_dir_entry_status_data.errnum, &ccfsm_hash_status_data->errnum);
                goto done;
        }

        ccimp_hash_algorithm_data.path = local_path;
        ccimp_hash_algorithm_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
        ccimp_hash_algorithm_data.imp_context = ccapi_data->service.file_system.imp_context;
        ccimp_hash_algorithm_data.hash_alg.actual = CCIMP_FS_HASH_NONE;
        ccimp_hash_algorithm_data.hash_alg.requested = ccimp_fs_hash_alg_from_ccfsm_file_system_hash_algorithm(ccfsm_hash_status_data->hash_algorithm.requested);

        ccimp_status = ccimp_fs_hash_alg(&ccimp_hash_algorithm_data);
        switch(ccimp_status)
        {
            case CCIMP_STATUS_OK:
            case CCIMP_STATUS_BUSY:
                break;
            case CCIMP_STATUS_ERROR:
            {
                ccapi_fs_set_ccimp_error(ccimp_hash_algorithm_data.errnum, &ccfsm_hash_status_data->errnum);
                break;
            }
        }

        ccapi_data->service.file_system.imp_context = ccimp_hash_algorithm_data.imp_context;
        ccfsm_hash_status_data->statbuf.file_size = ccimp_dir_entry_status_data.status.file_size;
        ccfsm_hash_status_data->statbuf.last_modified = ccimp_dir_entry_status_data.status.last_modified;
        ccfsm_hash_status_data->statbuf.flags = ccfsm_file_system_file_type_from_ccimp_fs_dir_entry_type(ccimp_dir_entry_status_data.status.type);
        ccfsm_hash_status_data->hash_algorithm.actual = ccfsm_file_system_hash_algorithm_from_ccimp_fs_hash_alg(ccimp_hash_algorithm_data.hash_alg.actual);
    }

done:
    if (must_free_local_path)
    {
        free_local_path(local_path);
    }
    return ccimp_status;
}

static ccimp_status_t ccapi_fs_hash_file(ccapi_data_t * const ccapi_data, connector_file_system_hash_t * const ccfsm_hash_file_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_hash_file_t ccimp_hash_file_data;
    ccapi_bool_t must_free_local_path;
    char const * const local_path = get_local_path_from_cloud_path(ccapi_data, ccfsm_hash_file_data->path, &must_free_local_path);

    if (local_path == NULL)
    {
        ccapi_fs_set_internal_error(CCAPI_FS_INTERNAL_ERROR_INVALID_PATH, &ccfsm_hash_file_data->errnum);
        goto done;
    }

    ccimp_hash_file_data.path = local_path;
    ccimp_hash_file_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_hash_file_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_hash_file_data.hash_algorithm = ccimp_fs_hash_alg_from_ccfsm_file_system_hash_algorithm(ccfsm_hash_file_data->hash_algorithm);
    ccimp_hash_file_data.hash_value = ccfsm_hash_file_data->hash_value;
    ccimp_hash_file_data.bytes_requested = ccfsm_hash_file_data->bytes_requested;

    ccimp_status = ccimp_fs_hash_file(&ccimp_hash_file_data);
    switch(ccimp_status)
    {
        case CCIMP_STATUS_OK:
        case CCIMP_STATUS_BUSY:
            break;
        case CCIMP_STATUS_ERROR:
        {
            ccapi_fs_set_ccimp_error(ccimp_hash_file_data.errnum, &ccfsm_hash_file_data->errnum);
            break;
        }
    }
    ccapi_data->service.file_system.imp_context = ccimp_hash_file_data.imp_context;

done:
    if (must_free_local_path)
    {
        free_local_path(local_path);
    }

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_error_desc(ccapi_data_t * const ccapi_data, connector_file_system_get_error_t * const ccfsm_error_desc_data)
{
    ccimp_status_t ccimp_status;
    ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_error_desc_data->errnum;

    if (error_handle->error_is_internal)
    {
        switch (error_handle->error.ccapi_error)
        {
            case CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED:
                ccfsm_error_desc_data->error_status = connector_file_system_permission_denied;
                break;
            case CCAPI_FS_INTERNAL_ERROR_INVALID_PATH:
                ccfsm_error_desc_data->error_status = connector_file_system_path_not_found;
                break;
        }
        ccimp_status = CCIMP_STATUS_OK;
        ccfsm_error_desc_data->bytes_used = 0;
    }
    else
    {
        ccimp_fs_error_desc_t ccimp_error_desc_data;

        ccimp_error_desc_data.errnum = error_handle->error.ccimp_error;
        ccimp_error_desc_data.imp_context = ccapi_data->service.file_system.imp_context;
        ccimp_error_desc_data.error_string = ccfsm_error_desc_data->buffer;
        ccimp_error_desc_data.bytes_available = ccfsm_error_desc_data->bytes_available;
        ccimp_error_desc_data.bytes_used = 0;
        ccimp_error_desc_data.error_status = CCIMP_FS_ERROR_UNKNOWN;

        ccimp_status = ccimp_fs_error_desc(&ccimp_error_desc_data);

        ccapi_data->service.file_system.imp_context = ccimp_error_desc_data.imp_context;
        ccfsm_error_desc_data->bytes_used = ccimp_error_desc_data.bytes_used;
        ccfsm_error_desc_data->error_status = ccfsm_file_system_error_status_from_ccimp_fs_error(ccimp_error_desc_data.error_status);
    }

    ccapi_free(error_handle);

    return ccimp_status;
}

static ccimp_status_t ccapi_fs_session_error(ccapi_data_t * const ccapi_data, connector_file_system_session_error_t const * const ccfsm_session_error_data)
{
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccimp_fs_session_error_t ccimp_session_error_data;

    ccimp_session_error_data.imp_context = ccapi_data->service.file_system.imp_context;
    ccimp_session_error_data.session_error = ccimp_session_error_from_ccfsm_session_error(ccfsm_session_error_data->session_error);

    ccimp_status = ccimp_fs_session_error(&ccimp_session_error_data);

    ccapi_data->service.file_system.imp_context = ccimp_session_error_data.imp_context;

    return ccimp_status;
}

connector_callback_status_t ccapi_filesystem_handler(connector_request_id_file_system_t const filesystem_request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t connector_status;
    ccimp_status_t ccimp_status = CCIMP_STATUS_ERROR;
    ccapi_bool_t lock_acquired = CCAPI_FALSE;

    {
        ccimp_status_t const ccapi_lock_acquire_status = ccapi_lock_acquire(ccapi_data->file_system_lock);

        switch (ccapi_lock_acquire_status)
        {
            case CCIMP_STATUS_OK:
                lock_acquired = CCAPI_TRUE;
                break;
            case CCIMP_STATUS_BUSY:
            case CCIMP_STATUS_ERROR:
                ccimp_status = ccapi_lock_acquire_status;
                goto done;
        }
    }

    switch (filesystem_request)
    {
        case connector_request_id_file_system_open:
        {
            connector_file_system_open_t * const ccfsm_open_data = data;

            ccimp_status = ccapi_fs_file_open(ccapi_data, ccfsm_open_data);
            break;
        }

        case connector_request_id_file_system_read:
        {
            connector_file_system_read_t * const ccfsm_read_data = data;

            ccimp_status = ccapi_fs_file_read(ccapi_data, ccfsm_read_data);
            break;
        }

        case connector_request_id_file_system_write:
        {
            connector_file_system_write_t * const ccfsm_write_data = data;

            ccimp_status = ccapi_fs_file_write(ccapi_data, ccfsm_write_data);
            break;
        }

        case connector_request_id_file_system_lseek:
        {
            connector_file_system_lseek_t * const ccfsm_seek_data = data;

            ccimp_status = ccapi_fs_file_seek(ccapi_data, ccfsm_seek_data);
            break;
        }

        case connector_request_id_file_system_close:
        {
            connector_file_system_close_t * const ccfsm_close_data = data;

            ccimp_status = ccapi_fs_file_close(ccapi_data, ccfsm_close_data);
            break;
        }

        case connector_request_id_file_system_ftruncate:
        {
            connector_file_system_truncate_t * const ccfsm_truncate_data = data;

            ccimp_status = ccapi_fs_file_truncate(ccapi_data, ccfsm_truncate_data);
            break;
        }

        case connector_request_id_file_system_remove:
        {
            connector_file_system_remove_t * const ccfsm_remove_data = data;

            ccimp_status = ccapi_fs_file_remove(ccapi_data, ccfsm_remove_data);
            break;
        }

        case connector_request_id_file_system_opendir:
        {
            connector_file_system_opendir_t * const ccfsm_dir_open_data = data;

            ccimp_status = ccapi_fs_dir_open(ccapi_data, ccfsm_dir_open_data);
            break;
        }

        case connector_request_id_file_system_readdir:
        {
            connector_file_system_readdir_t * const ccfsm_dir_read_entry_data = data;

            ccimp_status = ccapi_fs_dir_read_entry(ccapi_data, ccfsm_dir_read_entry_data);
            break;
        }

        case connector_request_id_file_system_stat_dir_entry:
        {
            connector_file_system_stat_dir_entry_t * const ccfsm_dir_entry_status_data = data;

            ccimp_status = ccapi_fs_dir_entry_status(ccapi_data, ccfsm_dir_entry_status_data);
            break;
        }

        case connector_request_id_file_system_closedir:
        {
            connector_file_system_closedir_t * const ccfsm_dir_close_data = data;

            ccimp_status = ccapi_fs_dir_close(ccapi_data, ccfsm_dir_close_data);
            break;
        }

        case connector_request_id_file_system_stat:
        {
            connector_file_system_stat_t * const ccfsm_hash_status_data = data;

            ccimp_status = ccapi_fs_hash_status(ccapi_data, ccfsm_hash_status_data);
            break;
        }

        case connector_request_id_file_system_hash:
        {
            connector_file_system_hash_t * const ccfsm_hash_file_data = data;

            ccimp_status = ccapi_fs_hash_file(ccapi_data, ccfsm_hash_file_data);
            break;
        }

        case connector_request_id_file_system_get_error:
        {
            connector_file_system_get_error_t * const ccfsm_error_desc_data = data;

            ccimp_status = ccapi_fs_error_desc(ccapi_data, ccfsm_error_desc_data);
            break;
        }

        case connector_request_id_file_system_session_error:
        {
            connector_file_system_session_error_t const * const ccfsm_session_error_data = data;

            ccimp_status = ccapi_fs_session_error(ccapi_data, ccfsm_session_error_data);
            break;
        }
    }

done:
    if (lock_acquired)
    {
        ccimp_status_t const ccapi_lock_release_status = ccapi_lock_release(ccapi_data->file_system_lock);

        switch (ccapi_lock_release_status)
        {
            case CCIMP_STATUS_OK:
                break;
            case CCIMP_STATUS_BUSY:
            case CCIMP_STATUS_ERROR:
                ccimp_status = ccapi_lock_release_status;
                break;
        }
    }

    connector_status = connector_callback_status_from_ccimp_status(ccimp_status);
    return connector_status;
}
#endif
