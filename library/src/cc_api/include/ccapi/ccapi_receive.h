/*
* Copyright (c) 2017-2022 Digi International Inc.
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

#ifndef _CCAPI_RECEIVE_H_
#define _CCAPI_RECEIVE_H_

#define CCAPI_RECEIVE_NO_LIMIT ((size_t) -1)

typedef enum {
    CCAPI_RECEIVE_ERROR_NONE,
    CCAPI_RECEIVE_ERROR_CCAPI_NOT_RUNNING,
    CCAPI_RECEIVE_ERROR_NO_RECEIVE_SUPPORT,
    CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY,
    CCAPI_RECEIVE_ERROR_INVALID_TARGET,
    CCAPI_RECEIVE_ERROR_TARGET_NOT_ADDED,
    CCAPI_RECEIVE_ERROR_TARGET_ALREADY_ADDED,
    CCAPI_RECEIVE_ERROR_INVALID_DATA_CB,
    CCAPI_RECEIVE_ERROR_LOCK_FAILED,
    CCAPI_RECEIVE_ERROR_USER_REFUSED_TARGET,
    CCAPI_RECEIVE_ERROR_REQUEST_TOO_BIG,
    CCAPI_RECEIVE_ERROR_STATUS_CANCEL,
    CCAPI_RECEIVE_ERROR_STATUS_TIMEOUT,
    CCAPI_RECEIVE_ERROR_STATUS_SESSION_ERROR
} ccapi_receive_error_t;

typedef ccapi_bool_t (*ccapi_receive_accept_cb_t)(char const * const target, ccapi_transport_t const transport);
typedef ccapi_receive_error_t (*ccapi_receive_data_cb_t)(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info);
typedef void (*ccapi_receive_status_cb_t)(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error);

typedef struct {
    ccapi_receive_accept_cb_t accept;
    ccapi_receive_data_cb_t data;
    ccapi_receive_status_cb_t status;
} ccapi_receive_service_t;

ccapi_receive_error_t ccapi_receive_add_target(char const * const target, ccapi_receive_data_cb_t data_cb, ccapi_receive_status_cb_t status_cb, size_t maximum_request_size);
ccapi_receive_error_t ccapi_receive_remove_target(char const * const target);

#endif
