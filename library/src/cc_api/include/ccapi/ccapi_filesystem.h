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

#ifndef _CCAPI_FILESYSTEM_H_
#define _CCAPI_FILESYSTEM_H_

typedef enum {
  CCAPI_FS_REQUEST_READ,
  CCAPI_FS_REQUEST_WRITE,
  CCAPI_FS_REQUEST_READWRITE,
  CCAPI_FS_REQUEST_REMOVE,
  CCAPI_FS_REQUEST_LIST,
  CCAPI_FS_REQUEST_UNKNOWN
} ccapi_fs_request_t;

typedef enum {
    CCAPI_FS_CHANGED_MODIFIED,
    CCAPI_FS_CHANGED_REMOVED
} ccapi_fs_changed_t;

typedef enum {
    CCAPI_FS_ACCESS_DENY,
    CCAPI_FS_ACCESS_ALLOW
} ccapi_fs_access_t;

typedef ccapi_fs_access_t (*ccapi_fs_access_cb_t)(char const * const local_path, ccapi_fs_request_t const request);
typedef void (*ccapi_fs_changed_cb_t)(char const * const local_path, ccapi_fs_changed_t const request);

typedef struct {
    ccapi_fs_access_cb_t access;
    ccapi_fs_changed_cb_t changed;
} ccapi_filesystem_service_t;

typedef enum {
    CCAPI_FS_ERROR_NONE,
    CCAPI_FS_ERROR_CCAPI_STOPPED,
    CCAPI_FS_ERROR_NO_FS_SUPPORT,
    CCAPI_FS_ERROR_INVALID_PATH,
    CCAPI_FS_ERROR_INSUFFICIENT_MEMORY,
    CCAPI_FS_ERROR_NOT_A_DIR,
    CCAPI_FS_ERROR_NOT_MAPPED,
    CCAPI_FS_ERROR_ALREADY_MAPPED,
    CCAPI_FS_ERROR_LOCK_FAILED
} ccapi_fs_error_t;

ccapi_fs_error_t ccapi_fs_add_virtual_dir(char const * const virtual_dir, char const * const local_dir);
ccapi_fs_error_t ccapi_fs_remove_virtual_dir(char const * const virtual_dir);

#endif
