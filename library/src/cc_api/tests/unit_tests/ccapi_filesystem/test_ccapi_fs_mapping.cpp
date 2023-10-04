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

TEST_GROUP(test_ccapi_fs_mapping_no_CCAPI)
{
    void setup()
    {
        Mock_create_all();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_fs_mapping_no_CCAPI, testCcapiNotStarted)
{
    ccapi_fs_error_t error;
    char const * const dir_path = "/home/user/my_directory";

    error = ccapi_fs_add_virtual_dir("my_virtual_dir", dir_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_CCAPI_STOPPED, error);

    error = ccapi_fs_remove_virtual_dir("my_virtual_dir");
    CHECK_EQUAL(CCAPI_FS_ERROR_CCAPI_STOPPED, error);
}

TEST_GROUP(test_ccapi_fs_mapping_no_filesystem)
{
    void setup()
    {
        Mock_create_all();
        th_start_ccapi();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_fs_mapping_no_filesystem, testCcapiNotStarted)
{
    ccapi_fs_error_t error;
    char const * const dir_path = "/home/user/my_directory";

    error = ccapi_fs_add_virtual_dir("my_virtual_dir", dir_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_NO_FS_SUPPORT, error);

    error = ccapi_fs_remove_virtual_dir("my_virtual_dir");
    CHECK_EQUAL(CCAPI_FS_ERROR_NO_FS_SUPPORT, error);
}

TEST_GROUP(test_ccapi_fs_mapping)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_filesystem_service_t fs_service = {NULL, NULL};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.file_system = &fs_service;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(fs_service.changed, ccapi_data_single_instance->service.file_system.user_callback.changed);
        CHECK_EQUAL(fs_service.access, ccapi_data_single_instance->service.file_system.user_callback.access);
        /* Simulate that imp_context was previously set by other call (file_open) */
        ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_fs_mapping, testInvalidPath)
{
    ccapi_fs_error_t error;
    char const * const local_path = "/home/user";
    char const * const virtual_path = "my_data";
    char const * const invalid_virtual_dir = "/my_data";
    char const * const invalid_virtual_dir_with_subdir = "my_data/subdir";
    char const * const invalid_virtual_dir_with_backslash = "my_data\\subdir";
    char const * const invalid_virtual_dir_too_long = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghiklmnopqrstuvwxy0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghiklmnopqrstuvwxy0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghiklmnopqrstuvwxy0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghiklmnopqrstuvwxy0123456789_ABCDEFGH";

    error = ccapi_fs_add_virtual_dir("", local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(NULL, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(virtual_path, "");
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(virtual_path, NULL);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(invalid_virtual_dir, NULL);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(invalid_virtual_dir, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(invalid_virtual_dir_with_subdir, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(invalid_virtual_dir_with_subdir, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(invalid_virtual_dir_with_backslash, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);

    error = ccapi_fs_add_virtual_dir(invalid_virtual_dir_too_long, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INVALID_PATH, error);
}

TEST(test_ccapi_fs_mapping, testMapAFile)
{
    ccapi_fs_error_t error;
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data;

    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data, "/notadir.txt");

    error = ccapi_fs_add_virtual_dir("my_virtual_dir", "/notadir.txt");
    CHECK_EQUAL(CCAPI_FS_ERROR_NOT_A_DIR, error);
}

TEST(test_ccapi_fs_mapping, testMapDirNoMemory4DirEntry)
{
    ccapi_fs_error_t error;
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data;
    char const * const local_path = "/home/user/my_directory";
    char const * const virtual_path = "my_virtual_dir";
    /* Test Setup */
    {
        void * malloc_for_dir_list_entry = NULL;
        Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_fs_virtual_dir_t), malloc_for_dir_list_entry);
    }

    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data, local_path);
    /* Test */
    error = ccapi_fs_add_virtual_dir(virtual_path, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INSUFFICIENT_MEMORY, error);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list == NULL);
}

TEST(test_ccapi_fs_mapping, testMapDirNoMemory4ActualPath)
{
    ccapi_fs_error_t error;
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data;
    char const * const local_path = "/home/user/my_directory";
    char const * const virtual_path = "my_virtual_dir";
    /* Test Setup */
    {
        void * malloc_for_dir_list_entry = malloc(sizeof (ccapi_fs_virtual_dir_t));
        void * malloc_for_local_path = NULL;

        Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_fs_virtual_dir_t), malloc_for_dir_list_entry);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(local_path) + 1, malloc_for_local_path);
        Mock_ccimp_os_free_expectAndReturn(malloc_for_dir_list_entry, CCIMP_STATUS_OK);
    }
    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data, local_path);

    /* Test */
    error = ccapi_fs_add_virtual_dir(virtual_path, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INSUFFICIENT_MEMORY, error);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list == NULL);
}

