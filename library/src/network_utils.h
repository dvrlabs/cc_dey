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

#ifndef network_utils_h
#define network_utils_h

#include <net/if.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define MAX_DNS_ADDRESSES	2
#define IPV4_GROUPS			4
#define MAC_ADDRESS_GROUPS	6

#define IP_STRING_LENTH 	(4 * IPV4_GROUPS)
#define IP_FORMAT			"%d.%d.%d.%d"
#define MAC_STRING_LENGTH	(3 * MAC_ADDRESS_GROUPS)
#define MAC_FORMAT			"%02x:%02x:%02x:%02x:%02x:%02x"

typedef struct {
	unsigned long long rx_bytes;
	unsigned long long rx_packets;
	unsigned long rx_errors;
	unsigned long rx_dropped;
	unsigned long rx_multicast;
	unsigned long rx_compressed;

	unsigned long long tx_bytes;
	unsigned long long tx_packets;
	unsigned long tx_errors;
	unsigned long tx_dropped;
	unsigned long tx_compressed;
	unsigned long collisions;

	/* rx errors */
	unsigned long rx_length_errors;
	unsigned long rx_over_errors;
	unsigned long rx_crc_errors;
	unsigned long rx_frame_errors;
	unsigned long rx_fifo_errors;
	unsigned long rx_missed_errors;
	/* tx errors */
	unsigned long tx_aborted_errors;
	unsigned long tx_carrier_errors;
	unsigned long tx_fifo_errors;
	unsigned long tx_heartbeat_errors;
	unsigned long tx_window_errors;
} net_stats_t;

typedef struct {
	uint8_t ipv4_addr[IPV4_GROUPS];
	uint8_t mac_addr[MAC_ADDRESS_GROUPS];
	char name[IFNAMSIZ];
	uint8_t gateway[IPV4_GROUPS];
	uint8_t submask[IPV4_GROUPS];
	uint8_t dnsaddr1[IPV4_GROUPS];
	uint8_t dnsaddr2[IPV4_GROUPS];
	ccapi_bool_t dhcp;
	ccapi_bool_t enabled;
} iface_info_t;

typedef struct {
	unsigned long long rx_bytes;
	unsigned long long tx_bytes;
} bt_stats_t;

typedef struct {
	char name[IFNAMSIZ];
	ccapi_bool_t enabled;
	uint8_t mac_addr[MAC_ADDRESS_GROUPS];
	bt_stats_t stats;
} bt_info_t;

int get_main_iface_info(const char *url, iface_info_t *info);
int get_iface_info(const char *iface_name, iface_info_t *info);
int get_net_stats(const char *iface_name, net_stats_t *net_stats);
uint8_t *get_primary_mac_address(uint8_t * const mac_addr);
int get_bt_info(const char *iface_name, bt_info_t *bt_info);

#endif
