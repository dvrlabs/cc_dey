/*
 * Copyright (c) 2014 Digi International Inc.
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
#if (defined CONNECTOR_TRANSPORT_SMS)
STATIC connector_status_t sm_decode_segment(connector_data_t * const connector_ptr, connector_sm_packet_t * const recv_ptr)
{
    size_t const data_size = 1 + ((recv_ptr->total_bytes - recv_ptr->processed_bytes) * 4)/5;
    void * data_ptr = NULL;
    connector_status_t result = malloc_data_buffer(connector_ptr, data_size, named_buffer_id(sm_data_block), &data_ptr);

    if (result == connector_working)
    {
        connector_debug_print_buffer("encoded", recv_ptr->data, recv_ptr->total_bytes);
        connector_debug_line("recv_ptr: processed_bytes=%zu, total_bytes=%zu", recv_ptr->processed_bytes, recv_ptr->total_bytes);

        recv_ptr->total_bytes = sm_decode85(data_ptr, data_size, recv_ptr->data + recv_ptr->processed_bytes, recv_ptr->total_bytes - recv_ptr->processed_bytes);

        connector_debug_print_buffer("decoded", data_ptr, data_size);

        memcpy(recv_ptr->data, data_ptr, recv_ptr->total_bytes);
        recv_ptr->processed_bytes = 0;

        connector_debug_print_buffer("moved", recv_ptr->data, recv_ptr->total_bytes);
        connector_debug_line("recv_ptr: processed_bytes=%zu, total_bytes=%zu", recv_ptr->processed_bytes, recv_ptr->total_bytes);

        result = free_data_buffer(connector_ptr, named_buffer_id(sm_data_block), data_ptr);
    }

    return result;
}

STATIC connector_status_t sm_verify_sms_preamble(connector_sm_data_t * const sm_ptr)
{
    connector_status_t result = connector_working;

    if (sm_ptr->transport.id_length > 0)
    {
        char const * const data_ptr = (char *) sm_ptr->network.recv_packet.data;
        #define PREFIX "("
        #define SUFFIX "):"
        #define VALID(func, ptr, var)    connector_bool(func((ptr) + (var).offset, (var).data, (var).length) == 0)
        struct cmp_tuple {
            char const * data;
            size_t length;
            size_t offset;
        }
        prefix = { PREFIX, sizeof PREFIX - 1, 0},
        shared_key = { (char *) sm_ptr->transport.id, sm_ptr->transport.id_length, prefix.offset + prefix.length },
        suffix = { SUFFIX, sizeof SUFFIX - 1, shared_key.offset + shared_key.length };
        connector_bool_t const valid_prefix = VALID(strncmp, data_ptr, prefix);
        connector_bool_t const valid_shared_key = VALID(strncasecmp, data_ptr, shared_key);
        connector_bool_t const valid_suffix = VALID(strncmp, data_ptr, suffix);
        #undef VALID
        #undef SUFFIX
        #undef PREFIX

        if (valid_prefix && valid_shared_key && valid_suffix)
        {
            sm_ptr->network.recv_packet.processed_bytes = suffix.offset + suffix.length;
        }
        else
        {
            connector_debug_line("sm_verify_sms_preamble: valid_prefix=%d, valid_shared_key=%d, valid_suffix=%d", valid_prefix, valid_shared_key, valid_suffix);
            connector_debug_line("%s", data_ptr);

            result = connector_invalid_response;
        }
    }

    return result;
}
#endif

#if (defined CONNECTOR_TRANSPORT_UDP)
STATIC connector_status_t sm_verify_udp_header(connector_sm_data_t * const sm_ptr)
{
    connector_status_t result = connector_invalid_response;
    connector_sm_packet_t * const recv_ptr = &sm_ptr->network.recv_packet;
    uint8_t * data_ptr = recv_ptr->data;
    uint8_t const version_and_type = *data_ptr++;
    uint8_t const version = version_and_type >> 4;
    connector_sm_id_type_t const type = (connector_sm_id_type_t)(version_and_type & 0x0F);

    if (version != SM_UDP_VERSION)
    {
        connector_debug_line("sm_verify_udp_header: invalid SM UDP version [%d]", version);
        result = connector_abort;
        goto error;
    }

    if (type != sm_ptr->transport.id_type)
        goto done;

    if (memcmp(data_ptr, sm_ptr->transport.id, sm_ptr->transport.id_length))
        goto done; /* not for us */

    recv_ptr->processed_bytes = sm_ptr->transport.id_length + 1;
    result = connector_working;

