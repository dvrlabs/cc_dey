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
#define ARRAY_SIZE(array)  				(sizeof array/sizeof array[0])

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
 * compare_iface() - Provide an ordering for network interfaces by their name.
 *
 * @n1:		Name of the first network interface.
 * @n2: 	Name of the second network interface.
 *
 * The interfaces priority order is the following:
 *   - Ethernet (eth0, eth1, ...)
 *   - Wi-Fi (wlan0, wlan1, ...)
 *   - No interface (empty string)
 *   - Other interface (any other string)
 *
 * Return:
 *      >0 when n1 > n2
 *       0 when n1 = n2
 *      <0 when n1 < n2
 */
static int compare_iface(const char *n1, const char *n2) {

	const char *patterns[] = { "^eth([0-9]{1,3})$", "^wlan([0-9]{1,3})$", "^$"};
	regmatch_t match_group[2];
	regex_t regex;
	size_t i;
	int retvalue = 1;
	char msgbuf[128];

	for (i = 0; i < ARRAY_SIZE(patterns); i++) {
		int error = regcomp(&regex, patterns[i], REG_EXTENDED);
		if (error != 0) {
			regerror(error, &regex, msgbuf, sizeof(msgbuf));
			log_error("compare_iface(): Could not compile regex: %s (%d)", msgbuf, error);
			regfree(&regex);
			goto done;
		}
		if (regexec(&regex, n1, 0, NULL, 0) != REG_NOMATCH &&
			regexec(&regex, n2, 0, NULL, 0) == REG_NOMATCH) {
			/* Only the first matches: n1 > n2 */
			retvalue = 1;
			regfree(&regex);
			break;
		} else if (regexec(&regex, n1, 0, NULL, 0) == REG_NOMATCH &&
				regexec(&regex, n2, 0, NULL, 0) != REG_NOMATCH) {
			/* Only the second matches: n2 > n1 */
			retvalue = -1;
			regfree(&regex);
			break;
		} else if (regexec(&regex, n1, 2, match_group, 0) != REG_NOMATCH &&
				regexec(&regex, n2, 0, NULL, 0) != REG_NOMATCH) {
			/* If both matches, use the number to decide */
			int j1 = atoi(n1 + match_group[1].rm_so);
			int j2 = atoi(n2 + match_group[1].rm_so);
			retvalue = j2 - j1;
			regfree(&regex);
			break;
		} else {
			/* If none matches, try the next pattern */
			regfree(&regex);
		}
	}

done:
	return retvalue;
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
 * The interfaces priority order is as specified in the compare_iface function above.
 *
 * Return: The MAC address of primary interface.
 */
uint8_t *get_mac_addr(uint8_t *const mac_addr)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	struct ifreq ifr;
	uint8_t *retval = NULL;
	iface_info_t iface = {0};
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		log_error("%s: socket() failed", __func__);
		goto done;
	}

	if (getifaddrs(&ifaddr) == -1) {
		log_error("%s: getifaddrs() failed", __func__);
		goto done;
	}

	/* iterate over all the interfaces and keep the best one */
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_PACKET &&
				compare_iface(iface.name, ifa->ifa_name) < 0) {
			strncpy(ifr.ifr_name, ifa->ifa_name, sizeof(ifr.ifr_name));
			if (ioctl(sock, SIOCGIFHWADDR, &ifr) != 0) {
				log_error("%s: ioctl SIOCGIFFLAGS failed", __func__);
				goto done;
			}
			memcpy(iface.mac_addr, ifr.ifr_hwaddr.sa_data, 6);
			strncpy(iface.name, ifa->ifa_name, sizeof(iface.name));
			log_debug("%s: Found better interface %s - MAC %02x:%02x:%02x:%02x:%02x:%02x",
					__func__, ifa->ifa_name, iface.mac_addr[0],
					iface.mac_addr[1], iface.mac_addr[2],
					iface.mac_addr[3], iface.mac_addr[4],
					iface.mac_addr[5]);
		}
	}

	/* return the best interface found (if any) */
	if (iface.name[0] == '\0') {
		log_error("%s: no valid network interface", __func__);
		retval = NULL;
	} else {
		memcpy(mac_addr, iface.mac_addr, sizeof(iface.mac_addr));
		retval = mac_addr;
	}

done:
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);
	if (sock > 0)
		close(sock);
	return retval;
}
