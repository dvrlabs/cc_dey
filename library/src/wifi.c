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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccapi/ccapi.h>

#include "cc_logging.h"
#include "services_util.h"
#include "string_utils.h"
#include "wifi.h"

/* CLI commands */
#define CMD_WIFI		"wpa_cli -i"
#define CMD_WIFI_PING		"wpa_cli ping"
#define CMD_GET_NETWORK		"get_network"
#define CMD_STATUS_SSID		"status | grep -E \"^ssid\" | cut -f 2 -d ="
#define CMD_STATUS_WPA_STATE	"status | grep wpa_state | cut -f 2 -d ="
#define CMD_GET_CURRENT_NETWORK	"list_networks | grep -E \"^[0-9]+\" | grep -E -v \"\\[DISABLED\\]$\" | cut -f 1"

#define SSID_FIELD		"ssid"

#define WPA_SUPPLICANT_FILE	"/etc/wpa_supplicant.conf"

#define NO_OUTPUT		">/dev/null 2>&1"

typedef enum {OUSIDE_NETWORK, INSIDE_NETWORK} supplicant_parser_state_t;

static int get_ssid(const char *iface_name, char *ssid);
static int read_param_from_cli(const char *iface_name, const char *param, char *value);
static int read_param_from_file(const char *param, char *value);
static bool is_supplicant_running(void);
static int get_current_wifi_network(const char *iface_name);
static wpa_state_t get_wpa_status(const char *iface_name);

/*
 * get_wifi_info() - Retrieve information about the given wireless interface.
 *
 * @iface_name:	Name of the wireless interface to retrieve its information.
 * @wifi_info:	Struct to fill with the wireless interface information.
 *
 * Return: 0 on success, -1 otherwise.
 */
int get_wifi_info(const char *iface_name, wifi_info_t *wifi_info)
{
	/* Sanity checks */
	if (wifi_info == NULL)
		return -1;

	/* Clear all values */
	memset(wifi_info, 0, sizeof (wifi_info_t));

	/* Get basic network information */
	if (get_iface_info(iface_name, &(wifi_info->iface_info)) != 0) {
		log_error("%s: error reading network information for '%s'", __func__, iface_name);
		return -1;
	}
	/* Fill SSID */
	get_ssid(iface_name, wifi_info->ssid);
	/* Fill WPA status */
	wifi_info->wpa_state = get_wpa_status(iface_name);

	return 0;
}

/*
 * get_ssid() - Retrieve the SSID to which the interface is connected to.
 *
 * @iface_name:	Name of the wireless interface to retrieve its connected SSID.
 * @ssid:		String to store the SSID value.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_ssid(const char *iface_name, char *ssid)
{
	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("Error getting '%s' SSID: Supplicant is not running", iface_name);
		return -1;
	}

	/* Get SSID using wpa_cli get_network */
	if (read_param_from_cli(iface_name, SSID_FIELD, ssid) != 0) {
		/* Get SSID using wpa_cli status */
		char cmd[255] = {0};
		char *resp = NULL, *val = NULL;

		sprintf(cmd, "%s %s %s", CMD_WIFI, iface_name, CMD_STATUS_SSID);
		if (execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
			if (resp != NULL)
				log_error("Error getting '%s' SSID: %s", iface_name, resp);
			else
				log_error("Error getting '%s' SSID", iface_name);
			free(resp);
			return -1;
		}
		
		val = delete_quotes(trim(resp));
		strncpy(ssid, val, SSID_SIZE + 1);

		free(resp);
	} else {
		char *s = delete_quotes(ssid);
		strncpy(ssid, s, SSID_SIZE + 1);
	}

	/* Crop SSID name */
	if (strlen(ssid) > SSID_SIZE) {
		ssid[SSID_SIZE] = 0;
	}

	return 0;
}

/*
 * read_param_from_cli() - Read WiFi parameter using supplicant command line.
 *
 * @iface_name:	Name of the wireless interface to read the parameter for.
 * @param:		Parameter to read.
 * @value:		String to store the parameter value.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int read_param_from_cli(const char *iface_name, const char *param, char *value)
{
	char cmd[255];
	char *resp = NULL, *val = NULL;
	int network, ret = -1;

	/* Sanity checks */
	if (iface_name == NULL || param == NULL || value == NULL)
		return -1;

	/* Set blank value (it might not exist) */
	value[0] = '\0';

	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("Error getting '%s' '%s' parameter: Supplicant is not running", iface_name, param);
		return -1;
	}

	/* Get selected network */
	network = get_current_wifi_network(iface_name);
	if (network < 0) {
		log_error("Error getting '%s' '%s' parameter: No network selected for %s", iface_name, param, iface_name);
		return -1;
	}

	/* Execute command */
	sprintf(cmd, "%s %s %s %d %s", CMD_WIFI, iface_name, CMD_GET_NETWORK, network, param);
	if (execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
		if (resp != NULL)
			log_error("Error getting '%s' '%s' parameter: %s", iface_name, param, resp);
		else
			log_error("Error getting '%s' '%s' parameter", iface_name, param);
		goto done;
	}

	val = trim(resp);
	val = delete_quotes(val);
	strcpy(value, val);

	if (strlen(value) == 0)
		value[0] = '\0';
	else if (strcmp(value, "FAIL") == 0)
		value[0] = '\0';
	else
		ret = 0;

done:
	free(resp);

	return ret;
}

