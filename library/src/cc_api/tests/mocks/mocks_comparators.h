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

#ifndef _MOCKS_COMPARATORS_H_
#define _MOCKS_COMPARATORS_H_

extern MockFunctionComparator ccimp_network_open_t_comparator;
extern MockFunctionComparator ccimp_network_send_t_comparator;
extern MockFunctionComparator ccimp_network_receive_t_comparator;
extern MockFunctionComparator ccimp_network_close_t_comparator;
extern MockFunctionComparator ccimp_create_thread_info_t_comparator;
extern MockFunctionComparator connector_transport_t_comparator;
extern MockFunctionComparator connector_initiate_stop_request_t_comparator;
extern MockFunctionComparator connector_request_data_service_send_t_comparator;
extern MockFunctionComparator connector_sm_send_ping_request_t_comparator;

extern MockFunctionComparator ccimp_fs_file_open_t_comparator;
extern MockFunctionComparator ccimp_fs_file_read_t_comparator;
extern MockFunctionComparator ccimp_fs_file_write_t_comparator;
extern MockFunctionComparator ccimp_fs_file_seek_t_comparator;
extern MockFunctionComparator ccimp_fs_file_close_t_comparator;
extern MockFunctionComparator ccimp_fs_file_truncate_t_comparator;
extern MockFunctionComparator ccimp_fs_file_remove_t_comparator;
extern MockFunctionComparator ccimp_fs_dir_open_t_comparator;
extern MockFunctionComparator ccimp_fs_dir_read_entry_t_comparator;
extern MockFunctionComparator ccimp_fs_dir_entry_status_t_comparator;
extern MockFunctionComparator ccimp_fs_dir_close_t_comparator;
extern MockFunctionComparator ccimp_fs_get_hash_alg_t_comparator;
extern MockFunctionComparator ccimp_fs_hash_file_t_comparator;
extern MockFunctionComparator ccimp_fs_error_desc_t_comparator;
extern MockFunctionComparator ccimp_fs_session_error_t_comparator;
extern MockFunctionComparator connector_request_data_point_t_comparator;

#endif
