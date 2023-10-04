/*
Copyright 2019 Digi International Inc., All Rights Reserved

This software contains proprietary and confidential information of Digi
International Inc.  By accepting transfer of this copy, Recipient agrees
to retain this software in confidence, to prevent disclosure to others,
and to make no use of this software other than that for which it was
delivered.  This is an unpublished copyrighted work of Digi International
Inc.  Except as permitted by federal law, 17 USC 117, copying is strictly
prohibited.

Restricted Rights Legend

Use, duplication, or disclosure by the Government is subject to
restrictions set forth in sub-paragraph (c)(1)(ii) of The Rights in
Technical Data and Computer Software clause at DFARS 252.227-7031 or
subparagraphs (c)(1) and (2) of the Commercial Computer Software -
Restricted Rights at 48 CFR 52.227-19, as applicable.

Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
*/

#ifndef CONNECTOR_API_STREAMING_CLI_H
#define CONNECTOR_API_STREAMING_CLI_H

#if (defined CONNECTOR_STREAMING_CLI_SERVICE)

#ifndef CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH
#define CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH 1024
#else
#if CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH < 30
#error "CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH must be greater than 30"
#endif
#endif

typedef enum {
    connector_request_id_streaming_cli_session_start,
    connector_request_id_streaming_cli_poll,
    connector_request_id_streaming_cli_send,
    connector_request_id_streaming_cli_receive,
    connector_request_id_streaming_cli_session_end,
    connector_request_id_streaming_cli_sessionless_execute,
    connector_request_id_streaming_cli_sessionless_store
} connector_request_id_streaming_cli_service_t;

typedef struct {
    enum {
        connector_cli_terminal_vt100
    } CONST terminal_mode;
    void * handle;
    char * CONST error_hint;
    enum {
        connector_cli_session_start_ok,
        connector_cli_session_start_error
    } session_start_status;
    connector_bool_t read_only;
} connector_streaming_cli_session_start_request_t;

typedef struct {
    void * CONST handle;
    char * CONST error_hint;
} connector_streaming_cli_session_end_request_t;

typedef struct {
    void * CONST handle;
    char * CONST error_hint;
    enum {
        connector_cli_session_state_idle,
        connector_cli_session_state_readable,
        connector_cli_session_state_done
    } session_state;
} connector_streaming_cli_poll_request_t;

typedef struct {
    void * CONST handle;
    size_t CONST bytes_available;
    size_t bytes_used;
    uint8_t * CONST buffer;
    connector_bool_t more_data;
} connector_streaming_cli_session_send_data_t;

typedef struct {
    void * CONST handle;
    size_t CONST bytes_available;
    size_t bytes_used;
    uint8_t CONST * CONST buffer;
    connector_bool_t CONST more_data;
} connector_streaming_cli_session_receive_data_t;

typedef struct {
    void * handle;
    size_t CONST bytes_available;
    size_t bytes_used;
    uint8_t * CONST buffer;
    connector_bool_t more_data;
    int timeout;
    connector_bool_t init;
    connector_bool_t read_only;
} connector_streaming_cli_session_sessionless_execute_store_request_t;

typedef struct {
    void * CONST handle;
    size_t CONST bytes_available;
    size_t bytes_used;
    uint8_t * CONST buffer;
    connector_bool_t more_data;
    int status;
    connector_bool_t read_only;
} connector_streaming_cli_session_sessionless_execute_run_request_t;

#endif

#if !defined _CONNECTOR_API_H
#error  "Illegal inclusion of connector_api_streaming_cli.h. You should only include connector_api.h in user code."
#endif

#else
#error  "Illegal inclusion of connector_api_streaming_cli.h. You should only include connector_api.h in user code."
#endif
