/*
 * Copyright (c) 2017-2022 Digi International Inc.
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
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

#include <confuse.h>
#include <errno.h>
#include <miniunz/unzip.h>
#include <pthread.h>
#include <recovery.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "cc_config.h"
#include "cc_firmware_update.h"
#include "cc_logging.h"
#include "file_utils.h"
#include "services_util.h"
#include "string_utils.h"

/* Swupdate support */
#include <network_ipc.h>
#include <swupdate_status.h>

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define FW_UPDATE_TAG "FW UPDATE:"

#define REBOOT_TIMEOUT 1

#define UPDATE_PACKAGE_EXT ".swu"
#define FRAGMENT_EXT ".zip"

#define MANIFEST_PROP_SIZE "size"
#define MANIFEST_PROP_FRAGMENTS "fragments"
#define MANIFEST_PROP_NAME "name"
#define MANIFEST_PROP_CHECKSUM "checksum"
#define MANIFEST_PROP_SRC_DIR "src_dir"
#define MANIFEST_PROP_UNKNOWN "__unknown"

#define WRITE_BUFFER_SIZE 128 * 1024 /* 128KB */
#define FW_SWU_CHUNK_SIZE 128 * 1024 /* 128KB, CC6UL flash sector size */

#define LINE_BUFSIZE 255
#define CMD_BUFSIZE 255
#define FW_UPDATE_CMD "firmware-update-dual.sh"
#define PRINTENV_ACTIVE_SYSTEM_CMD "fw_printenv -n active_system"

/*------------------------------------------------------------------------------
                 D A T A    T Y P E S    D E F I N I T I O N S
------------------------------------------------------------------------------*/
typedef enum { CC_FW_TARGET_SWU, CC_FW_TARGET_MANIFEST } cc_fw_target_t;

/*
 * struct fw_manifest_t - Firmware manifest type
 *
 * @fw_total_size:	Total size in bytes of the firmware package
 * @n_fragments:	Number of fragments to reconstruct the firmware package
 * @fragment_name:	Name of each fragment (no index, no extension)
 * @fw_checksum:	CRC32 of the firmware package
 * @fragments_dir:	Directory where the fragments are located
 */
typedef struct {
    size_t fw_total_size;
    int n_fragments;
    char *fragment_name;
    uint32_t fw_checksum;
    char *fragments_dir;
} fw_manifest_t;

/*
 * struct fragment_t - Firmware package fragment type
 *
 * @path:	Absolute path of the firmware fragment
 * @name:	Name of the fragment (with index and extension)
 * @index:	Fragment index
 */
typedef struct {
    char *path;
    char *name;
    int index;
} fragment_t;

/*
 * struct firmware_info_t - Firmware package information type
 *
 * @file_path:		Absolute path of the assembled firmware package
 * @file_name:		Name of the firmware package (with extension)
 * @manifest:		Firmware manifest
 * @fragments:		List of fragments to reconstruct the firmware package
 * @n_fragments:	Number of fragments in the list
 */
typedef struct {
    char *file_path;
    char *file_name;
    fw_manifest_t manifest;
    fragment_t *fragments;
    int n_fragments;
} firmware_info_t;

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * log_fw_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_fw_debug(format, ...)                                              \
    log_debug("%s " format, FW_UPDATE_TAG, __VA_ARGS__)

/**
 * log_fw_info() - Log the given message as info
 *
 * @format:		Info message to log.
 * @args:		Additional arguments.
 */
#define log_fw_info(format, ...)                                               \
    log_info("%s " format, FW_UPDATE_TAG, __VA_ARGS__)

/**
 * log_fw_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_fw_error(format, ...)                                              \
    log_error("%s " format, FW_UPDATE_TAG, __VA_ARGS__)

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static ccapi_fw_request_error_t app_fw_request_cb(unsigned int const target,
                                                  char const *const filename,
                                                  size_t const total_size);
static ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target,
                                            uint32_t offset,
                                            void const *const data, size_t size,
                                            ccapi_bool_t last_chunk);
static void app_fw_cancel_cb(unsigned int const target,
                             ccapi_fw_cancel_error_t cancel_reason);
static void app_fw_reset_cb(unsigned int const target,
                            ccapi_bool_t *system_reset,
                            ccapi_firmware_target_version_t *version);
static ccapi_fw_data_error_t process_swu_package(const char *swu_path,
                                                 int target);
static int generate_manifest_firmware(const char *manifest_path, int target);
static void *reboot_threaded(void *reboot_timeout);
static int parse_manifest(const char *const manifest_path,
                          firmware_info_t *fw_info);
static int get_fw_path(firmware_info_t *fw_info);
static int get_fragments(firmware_info_t *fw_info);
static size_t get_available_space(const char *path);
static int generate_firmware_package(firmware_info_t *const fw_info);
static int assemble_fragment(fragment_t *fragment, const char *file_name,
                             FILE *swu_fp);
static char *concatenate_path(const char *directory, const char *file);
static char *get_fragment_file_name(const char *name, int index);
static void delete_fragments(firmware_info_t *fw_info);
static void free_fw_info(firmware_info_t *fw_info);
static void free_fragments(fragment_t *fragments, int n_fragments);
static int check_manifest_size(cfg_t *manifest_cfg, cfg_opt_t *opt);
static int check_manifest_fragments(cfg_t *manifest_cfg, cfg_opt_t *opt);
static int check_manifest_name(cfg_t *manifest_cfg, cfg_opt_t *opt);
static int check_manifest_checksum(cfg_t *manifest_cfg, cfg_opt_t *opt);
static int check_manifest_src_dir(cfg_t *manifest_cfg, cfg_opt_t *opt);

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
extern cc_cfg_t *cc_cfg;
static FILE *fw_fp = NULL;
static char *fw_downloaded_path = NULL;

/* Swupdate on the fly variables */
static char otf_buffer[WRITE_BUFFER_SIZE];
static int otf_end_status = EXIT_SUCCESS;
static pthread_mutex_t otf_mutex;
static pthread_cond_t otf_cv_end = PTHREAD_COND_INITIALIZER;

