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

#ifndef CCIMP_OS_H_
#define CCIMP_OS_H_

#include "ccimp/ccimp_types.h"

#if (defined CCAPI_CONST_PROTECTION_UNLOCK)
#define CONST
#else
#define CONST   const
#endif

typedef void (* ccimp_os_thread_start_t) (void * const argument);

typedef enum {
    CCIMP_THREAD_FSM,
    CCIMP_THREAD_RCI,
    CCIMP_THREAD_RECEIVE,
    CCIMP_THREAD_CLI,
    CCIMP_THREAD_FIRMWARE
} ccimp_os_thread_type_t;

typedef struct
{
    ccimp_os_thread_type_t CONST type;
    ccimp_os_thread_start_t CONST start;
    void * CONST argument;
} ccimp_os_create_thread_info_t;

typedef struct {
    size_t CONST size;
    void * ptr;
} ccimp_os_malloc_t;

typedef struct {
    void CONST * CONST ptr;
} ccimp_os_free_t;

typedef struct {
    size_t CONST old_size;
    size_t CONST new_size;
    void * ptr;
} ccimp_os_realloc_t;

typedef struct {
    unsigned long sys_uptime;
} ccimp_os_system_up_time_t;

#define OS_LOCK_ACQUIRE_NOWAIT             0UL
#define OS_LOCK_ACQUIRE_INFINITE           -1UL

typedef struct {
    void CONST * lock;
} ccimp_os_lock_create_t;

typedef struct {
    void * CONST lock;
    unsigned long CONST timeout_ms;
    ccapi_bool_t acquired;
} ccimp_os_lock_acquire_t;

typedef struct {
    void * CONST lock;
} ccimp_os_lock_release_t;

typedef struct {
    void * CONST lock;
} ccimp_os_lock_destroy_t;

ccimp_status_t ccimp_os_malloc(ccimp_os_malloc_t * const malloc_info);
ccimp_status_t ccimp_os_free(ccimp_os_free_t * const free_info);
ccimp_status_t ccimp_os_realloc(ccimp_os_realloc_t * const realloc_info);

ccimp_status_t ccimp_os_create_thread(ccimp_os_create_thread_info_t * const create_thread_info);

ccimp_status_t ccimp_os_get_system_time(ccimp_os_system_up_time_t * const system_up_time);
ccimp_status_t ccimp_os_yield(void);

ccimp_status_t ccimp_os_lock_create(ccimp_os_lock_create_t * const data);
ccimp_status_t ccimp_os_lock_acquire(ccimp_os_lock_acquire_t * const data);
ccimp_status_t ccimp_os_lock_release(ccimp_os_lock_release_t * const data);
ccimp_status_t ccimp_os_lock_destroy(ccimp_os_lock_destroy_t * const data);


#endif
