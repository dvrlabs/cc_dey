/*
 * utils.c
 *
 * Copyright (C) 2016 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi.
 * International Inc. By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered. This is an unpublished copyrighted work of Digi International
 * Inc. Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to restrictions
 * set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
 * Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and
 * (2) of the Commercial Computer Software - Restricted Rights at 48 CFR
 * 52.227-19, as applicable.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 *
 * Description: Utils.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <regex.h>
#include <ctype.h>

#include "utils.h"
#include "device_request.h"
#include "firmware_update.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define DEVICE_ID_FORMAT	"%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX"

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static ccapi_bool_t tcp_reconnect_cb(ccapi_tcp_close_cause_t cause);
static int get_device_id_from_mac(uint8_t * const device_id,
		const uint8_t * const mac_addr);
static int get_ipv4_address(uint8_t * const ipv4_addr);
static uint8_t * get_mac_addr(uint8_t * const mac_addr);
static uint8_t * get_interface_mac_addr(const struct ifreq * const iface,
		uint8_t * const mac_addr, const char * const pattern);

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
/*
 * create_ccapi_start_struct() - Create a ccapi_start_t struct from the given config
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * Return: The created ccapi_start_t struct with the data read from the
 *         configuration file.
 */
ccapi_start_t * create_ccapi_start_struct(const cc_cfg_t * const cc_cfg)
{
	uint8_t mac_address[6];

	ccapi_start_t * start = malloc(sizeof *start);
	if (start == NULL) {
		log_error("create_ccapi_start_struct(): malloc failed for ccapi_start_t");
		return start;
	}

	start->device_cloud_url = cc_cfg->url;
	start->device_type = cc_cfg->device_type;
	start->vendor_id = cc_cfg->vendor_id;
	if (get_device_id_from_mac(start->device_id, get_mac_addr(mac_address)) != 0) {
		log_error("create_ccapi_start_struct(): cannot calculate device id");
		free_ccapi_start_struct(start);
		start = NULL;
		return start;
	}

	start->status = NULL;

	/* Initialize CLI service. */
	start->service.cli = NULL;

	/* Initialize RCI service. */
	start->service.rci = NULL;

	/* Initialize device request service . */
	if (cc_cfg->services & DATA_SERVICE) {
		ccapi_receive_service_t * dreq_service = malloc(sizeof *dreq_service);
		if (dreq_service == NULL) {
			log_error("create_ccapi_start_struct(): malloc failed for ccapi_receive_service_t");
			free_ccapi_start_struct(start);
			start = NULL;
			return start;
		}
		dreq_service->accept = app_receive_default_accept_cb;
		dreq_service->data = app_receive_default_data_cb;
		dreq_service->status = app_receive_default_status_cb;
		start->service.receive = dreq_service;
	} else {
		start->service.receive = NULL;
	}

	/* Initialize short messaging. */
	start->service.sm = NULL;

	/* Initialize file system service. */
	if (cc_cfg->services & FS_SERVICE) {
		ccapi_filesystem_service_t * fs_service = malloc(sizeof *fs_service);
		if (fs_service == NULL) {
			log_error("create_ccapi_start_struct(): malloc failed for ccapi_filesystem_service_t");
			free_ccapi_start_struct(start);
			start = NULL;
			return start;
		}
		fs_service->access = NULL;
		fs_service->changed = NULL;
		start->service.file_system = fs_service;
	} else {
		start->service.file_system = NULL;
	}

	/* Initialize firmware service. */
	start->service.firmware = NULL;
	/* TODO */
	if (cc_cfg->fw_version == NULL) {
		start->service.firmware = NULL;
	} else {
		unsigned int fw_version_major;
		unsigned int fw_version_minor;
		unsigned int fw_version_revision;
		unsigned int fw_version_build;
		int error;

		error = sscanf(cc_cfg->fw_version, "%u.%u.%u.%u", &fw_version_major,
				&fw_version_minor, &fw_version_revision, &fw_version_build);
		if (error != 4) {
			log_error("Bad firmware_version string '%s', firmware update disabled",
					cc_cfg->fw_version);
			start->service.firmware = NULL;
		} else {
			ccapi_firmware_target_t * const fw_list = malloc(sizeof *fw_list);
			ccapi_fw_service_t * const fw_service = malloc(sizeof *fw_service);

			if (fw_list == NULL) {
				log_error("create_ccapi_start_struct(): malloc failed for ccapi_firmware_target_t");
				free_ccapi_start_struct(start);
				start = NULL;
				return start;
			}

			if (fw_service == NULL) {
				log_error("create_ccapi_start_struct(): malloc failed for ccapi_fw_service_t");
				free_ccapi_start_struct(start);
				start = NULL;
				return start;
			}

			fw_list->chunk_size = 0;
			fw_list->description = "System image";
			fw_list->filespec = ".*\\.[zZ][iI][pP]";
			fw_list->maximum_size = 0;
			fw_list->version.major = (uint8_t) fw_version_major;
			fw_list->version.minor = (uint8_t) fw_version_minor;
			fw_list->version.revision = (uint8_t) fw_version_revision;
			fw_list->version.build = (uint8_t) fw_version_build;

			fw_service->target.count = 1;
			fw_service->target.item = fw_list;

			fw_service->callback.request = app_fw_request_cb;
			fw_service->callback.data = app_fw_data_cb;
			fw_service->callback.reset = app_fw_reset_cb;
			fw_service->callback.cancel = app_fw_cancel_cb;

			start->service.firmware = fw_service;
		}
	}

	return start;
}

