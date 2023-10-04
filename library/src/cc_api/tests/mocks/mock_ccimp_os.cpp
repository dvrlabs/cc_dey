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

#include <pthread.h>

#define CCAPI_CONST_PROTECTION_UNLOCK
#include "mocks.h"

void Mock_ccimp_os_malloc_create(void)
{
    return;
}

void Mock_ccimp_os_malloc_destroy(void)
{
    mock("ccimp_os_malloc").checkExpectations();
}

void Mock_ccimp_os_malloc_expectAndReturn(size_t expect, void * retval)
{
    mock("ccimp_os_malloc").expectOneCall("ccimp_os_malloc")
            .withParameter("size", (int)expect)
            .andReturnValue(retval);

    /* If we are calling expectations, then override default malloc */
    mock("ccimp_os_malloc").setData("behavior", MOCK_MALLOC_ENABLED);
}

void Mock_ccimp_os_free_create(void)
{
    return;
}

void Mock_ccimp_os_free_destroy(void)
{
    mock("ccimp_os_free").checkExpectations();
}

void Mock_ccimp_os_free_expectAndReturn(void * ptr, ccimp_status_t retval)
{
    /* Pass a NULL pointer if you don't know beforehand the value */
    if (ptr != NULL)
    {
        mock("ccimp_os_free").expectOneCall("ccimp_os_free")
            .withParameter("ptr", (void *)ptr)
            .andReturnValue((int)retval);

        mock("ccimp_os_free").setData("behavior", MOCK_FREE_ENABLED_CHECK_PARAMETER);
    }
    else
    {
        mock("ccimp_os_free").expectOneCall("ccimp_os_free")
            .andReturnValue((int)retval);

        mock("ccimp_os_free").setData("behavior", MOCK_FREE_ENABLED_DONT_CHECK_PARAMETER);
    }
}

void Mock_ccimp_os_free_notExpected(void)
{
    mock("ccimp_os_free").setData("behavior", MOCK_FREE_ENABLED_NOT_EXPECTED);
}

void Mock_ccimp_os_create_thread_create(void)
{
    mock().installComparator("ccimp_os_create_thread_info_t", ccimp_create_thread_info_t_comparator);
    return;
}

void Mock_ccimp_os_create_thread_destroy(void)
{
    mock("ccimp_create_thread").checkExpectations();
}

void Mock_ccimp_os_create_thread_expectAndReturn(ccimp_os_create_thread_info_t * const create_thread_info, mock_thread_bahavior_t behavior, ccimp_status_t retval)
{
    mock("ccimp_create_thread").expectOneCall("ccimp_create_thread")
            .withParameterOfType("ccimp_os_create_thread_info_t", "parameterName", create_thread_info)
            .andReturnValue(retval);

    mock("ccimp_create_thread").setData("behavior", behavior);
}

void Mock_ccimp_os_get_system_time_create(void)
{
    return;
}

void Mock_ccimp_os_get_system_time_destroy(void)
{
    mock("ccimp_os_get_system_time").checkExpectations();
}

void Mock_ccimp_os_get_system_time_return(unsigned long retval)
{
    mock("ccimp_os_get_system_time").expectOneCall("ccimp_os_get_system_time").andReturnValue((int)retval);
}

void Mock_ccimp_os_lock_create_create(void)
{
    mock("ccimp_os_lock_create").setData("behavior", MOCK_MALLOC_DISABLED);
    return;
}
void Mock_ccimp_os_lock_create_destroy(void)
{
    mock("ccimp_os_lock_create").checkExpectations();
}

void Mock_ccimp_os_lock_create_return(unsigned long retval)
{
    mock("ccimp_os_lock_create").expectOneCall("ccimp_os_lock_create").andReturnValue((int)retval);
    mock("ccimp_os_lock_create").setData("behavior", MOCK_MALLOC_ENABLED);
}