static bool otf_data_chunk_ready = false;
static int otf_chunk_size;
static int otf_last_chunk_size = 0;
static bool otf_update_successful = false;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/

/**
 * Copies a file from the source path to the destination path.
 * @param sourcePath The path of the source file.
 * @param destPath The path of the destination file.
 * @return 0 if successful, non-zero otherwise.
 */
int copy_file(const char *sourcePath, const char *destPath) {
    FILE *sourceFile, *destFile;
    char buffer[1024]; // Buffer to store file content
    size_t bytesRead;

    // Open the source file
    sourceFile = fopen(sourcePath, "rb");
    if (sourceFile == NULL) {
        perror("Error opening source file");
        return 1;
    }

    // Open the destination file
    destFile = fopen(destPath, "wb");
    if (destFile == NULL) {
        perror("Error opening destination file");
        fclose(sourceFile); // Close source file before exiting
        return 1;
    }

    // Copy the file content
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destFile);
    }

    // Close the files
    fclose(sourceFile);
    fclose(destFile);

    return 0;
}

/**
 * Deletes a file.
 * @param filePath The path of the file to be deleted.
 * @return 0 if successful, non-zero otherwise.
 */
int delete_file(const char *filePath) {
    if (remove(filePath) == 0) {
        return 0; // File successfully deleted
    } else {
        perror("Error deleting file");
        return 1; // Error occurred
    }
}

/**
 * Moves a file from the source path to the destination path.
 * @param sourcePath The path of the source file.
 * @param destPath The path of the destination file.
 * @return 0 if successful, non-zero otherwise.
 */
int move_file(const char *sourcePath, const char *destPath) {
    // First, try to copy the file
    if (copy_file(sourcePath, destPath) != 0) {
        return 1; // Copying failed, return error
    }

    // Copy was successful, now delete the source file
    if (delete_file(sourcePath) != 0) {
        return 2; // Deletion failed, return error
    }

    return 0; // Move was successful
}

/*
 * readBooleanFromFile() - Reads a boolean value from a text file
 *
 * @filename:  Path to the file containing the boolean value.
 *
 * This function opens a text file specified by the given filename and reads
 * the first line, expecting it to be either "true" or "false". It is designed
 * to read simple configuration files where a boolean value is stored as text.
 * The function returns TRUE (1) if the file contains "true", FALSE (0) if it
 * contains "false", and -1 in case of an error or if the file content is
 * neither "true" nor "false".
 *
 * Note: The function will treat any non-empty line that does not exactly match
 * "true" or "false" (case-sensitive) as an error.
 */
int readBooleanFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        /* perror("Error opening file"); */
        return -1; // Indicates an error
    }

    char buffer[6]; // Enough to store "true" or "false" plus a null terminator
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        /* perror("Error reading file"); */
        fclose(file);
        return -1; // Indicates an error
    }
    fclose(file);

    // Remove newline character if present
    buffer[strcspn(buffer, "\n")] = 0;

    if (strcmp(buffer, "true") == 0) {
        return 1; // True
    } else if (strcmp(buffer, "false") == 0) {
        return 0; // False
    } else {
        /* fprintf(stderr, "Invalid content in file\n"); */
        return -1; // Indicates an error or invalid content
    }
}

/**
 * Changes the filename in a given path.
 * @param buffer The buffer to store the new path.
 * @param bufferSize The size of the buffer.
 * @param originalPath The original file path.
 * @param newFilename The new filename to replace the old one.
 * @return A pointer to the buffer, or NULL if an error occurs.
 */
char *change_filename(char *buffer, size_t bufferSize, const char *originalPath,
                      const char *newFilename) {
    // Find the last occurrence of '/'
    const char *lastSlash = strrchr(originalPath, '/');
    if (lastSlash == NULL) {
        // The original path does not contain a '/', return NULL
        return NULL;
    }

    // Calculate the length of the directory path
    int dirPathLength = lastSlash - originalPath + 1; // +1 to include the '/'

    // Check if buffer is large enough
    if (bufferSize < dirPathLength + strlen(newFilename) + 1) {
        // Buffer is too small, return NULL
        return NULL;
    }

    // Copy the directory path
    strncpy(buffer, originalPath, dirPathLength);
    buffer[dirPathLength] = '\0'; // Null-terminate the directory path

    // Concatenate the new filename
    strcat(buffer, newFilename);

    return buffer;
}

/*
 * read_image() - Swupdate callback to read a new chunk of the on the fly image
 *
 * @p:		Buffer for the new chunk data.
 * @size:	Size of the new chunk.
 *
 * this is the callback to get a new chunk of the image in the on the fly
 * firmware update process.
 * It is called by a thread generated by the library and can block.
 */
static int read_image(char **p, int *size) {
    while (!otf_data_chunk_ready)
        usleep(1000);

    *p = otf_buffer;
    *size = otf_chunk_size;
    if (otf_chunk_size >= otf_last_chunk_size)
        otf_data_chunk_ready = false;
    otf_last_chunk_size = otf_chunk_size;
    otf_chunk_size = 0;

    return otf_last_chunk_size;
}

/*
 * print_status() - Swupdate callback to report on the fly firmware update
 *                 progress status
 *
 * @msg:	IPC message with the status of the on the fly firmware update.
 *
 * This is called by the Swupdate library to inform
 * about the current status of the upgrade
 *
 * Returns 0.
 */
static int print_status(ipc_message *msg) {
    log_fw_debug("Status: %d message: %s", msg->data.status.current,
                 strlen(msg->data.status.desc) > 0 ? msg->data.status.desc
                                                   : "");

    return 0;
}

/*
 * end_on_the_fly() - Swupdate callback to report and finish the on the fly
 * firmware update
 *
 * @status:	Status of the on the fly firmware update.
 *
 * This is called at the end reporting the status
 * of the on the fly upgrade and running any post-update
 * actions if successful
 *
 * Returns 0.
 */
