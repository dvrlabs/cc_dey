/*
 * Copyright (c) 2017-2022 Digi International Inc.
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
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

#include <errno.h>
#include <json-c/json_object_iterator.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <libdigiapix/bluetooth.h>
#include <libdigiapix/gpio.h>
#include <libdigiapix/process.h>
#include <libdigiapix/network.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

#include "device_request.h"
#include "file_utils.h"
#include "network_utils.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define TARGET_DEVICE_INFO		"device_info"
#define TARGET_GET_TIME			"get_time"
#define TARGET_PLAY_MUSIC		"play_music"
#define TARGET_SET_VOLUME		"set_audio_volume"
#define TARGET_STOP_CC			"stop_cc"
#define TARGET_USER_LED			"user_led"

#define RESPONSE_ERROR			"ERROR"
#define RESPONSE_OK				"OK"

#define USER_LED_ALIAS			"USER_LED"

#define DEVREQ_TAG				"DEVREQ:"

#define MAX_RESPONSE_SIZE			512

#define EMMC_SIZE_FILE				"/sys/class/mmc_host/mmc0/mmc0:0001/block/mmcblk0/size"
#define NAND_SIZE_FILE				"/proc/mtd"
#define RESOLUTION_FILE				"/sys/class/graphics/fb0/modes"
#define RESOLUTION_FILE_CCMP		"/sys/class/drm/card0/card0-DPI-1/modes"
#define RESOLUTION_FILE_CCMP_HDMI	"/sys/class/drm/card0/card0-HDMI-A-1/modes"

#define CMD_PLAY_MUSIC		"setsid mpg123 %s"
#define CMD_STOP_MUSIC		"pkill -KILL -f mpg123"
#define CMD_SET_VOLUME		"amixer set 'Speaker' %d%% && amixer set 'Headphone' %d%%"

#define FIELD_PLAY			"play"
#define FIELD_MUSIC_FILE	"music_file"

#if !(defined UNUSED_ARGUMENT)
#define UNUSED_ARGUMENT(a)	(void)(a)
#endif

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/*
 * log_dr_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_dr_debug(format, ...)									\
	log_debug("%s " format, DEVREQ_TAG, __VA_ARGS__)

/*
 * log_dr_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_dr_error(format, ...)									\
	log_error("%s " format, DEVREQ_TAG, __VA_ARGS__)

/**
 * get_emmc_size() - Returns the total eMMC storage size.
 *
 * Return: total size read.
 */
static long get_emmc_size(void)
{
	char data[MAX_RESPONSE_SIZE] = {0};
	long total_size = 0;

	if (read_file(EMMC_SIZE_FILE, data, MAX_RESPONSE_SIZE) <= 0)
		log_dr_error("%s", "Error getting storage size: Could not read file");
	if (sscanf(data, "%ld", &total_size) < 1)
		log_dr_error("%s", "Error getting storage size: Invalid file contents");

	return total_size * 512 / 1024; /* kB */
}

/**
 * get_nand_size() - Returns the total NAND storage size.
 *
 * Return: total size read.
 */
static long get_nand_size(void)
{
	char buffer[MAX_RESPONSE_SIZE] = {0};
	long total_size = 0;
	FILE *fd;

	fd = fopen(NAND_SIZE_FILE, "r");
	if (!fd) {
		log_dr_error("%s", "Error getting storage size: Could not open file");
		return total_size;
	}
	/* Ignore first line */
	if (fgets(buffer, sizeof(buffer), fd) == NULL) {
		log_dr_error("%s", "Error getting storage size: Could not read file");
		fclose(fd);
		return total_size;
	}
	/* Start reading line by line */
	while (fgets(buffer, sizeof(buffer), fd)) {
		char partition_id[20] = {'\0'};
		char partition_name[20] = {'\0'};
		char size_hex[20] = {'\0'};
		char erase_size_hex[20] = {'\0'};
		unsigned long size;

		sscanf(buffer,
			"%s %s %s %s",
			partition_id,
			size_hex,
			erase_size_hex,
			partition_name);

		size = strtol(size_hex, NULL, 16);
		total_size = total_size + size;
	}
	if (ferror(fd))
		log_dr_error("%s", "Error getting storage size: File read error");
	fclose(fd);

	return total_size / 1024; /* kB */
}