done:
error:
    return result;
}
#endif

#if (defined CONNECTOR_SM_MULTIPART)
STATIC connector_status_t sm_multipart_allocate(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, connector_sm_session_t * const session)
{
    connector_status_t status = connector_working;
    size_t const max_payload_bytes = sm_get_max_payload_bytes(sm_ptr);
    size_t const max_session_bytes = (((max_payload_bytes * session->segments.count) + 3)/4) * 4; /* make sure it is word aligned */
    size_t const size_array_bytes = sizeof(*session->segments.size_array) * session->segments.count;

    ASSERT_GOTO(session->in.data == NULL, error);
    session->in.bytes = max_session_bytes + size_array_bytes;
    status = sm_allocate_user_buffer(connector_ptr, &session->in);
    ASSERT_GOTO(status == connector_working, error);
    session->segments.size_array = (void *)(session->in.data + max_session_bytes); /* alignment issue is taken care where max_session_bytes is defined */
    memset(session->segments.size_array, 0, size_array_bytes);

error:
    return status;
}
#endif

STATIC connector_status_t sm_more_data_callback(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr)
{
    connector_status_t result;
    connector_request_id_t request_id;
    connector_callback_status_t callback_status;
    connector_sm_more_data_t cb_data;

    cb_data.transport = sm_ptr->network.transport;
    request_id.sm_request = connector_request_id_sm_more_data;
    callback_status = connector_callback(connector_ptr->callback, connector_class_id_short_message, request_id, &cb_data, connector_ptr->context);
    result = sm_map_callback_status_to_connector_status(callback_status);

    return result;
}

typedef struct
{
    uint32_t request_id;
    size_t bytes;

    connector_bool_t isRequest;
    connector_bool_t isResponseNeeded;
    connector_bool_t isPackCmd;
    connector_bool_t isMultipart;
    connector_bool_t isCompressed;
    connector_bool_t isEncrypted;
    connector_bool_t hasNewKey;
    connector_bool_t isError;

    connector_sm_cmd_t command;

    enum
    {
        sm_single_segment,
        sm_multipart_first_segment,
        sm_multipart_subsequent_segment
    } type;

    struct
    {
        uint8_t count;
        uint8_t number;
    } segment;

    uint16_t crc16;
    uint8_t info;
    uint8_t cmd_status;
} sm_header_t;

