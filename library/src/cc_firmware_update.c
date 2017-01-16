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
#include <pthread.h>
#include <sys/statvfs.h>
#include <recovery.h>

#include "cc_config.h"
#include "cc_firmware_update.h"
#include "cc_logging.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(a)			((void)(a))
#endif

#define FW_UPDATE_TAG				"FW UPDATE:"

#define REBOOT_TIMEOUT				1

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * log_fw_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_fw_debug(format, ...)									\
	log_debug("%s " format, FW_UPDATE_TAG, __VA_ARGS__)

/**
 * log_fw_info() - Log the given message as info
 *
 * @format:		Info message to log.
 * @args:		Additional arguments.
 */
#define log_fw_info(format, ...)									\
	log_info("%s " format, FW_UPDATE_TAG, __VA_ARGS__)

/**
 * log_fw_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_fw_error(format, ...)									\
	log_error("%s " format, FW_UPDATE_TAG, __VA_ARGS__)

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static void *reboot_threaded(void *reboot_timeout);
static size_t get_available_space(const char* path);
static char* concatenate_path(const char *directory, const char* file);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
extern cc_cfg_t *cc_cfg;
static FILE *fw_fp = NULL;
static char *fw_downloaded_path = NULL;
static pthread_t reboot_thread;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
/*
 * app_fw_request_cb() - Incoming firmware update request callback
 *
 * @target:		Target number of the firmware update request.
 * @filename:	Name of the firmware file to download.
 * @total_size:	Total size required for the downloaded firmware.
 *
 * This callback ask for acceptance of an incoming firmware update request.
 * The decision can be taken based on the request target number, the file name,
 * and the total size.
 *
 * Returns: 0 on success, error code otherwise.
 */
ccapi_fw_request_error_t app_fw_request_cb(unsigned int const target,
		char const *const filename, size_t const total_size) {
	ccapi_fw_request_error_t error = CCAPI_FW_REQUEST_ERROR_NONE;
	size_t available_space;

	log_fw_info("Firmware download requested (target '%d')", target);

	if (get_configuration(cc_cfg) != 0) {
		log_fw_error("Cannot load configuration (target '%d')", target);
		return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
	}

	fw_downloaded_path = concatenate_path(cc_cfg->fw_download_path, filename);
	if (fw_downloaded_path == NULL) {
		log_fw_error(
				"Cannot allocate memory for '%s' firmware file (target '%d')",
				filename, target);
		return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
	}

	available_space = get_available_space(cc_cfg->fw_download_path);
	if (available_space == 0) {
		log_fw_error("Unable to get available space (target '%d')", target);
		error = CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
		goto done;
	}
	if (available_space < total_size) {
		log_fw_error("Not enough space to download '%s' firmware file (target '%d')", filename, target);
		error = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_SIZE;
		goto done;
	}

	fw_fp = fopen(fw_downloaded_path, "wb+");
	if (fw_fp == NULL) {
		log_fw_error("Unable to create '%s' file (target '%d')", filename, target);
		error = CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
		goto done;
	}

done:

	if (error != CCAPI_FW_REQUEST_ERROR_NONE)
		free(fw_downloaded_path);

	return error;
}

/*
 * app_fw_data_cb() - Receive firmware data chunk callback
 *
 * @target:		Target number of the firmware data chunk.
 * @offset:		Offset in the received data.
 * @data:		Firmware data chunk.
 * @size:		Size of the data chunk.
 * @last_chunk:	CCAPI_TRUE if it is the last data chunk.
 *
 * Data to program is received including the offset where it should be
 * programmed. The size of the data will be the one configured by the user in
 * the chuck_size field of the target information.
 *
 * Returns: 0 on success, error code otherwise.
 */
ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target, uint32_t offset,
		void const *const data, size_t size, ccapi_bool_t last_chunk) {
	ccapi_fw_data_error_t error = CCAPI_FW_DATA_ERROR_NONE;
	int retval;

	log_fw_debug("Received chunk: target=%d offset=0x%x length=%zu last_chunk=%d", target, offset, size, last_chunk);

	retval = fwrite(data, size, 1, fw_fp);
	if (retval != 1) {
		log_fw_error("%s", "Error writing to firmware file");
		return CCAPI_FW_DATA_ERROR_INVALID_DATA;
	}

	if (last_chunk) {
		if (fw_fp != NULL) {
			int fd = fileno(fw_fp);
			if (fsync(fd) != 0 || fclose(fw_fp) != 0) {
				log_fw_error("Unable to close firmware file (errno %d: %s)", errno, strerror(errno));
				return CCAPI_FW_DATA_ERROR_INVALID_DATA;
			}
		}
		log_fw_info("Firmware download completed for target '%d'", target);

		log_fw_info("Starting firmware update process (target '%d')", target);

		switch(target) {
		/* Target for *.swu files. */
		case 0: {
			if (update_firmware(fw_downloaded_path)) {
				log_fw_error(
						"Error updating firmware using package '%s' for target '%d'",
						fw_downloaded_path, target);
				error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
			}
			break;
		}
		default:
			error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
		}

		free(fw_downloaded_path);
	}

	return error;
}

