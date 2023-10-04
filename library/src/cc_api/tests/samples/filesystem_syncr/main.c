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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "ccapi/ccapi.h"
#include "../source/ccapi_definitions.h"

#define VIRTUAL_DIR_1   "Sebastian Pastor"
#define LOCAL_DIR_1     "/home/spastor"
#define VIRTUAL_DIR_2   "Hector Bujanda"
#define LOCAL_DIR_2     "/home/spastor/cc_api"
#define VIRTUAL_DIR_3   "Arturo Buzarra"
#define LOCAL_DIR_3     "/etc"
#define VIRTUAL_DIR_4   "Pedro Perez"
#define LOCAL_DIR_4     "/home"
#define VIRTUAL_DIR_5   "Aaron Kurland"
#define LOCAL_DIR_5     "/usr/local"

typedef struct {
    char const * virtual;
    char const * local;
} add_dir_thread_argument_t;

typedef struct {
    char const * virtual;
    char const * local;
} remove_dir_thread_argument_t;

ccapi_start_error_t app_start_ccapi(void);
ccapi_tcp_start_error_t app_start_tcp_transport(void);

static void dump_virtual_dir_table(ccapi_fs_virtual_dir_t const * const first_virtual_dir_entry)
{
    int i;
    ccapi_fs_virtual_dir_t const * virtual_dir_entry = first_virtual_dir_entry;

    for (i = 0; virtual_dir_entry != NULL; i++)
    {
        printf("\nEntry\t#%d (%p)\n", i, (void *)virtual_dir_entry);
        printf("Virtual\t'%s'\n", virtual_dir_entry->virtual_dir);
        printf("Local\t'%s'\n", virtual_dir_entry->local_dir);
        printf("Next\t'%p'\n", (void *)virtual_dir_entry->next);
        virtual_dir_entry = virtual_dir_entry->next;
    }
}

int entry_is_in_list(ccapi_fs_virtual_dir_t const * const dir_entry, ccapi_fs_virtual_dir_t const * const first_dir_entry_list)
{
    int i;
    int is_in_list = 0;
    ccapi_fs_virtual_dir_t const * list_entry = first_dir_entry_list;

    for (i = 0; list_entry != NULL && !is_in_list; i++)
    {
        if (strcmp(dir_entry->local_dir, list_entry->local_dir) == 0 &&
            strcmp(dir_entry->virtual_dir, list_entry->virtual_dir) == 0)
        {
            is_in_list = 1;
        }
        list_entry = list_entry->next;
    }
    return is_in_list;
}

static int virtual_dir_lists_are_different(ccapi_fs_virtual_dir_t const * const first_dir_entry_1, ccapi_fs_virtual_dir_t const * const  first_dir_entry_2)
{
    ccapi_fs_virtual_dir_t const * dir_entry_1 = first_dir_entry_1;
    ccapi_fs_virtual_dir_t const * dir_entry_2 = first_dir_entry_2;

    int i;
    int are_different = 0;

    for (i = 0; dir_entry_1 != NULL && !are_different; i++)
    {
        if (!entry_is_in_list(dir_entry_1, dir_entry_2))
        {
            are_different = 1;
        }
        dir_entry_1 = dir_entry_1->next;
    }

    return are_different;
}

static void * thread_add_dir(void * arg)
{
    add_dir_thread_argument_t * argument = arg;
    ccapi_fs_error_t add_dir_error;

    add_dir_error = ccapi_fs_add_virtual_dir(argument->virtual, argument->local);
    if (add_dir_error != CCAPI_FS_ERROR_NONE)
    {
        printf("thread_add_dir failed with error %d\n", add_dir_error);
        assert(add_dir_error == CCAPI_FS_ERROR_NONE);
    }

    return NULL;
}

static pthread_t create_thread_add_dir(add_dir_thread_argument_t * arg)
{
    pthread_t pthread;

    int ccode = pthread_create(&pthread, NULL, thread_add_dir, arg);

    if (ccode != 0)
    {
        printf("create_thread_add_dir() error %d\n", ccode);
    }

    return pthread;
}

static void * thread_remove_dir(void * arg)
{
    remove_dir_thread_argument_t * argument = arg;
    ccapi_fs_error_t add_dir_error;

    add_dir_error = ccapi_fs_remove_virtual_dir(argument->virtual);
    if (add_dir_error != CCAPI_FS_ERROR_NONE)
    {
        printf("thread_add_dir failed with error %d\n", add_dir_error);
        assert(add_dir_error == CCAPI_FS_ERROR_NONE);
    }

    return NULL;
}

