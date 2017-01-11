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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <regex.h>
#include <confuse.h>

#include "ccapi/ccapi.h"
#include "cc_config.h"
#include "cc_logging.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define GROUP_VIRTUAL_DIRS			"virtual-dirs"
#define GROUP_VIRTUAL_DIR			"vdir"

#define ENABLE_DATA_SERVICE			"enable_data_service"
#define ENABLE_FS_SERVICE			"enable_file_system"

#define ENABLE_SYSTEM_MONITOR		"enable_system_monitor"
#define ENABLE_SYS_MON_MEMORY		"system_monitor_enable_memory_sampling"
#define ENABLE_SYS_MON_LOAD			"system_monitor_enable_cpu_load_sampling"
#define ENABLE_SYS_MON_TEMP			"system_monitor_enable_cpu_temperature_sampling"

#define SETTING_VENDOR_ID			"vendor_id"
#define SETTING_DEVICE_TYPE			"device_type"
#define SETTING_FW_VERSION			"firmware_version"

#define SETTING_DC_URL				"url"
#define SETTING_ENABLE_RECONNECT	"enable_reconnect"
#define SETTING_KEEPALIVE_TX		"keep_alive_time"
#define SETTING_KEEPALIVE_RX		"server_keep_alive_time"
#define SETTING_WAIT_TIMES			"wait_times"

#define SETTING_NAME				"name"
#define SETTING_PATH				"path"

#define SETTING_SYS_MON_SAMPLE_RATE	"system_monitor_sample_rate"
#define SETTING_SYS_MON_UPLOAD_SIZE	"system_monitor_upload_samples_size"

#define SETTING_LOG_LEVEL			"log_level"
#define SETTING_LOG_CONSOLE			"log_console"

#define SETTING_UNKNOWN				"__unknown"

#define LOG_LEVEL_ERROR_STR			"error"
#define LOG_LEVEL_INFO_STR			"info"
#define LOG_LEVEL_DEBUG_STR			"debug"

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static int fill_connector_config(cc_cfg_t *cc_cfg);
static int set_connector_config(cc_cfg_t *cc_cfg);
static int cfg_check_vendor_id(cfg_t *cfg, cfg_opt_t *opt);
static int check_vendor_id(long int value);
static int cfg_check_device_type(cfg_t *cfg, cfg_opt_t *opt);
static int cfg_check_fw_version(cfg_t *cfg, cfg_opt_t *opt);
static int cfg_check_dc_url(cfg_t *cfg, cfg_opt_t *opt);
static int cfg_check_keepalive_rx(cfg_t *cfg, cfg_opt_t *opt);
static int cfg_check_keepalive_tx(cfg_t *cfg, cfg_opt_t *opt);
static int cfg_check_range(cfg_t *cfg, cfg_opt_t *opt, uint16_t min, uint16_t max);
static int cfg_check_wait_times(cfg_t *cfg, cfg_opt_t *opt);
static int cfg_check_int_positive(cfg_t *cfg, cfg_opt_t *opt);
static void get_virtual_directories(cfg_t *const cfg, cc_cfg_t *const cc_cfg);
static int get_log_level(void);
static int file_exists(const char *const filename);
static int file_readable(const char *const filename);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
static cfg_t *cfg;
static const char *cfg_path;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
/*
 * parse_configuration() - Parse and save the settings of a configuration file
 *
 * @filename:	Name of the file containing the configuration settings.
 * @cc_cfg:		Connector configuration struct (cc_cfg_t) where the
 * 				settings parsed from the configuration file are saved.
 *
 * Read the provided configuration file and save the settings in the given
 * cc_cfg_t struct. If the file does not exist or cannot be read, the
 * configuration struct is initialized with the default settings.
 *
 * Return: 0 if the file is parsed successfully, -1 if there is an error
 *         parsing the file.
 */