/*
 * create_ccapi_tcp_start_info_struct() - Generate a ccapi_tcp_info_t struct
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * Return: The created ccapi_start_t struct with the data read from the
 *         configuration file.
 */
ccapi_tcp_info_t * create_ccapi_tcp_start_info_struct(const cc_cfg_t * const cc_cfg)
{
	ccapi_tcp_info_t * tcp_info = malloc(sizeof *tcp_info);
	if (tcp_info == NULL) {
		log_error("create_ccapi_tcp_start_info_struct(): malloc failed for ccapi_tcp_info_t");
		return tcp_info;
	}

	tcp_info->callback.close = NULL;
	if (cc_cfg->enable_reconnect)
		tcp_info->callback.close = tcp_reconnect_cb;

	tcp_info->callback.keepalive = NULL;

	tcp_info->connection.type = CCAPI_CONNECTION_LAN;
	tcp_info->connection.max_transactions = 0;
	tcp_info->connection.password = NULL;
	tcp_info->connection.start_timeout = 10;
	tcp_info->connection.ip.type = CCAPI_IPV4;
	if (get_ipv4_address(tcp_info->connection.ip.address.ipv4) != 0
			|| get_mac_addr(tcp_info->connection.info.lan.mac_address) == NULL) {
		free_ccapi_tcp_start_info_struct(tcp_info);
		tcp_info = NULL;
		return tcp_info;
	}

	tcp_info->keepalives.rx = cc_cfg->keepalive_rx;
	tcp_info->keepalives.tx = cc_cfg->keepalive_tx;
	tcp_info->keepalives.wait_count = cc_cfg->wait_count;

	return tcp_info;
}

/**
 * free_ccapi_start_struct() - Release the ccapi_start_t struct
 *
 * @ccapi_start:	CCAPI start struct (ccapi_start_t) to be released.
 */
void free_ccapi_start_struct(ccapi_start_t * ccapi_start)
{
	if (ccapi_start != NULL) {
		if (ccapi_start->service.firmware != NULL)
			free(ccapi_start->service.firmware->target.item);

		free(ccapi_start->service.firmware);
		free(ccapi_start->service.file_system);
		free(ccapi_start);
	}
}

