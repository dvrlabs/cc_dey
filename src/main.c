/*
 * main.c
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
 * Description: Cloud Connector main file.
 *
 */

#include <stdio.h>
#include <unistd.h>

#include "ccapi/ccapi.h"
#include "ccimp/utils.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define CC_CONFIG_FILE		"/etc/cc.conf"

#define START_ERROR			"ccapi_start error %d"
#define START_TCP_ERROR		"ccapi_start_transport_tcp failed with error %d"
#define STOP_SUCCESS		"ccapi_stop success"
#define STOP_ERROR			"ccapi_stop error %d"

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
extern void add_sigkill_signal(void);
extern ccapi_dp_error_t start_system_monitor(const cc_cfg_t * const cc_cfg);
extern void stop_system_monitor(void);
static ccapi_start_error_t app_start_ccapi(const cc_cfg_t * const cc_cfg);
static ccapi_tcp_start_error_t app_start_tcp_transport(const cc_cfg_t * const cc_cfg);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
ccapi_bool_t stop = CCAPI_FALSE;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
int main(void)
{
	cc_cfg_t cc_cfg;
	int log_options = LOG_CONS | LOG_NDELAY | LOG_PID;
	int error;

	add_sigkill_signal();

	error = parse_configuration(CC_CONFIG_FILE, &cc_cfg);
	if (error != 0)
		return EXIT_FAILURE;

	if (cc_cfg.log_console)
		log_options = log_options | LOG_PERROR;
	init_logger(cc_cfg.log_level, log_options);

	if (app_start_ccapi(&cc_cfg) != CCAPI_START_ERROR_NONE)
		goto done;

	add_virtual_directories(cc_cfg.vdirs, cc_cfg.n_vdirs);

	if (app_start_tcp_transport(&cc_cfg) != CCAPI_TCP_START_ERROR_NONE)
		goto done;

	start_system_monitor(&cc_cfg);
	do {
		sleep(1);
	} while (check_stop() != CCAPI_TRUE);

done:
	free_cfg(&cc_cfg);
	closelog();
	return EXIT_SUCCESS;
}

/*
 * check_stop() - Stop Cloud Connector
 *
 * Return: CCAPI_TRUE if Cloud Connector is successfully stopped, CCAPI_FALSE
 *         otherwise.
 */
ccapi_bool_t check_stop(void)
{
	ccapi_stop_error_t stop_error = CCAPI_STOP_ERROR_NONE;

	if (stop == CCAPI_TRUE) {
		stop_system_monitor();
		stop_error = ccapi_stop(CCAPI_STOP_GRACEFULLY);

		if (stop_error == CCAPI_STOP_ERROR_NONE)
			log_info(STOP_SUCCESS);
		else
			log_error(STOP_ERROR, stop_error);
	}
	return stop;
}

/*
 * app_start_ccapi() - Start CCAPI layer
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the settings parsed
 * 			from the configuration file are stored.
 *
 * Return: CCAPI_START_ERROR_NONE on success, any other ccapi_start_error_t
 *         otherwise.
 */
static ccapi_start_error_t app_start_ccapi(const cc_cfg_t * const cc_cfg)
{
	ccapi_start_t * start = NULL;
	ccapi_start_error_t error;

	start = create_ccapi_start_struct(cc_cfg);
	if (start == NULL)
		return CCAPI_START_ERROR_NULL_PARAMETER;

	error = ccapi_start(start);
	if (error != CCAPI_START_ERROR_NONE)
		log_error(START_ERROR, error);

	free_ccapi_start_struct(start);
	return error;
}

/*
 * app_start_tcp_transport() - Start TCP transport
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the settings parsed
 * 			from the configuration file are stored.
 *
 * Return: CCAPI_TCP_START_ERROR_NONE on success, any other
 *         ccapi_tcp_start_error_t otherwise.
 */
static ccapi_tcp_start_error_t app_start_tcp_transport(const cc_cfg_t * const cc_cfg)
{
	ccapi_tcp_info_t * tcp_info = NULL;
	ccapi_tcp_start_error_t error;

	tcp_info = create_ccapi_tcp_start_info_struct(cc_cfg);
	if (tcp_info == NULL)
		return CCAPI_TCP_START_ERROR_NULL_POINTER;

	do {
		error = ccapi_start_transport_tcp(tcp_info);
		if (error != CCAPI_TCP_START_ERROR_NONE) {
			if (error == CCAPI_TCP_START_ERROR_TIMEOUT) {
				log_info("ccapi_start_transport_tcp() timed out, retrying in 30 seconds");
				sleep(30);
			} else {
				log_error(START_TCP_ERROR, error);
				break;
			}
		}
	} while (error == CCAPI_TCP_START_ERROR_TIMEOUT);

	free_ccapi_tcp_start_info_struct(tcp_info);
	return error;
}

