/*
 * device_requests.c
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
 * Description: Device request callbacks.
 *
 */

#include <stdio.h>

#include "device_request.h"
#include "utils.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define DEVICE_REQUEST_TAG	"DEVREQ:"

#define MAX_RESPONSE_SIZE	400

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * log_dr_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_dr_debug(format, args...)									\
    log_debug("%s " format, DEVICE_REQUEST_TAG, ##args)

/**
 * log_dr_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_dr_error(format, args...)									\
    log_error("%s " format, DEVICE_REQUEST_TAG, ##args)

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
/**
 * app_receive_default_accept_cb() - Default accept callback for non registered
 *                                   device requests
 *
 * @target:		Target ID associated to the device request.
 * @transport:	Communication transport used by the device request.
 *
 * Return: CCAPI_FALSE if the device request is not accepted,
 *         CCAPI_TRUE otherswise.
 */
ccapi_bool_t app_receive_default_accept_cb(char const * const target,
		ccapi_transport_t const transport)
{
	ccapi_bool_t accept_target = CCAPI_TRUE;

	if (transport == CCAPI_TRANSPORT_SMS || transport == CCAPI_TRANSPORT_UDP) {
		/* Don't accept requests from SMS and UDP transports */
		log_dr_debug("app_receive_default_accept_cb(): not accepted request -"\
				" target='%s' - transport='%d'", target, transport);
		accept_target = CCAPI_FALSE;
	}

	return accept_target;
}

/**
 * app_receive_default_data_cb() - Default data callback for non registered
 *                                 device requests
 *
 * @target:					Target ID associated to the device request.
 * @transport:				Communication transport used by the device request.
 * @request_buffer_info:	Buffer containing the device request.
 * @response_buffer_info:	Buffer to store the answer of the request.
 *
 * Logs information about the received request and sends an answer to Device
 * Cloud indicating that the device request with that target is not registered.
 */
void app_receive_default_data_cb(char const * const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t const * const request_buffer_info,
		ccapi_buffer_info_t * const response_buffer_info)
{
	char * request_buffer;
	char * request_data;
	size_t i;

	/* Log request data */
	log_dr_debug("app_receive_default_data_cb(): not registered target -"     \
			" target='%s' - transport='%d'", target, transport);
	request_buffer = malloc(sizeof(char) * request_buffer_info->length + 1);
	if (request_buffer == NULL) {
		log_dr_error("app_receive_default_data_cb():"                         \
				" request_buffer malloc error");
		return;
	}
	for (i = 0 ; i < request_buffer_info->length ; i++)
		request_buffer[i] = ((char*)request_buffer_info->buffer)[i];
	request_buffer[request_buffer_info->length] = '\0';
	request_data = strtrim(request_buffer);
	if (request_data == NULL) {
		log_dr_error("app_receive_default_data_cb():"                         \
				" request_data malloc error");
		free(request_buffer);
		return;
	}
	log_dr_debug("app_receive_default_data_cb(): not registered target -"     \
			" request='%s'", request_data);
	free(request_buffer);
	free(request_data);

	/* Provide response to Device Cloud */
	if (response_buffer_info != NULL) {
		response_buffer_info->buffer = malloc(sizeof(char) * MAX_RESPONSE_SIZE);
		if (response_buffer_info->buffer == NULL) {
			log_dr_error("app_receive_default_data_cb():" \
					" response_buffer_info malloc error");
			return;
		}
		response_buffer_info->length = sprintf(response_buffer_info->buffer,
				"Target '%s' not registered", target);
	}
}

/**
 * app_receive_default_status_cb() - Default status callback for non registered
 *                                   device requests
 *
 * @target:					Target ID associated to the device request.
 * @transport:				Communication transport used by the device request.
 * @response_buffer_info:	Buffer containing the response data.
 * @receive_error:			The error status of the receive process.
 *
 * This callback is executed when the receive process has finished. It doesn't
 * matter if everything worked or there was an error during the process.
 *
 * Cleans and frees the response buffer.
 */
void app_receive_default_status_cb(char const * const target,
		ccapi_transport_t const transport,
		ccapi_buffer_info_t * const response_buffer_info,
		ccapi_receive_error_t receive_error)
{
	log_dr_debug("app_receive_default_status_cb(): target='%s' -"             \
			" transport='%d' - error='%d'", target, transport, receive_error);
	/* Free the response buffer */
	if (response_buffer_info != NULL)
		free(response_buffer_info->buffer);
}
