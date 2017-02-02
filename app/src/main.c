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

#include <cloudconnector.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "daemonize.h"
#include "device_request.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define VERSION		"0.1" GIT_REVISION

#define USAGE \
	"Cloud Connector client.\n" \
	"Copyright(c) Digi International Inc.\n" \
	"\n" \
	"Version: %s\n" \
	"\n" \
	"Usage: %s [options]\n\n" \
	"  -d  --daemon              Daemonize the process\n" \
	"  -c  --config-file=<PATH>  Use a custom configuration file instead of\n" \
	"                            the default one located in /etc/cc.conf\n" \
	"  -h  --help                Print help and exit\n" \
	"\n"

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static int start_connector(const char *config_file);
static ccapi_receive_error_t register_custom_device_requests(void);
static ccapi_bool_t check_stop(void);
static void add_sigkill_signal(void);
static void graceful_shutdown(void);
static void sigint_handler(int signum);
static void usage(char const *const name);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
static volatile ccapi_bool_t stop = CCAPI_FALSE;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	int result = EXIT_SUCCESS;
	char *name = basename(argv[0]);
	static int opt, opt_index;
	int create_daemon = 0;
	int log_options = LOG_CONS | LOG_NDELAY | LOG_PID | LOG_PERROR;
	char *config_file = NULL;
	static const char *short_options = "dc:h";
	static const struct option long_options[] = {
			{"daemon", no_argument, NULL, 'd'},
			{"config-file", required_argument, NULL, 'c'},
			{"help", no_argument, NULL, 'h'},
			{NULL, 0, NULL, 0}
	};

	/* Initialize the logging interface. */
	init_logger(LOG_DEBUG, log_options);

	while (1) {
		opt = getopt_long(argc, argv, short_options, long_options,
				&opt_index);
		if (opt == -1)
			break;

		switch (opt) {
		case 'd':
			create_daemon = 1;
			break;
		case 'c':
			config_file = optarg;
			break;
		case 'h':
			usage(name);
			goto done;
		default:
			usage(name);
			result = EXIT_FAILURE;
			goto done;
		}
	}

	/* Daemonize if requested. */
	if (create_daemon) {
		if (start_daemon(name) != 0) {
			result = EXIT_FAILURE;
			goto done;
		}
	}

	/* Do the real work. */
	start_connector(config_file);

done:
	closelog();

	return result;
}

/*
 * start_connector() - Start Cloud Connector
 *
 * @config_file:	Absolute path of the configuration file to use. NULL to use
 * 					the default one (/etc/cc.conf).
 *
 * Return: 0 on success, 1 otherwise.
 */
static int start_connector(const char *config_file)
{
	cc_init_error_t init_error;
	cc_start_error_t start_error;

	add_sigkill_signal();

	init_error = init_cloud_connection(config_file);
	if (init_error != CC_INIT_ERROR_NONE && init_error != CC_INIT_ERROR_ADD_VIRTUAL_DIRECTORY) {
		log_error("Cannot initialize cloud connection, error %d", init_error);
		return EXIT_FAILURE;
	}

	register_custom_device_requests();

	start_error = start_cloud_connection();
	if (start_error != CC_START_ERROR_NONE) {
		log_error("Cannot start cloud connection, error %d", start_error);
		return EXIT_FAILURE;
	}

	do {
		sleep(2);
	} while (check_stop() != CCAPI_TRUE);

	return EXIT_SUCCESS;
}

/*
 * register_custom_device_requests() - Register custom device requests
 *
 * Return: Error code after registering the custom device requests.
 */
static ccapi_receive_error_t register_custom_device_requests(void)
{
	ccapi_receive_error_t receive_error;

	receive_error = ccapi_receive_add_target(TARGET_GET_TIME, get_time_cb,
			get_time_status_cb, 0);
	if (receive_error != CCAPI_RECEIVE_ERROR_NONE) {
		log_error("Cannot register target '%s', error %d", TARGET_GET_TIME,
				receive_error);
	}
	receive_error = ccapi_receive_add_target(TARGET_STOP_CC, stop_cb,
			stop_status_cb, 0);
	if (receive_error != CCAPI_RECEIVE_ERROR_NONE) {
		log_error("Cannot register target '%s', error %d", TARGET_STOP_CC,
				receive_error);
	}

	return receive_error;
}

/*
 * check_stop() - Stop Cloud Connector
 *
 * Return: CCAPI_TRUE if Cloud Connector is successfully stopped, CCAPI_FALSE
 *         otherwise.
 */
static ccapi_bool_t check_stop(void)
{
	if (stop == CCAPI_TRUE)
		stop_cloud_connection();

	return stop;
}

/*
 * add_sigkill_signal() - Add the kill signal to the process
 */
static void add_sigkill_signal(void)
{
	struct sigaction new_action;
	struct sigaction old_action;

	/* Setup signal hander. */
	atexit(graceful_shutdown);
	new_action.sa_handler = sigint_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction(SIGINT, &new_action, NULL);
}

/*
 * graceful_shutdown() - Stop Cloud Connector and all threads
 */
void graceful_shutdown(void)
{
	stop = CCAPI_TRUE;
	check_stop();
	wait_for_ccimp_threads();
}

/**
 * sigint_handler() - Manage signal received.
 *
 * @signum: Received signal.
 */
static void sigint_handler(int signum)
{
	log_debug("sigint_handler(): received signal %d to close Cloud connection.", signum);
	exit(0);
}

/**
 * usage() - Print usage information
 *
 * @name:	Name of the daemon.
 */
static void usage(char const *const name)
{
	printf(USAGE, VERSION, name);
}
