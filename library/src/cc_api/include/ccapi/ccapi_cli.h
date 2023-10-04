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

#ifndef _CCAPI_CLI_H_
#define _CCAPI_CLI_H_

typedef enum {
    CCAPI_CLI_ERROR_NONE,
    CCAPI_CLI_ERROR_NO_CLI_SUPPORT,
    CCAPI_CLI_ERROR_INSUFFICIENT_MEMORY,
    CCAPI_CLI_ERROR_STATUS_CANCEL,
    CCAPI_CLI_ERROR_STATUS_ERROR
} ccapi_cli_error_t;

typedef void (*ccapi_cli_request_cb_t)(ccapi_transport_t const transport, char const * const command, char const * * const output);
typedef void (*ccapi_cli_finished_cb_t)(char * const output, ccapi_cli_error_t cli_error);


typedef struct {
    ccapi_cli_request_cb_t request;
    ccapi_cli_finished_cb_t finished;
} ccapi_cli_service_t;

#endif
