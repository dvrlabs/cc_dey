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

#ifndef WIFI_H
#define WIFI_H

#include "network_utils.h"

/* Wi-Fi constants */
#define SSID_SIZE			32

/* CLI commands */
#define WIFI_CMD				"wpa_cli -i"
#define WIFI_PING_CMD			"wpa_cli ping"
#define SSID_FIELD				"ssid"
#define GET_NETWORK_CMD			"get_network"
#define STATUS_SSID_CMD			"status | grep -E \"^ssid\" | cut -f 2 -d ="
#define STATUS_WPA_STATE_CMD	"status | grep wpa_state | cut -f 2 -d ="
#define GET_CURRENT_NETWORK_CMD	"list_networks | grep -E \"^[0-9]+\" | grep -E -v \"\\[DISABLED\\]$\" | cut -f 1"

#define WPA_SUPPLICANT_FILE		"/etc/wpa_supplicant.conf"

/* WPA states */
#define WPA_UNKNOWN_STRING			"UNKNOWN"
#define WPA_DISCONNECTED_STRING		"DISCONNECTED"
#define WPA_INACTIVE_STRING			"INACTIVE"
#define WPA_SCANNING_STRING			"SCANNING"
#define WPA_AUTHENTICATING_STRING	"AUTHENTICATING"
#define WPA_ASSOCIATING_STRING		"ASSOCIATING"
#define WPA_ASSOCIATED_STRING		"ASSOCIATED"
#define WPA_4WAY_HANDSHAKE_STRING	"4WAY_HANDSHAKE"
#define WPA_GROUP_HANDSHAKE_STRING	"GROUP_HANDSHAKE"
#define WPA_WPA_COMPLETED_STRING	"COMPLETED"

typedef enum {
	WPA_UNKNOWN,
	WPA_DISCONNECTED,
	WPA_INACTIVE,
	WPA_SCANNING,
	WPA_AUTHENTICATING,
	WPA_ASSOCIATING,
	WPA_ASSOCIATED,
	WPA_4WAY_HANDSHAKE,
	WPA_GROUP_HANDSHAKE,
	WPA_COMPLETED,
} wpa_state_t;

typedef struct {
	iface_info_t iface_info;
	char ssid[SSID_SIZE + 1];
	wpa_state_t wpa_state;
} wifi_info_t;

int get_wifi_info(const char *iface_name, wifi_info_t *wifi_info);

#endif /* WIFI_H */
