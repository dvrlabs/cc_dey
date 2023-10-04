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

#ifndef CONNECTOR_STREAMING_CLI_MAX_SESSIONS
#define CONNECTOR_STREAMING_CLI_MAX_SESSIONS 1
#else
#if (CONNECTOR_STREAMING_CLI_MAX_SESSIONS < 0 || CONNECTOR_STREAMING_CLI_MAX_SESSIONS > 255)
#error "CONNECTOR_STREAMING_CLI_MAX_SESSIONS must be between 1-255 or 0 for unlimited"
#endif
#endif

/* Opcodes */
#define STREAMING_CLI_OPCODE_CAPABILITIES       0x00
#define STREAMING_CLI_OPCODE_START_REQ          0x01
#define STREAMING_CLI_OPCODE_START_RESP         0x02
#define STREAMING_CLI_OPCODE_DATA               0x03
#define STREAMING_CLI_OPCODE_CLOSE              0x04
#define STREAMING_CLI_OPCODE_EXEC_REQ           0x05
#define STREAMING_CLI_OPCODE_EXEC_RESP          0x06

/* Status codes */
#define STREAMING_CLI_START_STATUS_SUCCESS      0x00
#define STREAMING_CLI_START_STATUS_ERROR        0x01
#define STREAMING_CLI_START_STATUS_NO_SESSIONS  0x02

#define STREAMING_CLI_EXEC_STATUS_SUCCESS       0x00
#define STREAMING_CLI_EXEC_STATUS_ERROR         0x01
#define STREAMING_CLI_EXEC_STATUS_NO_SESSIONS   0x02
#define STREAMING_CLI_EXEC_STATUS_TIMEOUT       0x03

static char const streaming_cli_service_unknown_session_error_msg[] = "Unknown session";

/* Message definitions */
enum streaming_cli_service_capabilities {
    field_define(streaming_cli_service_capabilities, opcode, uint8_t),
    field_define(streaming_cli_service_capabilities, max_concurrent_sessions, uint8_t),
    field_define(streaming_cli_service_capabilities, flags, uint8_t),
    record_end(streaming_cli_service_capabilities)
};

enum streaming_cli_service_session_start_request {
    field_define(streaming_cli_service_session_start_request, opcode, uint8_t),
    field_define(streaming_cli_service_session_start_request, session_id, uint16_t),
    field_define(streaming_cli_service_session_start_request, terminal_mode, uint8_t),
    field_define(streaming_cli_service_session_start_request, flags, uint8_t),
    record_end(streaming_cli_service_session_start_request)
};

enum streaming_cli_service_session_start_response {
    field_define(streaming_cli_service_session_start_response, opcode, uint8_t),
    field_define(streaming_cli_service_session_start_response, status, uint8_t),
    record_end(streaming_cli_service_session_start_response)
};

enum streaming_cli_service_session_data {
    field_define(streaming_cli_service_session_data, opcode, uint8_t),
    field_define(streaming_cli_service_session_data, session_id, uint16_t),
    record_end(streaming_cli_service_session_data)
};

enum streaming_cli_service_session_close {
    field_define(streaming_cli_service_session_close, opcode, uint8_t),
    field_define(streaming_cli_service_session_close, session_id, uint16_t),
    record_end(streaming_cli_service_session_close)
};

enum streaming_cli_service_execute_request {
    field_define(streaming_cli_service_execute_request, opcode, uint8_t),
    field_define(streaming_cli_service_execute_request, timeout, uint16_t),
    field_define(streaming_cli_service_execute_request, flags, uint8_t),
    record_end(streaming_cli_service_execute_request)
};

enum streaming_cli_service_execute_response {
    field_define(streaming_cli_service_execute_response, opcode, uint8_t),
    field_define(streaming_cli_service_execute_response, status, uint8_t),
    record_end(streaming_cli_service_execute_response)
};

enum CliServiceFlags {
    READ_ONLY  = 1 << 0
};

#define MAX_ERROR_MSG_SIZE (record_bytes(streaming_cli_service_session_close) + sizeof streaming_cli_service_unknown_session_error_msg)

/* Data structures */
typedef struct streaming_cli_session
{
    size_t bytes_consumed;
    void * handle;
    struct streaming_cli_session * prev;
    struct streaming_cli_session * next;
    enum
    {
        streaming_cli_session_state_uninitialized,
        streaming_cli_session_state_start_failed,
        streaming_cli_session_state_ack_start,
        streaming_cli_session_state_started,
        streaming_cli_session_state_closing,
        streaming_cli_session_state_send_close,
        streaming_cli_session_state_pending_error,
        streaming_cli_session_state_execute_command
    } session_state;
    union
    {
        struct
        {
            msg_session_t * active_recv_transaction;
            msg_session_t * active_send_transaction;
            uint16_t session_id;
            uint8_t terminal_mode;
        } streaming;
        struct
        {
            uint8_t status;
        } execute;
    } info;
    connector_bool_t read_only;
    uint8_t last_opcode;
    char close_reason[CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH];
} streaming_cli_session_t;

static struct
{
    size_t len;
    msg_session_t * transaction;
    uint8_t data[MAX_ERROR_MSG_SIZE];
} streaming_cli_error_buffer;

static unsigned int streaming_cli_num_sessions;
static streaming_cli_session_t * streaming_cli_sessions;

