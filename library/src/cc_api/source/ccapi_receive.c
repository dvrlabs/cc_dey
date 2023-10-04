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

#if (defined CCIMP_DATA_SERVICE_ENABLED)

static ccapi_receive_error_t add_target_entry(ccapi_data_t * const ccapi_data, ccapi_receive_target_t * const new_entry)
{
    ccapi_receive_error_t error = CCAPI_RECEIVE_ERROR_NONE;
    ccimp_status_t ccimp_status;

    ccimp_status = ccapi_lock_acquire(ccapi_data->service.receive.receive_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_RECEIVE_ERROR_LOCK_FAILED;
            goto done;
    }

    {
        ccapi_receive_target_t * const next_entry = ccapi_data->service.receive.target_list;

        new_entry->next = next_entry;
        ccapi_data->service.receive.target_list = new_entry;
    }

    ccimp_status = ccapi_lock_release(ccapi_data->service.receive.receive_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_RECEIVE_ERROR_LOCK_FAILED;
            ASSERT_MSG_GOTO(ccimp_status == CCIMP_STATUS_OK, done);
            break;
    }

done:
    return error;
}

static ccapi_receive_target_t * create_target_entry(char const * const target, ccapi_receive_data_cb_t const data_cb, ccapi_receive_status_cb_t const status_cb, size_t const max_request_size)
{
    ccapi_receive_target_t * new_target_entry = ccapi_malloc(sizeof *new_target_entry);

    if (new_target_entry == NULL)
    {
        goto done;
    }

    new_target_entry->target = ccapi_strdup(target);
    if (new_target_entry->target == NULL)
    {
        reset_heap_ptr(&new_target_entry);
        goto done;
    }

    new_target_entry->user_callback.data = data_cb;
    new_target_entry->user_callback.status = status_cb;
    new_target_entry->max_request_size = max_request_size;
    new_target_entry->next = NULL;

done:
    return new_target_entry;
}

ccapi_receive_error_t ccxapi_receive_add_target(ccapi_data_t * const ccapi_data, char const * const target, ccapi_receive_data_cb_t const data_cb, ccapi_receive_status_cb_t const status_cb, size_t const max_request_size)
{
    ccapi_receive_error_t error = CCAPI_RECEIVE_ERROR_NONE;
    ccapi_receive_target_t * new_target_entry;

    if (target == NULL)
    {
        error = CCAPI_RECEIVE_ERROR_INVALID_TARGET;
        goto done;
    }

    if (data_cb == NULL)
    {
        error = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
        goto done;
    }

    if (!CCAPI_RUNNING(ccapi_data))
    {
        ccapi_logging_line("ccxapi_receive_add_target: CCAPI not running");

        error = CCAPI_RECEIVE_ERROR_CCAPI_NOT_RUNNING;
        goto done;
    }

    if (!ccapi_data->config.receive_supported)
    {
        error = CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT;
        goto done;
    }

    if (NULL != *get_pointer_to_target_entry(ccapi_data, target))
    {
        error = CCAPI_RECEIVE_ERROR_TARGET_ALREADY_ADDED;
        goto done;
    }

    new_target_entry = create_target_entry(target, data_cb, status_cb, max_request_size);
    if (new_target_entry == NULL)
    {
        error = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
        goto done;
    }

    error = add_target_entry(ccapi_data, new_target_entry);

done:
    return error;
}

ccapi_receive_target_t * * get_pointer_to_target_entry(ccapi_data_t * const ccapi_data, char const * const target)
{
    ccapi_receive_target_t * * p_target = &ccapi_data->service.receive.target_list;
    ccapi_bool_t finished = CCAPI_FALSE;

    do {
        ccapi_receive_target_t * const current_target = *p_target;

        if (current_target != NULL)
        {
            unsigned int const longest_strlen = CCAPI_MAX_OF(strlen(current_target->target), strlen(target));
            if (strncmp(current_target->target, target, longest_strlen) == 0)
            {
                finished = CCAPI_TRUE;
            }
            else
            {
                p_target = &current_target->next;
            }
        }
        else
        {
            finished = CCAPI_TRUE;
        }
    } while (!finished);

    return p_target;
}

ccapi_receive_error_t ccxapi_receive_remove_target(ccapi_data_t * const ccapi_data, char const * const target)
{
    ccapi_receive_error_t error = CCAPI_RECEIVE_ERROR_NONE;
    ccapi_receive_target_t * * p_target = NULL;
    ccimp_status_t ccimp_status;

    if (!CCAPI_RUNNING(ccapi_data))
    {
        ccapi_logging_line("ccxapi_fs_remove_virtual_dir: CCAPI not started");

        error = CCAPI_RECEIVE_ERROR_CCAPI_NOT_RUNNING;
        goto done;
    }

    if (!ccapi_data->config.receive_supported)
    {
        error = CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT;
        goto done;
    }

    if (target == NULL)
    {
        error = CCAPI_RECEIVE_ERROR_INVALID_TARGET;
        goto done;
    }

    p_target = get_pointer_to_target_entry(ccapi_data, target);
    if (*p_target == NULL)
    {
        error = CCAPI_RECEIVE_ERROR_TARGET_NOT_ADDED;
        goto done;
    }

    ccimp_status = ccapi_lock_acquire(ccapi_data->service.receive.receive_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_RECEIVE_ERROR_LOCK_FAILED;
            goto done;
    }

    {
        ccapi_receive_target_t * const target_entry = *p_target;
        ccapi_receive_target_t * const next_target_entry = target_entry->next;

        ccapi_free(target_entry->target);
        ccapi_free(target_entry);
        *p_target = next_target_entry;
    }

    ccimp_status = ccapi_lock_release(ccapi_data->service.receive.receive_lock);
    switch (ccimp_status)
    {
        case CCIMP_STATUS_OK:
            break;
        case CCIMP_STATUS_ERROR:
        case CCIMP_STATUS_BUSY:
            error = CCAPI_RECEIVE_ERROR_LOCK_FAILED;
            goto done;
    }
done:
    return error;
}

ccapi_receive_error_t ccapi_receive_add_target(char const * const target, ccapi_receive_data_cb_t const data_cb, ccapi_receive_status_cb_t const status_cb, size_t const max_request_size)
{
    return ccxapi_receive_add_target(ccapi_data_single_instance, target, data_cb, status_cb, max_request_size);
}

ccapi_receive_error_t ccapi_receive_remove_target(char const * const target)
{
    return ccxapi_receive_remove_target(ccapi_data_single_instance, target);
}

#endif
