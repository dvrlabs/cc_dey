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

#ifndef CC_CONFIG_H_
#define CC_CONFIG_H_

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
	char *name;
	char *path;
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
	char *device_type;
	char *fw_version;

	char *url;
	ccapi_bool_t enable_reconnect;
	uint16_t keepalive_rx;
	uint16_t keepalive_tx;
	uint16_t wait_count;

	uint8_t services;

	vdir_t *vdirs;
	unsigned int n_vdirs;

	uint16_t sys_mon_parameters;

	uint32_t sys_mon_sample_rate;
	uint32_t sys_mon_num_samples_upload;

	ccapi_bool_t use_static_location;
	float latitude;
	float longitude;
	float altitude;

	int log_level;
	ccapi_bool_t log_console;
} cc_cfg_t;

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
int parse_configuration(const char *const filename, cc_cfg_t *cc_cfg);
void free_configuration(cc_cfg_t *const config);
int get_configuration(cc_cfg_t *cc_cfg);
int save_configuration(cc_cfg_t *cc_cfg);
void close_configuration(void);

#endif /* CC_CONFIG_H_ */