STATIC streaming_cli_session_t * streaming_cli_service_find_session(uint16_t const id)
{
    streaming_cli_session_t * first_session = streaming_cli_sessions;

    if (first_session != NULL)
    {
        streaming_cli_session_t * current_session = first_session;

        do
        {
            if (current_session->info.streaming.session_id == id)
            {
                return current_session;
            }
            current_session = current_session->next;
        } while (current_session != first_session);
    }
    return NULL;
}

STATIC connector_status_t streaming_cli_service_run_cb(connector_data_t * const connector_ptr, connector_request_id_streaming_cli_service_t const request, void * const arg)
{
    connector_callback_status_t callback_status;
    connector_request_id_t request_id;
    request_id.streaming_cli_service_request = request;

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_streaming_cli, request_id, arg, connector_ptr->context);
    switch (callback_status)
    {
        case connector_callback_continue:
            return connector_working;
            break;
        case connector_callback_busy:
            return connector_pending;
            break;
        default:
            return connector_abort;
            break;
    }
}

STATIC connector_status_t streaming_cli_service_read_data(connector_data_t * const connector_ptr, msg_session_t * const msg_session, msg_service_data_t * const service_data)
{
    connector_status_t status;
    streaming_cli_session_t * const session = msg_session->service_context;
    connector_streaming_cli_session_send_data_t request =
    {
        session->handle,
        service_data->length_in_bytes,
        0,
        service_data->data_ptr,
        connector_false
    };
#ifdef STREAMING_CLI_MAX_TRANSACTION_BYTES
    size_t bytes_remaining_for_transaction = (STREAMING_CLI_MAX_TRANSACTION_BYTES) - msg_session->out_dblock->total_bytes;
#endif

    if (MsgIsStart(service_data->flags))
    {
        uint8_t * const streaming_cli_service_session_data = service_data->data_ptr;
        message_store_u8(streaming_cli_service_session_data, opcode, STREAMING_CLI_OPCODE_DATA);
        message_store_be16(streaming_cli_service_session_data, session_id, session->info.streaming.session_id);
        request.bytes_available -= record_bytes(streaming_cli_service_session_data);
        request.buffer += record_bytes(streaming_cli_service_session_data);
    }
#ifdef STREAMING_CLI_MAX_TRANSACTION_BYTES
    else
    {
        bytes_remaining_for_transaction += record_bytes(streaming_cli_service_session_data);
    }
#endif

#ifdef STREAMING_CLI_MAX_TRANSACTION_BYTES
    if (request.bytes_available > bytes_remaining_for_transaction)
    {
        request.bytes_available = bytes_remaining_for_transaction;
    }
#endif

    status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_send, &request);
    if (status == connector_working)
    {
        service_data->length_in_bytes = MsgIsStart(service_data->flags) ? record_bytes(streaming_cli_service_session_data) + request.bytes_used : request.bytes_used;
#ifdef STREAMING_CLI_MAX_TRANSACTION_BYTES
        if (!request.more_data || request.bytes_used == bytes_remaining_for_transaction)
#else
        if (!request.more_data)
#endif
        {
            MsgSetLastData(service_data->flags);
        }
    }

    return status;
}

STATIC connector_status_t streaming_cli_service_write_data(connector_data_t * const connector_ptr, streaming_cli_session_t * const session, msg_service_data_t * const service_data)
{
    uint8_t * const buffer = service_data->data_ptr;
    if (session->session_state == streaming_cli_session_state_started)
    {
        connector_status_t status;
        connector_streaming_cli_session_receive_data_t request =
        {
            session->handle,
            service_data->length_in_bytes - session->bytes_consumed,
            0,
            buffer + session->bytes_consumed,
            connector_false
        };

        if (MsgIsStart(service_data->flags) && session->bytes_consumed == 0)
        {
            request.bytes_available -= record_bytes(streaming_cli_service_session_data);
            request.buffer += record_bytes(streaming_cli_service_session_data);
            session->bytes_consumed = record_bytes(streaming_cli_service_session_data);
        }

        status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_receive, &request);
        if (status == connector_working)
        {
            session->bytes_consumed += request.bytes_used;
            if (session->bytes_consumed == service_data->length_in_bytes)
            {
                session->bytes_consumed = 0;
                return connector_working;
            }
            else
            {
                return connector_pending;
            }
        }
        return status;
    }
    else
    {
        return connector_working;
    }
}

STATIC connector_status_t streaming_cli_service_start_session(connector_data_t * const connector_ptr, streaming_cli_session_t * const session)
{
    connector_status_t status;

    if (session->session_state == streaming_cli_session_state_uninitialized)
    {
        connector_streaming_cli_session_start_request_t request = {
            session->info.streaming.terminal_mode,
            NULL,
            session->close_reason,
            connector_cli_session_start_ok,
            session->read_only
        };
        status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_session_start, &request);
        if (status == connector_working)
        {
            session->handle = request.handle;
            if (request.session_start_status == connector_cli_session_start_ok)
            {
                session->session_state = streaming_cli_session_state_ack_start;
            }
            else
            {
                session->session_state = streaming_cli_session_state_start_failed;
            }
        }
    }
    else
    {
        connector_streaming_cli_session_end_request_t request = { session->handle, "Restart" };
        if (session->info.streaming.active_recv_transaction != NULL)
        {
            session->info.streaming.active_recv_transaction->service_context = NULL;
            session->info.streaming.active_recv_transaction = NULL;
        }
        if (session->info.streaming.active_send_transaction != NULL)
        {
            session->info.streaming.active_send_transaction->service_context = NULL;
            session->info.streaming.active_send_transaction = NULL;
        }
        status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_session_end, &request);
        if (status == connector_working)
        {
            session->bytes_consumed = 0;
            session->handle = NULL;
            session->close_reason[0] = '\0';
            session->session_state = streaming_cli_session_state_uninitialized;
            status = connector_pending;
        }
    }

    return status;
}

