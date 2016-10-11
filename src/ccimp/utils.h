/*
 * utils.h
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
 * Description: Utils header.
 *
 */
#ifndef UTILS_H_
#define UTILS_H_

#include <syslog.h>
#include "ccapi/ccapi.h"
#include "load_config.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define DAEMON_NAME         "CONNECTOR"

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
ccapi_tcp_info_t * create_ccapi_tcp_start_info_struct(
		const cc_cfg_t * const cc_cfg);
void free_ccapi_start_struct(ccapi_start_t * ccapi_start);
void free_ccapi_tcp_start_info_struct(ccapi_tcp_info_t * const tcp_info);
void add_virtual_directories(const vdir_t * const vdirs, int n_vdirs);
ccapi_bool_t check_stop(void);
int file_exists(const char * const filename);
int file_readable(const char * const filename);

#endif /* UTILS_H_ */
