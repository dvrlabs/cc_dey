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

#ifndef _TEST_HELPER_FUNCTIONS_H_
#define _TEST_HELPER_FUNCTIONS_H_

#define CCAPI_CONST_PROTECTION_UNLOCK
#define CONNECTOR_CONST_PROTECTION

#define TH_DEVICE_TYPE_STRING      "Device type"
#define TH_DEVICE_CLOUD_URL_STRING "devicecloud.digi.com"

#include "CppUTest/CommandLineTestRunner.h"
#include "mocks.h"
#include <cstdio>

extern "C" {
#include "ccapi/ccapi.h"
#include "ccapi_definitions.h"

extern void wait_for_ccimp_threads(void);

typedef struct {
    ccimp_fs_file_handle_t ccimp_handle;
    char * file_path;
    ccapi_fs_request_t request;
} ccapi_fs_file_handle_t;
}

typedef int my_filesystem_context_t;
typedef int my_filesystem_dir_handle_t;

typedef enum {
    TH_MALLOC_RETURN_NORMAL,
    TH_MALLOC_RETURN_NULL
} th_malloc_behavior_t;

extern my_filesystem_context_t my_fs_context; /* Defined in mock_ccimp_filesystem.cpp */
extern my_filesystem_dir_handle_t dir_handle; /* Defined in mock_ccimp_filesystem.cpp */
extern ccapi_bool_t ccapi_tcp_keepalives_cb_called;
extern ccapi_keepalive_status_t ccapi_tcp_keepalives_cb_argument;
extern ccapi_bool_t ccapi_tcp_close_cb_called;
extern ccapi_tcp_close_cause_t ccapi_tcp_close_cb_argument;
extern ccapi_bool_t ccapi_udp_close_cb_called;
extern ccapi_udp_close_cause_t ccapi_udp_close_cb_argument;
extern ccapi_bool_t ccapi_sms_close_cb_called;
extern ccapi_sms_close_cause_t ccapi_sms_close_cb_argument;


void th_fill_start_structure_with_good_parameters(ccapi_start_t * start);
void th_start_ccapi(void);
void th_fill_tcp_wan_ipv4_callbacks_info(ccapi_tcp_info_t * tcp_start);
void th_fill_tcp_lan_ipv4(ccapi_tcp_info_t * tcp_start);

void th_start_udp(void);
void th_start_sms(void);

void th_start_tcp_wan_ipv4_with_callbacks(void);
void th_start_tcp_lan_ipv4(void);
void th_start_tcp_lan_ipv6_password_keepalives(void);
void th_stop_ccapi(ccapi_data_t * const ccapi_data);
pthread_t th_aux_ccapi_start(void * argument);
int th_stop_aux_thread(pthread_t pthread);
ccapi_fs_file_handle_t * th_filesystem_openfile(char const * const path, connector_file_system_open_t * const ccfsm_open_data, int flags);
void th_filesystem_prepare_ccimp_dir_entry_status_call(ccimp_fs_dir_entry_status_t * const ccimp_fs_dir_entry_status_data, char const * const path);
void th_filesystem_prepare_ccimp_dir_open_data_call(ccimp_fs_dir_open_t * const ccimp_dir_open_data, char const * const path);
void th_filesystem_prepare_ccimp_dir_close_call(ccimp_fs_dir_close_t * const ccimp_dir_close_data);
void th_call_ccimp_fs_error_desc_and_check_error(connector_filesystem_errnum_t ccfsm_errnum, connector_file_system_error_t ccfsm_fs_error);
void create_test_file(char const * const path, void const * const data, size_t bytes);
void destroy_test_file(char const * const path);

void * th_expect_malloc(size_t size, th_malloc_behavior_t behavior, bool expect_free);

void th_check_collection_dp_count(ccapi_dp_collection_handle_t dp_collection, uint32_t const expected_value);

#endif
