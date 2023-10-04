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

#ifndef _CCAPI_FIRMWARE_UPDATE_H_
#define _CCAPI_FIRMWARE_UPDATE_H_

/* We enum here values with meaning for the cloud for the request operation */
typedef enum {
    CCAPI_FW_REQUEST_ERROR_NONE,
    CCAPI_FW_REQUEST_ERROR_DOWNLOAD_DENIED,
    CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_SIZE,
    CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_VERSION,
    CCAPI_FW_REQUEST_ERROR_DOWNLOAD_UNAUTHENTICATED,
    CCAPI_FW_REQUEST_ERROR_DOWNLOAD_NOT_ALLOWED,
    CCAPI_FW_REQUEST_ERROR_DOWNLOAD_CONFIGURED_TO_REJECT,
    CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR
} ccapi_fw_request_error_t;

/* Any error reported to cloud different than 0 have the same result: Failed with status[12]: Aborted By Target */
typedef enum {
    CCAPI_FW_DATA_ERROR_NONE,
    CCAPI_FW_DATA_ERROR_INVALID_DATA
} ccapi_fw_data_error_t;

/* Values that cc_fsm will forward as a cause */
typedef enum {
    CCAPI_FW_CANCEL_USER_ABORT,
    CCAPI_FW_CANCEL_DEVICE_ERROR,
    CCAPI_FW_CANCEL_INVALID_OFFSET,
    CCAPI_FW_CANCEL_INVALID_DATA,
    CCAPI_FW_CANCEL_HARDWARE_ERROR
} ccapi_fw_cancel_error_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
    uint8_t build;
} ccapi_firmware_target_version_t;

typedef ccapi_fw_request_error_t (*ccapi_fw_request_cb_t)(unsigned int const target, char const * const filename, size_t const total_size);
typedef ccapi_fw_data_error_t (*ccapi_fw_data_cb_t)(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk);
typedef void (*ccapi_fw_cancel_cb_t)(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason);
typedef void (*ccapi_fw_reset_cb_t)(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * version);

typedef struct {
    ccapi_firmware_target_version_t version;
    char const * description;
    char const * filespec;
    size_t maximum_size;
    size_t chunk_size;
} ccapi_firmware_target_t;

typedef struct {
    struct {
        uint8_t count;
        ccapi_firmware_target_t * item;
    } target;
    struct {
        ccapi_fw_request_cb_t request;
        ccapi_fw_data_cb_t data;
        ccapi_fw_cancel_cb_t cancel;
        ccapi_fw_reset_cb_t reset;
    } callback;
} ccapi_fw_service_t;

#endif
