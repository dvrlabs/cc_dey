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

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <regex.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "ccimp/ccimp_network.h"
#include "ccimp/dns_helper.h"
#include "cc_logging.h"
#include "network_utils.h"

#define cast_for_alignment(cast, ptr)	((cast) ((void *) (ptr)))
#define ARRAY_SIZE(array)				(sizeof array/sizeof array[0])

#define DNS_FILE						"/etc/resolv.conf"
#define DNS_ENTRY						"nameserver"

static ccapi_bool_t interface_exists(const char *iface_name);
static int get_dns(uint8_t *dnsaddr1, uint8_t *dnsaddr2);
static int get_gateway(const char *iface_name, uint8_t *gateway);
static int get_mac_address(const char *iface_name, uint8_t *mac_address);
static ccapi_bool_t is_dhcp(const char *iface_name);
static int compare_iface(const char *n1, const char *n2);

/*
 * get_main_iface_info() - Retrieve information about the network
 *                         interface used to connect to url.
 *
 * @url:		URL to connect to to determine main network interface.
 * @iface_info:	Struct to fill with the network interface information
 *
 * Return: 0 on success, -1 otherwise.
 */
int get_main_iface_info(const char *url, iface_info_t *iface_info)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	int retval = -1;
	struct sockaddr_in sin = {0};
	struct sockaddr_in info = {0};
	in_addr_t ip_addr = {0};
	int sockfd = -1;
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

	/* 2 - Determine the interface used to connect */
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
			if (strcmp(ipv4_string, buf) == 0) {
				/* 3 - Get the interface info */
				if (get_iface_info(ifa->ifa_name, iface_info) == 0)
					retval = 0;
				break;
			}
		}
	}

done:
	freeifaddrs(ifaddr);
	if (sockfd >= 0)
		close(sockfd);

	return retval;
}

/*
 * get_iface_info() - Retrieve information about the given network interface.
 *
 * @name:		Network interface name.
 * @iface_info:	Struct to fill with the network interface information
 *
 * Return: 0 on success, -1 otherwise.
 */
int get_iface_info(const char *iface_name, iface_info_t *iface_info)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	struct sockaddr_in *sin;

	/* Clear all values */
	memset(iface_info, 0, sizeof (iface_info_t));
	/* Check if interface exists */
	if (!interface_exists(iface_name)) {
		log_error("%s: Interface '%s' not found\n", __func__, iface_name);
		return -1;
	}
	/* Fill interface name */
	strncpy(iface_info->name, iface_name, IFNAMSIZ - 1);
	/* Fill MAC address */
	get_mac_address(iface_name, iface_info->mac_addr);
	if (getifaddrs(&ifaddr) == -1) {
		log_error("%s: getifaddrs() failed", __func__);
		return -1;
	}
	/* Fill IPv4, subnet mask and enabled status */
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET
				&& strncmp(iface_name, ifa->ifa_name, strlen(iface_name)) == 0) {
			/* Set IPv4 address */
			sin = cast_for_alignment(struct sockaddr_in *, ifa->ifa_addr);
			memcpy(iface_info->ipv4_addr, &(sin->sin_addr), IPV4_GROUPS);
			/* Set subnet mask address */
			sin = cast_for_alignment(struct sockaddr_in *, ifa->ifa_netmask);
			memcpy(iface_info->submask, &(sin->sin_addr), IPV4_GROUPS);
			/* Set enablement status */
			if ((ifa->ifa_flags & IFF_UP) == IFF_UP)
				iface_info->enabled = CCAPI_TRUE;
			else
				iface_info->enabled = CCAPI_FALSE;
		}
	}
	/* Fill DNS addresses */
	get_dns(iface_info->dnsaddr1, iface_info->dnsaddr2);
	/* Fill gateway address */
	get_gateway(iface_name, iface_info->gateway);
	/* Fill DHCP flag */
	iface_info->dhcp = is_dhcp(iface_name);

	if (ifaddr != NULL)
		freeifaddrs(ifaddr);

	return 0;
}

/**
 * get_primary_mac_address() - Get the primary MAC address of the device.
 *
 * This is not guaranteed to be the MAC of the active network interface, and
 * should be only used for device identification purposes, where the same MAC
 * is desired no matter which network interface is active.
 *
 * The interfaces priority order is the following:
 *   - Ethernet (eth0, eth1, ...)
 *   - Wi-Fi (wlan0, wlan1, ...)
 *   - No interface (empty string)
 *   - Other interface (any other string)
 *
 * @mac_addr:	Pointer to store the MAC address.
 *
 * Return: The MAC address of primary interface.
 */
uint8_t *get_primary_mac_address(uint8_t *const mac_addr)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	uint8_t *retval = NULL;
	iface_info_t iface = {{0}};

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
			if (get_mac_address(ifa->ifa_name, iface.mac_addr) != 0)
				goto done;
			strncpy(iface.name, ifa->ifa_name, IFNAMSIZ - 1);
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

	return retval;
}

/**
 * interface_exists() - Check if provided interface exists or not.
 *
 * @iface_name:	Name of the network interface to check
 *
 * Return: CCAPI_TRUE if interface exists, CCAPI_FALSE otherwise.
 */