int parse_configuration(const char *const filename, cc_cfg_t *cc_cfg)
{
	/* Virtual directory settings. */
	static cfg_opt_t vdir_opts[] = {
			/* ------------------------------------------------------------ */
			/*|  TYPE   |   SETTING NAME    |  DEFAULT VALUE   |   FLAGS   |*/
			/* ------------------------------------------------------------ */
			CFG_STR		(SETTING_NAME,			"/",			CFGF_NONE),
			CFG_STR		(SETTING_PATH,			"/",			CFGF_NONE),

			/* Needed for unknown settings. */
			CFG_STR 	(SETTING_UNKNOWN,		NULL,			CFGF_NONE),
			CFG_END()
	};

	/* Virtual directories settings. */
	static cfg_opt_t virtual_dirs_opts[] = {
			/* ------------------------------------------------------------ */
			/*|  TYPE   |   SETTING NAME    |  DEFAULT VALUE   |   FLAGS   |*/
			/* ------------------------------------------------------------ */
			CFG_SEC (GROUP_VIRTUAL_DIR,			vdir_opts,		CFGF_MULTI),

			/* Needed for unknown settings. */
			CFG_STR (SETTING_UNKNOWN,			NULL,			CFGF_NONE),
			CFG_END()
	};

	/* Overall structure of the settings. */
	static cfg_opt_t opts[] = {
			/* ------------------------------------------------------------ */
			/*|  TYPE   |   SETTING NAME    |  DEFAULT VALUE   |   FLAGS   |*/
			/* ------------------------------------------------------------ */
			/* General settings. */
			CFG_INT		(SETTING_VENDOR_ID,		0x0,			CFGF_NODEFAULT),
			CFG_STR		(SETTING_DEVICE_TYPE,	"DEY device",	CFGF_NONE),
			CFG_STR		(SETTING_FW_VERSION,	NULL,			CFGF_NODEFAULT),

			/* Connection settings. */
			CFG_STR		(SETTING_DC_URL, "devicecloud.digi.com", CFGF_NONE),
			CFG_BOOL	(SETTING_ENABLE_RECONNECT, cfg_true,	CFGF_NONE),
			CFG_INT		(SETTING_KEEPALIVE_TX,			75,		CFGF_NONE),
			CFG_INT		(SETTING_KEEPALIVE_RX,			75,		CFGF_NONE),
			CFG_INT		(SETTING_WAIT_TIMES,			5,		CFGF_NONE),

			/* Services settings. */
			CFG_BOOL	(ENABLE_FS_SERVICE,		cfg_true,		CFGF_NONE),
			CFG_BOOL	(ENABLE_DATA_SERVICE,	cfg_true,		CFGF_NONE),

			/* File system settings. */
			CFG_SEC		(GROUP_VIRTUAL_DIRS, virtual_dirs_opts, CFGF_NONE),

			/* System monitor settings. */
			CFG_BOOL	(ENABLE_SYSTEM_MONITOR,		cfg_true,	CFGF_NONE),
			CFG_BOOL	(ENABLE_SYS_MON_MEMORY,		cfg_true,	CFGF_NONE),
			CFG_BOOL	(ENABLE_SYS_MON_LOAD,		cfg_true,	CFGF_NONE),
			CFG_BOOL	(ENABLE_SYS_MON_TEMP,		cfg_true,	CFGF_NONE),
			CFG_INT		(SETTING_SYS_MON_SAMPLE_RATE,	5,		CFGF_NONE),
			CFG_INT		(SETTING_SYS_MON_UPLOAD_SIZE,	10,		CFGF_NONE),

			/* Logging settings. */
			CFG_STR		(SETTING_LOG_LEVEL,	LOG_LEVEL_ERROR_STR,CFGF_NONE),
			CFG_BOOL	(SETTING_LOG_CONSOLE,	cfg_false,		CFGF_NONE),

			/* Needed for unknown settings. */
			CFG_STR		(SETTING_UNKNOWN,		NULL,			CFGF_NONE),
			CFG_END()
	};

	if (!file_exists(filename)) {
		log_error("File '%s' does not exist.", filename);
		return -1;
	}

	if (!file_readable(filename)) {
		log_error("File '%s' cannot be read.", filename);
		return -1;
	}

	cfg_path = filename;
	cfg = cfg_init(opts, CFGF_IGNORE_UNKNOWN);
	cfg_set_validate_func(cfg, SETTING_VENDOR_ID, cfg_check_vendor_id);
	cfg_set_validate_func(cfg, SETTING_DEVICE_TYPE, cfg_check_device_type);
	cfg_set_validate_func(cfg, SETTING_FW_VERSION, cfg_check_fw_version);
	cfg_set_validate_func(cfg, SETTING_DC_URL, cfg_check_dc_url);
	cfg_set_validate_func(cfg, SETTING_KEEPALIVE_RX, cfg_check_keepalive_rx);
	cfg_set_validate_func(cfg, SETTING_KEEPALIVE_TX, cfg_check_keepalive_tx);
	cfg_set_validate_func(cfg, SETTING_WAIT_TIMES, cfg_check_wait_times);
	cfg_set_validate_func(cfg, SETTING_SYS_MON_SAMPLE_RATE,
			cfg_check_int_positive);
	cfg_set_validate_func(cfg, SETTING_SYS_MON_UPLOAD_SIZE,
			cfg_check_int_positive);

	/* Parse the configuration file. */
	switch (cfg_parse(cfg, filename)) {
	case CFG_FILE_ERROR:
		log_error("Configuration file '%s' could not be read: %s\n", filename,
				strerror(errno));
		return -1;
	case CFG_SUCCESS:
		break;
	case CFG_PARSE_ERROR:
		log_error("Error parsing configuration file '%s'\n", filename);
		return -1;
	}

	return fill_connector_config(cc_cfg);
}

