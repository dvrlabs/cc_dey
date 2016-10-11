/*
 * load_config.h
 *
 * Copyright (C) 2016 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi.
 * International Inc. By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered. This is an unpublished copyrighted work of Digi International
 * Inc. Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to restrictions
 * set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
 * Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and
 * (2) of the Commercial Computer Software - Restricted Rights at 48 CFR
 * 52.227-19, as applicable.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 *
 * Description: Cloud Connector configuration loader header.
 *
 */
#ifndef LOAD_CONFIG_H_
#define LOAD_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "ccimp/ccimp_types.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define FS_SERVICE			(1 << 0)
#define DATA_SERVICE		(1 << 1)
#define SYS_MONITOR_SERVICE	(1 << 2)

#define SYS_MON_MEMORY		(1 << 0)
#define SYS_MON_LOAD		(1 << 1)
#define SYS_MON_TEMP		(1 << 2)

#define LOG_LEVEL_ERROR		LOG_ERR
#define LOG_LEVEL_INFO		LOG_INFO
#define LOG_LEVEL_DEBUG		LOG_DEBUG

/*------------------------------------------------------------------------------
                 D A T A    T Y P E S    D E F I N I T I O N S
------------------------------------------------------------------------------*/
/**
 * struct vdir_t - Virtual directory configuration type
 *
 * @name:	Name of the virtual directory
 * @path:	Local path where the virtual directory is mapped
 */
typedef struct {
	char * name;
	char * path;
} vdir_t;

/**
 * struct cc_cfg_t - Cloud Connector configuration type
 *
 * @vendor_id:					Identifier of the Device Cloud user account
 * @device_type:				Name of the device running Cloud Connector
 * @fw_version:					Version of the firmware running Cloud Connector
 * @url:						Device Cloud URL
 * @enable_reconnect:			Enabled reconnection when connection is lost
 * @keepalive_rx:				Keep alive receiving frequency (seconds)
 * @keepalive_tx:				Keep alive transmitting frequency (seconds)
 * @wait_count:					Number of lost keep alives to consider the connection lost
 * @services:					Enabled services
 * @vdirs:						List of virtual directories
 * @n_vdirs:					Number of virtual directories in the list
 * @sys_mon_parameters:			Enabled parameters to monitor
 * @sys_mon_sample_rate:		Frequency at which gather system information
 * @sys_mon_num_samples_upload:	Number of samples of each channel to gather before uploading
 * @log_level:					Level of messaging to log
 * @log_console:				Enable messages logging to the console
 *
 */
typedef struct {
	uint32_t vendor_id;
	char * device_type;
	char * fw_version;

	char * url;
	ccapi_bool_t enable_reconnect;
	uint16_t keepalive_rx;
	uint16_t keepalive_tx;
	uint16_t wait_count;

	uint8_t services;

	vdir_t * vdirs;
	unsigned int n_vdirs;

	uint16_t sys_mon_parameters;

	uint32_t sys_mon_sample_rate;
	uint32_t sys_mon_num_samples_upload;

	int log_level;
	ccapi_bool_t log_console;
} cc_cfg_t;

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
int parse_configuration(const char * const filename, cc_cfg_t * cc_cfg);
void free_cfg(cc_cfg_t * const config);

#endif /* LOAD_CONFIG_H_ */
