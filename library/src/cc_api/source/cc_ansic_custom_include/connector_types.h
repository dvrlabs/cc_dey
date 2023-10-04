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

#ifndef CONNECTOR_TYPES_H_
#define CONNECTOR_TYPES_H_

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stddef.h>

#include "custom/custom_ccimp_types.h"

typedef ccimp_network_handle_t connector_network_handle_t;

#define CONNECTOR_NETWORK_HANDLE_NOT_INITIALIZED            CCIMP_NETWORK_HANDLE_NOT_INITIALIZED

typedef void * connector_filesystem_file_handle_t;
#define CONNECTOR_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED    NULL

typedef ccimp_fs_dir_handle_t connector_filesystem_dir_handle_t;
#define CONNECTOR_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED     CCIMP_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED

typedef void * connector_filesystem_errnum_t;
#define CONNECTOR_FILESYSTEM_ERRNUM_NONE    NULL


#endif
