/*
 * ccimp_logging.c
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
 * Description: Cloud Connector logging implementation.
 *
 */

#include "ccimp/ccimp_logging.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#if (defined UNIT_TEST)
#define ccimp_hal_logging_vprintf		ccimp_hal_logging_vprintf_real
#endif

#if (defined CCIMP_DEBUG_ENABLED)
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define MAX_CHARS				256
#define CCAPI_DEBUG_PREFIX		"CCAPI: "

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
static char * buffer = NULL;
static size_t bufsize = 0;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
void ccimp_hal_logging_vprintf(debug_t const debug, char const * const format, va_list args)
{
	switch (debug) {
	case debug_beg: {
		size_t offset = 0;
		if (buffer == NULL) {
			bufsize = MAX_CHARS + sizeof(CCAPI_DEBUG_PREFIX);
			buffer = (char*) malloc(sizeof(char) * bufsize);
		}

		snprintf(buffer, bufsize, "%s", CCAPI_DEBUG_PREFIX);
		offset = strlen(buffer);

		vsnprintf(buffer + offset, bufsize - offset, format, args);
		break;
	}
	case debug_mid: {
		size_t offset = strlen(buffer);

		vsnprintf(buffer + offset, bufsize - offset, format, args);
		break;
	}
	case debug_end: {
		size_t offset = strlen(buffer);

		vsnprintf(buffer + offset, bufsize - offset, format, args);
		log_debug("%s", buffer);
		free(buffer);
		buffer = NULL;
		bufsize = 0;
		break;
	}
	case debug_all: {
		size_t offset = 0;
		if (buffer == NULL) {
			bufsize = MAX_CHARS + sizeof(CCAPI_DEBUG_PREFIX);
			buffer = (char*) malloc(sizeof(char) * bufsize);
		}

		snprintf(buffer, bufsize, "%s", CCAPI_DEBUG_PREFIX);
		offset = strlen(buffer);

		vsnprintf(buffer + offset, bufsize - offset, format, args);
		log_debug("%s", buffer);
		free(buffer);
		buffer = NULL;
		bufsize = 0;
		break;
	}
	}
	return;
}
#else
 /* to avoid ISO C forbids an empty translation unit compiler error */
typedef int dummy;
#endif
