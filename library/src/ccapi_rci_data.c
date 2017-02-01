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

#include "ccapi/ccapi.h"
#include "ccapi_rci_functions.h"

static ccapi_rci_element_t const setting_ethernet_elements[] =
{
	{  /*iface_name*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_iface_name_get
	},
	{  /*enabled*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_enabled_get
	},
	{  /*conn_type*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_conn_type_get
	},
	{  /*ipaddr*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_ipaddr_get
	},
	{  /*netmask*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_netmask_get
	},
	{  /*dns1*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_dns1_get
	},
	{  /*dns2*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_dns2_get
	},
	{  /*gateway*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_gateway_get
	},
	{  /*mac_addr*/
		NULL,
		(ccapi_rci_function_t)rci_setting_ethernet_mac_addr_get
	}
};

static ccapi_rci_element_t const setting_wifi_elements[] = {
	{  /*iface_name*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_iface_name_get
	},
	{  /*enabled*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_enabled_get
	},
	{  /*ssid*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_ssid_get
	},
	{  /*wpa_status*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_wpa_status_get
	},
	{  /*conn_type*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_conn_type_get
	},
	{  /*ipaddr*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_ipaddr_get
	},
	{  /*netmask*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_netmask_get
	},
	{  /*dns1*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_dns1_get
	},
	{  /*dns2*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_dns2_get
	},
	{  /*gateway*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_gateway_get
	},
	{  /*mac_addr*/
		NULL,
		(ccapi_rci_function_t)rci_setting_wifi_mac_addr_get
	}
};

static ccapi_rci_element_t const setting_static_location_elements[] =
{
	{  /*use_static_location*/
		(ccapi_rci_function_t)rci_setting_static_location_use_static_location_set,
		(ccapi_rci_function_t)rci_setting_static_location_use_static_location_get
	},
	{  /*latitude*/
		(ccapi_rci_function_t)rci_setting_static_location_latitude_set,
		(ccapi_rci_function_t)rci_setting_static_location_latitude_get
	},
	{  /*longitude*/
		(ccapi_rci_function_t)rci_setting_static_location_longitude_set,
		(ccapi_rci_function_t)rci_setting_static_location_longitude_get
	},
	{  /*altitude*/
		(ccapi_rci_function_t)rci_setting_static_location_altitude_set,
		(ccapi_rci_function_t)rci_setting_static_location_altitude_get
	}
};

static ccapi_rci_element_t const setting_system_monitor_elements[] =
{
	{  /*enable_sysmon*/
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_set,
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_get
	},
	{  /*sample_rate*/
		(ccapi_rci_function_t)rci_setting_system_monitor_sample_rate_set,
		(ccapi_rci_function_t)rci_setting_system_monitor_sample_rate_get
	},
	{  /*n_dp_upload*/
		(ccapi_rci_function_t)rci_setting_system_monitor_n_dp_upload_set,
		(ccapi_rci_function_t)rci_setting_system_monitor_n_dp_upload_get
	},
	{  /*enable_sysmon_mem*/
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_mem_set,
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_mem_get
	},
	{  /*enable_sysmon_cpuload*/
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_cpuload_set,
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_cpuload_get
	},
	{  /*enable_sysmon_cputemp*/
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_cputemp_set,
		(ccapi_rci_function_t)rci_setting_system_monitor_enable_sysmon_cputemp_get
	}
};

static ccapi_rci_element_t const setting_system_elements[] = {
	{  /*description*/
		(ccapi_rci_function_t)rci_setting_system_description_set,
		(ccapi_rci_function_t)rci_setting_system_description_get
	},
	{  /*contact*/
		(ccapi_rci_function_t)rci_setting_system_contact_set,
		(ccapi_rci_function_t)rci_setting_system_contact_get
	},
	{  /*location*/
		(ccapi_rci_function_t)rci_setting_system_location_set,
		(ccapi_rci_function_t)rci_setting_system_location_get
	}
};