STATIC connector_status_t streaming_cli_service_remove_session(connector_data_t * const connector_ptr, streaming_cli_session_t * session)
{
    connector_status_t status;
    streaming_cli_session_t ** head_ptr = &streaming_cli_sessions;
    remove_circular_node(head_ptr, session);
    if (session->session_state != streaming_cli_session_state_execute_command)
    {
        if (session->info.streaming.active_recv_transaction)
        {
            session->info.streaming.active_recv_transaction->service_context = NULL;
        }
        if (session->info.streaming.active_send_transaction)
        {
            session->info.streaming.active_send_transaction->service_context = NULL;
        }
    }
    status = free_data_buffer(connector_ptr, named_buffer_id(streaming_cli_session), session);
    streaming_cli_num_sessions--;

    return status;
}

STATIC connector_status_t streaming_cli_service_close_session(connector_data_t * const connector_ptr, streaming_cli_session_t * session)
{
    connector_status_t status = connector_working;

    if (session->session_state != streaming_cli_session_state_uninitialized)
    {
        connector_streaming_cli_session_end_request_t request = {
            session->handle,
            session->close_reason
        };
        status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_session_end, &request);
    }

    if (status == connector_working)
    {
        return streaming_cli_service_remove_session(connector_ptr, session);
    }
    return status;
}

STATIC connector_status_t streaming_cli_service_execute_store_command(connector_data_t * const connector_ptr, streaming_cli_session_t * const session, msg_service_data_t * const service_data)
{
    connector_status_t status = connector_working;
    uint8_t * const buffer = service_data->data_ptr;

    connector_streaming_cli_session_sessionless_execute_store_request_t request;
    request.bytes_available = service_data->length_in_bytes - session->bytes_consumed;
    request.bytes_used = 0;
    request.buffer = buffer + session->bytes_consumed;
    request.more_data = connector_bool(MsgIsLastData(service_data->flags));
    request.init = connector_false;

    if connector_bool(MsgIsStart(service_data->flags)) {
        uint8_t * const streaming_cli_service_execute_request = service_data->data_ptr;
        request.bytes_available -= record_bytes(streaming_cli_service_execute_request);
        request.buffer += record_bytes(streaming_cli_service_execute_request);
        session->bytes_consumed = record_bytes(streaming_cli_service_execute_request);
        request.timeout = (int) message_load_be16(streaming_cli_service_execute_request, timeout);
        session->read_only = (connector_bool_t) (READ_ONLY & message_load_u8(streaming_cli_service_execute_request, flags));
        request.init = connector_true;
    }

    request.handle = session->handle;
    request.read_only = session->read_only;
    status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_sessionless_store, &request);
    session->handle = request.handle;

    if (status == connector_working)
    {
        session->bytes_consumed += request.bytes_used;
        if (session->bytes_consumed == service_data->length_in_bytes)
        {
            session->bytes_consumed = 0;
            return connector_working;
        }
        else
        {
            return connector_pending;
        }
    }
    return connector_working;
}

STATIC connector_status_t streaming_cli_service_execute_run_command(connector_data_t * const connector_ptr, msg_session_t * const msg_session, msg_service_data_t * const service_data)
{
    connector_status_t status;
    uint8_t * const streaming_cli_service_execute_response = service_data->data_ptr;
    streaming_cli_session_t * const session = msg_session->service_context;

    connector_streaming_cli_session_sessionless_execute_run_request_t request;
    request.handle = session->handle;
    request.bytes_available = service_data->length_in_bytes - 1; // subtract 1 byte to leave room for status
    request.bytes_used = 0;
    request.buffer = service_data->data_ptr;
    request.more_data = connector_false;
    request.status = session->info.execute.status;
    request.read_only = session->read_only;

    if (MsgIsStart(service_data->flags))
    {
        message_store_u8(streaming_cli_service_execute_response, opcode, STREAMING_CLI_OPCODE_EXEC_RESP);
        request.bytes_available -= record_bytes(streaming_cli_service_execute_response);
        request.buffer += record_bytes(streaming_cli_service_execute_response);
    }

    status = streaming_cli_service_run_cb(connector_ptr, connector_request_id_streaming_cli_sessionless_execute, &request);
    session->info.execute.status = request.status;

    if (status == connector_working)
    {   
        service_data->length_in_bytes = MsgIsStart(service_data->flags) ? record_bytes(streaming_cli_service_execute_response) + request.bytes_used : request.bytes_used;
        if (!request.more_data)
        {
            uint8_t status_code = request.status == 0 ? STREAMING_CLI_EXEC_STATUS_SUCCESS : request.status == -13 ? STREAMING_CLI_EXEC_STATUS_TIMEOUT : STREAMING_CLI_EXEC_STATUS_ERROR;
            streaming_cli_service_execute_response[service_data->length_in_bytes] = status_code;
            service_data->length_in_bytes++;  //storing another byte need to increment
            MsgSetLastData(service_data->flags);
            status = streaming_cli_service_close_session(connector_ptr, session);
        }
    }
    return status;
}