static pthread_t create_thread_remove_dir(remove_dir_thread_argument_t * arg)
{
    pthread_t pthread;

    int ccode = pthread_create(&pthread, NULL, thread_remove_dir, arg);

    if (ccode != 0)
    {
        printf("create_thread_add_dir() error %d\n", ccode);
    }

    return pthread;
}

int main (void)
{
    static add_dir_thread_argument_t add_arg1, add_arg2, add_arg3, add_arg4, add_arg5;
    static remove_dir_thread_argument_t remove_arg1, remove_arg2;
    int retval = 0;
    pthread_t thread_1, thread_2, thread_3, thread_4, thread_5;
    ccapi_fs_virtual_dir_t dir_entry_1, dir_entry_2, dir_entry_3, dir_entry_4, dir_entry_5;
    ccapi_fs_virtual_dir_t const * const expected_dir_list = &dir_entry_1;

    dir_entry_1.virtual_dir = VIRTUAL_DIR_1;
    dir_entry_1.local_dir = LOCAL_DIR_1;
    dir_entry_1.next = &dir_entry_2;

    dir_entry_2.virtual_dir = VIRTUAL_DIR_2;
    dir_entry_2.local_dir = LOCAL_DIR_2;
    dir_entry_2.next = &dir_entry_3;

    dir_entry_3.virtual_dir = VIRTUAL_DIR_3;
    dir_entry_3.local_dir = LOCAL_DIR_3;
    dir_entry_3.next = &dir_entry_4;

    dir_entry_4.virtual_dir = VIRTUAL_DIR_4;
    dir_entry_4.local_dir = LOCAL_DIR_4;
    dir_entry_4.next = &dir_entry_5;

    dir_entry_5.virtual_dir = VIRTUAL_DIR_5;
    dir_entry_5.local_dir = LOCAL_DIR_5;
    dir_entry_5.next = NULL;

    if (app_start_ccapi() != CCAPI_START_ERROR_NONE)
    {
        goto done;
    }

    puts("Starting adding test");
    add_arg1.virtual = VIRTUAL_DIR_1;
    add_arg1.local = LOCAL_DIR_1;
    thread_1 = create_thread_add_dir(&add_arg1);

    add_arg2.virtual = VIRTUAL_DIR_2;
    add_arg2.local = LOCAL_DIR_2;
    thread_2 = create_thread_add_dir(&add_arg2);

    add_arg3.virtual = VIRTUAL_DIR_3;
    add_arg3.local = LOCAL_DIR_3;
    thread_3 = create_thread_add_dir(&add_arg3);

    add_arg4.virtual = VIRTUAL_DIR_4;
    add_arg4.local = LOCAL_DIR_4;
    thread_4 = create_thread_add_dir(&add_arg4);

    add_arg5.virtual = VIRTUAL_DIR_5;
    add_arg5.local = LOCAL_DIR_5;
    thread_5 = create_thread_add_dir(&add_arg5);

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);
    pthread_join(thread_3, NULL);
    pthread_join(thread_4, NULL);
    pthread_join(thread_5, NULL);

    if (virtual_dir_lists_are_different(expected_dir_list, ccapi_data_single_instance->service.file_system.virtual_dir_list))
    {
        puts("\tERROR!!! Lists a re not equivalent");
        puts("\tExpected dir table:");
        dump_virtual_dir_table(expected_dir_list);
        puts("\nObtained dir table:");
        dump_virtual_dir_table(ccapi_data_single_instance->service.file_system.virtual_dir_list);
        retval = -1;
        goto done;
    }

    puts("\tBoth lists are equivalent!");

    puts("Starting removing test");
    remove_arg1.virtual = VIRTUAL_DIR_4;
    thread_1 = create_thread_remove_dir(&remove_arg1);

    remove_arg2.virtual = VIRTUAL_DIR_2;
    thread_2 = create_thread_remove_dir(&remove_arg2);

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);

    dir_entry_1.next = &dir_entry_3;
    dir_entry_3.next = &dir_entry_5;
    dir_entry_5.next = NULL;

    if (virtual_dir_lists_are_different(expected_dir_list, ccapi_data_single_instance->service.file_system.virtual_dir_list))
    {
        puts("\tERROR!!! Lists a re not equivalent");
        puts("\tExpected dir table:");
        dump_virtual_dir_table(expected_dir_list);
        puts("\nObtained dir table:");
        dump_virtual_dir_table(ccapi_data_single_instance->service.file_system.virtual_dir_list);
        retval = -1;
        goto done;
    }
    puts("\tBoth lists are equivalent!");

done:
    return retval;
}
