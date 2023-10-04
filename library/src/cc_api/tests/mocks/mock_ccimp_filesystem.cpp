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

#include "mocks.h"

/* * * ccimp_fs_file_open * * */
void Mock_ccimp_fs_file_open_create(void)
{
    mock().installComparator("ccimp_fs_file_open_t", ccimp_fs_file_open_t_comparator);
}

void Mock_ccimp_fs_file_open_destroy(void)
{
    mock("ccimp_fs_file_open").checkExpectations();
}

void Mock_ccimp_fs_file_open_expectAndReturn(ccimp_fs_file_open_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_open").expectOneCall("ccimp_fs_file_open")
            .withParameterOfType("ccimp_fs_file_open_t", "data", data)
            .andReturnValue(retval);

    mock("ccimp_fs_file_open").setData("behavior", MOCK_FS_FILE_OPEN_ENABLED);
}

/* * * ccimp_fs_file_read * * */
void Mock_ccimp_fs_file_read_create(void)
{
    mock().installComparator("ccimp_fs_file_read_t", ccimp_fs_file_read_t_comparator);
}

void Mock_ccimp_fs_file_read_destroy(void)
{
    mock("ccimp_fs_file_read").checkExpectations();
}

void Mock_ccimp_fs_file_read_expectAndReturn(ccimp_fs_file_read_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_read").expectOneCall("ccimp_fs_file_read")
            .withParameterOfType("ccimp_fs_file_read_t", "data", data)
            .andReturnValue(retval);

    mock("ccimp_fs_file_read").setData("behavior", MOCK_FS_FILE_READ_ENABLED);
}

/* * * ccimp_fs_file_write * * */
void Mock_ccimp_fs_file_write_create(void)
{
    mock().installComparator("ccimp_fs_file_write_t", ccimp_fs_file_write_t_comparator);
}

void Mock_ccimp_fs_file_write_destroy(void)
{
    mock("ccimp_fs_file_write").checkExpectations();
}

