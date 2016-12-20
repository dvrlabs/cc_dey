/*
* Copyright (c) 2016 Digi International Inc.
* All rights not expressly granted are reserved.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
* =======================================================================
*/

#ifndef CC_LOGGING_H_
#define CC_LOGGING_H_

#include <syslog.h>

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
#define init_logger(level, options)								\
	do {														\
		openlog(DAEMON_NAME, options, LOG_USER);				\
		setlogmask(LOG_UPTO(level));							\
	} while (0)

/**
 * set_log_level() - Set the new log level
 *
 * @level:	New log level.
 */
#define set_log_level(level)									\
	setlogmask(LOG_UPTO(level))

/**
 * log_error() - Log the given message as error
 *
 * @format:	Error message to log.
 * @args:	Additional arguments.
 */
#define log_error(format, ...)										\
	syslog(LOG_ERR, format, __VA_ARGS__)

/**
 * log_info() - Log the given message as info
 *
 * @format:	Info message to log.
 * @args:	Additional arguments.
 */
#define log_info(format, ...)										\
	syslog(LOG_INFO, format, __VA_ARGS__)

/**
 * log_debug() - Log the given message as debug
 *
 * @format:	Debug message to log.
 * @args:	Additional arguments.
 */
#define log_debug(format, ...)										\
	syslog(LOG_DEBUG, format, __VA_ARGS__)

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
void wait_for_ccimp_threads(void);

#endif /* CC_LOGGING_H_ */