/*
 * close_configuration() - Close configuration and free internal vars
 *
 * Note that after calling this method, the configuration must be parsed again
 * from the configuration file using 'parse_configuration()' method before
 * trying to use any other configuration function.
 */
void close_configuration(void)
{
	cfg_free(cfg);
}

/*
 * free_configuration() - Release the configuration var
 *
 * @cc_cfg:	General configuration struct (cc_cfg_t) holding the
 * 			current connector configuration.
 */
void free_configuration(cc_cfg_t *cc_cfg)
{
	if (cc_cfg != NULL) {
		unsigned int i;

		free(cc_cfg->device_type);
		cc_cfg->device_type = NULL;
		free(cc_cfg->fw_version);
		cc_cfg->fw_version = NULL;
		free(cc_cfg->url);
		cc_cfg->url = NULL;

		for (i = 0; i < cc_cfg->n_vdirs; i++) {
			cc_cfg->vdirs[i].name = NULL;
			cc_cfg->vdirs[i].path = NULL;
		}
		free(cc_cfg->vdirs);
		cc_cfg->vdirs = 0;

		free(cc_cfg);
		cc_cfg = NULL;
	}
}

/*
 * get_configuration() - Retrieve current connector configuration
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) that will hold
 * 			the current connector configuration.
 *
 * Return: 0 if the configuration is retrieved successfully, -1 otherwise.
 */
int get_configuration(cc_cfg_t *cc_cfg)
{
	/* Check if configuration is initialized. */
	if (cfg == NULL) {
		log_error("%s", "Configuration is not initialized");
		return -1;
	}

	return fill_connector_config(cc_cfg);
}

/*
 * save_configuration() - Save the given connector configuration
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) containing
 *			the connector settings to save.
 *
 * Return: 0 if the configuration is saved successfully, -1 otherwise.
 */
