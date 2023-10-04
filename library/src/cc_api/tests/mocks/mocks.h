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

#ifndef _MOCKS_H_
#define _MOCKS_H_

#include "CppUTestExt/MockSupport.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <cstdio>

extern "C" {
#include "ccapi_definitions.h"
#include "ccimp/ccimp_logging.h"

extern char * assert_buffer;
extern char * assert_file;
}

extern "C" {
#include "ccapi/ccxapi.h"
}

#include "mock_ccimp_filesystem.h"
#include "mock_ccimp_logging.h"
#include "mock_ccimp_os.h"
#include "mock_ccimp_reset.h"
#include "mock_connector_api.h"
#include "mock_ccimp_network_tcp.h"
#include "mock_ccimp_network_udp.h"
#include "mock_ccimp_network_sms.h"
#include "mocks_comparators.h"

void Mock_create_all(void);
void Mock_destroy_all(void);

#endif
