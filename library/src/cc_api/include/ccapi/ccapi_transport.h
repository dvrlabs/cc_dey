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

#ifndef _CCAPI_TRANSPORT_H_
#define _CCAPI_TRANSPORT_H_

typedef enum {
#if (defined CCIMP_TCP_TRANSPORT_ENABLED)
    CCAPI_TRANSPORT_TCP,
#endif
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
    CCAPI_TRANSPORT_UDP,
#endif
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
    CCAPI_TRANSPORT_SMS
#endif
} ccapi_transport_t;

typedef enum {
    CCAPI_TRANSPORT_STOP_GRACEFULLY,
    CCAPI_TRANSPORT_STOP_IMMEDIATELY
} ccapi_transport_stop_t;

#include "ccapi/ccapi_transport_tcp.h"
#include "ccapi/ccapi_transport_udp.h"
#include "ccapi/ccapi_transport_sms.h"

#endif
