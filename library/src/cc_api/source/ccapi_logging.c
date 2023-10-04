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

#define CCAPI_CONST_PROTECTION_UNLOCK

#include "ccapi_definitions.h"

#if (defined CCIMP_DEBUG_ENABLED)

unsigned int logging_lock_users = 0;
void * logging_lock = NULL;

/* TODO:
       - categories
*/

static ccimp_status_t ccapi_logging_lock_acquire(void)
{
    ccimp_status_t status = CCIMP_STATUS_ERROR;
    
    if (logging_lock != NULL)
    {
        status = ccapi_lock_acquire(logging_lock);
    }

    return status;
}

static ccimp_status_t ccapi_logging_lock_release(void)
{
    ccimp_status_t status = CCIMP_STATUS_ERROR;
    
    if (logging_lock != NULL)
    {
        status = ccapi_lock_release(logging_lock);
    }

    return status;
}

void connector_debug_vprintf(debug_t const debug, char const * const format, va_list args)
{
    ccapi_logging_lock_acquire();

    /* TODO: Macro in ccapi_definitions.h? */
    ccimp_hal_logging_vprintf(debug, format, args);

    ccapi_logging_lock_release();
}

#define CALL_LOGGING_VPRINTF(type, format) \
    do \
    { \
        va_list args; \
 \
        va_start(args, (format)); \
        ccimp_hal_logging_vprintf((type), (format), args); \
        va_end(args); \
    } \
    while (0)

void ccapi_logging_line(char const * const format, ...)
{
    ccapi_logging_lock_acquire();
    CALL_LOGGING_VPRINTF(debug_all, format);
    ccapi_logging_lock_release();
}

static void ccapi_logging_line_beg(char const * const format, ...)
{
    CALL_LOGGING_VPRINTF(debug_beg, format);
}

static void ccapi_logging_line_mid(char const * const format, ...)
{
    CALL_LOGGING_VPRINTF(debug_mid, format);
}

static void ccapi_logging_line_end(char const * const format, ...)
{
    CALL_LOGGING_VPRINTF(debug_end, format);
}

void ccapi_logging_print_buffer(char const * const label, void const * const buffer, size_t const length)
{
    size_t i;
    uint8_t const * const content = buffer;

    ccapi_logging_lock_acquire();

    ccapi_logging_line_beg("%s:", label);
    for (i = 0; i < length; i++)
    {
        if ((i % 16) == 0)
        {
            ccapi_logging_line_mid("\n");
        }

        ccapi_logging_line_mid(" %02X", content[i]);
    }
    ccapi_logging_line_end("");

    ccapi_logging_lock_release();
}
#else
void ccapi_logging_line(char const * const format, ...)
{
    (void)format;
}
void ccapi_logging_print_buffer(char const * const label, void const * const buffer, size_t const length)
{  
    (void)label;
    (void)buffer;
    (void)length;
}
#endif /* (defined CCIMP_DEBUG_ENABLED) */

