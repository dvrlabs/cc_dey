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

#ifndef _CCAPI_INIT_H_
#define _CCAPI_INIT_H_

typedef enum {
     CCAPI_STOP_CCFSM_ERROR
} ccapi_stop_cause_t;

typedef struct {
    ccapi_stop_cause_t CONST stop_cause;
} ccapi_status_info_t;

typedef void (*ccapi_status_callback_t)(ccapi_status_info_t * const info);

typedef enum {
    CCAPI_START_ERROR_NONE,
    CCAPI_START_ERROR_NULL_PARAMETER,
    CCAPI_START_ERROR_INVALID_VENDORID,
    CCAPI_START_ERROR_INVALID_DEVICEID,
    CCAPI_START_ERROR_INVALID_URL,
    CCAPI_START_ERROR_INVALID_DEVICETYPE,
    CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK,
    CCAPI_START_ERROR_INVALID_RCI_REQUEST_CALLBACK,
    CCAPI_START_ERROR_INVALID_FIRMWARE_INFO,
    CCAPI_START_ERROR_INVALID_FIRMWARE_DATA_CALLBACK,
    CCAPI_START_ERROR_INVALID_SM_ENCRYPTION_CALLBACK,
    CCAPI_START_ERROR_INSUFFICIENT_MEMORY,
    CCAPI_START_ERROR_THREAD_FAILED,
    CCAPI_START_ERROR_LOCK_FAILED,
    CCAPI_START_ERROR_ALREADY_STARTED
} ccapi_start_error_t;

typedef enum {
    CCAPI_STOP_ERROR_NONE,
    CCAPI_STOP_ERROR_NOT_STARTED
} ccapi_stop_error_t;

typedef enum {
    CCAPI_STOP_GRACEFULLY,
    CCAPI_STOP_IMMEDIATELY
} ccapi_stop_t;

typedef struct {
        uint32_t vendor_id;
        uint8_t device_id[16];
        char const * device_type;
        char const * device_cloud_url;         /* Shared for TCP and UDP transports, meaningless for SMS */
        ccapi_status_callback_t status;
        struct {
                ccapi_receive_service_t * receive;
                ccapi_fw_service_t * firmware; /* EDP/TCP only */
                ccapi_rci_service_t * rci; /* EDP/TCP only */
                ccapi_filesystem_service_t * file_system;
                ccapi_cli_service_t * cli; /* SM only */
                ccapi_sm_service_t * sm;
#if (defined CONNECTOR_STREAMING_CLI_SERVICE)
                ccapi_streaming_cli_service_t * streaming_cli;
#endif
        } service;
} ccapi_start_t;

ccapi_start_error_t ccapi_start(ccapi_start_t const * const start);
ccapi_stop_error_t ccapi_stop(ccapi_stop_t const behavior);

#endif