STATIC connector_status_t streaming_cli_service_set_max_sessions_error(uint8_t const opcode, msg_session_t * const msg_session)
{
    if (streaming_cli_error_buffer.transaction != NULL) return connector_pending;

    streaming_cli_error_buffer.transaction = msg_session;

    if (opcode == STREAMING_CLI_OPCODE_START_REQ)
    {
        uint8_t * const streaming_cli_service_session_start_response = streaming_cli_error_buffer.data;
        message_store_u8(streaming_cli_service_session_start_response, opcode, STREAMING_CLI_OPCODE_START_RESP);
        message_store_u8(streaming_cli_service_session_start_response, status, STREAMING_CLI_START_STATUS_NO_SESSIONS);
        streaming_cli_service_session_start_response[record_bytes(streaming_cli_service_session_start_response)] = '\0';
        streaming_cli_error_buffer.len = record_bytes(streaming_cli_service_session_start_response) + 1;
    }
    else if (opcode == STREAMING_CLI_OPCODE_EXEC_REQ)
    {
        uint8_t * const streaming_cli_service_execute_response = streaming_cli_error_buffer.data;
        message_store_u8(streaming_cli_service_execute_response, opcode, STREAMING_CLI_OPCODE_EXEC_RESP);
        message_store_u8(streaming_cli_service_execute_response, status, STREAMING_CLI_EXEC_STATUS_NO_SESSIONS);
        streaming_cli_service_execute_response[record_bytes(streaming_cli_service_execute_response)] = '\0';
        streaming_cli_error_buffer.len = record_bytes(streaming_cli_service_execute_response) + 1;
    }
    return connector_working;
}

STATIC connector_status_t streaming_cli_service_set_unknown_session_error(connector_data_t * const connector_ptr, uint16_t const session_id, msg_session_t * const msg_session)
{
    if (streaming_cli_error_buffer.transaction != NULL) return connector_pending;

    {
        connector_status_t status;
        connector_msg_data_t * const msg_ptr = get_facility_data(connector_ptr, E_MSG_FAC_MSG_NUM);
        msg_session_t * response_session = msg_create_session(connector_ptr, msg_ptr, msg_service_id_cli_extended, connector_true, &status);
        if (status == connector_working)
        {
            if (msg_initialize_data_block(response_session, msg_ptr->capabilities[msg_capability_cloud].window_size,
                                          msg_block_state_send_request) == connector_session_error_none)
            {
                uint8_t * streaming_cli_service_session_close;

                MsgSetNoReply(msg_session->in_dblock->status_flag);
                streaming_cli_error_buffer.transaction = response_session;
                streaming_cli_service_session_close = streaming_cli_error_buffer.data;

                message_store_u8(streaming_cli_service_session_close, opcode, STREAMING_CLI_OPCODE_CLOSE);
                message_store_be16(streaming_cli_service_session_close, session_id, session_id);
                memcpy(GET_PACKET_DATA_POINTER(streaming_cli_service_session_close, record_bytes(streaming_cli_service_session_close)),
                       streaming_cli_service_unknown_session_error_msg, sizeof streaming_cli_service_unknown_session_error_msg);

                streaming_cli_error_buffer.len = record_bytes(streaming_cli_service_session_close) +
                                                              sizeof streaming_cli_service_unknown_session_error_msg;
            }
            else
            {
                status = msg_delete_session(connector_ptr, msg_ptr, response_session);
            }
        }
        return status;
    }
}

STATIC connector_status_t streaming_cli_service_set_general_start_error(msg_session_t * const msg_session)
{
    uint8_t * streaming_cli_service_session_start_response;
    if (streaming_cli_error_buffer.transaction != NULL) return connector_pending;

    streaming_cli_service_session_start_response = streaming_cli_error_buffer.data;
    streaming_cli_error_buffer.transaction = msg_session;

    message_store_u8(streaming_cli_service_session_start_response, opcode, STREAMING_CLI_OPCODE_START_RESP);
    message_store_u8(streaming_cli_service_session_start_response, status, STREAMING_CLI_START_STATUS_ERROR);

    streaming_cli_error_buffer.len = record_bytes(streaming_cli_service_session_start_response);

    return connector_working;
}

STATIC void streaming_cli_service_copy_close_message(streaming_cli_session_t * const session, msg_service_data_t const * const service_data)
{
    size_t const offset = MsgIsStart(service_data->flags) ? record_bytes(streaming_cli_service_session_close) : 0;
    size_t const num_bytes = service_data->length_in_bytes - offset;
    size_t const total_bytes = session->bytes_consumed + num_bytes;
    char const * const buffer = service_data->data_ptr;

    if (MsgIsLastData(service_data->flags) && *(buffer + service_data->length_in_bytes - 1) != '\0')
    {
        char const error_msg[] = "Close msg malformed";
        memcpy(session->close_reason, error_msg, sizeof error_msg);
    }
    else if (total_bytes > CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH)
    {
        char const error_msg[] = "Close msg too long";
        memcpy(session->close_reason, error_msg, sizeof error_msg);
        session->bytes_consumed = CONNECTOR_STREAMING_CLI_MAX_CLOSE_REASON_LENGTH;
    }
    else
    {
        memcpy(session->close_reason + session->bytes_consumed, buffer + offset, num_bytes);
        session->bytes_consumed = total_bytes;
    }
}