STATIC connector_status_t sm_process_header(connector_sm_packet_t * const recv_ptr, sm_header_t * const header)
{
    connector_status_t result = connector_invalid_payload_packet;
    uint8_t * data_ptr = &recv_ptr->data[recv_ptr->processed_bytes];
    uint8_t * const segment = data_ptr;

    header->segment.count = 1;
    header->segment.number = 0;
    header->bytes = 0;
    header->command = connector_sm_cmd_opaque_response;
    header->isError = connector_false;
    header->isCompressed = connector_false;
    header->isEncrypted = connector_false;
    header->hasNewKey = connector_false;

    {
        uint8_t const info_field = message_load_u8(segment, info);
        uint8_t const request_id_high_bits_mask = 0x03;

        header->request_id = (info_field & request_id_high_bits_mask) << 8;
        header->request_id |= message_load_u8(segment, request);

        header->isMultipart = SmIsMultiPart(info_field);
        header->isRequest = SmIsRequest(info_field);
        header->isResponseNeeded = SmIsResponseNeeded(info_field);

        header->info = info_field;
    }

    if (header->isMultipart)
    {
        header->cmd_status = 0;

        #if (defined CONNECTOR_SM_MULTIPART)
        {
            uint8_t * const segment0 = data_ptr;

            header->segment.number = message_load_u8(segment0, segment);
            if (header->segment.number > 0)
            {
                uint8_t * const segmentn = data_ptr;

                header->cmd_status = 0;
                header->type = sm_multipart_subsequent_segment;

                if (header->isPackCmd)
                {
                    header->bytes = record_end(segmentn) - sizeof header->crc16;
                }
                else
                {
                    header->crc16 = message_load_be16(segmentn, crc);
                    message_store_be16(segmentn, crc, 0);
                    header->bytes = record_end(segmentn);
                }
            }
            else
            {
                header->type = sm_multipart_first_segment;
                header->segment.count = message_load_u8(segment0, count);
                header->cmd_status = message_load_u8(segment0, cmd_status);

                if (header->isPackCmd)
                {
                    header->bytes = record_end(segment0) - sizeof header->crc16;
                }
                else
                {
                    header->crc16 = message_load_be16(segment0, crc);
                    message_store_be16(segment0, crc, 0);
                    header->bytes = record_end(segment0);
                }
            }
        }
        #else
        {
            connector_debug_line("Received multipart packet, but multipart is disabled");
            connector_debug_line("Review CONNECTOR_SM_MULTIPART and CONNECTOR_SM_MAX_RX_SEGMENTS defines");
            goto error;
        }
        #endif
    }
    else
    {
        header->cmd_status = message_load_u8(segment, cmd_status);
        if (header->isPackCmd)
        {
            header->bytes = record_end(segment) - sizeof header->crc16;
        }
        else
        {
            header->crc16 = message_load_be16(segment, crc);
            message_store_be16(segment, crc, 0);
            header->bytes = record_end(segment);
        }
    }

    header->isCompressed = SmIsCompressed(header->cmd_status);
    #if !(defined CONNECTOR_COMPRESSION)
    if (header->isCompressed)
    {
        connector_debug_line("sm_process_header: Received compressed packet, but compression is disabled");
        goto error;
    }
    #endif

    #if (defined CONNECTOR_SM_MULTIPART)
    if (!header->isMultipart || header->segment.number == 0)
    #endif
    {
        header->isEncrypted = SmIsEncrypted(header->cmd_status);
        header->hasNewKey = SmHasNewKey(header->cmd_status);
        #if (defined CONNECTOR_SM_ENCRYPTION)
        if (!header->isEncrypted)
        {
            connector_debug_line("sm_process_header: Received non-encrypted packet, but encryption is enabled");
            goto error;
        }
        #else
        if (header->isEncrypted)
        {
            connector_debug_line("sm_process_header: Received encrypted packet, but encryption is disabled");
            goto error;
        }
        if (header->hasNewKey)
        {
            connector_debug_line("sm_process_header: Received a packet with an encryption key, but encryption is disabled");
            goto error;
        }
        #endif
    }

    if (header->isRequest)
    {
        header->command = (connector_sm_cmd_t) (header->cmd_status & SM_COMMAND_MASK);
        if (header->isPackCmd && (header->command == connector_sm_cmd_pack))
        {
            connector_debug_line("sm_process_header: Pack command inside a pack command is not allowed");
            goto error;
        }
        header->isError = connector_false;
    }
    else
    {
        header->isError = SmIsError(header->cmd_status);
        header->command = connector_sm_cmd_opaque_response;
    }

    if (!header->isPackCmd)
    {
        size_t const sm_bytes = recv_ptr->total_bytes - recv_ptr->processed_bytes;
        uint16_t calculated_crc = 0;

        calculated_crc = sm_calculate_crc16(calculated_crc, data_ptr, sm_bytes);
        if(calculated_crc != header->crc16)
        {
            connector_debug_line("sm_process_header: crc error");
            goto error;
        }

        if (header->isRequest)
            header->isPackCmd = connector_bool(header->command == connector_sm_cmd_pack);
    }

    recv_ptr->processed_bytes += header->bytes;
    result = connector_working;

error:
    return result;
}