int save_configuration(cc_cfg_t *cc_cfg)
{
	FILE *fp;

	/* Check if configuration is initialized. */
	if (cfg == NULL) {
		log_error("%s", "Configuration is not initialized");
		goto error;
	}

	/* Set the current configuration. */
	if (set_connector_config(cc_cfg)) {
		log_error("%s", "Error setting configuration values");
		goto error;
	}

	/* Write configuration to file. */
	fp = fopen(cfg_path, "w+");
	if (fp == NULL) {
		log_error("Error opening configuration file to write '%s'", cfg_path);
		goto error;
	}
	cfg_print(cfg, fp);
	fclose(fp);

	return 0;

error:
	return -1;
}

/*
 * fill_connector_config() - Fill the connector configuration struct
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration will be saved.
 *
 * Return: 0 if the configuration is filled successfully, -1 otherwise.
 */
static int fill_connector_config(cc_cfg_t *cc_cfg)
{
	/* Fill general settings. */
	cc_cfg->vendor_id = cfg_getint(cfg, SETTING_VENDOR_ID);
	if (check_vendor_id(cc_cfg->vendor_id) != 0)
		return -1;
	cc_cfg->device_type = strdup(cfg_getstr(cfg, SETTING_DEVICE_TYPE));
	if (cc_cfg->device_type == NULL)
		return -1;
	cc_cfg->fw_version = strdup(cfg_getstr(cfg, SETTING_FW_VERSION));
	if (cc_cfg->fw_version == NULL)
		return -1;

	/* Fill connection settings. */
	cc_cfg->url = strdup(cfg_getstr(cfg, SETTING_DC_URL));
	if (cc_cfg->url == NULL)
		return -1;
	cc_cfg->enable_reconnect = cfg_getbool(cfg, SETTING_ENABLE_RECONNECT);
	cc_cfg->keepalive_rx = cfg_getint(cfg, SETTING_KEEPALIVE_RX);
	cc_cfg->keepalive_tx = cfg_getint(cfg, SETTING_KEEPALIVE_TX);
	cc_cfg->wait_count = cfg_getint(cfg, SETTING_WAIT_TIMES);

	/* Fill services settings. */
	cc_cfg->services = 0;
	if (cfg_getbool(cfg, ENABLE_FS_SERVICE)) {
		cc_cfg->services = cc_cfg->services | FS_SERVICE;
		get_virtual_directories(cfg, cc_cfg);
	}
	if (cfg_getbool(cfg, ENABLE_DATA_SERVICE)) {
		cc_cfg->services = cc_cfg->services | DATA_SERVICE;
		if (cfg_getbool(cfg, ENABLE_SYSTEM_MONITOR))
			cc_cfg->services = cc_cfg->services | SYS_MONITOR_SERVICE;
	}

	/* Fill system monitor settings. */
	cc_cfg->sys_mon_parameters = 0;
	if (cfg_getbool(cfg, ENABLE_SYS_MON_MEMORY))
		cc_cfg->sys_mon_parameters = cc_cfg->sys_mon_parameters | SYS_MON_MEMORY;
	if (cfg_getbool(cfg, ENABLE_SYS_MON_LOAD))
		cc_cfg->sys_mon_parameters = cc_cfg->sys_mon_parameters | SYS_MON_LOAD;
	if (cfg_getbool(cfg, ENABLE_SYS_MON_TEMP))
		cc_cfg->sys_mon_parameters = cc_cfg->sys_mon_parameters | SYS_MON_TEMP;

	cc_cfg->sys_mon_sample_rate = cfg_getint(cfg, SETTING_SYS_MON_SAMPLE_RATE);
	cc_cfg->sys_mon_num_samples_upload = cfg_getint(cfg, SETTING_SYS_MON_UPLOAD_SIZE);

	/* Fill logging settings. */
	cc_cfg->log_level = get_log_level();
	cc_cfg->log_console = cfg_getbool(cfg, SETTING_LOG_CONSOLE);

	return 0;
}

/*
 * set_connector_config() - Sets the connector configuration as current
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) containing the
 * 			connector configuration.
 *
 * Return: 0 if the configuration is set successfully, -1 otherwise.
 */
