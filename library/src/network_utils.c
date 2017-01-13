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

#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

#include "network_utils.h"
#include "cc_logging.h"
/*
 * get_ipv4_address() - Retrieves the IPv4 address of the first interface
 *
 * @ipv4_addr:	Pointer to store the IPv4.
 *
 * The interface must be different from loopback.
 *
 * Return: 0 on success, -1 otherwise.
 */
int get_ipv4_address(uint8_t *const ipv4_addr)
{
	unsigned int const ipv4_len = 4;
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	int retval = -1;

	if (getifaddrs(&ifaddr) == -1) {
		log_error("%s", "get_ipv4_address(): getifaddrs() failed");
		goto done;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		int family;

		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;
		if (family == AF_INET) {
			char host[NI_MAXHOST];
			in_addr_t ipv4;

			int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
					NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				log_error("get_ipv4_address(): getnameinfo() failed: %s", gai_strerror(s));
				continue;
			}

			ipv4 = inet_addr(host);
			if (ipv4 != htonl(INADDR_LOOPBACK)) {
				log_info("get_ipv4_address(): Interface name [%s] - IP Address [%s]", ifa->ifa_name, host);
				memcpy(ipv4_addr, &ipv4, ipv4_len);
				retval = 0;
				break;
			}
		}
	}
done:
	freeifaddrs(ifaddr);
	return retval;
}

/**
 * get_mac_addr() - Get the primary MAC address of the device.
 *
 * @mac_addr:	Pointer to store the MAC address.
 *
 * The interfaces priority order is the following:
 *   - Ethernet (eth0, eth1, ...)
 *   - Wi-Fi (wlan0, wlan1, ...)
 *
 * Return: The MAC address of primary interface.
 */
uint8_t *get_mac_addr(uint8_t *const mac_addr)
{
	unsigned int const max_interfaces = 8;
	size_t const buf_size = max_interfaces * sizeof(struct ifreq);
	struct ifconf ifconf;
	uint8_t *retval = NULL;
	int sock = -1;

	char *const buf = malloc(sizeof(char) * buf_size);
	if (buf == NULL) {
		log_error("%s", "get_mac_addr(): malloc failed for char");
		goto done;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		log_error("%s", "get_mac_addr(): socket() failed");
		goto done;
	}

	ifconf.ifc_len = buf_size;
	ifconf.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0) {
		log_error("%s", "get_mac_addr(): Error using ioctl SIOCGIFCONF.");
		goto done;
	}

	{
		unsigned int entries = 0;
		unsigned int i;

		entries = (unsigned int) ifconf.ifc_len / sizeof(struct ifreq);
		if (entries == 0)
			goto done;

		for (i = 0; i < entries; i++) {
			struct ifreq ifr;
			struct ifreq *ifreq = &ifconf.ifc_req[i];

			strcpy(ifr.ifr_name, ifreq->ifr_name);
			if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
				log_error("%s", "get_mac_addr(): Error using ioctl SIOCGIFFLAGS.");
				continue;
			}

			if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
					if (get_interface_mac_addr(&ifr, mac_addr, "^eth([0-9]{1,3})$") != NULL
							|| get_interface_mac_addr(&ifr, mac_addr, "^wlan([0-9]{1,3})$") != NULL) {
						memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);
						retval = mac_addr;
						log_info("Primary interface %s - MAC %02x:%02x:%02x:%02x:%02x:%02x",
								ifr.ifr_name, mac_addr[0], mac_addr[1],
								mac_addr[2], mac_addr[3], mac_addr[4],
								mac_addr[5]);
						break;
					}
				} else {
					log_error("%s", "get_mac_addr(): Error using ioctl SIOCGIFHWADDR.");
				}
			}
		}
	}

done:
	if (sock != -1)
		close(sock);
	free(buf);
	return retval;
}

/**
 * get_interface_mac_addr() - Get the MAC address of the interface if matches the pattern
 *
 * @iface:		The interface to check its MAC address.
 * @mac_addr:	Pointer to store the MAC address.
 * @pattern:	Pattern for the name of the interface.
 *
 * Return: The MAC address of the interface, or NULL if it does not matches.
 */
uint8_t *get_interface_mac_addr(const struct ifreq *const iface,
		uint8_t *const mac_addr, const char *const pattern)
{
	regex_t regex;
	char msgbuf[100];
	uint8_t *retval = NULL;

	const char *name = iface->ifr_name;

	int error = regcomp(&regex, pattern, REG_EXTENDED);
	if (error != 0) {
		regerror(error, &regex, msgbuf, sizeof(msgbuf));
		log_error("get_interface_mac_addr(): Could not compile regex: %s (%d)", msgbuf, error);
		goto done;
	}
	error = regexec(&regex, name, 0, NULL, 0);
	if (error != 0)
		goto done;

	memcpy(mac_addr, iface->ifr_hwaddr.sa_data, 6);
	retval = mac_addr;

done:
	regfree(&regex);
	return retval;
}