/*
 * device_info_cb() - Data callback for 'device_info' device requests
 *
 * @target:					Target ID of the device request (device_info).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
static ccapi_receive_error_t device_info_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	json_object *root = NULL;
	char *response = NULL;
	ccapi_receive_error_t status = CCAPI_RECEIVE_ERROR_NONE;

	UNUSED_ARGUMENT(request_buffer_info);

	log_dr_debug("%s: target='%s' - transport='%d'", __func__, target, transport);

	/*
		target "device_info"

		Request: -

		Response:
		{
			"total_st": 0,
			"total_mem": 2002120,
			"resolution": "1920x1080p-0",
			"bt-mac": "00:40:9d:7d:1b:8f",
			"lo": {
				"mac": "00:00:00:00:00:00",
				"ip": "127.0.0.1"
			},
			"eth0": {
				"mac": "00:40:9d:ee:b6:96",
				"ip": "192.168.1.44"
			},
			"can0": {
				"mac": "00:00:00:00:00:00",
				"ip": "0.0.0.0"
			},
			"wlan0": {
				"mac": "00:40:9d:dd:bd:13",
				"ip": "0.0.0.0"
			}
		}
	*/

	root = json_object_new_object();
	if (!root) {
		log_dr_error("Cannot generate response for target '%s': Out of memory", target);
		return CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
	}

	{
		long total_st = 0;

		/* Check first emmc, because '/proc/mtd' may exists although empty */
		if (file_readable(EMMC_SIZE_FILE))
			total_st = get_emmc_size();
		else if (file_readable(NAND_SIZE_FILE))
			total_st = get_nand_size();
		else
			log_dr_error("%s", "Error getting storage size: File not readable");

		if (json_object_object_add(root, "total_st", json_object_new_int64(total_st)) < 0) {
			log_dr_error("Cannot generate response for target '%s': Out of memory", target);
			status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
			goto done;
		}
	}

	{
		struct sysinfo s_info;
		long total_mem = -1;

		if (sysinfo(&s_info) != 0)
			log_dr_error("Error getting total memory: %s (%d)", strerror(errno), errno);
		else
			total_mem = s_info.totalram / 1024;

		if (json_object_object_add(root, "total_mem", json_object_new_int64(total_mem)) < 0) {
			log_dr_error("Cannot generate response for target '%s': Out of memory", target);
			status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
			goto done;
		}
	}

	{
		char data[MAX_RESPONSE_SIZE] = {0};
		char resolution[MAX_RESPONSE_SIZE] = {0};
		char *resolution_file = "";

		if (file_readable(RESOLUTION_FILE))
			resolution_file = RESOLUTION_FILE;
		else if (file_readable(RESOLUTION_FILE_CCMP))
			resolution_file = RESOLUTION_FILE_CCMP;
		else if (file_readable(RESOLUTION_FILE_CCMP_HDMI))
			resolution_file = RESOLUTION_FILE_CCMP_HDMI;

		if (!file_readable(resolution_file))
			log_dr_error("%s", "Error getting video resolution: File not readable");
		else if (read_file(resolution_file, data, MAX_RESPONSE_SIZE) <= 0)
			log_dr_error("%s", "Error getting video resolution");
		else if (sscanf(data, "U:%s", resolution) < 1) {
			if (sscanf(data, "%s", resolution) < 1)
				log_dr_error("%s", "Error getting video resolution");
		}

		if (json_object_object_add(root, "resolution", json_object_new_string(resolution)) < 0) {
			log_dr_error("Cannot generate response for target '%s': Out of memory", target);
			status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
			goto done;
		}
	}

	{
		bt_state_t bt_state;
		uint8_t *mac = NULL;
		char mac_str[18];

		ldx_bt_get_state(0, &bt_state);
		mac = bt_state.mac;

		snprintf(mac_str, sizeof(mac_str), MAC_FORMAT,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		if (json_object_object_add(root, "bt-mac", json_object_new_string(mac_str)) < 0) {
			log_dr_error("Cannot generate response for target '%s': Out of memory", target);
			status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
			goto done;
		}
	}

	{
		net_names_list_t list_ifaces;
		int i;

		ldx_net_list_available_ifaces(&list_ifaces);

		for (i = 0; i < list_ifaces.n_ifaces; i++) {
			net_state_t net_state;
			uint8_t *mac = NULL, *ip = NULL;
			char mac_str[MAC_STRING_LENGTH], ip_str[IP_STRING_LENGTH];
			json_object *iface_item = json_object_new_object();

			ldx_net_get_iface_state(list_ifaces.names[i], &net_state);
			mac = net_state.mac;
			ip = net_state.ipv4;

			if (!iface_item || json_object_object_add(root, list_ifaces.names[i], iface_item) < 0) {
				log_dr_error("Cannot generate response for target '%s': Out of memory", target);
				if (iface_item)
					json_object_put(iface_item);
				status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
				goto done;
			}
			
			snprintf(mac_str, sizeof(mac_str), MAC_FORMAT,
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			snprintf(ip_str, sizeof(ip_str), IP_FORMAT,
				ip[0], ip[1], ip[2], ip[3]);
		
			if (json_object_object_add(iface_item, "mac", json_object_new_string(mac_str)) < 0) {
				log_dr_error("Cannot generate response for target '%s': Out of memory", target);
				status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
				goto done;
			}

			if (json_object_object_add(iface_item, "ip", json_object_new_string(ip_str)) < 0) {
				log_dr_error("Cannot generate response for target '%s': Out of memory", target);
				status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
				goto done;
			}
		}
	}

	response = strdup(json_object_to_json_string(root));
	if (response == NULL) {
		log_dr_error("Cannot generate response for target '%s': Out of memory", target);
		status = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
		goto done;
	}

	response_buffer_info->buffer = response;
	response_buffer_info->length = strlen(response);

	log_dr_debug("%s: response: %s (len: %zu)", __func__,
		(char *)response_buffer_info->buffer, response_buffer_info->length);

done:
	json_object_put(root);

	return status;
}

/*
 * get_time_cb() - Data callback for 'get_time' device requests
 *
 * @target:					Target ID of the device request (get_time).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
static ccapi_receive_error_t get_time_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	UNUSED_ARGUMENT(request_buffer_info);

	log_dr_debug("%s: target='%s' - transport='%d'", __func__, target, transport);

	response_buffer_info->buffer = calloc(MAX_RESPONSE_SIZE + 1, sizeof(char));
	if (response_buffer_info->buffer == NULL) {
		log_dr_error("Cannot generate response for target '%s': Out of memory", target);
		return CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
	}

	time_t t = time(NULL);
	response_buffer_info->length = snprintf(response_buffer_info->buffer,
			MAX_RESPONSE_SIZE, "Time: %s", ctime(&t));

	return CCAPI_RECEIVE_ERROR_NONE;
}

/*
 * stop_cb() - Data callback for 'stop_cc' device requests
 *
 * @target:					Target ID of the device request (stop_cc).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
static ccapi_receive_error_t stop_cb(char const *const target, ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	static char const stop_response[] = "I'll stop";

	UNUSED_ARGUMENT(request_buffer_info);

	log_dr_debug("%s: target='%s' - transport='%d'", __func__, target, transport);

	response_buffer_info->buffer = calloc(MAX_RESPONSE_SIZE + 1, sizeof(char));
	if (response_buffer_info->buffer == NULL) {
		log_dr_error("Cannot generate response for target '%s': Out of memory", target);
		return CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
	}

	response_buffer_info->length = snprintf(response_buffer_info->buffer,
			strlen(stop_response) + 1, "%s", stop_response);

	return CCAPI_RECEIVE_ERROR_NONE;
}

/*
 * update_user_led_cb() - Data callback for 'user_led' device requests
 *
 * @target:					Target ID of the device request (user_led).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
static ccapi_receive_error_t update_user_led_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	ccapi_receive_error_t ret = CCAPI_RECEIVE_ERROR_NONE;
	char *val = NULL, *error_msg = NULL;
	gpio_t *led = NULL;
	gpio_value_t led_value = GPIO_LOW;

	log_dr_debug("%s: target='%s' - transport='%d'", __func__, target, transport);

	response_buffer_info->buffer = calloc(MAX_RESPONSE_SIZE + 1, sizeof(char));
	val = calloc(request_buffer_info->length + 1, sizeof(char));
	if (response_buffer_info->buffer == NULL || val == NULL) {
		log_dr_error("Cannot generate response for target '%s': Out of memory", target);
		ret = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
		goto exit;
	}

	strncpy(val, request_buffer_info->buffer, request_buffer_info->length);
	log_dr_debug("%s=%s", target, val);

	if (strcmp(val, "true") == 0 || strcmp(val, "on") == 0 || strcmp(val, "1") == 0) {
		led_value = GPIO_HIGH;
	} else if (strcmp(val, "false") == 0 || strcmp(val, "off") == 0 || strcmp(val, "0") == 0) {
		led_value = GPIO_LOW;
	} else {
		error_msg = "Unknown LED status";
		ret = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
		goto done;
	}

	/* Request User LED GPIO */
	led = ldx_gpio_request_by_alias(USER_LED_ALIAS, GPIO_OUTPUT_LOW, REQUEST_SHARED);
	if (led == NULL) {
		error_msg = "Failed to initialize LED";
		ret = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
		goto done;
	}

	if (ldx_gpio_set_value(led, led_value) != EXIT_SUCCESS) {
		error_msg = "Failed to set LED";
		ret = CCAPI_RECEIVE_ERROR_STATUS_SESSION_ERROR;
		goto done;
	}

done:
	if (ret != CCAPI_RECEIVE_ERROR_NONE) {
		response_buffer_info->length = sprintf(response_buffer_info->buffer, "ERROR: %s", error_msg);
		log_dr_error("Cannot process request for target '%s': %s", target, error_msg);
	} else {
		response_buffer_info->length = sprintf(response_buffer_info->buffer, "OK");
	}

exit:
	ldx_gpio_free(led);
	free(val);

	return ret;
}

