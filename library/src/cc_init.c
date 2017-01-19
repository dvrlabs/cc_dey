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

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "cc_init.h"
#include "cc_logging.h"
#include "cc_device_request.h"
#include "cc_firmware_update.h"
#include "cc_system_monitor.h"
#include "network_utils.h"
#include "rci_config.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(a)			((void)(a))
#endif

#define DEVICE_ID_FORMAT	"%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX"

#define CC_CONFIG_FILE		"/etc/cc.conf"

#define FW_SWU_CHUNK_SIZE	128 * 1024 /* 128KB, CC6UL flash sector size */

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static ccapi_start_t *create_ccapi_start_struct(const cc_cfg_t *const cc_cfg);
static ccapi_tcp_info_t *create_ccapi_tcp_start_info_struct(const cc_cfg_t *const cc_cfg);
static ccapi_start_error_t initialize_ccapi(const cc_cfg_t *const cc_cfg);
static ccapi_tcp_start_error_t initialize_tcp_transport(const cc_cfg_t *const cc_cfg);
static void free_ccapi_start_struct(ccapi_start_t *ccapi_start);
static void free_ccapi_tcp_start_info_struct(ccapi_tcp_info_t *const tcp_info);
static int add_virtual_directories(const vdir_t *const vdirs, int n_vdirs);
static ccapi_bool_t tcp_reconnect_cb(ccapi_tcp_close_cause_t cause);
static void *reconnect_threaded(void *unused);
static void reconnect_cleanup_handler(void *p_tcp_info);
static int get_device_id_from_mac(uint8_t *const device_id,
		const uint8_t *const mac_addr);
static uint32_t fw_string_to_int(const char *fw_string);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
extern ccapi_rci_data_t const ccapi_rci_data;
extern connector_remote_config_data_t rci_internal_data;
static ccapi_rci_service_t rci_service;
static pthread_t reconnect_thread;
cc_cfg_t *cc_cfg = NULL;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
/*
 * init_cloud_connection() - Initialize Cloud connection
 *
 * @config_file:	Absolute path of the configuration file to use. NULL to use
 * 					the default one (/etc/cc.conf).
 *
 * Return:	0 if Cloud connection is successfully initialized, error code
 *			otherwise.
 */
cc_init_error_t init_cloud_connection(const char *config_file)
{
	int log_options = LOG_CONS | LOG_NDELAY | LOG_PID;
	ccapi_start_error_t ccapi_error;
	int error;

	cc_cfg = calloc(1, sizeof(cc_cfg_t));
	if (cc_cfg == NULL) {
		log_error("Cannot allocate memory for configuration (errno %d: %s)",
				errno, strerror(errno));
		return CC_INIT_ERROR_INSUFFICIENT_MEMORY;
	}

	error = parse_configuration(config_file ? config_file : CC_CONFIG_FILE, cc_cfg);
	if (error != 0)
		return CC_INIT_ERROR_PARSE_CONFIGURATION;

	closelog();
	if (cc_cfg->log_console)
		log_options = log_options | LOG_PERROR;
	init_logger(cc_cfg->log_level, log_options);

	ccapi_error = initialize_ccapi(cc_cfg);
	if (ccapi_error != CCAPI_START_ERROR_NONE)
		return ccapi_error;

	error = add_virtual_directories(cc_cfg->vdirs, cc_cfg->n_vdirs);
	if (error != 0)
		return CC_INIT_ERROR_ADD_VIRTUAL_DIRECTORY;

	return CC_INIT_ERROR_NONE;
}

/*
 * start_cloud_connection() - Start Cloud connection
 *
 * Return:	0 if Cloud connection is successfully started, error code otherwise.
 */
cc_start_error_t start_cloud_connection(void)
{
	ccapi_tcp_start_error_t tcp_start_error;
	cc_sys_mon_error_t sys_mon_error;

	if (cc_cfg == NULL) {
		log_error("%s", "Initialize the connection before starting\n");
		return CC_START_ERROR_NOT_INITIALIZE;
	}

	tcp_start_error = initialize_tcp_transport(cc_cfg);
	if (tcp_start_error != CCAPI_TCP_START_ERROR_NONE)
		return tcp_start_error;

	sys_mon_error = start_system_monitor(cc_cfg);
	if (sys_mon_error != CC_SYS_MON_ERROR_NONE)
		return CC_START_ERROR_SYSTEM_MONITOR;

	return CC_START_ERROR_NONE;
}

/*
 * stop_cloud_connection() - Stop Cloud connection
 *
 * Return:	0 if Cloud connection is successfully stopped, error code otherwise.
 */
