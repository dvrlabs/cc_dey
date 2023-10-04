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

#ifndef _MOCK_CCIMP_NETWORK_UDP_H_
#define _MOCK_CCIMP_NETWORK_UDP_H_

void Mock_ccimp_network_udp_open_create(void);
void Mock_ccimp_network_udp_open_destroy(void);
void Mock_ccimp_network_udp_open_expectAndReturn(ccimp_network_open_t * expect, ccimp_status_t retval);

void Mock_ccimp_network_udp_send_create(void);
void Mock_ccimp_network_udp_send_destroy(void);
void Mock_ccimp_network_udp_send_expectAndReturn(ccimp_network_send_t * expect, ccimp_status_t retval);

void Mock_ccimp_network_udp_receive_create(void);
void Mock_ccimp_network_udp_receive_destroy(void);
void Mock_ccimp_network_udp_receive_expectAndReturn(ccimp_network_receive_t * expect, ccimp_status_t retval);

void Mock_ccimp_network_udp_close_create(void);
void Mock_ccimp_network_udp_close_destroy(void);
void Mock_ccimp_network_udp_close_expectAndReturn(ccimp_network_close_t * expect, ccimp_status_t retval);

#endif
