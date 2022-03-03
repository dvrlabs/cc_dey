/*****************************************************************************
 * Copyright 2019 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi
 * International Inc.  By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered.  This is an unpublished copyrighted work of Digi International
 * Inc.  Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in sub-paragraph (c)(1)(ii) of The Rights in
 * Technical Data and Computer Software clause at DFARS 252.227-7031 or
 * subparagraphs (c)(1) and (2) of the Commercial Computer Software -
 * Restricted Rights at 48 CFR 52.227-19, as applicable.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 ****************************************************************************/
 
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "ccapi/ccapi.h"
#include "cc_logging.h"
#include "service_dp_upload.h"
#include "services_util.h"

typedef enum {
    upload_datapoint_file_terminate,
    upload_datapoint_file_metrics,
    upload_datapoint_file_events,
    upload_datapoint_file_count
} upload_datapoint_file_t;

static ccapi_send_error_t upload_datapoint_file(char const * const buff, ccapi_string_info_t * const hint_string_info, size_t size, char const cloud_path[])
{
#define TIMEOUT 5
	ccapi_send_error_t send_error;
	char const file_type[] = "text/plain";
	send_error = ccapi_send_data_with_reply(CCAPI_TRANSPORT_TCP,
											cloud_path, file_type,
											buff, size,
											CCAPI_SEND_BEHAVIOR_OVERWRITE,
											TIMEOUT,
											hint_string_info);
	if (send_error != CCAPI_SEND_ERROR_NONE)
	{
		log_error("Send error: %d Hint: %s", send_error, hint_string_info->string);
	}
	return send_error;
#undef TIMEOUT
}

int handle_datapoint_file_upload(int fd)
{
	while(1)
	{
		ccapi_send_error_t ret;
		uint32_t type;
		size_t size;
		void * blob;
		struct timeval timeout = {
			.tv_sec = SOCKET_READ_TIMEOUT_SEC,
			.tv_usec = 0
		};
		char hint[256];
		ccapi_string_info_t hint_string_info;

		hint[0] = '\0';
		hint_string_info.length = sizeof hint;
		hint_string_info.string = hint;
		
		if (read_uint32(fd, &type, &timeout))	/* read the record type from the client message */
		{
			send_error(fd, "Failed to read data type");
			return 1;
		}

		if (type == upload_datapoint_file_terminate)
		{
			break; 
		}
		else if (type != upload_datapoint_file_metrics && type != upload_datapoint_file_events)
		{
			send_error(fd, "Invalid datapoint type");
			return 1;
		}

		if (read_blob(fd, &blob, &size, &timeout))			/* Read the datapoint(s) blob of data from the client process */
		{
			send_error(fd, "Failed to read datapoint data");
			return 1;
		}

		char* cloud_path;
		switch(type)
		{
			case upload_datapoint_file_events:
				cloud_path = "DeviceLog/EventLog.json";
				break;
			case upload_datapoint_file_metrics:
			default:
				cloud_path = "DataPoint/.csv";
				break;
		}

		ret = upload_datapoint_file(blob, &hint_string_info, size, cloud_path);		/* upload the blob to the cloud */

		free(blob);

		if (ret)
		{
			char const * const err_msg = to_send_error_msg(ret);
			char * err_msg_with_hint = NULL;

			if ((hint[0] != '\0') && (asprintf(&err_msg_with_hint, "%s, %s", err_msg, hint) > 0))
			{
				send_error(fd, err_msg_with_hint);
				free(err_msg_with_hint);
			}
			else
			{
				send_error(fd, err_msg);
			}
		}
		else
		{
			send_ok(fd);
		}

		if (ret != CCAPI_SEND_ERROR_NONE)
		{
			return 1;
		}
	}
	return 0;
}
