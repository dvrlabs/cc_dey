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

static char const * ccapi_fs_access_expected_path = NULL;
static ccapi_fs_request_t ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_UNKNOWN;
static ccapi_fs_access_t ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;
static ccapi_bool_t ccapi_fs_access_cb_called = CCAPI_FALSE;

static ccapi_fs_access_t ccapi_fs_access_cb(char const * const local_path, ccapi_fs_request_t const request)
{
    STRCMP_EQUAL(ccapi_fs_access_expected_path, local_path);
    CHECK_EQUAL(ccapi_fs_access_expected_request, request);
    ccapi_fs_access_cb_called = CCAPI_TRUE;
    return ccapi_fs_access_retval;
}

TEST_GROUP(test_ccapi_fs_access)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_filesystem_service_t fs_service = {ccapi_fs_access_cb, NULL};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.file_system = &fs_service;

        ccapi_fs_access_expected_path = NULL;
        ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READ;
        ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;
        ccapi_fs_access_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(fs_service.access, ccapi_data_single_instance->service.file_system.user_callback.access);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_fs_access, testAccessRead)
{
    connector_request_id_t request;
    ccimp_fs_file_open_t ccimp_open_data;
    connector_file_system_open_t ccfsm_open_data;
    connector_callback_status_t status;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READ;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;

    ccimp_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_open_data.imp_context = NULL;
    ccimp_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccimp_open_data.flags = CCIMP_FILE_O_RDONLY | CCIMP_FILE_O_APPEND;
    ccimp_open_data.path = ccapi_fs_access_expected_path;

    ccfsm_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.handle = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.oflag = CONNECTOR_FILE_O_RDONLY | CONNECTOR_FILE_O_APPEND;
    ccfsm_open_data.path = ccimp_open_data.path;
    ccfsm_open_data.user_context = NULL;

    Mock_ccimp_fs_file_open_expectAndReturn(&ccimp_open_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_open;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(ccfsm_open_data.handle != CCIMP_FILESYSTEM_ERRNUM_NONE);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
}

TEST(test_ccapi_fs_access, testAccessWrite)
{
    connector_request_id_t request;
    ccimp_fs_file_open_t ccimp_open_data;
    connector_file_system_open_t ccfsm_open_data;
    connector_callback_status_t status;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_WRITE;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;

    ccimp_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_open_data.imp_context = NULL;
    ccimp_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccimp_open_data.flags = CCIMP_FILE_O_WRONLY | CCIMP_FILE_O_APPEND;
    ccimp_open_data.path = ccapi_fs_access_expected_path;

    ccfsm_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.handle = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.oflag = CONNECTOR_FILE_O_WRONLY | CONNECTOR_FILE_O_APPEND;
    ccfsm_open_data.path = ccimp_open_data.path;
    ccfsm_open_data.user_context = NULL;

    Mock_ccimp_fs_file_open_expectAndReturn(&ccimp_open_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_open;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(ccfsm_open_data.handle != CCIMP_FILESYSTEM_ERRNUM_NONE);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);

    /* Check access denied, no call to mocks */
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK(ccfsm_open_data.handle == CCIMP_FILESYSTEM_ERRNUM_NONE);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
}

TEST(test_ccapi_fs_access, testAccessReadWrite)
{
    connector_request_id_t request;
    ccimp_fs_file_open_t ccimp_open_data;
    connector_file_system_open_t ccfsm_open_data;
    connector_callback_status_t status;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READWRITE;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;

    ccimp_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_open_data.imp_context = NULL;
    ccimp_open_data.handle = CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED;
    ccimp_open_data.flags = CCIMP_FILE_O_RDWR | CCIMP_FILE_O_APPEND;
    ccimp_open_data.path = ccapi_fs_access_expected_path;

    ccfsm_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.handle = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.oflag = CONNECTOR_FILE_O_RDWR | CONNECTOR_FILE_O_APPEND;
    ccfsm_open_data.path = ccimp_open_data.path;
    ccfsm_open_data.user_context = NULL;

    Mock_ccimp_fs_file_open_expectAndReturn(&ccimp_open_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_open;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(ccfsm_open_data.handle != CCIMP_FILESYSTEM_ERRNUM_NONE);

    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);

    /* Check access denied, no call to mocks */
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK(ccfsm_open_data.handle == CCIMP_FILESYSTEM_ERRNUM_NONE);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
}

TEST(test_ccapi_fs_access, testAccessRemove)
{
    connector_request_id_t request;
    ccimp_fs_file_remove_t ccimp_remove_data;
    connector_file_system_remove_t ccfsm_remove_data;
    connector_callback_status_t status;
    int fs_context;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &fs_context;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_REMOVE;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;

    ccimp_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_remove_data.imp_context = &fs_context;
    ccimp_remove_data.path = ccapi_fs_access_expected_path;

    ccfsm_remove_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_remove_data.user_context = NULL;
    ccfsm_remove_data.path = ccimp_remove_data.path;

    Mock_ccimp_fs_file_remove_expectAndReturn(&ccimp_remove_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_remove;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
}

TEST(test_ccapi_fs_access, testAccessList)
{
    connector_request_id_t request;
    ccimp_fs_dir_open_t ccimp_dir_open_data;
    connector_file_system_opendir_t ccfsm_dir_open_data;
    connector_callback_status_t status;
    int fs_context;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &fs_context;

    ccapi_fs_access_expected_path = "/tmp/";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_LIST;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;

    ccimp_dir_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_dir_open_data.imp_context = ccapi_data_single_instance->service.file_system.imp_context;
    ccimp_dir_open_data.handle = CCIMP_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED;
    ccimp_dir_open_data.path = ccapi_fs_access_expected_path;

    ccfsm_dir_open_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_open_data.user_context = NULL;
    ccfsm_dir_open_data.handle = ccimp_dir_open_data.handle;
    ccfsm_dir_open_data.path = ccimp_dir_open_data.path;

    Mock_ccimp_fs_dir_open_expectAndReturn(&ccimp_dir_open_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_opendir;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_open_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK(CCIMP_FILESYSTEM_ERRNUM_NONE != ccfsm_dir_open_data.handle);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
}

TEST(test_ccapi_fs_access, testAccessOpenDenied)
{
    connector_request_id_t request;
    connector_file_system_open_t ccfsm_open_data;
    connector_callback_status_t status;
    ccapi_fs_error_handle_t * error_handle;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READ;

    ccfsm_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.handle = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_open_data.oflag = CONNECTOR_FILE_O_RDONLY | CONNECTOR_FILE_O_APPEND;
    ccfsm_open_data.path = ccapi_fs_access_expected_path;
    ccfsm_open_data.user_context = NULL;

    /* Check access denied, no call to mocks */
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;

    request.file_system_request = connector_request_id_file_system_open;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_open_data, ccapi_data_single_instance);

    error_handle = (ccapi_fs_error_handle_t *)ccfsm_open_data.errnum;

    CHECK_EQUAL(connector_callback_error, status);
    CHECK(ccfsm_open_data.handle == CCIMP_FILESYSTEM_ERRNUM_NONE);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
    CHECK_EQUAL(CCAPI_TRUE, error_handle->error_is_internal);
    CHECK_EQUAL(CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED, error_handle->error.ccapi_error);

    th_call_ccimp_fs_error_desc_and_check_error(ccfsm_open_data.errnum, connector_file_system_permission_denied);
}

TEST(test_ccapi_fs_access, testAccessListDenied)
{
    connector_request_id_t request;
    connector_file_system_opendir_t ccfsm_dir_open_data;
    connector_callback_status_t status;
    int fs_context;
    ccapi_fs_error_handle_t * error_handle;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &fs_context;

    ccapi_fs_access_expected_path = "/tmp/";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_LIST;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;

    ccfsm_dir_open_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_open_data.user_context = NULL;
    ccfsm_dir_open_data.handle = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_dir_open_data.path = ccapi_fs_access_expected_path;

    request.file_system_request = connector_request_id_file_system_opendir;

    /* Check access denied, no call to mocks */
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_dir_open_data, ccapi_data_single_instance);
    error_handle = (ccapi_fs_error_handle_t *)ccfsm_dir_open_data.errnum;

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
    CHECK_EQUAL(CCAPI_TRUE, error_handle->error_is_internal);
    CHECK_EQUAL(CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED, error_handle->error.ccapi_error);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK(ccfsm_dir_open_data.handle == CCIMP_FILESYSTEM_ERRNUM_NONE);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);

    th_call_ccimp_fs_error_desc_and_check_error(ccfsm_dir_open_data.errnum, connector_file_system_permission_denied);
}

TEST(test_ccapi_fs_access, testAccessRemoveDenied)
{
    connector_request_id_t request;
    connector_file_system_remove_t ccfsm_remove_data;
    connector_callback_status_t status;
    int fs_context;

    /* Simulate that imp_context was previously set by other call (file_open) */
    ccapi_data_single_instance->service.file_system.imp_context = &fs_context;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_REMOVE;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;

    ccfsm_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccfsm_remove_data.user_context = NULL;
    ccfsm_remove_data.path = ccapi_fs_access_expected_path;

    request.file_system_request = connector_request_id_file_system_remove;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);

    /* Check access denied, no call to mocks */
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
    th_call_ccimp_fs_error_desc_and_check_error(ccfsm_remove_data.errnum, connector_file_system_permission_denied);
}