STATIC connector_status_t sm_update_session(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr,
                                            sm_header_t * const header, size_t const payload_bytes)
{
    connector_status_t result;
    connector_sm_packet_t * const recv_ptr = &sm_ptr->network.recv_packet;
    connector_bool_t const client_originated = connector_bool(!header->isRequest);
    connector_sm_session_t * session = get_sm_session(sm_ptr, header->request_id, client_originated);

    connector_debug_print_buffer("payload:", &recv_ptr->data[recv_ptr->processed_bytes], payload_bytes);

    if (session == NULL)
    {
        session = sm_create_session(connector_ptr, sm_ptr, connector_false);
        if (session == NULL)
        {
            result = connector_pending;
            goto error;
        }

        session->request_id = header->request_id;
        session->info = header->info;
        session->cmd_status = header->cmd_status;
        session->command = (client_originated == connector_true) ? connector_sm_cmd_opaque_response : header->command;
    }

    if (header->segment.number == 0)
    {
         /* Decryption is not aware of the fact that a message could be split into multiple segments and thus requires the info and cmd_status bytes are from segment0 */
         session->info = header->info;
         session->cmd_status = header->cmd_status;

        if (header->isCompressed) SmSetCompressed(session->flags);

        if (header->isRequest)
        {
            if (header->isResponseNeeded) SmSetResponseNeeded(session->flags);
            /* If the first segment of a multipart SMS that arrived was not segment0, previously stored session->command information is wrong as
             * for multipart messages, type is only included in the 0th segment. Update it here */
            session->command = header->command;
        }
        else if (header->isError)
        {
            uint16_t const error_value = LoadBE16(&recv_ptr->data[recv_ptr->processed_bytes]);

            switch (error_value)
            {
                case connector_sm_error_in_request:
                    session->error = connector_sm_error_in_request;
                    break;

                case connector_sm_error_unavailable:
                    session->error = connector_sm_error_unavailable;
                    break;

                default:
                    session->error = connector_sm_error_unknown;
                    break;
            }

            SmSetError(session->flags);
            recv_ptr->processed_bytes += sizeof error_value;
        }

        session->segments.count = header->segment.count;
    }

    #if (defined CONNECTOR_SM_MULTIPART)
    if (header->isMultipart)
    {
        SmSetMultiPart(session->flags);
        if (session->in.data == NULL)
        {
            if (header->segment.number > 0)
                session->segments.count = sm_ptr->session.max_segments;

            result = sm_multipart_allocate(connector_ptr, sm_ptr, session);
            ASSERT_GOTO(result == connector_working, error);
            ASSERT_GOTO(session->in.data != NULL, error);
        }

        if (session->segments.size_array[header->segment.number] == 0)
        {
            size_t const max_payload_bytes = sm_get_max_payload_bytes(sm_ptr);
            uint8_t * copy_to = session->in.data + (header->segment.number * max_payload_bytes);

            memcpy(copy_to, &recv_ptr->data[recv_ptr->processed_bytes], payload_bytes);
            session->segments.size_array[header->segment.number] = payload_bytes;
            session->segments.processed++;
        }
        else
        {
            connector_debug_line("sm_update_session: duplicate segment %d, in id %d", header->segment.number, session->request_id);
        }
    }
    else
    #endif
    {
        if (payload_bytes > 0)
        {
            session->in.bytes = payload_bytes;
            result = sm_allocate_user_buffer(connector_ptr, &session->in);
            ASSERT_GOTO(result == connector_working, error);

            memcpy(session->in.data, &recv_ptr->data[recv_ptr->processed_bytes], payload_bytes);
        }
        else
        {
            session->in.bytes = 0;
            session->in.data = NULL;
        }
        session->segments.processed++;
    }

    if (session->segments.processed >= session->segments.count)
    {
        session->bytes_processed = 0;
        session->segments.processed = 0;
        #if (defined CONNECTOR_SM_ENCRYPTION)
        session->sm_state = connector_sm_state_decrypt;
        #else
        session->sm_state = connector_sm_state_process_payload;
        #if (defined CONNECTOR_COMPRESSION)
        if (SmIsCompressed(session->flags))
        {
            session->compress.out.data = NULL;
            session->sm_state = connector_sm_state_decompress;
        }
        #endif
        #endif
    }

    recv_ptr->processed_bytes += payload_bytes;
    connector_debug_line("sm_update_session: recv_ptr total_bytes=%zu processed_bytes=%zu", recv_ptr->total_bytes, recv_ptr->processed_bytes);

    result = connector_working;

error:
    return result;
}