STATIC connector_status_t streaming_cli_service_handle_close_request(connector_data_t * const connector_ptr, msg_session_t * const msg_session, msg_service_data_t * const service_data)
{
    streaming_cli_session_t * const session = msg_session->service_context;

    if (MsgIsStart(service_data->flags))
    {
        session->bytes_consumed = 0;
        session->session_state = streaming_cli_session_state_closing;
        if (session->info.streaming.active_send_transaction != NULL)
        {
            session->info.streaming.active_send_transaction->service_context = NULL;
            session->info.streaming.active_send_transaction = NULL;
        }
    }

    streaming_cli_service_copy_close_message(session, service_data);

    if (MsgIsLastData(service_data->flags))
    {
        MsgSetNoReply(msg_session->in_dblock->status_flag);
        return streaming_cli_service_close_session(connector_ptr, session);
    }
    else
    {
        return connector_working;
    }
}

STATIC connector_status_t streaming_cli_service_handle_receive_transaction(connector_data_t * const connector_ptr, msg_session_t * const msg_session, msg_service_data_t * const service_data)
{
    connector_status_t status = connector_abort;
    streaming_cli_session_t * const session = msg_session->service_context;
    if (session != NULL && session->last_opcode != STREAMING_CLI_OPCODE_EXEC_REQ) {
        ASSERT(session->info.streaming.active_recv_transaction == msg_session);
    }

    switch (session->last_opcode)
    {
        case STREAMING_CLI_OPCODE_START_REQ:
            status = streaming_cli_service_start_session(connector_ptr, session);
            break;
        case STREAMING_CLI_OPCODE_DATA:
            MsgSetNoReply(msg_session->in_dblock->status_flag);
            status = streaming_cli_service_write_data(connector_ptr, session, service_data);
            break;
        case STREAMING_CLI_OPCODE_CLOSE:
            return streaming_cli_service_handle_close_request(connector_ptr, msg_session, service_data);
            break;
        case STREAMING_CLI_OPCODE_EXEC_REQ:
            status = streaming_cli_service_execute_store_command(connector_ptr, session, service_data);
            break;
    }

    if (MsgIsLastData(service_data->flags))
    {
        session->info.streaming.active_recv_transaction = NULL;

        if (MsgReplyExpected(msg_session->in_dblock->status_flag))
        {
            session->info.streaming.active_send_transaction = msg_session;
        }
        else
        {
            msg_session->service_context = NULL;
        }
    }
    return status;
}

STATIC streaming_cli_session_t * streaming_cli_service_create_new_session(connector_data_t * const connector_ptr, msg_session_t * const msg_session, uint8_t * const data, connector_status_t * const status)
{
    streaming_cli_session_t * session;
    uint8_t const opcode = *data;

    if (CONNECTOR_STREAMING_CLI_MAX_SESSIONS != 0 && streaming_cli_num_sessions >= CONNECTOR_STREAMING_CLI_MAX_SESSIONS)
    {
        *status = streaming_cli_service_set_max_sessions_error(opcode, msg_session);
        return NULL;
    }
    if (malloc_data_buffer(connector_ptr, sizeof *session, named_buffer_id(streaming_cli_session), (void **) &session) != connector_working)
    {
        *status = streaming_cli_service_set_general_start_error(msg_session);
        return NULL;
    }
    streaming_cli_num_sessions++;
    session->session_state = streaming_cli_session_state_uninitialized;
    session->bytes_consumed = 0;
    session->last_opcode = opcode;
    session->close_reason[0] = '\0';
    streaming_cli_session_t ** const head_ptr = &streaming_cli_sessions;
    switch (opcode)
    {
        case STREAMING_CLI_OPCODE_START_REQ:
        {
            uint8_t * const streaming_cli_service_session_start_request = data;

            session->info.streaming.active_recv_transaction = NULL;
            session->info.streaming.active_send_transaction = NULL;

            session->info.streaming.session_id = message_load_be16(streaming_cli_service_session_start_request, session_id);
            {
                uint8_t const terminal_mode = message_load_u8(streaming_cli_service_session_start_request, terminal_mode);

                ASSERT(terminal_mode == 0); UNUSED_VARIABLE(terminal_mode);
                session->info.streaming.terminal_mode = connector_cli_terminal_vt100;
                session->read_only = (connector_bool_t) (READ_ONLY & message_load_u8(streaming_cli_service_session_start_request, flags));
            }
            add_circular_node(head_ptr, session);
            break;
        }

        case STREAMING_CLI_OPCODE_EXEC_REQ:
        {
            
            session->prev = NULL;
            session->next = NULL;
            session->session_state = streaming_cli_session_state_execute_command;
            session->info.execute.status = STREAMING_CLI_EXEC_STATUS_SUCCESS;
            session->info.streaming.active_recv_transaction = NULL;
            session->info.streaming.active_send_transaction = NULL;
            add_circular_node(head_ptr, session);
            break;
        }

        default:
        {
            ASSERT(connector_false);
            break;
        }
    }

    *status = connector_working;
    return session;
}

