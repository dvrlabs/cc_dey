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

#include "connector_api.h"

#if !(defined CONST)
#define CONST const
#endif
#include "rci_config.h"

#define CONNECTOR_RCI_ERROR_BAD_COMMAND (connector_remote_all_strings+0)
#define CONNECTOR_RCI_ERROR_BAD_DESCRIPTOR (connector_remote_all_strings+12)
#define CONNECTOR_RCI_ERROR_BAD_VALUE (connector_remote_all_strings+30)
#define CONNECTOR_GLOBAL_ERROR_LOAD_FAIL (connector_remote_all_strings+40)
#define CONNECTOR_GLOBAL_ERROR_SAVE_FAIL (connector_remote_all_strings+50)
#define CONNECTOR_GLOBAL_ERROR_MEMORY_FAIL (connector_remote_all_strings+60)
#define CONNECTOR_GLOBAL_ERROR_NOT_IMPLEMENTED (connector_remote_all_strings+80)

static char CONST connector_remote_all_strings[] = {
	11,'B','a','d',' ','c','o','m','m','a','n','d',
	17,'B','a','d',' ','c','o','n','f','i','g','u','r','a','t','i','o','n',
	9,'B','a','d',' ','v','a','l','u','e',
	9,'L','o','a','d',' ','f','a','i','l',
	9,'S','a','v','e',' ','f','a','i','l',
	19,'I','n','s','u','f','f','i','c','i','e','n','t',' ','m','e','m','o','r','y',
	15,'N','o','t',' ','i','m','p','l','e','m','e','n','t','e','d'
};

static char const * const connector_rci_errors[] = {
	CONNECTOR_RCI_ERROR_BAD_COMMAND, /*bad_command*/
	CONNECTOR_RCI_ERROR_BAD_DESCRIPTOR, /*bad_descriptor*/
	CONNECTOR_RCI_ERROR_BAD_VALUE, /*bad_value*/
	CONNECTOR_GLOBAL_ERROR_LOAD_FAIL, /*load_fail*/
	CONNECTOR_GLOBAL_ERROR_SAVE_FAIL, /*save_fail*/
	CONNECTOR_GLOBAL_ERROR_MEMORY_FAIL, /*memory_fail*/
	CONNECTOR_GLOBAL_ERROR_NOT_IMPLEMENTED /*not_implemented*/
};

static connector_group_element_t CONST setting_static_location_elements[] =
{
	{  /*use_static_location*/
		connector_element_access_read_write,
		connector_element_type_on_off,
		{
			0,
			NULL
		}
	},
	{  /*latitude*/
		connector_element_access_read_write,
		connector_element_type_float,
		{
			0,
			NULL
		}
	},
	{  /*longitude*/
		connector_element_access_read_write,
		connector_element_type_float,
		{
			0,
			NULL
		}
	},
	{  /*altitude*/
		connector_element_access_read_write,
		connector_element_type_float,
		{
			0,
			NULL
		}
	}
};

static connector_group_element_t CONST setting_system_monitor_elements[] =
{
	{  /*enable_sysmon*/
		connector_element_access_read_write,
		connector_element_type_on_off,
		{
			0,
			NULL
		}
	},
	{  /*sample_rate*/
		connector_element_access_read_write,
		connector_element_type_uint32,
		{
			0,
			NULL
		}
	},
	{  /*n_dp_upload*/
		connector_element_access_read_write,
		connector_element_type_uint32,
		{
			0,
			NULL
		}
	},
	{  /*enable_sysmon_mem*/
		connector_element_access_read_write,
		connector_element_type_on_off,
		{
			0,
			NULL
		}
	},
	{  /*enable_sysmon_cpuload*/
		connector_element_access_read_write,
		connector_element_type_on_off,
		{
			0,
			NULL
		}
	},
	{  /*enable_sysmon_cputemp*/
		connector_element_access_read_write,
		connector_element_type_on_off,
		{
			0,
			NULL
		}
	}
};

static connector_group_element_t CONST setting_system_elements[] = {
	{  /*description*/
		connector_element_access_read_write,
		connector_element_type_string,
		{
			0,
			NULL
		}
	},
	{  /*contact*/
		connector_element_access_read_write,
		connector_element_type_string,
		{
			0,
			NULL
		}
	},
	{  /*location*/
		connector_element_access_read_write,
		connector_element_type_string,
		{
			0,
			NULL
		}
	}
};

static connector_group_t CONST connector_setting_groups[] =
{
	{  /*static_location*/
		1 , /* instances */
		{
			ARRAY_SIZE(setting_static_location_elements),
			setting_static_location_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	},
	{  /*system_monitor*/
		1 , /* instances */
		{
			ARRAY_SIZE(setting_system_monitor_elements),
			setting_system_monitor_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	},
	{  /*system*/
		1 , /* instances */
		{
			ARRAY_SIZE(setting_system_elements),
			setting_system_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	}
};

static connector_group_element_t CONST state_device_state_elements[] =
{
	{  /*system_up_time*/
		connector_element_access_read_only,
		connector_element_type_uint32,
		{
			0,
			NULL
		}
	}
};

static connector_group_element_t CONST state_gps_stats_elements[] =
{
	{  /*latitude*/
		connector_element_access_read_only,
		connector_element_type_string,
		{
			0,
			NULL
		}
	},
	{  /*longitude*/
		connector_element_access_read_only,
		connector_element_type_string,
		{
			0,
			NULL
		}
	}
};

static connector_group_element_t CONST state_device_information_elements[] = {
	{  /*dey_version*/
		connector_element_access_read_only,
		connector_element_type_string
	},
	{  /*kernel_version*/
		connector_element_access_read_only,
		connector_element_type_string
	},
	{  /*uboot_version*/
		connector_element_access_read_only,
		connector_element_type_string
	},
	{  /*hardware*/
		connector_element_access_read_only,
		connector_element_type_string
	},
	{  /*kinetis*/
		connector_element_access_read_only,
		connector_element_type_string
	}
};

static connector_group_element_t CONST state_primary_interface_elements[] =
{
	{  /*connection_type*/
		connector_element_access_read_only,
		connector_element_type_string,
		{
			0,
			NULL
		}
	},
	{  /*ip_addr*/
		connector_element_access_read_only,
		connector_element_type_string,
		{
			0,
			NULL
		}
	}
};

static connector_group_t CONST connector_state_groups[] =
{
	{  /*device_state*/
		1 , /* instances */
		{
			ARRAY_SIZE(state_device_state_elements),
			state_device_state_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	},
	{  /*primary_interface*/
		1 , /* instances */
		{
			ARRAY_SIZE(state_primary_interface_elements),
			state_primary_interface_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	},
	{  /*gps_stats*/
		1 , /* instances */
		{
			ARRAY_SIZE(state_gps_stats_elements),
			state_gps_stats_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	},
	{  /*device_information*/
		1 , /* instances */
		{
			ARRAY_SIZE(state_device_information_elements),
			state_device_information_elements
		},
		{
			0,
			NULL
		}  /* errors*/
	}
};

static connector_remote_group_table_t CONST connector_group_table[] =
{
	{
		connector_setting_groups,
		ARRAY_SIZE(connector_setting_groups)
	},
	{
		connector_state_groups,
		ARRAY_SIZE(connector_state_groups)
	}
};

connector_remote_config_data_t rci_internal_data = {
	connector_group_table,
	connector_rci_errors,
	7,
	0,
	0,
	NULL
};

connector_remote_config_data_t * const rci_descriptor_data = &rci_internal_data;