static ccapi_rci_group_t const ccapi_setting_groups[] =
{
	{  /*ethernet*/
		setting_ethernet_elements,
		ARRAY_SIZE(setting_ethernet_elements),
		{
			(ccapi_rci_function_t)rci_setting_ethernet_start,
			(ccapi_rci_function_t)rci_setting_ethernet_end
		}
	},
	{  /*wifi*/
		setting_wifi_elements,
		ARRAY_SIZE(setting_wifi_elements),
		{
			(ccapi_rci_function_t)rci_setting_wifi_start,
			(ccapi_rci_function_t)rci_setting_wifi_end
		}
	},
	{  /*static_location*/
		setting_static_location_elements,
		ARRAY_SIZE(setting_static_location_elements),
		{
			(ccapi_rci_function_t)rci_setting_static_location_start,
			(ccapi_rci_function_t)rci_setting_static_location_end
		}
	},
	{  /*system_monitor*/
		setting_system_monitor_elements,
		ARRAY_SIZE(setting_system_monitor_elements),
		{
			(ccapi_rci_function_t)rci_setting_system_monitor_start,
			(ccapi_rci_function_t)rci_setting_system_monitor_end
		}
	},
	{  /*system*/
		setting_system_elements,
		ARRAY_SIZE(setting_system_elements),
		{
			(ccapi_rci_function_t)rci_setting_system_start,
			(ccapi_rci_function_t)rci_setting_system_end
		}
	}
};

static ccapi_rci_element_t const state_device_state_elements[] =
{
	{  /*system_up_time*/
		NULL,
		(ccapi_rci_function_t)rci_state_device_state_system_up_time_get
	}
};

static ccapi_rci_element_t const state_primary_interface_elements[] =
{
	{  /*connection_type*/
		NULL,
		(ccapi_rci_function_t)rci_state_primary_interface_connection_type_get
	},
	{  /*ip_addr*/
		NULL,
		(ccapi_rci_function_t)rci_state_primary_interface_ip_addr_get
	}
};

static ccapi_rci_element_t const state_gps_stats_elements[] = {
	{  /*latitude*/
		NULL,
		(ccapi_rci_function_t)rci_state_gps_stats_latitude_get
	},
	{  /*longitude*/
		NULL,
		(ccapi_rci_function_t)rci_state_gps_stats_longitude_get
	}
};

static ccapi_rci_element_t const state_device_information_elements[] = {
	{  /*dey_version*/
		NULL,
		(ccapi_rci_function_t)rci_state_device_information_dey_version_get
	},
	{  /*kernel_version*/
		NULL,
		(ccapi_rci_function_t)rci_state_device_information_kernel_version_get
	},
	{  /*uboot_version*/
		NULL,
		(ccapi_rci_function_t)rci_state_device_information_uboot_version_get
	},
	{  /*hardware*/
		NULL,
		(ccapi_rci_function_t)rci_state_device_information_hardware_get
	},
	{  /*kinetis*/
		NULL,
		(ccapi_rci_function_t)rci_state_device_information_kinetis_get
	}
};

static ccapi_rci_group_t const ccapi_state_groups[] =
{
	{  /*device_state*/
		state_device_state_elements,
		ARRAY_SIZE(state_device_state_elements),
		{
			(ccapi_rci_function_t)rci_state_device_state_start,
			(ccapi_rci_function_t)rci_state_device_state_end
		}
	},
	{  /*primary_interface*/
		state_primary_interface_elements,
		ARRAY_SIZE(state_primary_interface_elements),
		{
			(ccapi_rci_function_t)rci_state_primary_interface_start,
			(ccapi_rci_function_t)rci_state_primary_interface_end
		}
	},
	{  /*gps_stats*/
		state_gps_stats_elements,
		ARRAY_SIZE(state_gps_stats_elements),
		{
			(ccapi_rci_function_t)rci_state_gps_stats_start,
			(ccapi_rci_function_t)rci_state_gps_stats_end
		}
	},
	{  /*device_information*/
		state_device_information_elements,
		ARRAY_SIZE(state_device_information_elements),
		{
			(ccapi_rci_function_t)rci_state_device_information_start,
			(ccapi_rci_function_t)rci_state_device_information_end
		}
	}
};

extern connector_remote_config_data_t const rci_internal_data;
ccapi_rci_data_t const ccapi_rci_data =
{
	{
		ccapi_setting_groups,
		ARRAY_SIZE(ccapi_setting_groups)
	},
	{
		ccapi_state_groups,
		ARRAY_SIZE(ccapi_state_groups)
	},
	{
		(ccapi_rci_function_t)rci_session_start_cb,
		(ccapi_rci_function_t)rci_session_end_cb,
		(ccapi_rci_function_t)rci_action_start_cb,
		(ccapi_rci_function_t)rci_action_end_cb,
		(ccapi_rci_function_t)rci_do_command_cb,
		(ccapi_rci_function_t)rci_set_factory_defaults_cb,
		(ccapi_rci_function_t)rci_reboot_cb
	},
	&rci_internal_data
};

