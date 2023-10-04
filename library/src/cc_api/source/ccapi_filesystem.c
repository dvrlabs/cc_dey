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
static ccapi_fs_error_t add_virtual_dir_entry(ccapi_data_t * const ccapi_data, ccapi_fs_virtual_dir_t * const new_entry)
{
    ccapi_fs_error_t error = CCAPI_FS_ERROR_NONE;
    ccimp_status_t ccimp_status;

    ccimp_status = ccapi_lock_acquire(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_FS_ERROR_LOCK_FAILED;
            goto done;
    }

    {
        ccapi_fs_virtual_dir_t * const next_entry = ccapi_data->service.file_system.virtual_dir_list;

        new_entry->next = next_entry;
        ccapi_data->service.file_system.virtual_dir_list = new_entry;
    }

    ccimp_status = ccapi_lock_release(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_FS_ERROR_LOCK_FAILED;
            ASSERT_MSG_GOTO(ccimp_status == CCIMP_STATUS_OK, done);
    }

done:
    return error;
}

static ccapi_fs_virtual_dir_t * create_virtual_dir_entry(char const * const virtual_dir, char const * const local_dir)
{
    ccapi_fs_virtual_dir_t * new_dir_entry = ccapi_malloc(sizeof *new_dir_entry);

    if (new_dir_entry == NULL)
    {
        goto done;
    }

    new_dir_entry->local_dir = ccapi_strdup(local_dir);
    if (new_dir_entry->local_dir == NULL)
    {
        reset_heap_ptr(&new_dir_entry);
        goto done;
    }

    new_dir_entry->virtual_dir = ccapi_strdup(virtual_dir);
    if (new_dir_entry->virtual_dir == NULL)
    {
        ccapi_free(new_dir_entry->local_dir);
        reset_heap_ptr(&new_dir_entry);
        new_dir_entry = NULL;
        goto done;
    }
    new_dir_entry->virtual_dir_length = strlen(virtual_dir);
    new_dir_entry->local_dir_length = strlen(local_dir);
    new_dir_entry->next = NULL;
done:
    return new_dir_entry;
}

static ccapi_bool_t path_is_a_dir(ccapi_data_t * const ccapi_data, char const * const local_path)
{
    ccapi_bool_t path_is_a_dir = CCAPI_FALSE;
    ccimp_status_t ccimp_status;
    ccimp_fs_stat_t fs_status;

    ccimp_status = ccapi_get_dir_entry_status(ccapi_data, local_path, &fs_status);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_BUSY:
        case CCIMP_STATUS_ERROR:
            goto done;
    }

    switch (fs_status.type)
    {
        case CCIMP_FS_DIR_ENTRY_DIR:
            break;
        case CCIMP_FS_DIR_ENTRY_FILE:
        case CCIMP_FS_DIR_ENTRY_UNKNOWN:
            goto done;
    }
    path_is_a_dir = CCAPI_TRUE;
done:
    return path_is_a_dir;
}

ccapi_fs_error_t ccxapi_fs_add_virtual_dir(ccapi_data_t * const ccapi_data, char const * const virtual_dir, char const * const local_dir)
{
    ccapi_fs_error_t error = CCAPI_FS_ERROR_NONE;
    ccapi_fs_virtual_dir_t * new_dir_entry;

    if (virtual_dir == NULL || local_dir == NULL || virtual_dir[0] == '\0' || local_dir[0] == '\0' || strstr(virtual_dir, "/") != NULL || strstr(virtual_dir, "\\") || strlen(virtual_dir) > (CCIMP_FILE_SYSTEM_MAX_PATH_LENGTH - sizeof "//.."))
    {
        error = CCAPI_FS_ERROR_INVALID_PATH;
        goto done;
    }

    if (ccapi_data == NULL || ccapi_data->thread.connector_run->status == CCAPI_THREAD_NOT_STARTED)
    {
        ccapi_logging_line("ccxapi_fs_add_virtual_dir: CCAPI not started");

        error = CCAPI_FS_ERROR_CCAPI_STOPPED;
        goto done;
    }

    if (!ccapi_data->config.filesystem_supported)
    {
        error = CCAPI_FS_ERROR_NO_FS_SUPPORT;
        goto done;
    }

    if (NULL != *get_pointer_to_dir_entry_from_virtual_dir_name(ccapi_data, virtual_dir, strlen(virtual_dir)))
    {
        error = CCAPI_FS_ERROR_ALREADY_MAPPED;
        goto done;
    }

    if (!path_is_a_dir(ccapi_data, local_dir))
    {
        error = CCAPI_FS_ERROR_NOT_A_DIR;
        goto done;
    }

    new_dir_entry = create_virtual_dir_entry(virtual_dir, local_dir);
    if (new_dir_entry == NULL)
    {
        error = CCAPI_FS_ERROR_INSUFFICIENT_MEMORY;
        goto done;
    }
    error = add_virtual_dir_entry(ccapi_data, new_dir_entry);

done:
    return error;
}

