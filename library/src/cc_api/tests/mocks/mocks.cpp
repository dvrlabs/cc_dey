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

#include "mocks.h"

char * assert_buffer;
char * assert_file;

void Mock_create_all()
{
    Mock_ccimp_logging_printf_create();
    Mock_ccimp_os_malloc_create();
    Mock_ccimp_os_free_create();
    Mock_ccimp_os_create_thread_create();
    Mock_connector_init_create();
    Mock_connector_run_create();
    Mock_connector_initiate_action_create();
    Mock_ccimp_network_tcp_open_create();
    Mock_ccimp_network_tcp_send_create();
    Mock_ccimp_network_tcp_receive_create();
    Mock_ccimp_network_tcp_close_create();
    Mock_ccimp_network_udp_open_create();
    Mock_ccimp_network_udp_send_create();
    Mock_ccimp_network_udp_receive_create();
    Mock_ccimp_network_udp_close_create();
    Mock_ccimp_network_sms_open_create();
    Mock_ccimp_network_sms_send_create();
    Mock_ccimp_network_sms_receive_create();
    Mock_ccimp_network_sms_close_create();
    Mock_ccimp_os_get_system_time_create();
    Mock_ccimp_os_lock_create_create();
    Mock_ccimp_os_lock_acquire_create();
    Mock_ccimp_os_lock_release_create();
    Mock_ccimp_fs_file_open_create();
    Mock_ccimp_fs_file_read_create();
    Mock_ccimp_fs_file_write_create();
    Mock_ccimp_fs_file_seek_create();
    Mock_ccimp_fs_file_close_create();
    Mock_ccimp_fs_file_truncate_create();
    Mock_ccimp_fs_file_remove_create();
    Mock_ccimp_fs_dir_open_create();
    Mock_ccimp_fs_dir_read_entry_create();
    Mock_ccimp_fs_dir_entry_status_create();
    Mock_ccimp_fs_dir_close_create();
    Mock_ccimp_fs_hash_alg_create();
    Mock_ccimp_fs_hash_file_create();
    Mock_ccimp_fs_error_desc_create();
    Mock_ccimp_fs_session_error_create();
}

void Mock_destroy_all()
{
    if (ccapi_data_single_instance != NULL)
    {
        /* Dear developer: If you want to call free on this pointer,
         * know that it leads to race conditions making TEST(ccapi_init_test, testDeviceTypeNoMemory) fail
         * once in ~100 iterations. Don't say I didn't tell you! Good luck! =)
         */
        ccapi_data_single_instance = NULL;
    }
    Mock_ccimp_os_create_thread_destroy();
    Mock_ccimp_logging_printf_destroy();
    Mock_ccimp_os_malloc_destroy();
    Mock_ccimp_os_free_destroy();
    Mock_ccimp_os_create_thread_destroy();
    Mock_ccimp_os_lock_create_destroy();
    Mock_ccimp_os_lock_acquire_destroy();
    Mock_ccimp_os_lock_release_destroy();
    Mock_connector_init_destroy();
    Mock_connector_run_destroy();
    Mock_connector_initiate_action_destroy();
    Mock_ccimp_network_tcp_open_destroy();
    Mock_ccimp_network_tcp_send_destroy();
    Mock_ccimp_network_tcp_receive_destroy();
    Mock_ccimp_network_tcp_close_destroy();
    Mock_ccimp_network_udp_open_destroy();
    Mock_ccimp_network_udp_send_destroy();
    Mock_ccimp_network_udp_receive_destroy();
    Mock_ccimp_network_udp_close_destroy();
    Mock_ccimp_network_sms_open_destroy();
    Mock_ccimp_network_sms_send_destroy();
    Mock_ccimp_network_sms_receive_destroy();
    Mock_ccimp_network_sms_close_destroy();
    Mock_ccimp_os_get_system_time_destroy();
    Mock_ccimp_fs_file_open_destroy();
    Mock_ccimp_fs_file_read_destroy();
    Mock_ccimp_fs_file_write_destroy();
    Mock_ccimp_fs_file_seek_destroy();
    Mock_ccimp_fs_file_close_destroy();
    Mock_ccimp_fs_file_truncate_destroy();
    Mock_ccimp_fs_file_remove_destroy();
    Mock_ccimp_fs_dir_open_destroy();
    Mock_ccimp_fs_dir_read_entry_destroy();
    Mock_ccimp_fs_dir_entry_status_destroy();
    Mock_ccimp_fs_dir_close_destroy();
    Mock_ccimp_fs_hash_alg_destroy();
    Mock_ccimp_fs_hash_file_destroy();
    Mock_ccimp_fs_error_desc_destroy();
    Mock_ccimp_fs_session_error_destroy();
    ASSERT_CLEAN();
    mock().removeAllComparators();
    mock().checkExpectations();
    mock().clear();
}
