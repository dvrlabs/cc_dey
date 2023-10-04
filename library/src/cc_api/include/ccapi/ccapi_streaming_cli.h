/*
* Copyright (c) 2018 Digi International Inc.
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

#ifndef _CCAPI_STREAMING_CLI_H_
#define _CCAPI_STREAMING_CLI_H_

#if (defined CONNECTOR_STREAMING_CLI_SERVICE)

typedef unsigned int (*ccapi_streaming_cli_start_session_t)(connector_streaming_cli_session_start_request_t * request);
typedef unsigned int (*ccapi_streaming_cli_poll_session_t)(connector_streaming_cli_poll_request_t * request);
typedef unsigned int (*ccapi_streaming_cli_send_t)(connector_streaming_cli_session_send_data_t * request);
typedef unsigned int (*ccapi_streaming_cli_receive_t)(connector_streaming_cli_session_receive_data_t * request);
typedef unsigned int (*ccapi_streaming_cli_end_session_t)(connector_streaming_cli_session_end_request_t * request);
typedef unsigned int (*ccapi_streaming_cli_sessionless_execute_t)(connector_streaming_cli_session_sessionless_execute_run_request_t * request);
typedef unsigned int (*ccapi_streaming_cli_sessionless_store_t)(connector_streaming_cli_session_sessionless_execute_store_request_t * request);

typedef struct {
    ccapi_streaming_cli_start_session_t start_session;
    ccapi_streaming_cli_poll_session_t poll_session;
    ccapi_streaming_cli_send_t send_data;
    ccapi_streaming_cli_receive_t receive_data;
    ccapi_streaming_cli_end_session_t end_session;
    ccapi_streaming_cli_sessionless_execute_t sessionless_execute;
    ccapi_streaming_cli_sessionless_store_t sessionless_store;
} ccapi_streaming_cli_service_t;

#endif

#endif

