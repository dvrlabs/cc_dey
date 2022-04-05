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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccapi/ccapi.h>

#include "wifi.h"
#include "cc_logging.h"
#include "string_utils.h"

#define NO_OUTPUT		">/dev/null 2>&1"

typedef enum {OUSIDE_NETWORK, INSIDE_NETWORK} supplicant_parser_state_t;

static int get_ssid(const char *iface_name, char *ssid);
static int read_param_from_cli(const char *iface_name, const char *param, char *value);
static int read_param_from_file(const char *param, char *value);
static ccapi_bool_t is_supplicant_running(void);
int get_current_wifi_network(const char *iface_name);
wpa_state_t get_wpa_status(const char *iface_name);

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
		log_error("%s: supplicant is not running", __func__);
		return -1;
	}

	/* Get SSID using wpa_cli get_network */
	if (read_param_from_cli(iface_name, SSID_FIELD, ssid) != 0) {
		/* Get SSID using wpa_cli status */
		char line[SSID_SIZE + 1] = {0};
		char cmd[255] = {0};
		FILE *fd;

		sprintf(cmd, "%s %s %s", WIFI_CMD, iface_name, STATUS_SSID_CMD);
		fd = popen(cmd, "r");
		if (fd == NULL) {
			log_error("%s: Error opening pipe for cmd '%s'", __func__, cmd);
			return -1;
		}
		if (fgets(line, sizeof (line) - 1, fd)) {
			char *l = trim(line);
			l = delete_quotes(l);
			strncpy(ssid, l, SSID_SIZE + 1);
		}
		pclose(fd);
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
	FILE *fd;
	char line[255] = {0}, cmd[255] = {0};
	int network;
	char *ret = NULL, *l = NULL;

	/* Sanity checks */
	if (iface_name == NULL || param == NULL || value == NULL)
		return -1;

	/* Set blank value (it might not exist) */
	value[0] = '\0';

	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("%s: supplicant is not running", __func__);
		return -1;
	}

	/* Get selected network */
	network = get_current_wifi_network(iface_name);
	if (network < 0) {
		log_error("%s: No network selected for %s", __func__, iface_name);
		return -1;
	}

	/* Execute command */
	sprintf(cmd, "%s %s %s %d %s", WIFI_CMD, iface_name, GET_NETWORK_CMD, network, param);
	fd = popen(cmd, "r");
	if (fd == NULL) {
		log_error("%s: Error opening pipe for cmd '%s'", __func__, cmd);
		return -1;
	}
	ret = fgets(line, sizeof (line) - 1, fd);
	pclose(fd);
	if (!ret)
		return -1;

	l = trim(line);
	l = delete_quotes(l);
	strcpy(value, l);
	if (strcmp(value, "FAIL") == 0) {
		value[0] = '\0';
		return -1;
	}
	if (strlen(value) == 0) {
		value[0] = '\0';
		return -1;
	}

	return 0;
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
static ccapi_bool_t is_supplicant_running(void)
{
	char cmd[255] = {0};

	sprintf(cmd, "%s %s", WIFI_PING_CMD, NO_OUTPUT);
	if (system(cmd) == 0)
		return CCAPI_TRUE;

	return CCAPI_FALSE;
}

/*
 * get_current_wifi_network() - Get the network index for the given interface.
 *
 * @iface_name:	Name of the wireless interface to retrieve its network index.
 *
 * Return: The interface network index, -1 on error.
 */
int get_current_wifi_network(const char *iface_name)
{
	char cmd[255], line[255];
	FILE *fd;
	int network;
	char *ret = NULL;

	/* Sanity checks */
	if (iface_name == NULL)
		return -1;

	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("%s: supplicant is not running", __func__);
		return -1;
	}

	/* Execute command */
	sprintf(cmd, "%s %s %s", WIFI_CMD, iface_name, GET_CURRENT_NETWORK_CMD);
	fd = popen(cmd, "r");
	if (fd == NULL) {
		log_error("%s: Error opening pipe for cmd '%s'", __func__, cmd);
		return -1;
	}
	ret = fgets(line, sizeof (line) - 1, fd);
	pclose(fd);
	if (!ret)
		return -1;

	if (sscanf(line, "%d", &network))
		return network;

	return -1;
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
	FILE *fd;
	char cmd[255], line[255];
	char *ret = NULL, *l = NULL;

	/* Sanity checks */
	if (iface_name == NULL)
		return WPA_UNKNOWN;

	/* Check if supplicant is running */
	if (!is_supplicant_running()) {
		log_error("%s: supplicant is not running", __func__);
		return WPA_UNKNOWN;
	}

	/* Execute command */
	sprintf(cmd, "%s %s %s", WIFI_CMD, iface_name, STATUS_WPA_STATE_CMD);
	fd = popen(cmd, "r");
	if (fd == NULL) {
		log_error("%s: Error opening pipe for cmd '%s'", __func__, cmd);
		return WPA_UNKNOWN;
	}
	ret = fgets(line, sizeof (line) - 1, fd);
	pclose(fd);
	if (!ret)
		return WPA_UNKNOWN;

	l = trim(line);

	if (strncmp(l, WPA_DISCONNECTED_STRING, strlen(WPA_DISCONNECTED_STRING)) == 0)
		return WPA_DISCONNECTED;
	if (strncmp(l, WPA_INACTIVE_STRING, strlen(WPA_INACTIVE_STRING)) == 0)
		return WPA_INACTIVE;
	if (strncmp(l, WPA_SCANNING_STRING, strlen(WPA_SCANNING_STRING)) == 0)
		return WPA_SCANNING;
	if (strncmp(l, WPA_ASSOCIATING_STRING, strlen(WPA_ASSOCIATING_STRING)) == 0)
		return WPA_ASSOCIATING;
	if (strncmp(l, WPA_ASSOCIATED_STRING, strlen(WPA_ASSOCIATED_STRING)) == 0)
		return WPA_ASSOCIATED;
	if (strncmp(l, WPA_4WAY_HANDSHAKE_STRING, strlen(WPA_4WAY_HANDSHAKE_STRING)) == 0)
		return WPA_4WAY_HANDSHAKE;
	if (strncmp(l, WPA_GROUP_HANDSHAKE_STRING, strlen(WPA_GROUP_HANDSHAKE_STRING)) == 0)
		return WPA_GROUP_HANDSHAKE;
	if (strncmp(l, WPA_WPA_COMPLETED_STRING, strlen(WPA_WPA_COMPLETED_STRING)) == 0)
		return WPA_COMPLETED;

	return WPA_UNKNOWN;
}
