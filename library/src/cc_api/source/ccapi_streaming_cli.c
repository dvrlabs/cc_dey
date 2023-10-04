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

#define CCAPI_CONST_PROTECTION_UNLOCK

#include "ccapi_definitions.h"

#if (defined CCIMP_STREAMING_CLI_SERVICE_ENABLED)

#define run_callback_if_available(cb, arg)      (cb) != NULL ? (cb)((arg)) : connector_callback_continue

connector_callback_status_t ccapi_streaming_cli_handler(connector_request_id_streaming_cli_service_t const request, void * const data, ccapi_data_t * const ccapi_data)
{
    connector_callback_status_t status = connector_callback_error;
    ccapi_streaming_cli_service_t * callbacks = &ccapi_data->service.streaming_cli.user_callback;

    switch (request)
    {
        case connector_request_id_streaming_cli_session_start:
            status = run_callback_if_available(callbacks->start_session, data);
            break;
        case connector_request_id_streaming_cli_poll:
            status = run_callback_if_available(callbacks->poll_session, data);
            break;
        case connector_request_id_streaming_cli_send:
            status = run_callback_if_available(callbacks->send_data, data);
            break;
        case connector_request_id_streaming_cli_receive:
            status = run_callback_if_available(callbacks->receive_data, data);
            break;
        case connector_request_id_streaming_cli_session_end:
            status = run_callback_if_available(callbacks->end_session, data);
            break;
        case connector_request_id_streaming_cli_sessionless_execute:
            status = run_callback_if_available(callbacks->sessionless_execute, data);
            break;
        case connector_request_id_streaming_cli_sessionless_store:
            status = run_callback_if_available(callbacks->sessionless_store, data);
            break;
    }

    return status;
}

#endif
