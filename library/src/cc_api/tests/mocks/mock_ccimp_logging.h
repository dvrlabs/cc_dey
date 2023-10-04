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

#ifndef _MOCK_CCIMP_LOGGING_H_
#define _MOCK_CCIMP_LOGGING_H_

#define CCIMP_LOGGING_PRINTF_DOESNT_EXPECT_A_CALL   "_MOCK_DOESNT_EXPECT_A_CALL_"

void Mock_ccimp_logging_printf_create(void);
void Mock_ccimp_logging_printf_destroy(void);
void Mock_ccimp_hal_logging_vprintf_expect(debug_t const debug, char const * const buffer);

extern "C" {
void ccimp_hal_logging_vprintf_real(debug_t const debug, char const * const format, va_list args);
ccimp_status_t ccimp_hal_halt_real(void);

}

#endif