/*
 * app_fw_cancel_cb() - Firmware update process abort callback
 *
 * @target:			Target number.
 * @cancel_reason:	Abort reason or status.
 *
 * Called when a firmware update abort message is received.
 */
void app_fw_cancel_cb(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason)
{
	log_fw_info("Cancel firmware update for target '%d'. Cancel_reason='%d'",
			target, cancel_reason);

	if (fw_fp != NULL) {
		int fd = fileno(fw_fp);
		if (fsync(fd) != 0 || fclose(fw_fp) != 0) {
			log_fw_error("Unable to close firmware file (errno %d: %s)", errno, strerror(errno));
		} else {
			if (remove(fw_downloaded_path) == -1)
				log_fw_error("Unable to remove firmware file (errno %d: %s)",
						errno, strerror(errno));
		}
	}

	free(fw_downloaded_path);
}

/*
 * app_fw_reset_cb() - Reset device callback
 *
 * @target:			Target number.
 * @system_reset:	CCAPI_TRUE to reboot the device, CCAPI_FALSE otherwise.
 * @version:		Version for the updated target.
 *
 * It is called when firmware update has finished. It lets the user decide
 * whether rebooting the device.
 */
void app_fw_reset_cb(unsigned int const target, ccapi_bool_t *system_reset, ccapi_firmware_target_version_t *version)
{
	unsigned int *reboot_timeout = NULL;
	int error = 0;

	UNUSED_PARAMETER(target);
	UNUSED_PARAMETER(version);

	*system_reset = CCAPI_FALSE;

	reboot_timeout = malloc(sizeof (unsigned int));
	if (reboot_timeout == NULL) {
		error = 1;
		goto error;
	}
	*reboot_timeout = REBOOT_TIMEOUT;

	log_fw_info("Rebooting in %d seconds", *reboot_timeout);

	error = pthread_create(&reboot_thread, NULL, reboot_threaded,
			(void *) reboot_timeout);

error:

	if (error) {
		free(reboot_timeout);
		/* If we cannot create the thread just reboot. */
		if (reboot_recovery(REBOOT_TIMEOUT))
			log_fw_error("%s", "Error rebooting in recovery mode");
	}
}

/*
 * reboot_threaded() - Perform the reboot in a new thread
 *
 * @reboot_timeout:	Timeout in seconds.
 */
static void *reboot_threaded(void *reboot_timeout)
{
	unsigned int *timeout = (unsigned int *)reboot_timeout;

	if (reboot_recovery(*timeout))
		log_fw_error("%s", "Error rebooting in recovery mode");

	free(timeout);

	pthread_exit(NULL);
	return NULL;
}

/*
 * get_available_space() - Retrieve the available space in bytes
 *
 * @path:	The path to get the available space.
 *
 * Return: Number of free bytes.
 */
static size_t get_available_space(const char* path)
{
	struct statvfs stat;

	if (statvfs(path, &stat) != 0)
		return 0;

	return stat.f_bsize * stat.f_bavail;
}

/*
 * concatenate_path() - Concatenate directory path and file name
 *
 * @directory:	Parent directory absolute path.
 * @file:		File name.
 *
 * Concatenate the given directory path and file name, and returns a pointer to
 * a new string with the result. If the given directory path does not finish
 * with a '/' it is automatically added.
 *
 * Memory for the new string is obtained with 'malloc' and can be freed with
 * 'free'.
 *
 * Return: A pointer to a new string with the concatenation or NULL if both
 * 			'directory' and 'file' are NULL.
 */
static char* concatenate_path(const char *directory, const char *file)
{
	char *full_path = NULL;
	int len = 0;

	if (directory == NULL && file == NULL)
		return NULL;

	if (directory == NULL && file != NULL)
		return strdup(file);

	if (directory != NULL && file == NULL)
		return strdup(directory);

	len = strlen(directory) + strlen(file)
			+ (directory[strlen(directory) - 1] != '/' ? 1 : 0) + 1;

	full_path = calloc(1, sizeof (char) * len);
	if (full_path == NULL)
		return NULL;

	strcpy(full_path, directory);
	if (directory[strlen(directory) - 1] != '/')
		strcat(full_path, "/");
	strcat(full_path, file);

	return full_path;
}
