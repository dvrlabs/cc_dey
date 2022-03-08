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
	uint8_t ipv4_addr[IPV4_GROUPS];
	uint8_t mac_addr[MAC_ADDRESS_GROUPS];
	char name[IFNAMSIZ];
	uint8_t gateway[IPV4_GROUPS];
	uint8_t submask[IPV4_GROUPS];
	uint8_t dnsaddr1[IPV4_GROUPS];
	uint8_t dnsaddr2[IPV4_GROUPS];
	ccapi_bool_t dhcp;
	ccapi_bool_t enabled;
} iface_info_t ;

int get_main_iface_info(const char *url, iface_info_t *info);
int get_iface_info(const char *iface_name, iface_info_t *info);
uint8_t *get_primary_mac_address(uint8_t * const mac_addr);

#endif
