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
#include "rci_setting_wifi.h"
#include "cc_logging.h"
#include "wifi.h"

#define INTERFACE_NAME		"wlan0"

typedef struct {
	wifi_info_t info;
	char ipaddr_buff[IP_STRING_LENTH];
	char submask_buff[IP_STRING_LENTH];
	char dns1_buff[IP_STRING_LENTH];
	char dns2_buff[IP_STRING_LENTH];
	char gw_buff[IP_STRING_LENTH];
	char mac_addr_buff[MAC_STRING_LENGTH];
} rci_iface_info_t;

static rci_iface_info_t *wifi_iface_info;

ccapi_setting_wifi_error_id_t rci_setting_wifi_start(ccapi_rci_info_t * const info)
{
	ccapi_setting_wifi_error_id_t ret = CCAPI_SETTING_WIFI_ERROR_NONE;
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	wifi_iface_info = (rci_iface_info_t *) malloc(sizeof(rci_iface_info_t));
	if (wifi_iface_info == NULL) {
		log_error("%s: Cannot allocate memory", __func__);
		ret = CCAPI_SETTING_WIFI_ERROR_MEMORY_FAIL;
		goto done;
	}

	if (get_wifi_info(INTERFACE_NAME, &(wifi_iface_info->info)) != 0)
		log_error("%s: get_iface_info failed", __func__);

done:
	return ret;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_end(ccapi_rci_info_t * const info)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	free(wifi_iface_info);
	wifi_iface_info = NULL;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_iface_name_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	*value = INTERFACE_NAME;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_enabled_get(ccapi_rci_info_t * const info, ccapi_on_off_t * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	*value = wifi_iface_info->info.iface_info.enabled ? CCAPI_ON : CCAPI_OFF;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_ssid_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	*value = wifi_iface_info->info.ssid;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_wpa_status_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	switch (wifi_iface_info->info.wpa_state) {
	case WPA_DISCONNECTED:
		*value = WPA_DISCONNECTED_STRING;
		break;
	case WPA_INACTIVE:
		*value = WPA_INACTIVE_STRING;
		break;
	case WPA_SCANNING:
		*value = WPA_SCANNING_STRING;
		break;
	case WPA_AUTHENTICATING:
		*value = WPA_AUTHENTICATING_STRING;
		break;
	case WPA_ASSOCIATING:
		*value = WPA_ASSOCIATING_STRING;
		break;
	case WPA_ASSOCIATED:
		*value = WPA_ASSOCIATED_STRING;
		break;
	case WPA_4WAY_HANDSHAKE:
		*value = WPA_4WAY_HANDSHAKE_STRING;
		break;
	case WPA_GROUP_HANDSHAKE:
		*value = WPA_GROUP_HANDSHAKE_STRING;
		break;
	case WPA_COMPLETED:
		*value = WPA_WPA_COMPLETED_STRING;
		break;
	default:
		*value = WPA_UNKNOWN_STRING;
		break;
	}

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_conn_type_get(ccapi_rci_info_t * const info, ccapi_setting_wifi_conn_type_id_t * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);

	*value = (wifi_iface_info->info.iface_info.dhcp == CCAPI_TRUE ? CCAPI_SETTING_WIFI_CONN_TYPE_DHCP : CCAPI_SETTING_WIFI_CONN_TYPE_STATIC);

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_ipaddr_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);
	uint8_t const * const ip = wifi_iface_info->info.iface_info.ipv4_addr;

	snprintf(wifi_iface_info->ipaddr_buff, IP_STRING_LENTH, IP_FORMAT,
			ip[0], ip[1], ip[2], ip[3]);
	*value = wifi_iface_info->ipaddr_buff;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_netmask_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);
	uint8_t const * const netmask = wifi_iface_info->info.iface_info.submask;

	snprintf(wifi_iface_info->submask_buff, IP_STRING_LENTH, IP_FORMAT,
			netmask[0], netmask[1], netmask[2], netmask[3]);
	*value = wifi_iface_info->submask_buff;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_dns1_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);
	uint8_t const * const dns1 = wifi_iface_info->info.iface_info.dnsaddr1;

	snprintf(wifi_iface_info->dns1_buff, IP_STRING_LENTH, IP_FORMAT,
			dns1[0], dns1[1], dns1[2], dns1[3]);
	*value = wifi_iface_info->dns1_buff;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_dns2_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);
	uint8_t const * const dns2 = wifi_iface_info->info.iface_info.dnsaddr2;

	snprintf(wifi_iface_info->dns2_buff, IP_STRING_LENTH, IP_FORMAT,
			dns2[0], dns2[1], dns2[2], dns2[3]);
	*value = wifi_iface_info->dns2_buff;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_gateway_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);
	uint8_t const * const gateway = wifi_iface_info->info.iface_info.gateway;

	snprintf(wifi_iface_info->gw_buff, IP_STRING_LENTH, IP_FORMAT,
			gateway[0], gateway[1], gateway[2], gateway[3]);
	*value = wifi_iface_info->gw_buff;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}

ccapi_setting_wifi_error_id_t rci_setting_wifi_mac_addr_get(ccapi_rci_info_t * const info, char const * * const value)
{
	UNUSED_PARAMETER(info);
	log_debug("    Called '%s'", __func__);
	uint8_t const * const mac = wifi_iface_info->info.iface_info.mac_addr;

	snprintf(wifi_iface_info->mac_addr_buff, MAC_STRING_LENGTH, MAC_FORMAT,
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	*value = wifi_iface_info->mac_addr_buff;

	return CCAPI_SETTING_WIFI_ERROR_NONE;
}