cc_stop_error_t stop_cloud_connection(void)
{
	ccapi_stop_error_t stop_error;

	if (!pthread_equal(reconnect_thread, 0))
		pthread_cancel(reconnect_thread);

	stop_system_monitor();
	stop_error = ccapi_stop(CCAPI_STOP_GRACEFULLY);

	if (stop_error == CCAPI_STOP_ERROR_NONE) {
		log_info("%s", "ccapi_stop success\n");
	} else {
		log_error("ccapi_stop error %d\n", stop_error);
	}

	free_configuration(cc_cfg);
	close_configuration();
	cc_cfg = NULL;
	closelog();

	return stop_error;
}

/*
 * initialize_ccapi() - Initialize CCAPI layer
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the settings parsed
 *			from the configuration file are stored.
 *
 * Return:	CCAPI_START_ERROR_NONE on success, any other ccapi_start_error_t
 * 			otherwise.
 */
static ccapi_start_error_t initialize_ccapi(const cc_cfg_t *const cc_cfg)
{
	ccapi_start_t *start_st = NULL;
	ccapi_start_error_t error;

	start_st = create_ccapi_start_struct(cc_cfg);
	if (start_st == NULL)
		return CCAPI_START_ERROR_NULL_PARAMETER;

	error = ccapi_start(start_st);
	if (error != CCAPI_START_ERROR_NONE)
		log_error("ccapi_start error %d\n", error);

	free_ccapi_start_struct(start_st);
	return error;
}

/*
 * initialize_tcp_transport() - Start TCP transport
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the settings parsed
 * 			from the configuration file are stored.
 *
 * Return: CCAPI_TCP_START_ERROR_NONE on success, any other
 *         ccapi_tcp_start_error_t otherwise.
 */
static ccapi_tcp_start_error_t initialize_tcp_transport(
		const cc_cfg_t *const cc_cfg)
{
	ccapi_tcp_info_t *tcp_info = NULL;
	ccapi_tcp_start_error_t error;

	tcp_info = create_ccapi_tcp_start_info_struct(cc_cfg);
	if (tcp_info == NULL)
		return CCAPI_TCP_START_ERROR_NULL_POINTER;

	error = ccapi_start_transport_tcp(tcp_info);
	while (error == CCAPI_TCP_START_ERROR_TIMEOUT) {
		log_info("Time out, retrying connection in %d seconds",
				cc_cfg->reconnect_time);
		sleep(cc_cfg->reconnect_time);
		error = ccapi_start_transport_tcp(tcp_info);
	}

	if (error) {
		log_error("ccapi_start_transport_tcp failed with error %d\n", error);
	}

	free_ccapi_tcp_start_info_struct(tcp_info);
	return error;
}

/*
 * create_ccapi_start_struct() - Create a ccapi_start_t struct from the given config
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * Return:	The created ccapi_start_t struct with the data read from the
 * 			configuration file.
 */