/*
 * play_music_cb() - Data callback for 'play_music' device requests
 *
 * @target:					Target ID of the device request (play_music).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
static ccapi_receive_error_t play_music_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	char *request = request_buffer_info->buffer, *error_msg = NULL, *resp = NULL, *music_file = NULL;
	json_object *req = NULL, *json_element = NULL;
	ccapi_receive_error_t ret = CCAPI_RECEIVE_ERROR_NONE;
	bool play = false;

	log_dr_debug("%s: target='%s' - transport='%d'", __func__, target, transport);

	if (request_buffer_info->length == 0)
		goto bad_format;

	request[request_buffer_info->length] = '\0';
	req = json_tokener_parse(request);
	if (!req)
		goto bad_format;

	/* Read the "play" value. */
	if (json_object_object_get_ex(req, FIELD_PLAY, &json_element)) {
		if (!json_object_is_type(json_element, json_type_boolean))
			goto bad_format; /* Must be a boolean field. */
		play = json_object_get_boolean(json_element);
	} else {
		goto bad_format; /* Required field. */
	}

	/* Read the "music_file" value. */
	if (play) {
		if (json_object_object_get_ex(req, FIELD_MUSIC_FILE, &json_element)) {
			if (!json_object_is_type(json_element, json_type_string))
				goto bad_format; /* Must be a string field. */
			music_file = (char *)json_object_get_string(json_element);
		} else {
			goto bad_format; /* Required field. */
		}
	}

	response_buffer_info->buffer = calloc(MAX_RESPONSE_SIZE + 1, sizeof(char));
	if (response_buffer_info->buffer == NULL) {
		error_msg = "Insufficient memory";
		ret = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
		log_dr_error("Cannot generate response for target '%s': %s", target, error_msg);
		goto done;
	}

	/* Stop any mpg123 process. Do not check for error because it will not return 0 if no music was playing. */
	ldx_process_execute_cmd(CMD_STOP_MUSIC, &resp, 2);

	/* If music is set to play, reproduce the sound. */
	if (play) {
		char *cmd = NULL;
		int cmd_len = 0;

		/* Verify that music file exists. */
		if (access(music_file, F_OK) != 0) {
			error_msg = "File does not exist";
			ret = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
			log_error("Error executing target '%s': Music file '%s' does not exist", target, music_file);
			goto done;
		}
		/* Build play command. */
		cmd_len = snprintf(NULL, 0, CMD_PLAY_MUSIC, music_file);
		cmd = calloc(cmd_len + 1, sizeof(char));
		if (cmd == NULL) {
			error_msg = "Insufficient memory";
			ret = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
			log_error("Error executing target '%s': %s", target, error_msg);
		} else {
			sprintf(cmd, CMD_PLAY_MUSIC, music_file);
			/* Do not check for error because 'setsid' always returns -15. */
			ldx_process_execute_cmd(cmd, &resp, 2);
			free(cmd);
		}
	}

	goto done;