static int set_connector_config(cc_cfg_t *cc_cfg)
{
	/* Set general settings. */
	if (check_vendor_id(cc_cfg->vendor_id) != 0)
		return -1;
	cfg_setint(cfg, SETTING_VENDOR_ID, cc_cfg->vendor_id);
	cfg_setstr(cfg, SETTING_DEVICE_TYPE, cc_cfg->device_type);
	cfg_setstr(cfg, SETTING_FW_VERSION, cc_cfg->fw_version);

	/* Fill connection settings. */
	cfg_setstr(cfg, SETTING_DC_URL, cc_cfg->url);
	cfg_setbool(cfg, SETTING_ENABLE_RECONNECT, cc_cfg->enable_reconnect);
	cfg_setint(cfg, SETTING_KEEPALIVE_RX, cc_cfg->keepalive_rx);
	cfg_setint(cfg, SETTING_KEEPALIVE_TX, cc_cfg->keepalive_tx);
	cfg_setint(cfg, SETTING_WAIT_TIMES, cc_cfg->wait_count);

	/* Fill services settings. */
	cfg_setbool(cfg, ENABLE_FS_SERVICE, cc_cfg->services & FS_SERVICE ? cfg_true : cfg_false);
	cfg_setbool(cfg, ENABLE_DATA_SERVICE, cc_cfg->services & DATA_SERVICE ? cfg_true : cfg_false);
	cfg_setbool(cfg, ENABLE_SYSTEM_MONITOR, cc_cfg->services & SYS_MONITOR_SERVICE ? cfg_true : cfg_false);
	/* TODO: Set virtual directories */

	/* Fill system monitor settings. */
	cfg_setbool(cfg, ENABLE_SYS_MON_MEMORY, cc_cfg->sys_mon_parameters & SYS_MON_MEMORY ? cfg_true : cfg_false);
	cfg_setbool(cfg, ENABLE_SYS_MON_LOAD, cc_cfg->sys_mon_parameters & SYS_MON_LOAD ? cfg_true : cfg_false);
	cfg_setbool(cfg, ENABLE_SYS_MON_TEMP, cc_cfg->sys_mon_parameters & SYS_MON_TEMP ? cfg_true : cfg_false);
	cfg_setint(cfg, SETTING_SYS_MON_SAMPLE_RATE, cc_cfg->sys_mon_sample_rate);
	cfg_setint(cfg, SETTING_SYS_MON_UPLOAD_SIZE, cc_cfg->sys_mon_num_samples_upload);

	/* Fill logging settings. */
	switch (cc_cfg->log_level) {
	case LOG_LEVEL_DEBUG:
		cfg_setstr(cfg, SETTING_LOG_LEVEL, LOG_LEVEL_DEBUG_STR);
		break;
	case LOG_LEVEL_INFO:
		cfg_setstr(cfg, SETTING_LOG_LEVEL, LOG_LEVEL_INFO_STR);
		break;
	default:
		cfg_setstr(cfg, SETTING_LOG_LEVEL, LOG_LEVEL_ERROR_STR);
		break;
	}
	cfg_setbool(cfg, SETTING_LOG_CONSOLE, cc_cfg->log_console);

	return 0;
}