static ccapi_start_t *create_ccapi_start_struct(const cc_cfg_t *const cc_cfg)
{
	uint8_t mac_address[6];

	ccapi_start_t *start = malloc(sizeof *start);
	if (start == NULL) {
		log_error("%s","create_ccapi_start_struct(): malloc failed for ccapi_start_t");
		return start;
	}

	start->device_cloud_url = cc_cfg->url;
	start->device_type = cc_cfg->device_type;
	start->vendor_id = cc_cfg->vendor_id;
	if (get_device_id_from_mac(start->device_id, get_mac_addr(mac_address)) != 0) {
		log_error("%s", "create_ccapi_start_struct(): cannot calculate device id");
		free_ccapi_start_struct(start);
		start = NULL;
		return start;
	}

	start->status = NULL;

	/* Initialize CLI service. */
	start->service.cli = NULL;

	/* Initialize RCI service. */
	rci_service.rci_data = &ccapi_rci_data;
	start->service.rci = &rci_service;
	rci_internal_data.firmware_target_zero_version = fw_string_to_int(cc_cfg->fw_version);
	rci_internal_data.vendor_id = cc_cfg->vendor_id;
	rci_internal_data.device_type = cc_cfg->device_type;

	/* Initialize device request service . */
	if (cc_cfg->services & DATA_SERVICE) {
		ccapi_receive_service_t *dreq_service = malloc(sizeof *dreq_service);
		if (dreq_service == NULL) {
			log_error("%s", "create_ccapi_start_struct(): malloc failed for ccapi_receive_service_t");
			free_ccapi_start_struct(start);
			start = NULL;
			return start;
		}
		dreq_service->accept = app_receive_default_accept_cb;
		dreq_service->data = app_receive_default_data_cb;
		dreq_service->status = app_receive_default_status_cb;
		start->service.receive = dreq_service;
	} else {
		start->service.receive = NULL;
	}

	/* Initialize short messaging. */
	start->service.sm = NULL;

	/* Initialize file system service. */
	if (cc_cfg->services & FS_SERVICE) {
		ccapi_filesystem_service_t *fs_service = malloc(sizeof *fs_service);
		if (fs_service == NULL) {
			log_error("%s", "create_ccapi_start_struct(): malloc failed for ccapi_filesystem_service_t");
			free_ccapi_start_struct(start);
			start = NULL;
			return start;
		}
		fs_service->access = NULL;
		fs_service->changed = NULL;
		start->service.file_system = fs_service;
	} else {
		start->service.file_system = NULL;
	}

	/* Initialize firmware service. */
	start->service.firmware = NULL;
	if (cc_cfg->fw_version == NULL) {
		start->service.firmware = NULL;
	} else {
		unsigned int fw_version_major;
		unsigned int fw_version_minor;
		unsigned int fw_version_revision;
		unsigned int fw_version_build;
		int error;

		error = sscanf(cc_cfg->fw_version, "%u.%u.%u.%u", &fw_version_major,
				&fw_version_minor, &fw_version_revision, &fw_version_build);
		if (error != 4) {
			log_error("Bad firmware_version string '%s', firmware update disabled",
					cc_cfg->fw_version);
			start->service.firmware = NULL;
		} else {
			uint8_t n_targets = 2;
			ccapi_firmware_target_t *fw_list = NULL;
			ccapi_fw_service_t *fw_service = NULL;

			fw_list = malloc(n_targets * sizeof *fw_list);
			if (fw_list == NULL) {
				log_error("%s", "create_ccapi_start_struct(): malloc failed for ccapi_firmware_target_t");
				free_ccapi_start_struct(start);
				start = NULL;
				return start;
			}

			fw_service = malloc(sizeof *fw_service);
			if (fw_service == NULL) {
				log_error("%s", "create_ccapi_start_struct(): malloc failed for ccapi_fw_service_t");
				free(fw_list);
				free_ccapi_start_struct(start);
				start = NULL;
				return start;
			}

			fw_list[0].chunk_size = FW_SWU_CHUNK_SIZE;
			fw_list[0].description = "System";
			fw_list[0].filespec = ".*\\.[sS][wW][uU]";
			fw_list[0].maximum_size = 0;
			fw_list[0].version.major = (uint8_t) fw_version_major;
			fw_list[0].version.minor = (uint8_t) fw_version_minor;
			fw_list[0].version.revision = (uint8_t) fw_version_revision;
			fw_list[0].version.build = (uint8_t) fw_version_build;

			fw_list[1].chunk_size = 0;
			fw_list[1].description = "Update manifest";
			fw_list[1].filespec = "[mM][aA][nN][iI][fF][eE][sS][tT]\\.[tT][xX][tT]";
			fw_list[1].maximum_size = 0;
			fw_list[1].version.major = (uint8_t) fw_version_major;
			fw_list[1].version.minor = (uint8_t) fw_version_minor;
			fw_list[1].version.revision = (uint8_t) fw_version_revision;
			fw_list[1].version.build = (uint8_t) fw_version_build;

			fw_service->target.count = n_targets;
			fw_service->target.item = fw_list;

			fw_service->callback.request = app_fw_request_cb;
			fw_service->callback.data = app_fw_data_cb;
			fw_service->callback.reset = app_fw_reset_cb;
			fw_service->callback.cancel = app_fw_cancel_cb;

			start->service.firmware = fw_service;
		}
	}

	return start;
}

/*
 * create_ccapi_tcp_start_info_struct() - Generate a ccapi_tcp_info_t struct
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * Return: The created ccapi_start_t struct with the data read from the
 *         configuration file.
 */
