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

#ifndef _CCIMP_LOGGING_H_
#define _CCIMP_LOGGING_H_

#include "ccimp/ccimp_types.h"

#include <stdarg.h>
#include "connector_debug.h"

#if (defined CCIMP_DEBUG_ENABLED)

/**
 * Logging output from Cloud Connector, Writes a formatted string to stdout, expanding the format
 * tags with the value of the argument list arg.  This function behaves exactly as
 * vprintf() except an additional argument is passed indicating which part of the line is represented.
 *
 * debug_beg: Start of line. A fixed string could be added to each line to distinguish differing
 * types of output or to enable locking of the output until debug_eol is sent (too keep content from
 * breaking up in the middle). If format is NULL, there is no content.
 *
 * debug_mid: Middle of line. No header or trailer should be added.
 *
 * debug_end: End of line. A trailer may be added, but a newline MUST be added. At the point the message
 * is complete and if locking is enabled the unlock should be here. If format is NULL, there is no content.
 *
 * debug_all: The full message is contained in this one call. Equivalent to calling:
 *
 *      ccimp_hal_logging_vprintf(debug_beg, format, args);
 *      ccimp_hal_logging_vprintf(debug_end, "", args);
 *
 */
void ccimp_hal_logging_vprintf(debug_t const debug, char const * const format, va_list args);

/* Temporary while we don't have zones */
#define TMP_INFO_PREFIX        "INFO: "
#define TMP_FATAL_PREFIX       "FATAL: "
#define TMP_FATAL_PREFIX_LEN   (sizeof TMP_FATAL_PREFIX - 1)


#else

#define debug_t void *

#define TMP_INFO_PREFIX

#endif

#endif