STATIC connector_status_t streaming_cli_service_request_callback(connector_data_t * const connector_ptr, msg_service_request_t * const service_request)
{
    msg_service_data_t * const service_data = service_request->have_data;
    msg_session_t * const msg_session = service_request->session;
    streaming_cli_session_t * session = msg_session->service_context;

    if (MsgIsStart(service_data->flags))
    {
        uint8_t const opcode = *((uint8_t *) service_data->data_ptr);

        if (session == NULL)
        {
            uint16_t session_id = 0;

            switch(opcode)
            {
                case STREAMING_CLI_OPCODE_START_REQ:
                case STREAMING_CLI_OPCODE_DATA:
                case STREAMING_CLI_OPCODE_CLOSE:
                {
                    uint8_t * const streaming_cli_service_session_start_request = service_data->data_ptr;
                    session_id = message_load_be16(streaming_cli_service_session_start_request, session_id);
                    session = streaming_cli_service_find_session(session_id);
                    break;
                }
                case STREAMING_CLI_OPCODE_EXEC_REQ:
                {
                    break;
                }
                default:
                    msg_set_error(msg_session, connector_session_error_invalid_opcode);
                    return connector_working;
                    break;
            }

            if (session == NULL)
            {
                connector_status_t status = connector_working;

                if (opcode == STREAMING_CLI_OPCODE_DATA)
                {
                    return streaming_cli_service_set_unknown_session_error(connector_ptr, session_id, msg_session);
                }
                if (opcode == STREAMING_CLI_OPCODE_CLOSE)
                {
                    MsgSetNoReply(msg_session->in_dblock->status_flag);
                    return connector_working;
                }
                session = streaming_cli_service_create_new_session(connector_ptr, msg_session, service_data->data_ptr, &status);

                if (session == NULL) return status;
            }

            if (opcode != STREAMING_CLI_OPCODE_EXEC_REQ)
            {
                if (session->info.streaming.active_recv_transaction != NULL)
                {
                    session->info.streaming.active_recv_transaction->service_context = NULL;
                    if (opcode == STREAMING_CLI_OPCODE_DATA)
                    {
                        static char const error_msg[] = "Message received out of order";
                        memcpy(session->close_reason, error_msg, sizeof error_msg);
                        session->session_state = streaming_cli_session_state_pending_error;
                        return connector_working;
                    }
                }
                if (opcode != STREAMING_CLI_OPCODE_DATA || session->session_state == streaming_cli_session_state_started)
                {
                    msg_session->service_context = session;
                    session->info.streaming.active_recv_transaction = msg_session;
                }
                else
                {
                    /* Received data for closing session. Discard */
                    return connector_working;
                }
            }
            session->last_opcode = opcode;
        }
    }
    else if (session == NULL)
    {
        /* Received data for cancelled session. Discard */
        return connector_working;
    }

    msg_session->service_context = session;
    return streaming_cli_service_handle_receive_transaction(connector_ptr, msg_session, service_data);
}

STATIC void streaming_cli_service_write_error_hint(streaming_cli_session_t * session, msg_service_data_t * const service_data, size_t offset)
{
    uint8_t * const packet = service_data->data_ptr;

    char const * const error_hint = session->close_reason + session->bytes_consumed;
    size_t bytes_left = service_data->length_in_bytes - offset;
    size_t hint_len = strlen(error_hint) + 1;

    if (bytes_left < hint_len)
    {
        memcpy(GET_PACKET_DATA_POINTER(packet, offset), error_hint, bytes_left);
        session->bytes_consumed += bytes_left;
    }
    else
    {
        memcpy(GET_PACKET_DATA_POINTER(packet, offset), error_hint, hint_len);
        service_data->length_in_bytes = offset + hint_len;
        MsgSetLastData(service_data->flags);
    }
}

STATIC connector_status_t streaming_cli_service_handle_start_response(connector_data_t * const connector_ptr, msg_session_t * const msg_session, msg_service_data_t * const service_data)
{
    connector_status_t status;
    streaming_cli_session_t * const session = msg_session->service_context;
    uint8_t * const streaming_cli_service_session_start_response = service_data->data_ptr;
    size_t bytes_used = 0;

    if (MsgIsStart(service_data->flags))
    {
        message_store_u8(streaming_cli_service_session_start_response, opcode, STREAMING_CLI_OPCODE_START_RESP);
        if (session->session_state == streaming_cli_session_state_ack_start)
        {
            session->session_state = streaming_cli_session_state_started;
            message_store_u8(streaming_cli_service_session_start_response, status, STREAMING_CLI_START_STATUS_SUCCESS);
        }
        else
        {
            message_store_u8(streaming_cli_service_session_start_response, status, STREAMING_CLI_START_STATUS_ERROR);
        }
        bytes_used = record_bytes(streaming_cli_service_session_start_response);
    }

    streaming_cli_service_write_error_hint(session, service_data, bytes_used);

    if (MsgIsLastData(service_data->flags))
    {
        if (session->session_state == streaming_cli_session_state_start_failed)
        {
            return streaming_cli_service_close_session(connector_ptr, session);
        }
        else
        {
            msg_session->service_context = NULL;
            session->info.streaming.active_send_transaction = NULL;
            status = connector_working;
        }
    }
    else
    {
        status = connector_working;
    }

    return status;
}

