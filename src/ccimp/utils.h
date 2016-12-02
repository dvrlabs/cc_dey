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

#ifndef UTILS_H_
#define UTILS_H_

#include <syslog.h>
#include "ccapi/ccapi.h"
#include "load_config.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define DAEMON_NAME			"CONNECTOR"

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * init_logger() - Initialize the logger with the given log level
 *
 * @level:	Log level.
 */
#define init_logger(level, options)										\
	do {																\
		openlog(DAEMON_NAME, options, LOG_USER);						\
		setlogmask(LOG_UPTO(level));									\
	} while (0)

/**
 * set_log_level() - Set the new log level
 *
 * @level:	New log level.
 */
#define set_log_level(level)											\
	setlogmask(LOG_UPTO(level))

/**
 * log_error() - Log the given message as error
 *
 * @format:	Error message to log.
 * @args:	Additional arguments.
 */
#define log_error(format, args...)										\
	syslog(LOG_ERR, format, ##args)

/**
 * log_info() - Log the given message as info
 *
 * @format:	Info message to log.
 * @args:	Additional arguments.
 */
#define log_info(format, args...)										\
	syslog(LOG_INFO, format, ##args)

/**
 * log_debug() - Log the given message as debug
 *
 * @format:	Debug message to log.
 * @args:	Additional arguments.
 */
#define log_debug(format, args...)										\
	syslog(LOG_DEBUG, format, ##args)

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
ccapi_start_t * create_ccapi_start_struct(const cc_cfg_t * const cc_cfg);
ccapi_tcp_info_t * create_ccapi_tcp_start_info_struct(const cc_cfg_t * const cc_cfg);
void free_ccapi_start_struct(ccapi_start_t * ccapi_start);
void free_ccapi_tcp_start_info_struct(ccapi_tcp_info_t * const tcp_info);
void add_virtual_directories(const vdir_t * const vdirs, int n_vdirs);
int file_exists(const char * const filename);
int file_readable(const char * const filename);
char * strltrim(const char *s);
char * strrtrim(const char *s);
char * strtrim(const char *s);

#endif /* UTILS_H_ */