bad_format:
	error_msg = "Invalid format";
	ret = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
	log_dr_error("Cannot parse request for target '%s': Invalid request format", target);

done:
	if (ret != CCAPI_RECEIVE_ERROR_NONE) {
		response_buffer_info->length = sprintf(response_buffer_info->buffer, "ERROR: %s", error_msg);
		log_dr_error("Cannot process request for target '%s': %s", target, error_msg);
	} else {
		response_buffer_info->length = sprintf(response_buffer_info->buffer, "OK");
	}

	/* Free resources. */
	free(resp);
	if (req && !json_object_is_type(req, json_type_string))
		json_object_put(req);

	return ret;
}

/*
 * set_volume_cb() - Data callback for 'set_audio_volume' device requests
 *
 * @target:					Target ID of the device request (set_audio_volume).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
static ccapi_receive_error_t set_volume_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	ccapi_receive_error_t ret = CCAPI_RECEIVE_ERROR_NONE;
	char *cmd = NULL, *error_msg = NULL, *resp = NULL, *val = NULL;
	uint16_t volume, cmd_len = 0;

	log_dr_debug("%s: target='%s' - transport='%d'", __func__, target, transport);

	response_buffer_info->buffer = calloc(MAX_RESPONSE_SIZE + 1, sizeof(char));
	val = calloc(request_buffer_info->length + 1, sizeof(char));
	if (response_buffer_info->buffer == NULL || val == NULL) {
		log_dr_error("Cannot generate response for target '%s': Out of memory", target);
		ret = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
		goto exit;
	}

	strncpy(val, request_buffer_info->buffer, request_buffer_info->length);
	log_dr_debug("%s=%s", target, val);

	/* Verify value is an integer. */
	volume = atoi(val);
	if (strlen(val) != (uint16_t)snprintf(NULL, 0, "%d", volume)) {
		error_msg = "Volume value must be an integer";
		log_dr_error("Argument for target '%s' is not an integer: '%s'", target, val);
		ret = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
		goto done;
	}

	/* Execute command. */
	cmd_len = snprintf(NULL, 0, CMD_SET_VOLUME, volume, volume);
	cmd = calloc(cmd_len + 1, sizeof(char));
	if (cmd == NULL) {
		error_msg = "Out of memory";
		log_dr_error("Cannot change volume: %s", error_msg);
		ret = CCAPI_RECEIVE_ERROR_INSUFFICIENT_MEMORY;
		goto done;
	}
	sprintf(cmd, CMD_SET_VOLUME, volume, volume);
	if (ldx_process_execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
		error_msg = "Error setting audio volume";
		ret = CCAPI_RECEIVE_ERROR_INVALID_DATA_CB;
		if (resp != NULL)
			log_dr_error("%s: %s", error_msg, resp);
		else
			log_dr_error("%s", error_msg);
	}

