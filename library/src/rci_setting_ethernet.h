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

#ifndef rci_setting_ethernet_h
#define rci_setting_ethernet_h

#include "connector_api.h"
#include "ccapi_rci_functions.h"

typedef enum {
 CCAPI_SETTING_ETHERNET_CONN_TYPE_DHCP,
 CCAPI_SETTING_ETHERNET_CONN_TYPE_STATIC,
 CCAPI_SETTING_ETHERNET_CONN_TYPE_COUNT
} ccapi_setting_ethernet_conn_type_id_t;

typedef enum {
 CCAPI_SETTING_ETHERNET_ERROR_NONE,
 CCAPI_SETTING_ETHERNET_ERROR_BAD_COMMAND, /* PROTOCOL DEFINED */
 CCAPI_SETTING_ETHERNET_ERROR_BAD_DESCRIPTOR,
 CCAPI_SETTING_ETHERNET_ERROR_BAD_VALUE,
 CCAPI_SETTING_ETHERNET_ERROR_INVALID_INDEX,
 CCAPI_SETTING_ETHERNET_ERROR_INVALID_NAME,
 CCAPI_SETTING_ETHERNET_ERROR_MISSING_NAME,
 CCAPI_SETTING_ETHERNET_ERROR_LOAD_FAIL, /* USER DEFINED (GLOBAL ERRORS) */
 CCAPI_SETTING_ETHERNET_ERROR_SAVE_FAIL,
 CCAPI_SETTING_ETHERNET_ERROR_MEMORY_FAIL,
 CCAPI_SETTING_ETHERNET_ERROR_NOT_IMPLEMENTED,
 CCAPI_SETTING_ETHERNET_ERROR_COUNT
} ccapi_setting_ethernet_error_id_t;

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_start(ccapi_rci_info_t * const info);
ccapi_setting_ethernet_error_id_t rci_setting_ethernet_end(ccapi_rci_info_t * const info);

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_iface_name_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_iface_name_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_enabled_get(ccapi_rci_info_t * const info, ccapi_on_off_t * const value);
#define rci_setting_ethernet_enabled_set    NULL

#if (defined RCI_ENUMS_AS_STRINGS)
ccapi_setting_ethernet_error_id_t rci_setting_ethernet_conn_type_get(ccapi_rci_info_t * const info, char const * * const value);
#else
ccapi_setting_ethernet_error_id_t rci_setting_ethernet_conn_type_get(ccapi_rci_info_t * const info, ccapi_setting_ethernet_conn_type_id_t * const value);
#endif /* RCI_ENUMS_AS_STRINGS */
#define rci_setting_ethernet_conn_type_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_ipaddr_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_ipaddr_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_netmask_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_netmask_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_dns1_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_dns1_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_dns2_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_dns2_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_gateway_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_gateway_set    NULL

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_mac_addr_get(ccapi_rci_info_t * const info, char const * * const value);
#define rci_setting_ethernet_mac_addr_set    NULL

#endif