STATIC connector_status_t sm_process_packet(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr)
{
    sm_header_t sm_header;
    connector_status_t result;
    connector_sm_packet_t * const recv_ptr = &sm_ptr->network.recv_packet;
    size_t sm_bytes = recv_ptr->total_bytes - recv_ptr->processed_bytes;

    sm_header.isPackCmd = connector_false;
    sm_header.crc16 = 0;
    do
    {
        connector_debug_line("Calling process_header");
        result = sm_process_header(recv_ptr, &sm_header);
        if (result != connector_working) goto error;

        if ((sm_header.segment.count > sm_ptr->session.max_segments) || (sm_header.segment.number >= sm_ptr->session.max_segments))
        {
            connector_debug_line("sm_process_packet: Exceeded maximum segments [%" PRIsize "/%" PRIsize "]", sm_header.segment.count, sm_ptr->session.max_segments);
            connector_debug_line("Review CONNECTOR_SM_MULTIPART and CONNECTOR_SM_MAX_RX_SEGMENTS defines");
            goto error;
        }

        if (sm_header.command == connector_sm_cmd_pack)
        {
            enum sm_pack_t
            {
                field_define(pack_header, flag, uint8_t),
                field_define(pack_header, length, uint16_t),
                record_end(pack_header)
            };
            uint8_t * const pack_header = &recv_ptr->data[recv_ptr->processed_bytes];
            uint8_t const flag = message_load_u8(pack_header, flag);

            #define SM_MESSAGE_PENDING 0x01
            if ((flag & SM_MESSAGE_PENDING) == SM_MESSAGE_PENDING)
            {
                result = sm_more_data_callback(connector_ptr, sm_ptr);
                if (result != connector_working) goto error;
            }
            #undef SM_MESSAGE_PENDING

            sm_bytes = message_load_be16(pack_header, length);
            recv_ptr->processed_bytes += record_end(pack_header);
            continue;
        }

        {
            size_t const payload_bytes = sm_bytes - sm_header.bytes;

            ASSERT(sm_bytes >= sm_header.bytes);
            result = sm_update_session(connector_ptr, sm_ptr, &sm_header, payload_bytes);
            connector_debug_line("sm_update_session result=%zu", result);

            if (result != connector_working) goto error;
        }

        if (!sm_header.isPackCmd) break;

        {
            size_t const sm_header_size = 5;
            size_t const remaining_bytes = recv_ptr->total_bytes - recv_ptr->processed_bytes;

            connector_debug_line("remaining_bytes=%zu", remaining_bytes);
            if (remaining_bytes < sm_header_size) break;
        }

        sm_bytes = LoadBE16(&recv_ptr->data[recv_ptr->processed_bytes]);
        recv_ptr->processed_bytes += sizeof(uint16_t);

    } while (recv_ptr->processed_bytes < recv_ptr->total_bytes);

    result = connector_working;

error:
    if (result == connector_invalid_payload_packet)  /* unreliable method, so silently ignore the packet */
        result = connector_working;

    recv_ptr->total_bytes = 0;
    recv_ptr->processed_bytes = 0;

    return result;
}

STATIC connector_status_t sm_receive_data(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr)
{
    connector_status_t result = connector_pending;
    connector_sm_packet_t * const recv_ptr = &sm_ptr->network.recv_packet;
    connector_callback_status_t status;
    connector_request_id_t request_id;
    connector_network_receive_t read_data;

    if (recv_ptr->total_bytes > 0) goto done;

    read_data.handle = sm_ptr->network.handle;
    read_data.buffer = recv_ptr->data;
    read_data.bytes_available = sm_ptr->transport.mtu;
    read_data.bytes_used = 0;

    request_id.network_request = connector_request_id_network_receive;
    status = connector_callback(connector_ptr->callback, sm_ptr->network.class_id, request_id, &read_data, connector_ptr->context);
    ASSERT(status != connector_callback_unrecognized);
    switch (status)
    {
        case connector_callback_busy:
            result = connector_idle;
            goto done;

        case connector_callback_continue:
            recv_ptr->total_bytes = read_data.bytes_used;
            recv_ptr->processed_bytes = 0;

            connector_debug_print_buffer("raw", read_data.buffer, read_data.bytes_used);

            switch (sm_ptr->network.transport)
            {
                #if (defined CONNECTOR_TRANSPORT_SMS)
                case connector_transport_sms:
                    result = sm_verify_sms_preamble(sm_ptr);
                    switch(result)
                    {
                        case connector_working:
                            break;
                        case connector_invalid_response:
                            /* not Device Cloud packet? Ignore the packet */
                            recv_ptr->total_bytes = 0;
                            recv_ptr->processed_bytes = 0;
                            result = connector_working;
                            goto done;
                        default:
                            goto done;
                    }

                    result = sm_decode_segment(connector_ptr, recv_ptr);

                    /* Remove sms preamble */
                    sm_ptr->network.recv_packet.processed_bytes = 0;

                    break;
                #endif

                #if (defined CONNECTOR_TRANSPORT_UDP)
                case connector_transport_udp:
                    result = sm_verify_udp_header(sm_ptr);
                    break;
                #endif

                default:
                    ASSERT_GOTO(connector_false, done);
                    break;
            }

            if (result != connector_working) goto done; /* not Device Cloud packet? */
            result = sm_process_packet(connector_ptr, sm_ptr);
            break;

        case connector_callback_abort:
            result = connector_abort;
            break;

        default:
            connector_debug_line("sm_receive_data: callback returned error [%d]", status);
            result = connector_device_error;
            break;
    }


    connector_debug_line("pre sm_verify_result result=%zu", result);
    sm_verify_result(sm_ptr, &result);
    connector_debug_line("post sm_verify_result result=%zu", result);

done:
    return result;
}

