/*
 * Copyright (c) 2013 Digi International Inc.
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
#include "connector_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "app_debug.h"


#if defined(CONNECTOR_DEBUG)

void app_alt_printf(char const * const format, ...)
{
    if (!app_verbose_mode())
    {
        va_list args;

        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        fflush(stdout);
    }
    else
    {
        (void) format;
    }
}

void connector_debug_printf(char const * const format, ...)
{
    if (app_verbose_mode())
    {
        va_list args;

        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        fflush(stdout);
    }
    else
    {
        (void) format;
    }
}
#else

void connector_debug_printf(char const * const format, ...)
{
    (void) format;
}
void app_alt_printf(char const * const format, ...)
{
 (void) format;
}
 
#endif