void Mock_ccimp_os_lock_acquire_create(void)
{
    mock("ccimp_os_lock_acquire").setData("behavior", MOCK_MALLOC_DISABLED);
    return;
}
void Mock_ccimp_os_lock_acquire_destroy(void)
{
    mock("ccimp_os_lock_acquire").checkExpectations();
}

void Mock_ccimp_os_lock_acquire_return(unsigned long retval)
{
    mock("ccimp_os_lock_acquire").expectOneCall("ccimp_os_lock_acquire").andReturnValue((int)retval);
    mock("ccimp_os_lock_acquire").setData("behavior", MOCK_MALLOC_ENABLED);
}

void Mock_ccimp_os_lock_release_create(void)
{
    mock("ccimp_os_lock_release").setData("behavior", MOCK_MALLOC_DISABLED);
    return;
}

void Mock_ccimp_os_lock_release_destroy(void)
{
    mock("ccimp_os_lock_release").checkExpectations();
}

void Mock_ccimp_os_lock_release_return(unsigned long retval)
{
    mock("ccimp_os_lock_release").expectOneCall("ccimp_os_lock_release").andReturnValue((int)retval);
    mock("ccimp_os_lock_release").setData("behavior", MOCK_MALLOC_ENABLED);
}

extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "ccapi_definitions.h"

ccimp_status_t ccimp_os_create_thread(ccimp_os_create_thread_info_t * const create_thread_info)
{
    uint8_t behavior;

    behavior = mock_scope_c("ccimp_create_thread")->getData("behavior").value.intValue;

    if (behavior == MOCK_THREAD_DISABLED)
    {
        /* Do not report actualCall */

        /* Create thread correctly */
        return ccimp_os_create_thread_real(create_thread_info);
    }
    else if (behavior == MOCK_THREAD_ENABLED_NORMAL)
    {
        mock_scope_c("ccimp_create_thread")->actualCall("ccimp_create_thread")->withParameterOfType("ccimp_os_create_thread_info_t", "parameterName", create_thread_info);

        /* Create thread correctly */
        return ccimp_os_create_thread_real(create_thread_info);
    }
    else if (behavior == MOCK_THREAD_ENABLED_DONT_CREATE_THREAD)
    {
        mock_scope_c("ccimp_create_thread")->actualCall("ccimp_create_thread")->withParameterOfType("ccimp_os_create_thread_info_t", "parameterName", create_thread_info);

        /* Don't create thread, return FALSE */
        return CCIMP_STATUS_ERROR;
    }
    else if (behavior == MOCK_THREAD_ENABLED_ARGUMENT_NULL)
    {
        mock_scope_c("ccimp_create_thread")->actualCall("ccimp_create_thread")->withParameterOfType("ccimp_os_create_thread_info_t", "parameterName", create_thread_info);

        /* Create thread setting argument to NULL */
        create_thread_info->argument = NULL;
        return ccimp_os_create_thread_real(create_thread_info);
    }

    return (ccimp_status_t)mock_scope_c("ccimp_create_thread")->returnValue().value.intValue;
}

