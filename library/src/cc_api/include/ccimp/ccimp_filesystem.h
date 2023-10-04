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

#ifndef _CCIMP_FILESYSTEM_H_
#define _CCIMP_FILESYSTEM_H_

#include "ccimp/ccimp_types.h"

#if (defined CCAPI_CONST_PROTECTION_UNLOCK)
#define CONST
#else
#define CONST   const
#endif

#define CCIMP_FILE_O_RDONLY     0
#define CCIMP_FILE_O_WRONLY     1
#define CCIMP_FILE_O_RDWR       2
#define CCIMP_FILE_O_APPEND     0x0008
#define CCIMP_FILE_O_CREAT      0x0200
#define CCIMP_FILE_O_TRUNC      0x0400

#if (defined CCIMP_FILE_SYSTEM_HAS_LARGE_FILES)
typedef int64_t ccimp_file_offset_t;
#else
typedef int32_t ccimp_file_offset_t;
#endif

typedef enum
{
    CCIMP_SEEK_SET,
    CCIMP_SEEK_CUR,
    CCIMP_SEEK_END
} ccimp_fs_seek_origin_t;

typedef enum {
    CCIMP_FS_DIR_ENTRY_UNKNOWN,
    CCIMP_FS_DIR_ENTRY_FILE,
    CCIMP_FS_DIR_ENTRY_DIR
} ccimp_fs_dir_entry_type_t;

typedef enum {
    CCIMP_FS_HASH_NONE,
    CCIMP_FS_HASH_SHA3_512,
    CCIMP_FS_HASH_SHA512,
    CCIMP_FS_HASH_MD5,
    CCIMP_FS_HASH_CRC32,
    CCIMP_FS_HASH_BEST
} ccimp_fs_hash_alg_t;

typedef struct {
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    char const * CONST path;
    int CONST flags;
    ccimp_fs_file_handle_t handle;
} ccimp_fs_file_open_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_file_handle_t CONST handle;
    void * CONST buffer;
    size_t CONST bytes_available;
    size_t bytes_used;
} ccimp_fs_file_read_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_file_handle_t CONST handle;
    void const * CONST buffer;
    size_t CONST bytes_available;
    size_t bytes_used;
} ccimp_fs_file_write_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_file_handle_t CONST handle;
} ccimp_fs_file_close_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_file_handle_t CONST handle;
    ccimp_file_offset_t CONST requested_offset;
    ccimp_file_offset_t resulting_offset;
    ccimp_fs_seek_origin_t CONST origin;
} ccimp_fs_file_seek_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_file_handle_t CONST handle;
    ccimp_file_offset_t CONST length_in_bytes ;
} ccimp_fs_file_truncate_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    char const * CONST path;
} ccimp_fs_file_remove_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    char const * CONST path;
    ccimp_fs_dir_handle_t handle;
} ccimp_fs_dir_open_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_dir_handle_t CONST handle;
    char * CONST entry_name;
    size_t CONST bytes_available;
} ccimp_fs_dir_read_entry_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    ccimp_fs_dir_handle_t CONST handle;
} ccimp_fs_dir_close_t;

typedef struct {
    uint32_t    last_modified;
    ccimp_fs_dir_entry_type_t type;
    ccimp_file_offset_t file_size;
} ccimp_fs_stat_t;

typedef struct {
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    char const * CONST path;
    ccimp_fs_stat_t status;
} ccimp_fs_dir_entry_status_t;

typedef struct {
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    char const * CONST path;
    struct {
        ccimp_fs_hash_alg_t CONST requested;
        ccimp_fs_hash_alg_t actual;
    } hash_alg;
} ccimp_fs_get_hash_alg_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t errnum;
    char const * CONST path;
    ccimp_fs_hash_alg_t CONST hash_algorithm;
    void * CONST hash_value;
    size_t CONST bytes_requested;
} ccimp_fs_hash_file_t;