TEST(test_ccapi_fs_mapping, testMapDirNoMemory4VirtualPath)
{
    ccapi_fs_error_t error;
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data;
    char const * const local_path = "/home/user/my_directory";
    char const * const virtual_path = "my_virtual_dir";
    /* Test Setup */
    {
        void * malloc_for_dir_list_entry = malloc(sizeof (ccapi_fs_virtual_dir_t));
        void * malloc_for_local_path = malloc(strlen(local_path) + 1);
        void * malloc_for_virtual_path = NULL;

        Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_fs_virtual_dir_t), malloc_for_dir_list_entry);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(local_path) + 1, malloc_for_local_path);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(virtual_path) + 1, malloc_for_virtual_path);
        Mock_ccimp_os_free_expectAndReturn(malloc_for_dir_list_entry, CCIMP_STATUS_OK);
        Mock_ccimp_os_free_expectAndReturn(malloc_for_local_path, CCIMP_STATUS_OK);
    }
    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data, local_path);

    /* Test */
    error = ccapi_fs_add_virtual_dir(virtual_path, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_INSUFFICIENT_MEMORY, error);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list == NULL);
}

TEST(test_ccapi_fs_mapping, testMapDirOK)
{
    ccapi_fs_error_t error;
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data;
    char const * const local_path = "/home/user/my_directory";
    char const * const virtual_path = "my_virtual_dir";
    /* Test Setup */
    {
        void * malloc_for_dir_list_entry = malloc(sizeof (ccapi_fs_virtual_dir_t));
        void * malloc_for_local_path = malloc(strlen(local_path) + 1);
        void * malloc_for_virtual_path = malloc(strlen(virtual_path) + 1);

        Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_fs_virtual_dir_t), malloc_for_dir_list_entry);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(local_path) + 1, malloc_for_local_path);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(virtual_path) + 1, malloc_for_virtual_path);
    }
    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data, local_path);

    /* Test */
    error = ccapi_fs_add_virtual_dir(virtual_path, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_NONE, error);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list != NULL);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list->next == NULL);
    STRCMP_EQUAL(virtual_path, ccapi_data_single_instance->service.file_system.virtual_dir_list->virtual_dir);
    STRCMP_EQUAL(local_path, ccapi_data_single_instance->service.file_system.virtual_dir_list->local_dir);

    /* Map to the same virtual dir and it will fail */
    error = ccapi_fs_add_virtual_dir(virtual_path, local_path);
    CHECK_EQUAL(CCAPI_FS_ERROR_ALREADY_MAPPED, error);

    error = ccapi_fs_add_virtual_dir(virtual_path, "/a/different/path");
    CHECK_EQUAL(CCAPI_FS_ERROR_ALREADY_MAPPED, error);
}

TEST(test_ccapi_fs_mapping, testMapTwoDirs)
{
    ccapi_fs_error_t error;
    ccimp_fs_dir_entry_status_t ccimp_fs_dir_entry_status_data_1, ccimp_fs_dir_entry_status_data_2;
    char const * const local_path_1 = "/home/user/my_directory";
    char const * const virtual_path_1 = "my_virtual_dir";
    char const * const local_path_2 = "/home/other_user/other_my_directory";
    char const * const virtual_path_2 = "other_virtual_dir";
    /* Test Setup */
    {
        void * malloc_for_dir_list_entry_1 = malloc(sizeof (ccapi_fs_virtual_dir_t));
        void * malloc_for_local_path_1 = malloc(strlen(local_path_1) + 1);
        void * malloc_for_virtual_path_1 = malloc(strlen(virtual_path_1) + 1);
        void * malloc_for_dir_list_entry_2 = malloc(sizeof (ccapi_fs_virtual_dir_t));
        void * malloc_for_local_path_2 = malloc(strlen(local_path_2) + 1);
        void * malloc_for_virtual_path_2 = malloc(strlen(virtual_path_2) + 1);

        Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_fs_virtual_dir_t), malloc_for_dir_list_entry_1);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(local_path_1) + 1, malloc_for_local_path_1);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(virtual_path_1) + 1, malloc_for_virtual_path_1);
        Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_fs_virtual_dir_t), malloc_for_dir_list_entry_2);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(local_path_2) + 1, malloc_for_local_path_2);
        Mock_ccimp_os_malloc_expectAndReturn(strlen(virtual_path_2) + 1, malloc_for_virtual_path_2);
    }
    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data_1, local_path_1);
    th_filesystem_prepare_ccimp_dir_entry_status_call(&ccimp_fs_dir_entry_status_data_2, local_path_2);

    /* Test */
    error = ccapi_fs_add_virtual_dir(virtual_path_1, local_path_1);
    CHECK_EQUAL(CCAPI_FS_ERROR_NONE, error);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list != NULL);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list->next == NULL);
    STRCMP_EQUAL(virtual_path_1, ccapi_data_single_instance->service.file_system.virtual_dir_list->virtual_dir);
    STRCMP_EQUAL(local_path_1, ccapi_data_single_instance->service.file_system.virtual_dir_list->local_dir);

    error = ccapi_fs_add_virtual_dir(virtual_path_2, local_path_2);
    CHECK_EQUAL(CCAPI_FS_ERROR_NONE, error);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list != NULL);
    CHECK(ccapi_data_single_instance->service.file_system.virtual_dir_list->next != NULL);
    STRCMP_EQUAL(virtual_path_2, ccapi_data_single_instance->service.file_system.virtual_dir_list->virtual_dir);
    STRCMP_EQUAL(local_path_2, ccapi_data_single_instance->service.file_system.virtual_dir_list->local_dir);
    STRCMP_EQUAL(virtual_path_1, ccapi_data_single_instance->service.file_system.virtual_dir_list->next->virtual_dir);
    STRCMP_EQUAL(local_path_1, ccapi_data_single_instance->service.file_system.virtual_dir_list->next->local_dir);
}
