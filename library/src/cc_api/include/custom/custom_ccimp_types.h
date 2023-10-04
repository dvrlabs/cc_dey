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

#ifndef _CUSTOM_CCIMP_TYPES_CONFIG_H_
#define _CUSTOM_CCIMP_TYPES_CONFIG_H_

#include "custom/custom_connector_config.h"

#if defined CCIMP_HAS_STDINT_HEADER
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#else
/**
* @defgroup user_types User Defined C types
* If your compiler is C89 complaint these defines are used: you will need
* to define them appropriately for your system.  If your compiler is C99 complaint
* then the types from stdint.h and inttypes.h are used.
* @{
*/
/**
 *  Unsigned 8 bit value.
 */
  typedef unsigned char uint8_t;

#define INT8_C(c)       c
#define UINT8_C(c)      c

#ifndef UINT8_MAX
/**
*  Unsigned 8 bit maximum value.
*/
#define UINT8_MAX  UINT8_C(0xFF)
#endif

/**
 *  Unsigned 16 bit value.
 */
  typedef unsigned short uint16_t;

#define INT16_C(c)      c
#define UINT16_C(c)     c

#ifndef UINT16_MAX
/**
*  Unsigned 16 bit maximum value.
*/
#define UINT16_MAX  UINT16_C(0xFFFF)
#endif

/**
 *  Unsigned 32 bit value.
 */
  typedef unsigned long int uint32_t;

#define INT32_C(c)      c
#define UINT32_C(c)     c ## U

#ifndef UINT32_MAX
/**
*  Unsigned 32 bit maximum value.
*/
#define UINT32_MAX UINT32_C(4294967295)
#endif

#ifndef SCNu32
/**
*  Scan format specifier for unsigned 32 bit value.
*/
#define SCNu32 "lu"
#endif

#ifndef PRIu32
/**
*  Print format specifier for unsigned 32 bit value.
*/
#define PRIu32 "lu"
#endif

/**
*  Signed 32 bit value.
*/
  typedef long int int32_t;

#ifndef INT32_MIN
/**
*  Signed 32 bit minimum value.
*/
#define INT32_MIN -INT32_C(2147483648)
#endif

#ifndef INT32_MAX
/**
*  Signed 32 bit maximum value.
*/
#define INT32_MAX INT32_C(2147483647)
#endif

#ifndef SCNd32
/**
*  Scan format specifier for signed 32 bit value.
*/
#define SCNd32 "ld"
#endif

#ifndef PRId32
/**
*  Print format specifier for signed 32 bit value.
*/
#define PRId32 "ld"
#endif

#ifndef SCNx32
/**
*  Scan format specifier for 32 bit hex value.
*/
#define SCNx32 "lx"
#endif

#ifndef PRIx32
/**
*  Print format specifier for 32 bit hex value.
*/
#define PRIx32 "lx"
#endif

#if (defined CONNECTOR_HAS_64_BIT_INTEGERS)

/**
 *  Unsigned 64 bit value.
 */
  typedef unsigned long long int uint64_t;


#define INT64_C(c)      c ## LL
#define UINT64_C(c)     c ## ULL

#ifndef UINT64_MAX
/**
*  Unsigned 64 bit maximum value.
*/
#define UINT64_MAX UINT64_C(18446744073709551615)
#endif


#ifndef SCNu64
/**
*  Scan format specifier for unsigned 64 bit value.
*/
#define SCNu64 "llu"
#endif

#ifndef PRIu64
/**
*  Print format specifier for unsigned 64 bit value.
*/
#define PRIu64 "llu"
#endif

/**
*  Signed 32 bit value.
*/
   typedef long long int int64_t;

#ifndef INT64_MIN
/**
*  Signed 64 bit minimum value.
*/
#define INT64_MIN -INT64_C(9223372036854775808)
#endif

#ifndef INT64_MAX
/**
*  Signed 64 bit maximum value.
*/
#define INT64_MAX INT64_C(9223372036854775807)
#endif

#ifndef SCNd64
/**
*  Scan format specifier for signed 64 bit value.
*/
#define SCNd64 "lld"
#endif

#ifndef PRId64
/**
*  Print format specifier for signed 64 bit value.
*/
#define PRId64 "lld"
#endif

#ifndef SCNx64
/**
*  Scan format specifier for 64 bit hex value.
*/
#define SCNx64 "llx"
#endif

#ifndef PRIx64
/**
*  Print format specifier for 64 bit hex value.
*/
#define PRIx64 "llx"
#endif

#endif

#ifndef SIZE_MAX
/**
*  size_t maximum value.
*/
#define SIZE_MAX  UINT32_MAX
#endif

/**
* @}
*/


#endif

#if __STDC_VERSION__ >= 199901L
#define PRIsize "zu"
#else
   /**
    *  Print format specifier for size_t.
    */
#define PRIsize "u"
#endif

typedef void * ccimp_network_handle_t;

#define CCIMP_NETWORK_HANDLE_NOT_INITIALIZED    NULL

typedef long int ccimp_fs_file_handle_t;
#define CCIMP_FILESYSTEM_FILE_HANDLE_NOT_INITIALIZED    -1

typedef void * ccimp_fs_dir_handle_t;
#define CCIMP_FILESYSTEM_DIR_HANDLE_NOT_INITIALIZED     NULL

typedef long int ccimp_fs_errnum_t;
#define CCIMP_FILESYSTEM_ERRNUM_NONE    0

#endif