/**
 * free_ccapi_tcp_start_info_struct() - Release the ccapi_tcp_info_t struct
 *
 * @ccapi_start:	CCAPI TCP info struct (ccapi_tcp_info_t) to be released.
 */
void free_ccapi_tcp_start_info_struct(ccapi_tcp_info_t * const tcp_info)
{
	free(tcp_info);
}

/**
 * add_virtual_directories() - Add defined virtual directories
 *
 * @vdirs:		List of virtual directories
 * @n_vdirs:	Number of elements in the list
 */
void add_virtual_directories(const vdir_t * const vdirs, int n_vdirs)
{
	int i;

	for (i = 0; i < n_vdirs; i++) {
		ccapi_fs_error_t add_dir_error;
		const vdir_t * v_dir = vdirs + i;

		log_info("New virtual directory %s (%s)", v_dir->name, v_dir->path);
		add_dir_error = ccapi_fs_add_virtual_dir(v_dir->name, v_dir->path);
		if (add_dir_error != CCAPI_FS_ERROR_NONE)
			log_error("add_virtual_directories() failed with error %d", add_dir_error);
	}
}

/**
 * file_exists() - Check that the file with the given name exists
 *
 * @filename:	Full path of the file to check if it exists.
 *
 * Return: 1 if the file exits, 0 if it does not exist.
 */
int file_exists(const char * const filename)
{
	return access(filename, F_OK) == 0;
}

/**
 * file_readable() - Check that the file with the given name can be read
 *
 * @filename:	Full path of the file to check if it is readable.
 *
 * Return: 1 if the file is readable, 0 if it cannot be read.
 */
int file_readable(const char * const filename)
{
	return access(filename, R_OK) == 0;
}

/**
 * tcp_close_cb() - Callback to tell if Cloud Connector should reconnect
 *
 * @cause:	Reason of the disconnection (disconnection, redirection, missing
 * 			keep alive, or any other data error).
 *
 * Return: CCAPI_TRUE if Cloud Connector should reconnect, CCAPI_FALSE otherwise.
 */
static ccapi_bool_t tcp_reconnect_cb(ccapi_tcp_close_cause_t cause)
{
	log_debug("ccapi_tcp_close_cb cause %d", cause);
	return CCAPI_TRUE;
}