/*
 * cfg_check_vendor_id() - Validate vendor_id in the configuration file
 *
 * @cfg:	The section where the vendor_id is defined.
 * @opt:	The vendor_id option.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_vendor_id(cfg_t *cfg, cfg_opt_t *opt)
{
	long int val = cfg_opt_getnint(opt, 0);

	if (val <= 0) {
		cfg_error(cfg, "Invalid %s (0x%08lx)", opt->name, val);
		return -1;
	}
	return 0;
}

/*
 * cfg_check_device_type() - Validate device_type in the configuration file
 *
 * @cfg:	The section were the device_type is defined.
 * @opt:	The device_type option.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_device_type(cfg_t *cfg, cfg_opt_t *opt)
{
	char *val = cfg_opt_getnstr(opt, 0);

	if (val == NULL || strlen(val) == 0) {
		cfg_error(cfg, "Invalid %s (%s): cannot be empty", opt->name, val);
		return -1;
	}
	if (strlen(val) > 255) {
		cfg_error(cfg, "Invalid %s (%s): maximum length 255", opt->name, val);
		return -1;
	}

	return 0;
}

/*
 * cfg_check_fw_version() - Validate firmware_version in the configuration file
 *
 * @cfg:	The section were the firmware_version is defined.
 * @opt:	The firmware_version option.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_fw_version(cfg_t *cfg, cfg_opt_t *opt)
{
	regex_t regex;
	char msgbuf[100];
	int error = 0;
	int ret;
	char *val = cfg_opt_getnstr(opt, 0);

	if (val == NULL || strlen(val) == 0) {
		cfg_error(cfg, "Invalid %s (%s): cannot be empty", opt->name, val);
		return -1;
	}

	ret = regcomp(&regex, "^([0-9]+\\.){0,3}[0-9]+$", REG_EXTENDED);
	if (ret != 0) {
		regerror(ret, &regex, msgbuf, sizeof(msgbuf));
		cfg_error(cfg, "Could not compile regex: %s (%d)", msgbuf, ret);
		error = -1;
		goto done;
	}
	ret = regexec(&regex, val, 0, NULL, 0);
	if (ret != 0) {
		regerror(ret, &regex, msgbuf, sizeof(msgbuf));
		cfg_error(cfg, "Invalid %s (%s): %s (%d)", opt->name, val, msgbuf, ret);
		error = -1;
	}

done:
	regfree(&regex);
	return error;
}

/*
 * cfg_check_dc_url() - Validate url in the configuration file
 *
 * @cfg:	The section were the url is defined.
 * @opt:	The url option.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_dc_url(cfg_t *cfg, cfg_opt_t *opt)
{
	char *val = cfg_opt_getnstr(opt, 0);
	if (val == NULL || strlen(val) == 0) {
		cfg_error(cfg, "Invalid %s (%s): cannot be empty", opt->name, val);
		return -1;
	}
	return 0;
}

/*
 * cfg_check_keepalive_rx() - Check RX keep alive value is between 5 and 7200
 *
 * @cfg:	The section were the option is defined.
 * @opt:	The option to check.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_keepalive_rx(cfg_t *cfg, cfg_opt_t *opt)
{
	return cfg_check_range(cfg, opt, CCAPI_KEEPALIVES_RX_MIN, CCAPI_KEEPALIVES_RX_MAX);
}

/*
 * cfg_check_keepalive_tx() - Check TX keep alive value is between 5 and 7200
 *
 * @cfg:	The section were the option is defined.
 * @opt:	The option to check.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_keepalive_tx(cfg_t *cfg, cfg_opt_t *opt)
{
	return cfg_check_range(cfg, opt, CCAPI_KEEPALIVES_TX_MIN, CCAPI_KEEPALIVES_TX_MAX);
}

/*
 * cfg_check_wait_times() - Check wait time value is between 2 and 64
 *
 * @cfg:	The section were the option is defined.
 * @opt:	The option to check.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_wait_times(cfg_t *cfg, cfg_opt_t *opt)
{
	return cfg_check_range(cfg, opt, CCAPI_KEEPALIVES_WCNT_MIN, CCAPI_KEEPALIVES_WCNT_MAX);
}

/*
 * cfg_check_range() - Check a parameter value is between given range
 *
 * @cfg:	The section were the option is defined.
 * @opt:	The option to check.
 * @min:	Minimum value of the parameter.
 * @max:	Maximum value of the parameter.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_range(cfg_t *cfg, cfg_opt_t *opt, uint16_t min, uint16_t max)
{
	long int val = cfg_opt_getnint(opt, 0);

	if (val > max || val < min) {
		cfg_error(cfg, "Invalid %s (%s): value must be between %d and %d", opt->name, val, min, max);
		return -1;
	}
	return 0;
}

/*
 * cfg_check_int_positive() - Check a value is positive (value >= 0)
 *
 * @cfg:	The section were the option is defined.
 * @opt:	The option to check.
 *
 * @Return: 0 on success, any other value otherwise.
 */
