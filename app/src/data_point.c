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
#include <time.h>
#include <cc_logging.h>

#include "data_point.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define DP_TAG			"DP:"

#define STREAM_NAME		"incremental"

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/*
 * log_dp_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_dp_debug(format, ...)									\
	log_debug("%s " format, DP_TAG, __VA_ARGS__)

/*
 * log_dp_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_dp_error(format, ...)									\
	log_error("%s " format, DP_TAG, __VA_ARGS__)

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/

static int get_incremental(void);
static ccapi_timestamp_t *get_timestamp(void);
static void free_timestamp(ccapi_timestamp_t *timestamp);

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
ccapi_dp_error_t init_sample_data_stream(ccapi_dp_collection_handle_t *dp_collection)
{
	ccapi_dp_collection_handle_t collection;
	ccapi_dp_error_t dp_error;

	dp_error = ccapi_dp_create_collection(&collection);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		log_dp_error("ccapi_dp_create_collection() error %d", dp_error);
		return dp_error;
	} else {
		*dp_collection = collection;
	}

	dp_error = ccapi_dp_add_data_stream_to_collection_extra(collection,
			STREAM_NAME, "int32 ts_iso", "counts", NULL);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		log_dp_error("ccapi_dp_add_data_stream_to_collection_extra() error %d",
				dp_error);
		free(collection);
	}

	return dp_error;
}

ccapi_dp_error_t add_sample_data_point(ccapi_dp_collection_handle_t dp_collection)
{
	ccapi_dp_error_t dp_error;
	ccapi_timestamp_t *timestamp = get_timestamp();

	dp_error = ccapi_dp_add(dp_collection, STREAM_NAME, get_incremental(), timestamp);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		log_dp_error("ccapi_dp_add() failed with error: %d", dp_error);
	}

	free_timestamp(timestamp);

	return dp_error;
}

ccapi_dp_error_t send_sample_data_stream(ccapi_dp_collection_handle_t dp_collection)
{
	ccapi_dp_error_t dp_error;

	log_dp_debug("%s", "Sending Data Stream with new incremental value");

	dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		log_dp_error("ccapi_dp_send_collection() error %d", dp_error);
	}

	return dp_error;
}

ccapi_dp_error_t destroy_sample_data_stream(ccapi_dp_collection_handle_t dp_collection)
{
	log_dp_debug("%s", "Destroying Data Stream");
	return ccapi_dp_destroy_collection(dp_collection);
}

/*
 * get_incremental() - Retrieves an incremental value each time
 */
static int get_incremental(void)
{
	static int incremental = -1;

	if (incremental == INT_MAX)
		incremental = 0;
	else
		incremental++;

	log_dp_debug("Incremental = %d", incremental);

	return incremental;
}

/*
 * get_timestamp() - Get the current timestamp of the system
 *
 * Return: The timestamp of the system.
 */
static ccapi_timestamp_t *get_timestamp(void)
{
	ccapi_timestamp_t *timestamp = NULL;
	size_t len = strlen("2016-09-27T07:07:09.546Z") + 1;
	char *date = NULL;
	time_t now;

	timestamp = (ccapi_timestamp_t*) malloc(sizeof(ccapi_timestamp_t));
	if (timestamp == NULL)
		return NULL;

	date = (char*) malloc(sizeof(char) * len);
	if (date == NULL) {
		free(timestamp);
		return NULL;
	}

	time(&now);
	if (strftime(date, len, "%FT%TZ", gmtime(&now)) > 0) {
		timestamp->iso8601 = date;
	} else {
		free(date);
		timestamp->iso8601 = "";
	}

	return timestamp;
}

/*
 * free_timestamp() - Free the given timestamp struct
 *
 * @timestamp:	Timestamp structure.
 */
static void free_timestamp(ccapi_timestamp_t *timestamp)
{
	if (timestamp != NULL) {
		if (timestamp->iso8601 != NULL) {
			free((char *) timestamp->iso8601);
			timestamp->iso8601 = NULL;
		}
		free(timestamp);
		timestamp = NULL;
	}
}
