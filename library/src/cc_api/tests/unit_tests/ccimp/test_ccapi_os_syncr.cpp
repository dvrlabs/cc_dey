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

#include "test_helper_functions.h"

TEST_GROUP(test_ccapi_os_lock)
{
    void * lock;

    void setup()
    {
        lock = NULL;
        {
            ccimp_os_lock_create_t create_data;
            ccimp_status_t status;
    
            status = ccimp_os_lock_create(&create_data);

            CHECK(status == CCIMP_STATUS_OK);

            lock= create_data.lock;
        }
        CHECK(lock != NULL);

        Mock_create_all();
    }

    void teardown()
    {
        Mock_destroy_all();

        {
            ccimp_os_lock_destroy_t destroy_data;
            ccimp_status_t status;

            destroy_data.lock = lock;
        
            status = ccimp_os_lock_destroy(&destroy_data);

            CHECK(status == CCIMP_STATUS_OK);
        }
    }
};

TEST(test_ccapi_os_lock, CreateLeavesObjectInClearedState)
{
    ccimp_status_t status;

    ccimp_os_lock_acquire_t acquire_data;

    acquire_data.lock = lock;       
    acquire_data.timeout_ms= 100;

    /* Should fail to acquire the object as it's created in clear state */
    status = ccimp_os_lock_acquire(&acquire_data);

    CHECK(status == CCIMP_STATUS_OK);
    CHECK(acquire_data.acquired == CCAPI_FALSE);
}

static void * thread_release(void * argument)
{
    ccimp_status_t status;
    ccimp_os_lock_release_t release_data;

    release_data.lock = argument;
        
    status = ccimp_os_lock_release(&release_data);

    CHECK(status == CCIMP_STATUS_OK);

    return NULL;
}

pthread_t create_thread_release(void * argument)
{
    pthread_t pthread;
    int ccode = pthread_create(&pthread, NULL, thread_release, argument);

    if (ccode != 0)
    {
        printf("create_thread_release() error %d\n", ccode);
    }

    return pthread;
}

TEST(test_ccapi_os_lock, AcquireClearsObjectAutomatically)
{
    ccimp_os_lock_acquire_t acquire_data;
    ccimp_status_t status;

    acquire_data.lock = lock;

    create_thread_release(lock);

    /* Wait infinite. The other thread should release the object */
    acquire_data.timeout_ms= OS_LOCK_ACQUIRE_INFINITE;

    status = ccimp_os_lock_acquire(&acquire_data);

    CHECK(status == CCIMP_STATUS_OK);
    CHECK(acquire_data.acquired == CCAPI_TRUE);

    /* Should fail to acquire again the object */
    acquire_data.timeout_ms= OS_LOCK_ACQUIRE_NOWAIT;

    status = ccimp_os_lock_acquire(&acquire_data);

    CHECK(status == CCIMP_STATUS_OK);
    CHECK(acquire_data.acquired == CCAPI_FALSE);
}

/* This test fails as linux semaphores don't support max count.
   TODO: Should we delete that limitation?
 */
IGNORE_TEST(test_ccapi_os_lock, ReleaseMaxCountIs1)
{
    ccimp_os_lock_acquire_t acquire_data;
    ccimp_os_lock_release_t release_data;
    ccimp_status_t status;

    acquire_data.lock = lock;       
    release_data.lock = lock;       

    status = ccimp_os_lock_release(&release_data);
    CHECK(status == CCIMP_STATUS_OK);

    /* This should have no effect */
    status = ccimp_os_lock_release(&release_data);
    CHECK(status == CCIMP_STATUS_OK);

    /* Wait infinite. The object should be released */
    acquire_data.timeout_ms= OS_LOCK_ACQUIRE_INFINITE;

    status = ccimp_os_lock_acquire(&acquire_data);

    CHECK(status == CCIMP_STATUS_OK);
    CHECK(acquire_data.acquired == CCAPI_TRUE);

    /* Should fail to acquire again the object as Max Count is 1 */
    acquire_data.timeout_ms= OS_LOCK_ACQUIRE_NOWAIT;

    status = ccimp_os_lock_acquire(&acquire_data);

    CHECK(status == CCIMP_STATUS_OK);
    CHECK(acquire_data.acquired == CCAPI_FALSE);
}

