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

#ifndef _CCXAPI_H_
#define _CCXAPI_H_

/* 
 * Cloud Connector Multi-instance API
 *
 * Don't distribute this file. It's only for internal usage.
 *
 * This header is the multi-instance variant of ccapi.h
 * include "ccapi/ccxapi.h" instead of "ccapi/ccapi.h" from your applications if you pretend to start several 
 * instances of Cloud Connector.
 * Then use the ccxapi_* functions declared below instead of the ccapi_* functions declared in "ccapi/ccapi.h"
 */

#include "ccapi/ccapi.h"

typedef struct ccapi_handle * ccapi_handle_t;

ccapi_start_error_t ccxapi_start(ccapi_handle_t * const ccapi_handle, ccapi_start_t const * const start);
ccapi_stop_error_t ccxapi_stop(ccapi_handle_t const ccapi_handle, ccapi_stop_t const behavior);

ccapi_tcp_start_error_t ccxapi_start_transport_tcp(ccapi_handle_t const ccapi_handle, ccapi_tcp_info_t const * const tcp_start);
ccapi_tcp_stop_error_t ccxapi_stop_transport_tcp(ccapi_handle_t const ccapi_handle, ccapi_tcp_stop_t const * const tcp_stop);

ccapi_udp_start_error_t ccxapi_start_transport_udp(ccapi_handle_t const ccapi_handle, ccapi_udp_info_t const * const udp_start);
ccapi_udp_stop_error_t ccxapi_stop_transport_udp(ccapi_handle_t const ccapi_handle, ccapi_udp_stop_t const * const udp_stop);

ccapi_sms_start_error_t ccxapi_start_transport_sms(ccapi_handle_t const ccapi_handle, ccapi_sms_info_t const * const sms_start);
ccapi_sms_stop_error_t ccxapi_stop_transport_sms(ccapi_handle_t const ccapi_handle, ccapi_sms_stop_t const * const sms_stop);

ccapi_send_error_t ccxapi_send_data(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t bytes, ccapi_send_behavior_t behavior);
ccapi_send_error_t ccxapi_send_data_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const cloud_path, char const * const content_type, void const * const data, size_t bytes, ccapi_send_behavior_t behavior, unsigned long const timeout, ccapi_string_info_t * const hint);
ccapi_send_error_t ccxapi_send_file(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t behavior);
ccapi_send_error_t ccxapi_send_file_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const local_path, char const * const cloud_path, char const * const content_type, ccapi_send_behavior_t behavior, unsigned long const timeout, ccapi_string_info_t * const hint);

ccapi_receive_error_t ccxapi_receive_add_target(ccapi_handle_t const ccapi_handle, char const * const target, ccapi_receive_data_cb_t data_cb, ccapi_receive_status_cb_t status_cb, size_t maximum_request_size);
ccapi_receive_error_t ccxapi_receive_remove_target(ccapi_handle_t const ccapi_handle, char const * const target);

ccapi_fs_error_t ccxapi_fs_add_virtual_dir(ccapi_handle_t const ccapi_handle, char const * const virtual_dir, char const * const actual_dir);
ccapi_fs_error_t ccxapi_fs_remove_virtual_dir(ccapi_handle_t const ccapi_handle, char const * const virtual_dir);

ccapi_dp_b_error_t ccxapi_dp_binary_send_data(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes);
ccapi_dp_b_error_t ccxapi_dp_binary_send_data_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const stream_id, void const * const data, size_t const bytes, unsigned long const timeout, ccapi_string_info_t * const hint);
ccapi_dp_b_error_t ccxapi_dp_binary_send_file(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const local_path, char const * const stream_id);
ccapi_dp_b_error_t ccxapi_dp_binary_send_file_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, char const * const local_path, char const * const stream_id, unsigned long const timeout, ccapi_string_info_t * const hint);

ccapi_dp_error_t ccxapi_dp_send_collection(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, ccapi_dp_collection_handle_t const dp_collection);
ccapi_dp_error_t ccxapi_dp_send_collection_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, ccapi_dp_collection_handle_t const dp_collection, unsigned long const timeout, ccapi_string_info_t * const hint);

ccapi_ping_error_t ccxapi_send_ping(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport);
ccapi_ping_error_t ccxapi_send_ping_with_reply(ccapi_handle_t const ccapi_handle, ccapi_transport_t const transport, unsigned long const timeout);

#endif