void Mock_ccimp_fs_file_write_expectAndReturn(ccimp_fs_file_write_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_write").expectOneCall("ccimp_fs_file_write")
            .withParameterOfType("ccimp_fs_file_write_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_file_seek * * */
void Mock_ccimp_fs_file_seek_create(void)
{
    mock().installComparator("ccimp_fs_file_seek_t", ccimp_fs_file_seek_t_comparator);
}

void Mock_ccimp_fs_file_seek_destroy(void)
{
    mock("ccimp_fs_file_seek").checkExpectations();
}

void Mock_ccimp_fs_file_seek_expectAndReturn(ccimp_fs_file_seek_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_seek").expectOneCall("ccimp_fs_file_seek")
            .withParameterOfType("ccimp_fs_file_seek_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_file_close * * */
void Mock_ccimp_fs_file_close_create(void)
{
    mock().installComparator("ccimp_fs_file_close_t", ccimp_fs_file_close_t_comparator);
}

void Mock_ccimp_fs_file_close_destroy(void)
{
    mock("ccimp_fs_file_close").checkExpectations();
}

void Mock_ccimp_fs_file_close_expectAndReturn(ccimp_fs_file_close_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_close").expectOneCall("ccimp_fs_file_close")
            .withParameterOfType("ccimp_fs_file_close_t", "data", data)
            .andReturnValue(retval);

    mock("ccimp_fs_file_close").setData("behavior", MOCK_FS_FILE_CLOSE_ENABLED);
}

/* * * ccimp_fs_file_truncate * * */
void Mock_ccimp_fs_file_truncate_create(void)
{
    mock().installComparator("ccimp_fs_file_truncate_t", ccimp_fs_file_truncate_t_comparator);
}

void Mock_ccimp_fs_file_truncate_destroy(void)
{
    mock("ccimp_fs_file_truncate").checkExpectations();
}

void Mock_ccimp_fs_file_truncate_expectAndReturn(ccimp_fs_file_truncate_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_truncate").expectOneCall("ccimp_fs_file_truncate")
            .withParameterOfType("ccimp_fs_file_truncate_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_file_remove * * */
void Mock_ccimp_fs_file_remove_create(void)
{
    mock().installComparator("ccimp_fs_file_remove_t", ccimp_fs_file_remove_t_comparator);
}

void Mock_ccimp_fs_file_remove_destroy(void)
{
    mock("ccimp_fs_file_remove").checkExpectations();
}

void Mock_ccimp_fs_file_remove_expectAndReturn(ccimp_fs_file_remove_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_file_remove").expectOneCall("ccimp_fs_file_remove")
            .withParameterOfType("ccimp_fs_file_remove_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_dir_open * * */
void Mock_ccimp_fs_dir_open_create(void)
{
    mock().installComparator("ccimp_fs_dir_open_t", ccimp_fs_dir_open_t_comparator);
}

void Mock_ccimp_fs_dir_open_destroy(void)
{
    mock("ccimp_fs_dir_open").checkExpectations();
}

void Mock_ccimp_fs_dir_open_expectAndReturn(ccimp_fs_dir_open_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_dir_open").expectOneCall("ccimp_fs_dir_open")
            .withParameterOfType("ccimp_fs_dir_open_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_dir_read_entry * * */
void Mock_ccimp_fs_dir_read_entry_create(void)
{
    mock().installComparator("ccimp_fs_dir_read_entry_t", ccimp_fs_dir_read_entry_t_comparator);
}

void Mock_ccimp_fs_dir_read_entry_destroy(void)
{
    mock("ccimp_fs_dir_read_entry").checkExpectations();
}

void Mock_ccimp_fs_dir_read_entry_expectAndReturn(ccimp_fs_dir_read_entry_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_dir_read_entry").expectOneCall("ccimp_fs_dir_read_entry")
            .withParameterOfType("ccimp_fs_dir_read_entry_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_dir_entry_status * * */
void Mock_ccimp_fs_dir_entry_status_create(void)
{
    mock().installComparator("ccimp_fs_dir_entry_status_t", ccimp_fs_dir_entry_status_t_comparator);
}

void Mock_ccimp_fs_dir_entry_status_destroy(void)
{
    mock("ccimp_fs_dir_entry_status").checkExpectations();
}

void Mock_ccimp_fs_dir_entry_status_expectAndReturn(ccimp_fs_dir_entry_status_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_dir_entry_status").expectOneCall("ccimp_fs_dir_entry_status")
            .withParameterOfType("ccimp_fs_dir_entry_status_t", "data", data)
            .andReturnValue(retval);

    mock("ccimp_fs_dir_entry_status").setData("behavior", MOCK_FS_DIR_ENTRY_STATUS_ENABLED);
}

/* * * ccimp_fs_dir_close * * */
void Mock_ccimp_fs_dir_close_create(void)
{
    mock().installComparator("ccimp_fs_dir_close_t", ccimp_fs_dir_close_t_comparator);
}

void Mock_ccimp_fs_dir_close_destroy(void)
{
    mock("ccimp_fs_dir_close").checkExpectations();
}

void Mock_ccimp_fs_dir_close_expectAndReturn(ccimp_fs_dir_close_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_dir_close").expectOneCall("ccimp_fs_dir_close")
            .withParameterOfType("ccimp_fs_dir_close_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_hash_status * * */
void Mock_ccimp_fs_hash_alg_create(void)
{
    mock().installComparator("ccimp_fs_get_hash_alg_t", ccimp_fs_get_hash_alg_t_comparator);
}

void Mock_ccimp_fs_hash_alg_destroy(void)
{
    mock("ccimp_fs_hash_alg").checkExpectations();
}

void Mock_ccimp_fs_hash_alg_expectAndReturn(ccimp_fs_get_hash_alg_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_hash_alg").expectOneCall("ccimp_fs_hash_alg")
            .withParameterOfType("ccimp_fs_get_hash_alg_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_hash_file * * */
void Mock_ccimp_fs_hash_file_create(void)
{
    mock().installComparator("ccimp_fs_hash_file_t", ccimp_fs_hash_file_t_comparator);
}

void Mock_ccimp_fs_hash_file_destroy(void)
{
    mock("ccimp_fs_hash_file").checkExpectations();
}

void Mock_ccimp_fs_hash_file_expectAndReturn(ccimp_fs_hash_file_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_hash_file").expectOneCall("ccimp_fs_hash_file")
            .withParameterOfType("ccimp_fs_hash_file_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_error_desc * * */
void Mock_ccimp_fs_error_desc_create(void)
{
    mock().installComparator("ccimp_fs_error_desc_t", ccimp_fs_error_desc_t_comparator);
}

void Mock_ccimp_fs_error_desc_destroy(void)
{
    mock("ccimp_fs_error_desc").checkExpectations();
}

void Mock_ccimp_fs_error_desc_expectAndReturn(ccimp_fs_error_desc_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_error_desc").expectOneCall("ccimp_fs_error_desc")
            .withParameterOfType("ccimp_fs_error_desc_t", "data", data)
            .andReturnValue(retval);
}

/* * * ccimp_fs_session_error * * */
void Mock_ccimp_fs_session_error_create(void)
{
    mock().installComparator("ccimp_fs_session_error_t", ccimp_fs_session_error_t_comparator);
}

void Mock_ccimp_fs_session_error_destroy(void)
{
    mock("ccimp_fs_session_error").checkExpectations();
}

void Mock_ccimp_fs_session_error_expectAndReturn(ccimp_fs_session_error_t * data, ccimp_status_t retval)
{
    mock("ccimp_fs_session_error").expectOneCall("ccimp_fs_session_error")
            .withParameterOfType("ccimp_fs_session_error_t", "data", data)
            .andReturnValue(retval);
}

extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "ccapi_definitions.h"
#include <errno.h>

typedef int my_filesystem_context_t;
typedef int my_filesystem_dir_handle_t;

my_filesystem_context_t my_fs_context = 0;

my_filesystem_dir_handle_t dir_handle;

ccimp_status_t ccimp_fs_file_open(ccimp_fs_file_open_t * const data)
{
    if (mock_scope_c("ccimp_fs_file_open")->getData("behavior").value.intValue == MOCK_FS_FILE_OPEN_ENABLED)
	{
        mock_scope_c("ccimp_fs_file_open")->actualCall("ccimp_fs_file_open")->withParameterOfType("ccimp_fs_file_open_t", "data", (void *)data);
        data->errnum = EAGAIN;
        data->handle = 5;
        data->imp_context = &my_fs_context;

        return (ccimp_status_t)mock_scope_c("ccimp_fs_file_open")->returnValue().value.intValue;
    }
    else
    {
        return ccimp_fs_file_open_real(data);
    }
}

ccimp_status_t ccimp_fs_file_read(ccimp_fs_file_read_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    if (mock_scope_c("ccimp_fs_file_read")->getData("behavior").value.intValue == MOCK_FS_FILE_READ_ENABLED)
	{
        mock_scope_c("ccimp_fs_file_read")->actualCall("ccimp_fs_file_read")->withParameterOfType("ccimp_fs_file_read_t", "data", (void *)data);
        strcpy((char *)data->buffer, "testFileRead");
        data->bytes_used = sizeof "testFileRead";
        data->errnum = ETIMEDOUT;
        *fs_context = 1;
        return (ccimp_status_t)mock_scope_c("ccimp_fs_file_read")->returnValue().value.intValue;
    }
    else
    {
        return ccimp_fs_file_read_real(data);
    }
}

ccimp_status_t ccimp_fs_file_write(ccimp_fs_file_write_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_file_write")->actualCall("ccimp_fs_file_write")->withParameterOfType("ccimp_fs_file_write_t", "data", (void *)data);
    data->errnum = ENODATA;
    *fs_context = 2;
    data->bytes_used = data->bytes_available;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_file_write")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_file_seek(ccimp_fs_file_seek_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_file_seek")->actualCall("ccimp_fs_file_seek")->withParameterOfType("ccimp_fs_file_seek_t", "data", (void *)data);
    *fs_context = 3;
    data->errnum = EINVAL;
    data->resulting_offset = data->requested_offset;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_file_seek")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_file_close(ccimp_fs_file_close_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    if (mock_scope_c("ccimp_fs_file_close")->getData("behavior").value.intValue == MOCK_FS_FILE_CLOSE_ENABLED)
	{
        mock_scope_c("ccimp_fs_file_close")->actualCall("ccimp_fs_file_close")->withParameterOfType("ccimp_fs_file_close_t", "data", (void *)data);
        *fs_context = 4;
        data->errnum = EROFS;
        return (ccimp_status_t)mock_scope_c("ccimp_fs_file_close")->returnValue().value.intValue;
    }
    else
    {
        return ccimp_fs_file_close_real(data);
    }
}

ccimp_status_t ccimp_fs_file_truncate(ccimp_fs_file_truncate_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_file_truncate")->actualCall("ccimp_fs_file_truncate")->withParameterOfType("ccimp_fs_file_truncate_t", "data", (void *)data);
    *fs_context = 12;
    data->errnum = EIO;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_file_truncate")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_file_remove(ccimp_fs_file_remove_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_file_remove")->actualCall("ccimp_fs_file_remove")->withParameterOfType("ccimp_fs_file_remove_t", "data", (void *)data);
    *fs_context = 5;
    data->errnum = EACCES;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_file_remove")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_dir_open(ccimp_fs_dir_open_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;
    mock_scope_c("ccimp_fs_dir_open")->actualCall("ccimp_fs_dir_open")->withParameterOfType("ccimp_fs_dir_open_t", "data", (void *)data);
    data->handle = &dir_handle;
    *fs_context = 6;
    data->errnum = ENOTDIR;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_dir_open")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_dir_read_entry(ccimp_fs_dir_read_entry_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_dir_read_entry")->actualCall("ccimp_fs_dir_read_entry")->withParameterOfType("ccimp_fs_dir_read_entry_t", "data", (void *)data);
    strcpy(data->entry_name, "/tmp/hello.txt");
    *fs_context = 7;
    data->errnum = ENOSYS;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_dir_read_entry")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_dir_entry_status(ccimp_fs_dir_entry_status_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    if (mock_scope_c("ccimp_fs_dir_entry_status")->getData("behavior").value.intValue == MOCK_FS_DIR_ENTRY_STATUS_ENABLED)
	{
        mock_scope_c("ccimp_fs_dir_entry_status")->actualCall("ccimp_fs_dir_entry_status")->withParameterOfType("ccimp_fs_dir_entry_status_t", "data", (void *)data);

        if (strstr(data->path, ".txt") != NULL)
        {
            data->status.file_size = 1024;
            data->status.last_modified = 1397488930;
            data->status.type = CCIMP_FS_DIR_ENTRY_FILE;
        }
        else
        {
            data->status.type = CCIMP_FS_DIR_ENTRY_DIR;
        }

        *fs_context = 8;
        data->errnum = ENOMEM;
        return (ccimp_status_t)mock_scope_c("ccimp_fs_dir_entry_status")->returnValue().value.intValue;
    }
    else
    {
        return ccimp_fs_dir_entry_status_real(data);
    }
}

ccimp_status_t ccimp_fs_dir_close(ccimp_fs_dir_close_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_dir_close")->actualCall("ccimp_fs_dir_close")->withParameterOfType("ccimp_fs_dir_close_t", "data", (void *)data);
    *fs_context = 9;
    data->errnum = EINVAL;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_dir_close")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_hash_alg(ccimp_fs_get_hash_alg_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_hash_alg")->actualCall("ccimp_fs_hash_alg")->withParameterOfType("ccimp_fs_get_hash_alg_t", "data", (void *)data);
    data->hash_alg.actual = data->hash_alg.requested;
    *fs_context = 10;
    data->errnum = ENAMETOOLONG;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_hash_alg")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_hash_file(ccimp_fs_hash_file_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;
    uint32_t * crc32 = (uint32_t *)data->hash_value;

    mock_scope_c("ccimp_fs_hash_file")->actualCall("ccimp_fs_hash_file")->withParameterOfType("ccimp_fs_hash_file_t", "data", (void *)data);
    *crc32 = 0x34EC;
    *fs_context = 11;
    data->errnum = ENOSPC;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_hash_file")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_error_desc(ccimp_fs_error_desc_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_error_desc")->actualCall("ccimp_fs_error_desc")->withParameterOfType("ccimp_fs_error_desc_t", "data", (void *)data);
    data->error_status = CCIMP_FS_ERROR_INVALID_PARAMETER;
    strcpy(data->error_string, strerror(data->errnum));
    data->bytes_used = strlen(data->error_string) + 1;
    *fs_context = 12;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_error_desc")->returnValue().value.intValue;
}

ccimp_status_t ccimp_fs_session_error(ccimp_fs_session_error_t * const data)
{
    my_filesystem_context_t * const fs_context = (my_filesystem_context_t *)data->imp_context;

    mock_scope_c("ccimp_fs_session_error")->actualCall("ccimp_fs_session_error")->withParameterOfType("ccimp_fs_session_error_t", "data", (void *)data);
    *fs_context = 13;
    return (ccimp_status_t)mock_scope_c("ccimp_fs_session_error")->returnValue().value.intValue;
}
}
