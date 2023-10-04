/*
Copyright 2019, Digi International Inc.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#if (defined SM_CONFIGURATION)
typedef enum
{
    sm_config_service_opcode_capabilities,
    sm_config_service_opcode_key_set,
    sm_config_service_opcode_key_response,
    sm_config_service_opcode_key_error,
} sm_config_service_opcode_t;

/* Opcodes */
#define SM_CONFIG_OPCODE_CAPABILITIES   0x00
#define SM_CONFIG_OPCODE_KEY_SET        0x01
#define SM_CONFIG_OPCODE_KEY_RESPONSE   0x02
#define SM_CONFIG_OPCODE_KEY_ERROR      0x03

/* Flags */
#define SM_CAPABILITY_SMS               0x01
#define SM_CAPABILITY_UDP               0x02
#define SM_CAPABILITY_ENCRYPTION        0x04
#define SM_CAPABILITY_HAVE_KEY          0x08
#define SM_CAPABILITY_COMPRESSSION      0x10
#define SM_CAPABILITY_PACK              0x20
#define SM_CAPABILITY_BATTERY           0x40

/* Message definitions */
enum sm_capabilities {
    field_define(sm_capabilities, opcode, uint8_t),
    field_define(sm_capabilities, flags, uint8_t),
    record_end(sm_capabilities)
};

#if (defined CONNECTOR_SM_ENCRYPTION)
enum sm_key_set {
    field_define(sm_key_set, opcode, uint8_t),
    field_define_array(sm_key_set, key, SM_KEY_LENGTH),
    record_end(sm_key_set)
};

enum sm_key_response {
    field_define(sm_key_response, opcode, uint8_t),
    field_define_array(sm_key_response, tag, SM_TAG_LENGTH),
    record_end(sm_key_response)
};

enum sm_key_error {
    field_define(sm_key_error, opcode, uint8_t),
    record_end(sm_key_error)
};

typedef struct sm_configuration_response
{
    uint8_t tag[SM_KEY_LENGTH];
    char const * error;
} sm_configuration_response_t;

STATIC connector_status_t sm_configuration_service_run_cb(connector_data_t * const connector_ptr, connector_request_id_sm_t const request, void * const arg)
{
    connector_status_t result = connector_invalid_data;
    connector_callback_status_t callback_status;
    connector_request_id_t request_id;

    request_id.sm_request = request;
    callback_status = connector_callback(connector_ptr->callback, connector_class_id_short_message, request_id, arg, connector_ptr->context);
    switch (callback_status)
    {
        case connector_callback_continue:
            result = connector_working;
            break;

        case connector_callback_busy:
        case connector_callback_unrecognized:
        case connector_callback_abort:
        case connector_callback_error:
            result = connector_invalid_data;
            break;
    }

    return result;
}

STATIC void sm_generate_iv(connector_data_t * const connector_ptr, uint8_t * const iv, size_t const length, connector_transport_t const transport, uint8_t const type, uint8_t const pool, uint16_t const request_id);
STATIC connector_bool_t sm_encryption_set_key(connector_data_t * connector_ptr, uint8_t const * const key, size_t const length);
STATIC connector_bool_t sm_write_tracking_data(connector_data_t * const connector_ptr, connector_transport_t const transport);