STATIC connector_status_t streaming_cli_service_send_close(connector_data_t * const connector_ptr, msg_session_t * const msg_session, msg_service_data_t * const service_data)
{
    connector_status_t status;
    streaming_cli_session_t * const session = msg_session->service_context;
    size_t bytes_used = 0;

    ASSERT(session->session_state == streaming_cli_session_state_send_close);

    if (MsgIsStart(service_data->flags))
    {
        uint8_t * const streaming_cli_service_session_close = service_data->data_ptr;

        session->bytes_consumed = 0;
        message_store_u8(streaming_cli_service_session_close, opcode, STREAMING_CLI_OPCODE_CLOSE);
        message_store_be16(streaming_cli_service_session_close, session_id, session->info.streaming.session_id);

        bytes_used = record_bytes(streaming_cli_service_session_close);
    }

    streaming_cli_service_write_error_hint(session, service_data, bytes_used);

    if (MsgIsLastData(service_data->flags))
    {
        MsgSetNoReply(msg_session->out_dblock->status_flag);
        return streaming_cli_service_close_session(connector_ptr, session);
    }
    else
    {
        status = connector_working;
    }

    return status;
}

STATIC connector_status_t streaming_cli_service_response_callback(connector_data_t * const connector_ptr, msg_service_request_t * const service_request)
{
    connector_status_t status;
    msg_service_data_t * const service_data = service_request->need_data;
    msg_session_t * const msg_session = service_request->session;
    streaming_cli_session_t * const session = msg_session->service_context;

    if (session == NULL)
    {
        if (streaming_cli_error_buffer.transaction == msg_session)
        {
            ASSERT(service_data->length_in_bytes >= streaming_cli_error_buffer.len);
            memcpy(service_data->data_ptr, streaming_cli_error_buffer.data, streaming_cli_error_buffer.len);
            service_data->length_in_bytes = streaming_cli_error_buffer.len;
            MsgSetLastData(service_data->flags);
            if (MsgIsRequest(msg_session->out_dblock->status_flag))
            {
                MsgSetNoReply(msg_session->out_dblock->status_flag);
            }
            streaming_cli_error_buffer.transaction = NULL;
        }
        else
        {
            /* If session is null and the pending errror is not for this transaction the streaming session was canceled, cancel the transaction */
            msg_set_error(msg_session, connector_session_error_cancel);
            return connector_pending;
        }
        return connector_working;
    }

    ASSERT(session->info.streaming.active_send_transaction == msg_session);

    if (session->session_state == streaming_cli_session_state_ack_start || session->session_state == streaming_cli_session_state_start_failed)
    {
        status = streaming_cli_service_handle_start_response(connector_ptr, msg_session, service_data);
    }
    else if (session->session_state == streaming_cli_session_state_started)
    {
        MsgSetNoReply(msg_session->out_dblock->status_flag);
        status = streaming_cli_service_read_data(connector_ptr, msg_session, service_data);

        if (status == connector_working && MsgIsLastData(service_data->flags))
        {
            msg_session->service_context = NULL;
            session->info.streaming.active_send_transaction = NULL;
        }
    }
    else if (session->session_state == streaming_cli_session_state_execute_command)
    {
            status = streaming_cli_service_execute_run_command(connector_ptr, msg_session, service_data);
    }
    else
    {
        status = streaming_cli_service_send_close(connector_ptr, msg_session, service_data);
    }

    return status;
}

STATIC connector_status_t streaming_cli_service_callback(connector_data_t * const connector_ptr, msg_service_request_t * const service_request)
{
    connector_status_t status = connector_abort;

    ASSERT_GOTO(connector_ptr != NULL, done);
    ASSERT_GOTO(service_request != NULL, done);
    ASSERT_GOTO((service_request->service_type == msg_service_type_capabilities) || (service_request->session != NULL), done);

    switch (service_request->service_type)
    {
        case msg_service_type_have_data:
            status = streaming_cli_service_request_callback(connector_ptr, service_request);
            break;

        case msg_service_type_need_data:
            status = streaming_cli_service_response_callback(connector_ptr, service_request);
            break;

        case msg_service_type_error:
        {

            msg_session_t * const msg_session = service_request->session;
            streaming_cli_session_t * const session = msg_session->service_context;
            if (session != NULL)
            {
                static char const error_msg[] = "Messaging service error";
                memcpy(session->close_reason, error_msg, sizeof error_msg);

                if (session->session_state == streaming_cli_session_state_started)
                {
                    if (session->info.streaming.active_recv_transaction != NULL)
                    {
                        session->info.streaming.active_recv_transaction->service_context = NULL;
                        session->info.streaming.active_recv_transaction = NULL;
                    }
                    if (session->info.streaming.active_send_transaction != NULL)
                    {
                        session->info.streaming.active_send_transaction->service_context = NULL;
                        session->info.streaming.active_send_transaction = NULL;
                    }
                    session->session_state = streaming_cli_session_state_pending_error;
                }
                else
                {
                    return streaming_cli_service_close_session(connector_ptr, session);
                }
            }
            status = connector_working;
            break;
        }

        case msg_service_type_free:
            status = connector_working;
            break;

        case msg_service_type_capabilities:
        {
            uint8_t * streaming_cli_service_capabilities;
            msg_service_data_t * const service_data = service_request->need_data;
            uint8_t capabilities_flags = 0;
            ASSERT(service_data != NULL);
            ASSERT(service_data->data_ptr != NULL);

            streaming_cli_service_capabilities = service_data->data_ptr;
            message_store_u8(streaming_cli_service_capabilities, opcode, STREAMING_CLI_OPCODE_CAPABILITIES);
            message_store_u8(streaming_cli_service_capabilities, max_concurrent_sessions, CONNECTOR_STREAMING_CLI_MAX_SESSIONS);
#ifdef CONNECTOR_STREAMING_CLI_CAPABILITIES_EXECUTE
            capabilities_flags |= 0x01;
#endif
#ifdef CONNECTOR_STREAMING_CLI_CAPABILITIES_READ_ONLY
            capabilities_flags |= 0x02;
#endif
            message_store_u8(streaming_cli_service_capabilities, flags, capabilities_flags);
            service_data->length_in_bytes = record_bytes(streaming_cli_service_capabilities);

            status = connector_working;
            break;
        }

        default:
            ASSERT(connector_false);
            break;
    }

done:
    return status;
}