#if (defined CONNECTOR_SM_ENCRYPTION)
STATIC connector_status_t sm_decrypt_data(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, connector_sm_session_t * const session)
{
    connector_status_t status;
    size_t const max_payload_bytes = sm_get_max_payload_bytes(sm_ptr);
    size_t const bytes_in = session->in.bytes;
    size_t bytes_out = bytes_in - SM_TAG_LENGTH;
    uint8_t * payload;

    sm_data_block_t input = { 0 };
    sm_data_block_t output = { .bytes = bytes_out };

    size_t session_length = 0;
    if SmIsMultiPart(session->flags)
    {
        for (size_t segment = 0; segment < session->segments.count; segment++)
        {
            session_length += session->segments.size_array[segment];
        }
        input.bytes = session_length;
    }

    connector_debug_line("sm_decrypt_data: session_length=%zu", session_length);
    connector_debug_line("sm_decrypt_data: session->bytes_processed=%zu", session->bytes_processed);
    connector_debug_line("sm_decrypt_data: session->in.data=%p, session->in.bytes=%zu", session->in.data, session->in.bytes);
    connector_debug_line("sm_decrypt_data: bytes_in=%zu, bytes_out=%zu", bytes_in, bytes_out);

    status = sm_allocate_user_buffer(connector_ptr, &output);
    ASSERT_GOTO(status == connector_working, error);
    payload = output.data;

    /*
        TODO:
        Since we don't know the size of each segment until it is received,
        the session->in.data object is organized as an array of buffers that
        are max_payload_bytes each. If the following coalescing step was done
        once before either the decryption (or if no encryption, the
        decompression) phase it would really make the subsequent processing
        much easier.
    */
    if SmIsMultiPart(session->flags)
    {
        uint8_t const * src;
        uint8_t * dst;
        status = sm_allocate_user_buffer(connector_ptr, &input);
        ASSERT_GOTO(status == connector_working, error);

        dst = input.data;
        src = session->in.data;
        for (size_t segment = 0; segment < session->segments.count; segment++)
        {
            size_t const length = session->segments.size_array[segment];

            memcpy(dst, src, length);

            dst += length;
            src += max_payload_bytes;
        }
        bytes_out = dst - input.data - SM_TAG_LENGTH;
    }
    else
    {
        input.data = session->in.data;
    }

    {
        connector_transport_t transport = sm_ptr->network.transport;
        uint16_t request_id = session->request_id;
        uint8_t const info_byte = session->info;
        uint8_t const cs_byte = session->cmd_status;
        size_t const length = bytes_out;
        uint8_t const * const ciphertext = input.data;
        uint8_t const * const tag = (ciphertext + length);
        uint8_t * const plaintext = payload;

        if (!sm_decrypt_block(connector_ptr, transport, request_id, info_byte, cs_byte, ciphertext, length, tag, SM_TAG_LENGTH, plaintext, length))
        {
            /* TODO: We should really differentiate between a failure to decrypt due to invalid encryption data versus because of OS errors */
            session->sm_state = connector_sm_state_complete; /* discard silently */
            goto done;
        }

        if (SmHasNewKey(session->cmd_status))
        {
            uint8_t const * const key = payload;

            if (!sm_encryption_set_key(connector_ptr, key, SM_KEY_LENGTH))
            {
                return connector_abort;
            }

            bytes_out -= SM_KEY_LENGTH;
            payload += SM_KEY_LENGTH;
        }
    }

    if SmIsMultiPart(session->flags)
    {
        uint8_t const * src = payload;
        uint8_t * dst = session->in.data;
        size_t remaining = bytes_out;
        size_t new_segment_count = session->segments.count;

        for (size_t segment = 0; segment < session->segments.count; segment++)
        {
            size_t const length = session->segments.size_array[segment];
            size_t const copy = (length <= remaining) ? length : remaining;

            memcpy(dst, src, copy);
            if (copy != length)
            {
                session->segments.size_array[segment] = copy;
                if (copy == 0)
                {
                    new_segment_count--;
                }
            }

            dst += max_payload_bytes;
            src += copy;
            remaining -= copy;
        }
        connector_debug_line("sm_decrypt_data: session->segments.count=%zu, new_segment_count=%zu", session->segments.count, new_segment_count);
    }
    else
    {
        memcpy(session->in.data, output.data, bytes_out);
    }
    session->in.bytes = bytes_out;

    session->sm_state = connector_sm_state_process_payload;
    #if (defined CONNECTOR_COMPRESSION)
    if (SmIsCompressed(session->flags))
    {
        session->compress.out.data = NULL;
        session->sm_state = connector_sm_state_decompress;
    }
    #endif

done:
    if SmIsMultiPart(session->flags)
    {
        status = free_data_buffer(connector_ptr, named_buffer_id(sm_data_block), input.data);
        ASSERT(status == connector_working);
    }
    status = free_data_buffer(connector_ptr, named_buffer_id(sm_data_block), output.data);
    ASSERT(status == connector_working);

error:
    return status;
}
#endif