static connector_bool_t sm_encryption_get_key_tag(connector_data_t * const connector_ptr, uint8_t * const tag, size_t const tag_length)
{
    connector_status_t status = connector_abort;
    connector_sm_encryption_data_t * const keyring = &connector_ptr->sm_encryption;
    uint16_t const request_id = 0;
    uint8_t iv[SM_IV_LENGTH];
    uint8_t input;
    uint8_t output;

    connector_sm_encrypt_gcm_t encrypt;

    /* The input and output pointers should not overlap, even if not used */
    encrypt.message.length = 0;
    encrypt.message.input = &input;
    encrypt.message.output = &output;

    encrypt.tag.length = tag_length;
    encrypt.tag.data = tag;

    encrypt.aad.length = sizeof connector_ptr->device_id;
    encrypt.aad.data = connector_ptr->device_id;

    encrypt.iv.length = sizeof iv;
    encrypt.iv.data = iv;
    sm_generate_iv(connector_ptr, iv, sizeof iv, connector_transport_tcp, SM_IV_TYPE_REQUEST, SM_IV_POOL_DEVICE, request_id);

    encrypt.key.length = sizeof keyring->current.key;
    encrypt.key.data = keyring->current.key;

    status = sm_configuration_service_run_cb(connector_ptr, connector_request_id_sm_encrypt_gcm, &encrypt);
    return (status == connector_working);
}
#endif

STATIC connector_status_t sm_configuration_service_request_callback(connector_data_t * const connector_ptr, msg_service_request_t * const service_request)
{
#if (defined CONNECTOR_SM_ENCRYPTION)

    msg_service_data_t * const service_data = service_request->have_data;
    msg_session_t * const msg_session = service_request->session;

    ASSERT(msg_session->service_context == NULL);
    if (MsgIsStart(service_data->flags))
    {
        uint8_t const opcode = *((uint8_t *) service_data->data_ptr);

        switch (opcode)
        {
            case SM_CONFIG_OPCODE_KEY_SET:
            {
                static sm_configuration_response_t response;
                uint8_t key[SM_KEY_LENGTH];

                /* grab key from message */
                {
                    uint8_t const * const sm_key_set = service_data->data_ptr;

                    message_load_array(key, sizeof key, sm_key_set, key);
                }

                if (!sm_encryption_set_key(connector_ptr, key, sizeof key))
                {
                    response.error = "unable to store key";
                }
#if (defined CONNECTOR_TRANSPORT_UDP)
                else if (!sm_write_tracking_data(connector_ptr, connector_transport_udp))
                {
                    response.error = "unable to write udp tracking data";
                }
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
                else if (!sm_write_tracking_data(connector_ptr, connector_transport_sms))
                {
                    response.error = "unable to write sms tracking data";
                }
#endif
                else if (!sm_encryption_get_key_tag(connector_ptr, response.tag, sizeof response.tag))
                {
                    response.error = "unable to generate tag";
                }
                else
                {
                    response.error = NULL;
                }

                msg_session->service_context = &response;
                return connector_working;
            }

            default:
                msg_set_error(msg_session, connector_session_error_invalid_opcode);
                return connector_working;
                break;
        }
    }
#else
    UNUSED_PARAMETER(connector_ptr);
    UNUSED_PARAMETER(service_request);
#endif
    return connector_abort;
}

STATIC connector_status_t sm_configuration_service_response_callback(connector_data_t * const connector_ptr, msg_service_request_t * const service_request)
{
#if (defined CONNECTOR_SM_ENCRYPTION)
    msg_service_data_t * const service_data = service_request->need_data;
    msg_session_t * const msg_session = service_request->session;
    sm_configuration_response_t * const response = msg_session->service_context;

    UNUSED_PARAMETER(connector_ptr);

    if (response != NULL)
    {
        MsgSetStart(service_data->flags);
        MsgSetLastData(service_data->flags);

        if (response->error == NULL)
        {
            uint8_t * const sm_key_response = service_data->data_ptr;

            message_store_u8(sm_key_response, opcode, SM_CONFIG_OPCODE_KEY_RESPONSE);
            message_store_array(sm_key_response, tag, response->tag, sizeof response->tag);
            service_data->length_in_bytes = record_bytes(sm_key_response);
        }
        else
        {
            uint8_t * const sm_key_error = service_data->data_ptr;
            size_t const length = strlen(response->error);

            message_store_u8(sm_key_error, opcode, SM_CONFIG_OPCODE_KEY_ERROR);
            service_data->length_in_bytes = record_bytes(sm_key_error);
            memcpy(GET_PACKET_DATA_POINTER(sm_key_error, service_data->length_in_bytes), response->error, length);
            service_data->length_in_bytes += length;
        }
    }

    return connector_working;
#else
    UNUSED_PARAMETER(connector_ptr);
    UNUSED_PARAMETER(service_request);

    return connector_abort;
#endif
}