ccimp_status_t ccimp_os_malloc(ccimp_os_malloc_t * const malloc_info)
{
    uint8_t behavior;

    behavior = mock_scope_c("ccimp_os_malloc")->getData("behavior").value.intValue;
    switch(behavior)
    {
        case MOCK_MALLOC_ENABLED:
        {
            mock_scope_c("ccimp_os_malloc")->actualCall("ccimp_os_malloc")->withIntParameters("size", malloc_info->size);
            malloc_info->ptr = mock_scope_c("ccimp_os_malloc")->returnValue().value.pointerValue;
            break;
        }
        case MOCK_MALLOC_DISABLED:
        default:
        {
            /* Skip mocking, use default malloc implementation */
            ccimp_os_malloc_real(malloc_info);
            memset(malloc_info->ptr, 0xFF, malloc_info->size); /* Try to catch hidden problems */
            break;
        }
    }

    return malloc_info->ptr == NULL ? CCIMP_STATUS_ERROR : CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_free(ccimp_os_free_t * const free_info)
{
    uint8_t behavior;
    ccimp_status_t retval = CCIMP_STATUS_OK;

    behavior = mock_scope_c("ccimp_os_free")->getData("behavior").value.intValue;
    switch(behavior)
    {
        case MOCK_FREE_ENABLED_CHECK_PARAMETER:
        {
            mock_scope_c("ccimp_os_free")->actualCall("ccimp_os_free")->withPointerParameters("ptr", free_info->ptr);
            retval = (ccimp_status_t)mock_scope_c("ccimp_os_free")->returnValue().value.intValue;
            break;
        }
        case MOCK_FREE_ENABLED_DONT_CHECK_PARAMETER:
        {
            mock_scope_c("ccimp_os_free")->actualCall("ccimp_os_free");
            retval =  (ccimp_status_t)mock_scope_c("ccimp_os_free")->returnValue().value.intValue;
            break;
        }
        case MOCK_FREE_ENABLED_NOT_EXPECTED:
        {
            mock_scope_c("ccimp_os_free")->actualCall("ccimp_os_free");
            break;
        }
        default:
        {
            ccimp_os_free_real(free_info);
            break;
        }
    }

    return retval;
}

ccimp_status_t ccimp_os_realloc(ccimp_os_realloc_t * const realloc_info)
{
    return ccimp_os_realloc_real(realloc_info);
}

ccimp_status_t ccimp_os_get_system_time(ccimp_os_system_up_time_t * const system_up_time)
{
    mock_scope_c("ccimp_os_get_system_time")->actualCall("ccimp_os_get_system_time");
    system_up_time->sys_uptime = mock_scope_c("ccimp_os_get_system_time")->returnValue().value.intValue;
    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_yield(void)
{
    pthread_testcancel();
    return ccimp_os_yield_real();
}

ccimp_status_t ccimp_os_lock_create(ccimp_os_lock_create_t * const data)
{
    int const behavior = mock_scope_c("ccimp_os_lock_create")->getData("behavior").value.intValue;
    ccimp_status_t ccimp_status;
    switch (behavior)
    {
        case MOCK_LOCK_DISABLED:
            ccimp_status = ccimp_os_lock_create_real(data);
            break;
        default:
            mock_scope_c("ccimp_os_lock_create")->actualCall("ccimp_os_lock_create");
            ccimp_status = (ccimp_status_t)mock_scope_c("ccimp_os_lock_create")->returnValue().value.intValue;
            break;
    }

    return ccimp_status;
}

ccimp_status_t ccimp_os_lock_acquire(ccimp_os_lock_acquire_t * const data)
{
    int const behavior = mock_scope_c("ccimp_os_lock_acquire")->getData("behavior").value.intValue;
    ccimp_status_t ccimp_status;
    switch (behavior)
    {
        case MOCK_LOCK_DISABLED:
            ccimp_status = ccimp_os_lock_acquire_real(data);
            break;
        default:
            mock_scope_c("ccimp_os_lock_acquire")->actualCall("ccimp_os_lock_acquire");
            ccimp_status = (ccimp_status_t)mock_scope_c("ccimp_os_lock_acquire")->returnValue().value.intValue;
            break;
    }

    return ccimp_status;
}

ccimp_status_t ccimp_os_lock_release(ccimp_os_lock_release_t * const data)
{
    int const behavior = mock_scope_c("ccimp_os_lock_release")->getData("behavior").value.intValue;
    ccimp_status_t ccimp_status;
    switch (behavior)
    {
        case MOCK_LOCK_DISABLED:
            ccimp_status = ccimp_os_lock_release_real(data);
            break;
        default:
            mock_scope_c("ccimp_os_lock_release")->actualCall("ccimp_os_lock_release");
            ccimp_status = (ccimp_status_t)mock_scope_c("ccimp_os_lock_release")->returnValue().value.intValue;
            break;
    }

    return ccimp_status;
}

}
