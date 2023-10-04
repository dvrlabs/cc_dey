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

#ifndef _CCAPI_DATAPOINTS_BINARY_H_
#define _CCAPI_DATAPOINTS_BINARY_H_

#define CCAPI_DP_B_WAIT_FOREVER 0UL

typedef enum {
    CCAPI_DP_B_ERROR_NONE,
    CCAPI_DP_B_ERROR_CCAPI_NOT_RUNNING,
    CCAPI_DP_B_ERROR_TRANSPORT_NOT_STARTED,
    CCAPI_DP_B_ERROR_FILESYSTEM_NOT_SUPPORTED,
    CCAPI_DP_B_ERROR_INVALID_STREAM_ID,
    CCAPI_DP_B_ERROR_INVALID_DATA,
    CCAPI_DP_B_ERROR_INVALID_LOCAL_PATH,
    CCAPI_DP_B_ERROR_NOT_A_FILE,
    CCAPI_DP_B_ERROR_ACCESSING_FILE,
    CCAPI_DP_B_ERROR_INVALID_HINT_POINTER,
    CCAPI_DP_B_ERROR_INSUFFICIENT_MEMORY,
    CCAPI_DP_B_ERROR_LOCK_FAILED,
    CCAPI_DP_B_ERROR_INITIATE_ACTION_FAILED,
    CCAPI_DP_B_ERROR_STATUS_CANCEL,
    CCAPI_DP_B_ERROR_STATUS_TIMEOUT,
    CCAPI_DP_B_ERROR_STATUS_SESSION_ERROR,
    CCAPI_DP_B_ERROR_RESPONSE_BAD_REQUEST,
    CCAPI_DP_B_ERROR_RESPONSE_UNAVAILABLE,
    CCAPI_DP_B_ERROR_RESPONSE_CLOUD_ERROR
} ccapi_dp_b_error_t;

ccapi_dp_b_error_t ccapi_dp_binary_send_data(ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes);
ccapi_dp_b_error_t ccapi_dp_binary_send_data_with_reply(ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes, unsigned long const timeout, ccapi_string_info_t * const hint);
ccapi_dp_b_error_t ccapi_dp_binary_send_file(ccapi_transport_t const transport, char const * const local_path, char const * const stream_id);
ccapi_dp_b_error_t ccapi_dp_binary_send_file_with_reply(ccapi_transport_t const transport, char const * const local_path, char const * const stream_id, unsigned long const timeout, ccapi_string_info_t * const hint);

#endif
