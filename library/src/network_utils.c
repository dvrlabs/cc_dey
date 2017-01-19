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
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

#include "ccimp/dns_helper.h"
#include "ccimp/ccimp_network.h"
#include "network_utils.h"
#include "cc_logging.h"

#define cast_for_alignment(cast, ptr)	((cast) ((void *) (ptr)))

/*
 * get_iface_info() - Retrieve information about the network interface used to
 * 					  connect to url.
 *
 * @url:		URL to connect to to determine network interface.
 * @iface_info:	Struct to fill with the network interface information
 *
 * Return: 0 on success, -1 otherwise.
 */
int get_iface_info(const char *url, iface_info_t *iface_info)
{
	unsigned int const ipv4_len = 4;
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	int retval = -1;
	struct sockaddr_in sin = {0};
	struct sockaddr_in info;
	in_addr_t ip_addr = {0};
	int sockfd = -1;
	struct ifreq ifr;
	socklen_t len = sizeof(struct sockaddr);

	/* 1 - Open a connection to url */
	if (dns_resolve(url, &ip_addr) != 0) {
		log_error("%s: dns_resolve() failed (url: %s)", __func__, url);
		goto done;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
		log_error("%s: socket() failed", __func__);
		goto done;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip_addr;
#if (defined APP_SSL)
	sin.sin_port = htons(CCIMP_SSL_PORT);
#else
	sin.sin_port = htons(CCIMP_TCP_PORT);
#endif

	if(connect(sockfd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) < 0) {
		log_error("%s: connect() failed", __func__);
		goto done;
	}

	if (getsockname(sockfd, (struct sockaddr *) &info, &len) < 0) {
		log_error("%s: getsockname() failed", __func__);
		goto done;
	}

	/* 2 - Get the name of the interface used */
	if (getifaddrs(&ifaddr) == -1) {
		log_error("%s: getifaddrs() failed", __func__);
		goto done;
	}
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET) {
			char buf[32];
			struct sockaddr_in * const sa = cast_for_alignment(struct sockaddr_in *, ifa->ifa_addr);
			char *ipv4_string;

			inet_ntop(ifa->ifa_addr->sa_family, (void *)&(sa->sin_addr), buf, sizeof(buf));
			ipv4_string = inet_ntoa(info.sin_addr);
			if (!strcmp(ipv4_string, buf)) {
				strncpy(ifr.ifr_name, ifa->ifa_name, sizeof(ifr.ifr_name));
				if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) != 0) {
					log_error("%s: ioctl SIOCGIFFLAGS failed", __func__);
					goto done;
				}

				strncpy(iface_info->name, ifa->ifa_name, sizeof(iface_info->name));
				memcpy(iface_info->mac_addr, ifr.ifr_hwaddr.sa_data, sizeof(iface_info->mac_addr));
				memcpy(iface_info->ipv4_addr, &(info.sin_addr), ipv4_len);
				log_debug("%s: Interface name [%s] - IP Address [%s] - MAC [%02x:%02x:%02x:%02x:%02x:%02x]",
						  __func__, ifa->ifa_name, ipv4_string,
						  iface_info->mac_addr[0], iface_info->mac_addr[1], iface_info->mac_addr[2],
						  iface_info->mac_addr[3], iface_info->mac_addr[4], iface_info->mac_addr[5]);
				retval = 0;
				goto done;
			}
		}
	}

done:
	freeifaddrs(ifaddr);
	if (sockfd > 0)
		close(sockfd);

	return retval;
}

/**
 * get_mac_addr() - Get the primary MAC address of the device.
 *
 * This is not guaranteed to be the MAC of the active network interface, and
 * should be only used for device identification purposes, where the same MAC
 * is desired no matter which network interface is active.
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
