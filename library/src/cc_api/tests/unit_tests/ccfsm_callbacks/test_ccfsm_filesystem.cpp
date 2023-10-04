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
#include <errno.h>

ccimp_fs_errnum_t expected_errnum[20];

#define FS_OPEN_ERRNUM_INDEX    0
#define FS_READ_ERRNUM_INDEX    1
#define FS_WRITE_ERRNUM_INDEX   2
#define FS_SEEK_ERRNUM_INDEX    3
#define FS_CLOSE_ERRNUM_INDEX   4
#define FS_REMOVE_ERRNUM_INDEX  5
#define FS_DIROPEN_ERRNUM_INDEX  6
#define FS_DIRREAD_ERRNUM_INDEX  7
#define FS_DIRSTAT_ERRNUM_INDEX  8
#define FS_DIRCLOSE_ERRNUM_INDEX  9
#define FS_HASHSTAT_ERRNUM_INDEX    10
#define FS_HASHFILE_ERRNUM_INDEX    11
#define FS_TRUNCATE_ERRNUM_INDEX    12

TEST_GROUP(test_ccfsm_filesystem)
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
        expected_errnum[FS_OPEN_ERRNUM_INDEX] = EAGAIN;
        expected_errnum[FS_READ_ERRNUM_INDEX] = ETIMEDOUT;
        expected_errnum[FS_WRITE_ERRNUM_INDEX] = ENODATA;
        expected_errnum[FS_SEEK_ERRNUM_INDEX] = EINVAL;
        expected_errnum[FS_CLOSE_ERRNUM_INDEX] = EROFS;
        expected_errnum[FS_REMOVE_ERRNUM_INDEX] = EACCES;
        expected_errnum[FS_DIROPEN_ERRNUM_INDEX] = ENOTDIR;
        expected_errnum[FS_DIRREAD_ERRNUM_INDEX] = ENOSYS;
        expected_errnum[FS_DIRSTAT_ERRNUM_INDEX] = ENOMEM;
        expected_errnum[FS_DIRCLOSE_ERRNUM_INDEX] = EINVAL;
        expected_errnum[FS_HASHSTAT_ERRNUM_INDEX] = ENAMETOOLONG;
        expected_errnum[FS_HASHFILE_ERRNUM_INDEX] = ENOSPC;
        expected_errnum[FS_TRUNCATE_ERRNUM_INDEX] = EIO;
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccfsm_filesystem, testFileOpen)
{
    connector_request_id_t request;
    ccimp_fs_file_open_t ccimp_open_data;
    connector_file_system_open_t ccfsm_open_data;
    connector_callback_status_t status;

    ccimp_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_open_data.imp_context = NULL;
    ccimp_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccimp_open_data.flags = CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND;
    ccimp_open_data.path = "/tmp/hello.txt";

    ccfsm_open_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.handle = CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccfsm_open_data.oflag = CONNECTOR_FILE_O_RDWR | CONNECTOR_FILE_O_APPEND;
    ccfsm_open_data.path = ccimp_open_data.path;
    ccfsm_open_data.user_context = NULL;

    Mock_ccimp_fs_file_open_expectAndReturn(&ccimp_open_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_open;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(&my_fs_context, ccapi_data_single_instance->service.file_system.imp_context);
    CHECK(ccfsm_open_data.handle != CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED);
    CHECK(ccfsm_open_data.errnum == CONNECTOR_FILESYSTEM_ERRNUM_NONE);
}

TEST(test_ccfsm_filesystem, testFileOpenFails)
{
    connector_request_id_t request;
    ccimp_fs_file_open_t ccimp_open_data;
    connector_file_system_open_t ccfsm_open_data;
    connector_callback_status_t status;

    ccimp_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_open_data.imp_context = NULL;
    ccimp_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccimp_open_data.flags = CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND;
    ccimp_open_data.path = "/tmp/hello.txt";

    ccfsm_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.handle = CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccfsm_open_data.oflag = CONNECTOR_FILE_O_RDWR | CONNECTOR_FILE_O_APPEND;
    ccfsm_open_data.path = ccimp_open_data.path;
    ccfsm_open_data.user_context = NULL;

    Mock_ccimp_fs_file_open_expectAndReturn(&ccimp_open_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_open;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(&my_fs_context, ccapi_data_single_instance->service.file_system.imp_context);
    CHECK(ccfsm_open_data.handle == CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_open_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_OPEN_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testFileRead)
{
    connector_request_id_t request;
    ccimp_fs_file_read_t ccimp_read_data;
    connector_file_system_read_t ccfsm_read_data;
    connector_callback_status_t status;
    uint8_t buffer[128] = {0x00};
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_CREAT);

    ccimp_read_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_read_data.imp_context = &my_fs_context;
    ccimp_read_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_read_data.buffer = buffer;
    ccimp_read_data.bytes_available = 128;
    ccimp_read_data.bytes_used = 0;

    ccfsm_read_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_read_data.user_context = NULL;
    ccfsm_read_data.handle = ccfsm_open_data.handle;
    ccfsm_read_data.buffer = ccimp_read_data.buffer;
    ccfsm_read_data.bytes_available = ccimp_read_data.bytes_available;
    ccfsm_read_data.bytes_used = ccimp_read_data.bytes_used;

    Mock_ccimp_fs_file_read_expectAndReturn(&ccimp_read_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_read;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_read_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(sizeof "testFileRead" == ccfsm_read_data.bytes_used);
    STRCMP_EQUAL("testFileRead", (char *)ccfsm_read_data.buffer);
    CHECK_EQUAL(1, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
}

TEST(test_ccfsm_filesystem, testFileReadFails)
{
    connector_request_id_t request;
    ccimp_fs_file_read_t ccimp_read_data;
    connector_file_system_read_t ccfsm_read_data;
    connector_callback_status_t status;
    uint8_t buffer[128] = {0x00};
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_CREAT);

    ccimp_read_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_read_data.imp_context = &my_fs_context;
    ccimp_read_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_read_data.buffer = buffer;
    ccimp_read_data.bytes_available = 128;
    ccimp_read_data.bytes_used = 0;

    ccfsm_read_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_read_data.user_context = NULL;
    ccfsm_read_data.handle = ccfsm_open_data.handle;
    ccfsm_read_data.buffer = ccimp_read_data.buffer;
    ccfsm_read_data.bytes_available = ccimp_read_data.bytes_available;
    ccfsm_read_data.bytes_used = ccimp_read_data.bytes_used;

    Mock_ccimp_fs_file_read_expectAndReturn(&ccimp_read_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_read;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_read_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(1, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_read_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_READ_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testFileWrite)
{
    connector_request_id_t request;
    ccimp_fs_file_write_t ccimp_write_data;
    connector_file_system_write_t ccfsm_write_data;
    connector_callback_status_t status;
    uint8_t buffer[] = "testFileWrite";
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND);

    ccimp_write_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_write_data.imp_context = &my_fs_context;
    ccimp_write_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_write_data.buffer = buffer;
    ccimp_write_data.bytes_available = sizeof buffer;
    ccimp_write_data.bytes_used = 0;

    ccfsm_write_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_write_data.user_context = NULL;
    ccfsm_write_data.handle = ccfsm_open_data.handle;
    ccfsm_write_data.buffer = ccimp_write_data.buffer;
    ccfsm_write_data.bytes_available = ccimp_write_data.bytes_available;
    ccfsm_write_data.bytes_used = 0;

    Mock_ccimp_fs_file_write_expectAndReturn(&ccimp_write_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_write;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_write_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(2, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(ccfsm_write_data.bytes_used, ccfsm_write_data.bytes_available);
}

TEST(test_ccfsm_filesystem, testFileWriteFails)
{
    connector_request_id_t request;
    ccimp_fs_file_write_t ccimp_write_data;
    connector_file_system_write_t ccfsm_write_data;
    connector_callback_status_t status;
    uint8_t buffer[] = "testFileWrite";
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND);

    ccimp_write_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_write_data.imp_context = &my_fs_context;
    ccimp_write_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_write_data.buffer = buffer;
    ccimp_write_data.bytes_available = sizeof buffer;
    ccimp_write_data.bytes_used = 0;

    ccfsm_write_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_write_data.user_context = NULL;
    ccfsm_write_data.handle = ccfsm_open_data.handle;
    ccfsm_write_data.buffer = ccimp_write_data.buffer;
    ccfsm_write_data.bytes_available = ccimp_write_data.bytes_available;
    ccfsm_write_data.bytes_used = 0;

    Mock_ccimp_fs_file_write_expectAndReturn(&ccimp_write_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_write;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_write_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(2, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(ccfsm_write_data.bytes_used, ccfsm_write_data.bytes_available);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_write_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_WRITE_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testFileSeek)
{
    connector_request_id_t request;
    ccimp_fs_file_seek_t ccimp_seek_data;
    connector_file_system_lseek_t ccfsm_seek_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND);

    ccimp_seek_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_seek_data.imp_context = &my_fs_context;
    ccimp_seek_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_seek_data.origin = CCIMP_SEEK_END;
    ccimp_seek_data.requested_offset = -128;
    ccimp_seek_data.resulting_offset = 0;

    ccfsm_seek_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_seek_data.user_context = NULL;
    ccfsm_seek_data.handle = ccfsm_open_data.handle;
    ccfsm_seek_data.origin = connector_file_system_seek_end;
    ccfsm_seek_data.requested_offset = ccimp_seek_data.requested_offset;
    ccfsm_seek_data.resulting_offset = 0;

    Mock_ccimp_fs_file_seek_expectAndReturn(&ccimp_seek_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_lseek;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_seek_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(3, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(ccfsm_seek_data.requested_offset, ccfsm_seek_data.resulting_offset);
}

TEST(test_ccfsm_filesystem, testFileSeekFails)
{
    connector_request_id_t request;
    ccimp_fs_file_seek_t ccimp_seek_data;
    connector_file_system_lseek_t ccfsm_seek_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND);

    ccimp_seek_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_seek_data.imp_context = &my_fs_context;
    ccimp_seek_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_seek_data.origin = CCIMP_SEEK_END;
    ccimp_seek_data.requested_offset = -128;
    ccimp_seek_data.resulting_offset = 0;

    ccfsm_seek_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_seek_data.user_context = NULL;
    ccfsm_seek_data.handle = ccfsm_open_data.handle;
    ccfsm_seek_data.origin = connector_file_system_seek_end;
    ccfsm_seek_data.requested_offset = ccimp_seek_data.requested_offset;
    ccfsm_seek_data.resulting_offset = 0;

    Mock_ccimp_fs_file_seek_expectAndReturn(&ccimp_seek_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_lseek;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_seek_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(3, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(ccfsm_seek_data.requested_offset, ccfsm_seek_data.resulting_offset);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_seek_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_SEEK_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testFileClose)
{
    connector_request_id_t request;
    ccimp_fs_file_close_t ccimp_close_data;
    connector_file_system_close_t ccfsm_close_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_WRONLY | CCIMP_FILE_O_CREAT);

    ccimp_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_close_data.imp_context = &my_fs_context;
    ccimp_close_data.handle = ccapi_fs_handle->ccimp_handle;

    ccfsm_close_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_close_data.user_context = NULL;
    ccfsm_close_data.handle = ccfsm_open_data.handle;

    Mock_ccimp_fs_file_close_expectAndReturn(&ccimp_close_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_close;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_close_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(4, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
}

TEST(test_ccfsm_filesystem, testFileCloseFails)
{
    connector_request_id_t request;
    ccimp_fs_file_close_t ccimp_close_data;
    connector_file_system_close_t ccfsm_close_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_WRONLY | CCIMP_FILE_O_CREAT);

    ccimp_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_close_data.imp_context = &my_fs_context;
    ccimp_close_data.handle = ccapi_fs_handle->ccimp_handle;

    ccfsm_close_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_close_data.user_context = NULL;
    ccfsm_close_data.handle = ccfsm_open_data.handle;

    Mock_ccimp_fs_file_close_expectAndReturn(&ccimp_close_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_close;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_close_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(4, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_close_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_CLOSE_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testFileTruncate)
{
    connector_request_id_t request;
    ccimp_fs_file_truncate_t ccimp_truncate_data;
    connector_file_system_truncate_t ccfsm_truncate_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_WRONLY | CCIMP_FILE_O_CREAT);

    ccimp_truncate_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_truncate_data.imp_context = &my_fs_context;
    ccimp_truncate_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_truncate_data.length_in_bytes = 1024;

    ccfsm_truncate_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_truncate_data.user_context = NULL;
    ccfsm_truncate_data.handle = ccfsm_open_data.handle;
    ccfsm_truncate_data.length_in_bytes = ccimp_truncate_data.length_in_bytes;

    Mock_ccimp_fs_file_truncate_expectAndReturn(&ccimp_truncate_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_ftruncate;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_truncate_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(12, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
}

TEST(test_ccfsm_filesystem, testFileTruncateFails)
{
    connector_request_id_t request;
    ccimp_fs_file_truncate_t ccimp_truncate_data;
    connector_file_system_truncate_t ccfsm_truncate_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_WRONLY | CCIMP_FILE_O_CREAT);

    ccimp_truncate_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_truncate_data.imp_context = &my_fs_context;
    ccimp_truncate_data.handle = ccapi_fs_handle->ccimp_handle;
    ccimp_truncate_data.length_in_bytes = 1024;

    ccfsm_truncate_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_truncate_data.user_context = NULL;
    ccfsm_truncate_data.handle = ccfsm_open_data.handle;
    ccfsm_truncate_data.length_in_bytes = ccimp_truncate_data.length_in_bytes;

    Mock_ccimp_fs_file_truncate_expectAndReturn(&ccimp_truncate_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_ftruncate;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_truncate_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(12, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_truncate_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_TRUNCATE_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testFileRemove)
{
    connector_request_id_t request;
    ccimp_fs_file_remove_t ccimp_remove_data;
    connector_file_system_remove_t ccfsm_remove_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_remove_data.imp_context = &my_fs_context;
    ccimp_remove_data.path = "/tmp/hello.txt";

    ccfsm_remove_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_remove_data.user_context = NULL;
    ccfsm_remove_data.path = ccimp_remove_data.path;

    Mock_ccimp_fs_file_remove_expectAndReturn(&ccimp_remove_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_remove;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(5, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
}

TEST(test_ccfsm_filesystem, testFileRemoveFails)
{
    connector_request_id_t request;
    ccimp_fs_file_remove_t ccimp_remove_data;
    connector_file_system_remove_t ccfsm_remove_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_remove_data.imp_context = &my_fs_context;
    ccimp_remove_data.path = "/tmp/hello.txt";

    ccfsm_remove_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_remove_data.user_context = NULL;
    ccfsm_remove_data.path = ccimp_remove_data.path;

    Mock_ccimp_fs_file_remove_expectAndReturn(&ccimp_remove_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_remove;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_remove_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_REMOVE_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testDirOpen)
{
    connector_request_id_t request;
    ccimp_fs_dir_open_t ccimp_dir_open_data;
    connector_file_system_opendir_t ccfsm_dir_open_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_open_data.imp_context = &my_fs_context;
    ccimp_dir_open_data.handle = CCIMP_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED;
    ccimp_dir_open_data.path = "/tmp/";

    ccfsm_dir_open_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_open_data.user_context = NULL;
    ccfsm_dir_open_data.handle = ccimp_dir_open_data.handle;
    ccfsm_dir_open_data.path = ccimp_dir_open_data.path;

    Mock_ccimp_fs_dir_open_expectAndReturn(&ccimp_dir_open_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_opendir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(6, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK(CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED != ccfsm_dir_open_data.handle);
}

TEST(test_ccfsm_filesystem, testDirOpenFails)
{
    connector_request_id_t request;
    ccimp_fs_dir_open_t ccimp_dir_open_data;
    connector_file_system_opendir_t ccfsm_dir_open_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_open_data.imp_context = &my_fs_context;
    ccimp_dir_open_data.handle = CCIMP_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED;
    ccimp_dir_open_data.path = "/tmp/";

    ccfsm_dir_open_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_open_data.user_context = NULL;
    ccfsm_dir_open_data.handle = ccimp_dir_open_data.handle;
    ccfsm_dir_open_data.path = ccimp_dir_open_data.path;

    Mock_ccimp_fs_dir_open_expectAndReturn(&ccimp_dir_open_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_opendir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(6, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_dir_open_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_DIROPEN_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testDirRead)
{
    connector_request_id_t request;
    ccimp_fs_dir_read_entry_t ccimp_dir_read_entry_data;
    connector_file_system_readdir_t ccfsm_dir_read_entry_data;
    connector_callback_status_t status;
    ccimp_fs_dir_handle_t handle = &handle; /* Not NULL */
    char entry_name[256] = {0};

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_read_entry_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_read_entry_data.imp_context = &my_fs_context;
    ccimp_dir_read_entry_data.handle = handle;
    ccimp_dir_read_entry_data.entry_name = entry_name;
    ccimp_dir_read_entry_data.bytes_available = sizeof entry_name;

    ccfsm_dir_read_entry_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_read_entry_data.user_context = NULL;
    ccfsm_dir_read_entry_data.handle = ccimp_dir_read_entry_data.handle;
    ccfsm_dir_read_entry_data.entry_name = ccimp_dir_read_entry_data.entry_name;
    ccfsm_dir_read_entry_data.bytes_available = ccimp_dir_read_entry_data.bytes_available;

    Mock_ccimp_fs_dir_read_entry_expectAndReturn(&ccimp_dir_read_entry_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_readdir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_read_entry_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(7, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    STRCMP_EQUAL("/tmp/hello.txt", ccfsm_dir_read_entry_data.entry_name);
}

TEST(test_ccfsm_filesystem, testDirReadFails)
{
    connector_request_id_t request;
    ccimp_fs_dir_read_entry_t ccimp_dir_read_entry_data;
    connector_file_system_readdir_t ccfsm_dir_read_entry_data;
    connector_callback_status_t status;
    ccimp_fs_dir_handle_t handle = &handle; /* Not NULL */
    char entry_name[256] = {0};

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_read_entry_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_read_entry_data.imp_context = &my_fs_context;
    ccimp_dir_read_entry_data.handle = handle;
    ccimp_dir_read_entry_data.entry_name = entry_name;
    ccimp_dir_read_entry_data.bytes_available = sizeof entry_name;

    ccfsm_dir_read_entry_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_read_entry_data.user_context = NULL;
    ccfsm_dir_read_entry_data.handle = ccimp_dir_read_entry_data.handle;
    ccfsm_dir_read_entry_data.entry_name = ccimp_dir_read_entry_data.entry_name;
    ccfsm_dir_read_entry_data.bytes_available = ccimp_dir_read_entry_data.bytes_available;

    Mock_ccimp_fs_dir_read_entry_expectAndReturn(&ccimp_dir_read_entry_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_readdir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_read_entry_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(7, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_dir_read_entry_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_DIRREAD_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testDirEntryStat)
{
    connector_request_id_t request;
    ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;
    connector_file_system_stat_dir_entry_t ccfsm_dir_entry_status_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_entry_status_data.imp_context = &my_fs_context;
    ccimp_dir_entry_status_data.path = "/tmp/hello.txt";
    ccimp_dir_entry_status_data.status.file_size = 0;
    ccimp_dir_entry_status_data.status.last_modified = 0;
    ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

    ccfsm_dir_entry_status_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_entry_status_data.user_context = NULL;
    ccfsm_dir_entry_status_data.path = ccimp_dir_entry_status_data.path;
    ccfsm_dir_entry_status_data.statbuf.file_size = ccimp_dir_entry_status_data.status.file_size;
    ccfsm_dir_entry_status_data.statbuf.last_modified = ccimp_dir_entry_status_data.status.last_modified;
    ccfsm_dir_entry_status_data.statbuf.flags = connector_file_system_file_type_none;

    Mock_ccimp_fs_dir_entry_status_expectAndReturn(&ccimp_dir_entry_status_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_stat_dir_entry;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_entry_status_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(8, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(1024, ccfsm_dir_entry_status_data.statbuf.file_size);
    CHECK_EQUAL(1397488930, ccfsm_dir_entry_status_data.statbuf.last_modified);
    CHECK_EQUAL(connector_file_system_file_type_is_reg, ccfsm_dir_entry_status_data.statbuf.flags);
}

TEST(test_ccfsm_filesystem, testDirEntryStatFails)
{
    connector_request_id_t request;
    ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;
    connector_file_system_stat_dir_entry_t ccfsm_dir_entry_status_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_entry_status_data.imp_context = &my_fs_context;
    ccimp_dir_entry_status_data.path = "/tmp/hello.txt";
    ccimp_dir_entry_status_data.status.file_size = 0;
    ccimp_dir_entry_status_data.status.last_modified = 0;
    ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

    ccfsm_dir_entry_status_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_entry_status_data.user_context = NULL;
    ccfsm_dir_entry_status_data.path = ccimp_dir_entry_status_data.path;
    ccfsm_dir_entry_status_data.statbuf.file_size = ccimp_dir_entry_status_data.status.file_size;
    ccfsm_dir_entry_status_data.statbuf.last_modified = ccimp_dir_entry_status_data.status.last_modified;
    ccfsm_dir_entry_status_data.statbuf.flags = connector_file_system_file_type_none;

    Mock_ccimp_fs_dir_entry_status_expectAndReturn(&ccimp_dir_entry_status_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_stat_dir_entry;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_entry_status_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(8, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(1024, ccfsm_dir_entry_status_data.statbuf.file_size);
    CHECK_EQUAL(1397488930, ccfsm_dir_entry_status_data.statbuf.last_modified);
    CHECK_EQUAL(connector_file_system_file_type_is_reg, ccfsm_dir_entry_status_data.statbuf.flags);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_dir_entry_status_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_DIRSTAT_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testDirClose)
{
    connector_request_id_t request;
    ccimp_fs_dir_close_t ccimp_dir_close_data;
    connector_file_system_close_t ccfsm_dir_close_data;
    connector_callback_status_t status;
    ccimp_fs_dir_handle_t handle = &handle; /* Not NULL */

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_close_data.imp_context = &my_fs_context;
    ccimp_dir_close_data.handle = handle;

    ccfsm_dir_close_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_close_data.user_context = NULL;
    ccfsm_dir_close_data.handle = ccimp_dir_close_data.handle;

    Mock_ccimp_fs_dir_close_expectAndReturn(&ccimp_dir_close_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_closedir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_close_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(9, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
}

TEST(test_ccfsm_filesystem, testDirCloseFails)
{
    connector_request_id_t request;
    ccimp_fs_dir_close_t ccimp_dir_close_data;
    connector_file_system_close_t ccfsm_dir_close_data;
    connector_callback_status_t status;
    ccimp_fs_dir_handle_t handle = &handle; /* Not NULL */

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_close_data.imp_context = &my_fs_context;
    ccimp_dir_close_data.handle = handle;

    ccfsm_dir_close_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_close_data.user_context = NULL;
    ccfsm_dir_close_data.handle = ccimp_dir_close_data.handle;

    Mock_ccimp_fs_dir_close_expectAndReturn(&ccimp_dir_close_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_closedir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_close_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(9, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_dir_close_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_DIRCLOSE_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testHashStatus)
{
    connector_request_id_t request;
    ccimp_fs_get_hash_alg_t ccimp_fs_hash_status_data;
    ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;
    connector_file_system_stat_t ccfsm_file_stat_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_fs_hash_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_hash_status_data.imp_context = &my_fs_context;
    ccimp_fs_hash_status_data.path = "/tmp/hello.txt";
    ccimp_fs_hash_status_data.hash_alg.actual = CCIMP_FS_HASH_NONE;
    ccimp_fs_hash_status_data.hash_alg.requested = CCIMP_FS_HASH_CRC32;

    ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_entry_status_data.imp_context = &my_fs_context;
    ccimp_dir_entry_status_data.path = "/tmp/hello.txt";
    ccimp_dir_entry_status_data.status.file_size = 0;
    ccimp_dir_entry_status_data.status.last_modified = 0;
    ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

    ccfsm_file_stat_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_file_stat_data.user_context = NULL;
    ccfsm_file_stat_data.path = ccimp_fs_hash_status_data.path;
    ccfsm_file_stat_data.hash_algorithm.actual = connector_file_system_hash_none;
    ccfsm_file_stat_data.hash_algorithm.requested = connector_file_system_hash_crc32;
    ccfsm_file_stat_data.statbuf.file_size = 0;
    ccfsm_file_stat_data.statbuf.last_modified = 0;
    ccfsm_file_stat_data.statbuf.flags = connector_file_system_file_type_none;

    Mock_ccimp_fs_dir_entry_status_expectAndReturn(&ccimp_dir_entry_status_data, CCIMP_STATUS_OK);
    Mock_ccimp_fs_hash_alg_expectAndReturn(&ccimp_fs_hash_status_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_stat;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_file_stat_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(10, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(connector_file_system_file_type_is_reg, ccfsm_file_stat_data.statbuf.flags);
    CHECK_EQUAL(1024, ccfsm_file_stat_data.statbuf.file_size);
    CHECK_EQUAL(1397488930, ccfsm_file_stat_data.statbuf.last_modified);
    CHECK_EQUAL(ccfsm_file_stat_data.hash_algorithm.requested, ccfsm_file_stat_data.hash_algorithm.actual);
}

TEST(test_ccfsm_filesystem, testStatInexistentFile)
{
    connector_request_id_t request;
    ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;
    connector_file_system_stat_t ccfsm_file_stat_data;
    connector_callback_status_t status;

    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_entry_status_data.imp_context = &my_fs_context;
    ccimp_dir_entry_status_data.path = "/tmp/hello.txt";
    ccimp_dir_entry_status_data.status.file_size = 0;
    ccimp_dir_entry_status_data.status.last_modified = 0;
    ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

    ccfsm_file_stat_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_file_stat_data.user_context = NULL;
    ccfsm_file_stat_data.path = ccimp_dir_entry_status_data.path;
    ccfsm_file_stat_data.hash_algorithm.actual = connector_file_system_hash_none;
    ccfsm_file_stat_data.hash_algorithm.requested = connector_file_system_hash_crc32;
    ccfsm_file_stat_data.statbuf.file_size = 0;
    ccfsm_file_stat_data.statbuf.last_modified = 0;
    ccfsm_file_stat_data.statbuf.flags = connector_file_system_file_type_none;

    Mock_ccimp_fs_dir_entry_status_expectAndReturn(&ccimp_dir_entry_status_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_stat;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_file_stat_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(8, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(connector_file_system_file_type_none, ccfsm_file_stat_data.statbuf.flags);
    CHECK_EQUAL(0, ccfsm_file_stat_data.statbuf.file_size);
    CHECK_EQUAL(0, ccfsm_file_stat_data.statbuf.last_modified);
    CHECK_EQUAL(connector_file_system_hash_none, ccfsm_file_stat_data.hash_algorithm.actual);
}

TEST(test_ccfsm_filesystem, testHashStatusFails)
{
    connector_request_id_t request;
    ccimp_fs_get_hash_alg_t ccimp_fs_hash_status_data;
    connector_file_system_stat_t ccfsm_file_stat_data;
    ccimp_fs_dir_entry_status_t ccimp_dir_entry_status_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_fs_hash_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_hash_status_data.imp_context = &my_fs_context;
    ccimp_fs_hash_status_data.path = "/tmp/hello.txt";
    ccimp_fs_hash_status_data.hash_alg.actual = CCIMP_FS_HASH_NONE;
    ccimp_fs_hash_status_data.hash_alg.requested = CCIMP_FS_HASH_CRC32;

    ccimp_dir_entry_status_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_entry_status_data.imp_context = &my_fs_context;
    ccimp_dir_entry_status_data.path = "/tmp/hello.txt";
    ccimp_dir_entry_status_data.status.file_size = 0;
    ccimp_dir_entry_status_data.status.last_modified = 0;
    ccimp_dir_entry_status_data.status.type = CCIMP_FS_DIR_ENTRY_UNKNOWN;

    ccfsm_file_stat_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_file_stat_data.user_context = NULL;
    ccfsm_file_stat_data.path = ccimp_fs_hash_status_data.path;
    ccfsm_file_stat_data.hash_algorithm.actual = connector_file_system_hash_none;
    ccfsm_file_stat_data.hash_algorithm.requested = connector_file_system_hash_crc32;
    ccfsm_file_stat_data.statbuf.file_size = 0;
    ccfsm_file_stat_data.statbuf.last_modified = 0;
    ccfsm_file_stat_data.statbuf.flags = connector_file_system_file_type_none;

    Mock_ccimp_fs_dir_entry_status_expectAndReturn(&ccimp_dir_entry_status_data, CCIMP_STATUS_OK);
    Mock_ccimp_fs_hash_alg_expectAndReturn(&ccimp_fs_hash_status_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_stat;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_file_stat_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(10, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(connector_file_system_file_type_is_reg, ccfsm_file_stat_data.statbuf.flags);
    CHECK_EQUAL(1024, ccfsm_file_stat_data.statbuf.file_size);
    CHECK_EQUAL(1397488930, ccfsm_file_stat_data.statbuf.last_modified);
    CHECK_EQUAL(ccfsm_file_stat_data.hash_algorithm.requested, ccfsm_file_stat_data.hash_algorithm.actual);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_file_stat_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_HASHSTAT_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testHashFile)
{
    connector_request_id_t request;
    ccimp_fs_hash_file_t ccimp_fs_hash_file_data;
    connector_file_system_hash_t ccfsm_hash_file_data;
    connector_callback_status_t status;
    uint32_t hash_value = 0;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_fs_hash_file_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_hash_file_data.imp_context = &my_fs_context;
    ccimp_fs_hash_file_data.path = "/tmp/hello.txt";
    ccimp_fs_hash_file_data.hash_algorithm = CCIMP_FS_HASH_CRC32;
    ccimp_fs_hash_file_data.hash_value = &hash_value;
    ccimp_fs_hash_file_data.bytes_requested = sizeof hash_value;

    ccfsm_hash_file_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_hash_file_data.user_context = NULL;
    ccfsm_hash_file_data.path = ccimp_fs_hash_file_data.path;
    ccfsm_hash_file_data.hash_algorithm = connector_file_system_hash_crc32;
    ccfsm_hash_file_data.hash_value = ccimp_fs_hash_file_data.hash_value;
    ccfsm_hash_file_data.bytes_requested = ccimp_fs_hash_file_data.bytes_requested;

    Mock_ccimp_fs_hash_file_expectAndReturn(&ccimp_fs_hash_file_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_hash;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_hash_file_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(11, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(0x34EC, hash_value);
}

TEST(test_ccfsm_filesystem, testHashFileFails)
{
    connector_request_id_t request;
    ccimp_fs_hash_file_t ccimp_fs_hash_file_data;
    connector_file_system_hash_t ccfsm_hash_file_data;
    connector_callback_status_t status;
    uint32_t hash_value = 0;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_fs_hash_file_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_fs_hash_file_data.imp_context = &my_fs_context;
    ccimp_fs_hash_file_data.path = "/tmp/hello.txt";
    ccimp_fs_hash_file_data.hash_algorithm = CCIMP_FS_HASH_CRC32;
    ccimp_fs_hash_file_data.hash_value = &hash_value;
    ccimp_fs_hash_file_data.bytes_requested = sizeof hash_value;

    ccfsm_hash_file_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_hash_file_data.user_context = NULL;
    ccfsm_hash_file_data.path = ccimp_fs_hash_file_data.path;
    ccfsm_hash_file_data.hash_algorithm = connector_file_system_hash_crc32;
    ccfsm_hash_file_data.hash_value = ccimp_fs_hash_file_data.hash_value;
    ccfsm_hash_file_data.bytes_requested = ccimp_fs_hash_file_data.bytes_requested;

    Mock_ccimp_fs_hash_file_expectAndReturn(&ccimp_fs_hash_file_data, CCIMP_STATUS_ERROR);

    request.file_system_request = connector_request_id_file_system_hash;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_hash_file_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(11, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(0x34EC, hash_value);
    {
        ccapi_fs_error_handle_t * const error_handle = (ccapi_fs_error_handle_t *)ccfsm_hash_file_data.errnum;
        CHECK(error_handle != NULL);
        CHECK_EQUAL(expected_errnum[FS_HASHFILE_ERRNUM_INDEX], error_handle->error.ccimp_error);
    }
}

TEST(test_ccfsm_filesystem, testErrorDesc)
{
    connector_request_id_t request;
    ccimp_fs_error_desc_t ccimp_error_desc_data;
    connector_file_system_get_error_t ccfsm_error_desc_data;
    connector_callback_status_t status;
    uint8_t buffer[256] = {0};
    ccapi_fs_error_handle_t * error_handle = (ccapi_fs_error_handle_t *)malloc(sizeof *error_handle);

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_error_desc_data.errnum = ETIMEDOUT;
    ccimp_error_desc_data.imp_context = &my_fs_context;
    ccimp_error_desc_data.error_string = (char *)buffer;
    ccimp_error_desc_data.bytes_available = sizeof buffer;
    ccimp_error_desc_data.bytes_used = 0;
    ccimp_error_desc_data.error_status = CCIMP_FS_ERROR_UNKNOWN;

    error_handle->error_is_internal = CCAPI_FALSE;
    error_handle->error.ccimp_error = ccimp_error_desc_data.errnum;

    ccfsm_error_desc_data.errnum = error_handle;
    ccfsm_error_desc_data.user_context = NULL;
    ccfsm_error_desc_data.buffer = ccimp_error_desc_data.error_string;
    ccfsm_error_desc_data.bytes_available = ccimp_error_desc_data.bytes_available;
    ccfsm_error_desc_data.bytes_used = ccimp_error_desc_data.bytes_used;
    ccfsm_error_desc_data.error_status = connector_file_system_unspec_error;

    Mock_ccimp_fs_error_desc_expectAndReturn(&ccimp_error_desc_data, CCIMP_STATUS_OK);
    Mock_ccimp_os_free_expectAndReturn(error_handle, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_get_error;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_error_desc_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(12, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
    CHECK_EQUAL(strlen(strerror(ccimp_error_desc_data.errnum)) + 1, ccfsm_error_desc_data.bytes_used);
    STRCMP_EQUAL(strerror(ccimp_error_desc_data.errnum), (char*)ccfsm_error_desc_data.buffer);
    CHECK_EQUAL(connector_file_system_invalid_parameter, ccfsm_error_desc_data.error_status);
}

TEST(test_ccfsm_filesystem, testSessionError)
{
    connector_request_id_t request;
    ccimp_fs_session_error_t ccimp_session_error_data;
    connector_file_system_session_error_t ccfsm_session_error_data;
    connector_callback_status_t status;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &my_fs_context;

    ccimp_session_error_data.imp_context = &my_fs_context;
    ccimp_session_error_data.session_error = CCIMP_FS_SESSION_ERROR_TIMEOUT;

    ccfsm_session_error_data.user_context = NULL;
    ccfsm_session_error_data.session_error = connector_session_error_timeout;

    Mock_ccimp_fs_session_error_expectAndReturn(&ccimp_session_error_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_session_error;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_session_error_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(13, *(my_filesystem_context_t *)ccapi_data_single_instance->service.file_system.imp_context);
}