#if (defined CONNECTOR_COMPRESSION)
STATIC connector_status_t sm_decompress_data(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, connector_sm_session_t * const session)
{
    connector_status_t status;
    size_t const max_payload_bytes = sm_get_max_payload_bytes(sm_ptr);
    uint8_t zlib_header[] = {0x58, 0xC3};
    z_streamp const zlib_ptr = &session->compress.zlib;
    int zret;

    if (session->compress.out.data == NULL)
    {
        session->compress.out.bytes = max_payload_bytes;
        status = sm_allocate_user_buffer(connector_ptr, &session->compress.out);
        ASSERT_GOTO(status == connector_working, done);

        memset(zlib_ptr, 0, sizeof *zlib_ptr);
        zret = inflateInit(zlib_ptr);
        ASSERT_GOTO(zret == Z_OK, error);
        zlib_ptr->next_out = session->compress.out.data;
        zlib_ptr->avail_out = session->compress.out.bytes;
        zlib_ptr->next_in = zlib_header;
        zlib_ptr->avail_in = sizeof zlib_header;
    }

    while (zlib_ptr->avail_out > 0)
    {
        if (zlib_ptr->avail_in == 0)
        {
            if (session->segments.processed == session->segments.count)
            {
                SmSetLastData(session->flags);
                break;
            }
            else
            {
                size_t const data_index = session->segments.processed * max_payload_bytes;

                zlib_ptr->next_in = &session->in.data[data_index];
                zlib_ptr->avail_in = SmIsMultiPart(session->flags) ? session->segments.size_array[session->segments.processed] : session->in.bytes;
                session->segments.processed++;
            }
        }

        zret = inflate(zlib_ptr, Z_NO_FLUSH);
        switch(zret)
        {
            case Z_STREAM_END:
            case Z_BUF_ERROR:
            case Z_OK:
                break;

            default:
                status = connector_abort;
                connector_debug_line("ZLIB Return value [%d]", zret);
                ASSERT_GOTO(connector_false, error);
                break;
        }
    }

    {
        size_t const payload_bytes = session->compress.out.bytes - zlib_ptr->avail_out;

        status = sm_pass_user_data(connector_ptr, session, session->compress.out.data, payload_bytes);
        switch (status)
        {
            case connector_pending:
                goto done;

            case connector_working:
                if (SmIsNotLastData(session->flags))
                {
                    zlib_ptr->next_out = session->compress.out.data;
                    zlib_ptr->avail_out = session->compress.out.bytes;
                    goto done;
                }
                break;

            default:
                break;
        }
    }

error:
    zret = inflateEnd(zlib_ptr);
    if (zret != Z_OK)
    {
        status = connector_abort;
        goto done;
    }
    ASSERT_GOTO(zret == Z_OK, done);

    if (status != connector_abort)
    {
        status = free_data_buffer(connector_ptr, named_buffer_id(sm_data_block), session->compress.out.data);
        session->compress.out.data = NULL;
    }

done:
    return status;
}
#endif

