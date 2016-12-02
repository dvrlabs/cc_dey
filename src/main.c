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
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

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
static void daemon_process(void);
static ccapi_start_error_t app_start_ccapi(const cc_cfg_t * const cc_cfg);
static ccapi_tcp_start_error_t app_start_tcp_transport(const cc_cfg_t * const cc_cfg);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
ccapi_bool_t stop = CCAPI_FALSE;
static cc_cfg_t * cc_cfg = NULL;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
int main(void)
{
	pid_t pid, sid;

	/* Fork off the parent process. */
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	sid = setsid();
	if (sid < 0)
		exit(EXIT_FAILURE);

	/* Change the current working directory. */
	if (chdir("/") < 0)
		exit(EXIT_FAILURE);

	/* Close out the standard file descriptors. */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	add_sigkill_signal();
	daemon_process();
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
		free_cfg(cc_cfg);
		closelog();
	}
	return stop;
}

/*
 * daemon_process() - Start Cloud Connector
 */
static void daemon_process(void)
{
	int log_options = LOG_CONS | LOG_NDELAY | LOG_PID;
	int error;

	init_logger(LOG_ERR, log_options);

	cc_cfg = malloc(sizeof(cc_cfg_t));
	if (cc_cfg == NULL) {
		log_error("Cannot allocate memory for configuration (errno %d: %s)", errno, strerror(errno));
		goto done;
	}

	error = parse_configuration(CC_CONFIG_FILE, cc_cfg);
	if (error != 0)
		goto done;

	closelog();
	if (cc_cfg->log_console)
		log_options = log_options | LOG_PERROR;
	init_logger(cc_cfg->log_level, log_options);

	if (app_start_ccapi(cc_cfg) != CCAPI_START_ERROR_NONE)
		goto done;

	add_virtual_directories(cc_cfg->vdirs, cc_cfg->n_vdirs);

	if (app_start_tcp_transport(cc_cfg) != CCAPI_TCP_START_ERROR_NONE)
		goto done;

	start_system_monitor(cc_cfg);
	do {
		sleep(1);
	} while (check_stop() != CCAPI_TRUE);

done:
	free_cfg(cc_cfg);
	closelog();
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

