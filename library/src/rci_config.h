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

#ifndef rci_config_h
#define rci_config_h

typedef enum {
	connector_global_error_OFFSET = connector_rci_error_COUNT,
	connector_global_error_load_fail = connector_global_error_OFFSET,
	connector_global_error_save_fail,
	connector_global_error_memory_fail,
	connector_global_error_not_implemented,
	connector_global_error_COUNT
} connector_global_error_id_t;

typedef enum {
	connector_setting_static_location_use_static_location,
	connector_setting_static_location_latitude,
	connector_setting_static_location_longitude,
	connector_setting_static_location_altitude,
	connector_setting_static_location_COUNT
} connector_setting_static_location_id_t;

typedef enum {
	connector_setting_system_monitor_enable_sysmon,
	connector_setting_system_monitor_sample_rate,
	connector_setting_system_monitor_n_dp_upload,
	connector_setting_system_monitor_enable_sysmon_mem,
	connector_setting_system_monitor_enable_sysmon_cpuload,
	connector_setting_system_monitor_enable_sysmon_cputemp,
	connector_setting_system_monitor_COUNT
} connector_setting_system_monitor_id_t;

typedef enum {
	connector_setting_system_description,
	connector_setting_system_contact,
	connector_setting_system_location,
	connector_setting_system_COUNT
} connector_setting_system_id_t;

typedef enum {
	connector_setting_static_location,
	connector_setting_system,
	connector_setting_COUNT
} connector_setting_id_t;

typedef enum {
	connector_state_device_state_system_up_time,
	connector_state_device_state_COUNT
} connector_state_device_state_id_t;

typedef enum {
	connector_state_primary_interface_connection_type,
	connector_state_primary_interface_ip_addr,
	connector_state_primary_interface_COUNT
} connector_state_primary_interface_id_t;

typedef enum {
	connector_state_gps_stats_latitude,
	connector_state_gps_stats_longitude,
	connector_state_gps_stats_COUNT
} connector_state_gps_stats_id_t;

typedef enum {
	connector_state_device_information_dey_version,
	connector_state_device_information_kernel_version,
	connector_state_device_information_uboot_version,
	connector_state_device_information_hardware,
	connector_state_device_information_kinetis,
	connector_state_device_information_COUNT
} connector_state_device_information_id_t;

typedef enum {
	connector_state_device_state,
	connector_state_primary_interface,
	connector_state_gps_stats,
	connector_state_device_information,
	connector_state_COUNT
} connector_state_id_t;

#endif
