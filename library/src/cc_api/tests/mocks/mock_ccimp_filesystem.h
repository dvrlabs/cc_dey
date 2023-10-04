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

#ifndef _MOCK_CCIMP_FILESYSTEM_H_
#define _MOCK_CCIMP_FILESYSTEM_H_

enum {
    MOCK_FS_FILE_OPEN_DISABLED,
    MOCK_FS_FILE_OPEN_ENABLED
};

enum {
    MOCK_FS_FILE_READ_DISABLED,
    MOCK_FS_FILE_READ_ENABLED
};

enum {
    MOCK_FS_FILE_CLOSE_DISABLED,
    MOCK_FS_FILE_CLOSE_ENABLED
};

enum {
    MOCK_FS_DIR_ENTRY_STATUS_DISABLED,
    MOCK_FS_DIR_ENTRY_STATUS_ENABLED
};

void Mock_ccimp_fs_file_open_create(void);
void Mock_ccimp_fs_file_open_destroy(void);
void Mock_ccimp_fs_file_open_expectAndReturn(ccimp_fs_file_open_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_file_read_create(void);
void Mock_ccimp_fs_file_read_destroy(void);
void Mock_ccimp_fs_file_read_expectAndReturn(ccimp_fs_file_read_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_file_write_create(void);
void Mock_ccimp_fs_file_write_destroy(void);
void Mock_ccimp_fs_file_write_expectAndReturn(ccimp_fs_file_write_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_file_seek_create(void);
void Mock_ccimp_fs_file_seek_destroy(void);
void Mock_ccimp_fs_file_seek_expectAndReturn(ccimp_fs_file_seek_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_file_close_create(void);
void Mock_ccimp_fs_file_close_destroy(void);
void Mock_ccimp_fs_file_close_expectAndReturn(ccimp_fs_file_close_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_file_truncate_create(void);
void Mock_ccimp_fs_file_truncate_destroy(void);
void Mock_ccimp_fs_file_truncate_expectAndReturn(ccimp_fs_file_truncate_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_file_remove_create(void);
void Mock_ccimp_fs_file_remove_destroy(void);
void Mock_ccimp_fs_file_remove_expectAndReturn(ccimp_fs_file_remove_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_dir_open_create(void);
void Mock_ccimp_fs_dir_open_destroy(void);
void Mock_ccimp_fs_dir_open_expectAndReturn(ccimp_fs_dir_open_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_dir_read_entry_create(void);
void Mock_ccimp_fs_dir_read_entry_destroy(void);
void Mock_ccimp_fs_dir_read_entry_expectAndReturn(ccimp_fs_dir_read_entry_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_dir_entry_status_create(void);
void Mock_ccimp_fs_dir_entry_status_destroy(void);
void Mock_ccimp_fs_dir_entry_status_expectAndReturn(ccimp_fs_dir_entry_status_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_dir_close_create(void);
void Mock_ccimp_fs_dir_close_destroy(void);
void Mock_ccimp_fs_dir_close_expectAndReturn(ccimp_fs_dir_close_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_hash_alg_create(void);
void Mock_ccimp_fs_hash_alg_destroy(void);
void Mock_ccimp_fs_hash_alg_expectAndReturn(ccimp_fs_get_hash_alg_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_hash_file_create(void);
void Mock_ccimp_fs_hash_file_destroy(void);
void Mock_ccimp_fs_hash_file_expectAndReturn(ccimp_fs_hash_file_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_error_desc_create(void);
void Mock_ccimp_fs_error_desc_destroy(void);
void Mock_ccimp_fs_error_desc_expectAndReturn(ccimp_fs_error_desc_t * expect, ccimp_status_t retval);

void Mock_ccimp_fs_session_error_create(void);
void Mock_ccimp_fs_session_error_destroy(void);
void Mock_ccimp_fs_session_error_expectAndReturn(ccimp_fs_session_error_t * expect, ccimp_status_t retval);

extern "C" {
ccimp_status_t ccimp_fs_file_open_real(ccimp_fs_file_open_t * const file_open_data);
ccimp_status_t ccimp_fs_file_read_real(ccimp_fs_file_read_t * const file_read_data);
ccimp_status_t ccimp_fs_file_write_real(ccimp_fs_file_write_t * const file_write_data) ;
ccimp_status_t ccimp_fs_file_close_real(ccimp_fs_file_close_t * const file_close_data);
ccimp_status_t ccimp_fs_file_seek_real(ccimp_fs_file_seek_t * const file_seek_data);
ccimp_status_t ccimp_fs_file_truncate_real(ccimp_fs_file_truncate_t * const file_truncate_data);
ccimp_status_t ccimp_fs_file_remove_real(ccimp_fs_file_remove_t * const file_remove_data);
ccimp_status_t ccimp_fs_dir_open_real(ccimp_fs_dir_open_t * const dir_open_data);
ccimp_status_t ccimp_fs_dir_read_entry_real(ccimp_fs_dir_read_entry_t * const dir_read_data);
ccimp_status_t ccimp_fs_dir_entry_status_real(ccimp_fs_dir_entry_status_t * const dir_entry_status_data);
ccimp_status_t ccimp_fs_dir_close_real(ccimp_fs_dir_close_t * const dir_close_data);
ccimp_status_t ccimp_fs_hash_status_real(ccimp_fs_get_hash_alg_t * const hash_status_data);
ccimp_status_t ccimp_fs_hash_file_real(ccimp_fs_hash_file_t * const file_hash_data);
ccimp_status_t ccimp_fs_error_desc_real(ccimp_fs_error_desc_t * const error_desc_data);
ccimp_status_t ccimp_fs_session_error_real(ccimp_fs_session_error_t * const session_error_data);
}
#endif