static int cfg_check_int_positive(cfg_t *cfg, cfg_opt_t *opt)
{
	long int val = cfg_opt_getnint(opt, 0);

	if (val <= 0) {
		cfg_error(cfg, "Invalid %s (%ld): must be greater than 0", opt->name, val);
		return -1;
	}
	return 0;
}

/*
 * check_vendor_id() - Validate the given Vendor ID
 *
 * @value:	The Vendor ID value.
 *
 * @Return: 0 for a valid value, -1 otherwise.
 */
static int check_vendor_id(long int value)
{
	if (value <= 0) {
		log_error("Invalid %s (0x%08lx)", SETTING_VENDOR_ID, value);
		return -1;
	}
	return 0;
}

/*
 * get_virtual_directories() - Get the list of virtual directories
 *
 * @cfg:	Configuration struct from config file to read the virtual directories
 * @cc_cfg:	Cloud Connector configuration to store the virtual directories
 *
 * @Return: 0 on success, any other value otherwise.
 */
static void get_virtual_directories(cfg_t *const cfg, cc_cfg_t *const cc_cfg)
{
	vdir_t *vdirs = NULL;
	int vdirs_num;
	int i;

	cfg_t *virtual_dir_cfg = cfg_getsec(cfg, GROUP_VIRTUAL_DIRS);

	vdirs_num = cfg_size(virtual_dir_cfg, GROUP_VIRTUAL_DIR);

	vdirs = malloc(sizeof(vdir_t) * vdirs_num);
	if (vdirs == NULL) {
		log_info("%s", "Cannot initialize virtual directories");
		cc_cfg->n_vdirs = 0;
		cc_cfg->vdirs = NULL;
		return;
	}

	for (i = 0; i < vdirs_num; i++) {
		vdir_t vdir;
		cfg_t *vdir_cfg = cfg_getnsec(virtual_dir_cfg, GROUP_VIRTUAL_DIR, i);

		vdir.name = cfg_getstr(vdir_cfg, SETTING_NAME);
		vdir.path = cfg_getstr(vdir_cfg, SETTING_PATH);
		vdirs[i] = vdir;
	}
	cc_cfg->n_vdirs = vdirs_num;
	cc_cfg->vdirs = vdirs;
}

/*
 * get_log_level() - Get the log level setting value
 *
 * @Return: The log level value.
 */
static int get_log_level(void)
{
	char *level = cfg_getstr(cfg, SETTING_LOG_LEVEL);
	if (level == NULL || strlen(level) == 0)
		return LOG_LEVEL_ERROR;
	if (strcmp(level, LOG_LEVEL_DEBUG_STR) == 0)
		return LOG_LEVEL_DEBUG;
	if (strcmp(level, LOG_LEVEL_INFO_STR) == 0)
		return LOG_LEVEL_INFO;
	return LOG_LEVEL_ERROR;
}

/**
 * file_exists() - Check that the file with the given name exists
 *
 * @filename:	Full path of the file to check if it exists.
 *
 * Return: 1 if the file exits, 0 if it does not exist.
 */
static int file_exists(const char *const filename)
{
	return access(filename, F_OK) == 0;
}

/**
 * file_readable() - Check that the file with the given name can be read
 *
 * @filename:	Full path of the file to check if it is readable.
 *
 * Return: 1 if the file is readable, 0 if it cannot be read.
 */
static int file_readable(const char *const filename)
{
	return access(filename, R_OK) == 0;
}