done:
	if (ret != CCAPI_RECEIVE_ERROR_NONE) {
		response_buffer_info->length = sprintf(response_buffer_info->buffer, "ERROR: %s", error_msg);
		log_dr_error("Cannot process request for target '%s': %s", target, error_msg);
	} else {
		response_buffer_info->length = sprintf(response_buffer_info->buffer, "OK");
	}

exit:
	free(val);
	free(cmd);
	free(resp);

	return ret;
}

/*
 * request_status_cb() - Status callback for application device requests
 *
 * @target:                 Target ID of the device request.
 * @transport:              Communication transport used by the device request.
 * @response_buffer_info:   Buffer containing the response data.
 * @receive_error:          The error status of the receive process.
 *
 * This callback is executed when the receive process has finished. It doesn't
 * matter if everything worked or there was an error during the process.
 *
 * Cleans and frees the response buffer.
 */
static void request_status_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t *const response_buffer_info,
		ccapi_receive_error_t receive_error)
{
	log_dr_debug(
			"%s: target='%s' - transport='%d' - error='%d'", __func__, target,
			transport, receive_error);

	/* Free the response buffer */
	if (response_buffer_info != NULL)
		free(response_buffer_info->buffer);

	if (receive_error == CCAPI_RECEIVE_ERROR_NONE && strcmp(TARGET_STOP_CC, target) == 0)
		kill(getpid(), SIGINT);
}