static ccapi_tcp_info_t *create_ccapi_tcp_start_info_struct(const cc_cfg_t *const cc_cfg)
{
	ccapi_tcp_info_t *tcp_info = malloc(sizeof *tcp_info);
	iface_info_t active_interface;
	if (tcp_info == NULL) {
		log_error("%s", "create_ccapi_tcp_start_info_struct(): malloc failed for ccapi_tcp_info_t");
		return tcp_info;
	}

	tcp_info->callback.close = NULL;
	tcp_info->callback.close = tcp_reconnect_cb;

	tcp_info->callback.keepalive = NULL;
	tcp_info->connection.type = CCAPI_CONNECTION_LAN;
	tcp_info->connection.max_transactions = 0;
	tcp_info->connection.password = NULL;
	tcp_info->connection.start_timeout = 10;
	tcp_info->connection.ip.type = CCAPI_IPV4;

	if (get_iface_info(cc_cfg->url, &active_interface) != 0) {
		free_ccapi_tcp_start_info_struct(tcp_info);
		tcp_info = NULL;
		return tcp_info;
	} else {
		memcpy(tcp_info->connection.ip.address.ipv4,
				active_interface.ipv4_addr,
				sizeof(tcp_info->connection.ip.address.ipv4));
		memcpy(tcp_info->connection.info.lan.mac_address,
				active_interface.mac_addr,
				sizeof(tcp_info->connection.info.lan.mac_address));
	}

	tcp_info->keepalives.rx = cc_cfg->keepalive_rx;
	tcp_info->keepalives.tx = cc_cfg->keepalive_tx;
	tcp_info->keepalives.wait_count = cc_cfg->wait_count;

	return tcp_info;
}

/**
 * free_ccapi_start_struct() - Release the ccapi_start_t struct
 *
 * @ccapi_start:	CCAPI start struct (ccapi_start_t) to be released.
 */
static void free_ccapi_start_struct(ccapi_start_t *ccapi_start)
{
	if (ccapi_start != NULL) {
		if (ccapi_start->service.firmware != NULL)
			free(ccapi_start->service.firmware->target.item);

		free(ccapi_start->service.firmware);
		free(ccapi_start->service.file_system);
		free(ccapi_start->service.receive);
		free(ccapi_start);
	}
}

/**
 * free_ccapi_tcp_start_info_struct() - Release the ccapi_tcp_info_t struct
 *
 * @ccapi_start:	CCAPI TCP info struct (ccapi_tcp_info_t) to be released.
 */
static void free_ccapi_tcp_start_info_struct(ccapi_tcp_info_t *const tcp_info)
{
	free(tcp_info);
}

/**
 * add_virtual_directories() - Add defined virtual directories
 *
 * @vdirs:		List of virtual directories
 * @n_vdirs:	Number of elements in the list
 *
 * Return: 0 on success, -1 otherwise.
 */
static int add_virtual_directories(const vdir_t *const vdirs, int n_vdirs)
{
	int error = 0;
	int i;

	for (i = 0; i < n_vdirs; i++) {
		ccapi_fs_error_t add_dir_error;
		const vdir_t *v_dir = vdirs + i;

		log_info("New virtual directory %s (%s)", v_dir->name, v_dir->path);
		add_dir_error = ccapi_fs_add_virtual_dir(v_dir->name, v_dir->path);
		if (add_dir_error != CCAPI_FS_ERROR_NONE) {
			error = -1;
			log_error("add_virtual_directories() failed with error %d", add_dir_error);
		}
	}

	return error;
}

/**
 * tcp_close_cb() - Callback to tell if Cloud Connector should reconnect
 *
 * @cause:	Reason of the disconnection (disconnection, redirection, missing
 * 			keep alive, or any other data error).
 *
 * Return: CCAPI_TRUE if Cloud Connector should reconnect, CCAPI_FALSE otherwise.
 */
static ccapi_bool_t tcp_reconnect_cb(ccapi_tcp_close_cause_t cause)
{
	log_debug("Reconnection, cause %d", cause);

	if (!pthread_equal(reconnect_thread, 0)
			&& !pthread_kill(reconnect_thread, 0))
		return CCAPI_FALSE;

	if (cc_cfg->enable_reconnect) {
		pthread_attr_t attr;
		int error;

		/* Always return CCAPI_FALSE, to "manually" start the connection again
		 * in another thread after the configured timeout.
		 * Do not return CCAPI_TRUE, it will immediately and automatically
		 * connect again (without any kind of timeout).
		 */

		error = pthread_attr_init(&attr);
		if (error != 0) {
			/* On Linux this function always succeeds. */
			log_error(
					"Unable to reconnect, cannot create reconnect thread: pthread_attr_init() error %d",
					error);
			return CCAPI_FALSE;
		}

		error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (error != 0) {
			log_error(
					"Unable to reconnect, cannot create reconnect thread: pthread_attr_setdetachstate() error %d",
					error);
			return CCAPI_FALSE;
		}

		error = pthread_create(&reconnect_thread, NULL, reconnect_threaded,
				NULL);
		if (error != 0) {
			log_error(
					"Unable to reconnect, cannot create reconnect thread: pthread_create() error %d",
					error);
			return CCAPI_FALSE;
		}
	}
	return CCAPI_FALSE;
}

