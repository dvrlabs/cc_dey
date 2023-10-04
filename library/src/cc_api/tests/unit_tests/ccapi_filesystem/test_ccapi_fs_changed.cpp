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

static char const * ccapi_fs_changed_expected_path = NULL;
static ccapi_fs_changed_t ccapi_fs_changed_expected_request = CCAPI_FS_CHANGED_MODIFIED;
static ccapi_bool_t ccapi_fs_changed_cb_called = CCAPI_FALSE;

static void ccapi_fs_changed_cb(char const * const local_path, ccapi_fs_changed_t const request)
{
    STRCMP_EQUAL(ccapi_fs_changed_expected_path, local_path);
    CHECK_EQUAL(ccapi_fs_changed_expected_request, request);
    ccapi_fs_changed_cb_called = CCAPI_TRUE;
}

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

TEST_GROUP(test_ccapi_fs_changed)
{
    void setup()
    {
        ccapi_start_t start = {0};
        ccapi_start_error_t error;
        ccapi_filesystem_service_t fs_service = {ccapi_fs_access_cb, ccapi_fs_changed_cb};
        Mock_create_all();

        th_fill_start_structure_with_good_parameters(&start);
        start.service.file_system = &fs_service;

        ccapi_fs_changed_expected_path = NULL;
        ccapi_fs_changed_expected_request = CCAPI_FS_CHANGED_MODIFIED;
        ccapi_fs_changed_cb_called = CCAPI_FALSE;

        ccapi_fs_access_expected_path = NULL;
        ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READ;
        ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;
        ccapi_fs_access_cb_called = CCAPI_FALSE;

        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
        CHECK_EQUAL(fs_service.changed, ccapi_data_single_instance->service.file_system.user_callback.changed);
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_fs_changed, testChangedRemoved)
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

    ccapi_fs_changed_expected_path = ccapi_fs_access_expected_path;
    ccapi_fs_changed_expected_request = CCAPI_FS_CHANGED_REMOVED;

    ccimp_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_remove_data.imp_context = &fs_context;
    ccimp_remove_data.path = ccapi_fs_changed_expected_path;

    ccfsm_remove_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_remove_data.user_context = NULL;
    ccfsm_remove_data.path = ccimp_remove_data.path;

    Mock_ccimp_fs_file_remove_expectAndReturn(&ccimp_remove_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_remove;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_changed_cb_called);
}

TEST(test_ccapi_fs_changed, testChangedRemoveNotAllowed)
{
    connector_request_id_t request;
    ccimp_fs_file_remove_t ccimp_remove_data;
    connector_file_system_remove_t ccfsm_remove_data;
    connector_callback_status_t status;
    int fs_context;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_REMOVE;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_DENY;

    ccimp_remove_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_remove_data.imp_context = &fs_context;
    ccimp_remove_data.path = ccapi_fs_access_expected_path;

    ccfsm_remove_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_remove_data.user_context = NULL;
    ccfsm_remove_data.path = ccimp_remove_data.path;

    request.file_system_request = connector_request_id_file_system_remove;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_remove_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_error, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_access_cb_called);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_fs_changed_cb_called);
}

TEST(test_ccapi_fs_changed, testChangedModified)
{
    connector_request_id_t request;
    ccimp_fs_file_close_t ccimp_close_data;
    connector_file_system_close_t ccfsm_close_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = NULL;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READWRITE;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;
    ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDWR | CCIMP_FILE_O_CREAT);

    ccapi_fs_changed_expected_path = "/tmp/hello.txt";
    ccapi_fs_changed_expected_request = CCAPI_FS_CHANGED_MODIFIED;

    ccimp_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_close_data.imp_context = &my_fs_context;
    ccimp_close_data.handle = ccapi_fs_handle->ccimp_handle;

    ccfsm_close_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_close_data.user_context = ccimp_close_data.imp_context;
    ccfsm_close_data.handle = ccfsm_open_data.handle;

    Mock_ccimp_fs_file_close_expectAndReturn(&ccimp_close_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_close;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_close_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_fs_changed_cb_called);
}

TEST(test_ccapi_fs_changed, testOpenForReadNotChanged)
{
    connector_request_id_t request;
    ccimp_fs_file_close_t ccimp_close_data;
    connector_file_system_close_t ccfsm_close_data;
    connector_callback_status_t status;
    connector_file_system_open_t ccfsm_open_data;
    ccapi_fs_file_handle_t * ccapi_fs_handle = NULL;

    ccapi_fs_access_expected_path = "/tmp/hello.txt";
    ccapi_fs_access_expected_request = CCAPI_FS_REQUEST_READ;
    ccapi_fs_access_retval = CCAPI_FS_ACCESS_ALLOW;

    ccapi_fs_handle = th_filesystem_openfile("/tmp/hello.txt", &ccfsm_open_data, CCIMP_FILE_O_RDONLY);

    ccimp_close_data.errnum = CCIMP_FILESYSTEM_ERRNUM_NONE;
    ccimp_close_data.imp_context = &my_fs_context;
    ccimp_close_data.handle = ccapi_fs_handle->ccimp_handle;

    ccfsm_close_data.errnum = CONNECTOR_FILESYSTEM_ERRNUM_NONE;
    ccfsm_close_data.user_context = ccimp_close_data.imp_context;
    ccfsm_close_data.handle = ccfsm_open_data.handle;

    Mock_ccimp_fs_file_close_expectAndReturn(&ccimp_close_data, CCIMP_STATUS_OK);

    request.file_system_request = connector_request_id_file_system_close;
    status = ccapi_connector_callback(connector_class_id_file_system, request, &ccfsm_close_data, ccapi_data_single_instance);

    CHECK_EQUAL(connector_callback_continue, status);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_fs_changed_cb_called);
}
