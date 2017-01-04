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
#include <time.h>
#include <signal.h>
#include <cc_logging.h>

#include "device_request.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define DEVREQ_TAG			"DEVREQ:"

#define MAX_RESPONSE_SIZE	256

#if !(defined UNUSED_ARGUMENT)
#define UNUSED_ARGUMENT(a)	(void)(a)
#endif

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/*
 * log_dr_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_dr_debug(format, ...)									\
	log_debug("%s " format, DEVREQ_TAG, __VA_ARGS__)

/*
 * log_dr_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_dr_error(format, ...)									\
	log_error("%s " format, DEVREQ_TAG, __VA_ARGS__)

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
void stop_cb(char const *const target, ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	static char const stop_response[] = "I'll stop";

	UNUSED_ARGUMENT(request_buffer_info);

	log_dr_debug("stop_cb(): target='%s' - transport='%d'", target, transport);

	response_buffer_info->buffer = malloc(sizeof(char) * strlen(stop_response) + 1);
	if (response_buffer_info->buffer == NULL) {
		log_dr_error("%s\n", "stop_cb(): response_buffer_info malloc error");
		return;
	}

	response_buffer_info->length = snprintf(response_buffer_info->buffer,
			MAX_RESPONSE_SIZE, "%s", stop_response);
}

/*
 * stop_status_cb() - Status callback for 'stop' device requests
 *
 * @target:					Target ID of the device request (stop)
 * @transport:				Communication transport used by the device request.
 * @response_buffer_info:	Buffer containing the response data.
 * @receive_error:			The error status of the receive process.
 *
 * This callback is executed when the receive process has finished. It doesn't
 * matter if everything worked or there was an error during the process.
 *
 * Cleans and frees the response buffer.
 */
void stop_status_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t *const response_buffer_info,
		ccapi_receive_error_t receive_error)
{
	log_dr_debug(
			"stop_status_cb(): target='%s' - transport='%d' - error='%d'",
			target, transport, receive_error);

	/* Free the response buffer */
	if (response_buffer_info != NULL)
		free(response_buffer_info->buffer);

	if (receive_error == CCAPI_RECEIVE_ERROR_NONE)
		kill(getpid(), SIGINT);
}

/*
 * get_time_cb() - Data callback for 'get_time' device requests
 *
 * @target:					Target ID of the device request (get_time).
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and executes the corresponding
 * command.
 */
void get_time_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const *const request_buffer_info,
		ccapi_buffer_info_t *const response_buffer_info)
{
	UNUSED_ARGUMENT(request_buffer_info);

	log_dr_debug("get_time_cb(): target='%s' - transport='%d'", target, transport);

	response_buffer_info->buffer = malloc(sizeof(char) * MAX_RESPONSE_SIZE + 1);
	if (response_buffer_info->buffer == NULL) {
		log_dr_error("%s\n", "get_time_cb(): response_buffer_info malloc error");
		return;
	}

	time_t t = time(NULL);
	response_buffer_info->length = snprintf(response_buffer_info->buffer,
			MAX_RESPONSE_SIZE, "Time: %s", ctime(&t));
}

/*
 * get_time_status_cb() - Status callback for 'get_time' device requests
 *
 * @target:					Target ID of the device request (get_time)
 * @transport:				Communication transport used by the device request.
 * @response_buffer_info:	Buffer containing the response data.
 * @receive_error:			The error status of the receive process.
 *
 * This callback is executed when the receive process has finished. It doesn't
 * matter if everything worked or there was an error during the process.
 *
 * Cleans and frees the response buffer.
 */
void get_time_status_cb(char const *const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t *const response_buffer_info,
		ccapi_receive_error_t receive_error)
{
	log_dr_debug(
			"get_time_status_cb(): target='%s' - transport='%d' - error='%d'",
			target, transport, receive_error);

	/* Free the response buffer */
	if (response_buffer_info != NULL)
		free(response_buffer_info->buffer);
}