STATIC connector_status_t sm_handle_error(connector_data_t * const connector_ptr, connector_sm_session_t * const session)
{
    connector_status_t result;
    connector_sm_state_t next_state = connector_sm_state_complete;

    if (SmIsCloudOwned(session->flags) && SmIsRequest(session->flags))
    {
        #if (defined CONNECTOR_SM_ENCRYPTION)
        next_state = connector_sm_state_encrypt;
        #else
        next_state = connector_sm_state_send_data;
        #endif
    }

    SmSetError(session->flags);
    if (session->user.context != NULL) /* let the user know */
    {
        result = sm_inform_session_complete(connector_ptr, session);
        if (result != connector_working)
        {
            goto error;
        }
    }

    result = sm_switch_path(connector_ptr, session, next_state);

error:
    return result;
}

STATIC connector_status_t sm_handle_complete(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, connector_sm_session_t * const session)
{
    connector_status_t result = connector_working;

    if (SmIsReboot(session->flags))
        result = sm_process_reboot(connector_ptr);

    switch(result)
    {
        case connector_abort:
            /* Keep connector_abort as the result */
            sm_delete_session(connector_ptr, sm_ptr, session);
            break;
        case connector_pending:
            break;
        default:
            result = sm_delete_session(connector_ptr, sm_ptr, session);
    }

    return result;
}

STATIC connector_status_t sm_process_recv_path(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, connector_sm_session_t * const session)
{
    connector_status_t result = connector_abort;

    connector_debug_line("sm_process_recv_path: in session->sm_state=%s", sm_state_to_string(session->sm_state));
    ASSERT_GOTO(session != NULL, error);
    switch (session->sm_state)
    {
        case connector_sm_state_receive_data:
            if (session->timeout_in_seconds != SM_WAIT_FOREVER)
            {
                unsigned long current_time = 0;

                result = get_system_time(connector_ptr, &current_time);
                ASSERT_GOTO(result == connector_working, error);
                if (current_time > (session->start_time + session->timeout_in_seconds))
                {
                    session->sm_state = connector_sm_state_error;
                    session->error = connector_sm_error_timeout;
                    connector_debug_line("Sm session [%u] timeout... start time:%u, current time:%u", session->request_id, session->start_time, current_time);
                }
            }

            result = connector_idle; /* still receiving data, handled in sm_receive_data() */
            break;

        #if (defined CONNECTOR_SM_ENCRYPTION)
        case connector_sm_state_decrypt:
            result = sm_decrypt_data(connector_ptr, sm_ptr, session);
            break;
        #endif

        #if (defined CONNECTOR_COMPRESSION)
        case connector_sm_state_decompress:
            result = sm_decompress_data(connector_ptr, sm_ptr, session);
            break;
        #endif

        case connector_sm_state_process_payload:
            result = sm_process_payload(connector_ptr, sm_ptr, session);
            break;

        case connector_sm_state_complete:
            result = sm_handle_complete(connector_ptr, sm_ptr, session);
            break;

        case connector_sm_state_error:
            result = sm_handle_error(connector_ptr, session);
            break;

        default:
            ASSERT(connector_false);
            break;
    }

    sm_verify_result(sm_ptr, &result);

error:
    connector_debug_line("sm_process_recv_path: out session->sm_state=%s", sm_state_to_string(session->sm_state));
    return result;
}