static ccapi_bool_t interface_exists(const char *iface_name)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	ccapi_bool_t exists = CCAPI_FALSE;

	if (iface_name == NULL || strlen(iface_name) == 0)
		goto done;
	if (getifaddrs(&ifaddr) == -1) {
		log_error("%s: getifaddrs() failed", __func__);
		goto done;
	}
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if ((ifa->ifa_addr->sa_family == AF_PACKET || ifa->ifa_addr->sa_family == AF_INET) &&
		     strncmp(iface_name, ifa->ifa_name, strlen(iface_name)) == 0) {
			exists = CCAPI_TRUE;
			break;
		}
	}

done:
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);

	return exists;
}

/**
 * get_mac_address() - Get the MAC address of the given interface.
 *
 * @iface_name: Name of the network interface to retrieve its MAC address
 * @mac_addr:	Pointer to store the MAC address.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_mac_address(const char *iface_name, uint8_t *mac_address)
{
	struct ifreq ifr;
	int sock = -1;
	int ret = 0;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		log_error("%s: socket() failed", __func__);
		ret = -1;
		goto done;
	}
	strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) != 0) {
		log_error("%s: ioctl SIOCGIFFLAGS failed", __func__);
		ret = -1;
		goto done;
	}
	memcpy(mac_address, ifr.ifr_hwaddr.sa_data, MAC_ADDRESS_GROUPS);

done:
	if (sock >= 0)
		close(sock);

	return ret;
}

/**
 * get_dns() - Get the DNS addresses of the given interface.
 *
 * @dnsaddr1: Pointer to store the primary DNS address.
 * @dnsaddr2: Pointer to store the secondary DNS address.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_dns(uint8_t *dnsaddr1, uint8_t *dnsaddr2)
{
	char line[255];
	char dns_str[20];
	int i = 0;
	struct in_addr iaddr;
	FILE *fp;

	/* Get the DNS addresses from system */
	if ((fp = fopen(DNS_FILE, "r")) == NULL) {
		printf("%s: fopen error on %s", __func__, DNS_FILE);
		return -1;
	}
	while (fgets(line, sizeof(line) - 1, fp) && i < MAX_DNS_ADDRESSES) {
		if (strncmp(line, DNS_ENTRY, strlen(DNS_ENTRY)) == 0) {
			/* This is a DNS entry */
			if (sscanf(line, "%*s %s", dns_str)) {
				if (inet_aton(dns_str, &iaddr)) {
					if (i == 0)
						memcpy(dnsaddr1, &iaddr, IPV4_GROUPS);
					else
						memcpy(dnsaddr2, &iaddr, IPV4_GROUPS);
					i += 1;
				} else {
					log_error("%s: couldn't convert '%s' into a valid IP\n", __func__, dns_str);
				}
			}
		}
	}
	fclose(fp);

	return 0;
}

/**
 * get_gateway() - Get the gateway address of the given interface.
 *
 * @iface_name:	Name of the network interface to retrieve its gateway address
 * @gateway:	Pointer to store the gateway address.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_gateway(const char *iface_name, uint8_t *gateway)
{
	char cmd[64] = {0};
	char line[255] = {0};
	struct in_addr iaddr;
	FILE *fp;

	/* Get the gateway addresses from command execution */
	sprintf(cmd, "route -n | grep %s | grep 'UG[ \t]' | awk '{print $2}'", iface_name);
	fp = popen(cmd, "r");
	if (fp == NULL) {
		log_error("%s: couldn't execute command '%s'\n", __func__, cmd);
		return -1;
	}
	if (fgets(line, sizeof(line) - 1, fp) == NULL) {
		pclose(fp);
		return -1;
	}
	pclose(fp);
	if (!inet_aton(line, &iaddr)) {
		log_error("%s: couldn't convert '%s' into a valid IP\n", __func__, line);
		return -1;
	}
	memcpy(gateway, &iaddr, IPV4_GROUPS);

	return 0;
}

/**
 * is_dhcp() - Check if provided interface uses DHCP or not.
 *
 * @iface_name:	Name of the network interface to retrieve its DHCP status
 *
 * Return: CCAPI_TRUE if interface uses DHCP, CCAPI_FALSE otherwise.
 */
static ccapi_bool_t is_dhcp(const char *iface_name)
{
	char cmd[64] = {0};
	char line[255] = {0};
	FILE *fp;

	/* Use NetworkManager to obtain the given interface method */
	sprintf(cmd, "nmcli conn show %s | grep \"ipv[4|6].method\" | grep auto", iface_name);
	fp = popen(cmd, "r");
	if (fp == NULL) {
		log_error("%s: couldn't execute command '%s'\n", __func__, cmd);
		return CCAPI_FALSE;
	}
	if (fgets(line, sizeof(line) - 1, fp) == NULL) {
		pclose(fp);
		return CCAPI_FALSE;
	}
	pclose(fp);

	return CCAPI_TRUE;
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
static int compare_iface(const char *n1, const char *n2)
{
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