STATIC connector_status_t connector_facility_streaming_cli_service_cleanup(connector_data_t * const connector_ptr)
{
    connector_status_t status = msg_cleanup_all_sessions(connector_ptr, msg_service_id_cli_extended);

    while (status == connector_working && streaming_cli_sessions != NULL)
    {
        streaming_cli_session_t * session = streaming_cli_sessions;
        status = streaming_cli_service_close_session(connector_ptr, session);
    }

    return status;
}

STATIC connector_status_t connector_facility_streaming_cli_service_delete(connector_data_t * const data_ptr)
{
    return msg_delete_facility(data_ptr, msg_service_id_cli_extended);
}

STATIC connector_status_t connector_facility_streaming_cli_service_init(connector_data_t * const data_ptr, unsigned int const facility_index)
{
    streaming_cli_error_buffer.transaction = NULL;
    streaming_cli_num_sessions = 0;
    streaming_cli_sessions = NULL;
    return msg_init_facility(data_ptr, facility_index, msg_service_id_cli_extended, streaming_cli_service_callback);
}

STATIC connector_bool_t streaming_cli_service_create_transaction(connector_data_t * const data_ptr, connector_msg_data_t * const msg_ptr, streaming_cli_session_t * const session, connector_status_t * const status)
{
    msg_session_t * const msg_session = msg_create_session(data_ptr, msg_ptr, msg_service_id_cli_extended, connector_true, status);
    if (*status == connector_working)
    {
        if (msg_initialize_data_block(msg_session, msg_ptr->capabilities[msg_capability_cloud].window_size, msg_block_state_send_request) == connector_session_error_none)
        {
            msg_session->service_context = session;
            session->info.streaming.active_send_transaction = msg_session;
            return connector_true;
        }
        *status = msg_delete_session(data_ptr, msg_ptr, msg_session);
    }
    return connector_false;
}

STATIC connector_status_t streaming_cli_service_run_poll_callback(connector_data_t * const data_ptr, connector_msg_data_t * const msg_ptr, streaming_cli_session_t * const session)
{
    connector_status_t status;
    connector_streaming_cli_poll_request_t request = {
        session->handle,
        session->close_reason,
        connector_cli_session_state_idle
    };

    status = streaming_cli_service_run_cb(data_ptr, connector_request_id_streaming_cli_poll, &request);
    if (status == connector_working)
    {
        switch (request.session_state)
        {
            case connector_cli_session_state_idle:
                status = connector_idle;
                break;
            case connector_cli_session_state_readable:
                streaming_cli_service_create_transaction(data_ptr, msg_ptr, session, &status);
                break;
            case connector_cli_session_state_done:
                if (streaming_cli_service_create_transaction(data_ptr, msg_ptr, session, &status))
                {
                    session->session_state = streaming_cli_session_state_send_close;
                }
                break;
        }
    }
    return status;
}

STATIC connector_status_t streaming_cli_service_poll_sessions(connector_data_t * const data_ptr, connector_msg_data_t * const msg_ptr)
{
    connector_status_t status = connector_idle;

    if (streaming_cli_sessions != NULL)
    {
        streaming_cli_session_t * first_session = streaming_cli_sessions;
        streaming_cli_session_t * current_session = first_session;

        do
        {
            if (current_session->session_state == streaming_cli_session_state_started && current_session->info.streaming.active_send_transaction == NULL)
            {
                status = streaming_cli_service_run_poll_callback(data_ptr, msg_ptr, current_session);
                break;
            }
            else if (current_session->session_state == streaming_cli_session_state_pending_error)
            {
                streaming_cli_service_create_transaction(data_ptr, msg_ptr, current_session, &status);
                if (status == connector_working)
                {
                    current_session->session_state = streaming_cli_session_state_send_close;
                }
                break;
            }
            current_session = current_session->next;
        } while (current_session != first_session);

        if (status == connector_idle || status == connector_working)
        {
            streaming_cli_sessions = first_session->next;
        }
    }

    return status;
}
