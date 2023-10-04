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

#include "ccapi/ccapi.h"
#include "ccimp/ccimp_os.h"

#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#if (defined UNIT_TEST)
#define ccimp_os_malloc             ccimp_os_malloc_real
#define ccimp_os_free               ccimp_os_free_real
#define ccimp_os_realloc            ccimp_os_realloc_real
#define ccimp_os_create_thread      ccimp_os_create_thread_real
#define ccimp_os_get_system_time    ccimp_os_get_system_time_real
#define ccimp_os_yield              ccimp_os_yield_real
#define ccimp_os_lock_create       ccimp_os_lock_create_real
#define ccimp_os_lock_acquire      ccimp_os_lock_acquire_real
#define ccimp_os_lock_release      ccimp_os_lock_release_real
#endif

#define ccapi_logging_line_info(message) /* TODO */

/******************** LINUX IMPLEMENTATION ********************/

ccimp_status_t ccimp_os_malloc(ccimp_os_malloc_t * const malloc_info)
{
    malloc_info->ptr = malloc(malloc_info->size);

    return malloc_info->ptr == NULL ? CCIMP_STATUS_ERROR : CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_free(ccimp_os_free_t * const free_info)
{
    free((void *)free_info->ptr);

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_realloc(ccimp_os_realloc_t * const realloc_info)
{
    ccimp_status_t status = CCIMP_STATUS_OK;

    realloc_info->ptr = realloc(realloc_info->ptr, realloc_info->new_size);
    if (realloc_info->ptr == NULL)
    {
        status = CCIMP_STATUS_ERROR;
    }

    return status;
}

static void * thread_wrapper(void * argument)
{
    ccimp_os_create_thread_info_t * create_thread_info = (ccimp_os_create_thread_info_t *)argument;

    create_thread_info->start(create_thread_info->argument);

    return NULL;
}

typedef struct thread_info {
    pthread_t thread;
    struct thread_info * next;
} thread_info_t;

static thread_info_t * thread_info_list = NULL;
static void add_thread_info(pthread_t thread)
{
    thread_info_t * const new_thread_info = malloc(sizeof *new_thread_info);

    new_thread_info->thread = thread;
    new_thread_info->next = thread_info_list;
    thread_info_list = new_thread_info;
}

void wait_for_ccimp_threads(void)
{
    while (thread_info_list != NULL)
    {
        thread_info_t * const next = thread_info_list->next;

        pthread_join(thread_info_list->thread, NULL);
        free(thread_info_list);
        thread_info_list = next;
    }
}

static void graceful_shutdown(void)
{
    ccapi_stop(CCAPI_STOP_GRACEFULLY);
    wait_for_ccimp_threads();
}

static void sigint_handler(int signum)
{
    printf("received signal %d to close CCAPI.\n", signum);
    exit(0);
}

void add_sigkill_signal(void)
{
    struct sigaction new_action;
    struct sigaction old_action;

    /* setup signal hander */
    atexit(graceful_shutdown);
    new_action.sa_handler = sigint_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGINT, &new_action, NULL);
}

ccimp_status_t ccimp_os_create_thread(ccimp_os_create_thread_info_t * const create_thread_info)
{
    pthread_t pthread;
    int ccode;
    pthread_attr_t attr;

    ccode = pthread_attr_init(&attr);
    if (ccode != 0)
    {
        printf("pthread_attr_init() error %d\n", ccode);
        return (CCIMP_STATUS_ERROR);
    }

#if (defined UNIT_TEST)
    {
        /* Use smaller stacks for threads so unit tests do not use too much memory.
         * Don't do this on actual applications or the memory will be leaked.
         */
        int stack_size;
        void * sp;
        int s;

        switch(create_thread_info->type)
        {
            case CCIMP_THREAD_FSM:
                stack_size = 100 * 1024;
                break;
            case CCIMP_THREAD_RCI:
            case CCIMP_THREAD_RECEIVE:
            case CCIMP_THREAD_CLI:
            case CCIMP_THREAD_FIRMWARE:
                stack_size = 100 * 1024;
                break;
        }

        s = posix_memalign(&sp, sysconf(_SC_PAGESIZE), stack_size);
        if (s != 0)
        {
            printf("error in posix_memalign\n");
            return (CCIMP_STATUS_ERROR);
        }

        /*printf("posix_memalign() allocated at %p\n", sp);*/

        s = pthread_attr_setstack(&attr, sp, stack_size);
        if (s != 0)
        {
            printf("error in pthread_attr_setstack\n");
            return (CCIMP_STATUS_ERROR);
        }
    }
#endif
    ccode = pthread_create(&pthread, &attr, thread_wrapper, create_thread_info);
    if (ccode != 0)
    {
        printf("pthread_create() error %d\n", ccode);
        return (CCIMP_STATUS_ERROR);
    }
    add_thread_info(pthread);
    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_get_system_time(ccimp_os_system_up_time_t * const system_up_time)
{
    static time_t start_system_up_time;
    time_t present_time;

    time(&present_time);

    if (start_system_up_time == 0)
       start_system_up_time = present_time;

    present_time -= start_system_up_time;
    system_up_time->sys_uptime = (unsigned long) present_time;

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_yield(void)
{
    int error;

    error = sched_yield();
    if (error)
    {
        /* In the Linux implementation this function always succeeds */
        printf("app_os_yield: sched_yield failed with %d\n", error);
    }

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_lock_create(ccimp_os_lock_create_t * const data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    sem_t * const sem = (sem_t *) malloc(sizeof *sem);

    if (sem == NULL)
    {
        printf("ccimp_os_lock_create insufficent memory\n");
        status = CCIMP_STATUS_ERROR;
        goto done;
    }

    if (sem_init(sem, 0, 0) == -1)
    {
        printf("ccimp_os_lock_create error\n");
        free(sem);
        status = CCIMP_STATUS_ERROR;
        goto done;
    }

    data->lock = sem;
done:
    return status;
}

ccimp_status_t ccimp_os_lock_acquire(ccimp_os_lock_acquire_t * const data)
{
    struct timespec ts = { 0 };
    int s;
    sem_t * sem = data->lock;

    assert(sem);

    data->acquired = CCAPI_FALSE;

    if (data->timeout_ms == OS_LOCK_ACQUIRE_NOWAIT)
    {
        ccapi_logging_line_info("ccimp_os_lock_acquire: about to call sem_trywait()\n");
        s = sem_trywait(sem);
    }
    else if (data->timeout_ms == OS_LOCK_ACQUIRE_INFINITE)
    {
        ccapi_logging_line_info("ccimp_os_lock_acquire: about to call sem_wait()\n");
        s = sem_wait(sem);
    }
    else
    {
        /* Calculate relative interval as current time plus number of milliseconds requested */
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            printf("ccimp_os_lock_acquire: clock_gettime error\n");
            return CCIMP_STATUS_ERROR;
        }

        ts.tv_sec += data->timeout_ms / 1000;
        ts.tv_nsec += (data->timeout_ms % 1000) * 1000 * 1000;

        /* Adjust if nsec rolls-over 999999999 */

        #define NSEC_ROLL_OVER (1 * 1000 * 1000 * 1000)

        if (ts.tv_nsec >= NSEC_ROLL_OVER)
        {   
            ts.tv_sec += 1;
            ts.tv_nsec %= NSEC_ROLL_OVER;
        }

        ccapi_logging_line_info("ccimp_os_lock_acquire: about to call sem_timedwait()\n");
        s = sem_timedwait(sem, &ts);
    }

    /* Check what happened */
    if (s == -1) 
    {
        if (errno == ETIMEDOUT)
        {
            ccapi_logging_line_info("ccimp_os_lock_acquire: timed out\n");
        }
        else if (data->timeout_ms == OS_LOCK_ACQUIRE_NOWAIT && errno == EAGAIN)
        {
            ccapi_logging_line_info("ccimp_os_lock_acquire: not signaled\n");
        }
        else
        {
            perror("sem_timedwait");
            return CCIMP_STATUS_ERROR;
        }
    } 
    else
    {
        ccapi_logging_line_info("ccimp_os_lock_acquire: got it\n");
        data->acquired = CCAPI_TRUE;
    }

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_lock_release(ccimp_os_lock_release_t * const data)
{
    sem_t * const sem = data->lock;

    assert(sem);

    if (sem_post(sem) == -1) 
    {
        printf("ccimp_os_lock_release error\n");
        return CCIMP_STATUS_ERROR;
    }

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_os_lock_destroy(ccimp_os_lock_destroy_t * const data)
{
    sem_t * const sem = data->lock;

    assert(sem);

    if (sem_destroy(sem) == -1) 
    {
        printf("ccimp_os_lock_destroy error\n");
        return CCIMP_STATUS_ERROR;
    }

    free(sem); 

    return CCIMP_STATUS_OK;
}