/*
 * reconnect_threaded() - Perform a manual reconnection in a new thread
 *
 * @unused:	Unused parameter.
 */
static void *reconnect_threaded(void *unused)
{
	ccapi_tcp_info_t *tcp_info = NULL;
	ccapi_tcp_start_error_t error;

	UNUSED_PARAMETER(unused);

	pthread_cleanup_push(reconnect_cleanup_handler, (void *)&tcp_info);

	do {
		int seconds;

		log_info("Reconnecting in %d seconds", cc_cfg->reconnect_time);

		for (seconds = 0; seconds < cc_cfg->reconnect_time; seconds++) {
			pthread_testcancel();
			sleep(1);
		}

		pthread_testcancel();
		if (tcp_info == NULL) {
			tcp_info = create_ccapi_tcp_start_info_struct(cc_cfg);
			if (tcp_info == NULL)
				continue;
		}

		pthread_testcancel();
		error = ccapi_start_transport_tcp(tcp_info);
	} while (error == CCAPI_TCP_START_ERROR_TIMEOUT);

	if (error) {
		log_error(
				"Unable to reconnect: ccapi_start_transport_tcp() failed with error %d",
				error);
	}

	pthread_cleanup_pop(1);

	pthread_exit(NULL);
	return NULL;
}

/*
 * reconnect_cleanup_handler() - Clean up the reconnection thread
 *
 * @p_tcp_info:	Pointer to ccapi_tcp_info_t to free.
 */
static void reconnect_cleanup_handler(void *p_tcp_info)
{
	ccapi_tcp_info_t **tcp_info = (ccapi_tcp_info_t **)p_tcp_info;

	free_ccapi_tcp_start_info_struct(*tcp_info);
}

/*
 * get_device_id_from_mac() - Generate a Device ID from a given MAC address
 *
 * @device_id:	Pointer to store the generated Device ID.
 * @mac_addr:	MAC address to generate the Device ID.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_device_id_from_mac(uint8_t *const device_id, const uint8_t *const mac_addr)
{
	const char *const deviceid_file = "/etc/cc.did";
	unsigned int const device_id_length = 16;
	FILE *fp = NULL;
	unsigned int n_items;

	memset(device_id, 0x00, device_id_length);

	fp = fopen(deviceid_file, "rb");
	if (fp != NULL) {
		n_items = fscanf(fp, DEVICE_ID_FORMAT, &device_id[0], &device_id[1],
				&device_id[2], &device_id[3], &device_id[4], &device_id[5],
				&device_id[6], &device_id[7], &device_id[8], &device_id[9],
				&device_id[10], &device_id[11], &device_id[12], &device_id[13],
				&device_id[14], &device_id[15]);
		fclose(fp);
		if (n_items == device_id_length)
			return 0;
	}

	if (mac_addr == NULL)
		return -1;

	device_id[8] = mac_addr[0];
	device_id[9] = mac_addr[1];
	device_id[10] = mac_addr[2];
	device_id[11] = 0xFF;
	device_id[12] = 0xFF;
	device_id[13] = mac_addr[3];
	device_id[14] = mac_addr[4];
	device_id[15] = mac_addr[5];

	fp = fopen(deviceid_file, "wb+");
	if (fp != NULL) {
		n_items = fprintf(fp, DEVICE_ID_FORMAT, device_id[0], device_id[1],
				device_id[2], device_id[3], device_id[4], device_id[5],
				device_id[6], device_id[7], device_id[8], device_id[9],
				device_id[10], device_id[11], device_id[12], device_id[13],
				device_id[14], device_id[15]);
		fclose(fp);
		if (n_items != device_id_length * 2 + 3)
			log_debug("%s", "Could not store Device ID");
	}

	return 0;
}

/**
 * fw_string_to_int() - Convert firmware version string into uint32_t
 *
 * @fw_wstring:		Firmware version string
 *
 * Return: The firmware version as a uint32_t
 */
static uint32_t fw_string_to_int(const char *fw_string)
{
	unsigned int fw_version[4] = {0};
	uint32_t fw_int = 0;
	int num_parts;

	num_parts = sscanf(fw_string, "%u.%u.%u.%u", &fw_version[0],
			&fw_version[1], &fw_version[2], &fw_version[3]);
	if (num_parts == 0 || num_parts > 4)
		fw_int = 0;
	else
		fw_int = (fw_version[0] << 24) |
			 (fw_version[1] << 16) |
			 (fw_version[2] << 8)  |
			  fw_version[3];

	return fw_int;
}