/*
 * read_param_from_file() - Read WiFi parameter using supplicant configuration file.
 *
 * @param:		Parameter to read.
 * @value:		String to store the parameter value.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int read_param_from_file(const char *param, char *value)
{
	FILE *fd;
	supplicant_parser_state_t state = 0;
	char gotfield = 0;
	char field[255] = {0}, line[255] = {0};

	/* Sanity checks */
	if (param == NULL || value == NULL)
		return -1;

	/* Read file */
	fd = fopen(WPA_SUPPLICANT_FILE, "r");
	if (fd == NULL) {
		log_error("%s: Cannot locate WPA supplicant config file '%s'", __func__, WPA_SUPPLICANT_FILE);
		return -1;
	}
	sprintf(field, "%s=", param);
	while (fgets(line, sizeof (line) - 1, fd)) {
		char *l = trim(line);
		switch (state) {
			case OUSIDE_NETWORK:
				if (strncmp(l, "network={", strlen("network={")) == 0) {
					/* Start 'network' section */
					state = INSIDE_NETWORK;
				}
				break;
			case INSIDE_NETWORK:
				/* Within 'network' section */
				if (l[0] == '}') {
					/* End of 'network' section */
					if (gotfield)
						goto done;
					else
						state = OUSIDE_NETWORK;
				} else if (strncmp(l, field, strlen(field)) == 0) {
					strcpy(value, l + strlen(field));
					gotfield = 1;
				} else if (strncmp(l, "disabled=1", strlen("disabled=1")) == 0) {
					/* Section is not valid. Reset flags and start again. */
					state = OUSIDE_NETWORK;
					gotfield = 0;
				}
				break;
		}
	}

done:
	fclose(fd);
	if (!gotfield) {
		value[0] = '\0';
		return -1;
	}

	return 0;
}

/**
 * is_supplicant_running() - Check if supplicant is running.
 *
 * Return: CCAPI_TRUE if supplicant is running, CCAPI_FALSE otherwise.
 */
static bool is_supplicant_running(void)
{
	char cmd[255] = {0};
	char *resp = NULL;
	bool res = false;

	sprintf(cmd, "%s %s", CMD_WIFI_PING, NO_OUTPUT);
	res = execute_cmd(cmd, &resp, 2) == 0;
	free(resp);

	return res;
}

/*
 * get_current_wifi_network() - Get the network index for the given interface.
 *
 * @iface_name:	Name of the wireless interface to retrieve its network index.
 *
 * Return: The interface network index, -1 on error.
 */
static int get_current_wifi_network(const char *iface_name)
{
	char cmd[255];
	char *resp = NULL;
	int network = -1;

	/* Sanity checks */
	if (iface_name == NULL)
		return -1;

	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("Error getting current Wi-Fi network: %s", "Supplicant is not running");
		return -1;
	}

	/* Execute command */
	sprintf(cmd, "%s %s %s", CMD_WIFI, iface_name, CMD_GET_CURRENT_NETWORK);
	if (execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
		if (resp != NULL)
			log_error("Error getting current Wi-Fi network: %s", resp);
		else
			log_error("%s", "Error getting current Wi-Fi network");
		goto done;
	}

	if (strlen(resp) > 0)
		resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */

	if (!sscanf(resp, "%d", &network))
		network = -1;
done:
	free(resp);

	return network;
}

/*
 * get_wpa_status() - Retrieve the current wpa status.
 *
 * @iface_name:	Name of the wireless interface to retrieve its wpa status.
 *
 * Return: The current wpa status. WPA_UNKNOWN on error.
 */
wpa_state_t get_wpa_status(const char *iface_name)
{
	char cmd[255];
	char *resp = NULL;
	wpa_state_t state = WPA_UNKNOWN;

	/* Sanity checks */
	if (iface_name == NULL)
		return WPA_UNKNOWN;

	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("Error getting '%s' WPA status: Supplicant is not running", iface_name);
		return WPA_UNKNOWN;
	}

	/* Execute command */
	sprintf(cmd, "%s %s %s", CMD_WIFI, iface_name, CMD_STATUS_WPA_STATE);
	if (execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
		if (resp != NULL)
			log_error("Error getting '%s' WPA status: %s", iface_name, resp);
		else
			log_error("Error getting '%s' WPA status", iface_name);
		goto done;
	}

	if (strlen(resp) > 0)
		resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */

	if (strncmp(resp, WPA_DISCONNECTED_STRING, strlen(WPA_DISCONNECTED_STRING)) == 0)
		state = WPA_DISCONNECTED;
	else if (strncmp(resp, WPA_INACTIVE_STRING, strlen(WPA_INACTIVE_STRING)) == 0)
		state = WPA_INACTIVE;
	else if (strncmp(resp, WPA_SCANNING_STRING, strlen(WPA_SCANNING_STRING)) == 0)
		state = WPA_SCANNING;
	else if (strncmp(resp, WPA_ASSOCIATING_STRING, strlen(WPA_ASSOCIATING_STRING)) == 0)
		state = WPA_ASSOCIATING;
	else if (strncmp(resp, WPA_ASSOCIATED_STRING, strlen(WPA_ASSOCIATED_STRING)) == 0)
		state = WPA_ASSOCIATED;
	else if (strncmp(resp, WPA_4WAY_HANDSHAKE_STRING, strlen(WPA_4WAY_HANDSHAKE_STRING)) == 0)
		state = WPA_4WAY_HANDSHAKE;
	else if (strncmp(resp, WPA_GROUP_HANDSHAKE_STRING, strlen(WPA_GROUP_HANDSHAKE_STRING)) == 0)
		state = WPA_GROUP_HANDSHAKE;
	else if (strncmp(resp, WPA_WPA_COMPLETED_STRING, strlen(WPA_WPA_COMPLETED_STRING)) == 0)
		state = WPA_COMPLETED;
	else
		state = WPA_UNKNOWN;
done:
	free(resp);

	return state;
}
