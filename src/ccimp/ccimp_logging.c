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
