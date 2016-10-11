/*
 * firmware_update.c
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
 * Description: Firmware update callbacks.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "firmware_update.h"
#include "utils.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(a)			((void)(a))
#endif

#define FW_UPDATE_TAG				"FW UPDATE:"

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * log_fw_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_fw_debug(format, args...)									\
    log_debug("%s " format, FW_UPDATE_TAG, ##args)

/**
 * log_fw_info() - Log the given message as info
 *
 * @format:		Info message to log.
 * @args:		Additional arguments.
 */
#define log_fw_info(format, args...)									\
    log_info("%s " format, FW_UPDATE_TAG, ##args)

/**
 * log_fw_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_fw_error(format, args...)									\
    log_error("%s " format, FW_UPDATE_TAG, ##args)


/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
static FILE * fw_fp = NULL;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
ccapi_fw_request_error_t app_fw_request_cb(unsigned int const target,
		char const * const filename, size_t const total_size) {
	UNUSED_PARAMETER(target);
	UNUSED_PARAMETER(total_size);

	fw_fp = fopen(filename, "wb+");
	if (fw_fp == NULL) {
		log_fw_error("Unable to create %s file", filename);
		return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
	}

	return CCAPI_FW_REQUEST_ERROR_NONE;
}

ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target, uint32_t offset,
		void const * const data, size_t size, ccapi_bool_t last_chunk) {
	int retval;

	UNUSED_PARAMETER(target);
	log_fw_debug("Received chunk: target=%d offset=0x%x length=%zu last_chunk=%d", target, offset, size, last_chunk);

	retval = fwrite(data, size, 1, fw_fp);
	if (retval != 1) {
		log_fw_error("Error writing to firmware file");
		return CCAPI_FW_DATA_ERROR_INVALID_DATA;
	}

	if (last_chunk) {
		if (fw_fp != NULL) {
			int fd = fileno(fw_fp);
			if (fsync(fd) != 0 ||fclose(fw_fp) != 0) {
				log_fw_error("Unable to close firmware file (errno %d: %s)", errno, strerror(errno));
				return CCAPI_FW_DATA_ERROR_INVALID_DATA;
			}
		}
		log_fw_info("Firmware download completed for target = %d", target);
	}

	return CCAPI_FW_DATA_ERROR_NONE;
}

void app_fw_cancel_cb(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason)
{
	log_fw_info("Cancel firmware update for target='%d'. Cancel_reason='%d'", target, cancel_reason);

	if (fw_fp != NULL) {
		if (fclose(fw_fp) != 0)
			log_fw_error("Unable to close firmware file (errno %d: %s)", errno, strerror(errno));
	}
}

void app_fw_reset_cb(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * version)
{
	UNUSED_PARAMETER(system_reset);
	UNUSED_PARAMETER(version);

	log_fw_info("Reset for target='%d'", target);
}