static int end_on_the_fly(RECOVERY_STATUS status) {
    otf_end_status = (status == SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;

    log_fw_info("Swupdate %s",
                status == FAILURE ? "*failed*!" : "was successful!");

    if (status == SUCCESS) {
        log_fw_info("Executing post-update actions. %s", __func__);
        ipc_message msg;
        msg.data.procmsg.len = 0;
        if (ipc_postupdate(&msg) != 0 || msg.type != ACK) {
            log_fw_error("Running post-update failed! %s", __func__);
            otf_end_status = EXIT_FAILURE;
        }
    }

    pthread_mutex_lock(&otf_mutex);
    pthread_cond_signal(&otf_cv_end);
    pthread_mutex_unlock(&otf_mutex);

    if (status == SUCCESS && otf_end_status == EXIT_SUCCESS)
        otf_update_successful = true;

    return 0;
}

/*
 * init_fw_service() - Initialization of firmware service
 *
 * @fw_version:	The current firmware version.
 * @fw_service:	Struct to store the firmware service initialization.
 *
 * Returns: 0 on success, 1 otherwise.
 */
int init_fw_service(const char *const fw_version,
                    ccapi_fw_service_t **fw_service) {
    uint8_t version[4];
    uint8_t n_targets = 2;
    ccapi_firmware_target_t *fw_list = NULL;

    *fw_service = NULL;

    if (fw_version == NULL)
        return 0;

    if (sscanf(fw_version, "%hhu.%hhu.%hhu.%hhu", &version[0], &version[1],
               &version[2], &version[3]) != 4) {
        log_error(
            "Error initilizing Cloud connection: Bad firmware_version string "
            "'%s', firmware update disabled",
            fw_version);
        return 0;
    }

    fw_list = calloc(n_targets, sizeof(*fw_list));
    *fw_service = calloc(1, sizeof(**fw_service));
    if (fw_list == NULL || *fw_service == NULL) {
        log_error("Error initilizing Cloud connection: %s", "Out of memory");
        free(fw_list);
        return 1;
    }

    fw_list[0].chunk_size = FW_SWU_CHUNK_SIZE;
    fw_list[0].description = "System";
    fw_list[0].filespec = ".*\\.[sS][wW][uU]";
    fw_list[0].maximum_size = 0;
    fw_list[0].version.major = version[0];
    fw_list[0].version.minor = version[1];
    fw_list[0].version.revision = version[2];
    fw_list[0].version.build = version[3];

    fw_list[1].chunk_size = 0;
    fw_list[1].description = "Update manifest";
    fw_list[1].filespec = "[mM][aA][nN][iI][fF][eE][sS][tT]\\.[tT][xX][tT]";
    fw_list[1].maximum_size = 0;
    fw_list[1].version.major = version[0];
    fw_list[1].version.minor = version[1];
    fw_list[1].version.revision = version[2];
    fw_list[1].version.build = version[3];

    (*fw_service)->target.count = n_targets;
    (*fw_service)->target.item = fw_list;

    (*fw_service)->callback.request = app_fw_request_cb;
    (*fw_service)->callback.data = app_fw_data_cb;
    (*fw_service)->callback.reset = app_fw_reset_cb;
    (*fw_service)->callback.cancel = app_fw_cancel_cb;

    return 0;
}

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
static ccapi_fw_request_error_t app_fw_request_cb(unsigned int const target,
                                                  char const *const filename,
                                                  size_t const total_size) {
    ccapi_fw_request_error_t error = CCAPI_FW_REQUEST_ERROR_NONE;
    size_t available_space;

    log_fw_info("Firmware download requested (target '%d')", target);

    if (get_configuration(cc_cfg) != 0) {
        log_fw_error("Cannot load configuration (target '%d')", target);
        return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
    }

    if (cc_cfg->dualboot && cc_cfg->on_the_fly) {
        char proc_path[256] = {0};
        struct stat stats;
        int active_system_len = 7;
        char *resp = NULL;
        char *active_system = NULL;
        int retval;
        static struct swupdate_request req;

        log_fw_debug("Firmware download streaming requested (target '%d')",
                     target);
        /* Start swupdate prcess */
        pthread_mutex_init(&otf_mutex, NULL);

        /* May be set non-zero by end_on_the_fly() function on failure */
        otf_end_status = EXIT_SUCCESS;

        /* Prepare request structure */
        swupdate_prepare_req(&req);

        if (execute_cmd(PRINTENV_ACTIVE_SYSTEM_CMD, &resp, 2) != 0 ||
            resp == NULL) {
            if (resp != NULL)
                log_error("Error getting active system: %s", resp);
            else
                log_error("%s: Error getting active system", __func__);
            free(resp);
            retval = -1;
        } else {
            /* Read active system */
            active_system = trim(resp);
            log_fw_debug("Active system detected: '%s'", active_system);

            /* Detect storage media */
            sprintf(proc_path, "%s", "/proc/mtd");
            stat(proc_path, &stats);
            /* On eMMC devices the size will be zero */
            if (stats.st_size != 0) {
                strncpy(req.software_set, "mtd", sizeof(req.software_set) - 1);
            } else {
                strncpy(req.software_set, "mmc", sizeof(req.software_set) - 1);
            }

            /* Detect active system */
            if (!strncmp(active_system, "linux_a", active_system_len)) {
                strncpy(req.running_mode, "secondary",
                        sizeof(req.running_mode) - 1);
            } else {
                strncpy(req.running_mode, "primary",
                        sizeof(req.running_mode) - 1);
            }
            retval = swupdate_async_start(read_image, print_status,
                                          end_on_the_fly, &req, sizeof(req));

            free(resp);
        }

        /* Return if we've hit an error scenario */
        if (retval < 0) {
            log_fw_error("Streaming update process failed, returns '%d'",
                         retval);
            pthread_mutex_unlock(&otf_mutex);
            return CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
        }
    } else {
        fw_downloaded_path =
            concatenate_path(cc_cfg->fw_download_path, filename);
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
            log_fw_error(
                "Not enough space to download '%s' firmware file (target '%d')",
                filename, target);
            error = CCAPI_FW_REQUEST_ERROR_DOWNLOAD_INVALID_SIZE;
            goto done;
        }

        fw_fp = fopen(fw_downloaded_path, "wb+");
        if (fw_fp == NULL) {
            log_fw_error("Unable to create '%s' file (target '%d')", filename,
                         target);
            error = CCAPI_FW_REQUEST_ERROR_ENCOUNTERED_ERROR;
            goto done;
        }
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
 * the chunk_size field of the target information.
 *
 * Returns: 0 on success, error code otherwise.
 */
static ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target,
                                            uint32_t offset,
                                            void const *const data, size_t size,
                                            ccapi_bool_t last_chunk) {
    ccapi_fw_data_error_t error = CCAPI_FW_DATA_ERROR_NONE;
    int retval, loops = 0;

    log_fw_debug(
        "Received chunk: target=%d offset=0x%x length=%zu last_chunk=%d",
        target, offset, size, last_chunk);
    log_fw_debug("TARGET: %d", target);

    if (cc_cfg->dualboot && cc_cfg->on_the_fly) {
        log_fw_debug("Get data package from Remote Manager %d", target);
        otf_chunk_size = size;
        memcpy(otf_buffer, data, otf_chunk_size);
        otf_data_chunk_ready = true;

        if (last_chunk) {
            log_fw_debug("Firmware download completed for target '%d'", target);
            /* End called, unlock and exit */
            pthread_mutex_lock(&otf_mutex);
            pthread_cond_wait(&otf_cv_end, &otf_mutex);
            pthread_mutex_unlock(&otf_mutex);

            /* Verify upgrade status and post-update actions */
            if (otf_end_status == EXIT_FAILURE) {
                log_fw_error("Firmware download streaming failed '%d'",
                             otf_end_status);
                return CCAPI_FW_DATA_ERROR_INVALID_DATA;
            }

            /* Due to asynchronous update writing, we must wait until
             * the last packet has been written.
             */
            while (!otf_update_successful && loops < 100) {
                usleep(10000);
                log_fw_debug("Waiting for download and update to finish (%d)",
                             otf_update_successful);
                loops++;
            }
            if (loops >= 100) {
                log_fw_error(
                    "Timeout waiting to finish on the fly update process (%d)",
                    loops);
                error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
            }
        }
    } else {
        retval = fwrite(data, size, 1, fw_fp);
        if (retval != 1) {
            log_fw_error("%s", "Error writing to firmware file");
            return CCAPI_FW_DATA_ERROR_INVALID_DATA;
        }

        if (last_chunk) {
            if (fw_fp != NULL) {
                int fd = fileno(fw_fp);

                if (fsync(fd) != 0 || fclose(fw_fp) != 0) {
                    log_fw_error("Unable to close firmware file (errno %d: %s)",
                                 errno, strerror(errno));
                    return CCAPI_FW_DATA_ERROR_INVALID_DATA;
                }
            }
            log_fw_info("Firmware download completed for target '%d'", target);

            log_fw_info("Starting firmware update process (target '%d')",
                        target);

            switch (target) {
            /* Target for manifest.txt files. */
            case CC_FW_TARGET_MANIFEST: {
                if (generate_manifest_firmware(fw_downloaded_path, target) !=
                    0) {
                    log_fw_error("Error generating firmware package from '%s' "
                                 "for target '%d'",
                                 fw_downloaded_path, target);
                    error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
                    break;
                }
                error = process_swu_package(fw_downloaded_path, target);
                break;
            }
            /* Target for *.swu files. */
            case CC_FW_TARGET_SWU: {
                error = process_swu_package(fw_downloaded_path, target);
                break;
            }
            default:
                error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
            }

            free(fw_downloaded_path);
        }
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
static void app_fw_cancel_cb(unsigned int const target,
                             ccapi_fw_cancel_error_t cancel_reason) {
    log_fw_info("Cancel firmware update for target '%d'. Cancel_reason='%d'",
                target, cancel_reason);

    if (fw_fp != NULL) {
        int fd = fileno(fw_fp);

        if (fsync(fd) != 0 || fclose(fw_fp) != 0)
            log_fw_error("Unable to close firmware file (errno %d: %s)", errno,
                         strerror(errno));
        else if (remove(fw_downloaded_path) == -1)
            log_fw_error("Unable to remove firmware file (errno %d: %s)", errno,
                         strerror(errno));
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
static void app_fw_reset_cb(unsigned int const target,
                            ccapi_bool_t *system_reset,
                            ccapi_firmware_target_version_t *version) {
    unsigned int reboot_timeout = REBOOT_TIMEOUT;

    UNUSED_ARGUMENT(target);
    UNUSED_ARGUMENT(version);

    *system_reset = CCAPI_FALSE;

    if (cc_cfg->dualboot) {
        if (cc_cfg->on_the_fly) {
            char *resp = NULL;

            if (!otf_update_successful) {
                log_fw_error("On the fly update failed (%d)",
                             otf_update_successful);
                return;
            }
            log_fw_debug("On the fly update finished. Now we will reboot the "
                         "system (%d)",
                         otf_update_successful);

            /* Swap the active system partition */
            if (execute_cmd("on-the-fly-swap-partition.sh", &resp, 2) != 0) {
                if (resp != NULL)
                    log_error("Error swapping active system: %s", resp);
                else
                    log_error("%s: Error swapping active system", __func__);
                free(resp);
                return;
            }

            free(resp);
        } else {
            log_fw_debug("%s", "Dualboot mode does not reboot the system");
            return;
        }
    }

    log_fw_info("Rebooting in %d seconds", reboot_timeout);

    if (cc_cfg->dualboot && cc_cfg->on_the_fly) {
        sync();
        fflush(stdout);
        sleep(reboot_timeout);
        reboot(RB_AUTOBOOT);
    } else {
        log_fw_info("Reached condition for reboot. %d", reboot_timeout);

        const char *filename = "/etc/cc_config_download_only.conf";
        int result = readBooleanFromFile(filename);
        if (result == 0) {
            if (reboot_recovery(1))
                log_fw_error("%s", "Error rebooting in recovery mode");
        } else {
            log_fw_debug(
                "%s", "Not rebooting, cc_config_download_only is set to true.");
        }
    }
}

/*
 * process_swu_package() - Perform the installation of the SWU software package
 *
 * @swu_path:		Absolute path to the downloaded SWU file.
 * @target:		Target number.
 */
static ccapi_fw_data_error_t process_swu_package(const char *swu_path,
                                                 int target) {
    ccapi_fw_data_error_t error = CCAPI_FW_DATA_ERROR_NONE;

    log_fw_debug("New update version: %s", cc_cfg->fw_version);

    if (cc_cfg->dualboot) {
        char cmd[CMD_BUFSIZE] = {0};
        char line[LINE_BUFSIZE] = {0};
        FILE *fp;

        log_fw_debug("We will start the %s script at path %s", FW_UPDATE_CMD,
                     swu_path);
        sprintf(cmd, "%s %s", FW_UPDATE_CMD, swu_path);
        /* Open process to execute update command */
        fp = popen(cmd, "r");
        if (fp == NULL) {
            log_fw_error("Couldn't execute dualboot installation cmd %s", cmd);
        } else {
            /* Read script output till finished */
            while (fgets(line, LINE_BUFSIZE, fp) != NULL) {
                log_fw_debug("swupdate: %s", line);
                if (strstr(line, "There was an error performing the update")) {
                    log_fw_error("Error updating firmware using package '%s' "
                                 "for target '%d'",
                                 swu_path, target);
                    error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
                }
            }
            /* close the process */
            pclose(fp);
        }
    } else {
        /* 0.  Setup some variables to use. */

        const char *filename = "/etc/cc_config_download_only.conf";
        const char *new_swu_filename = "UPDATE.swu";
        char new_swu_path[256];

        /* 1.  Modify the absolute path + file to absolute path + new name */

        if (change_filename(new_swu_path, sizeof(new_swu_path), swu_path,
                            new_swu_filename) != NULL) {
            log_fw_debug("New swu path: %s\n", new_swu_path);
        } else {
            log_fw_error("%s", "Error: Could not change the swu filename");
            error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
        }

        /* 2. "mv" the file from swu_path to new_swu_path */

        if (move_file(swu_path, new_swu_path) == 0) {
            log_fw_debug("Swu file moved successfully to: %s\n", new_swu_path);
        } else {
            log_fw_error("%s", "Failed to move file.\n");
            error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
        }

        /* 3. Inspect the download_only config. If false, update right now. */
        if (readBooleanFromFile(filename) == 0) {

            if (update_firmware(new_swu_path)) {
                log_fw_error("Error updating firmware using package '%s' for "
                             "target '%d'",
                             new_swu_path, target);
                error = CCAPI_FW_DATA_ERROR_INVALID_DATA;
            }
        } else {
            log_fw_debug("%s",
                         "Not updating firmware, cc_config_download only is "
                         "set to true. Or, error reading config file.");
        }
    }

    return error;
}

/*
 * generate_manifest_firmware() - Generate firmware package via manifest
 *
 * @manifest_path:	Absolute path to the downloaded manifest file.
 * @target:			Target number.
 *
 * Steps of the firmware update via manifest:
 * 		1. Load the downloaded 'manifest.txt'.
 * 		2. Check if there is enough space for the complete firmware
 * package (once it is assembled) plus a single uncompressed fragment.
 * 		3. Check if all the fragments are located in the path specified
 * in the 'manifest.txt' file.
 * 		4. Generate the firmware package using the fragments:
 * 				a. Uncompress each fragment and assemble to the
 * final package. b. Delete the fragment. c. Compare the package size with the
 * specified in the 'manifest.txt' file. d. Calculate the package CRC32 and
 * compare with the specified in the 'manifest.txt' file.
 * 		5. Return generated package path.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int generate_manifest_firmware(const char *manifest_path, int target) {
    size_t available_space;
    firmware_info_t fw_info = {0};
    int error = 0;

    /* Load received manifest file. */

    if (parse_manifest(manifest_path, &fw_info) != 0) {
        log_fw_error("Error loading firmware manifest file '%s'",
                     manifest_path);
        error = -1;
        goto done;
    }

    /* Check available space. */

    available_space = get_available_space(cc_cfg->fw_download_path);
    if (available_space == 0) {
        log_fw_error("Unable to get available space (target '%d')", target);
        error = -1;
        goto done;
    }

    if (available_space < fw_info.manifest.fw_total_size) {
        log_fw_error("Not enough space in %s to update firmware (target '%d'), "
                     "needed %zu have %zu",
                     cc_cfg->fw_download_path, target,
                     fw_info.manifest.fw_total_size, available_space);
        error = -1;
        goto done;
    }

    /* Check fragments. */

    if (get_fw_path(&fw_info) != 0 || !get_fragments(&fw_info)) {
        error = -1;
        goto done;
    }

    log_fw_debug("%d fragments are ready. Begin image assembly",
                 fw_info.n_fragments);

    /* Generate firmware package from fragments. */

    if (generate_firmware_package(&fw_info) != 0) {
        error = -1;
        goto done;
    }

    /* Save firmware package path */
    log_fw_debug("Image was assembly in '%s'", fw_info.file_path);
    char *tmp = NULL;
    tmp = calloc(strlen(fw_info.file_path) + 1, sizeof(*tmp));
    if (tmp == NULL) {
        log_fw_error("Unable to install software package %s: Out of memory",
                     fw_info.file_path);
        error = -1;
        goto done;
    }
    free(fw_downloaded_path);
    fw_downloaded_path = tmp;
    strcpy(fw_downloaded_path, fw_info.file_path);

done:
    free_fw_info(&fw_info);

    return error;
}

/*
 * reboot_threaded() - Perform the reboot in a new thread
 *
 * @reboot_timeout:	Timeout in seconds.
 */
static void *reboot_threaded(void *reboot_timeout) {
    unsigned int timeout = *((unsigned int *)reboot_timeout);

    if (reboot_recovery(timeout))
        log_fw_error("%s", "Error rebooting in recovery mode");

    pthread_exit(NULL);

    return NULL;
}

/*
 * parse_manifest() - Load the downloaded 'manifest.txt' file
 *
 * @manifest_path:	Absolute path of the 'manifest.txt' file.
 * @fw_info:		Firmware information struct (firmware_info_t) where the
 * 					settings are saved.
 *
 * Read the provided 'manifest.txt' file and save the settings in the given
 * firmware_info_t struct. If the file does not exist or cannot be read, the
 * struct is initialized with the default settings.
 *
 * Return: 0 if the file is loaded successfully, -1 otherwise.
 */
static int parse_manifest(const char *const manifest_path,
                          firmware_info_t *fw_info) {
    cfg_t *manifest_cfg = NULL;
    int error = 0;

    /* Overall structure of the manifest properties. */
    static cfg_opt_t opts[] = {
        /* ------------------------------------------------------------ */
        /*|  TYPE   |   SETTING NAME    |  DEFAULT VALUE   |   FLAGS   |*/
        /* ------------------------------------------------------------ */
        CFG_INT(MANIFEST_PROP_SIZE, 0, CFGF_NODEFAULT),
        CFG_INT(MANIFEST_PROP_FRAGMENTS, 0, CFGF_NODEFAULT),
        CFG_STR(MANIFEST_PROP_NAME, NULL, CFGF_NODEFAULT),
        CFG_STR(MANIFEST_PROP_CHECKSUM, NULL, CFGF_NODEFAULT),
        CFG_STR(MANIFEST_PROP_SRC_DIR, NULL, CFGF_NODEFAULT),

        /* Needed for unknown properties. */
        CFG_STR(MANIFEST_PROP_UNKNOWN, NULL, CFGF_NONE), CFG_END()};

    if (access(manifest_path, R_OK) != 0) {
        log_fw_error("Firmware manifest file '%s' cannot be read",
                     manifest_path);
        return -1;
    }

    manifest_cfg = cfg_init(opts, CFGF_IGNORE_UNKNOWN);
    cfg_set_validate_func(manifest_cfg, MANIFEST_PROP_SIZE,
                          check_manifest_size);
    cfg_set_validate_func(manifest_cfg, MANIFEST_PROP_FRAGMENTS,
                          check_manifest_fragments);
    cfg_set_validate_func(manifest_cfg, MANIFEST_PROP_NAME,
                          check_manifest_name);
    cfg_set_validate_func(manifest_cfg, MANIFEST_PROP_CHECKSUM,
                          check_manifest_checksum);
    cfg_set_validate_func(manifest_cfg, MANIFEST_PROP_SRC_DIR,
                          check_manifest_src_dir);

    /* Parse the manifest file. */
    switch (cfg_parse(manifest_cfg, manifest_path)) {
    case CFG_FILE_ERROR:
        log_fw_error("Firmware manifest file '%s' could not be read: %s",
                     manifest_path, strerror(errno));
        error = -1;
        goto done;
    case CFG_SUCCESS:
        break;
    case CFG_PARSE_ERROR:
        log_fw_error("Error parsing firmware manifest file '%s'",
                     manifest_path);
        error = -1;
        goto done;
    }

    /* Fill manifest properties. */
    fw_info->manifest.fw_total_size =
        cfg_getint(manifest_cfg, MANIFEST_PROP_SIZE);
    fw_info->manifest.n_fragments =
        cfg_getint(manifest_cfg, MANIFEST_PROP_FRAGMENTS);
    fw_info->manifest.fw_checksum =
        strtoul(cfg_getstr(manifest_cfg, MANIFEST_PROP_CHECKSUM), NULL, 10);
    fw_info->manifest.fragment_name =
        strdup(cfg_getstr(manifest_cfg, MANIFEST_PROP_NAME));
    if (fw_info->manifest.fragment_name == NULL) {
        error = -1;
        goto done;
    }
    fw_info->manifest.fragments_dir =
        strdup(cfg_getstr(manifest_cfg, MANIFEST_PROP_SRC_DIR));
    if (fw_info->manifest.fragments_dir == NULL) {
        error = -1;
        goto done;
    }

done:
    cfg_free(manifest_cfg);

    return error;
}

/*
 * get_fw_path() - Retrieve the absolute path of the firmware update package
 *
 * @fw_info:		Firmware information struct (firmware_info_t) where the
 * 					path is stored.
 *
 * Memory for the path is obtained with 'malloc' and can be freed with 'free'.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int get_fw_path(firmware_info_t *fw_info) {
    fw_manifest_t manifest = fw_info->manifest;
    int len = strlen(manifest.fragment_name) + strlen(UPDATE_PACKAGE_EXT) + 1;

    fw_info->file_name = calloc(1, sizeof(char) * len);
    if (fw_info->file_name == NULL) {
        log_fw_error("Cannot allocate memory for update package '%s%s",
                     manifest.fragment_name, UPDATE_PACKAGE_EXT);
        return -1;
    }
    strcpy(fw_info->file_name, manifest.fragment_name);
    strcat(fw_info->file_name, UPDATE_PACKAGE_EXT);

    fw_info->file_path =
        concatenate_path(cc_cfg->fw_download_path, fw_info->file_name);
    if (fw_info->file_path == NULL) {
        log_fw_error("Cannot allocate memory for update package '%s%s",
                     manifest.fragment_name, UPDATE_PACKAGE_EXT);
        return -1;
    }

    return 0;
}

/*
 * get_fragments() - Retrieve all fragments information
 *
 * @fw_info:		Firmware information struct (firmware_info_t) where the
 * 					fragments information is stored.
 *
 * Return: Number of total fragments, 0 if no fragment is found or if any error
 * 			occurs.
 */
static int get_fragments(firmware_info_t *fw_info) {
    fw_manifest_t manifest = fw_info->manifest;
    int n_fragments = 0;
    int i;

    fw_info->fragments = calloc(manifest.n_fragments, sizeof(fragment_t));
    if (fw_info->fragments == NULL) {
        log_fw_error("%s", "Cannot allocate memory for firmware fragments");
        return 0;
    }

    for (i = 0; i < manifest.n_fragments; i++) {
        fragment_t *fragment = &fw_info->fragments[i];

        n_fragments++;
        fragment->index = i;

        /* Get fragment file path */
        fragment->name = get_fragment_file_name(manifest.fragment_name, i);
        if (fragment->name == NULL) {
            log_fw_error("Cannot allocate memory for fragment file '%s%d%s",
                         manifest.fragment_name, i, FRAGMENT_EXT);
            goto error;
        }

        fragment->path =
            concatenate_path(manifest.fragments_dir, fragment->name);
        if (fragment->path == NULL) {
            log_fw_error("Cannot allocate memory for fragment file '%s",
                         fragment->name);
            goto error;
        }

        if (access(fragment->path, F_OK) != 0) {
            log_fw_error("Missing fragment number '%d' ('%s')", i,
                         fragment->path);
            goto error;
        }
    }
    goto done;

error:
    free_fragments(fw_info->fragments, n_fragments);
    fw_info->fragments = NULL;
    n_fragments = 0;

done:
    fw_info->n_fragments = n_fragments;
    return n_fragments;
}

/*
 * get_available_space() - Retrieve the available space in bytes
 *
 * @path:	The path to get the available space.
 *
 * Return: Number of free bytes.
 */
static size_t get_available_space(const char *path) {
    struct statvfs stat;

    if (statvfs(path, &stat) != 0)
        return 0;

    return stat.f_bsize * stat.f_bavail;
}

/*
 * generate_firmware_package() - Assemble fragments to generate the firmware
 * package
 *
 * @fw_info:	Firmware information struct (firmware_info_t).
 *
 * The generation of the firmware package follow these steps:
 * 		1. Uncompress each fragment and assemble to the final package.
 * 		2. Delete the fragment.
 * 		3. Compare the package size with the specified in the
 * 'manifest.txt' file.
 * 		4. Calculate the package CRC32 and compare with the specified in
 * the 'manifest.txt' file.
 *
 * Return: 0 on success, -1 otherwise.
 */
static int generate_firmware_package(firmware_info_t *const fw_info) {
    int error = 0;
    int i;
    struct stat st;
    uint32_t crc32 = 0xFFFFFFFF;
    FILE *swu_fp = fopen(fw_info->file_path, "wb+");

    if (swu_fp == NULL) {
        log_fw_error("Unable to create '%s' firmware package",
                     fw_info->file_path);
        delete_fragments(fw_info);
        return -1;
    }

    /* Assemble fragments. */

    for (i = 0; i < fw_info->n_fragments; i++) {
        fragment_t fragment = fw_info->fragments[i];

        log_fw_debug("Processing fragment %d", i);

        if (assemble_fragment(&fragment, fw_info->file_name, swu_fp) != 0) {
            error = -1;
            break;
        }

        log_fw_debug("Fragment %d assembled", i);
        if (remove(fragment.path) == -1)
            log_fw_error("Unable to remove fragment %d (errno %d: %s)", i,
                         errno, strerror(errno));
    }

    if (fsync(fileno(swu_fp)) != 0 || fclose(swu_fp) != 0) {
        log_fw_error("Unable to close firmware package (errno %d: %s)", errno,
                     strerror(errno));
        error = -1;
    }

    if (error != 0) {
        delete_fragments(fw_info);
        error = -1;
        goto error;
    }

    log_fw_debug("Firmware package ready, '%s'", fw_info->file_path);

    /* Check file size */

    stat(fw_info->file_path, &st);
    if ((size_t)st.st_size != fw_info->manifest.fw_total_size) {
        log_fw_error("Bad firmware package size: %zu, expected %zu",
                     (size_t)st.st_size, fw_info->manifest.fw_total_size);
        error = -1;
        goto error;
    }

    /* Check CRC32 of the assembled file. */

    if (crc32file(fw_info->file_path, &crc32) != 0) {
        log_fw_error("Unable to calculate CRC32 of firmware package '%s'",
                     fw_info->file_name);
        error = -1;
        goto error;
    }

    if (crc32 != fw_info->manifest.fw_checksum) {
        log_fw_error("Wrong CRC32, calculated 0x%08x, expected 0x%08x", crc32,
                     fw_info->manifest.fw_checksum);
        error = -1;
        goto error;
    }

    log_fw_debug("CRC32 (0x%08x) is correct", crc32);

    goto done;

error:

    if (remove(fw_info->file_path) == -1)
        log_fw_error("Unable to remove firmware package (errno %d: %s)", errno,
                     strerror(errno));

done:

    return error;
}

/**
 * assemble_fragment() - Append a fragment to a file
 *
 * @fragment:		Fragment file to be assembled.
 * @file_name:		Name of the file compressed in the fragment.
 * @swu_fp:			File pointer to the destination file.
 *
 * Return: 0 if the file was successfully assembled, -1 otherwise.
 */
static int assemble_fragment(fragment_t *fragment, const char *file_name,
                             FILE *swu_fp) {
    unzFile src = NULL;
    char buffer[WRITE_BUFFER_SIZE];
    int size_buffer = WRITE_BUFFER_SIZE;
    int error = 0;

    src = unzOpen(fragment->path);
    if (src == NULL) {
        log_fw_error("Error assembling fragment, cannot open fragment '%s'",
                     fragment->path);
        return -1;
    }

    if (unzLocateFile(src, file_name, 1) != UNZ_OK) {
        log_fw_error(
            "Error assembling fragment, file '%s' not found in fragment",
            file_name);
        error = -1;
        goto done;
    }

    if (unzOpenCurrentFilePassword(src, NULL) != UNZ_OK) {
        log_fw_error("Error assembling fragment, cannot open fragment '%s' for "
                     "decompression",
                     fragment->name);
        error = -1;
        goto done;
    }

    do {
        int read = unzReadCurrentFile(src, buffer, size_buffer);
        if (read > 0) {
            size_t written = fwrite(buffer, read, 1, swu_fp);
            if (written != 1) {
                error = -1;
                break;
            }
        } else {
            error = (!read ? 0 : -1);
            break;
        }
    } while (error == 0);

    if (error)
        log_fw_error("Error assembling fragment '%s'", fragment->path);

done:
    unzCloseCurrentFile(src);

    return error;
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
static char *concatenate_path(const char *directory, const char *file) {
    char *full_path = NULL;
    int len = 0;

    if (directory == NULL && file == NULL)
        return NULL;

    if (directory == NULL && file != NULL)
        return strdup(file);

    if (directory != NULL && file == NULL)
        return strdup(directory);

    len = strlen(directory) + strlen(file) +
          (directory[strlen(directory) - 1] != '/' ? 1 : 0) + 1;

    full_path = calloc(1, sizeof(char) * len);
    if (full_path == NULL)
        return NULL;

    strcpy(full_path, directory);
    if (directory[strlen(directory) - 1] != '/')
        strcat(full_path, "/");
    strcat(full_path, file);

    return full_path;
}

/*
 * get_fragment_file_name() - Retrieve a fragment complete name including index
 * 								and extension
 *
 * @name:	Fragment base name without index and without extension.
 * @index:	Fragment index.
 *
 * Memory for the name is obtained with 'malloc' and can be freed with 'free'.
 *
 * Return: The fragment name or NULL in case of failure.
 */
static char *get_fragment_file_name(const char *name, int index) {
    int len = snprintf(NULL, 0, "%s%d" FRAGMENT_EXT, name, index);
    char *fragment_name = calloc(len + 1, sizeof(char));

    if (!fragment_name)
        return NULL;

    sprintf(fragment_name, "%s%d" FRAGMENT_EXT, name, index);

    return fragment_name;
}

/*
 * delete_fragments() - Remove all fragment files of a firmware package
 *
 * @fw_info:	Firmware information struct (firmware_info_t).
 */
static void delete_fragments(firmware_info_t *fw_info) {
    int i;

    for (i = 0; i < fw_info->n_fragments; i++)
        remove(fw_info->fragments[i].path);
}

/*
 * free_fw_info() - Release the provided firmware information
 *
 * @fw_info:	Firmware information struct (firmware_info_t).
 */
static void free_fw_info(firmware_info_t *fw_info) {
    if (fw_info == NULL)
        return;

    free(fw_info->file_path);
    fw_info->file_path = NULL;
    free(fw_info->file_name);
    fw_info->file_name = NULL;
    free(fw_info->manifest.fragment_name);
    fw_info->manifest.fragment_name = NULL;
    free(fw_info->manifest.fragments_dir);
    fw_info->manifest.fragments_dir = NULL;

    free_fragments(fw_info->fragments, fw_info->n_fragments);
}

/*
 * free_fw_info() - Release the provided list of fragments
 *
 * @free_fragments:	List of fragments (fragment_t) to release.
 * @n_fragments:	Number of fragments in the list.
 */
static void free_fragments(fragment_t *fragments, int n_fragments) {
    int i;

    if (fragments == NULL)
        return;

    for (i = 0; i < n_fragments; i++) {
        free(fragments[i].name);
        fragments[i].name = NULL;
        free(fragments[i].path);
        fragments[i].path = NULL;
    }
    free(fragments);
    fragments = NULL;
}

/*
 * check_manifest_size() - Validate size property of the manifest
 *
 * @manifest_cfg:	The section were the size is defined.
 * @opt:			The size option.
 *
 * @Return: 0 on success, -1 otherwise.
 */
static int check_manifest_size(cfg_t *manifest_cfg, cfg_opt_t *opt) {
    long int size = cfg_opt_getnint(opt, 0);

    if (size <= 0) {
        cfg_error(manifest_cfg, "Invalid %s (%l): size must be greater than 1",
                  opt->name, size);
        return -1;
    }

    return 0;
}

/*
 * check_manifest_fragments() - Validate fragments property of the manifest
 *
 * @manifest_cfg:	The section were the fragments is defined.
 * @opt:			The fragments option.
 *
 * @Return: 0 on success, -1 otherwise.
 */
static int check_manifest_fragments(cfg_t *manifest_cfg, cfg_opt_t *opt) {
    long int fragments = cfg_opt_getnint(opt, 0);

    if (fragments <= 0) {
        cfg_error(manifest_cfg,
                  "Invalid %s (%l): number of fragments must be greater than 1",
                  opt->name, fragments);
        return -1;
    }

    return 0;
}

/*
 * check_manifest_name() - Validate name property of the manifest
 *
 * @manifest_cfg:	The section were the name is defined.
 * @opt:			The name option.
 *
 * @Return: 0 on success, -1 otherwise.
 */
static int check_manifest_name(cfg_t *manifest_cfg, cfg_opt_t *opt) {
    char *name = cfg_opt_getnstr(opt, 0);

    if (name == NULL || strlen(name) == 0) {
        cfg_error(manifest_cfg, "Invalid %s: cannot be empty", opt->name);
        return -1;
    }

    return 0;
}

/*
 * check_manifest_checksum() - Validate checksum property of the manifest
 *
 * @manifest_cfg:	The section were the checksum is defined.
 * @opt:			The checksum option.
 *
 * @Return: 0 on success, -1 otherwise.
 */
static int check_manifest_checksum(cfg_t *manifest_cfg, cfg_opt_t *opt) {
    char *checksum = cfg_opt_getnstr(opt, 0);

    if (checksum == NULL || strlen(checksum) == 0) {
        cfg_error(manifest_cfg, "Invalid %s: cannot be empty", opt->name);
        return -1;
    }

    return 0;
}

/*
 * check_manifest_src_dir() - Validate src_dir property of the manifest
 *
 * @manifest_cfg:	The section were the src_dir is defined.
 * @opt:			The src_dir option.
 *
 * @Return: 0 on success, -1 otherwise.
 */
static int check_manifest_src_dir(cfg_t *manifest_cfg, cfg_opt_t *opt) {
    char *src_dir = cfg_opt_getnstr(opt, 0);

    if (src_dir == NULL || strlen(src_dir) == 0) {
        cfg_error(manifest_cfg, "Invalid %s: cannot be empty", opt->name);
        return -1;
    }

    if (access(src_dir, R_OK) != 0) {
        cfg_error(manifest_cfg,
                  "Invalid %s (%s): file does not exist or is not readable",
                  opt->name, src_dir);
        return -1;
    }

    return 0;
}