typedef enum
{
    CCIMP_FS_ERROR_UNKNOWN,
    CCIMP_FS_ERROR_PATH_NOT_FOUND,
    CCIMP_FS_ERROR_INSUFFICIENT_SPACE,
    CCIMP_FS_ERROR_FORMAT,
    CCIMP_FS_ERROR_INVALID_PARAMETER,
    CCIMP_FS_ERROR_INSUFFICIENT_MEMORY,
    CCIMP_FS_ERROR_PERMISSION_DENIED
} ccimp_fs_error_t;

typedef struct
{
    void * imp_context;
    ccimp_fs_errnum_t CONST errnum;
    char  * CONST error_string;
    size_t  CONST bytes_available;
    size_t  bytes_used;
    ccimp_fs_error_t error_status;
} ccimp_fs_error_desc_t;

typedef enum
{
    CCIMP_FS_SESSION_ERROR_NONE,
    CCIMP_FS_SESSION_ERROR_FATAL,
    CCIMP_FS_SESSION_ERROR_INVALID_OPCODE,
    CCIMP_FS_SESSION_ERROR_FORMAT,
    CCIMP_FS_SESSION_ERROR_SESSION_IN_USE,
    CCIMP_FS_SESSION_ERROR_UNKNOWN_SESSION,
    CCIMP_FS_SESSION_ERROR_COMPRESSION_FAILURE,
    CCIMP_FS_SESSION_ERROR_DECOMPRESSION_FAILURE,
    CCIMP_FS_SESSION_ERROR_MEMORY_FAILED,
    CCIMP_FS_SESSION_ERROR_SENDING,
    CCIMP_FS_SESSION_ERROR_CANCEL,
    CCIMP_FS_SESSION_ERROR_BUSY,
    CCIMP_FS_SESSION_ERROR_ACK,
    CCIMP_FS_SESSION_ERROR_TIMEOUT,
    CCIMP_FS_SESSION_ERROR_NO_SERVICE
} ccimp_session_error_status_t;

typedef struct
{
    void * imp_context;
    ccimp_session_error_status_t CONST session_error;
} ccimp_fs_session_error_t;

ccimp_status_t ccimp_fs_file_open(ccimp_fs_file_open_t * const file_open_data);
ccimp_status_t ccimp_fs_file_read(ccimp_fs_file_read_t * const file_read_data);
ccimp_status_t ccimp_fs_file_write(ccimp_fs_file_write_t * const file_write_data) ;
ccimp_status_t ccimp_fs_file_close(ccimp_fs_file_close_t * const file_close_data);
ccimp_status_t ccimp_fs_file_seek(ccimp_fs_file_seek_t * const file_seek_data);
ccimp_status_t ccimp_fs_file_truncate(ccimp_fs_file_truncate_t * const file_truncate_data);
ccimp_status_t ccimp_fs_file_remove(ccimp_fs_file_remove_t * const file_remove_data);
ccimp_status_t ccimp_fs_dir_open(ccimp_fs_dir_open_t * const dir_open_data);
ccimp_status_t ccimp_fs_dir_read_entry(ccimp_fs_dir_read_entry_t * const dir_read_data);
ccimp_status_t ccimp_fs_dir_entry_status(ccimp_fs_dir_entry_status_t * const dir_entry_status_data);
ccimp_status_t ccimp_fs_dir_close(ccimp_fs_dir_close_t * const dir_close_data);
ccimp_status_t ccimp_fs_hash_alg(ccimp_fs_get_hash_alg_t * const hash_status_data);
ccimp_status_t ccimp_fs_hash_file(ccimp_fs_hash_file_t * const file_hash_data);
ccimp_status_t ccimp_fs_error_desc(ccimp_fs_error_desc_t * const error_desc_data);
ccimp_status_t ccimp_fs_session_error(ccimp_fs_session_error_t * const session_error_data);

#endif