STATIC connector_status_t sm_configuration_service_callback(connector_data_t * const connector_ptr, msg_service_request_t * const service_request)
{
    connector_status_t status = connector_abort;

    ASSERT_GOTO(connector_ptr != NULL, done);
    ASSERT_GOTO(service_request != NULL, done);
    ASSERT_GOTO((service_request->service_type == msg_service_type_capabilities) || (service_request->session != NULL), done);

    switch (service_request->service_type)
    {
        case msg_service_type_have_data:
            status = sm_configuration_service_request_callback(connector_ptr, service_request);
            break;

        case msg_service_type_need_data:
            status = sm_configuration_service_response_callback(connector_ptr, service_request);
            break;

        case msg_service_type_error:
        {
            connector_debug_line("sm_configuration_service_callback error: %d", service_request->error_value);
            status = connector_working;
            break;
        }

        case msg_service_type_free:
            status = connector_working;
            break;

        case msg_service_type_capabilities:
        {
            msg_service_data_t * const service_data = service_request->need_data;
            connector_bool_t have_key = connector_false;
#if (defined CONNECTOR_SM_ENCRYPTION)
            connector_sm_encryption_data_t * const encryption_data = &connector_ptr->sm_encryption;
            uint8_t tag[SM_TAG_LENGTH];
            size_t const tag_length = sizeof tag;

            /* get auth tag */
            if (encryption_data->current.valid)
            {
                if (!sm_encryption_get_key_tag(connector_ptr, tag, tag_length))
                {
                    encryption_data->current.valid = connector_false;
                }
            }
            have_key = encryption_data->current.valid;
#endif

            ASSERT(service_data != NULL);

            /* generate response */
            {
                uint8_t * const sm_capabilities = service_data->data_ptr;
                uint8_t const flags =
#if (defined CONNECTOR_TRANSPORT_SMS)
                    SM_CAPABILITY_SMS |
#endif
#if (defined CONNECTOR_TRANSPORT_UDP)
                    SM_CAPABILITY_UDP |
#endif
#if (defined CONNECTOR_SM_ENCRYPTION)
                    SM_CAPABILITY_ENCRYPTION |
#endif
#if (defined CONNECTOR_COMPRESSION)
                    SM_CAPABILITY_COMPRESSSION |
#endif
                    /* always on */
                    SM_CAPABILITY_PACK |
                    /* SM_CAPABILITY_BATTERY | */
                    (have_key ? SM_CAPABILITY_HAVE_KEY : 0);

                ASSERT(service_data->data_ptr != NULL);
                message_store_u8(sm_capabilities, opcode, SM_CONFIG_OPCODE_CAPABILITIES);
                message_store_u8(sm_capabilities, flags, flags);
                service_data->length_in_bytes = record_bytes(sm_capabilities);
#if (defined CONNECTOR_SM_ENCRYPTION)
                if (have_key)
                {
                    memcpy(GET_PACKET_DATA_POINTER(sm_capabilities, service_data->length_in_bytes), tag, tag_length);
                    service_data->length_in_bytes += tag_length;
                }
#endif
            }
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

STATIC connector_status_t connector_facility_sm_configuration_service_init(connector_data_t * const data_ptr, unsigned int const facility_index)
{
    return msg_init_facility(data_ptr, facility_index, msg_service_id_sm, sm_configuration_service_callback);
}

STATIC connector_status_t connector_facility_sm_configuration_service_delete(connector_data_t * const data_ptr)
{
    return msg_delete_facility(data_ptr, msg_service_id_sm);
}
#endif