/*
 * register_custom_device_requests() - Register custom device requests
 *
 * Return: Error code after registering the custom device requests.
 */
ccapi_receive_error_t register_custom_device_requests(void)
{
	char *target = TARGET_DEVICE_INFO;
	ccapi_receive_error_t error = ccapi_receive_add_target(target,
			device_info_cb, request_status_cb, 0);

	if (error != CCAPI_RECEIVE_ERROR_NONE)
		goto done;

	target = TARGET_GET_TIME;
	error = ccapi_receive_add_target(target, get_time_cb, request_status_cb, 0);
	if (error != CCAPI_RECEIVE_ERROR_NONE)
		goto done;

	target = TARGET_STOP_CC;
	error = ccapi_receive_add_target(target, stop_cb, request_status_cb, 0);
	if (error != CCAPI_RECEIVE_ERROR_NONE)
		goto done;

	target = TARGET_USER_LED;
	error = ccapi_receive_add_target(target, update_user_led_cb,
			request_status_cb, 5); /* Max size of possible values (on, off, 0, 1, true, false): 5 */
	if (error != CCAPI_RECEIVE_ERROR_NONE)
		goto done;

	target = TARGET_PLAY_MUSIC;
	error = ccapi_receive_add_target(target, play_music_cb,
			request_status_cb, 255);
	if (error != CCAPI_RECEIVE_ERROR_NONE)
		goto done;

	target = TARGET_SET_VOLUME;
	error = ccapi_receive_add_target(target, set_volume_cb,
			request_status_cb, 3); /* Max size of possible values (0-100): 3 */

done:
	if (error != CCAPI_RECEIVE_ERROR_NONE)
		log_error("Cannot register target '%s', error %d", target, error);

	return error;
}