ccapi_fs_virtual_dir_t * * get_pointer_to_dir_entry_from_virtual_dir_name(ccapi_data_t * const ccapi_data, char const * const virtual_dir, unsigned int const virtual_dir_length)
{
    ccapi_fs_virtual_dir_t * * p_dir_entry = &ccapi_data->service.file_system.virtual_dir_list;
    ccapi_bool_t finished = CCAPI_FALSE;

    do {
        ccapi_fs_virtual_dir_t * dir_entry = *p_dir_entry;

        if (dir_entry != NULL)
        {
            unsigned int const longest_strlen = CCAPI_MAX_OF(dir_entry->virtual_dir_length, virtual_dir_length);
            if (strncmp(dir_entry->virtual_dir, virtual_dir, longest_strlen) == 0)
            {
                finished = CCAPI_TRUE;
            }
            else
            {
                p_dir_entry = &dir_entry->next;
            }
        }
        else
        {
            finished = CCAPI_TRUE;
        }
    } while (!finished);

    return p_dir_entry;
}

ccapi_fs_error_t ccxapi_fs_remove_virtual_dir(ccapi_data_t * const ccapi_data, char const * const virtual_dir)
{
    ccapi_fs_error_t error;
    ccapi_fs_virtual_dir_t * dir_entry_to_remove;
    ccimp_status_t ccimp_status;

    if (ccapi_data == NULL || ccapi_data->thread.connector_run->status == CCAPI_THREAD_NOT_STARTED)
    {
        ccapi_logging_line("ccxapi_fs_remove_virtual_dir: CCAPI not started");

        error = CCAPI_FS_ERROR_CCAPI_STOPPED;
        goto done;
    }

    if (!ccapi_data->config.filesystem_supported)
    {
        error = CCAPI_FS_ERROR_NO_FS_SUPPORT;
        goto done;
    }

    if (virtual_dir == NULL || virtual_dir[0] == '\0' || virtual_dir[0] == '/')
    {
        error = CCAPI_FS_ERROR_INVALID_PATH;
        goto done;
    }

    ccimp_status = ccapi_lock_acquire(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_FS_ERROR_LOCK_FAILED;
            goto done;
    }

    {
        ccapi_fs_virtual_dir_t * * const p_dir_entry = get_pointer_to_dir_entry_from_virtual_dir_name(ccapi_data, virtual_dir, strlen(virtual_dir));

        dir_entry_to_remove = *p_dir_entry;
        if (*p_dir_entry != NULL)
        {
            *p_dir_entry = dir_entry_to_remove->next;
        }
        else
        {
            error = CCAPI_FS_ERROR_NOT_MAPPED;
        }
    }

    ccimp_status = ccapi_lock_release(ccapi_data->file_system_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_FS_ERROR_LOCK_FAILED;
            ccapi_logging_line("WARNING: ccapi_lock_release() failed with status %d, directory will be removed anyway", ccimp_status);
            break;
    }

    if (dir_entry_to_remove != NULL)
    {
        ccapi_free(dir_entry_to_remove->local_dir);
        ccapi_free(dir_entry_to_remove->virtual_dir);
        ccapi_free(dir_entry_to_remove);
        error = CCAPI_FS_ERROR_NONE;
    }

done:
    return error;
}

ccapi_fs_error_t ccapi_fs_add_virtual_dir(char const * const virtual_dir, char const * const local_dir)
{
    return ccxapi_fs_add_virtual_dir(ccapi_data_single_instance, virtual_dir, local_dir);
}

ccapi_fs_error_t ccapi_fs_remove_virtual_dir(char const * const virtual_dir)
{
    return ccxapi_fs_remove_virtual_dir(ccapi_data_single_instance, virtual_dir);
}
#endif
