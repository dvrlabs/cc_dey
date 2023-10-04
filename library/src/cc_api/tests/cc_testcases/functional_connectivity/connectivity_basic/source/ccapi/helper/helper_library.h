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

#define DEVICE_TYPE_STRING      "Device type"
#define DEVICE_CLOUD_URL_STRING "devicecloud.digi.com"
#define DEVICE_CLOUD_VENDOR_ID  0x03000026
#define DEVICE_IP               {0xC0, 0xA8, 0x01, 0x01}
#define DEVICE_MAC_ADDRESS      {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}
#define DEVICE_ID      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x9D, 0xFF, 0xFF, 0xAA, 0xBB, 0xCC}



/* Basic */
extern void fill_device_settings(ccapi_start_t * start);
extern void print_ccapi_start_error(ccapi_start_error_t error);
/* Filesystem */
extern ccapi_fs_access_t ccapi_filesystem_access_callback(char const * const local_path, ccapi_fs_request_t const request);
extern void ccapi_filesystem_changed_callback(char const * const local_path, ccapi_fs_changed_t const request);
extern void fill_filesystem_service(ccapi_start_t * start);
/* Firmware */
extern ccapi_fw_data_error_t ccapi_firmware_data_callback(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk);
extern void ccapi_firmware_cancel_callback(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason);
extern void ccapi_firmware_reset_callback(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * version);
extern void fill_firmwareupdate_service(ccapi_start_t * start);
/* TCP Transport */
extern ccapi_bool_t ccapi_tcp_close_cb(ccapi_tcp_close_cause_t cause);
extern void fill_tcp_transport_settings(ccapi_tcp_info_t * tcp_info);