/*
 * get_device_id_from_mac() - Generate a Device ID from a given MAC address
 *
 * @device_id:	Pointer to store the generated Device ID.
 * @mac_addr:	MAC address to generate the Device ID.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_device_id_from_mac(uint8_t * const device_id, const uint8_t * const mac_addr)
{
	const char * const deviceid_file = "/etc/cc.did";
	unsigned int const device_id_length = 16;
	FILE * fp = NULL;
	unsigned int n_items;

	memset(device_id, 0x00, device_id_length);

	fp = fopen(deviceid_file, "rb+");
	if (fp != NULL) {
		n_items = fscanf(fp, DEVICE_ID_FORMAT, &device_id[0], &device_id[1],
				&device_id[2], &device_id[3], &device_id[4], &device_id[5],
				&device_id[6], &device_id[7], &device_id[8], &device_id[9],
				&device_id[10], &device_id[11], &device_id[12], &device_id[13],
				&device_id[14], &device_id[15]);
		fclose(fp);
		if (n_items == device_id_length)
			return 0;
	}

	if (mac_addr == NULL)
		return -1;

	device_id[8] = mac_addr[0];
	device_id[9] = mac_addr[1];
	device_id[10] = mac_addr[2];
	device_id[11] = 0xFF;
	device_id[12] = 0xFF;
	device_id[13] = mac_addr[3];
	device_id[14] = mac_addr[4];
	device_id[15] = mac_addr[5];

	fp = fopen(deviceid_file, "wb+");
	if (fp != NULL) {
		n_items = fprintf(fp, DEVICE_ID_FORMAT, device_id[0], device_id[1],
				device_id[2], device_id[3], device_id[4], device_id[5],
				device_id[6], device_id[7], device_id[8], device_id[9],
				device_id[10], device_id[11], device_id[12], device_id[13],
				device_id[14], device_id[15]);
		fclose(fp);
		if (n_items != device_id_length * 2 + 3)
			log_debug("Could not store Device ID");
	}

	return 0;
}

/*
 * get_ipv4_address() - Retrieves the IPv4 address of the first interface
 *
 * @ipv4_addr:	Pointer to store the IPv4.
 *
 * The interface must be different from loopback.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_ipv4_address(uint8_t * const ipv4_addr)
{
	unsigned int const ipv4_len = 4;
	struct ifaddrs * ifaddr = NULL, *ifa = NULL;
	int retval = -1;

	if (getifaddrs(&ifaddr) == -1) {
		log_error("get_ipv4_address(): getifaddrs() failed");
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
				log_info("get_ipv4_address(): Interface name [%s] - IP Address [%s]\n", ifa->ifa_name, host);
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
static uint8_t * get_mac_addr(uint8_t * const mac_addr)
{
	unsigned int const max_interfaces = 8;
	size_t const buf_size = max_interfaces * sizeof(struct ifreq);
	struct ifconf ifconf;
	uint8_t * retval = NULL;
	int sock = -1;

	char * const buf = malloc(sizeof(char) * buf_size);
	if (buf == NULL) {
		log_error("get_mac_addr(): malloc failed for char");
		goto done;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		log_error("get_mac_addr(): socket() failed");
		goto done;
	}

	ifconf.ifc_len = buf_size;
	ifconf.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0) {
		log_error("get_mac_addr(): Error using ioctl SIOCGIFCONF.\n");
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
			struct ifreq * ifreq = &ifconf.ifc_req[i];

			strcpy(ifr.ifr_name, ifreq->ifr_name);
			if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
				log_error("get_mac_addr(): Error using ioctl SIOCGIFFLAGS.\n");
				continue;
			}

			if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
					if (get_interface_mac_addr(&ifr, mac_addr, "^eth([0-9]{1,3})$") != NULL
							|| get_interface_mac_addr(&ifr, mac_addr, "^wlan([0-9]{1,3})$") != NULL) {
						memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);
						retval = mac_addr;
						log_info("Primary interface %s - MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
								ifr.ifr_name, mac_addr[0], mac_addr[1],
								mac_addr[2], mac_addr[3], mac_addr[4],
								mac_addr[5]);
						break;
					}
				} else {
					log_error("get_mac_addr(): Error using ioctl SIOCGIFHWADDR.\n");
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
static uint8_t * get_interface_mac_addr(const struct ifreq * const iface,
		uint8_t * const mac_addr, const char * const pattern)
{
	regex_t regex;
	char msgbuf[100];
	uint8_t * retval = NULL;

	const char * name = iface->ifr_name;

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

/**
 * strltrim() - Remove leading whitespaces from the given string
 *
 * @s: String to remove leading whitespaces.
 *
 * Return: New string without leading whitespaces.
 */
char * strltrim(const char *s)
{
	while (isspace(*s) || !isprint(*s))
		++s;

	return strdup(s);
}

/**
 * strrtrim() - Remove trailing whitespaces from the given string
 *
 * @s: String to remove trailing whitespaces.
 *
 * Return: New string without trailing whitespaces.
 */
char * strrtrim(const char * s)
{
	char *r = strdup(s);

	if (r != NULL) {
		char *fr = r + strlen(s) - 1;
		while ((isspace(*fr) || !isprint(*fr) || *fr == 0) && fr >= r)
			--fr;
		*++fr = 0;
	}

	return r;
}

/**
 * strtrim() - Remove leading and trailing whitespaces from the given string
 *
 * @s: String to remove leading and trailing whitespaces.
 *
 * Return: New string without leading and trailing whitespaces.
 */
char * strtrim(const char * s)
{
	char *r = strrtrim(s);
	char *f = strltrim(r);
	free(r);

	return f;
}
