/*
 * Copyright (c) 2014 Digi International Inc.
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

#if (defined CONNECTOR_DEBUG)

#include <stdarg.h>
#include <ctype.h>
#include "connector_debug.h"

#if !(defined CALL_DEBUG_VPRINTF)
#define CALL_DEBUG_VPRINTF(type, format) \
    do \
    { \
        va_list args; \
        \
        va_start(args, (format)); \
        connector_debug_vprintf((type), (format), args); \
        va_end(args); \
    } \
    while (0)
#endif

#else

#if (defined CALL_DEBUG_VPRINTF)
#undef CALL_DEBUG_VPRINTF
#endif

#define CALL_DEBUG_VPRINTF(type, format) UNUSED_PARAMETER(format)

#endif

STATIC void connector_debug_line(char const * const format, ...)
{
    CALL_DEBUG_VPRINTF(debug_all, format);
}

#if (defined CONNECTOR_DEBUG)

#define enum_to_case(name)  case name: result = #name; break

#if !(defined CONNECTOR_DEBUG_NEW_LINE_STR)
#define CONNECTOR_DEBUG_NEW_LINE_STR    "\n"
#endif

#if (defined CONNECTOR_DEBUG_PRINT_BUFFERS)

STATIC void connector_debug_line_beg(char const * const format, ...)
{
    CALL_DEBUG_VPRINTF(debug_beg, format);
}

STATIC void connector_debug_line_mid(char const * const format, ...)
{
    CALL_DEBUG_VPRINTF(debug_mid, format);
}

STATIC void connector_debug_line_end(char const * const format, ...)
{
    CALL_DEBUG_VPRINTF(debug_end, format);
}

STATIC void connector_debug_print_buffer(char const * const label, void const * const buffer, size_t const length)
{
    uint8_t const * const content = buffer;

    connector_debug_line("%s (length=%zu):", label, length);
    for (size_t i = 0; i < length; i += 16)
    {
        connector_debug_line_beg("%03x: ", i);
        for (size_t n = 0; n < 16; n++)
        {
            size_t const position = (i + n);
            uint8_t const ch = content[position];

            if (position < length)
            {
                connector_debug_line_mid("%02x%c", ch, n == 7 ? '-' : ' ');
            } else {
                connector_debug_line_mid("   ");
            }
        }
        for (size_t n = 0; n < 16; n++)
        {
            size_t const position = (i + n);
            uint8_t const ch = content[position];

            if (position < length) {
                connector_debug_line_mid("%c", isprint(ch) ? ch : '.');
            } else {
                connector_debug_line_mid(" ");
            }
        }
        connector_debug_line_end("");
    }
}
#else
#define connector_debug_print_buffer(label, buffer, length)
#endif

#else

#define connector_debug_print_buffer(label, buffer, length)

#define CALL_DEBUG_VPRINTF(type, format) UNUSED_PARAMETER(format)

#endif

#if (defined CONNECTOR_DEBUG)
static char const * transport_to_string(connector_transport_t const value)
{
    char const * result = NULL;
    switch (value)
    {
        #if (defined CONNECTOR_TRANSPORT_TCP)
        enum_to_case(connector_transport_tcp);
        #endif
        #if (defined CONNECTOR_TRANSPORT_UDP)
        enum_to_case(connector_transport_udp);
        #endif
        #if (defined CONNECTOR_TRANSPORT_SMS)
        enum_to_case(connector_transport_sms);
        #endif
        enum_to_case(connector_transport_all);
    }
    return result;
}
#else
#define transport_to_string(value)  NULL
#endif
