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

static uint16_t const crc_table[] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

#define SmCRC16(crcval, newchar) crcval = ((crcval) >> 8) ^ crc_table[((crcval) ^ (newchar)) & 0x00ff]

STATIC uint16_t sm_calculate_crc16(uint16_t crc, uint8_t const * const data, size_t const bytes)
{
    size_t i;

    for (i = 0; i < bytes; i++)
        SmCRC16(crc, data[i]);

    return crc;
}

#if (defined CONNECTOR_TRANSPORT_SMS)
/* Base85 encoding lookup table. */
static uint8_t const encode85_table[] =
{
    '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*',
    '+', ',', '-', '.', '/',  '0', '1',  '2', '3', '4',
    '5', '6', '7', '8', '9',  ':', ';',  '<', '=', '>',
    '?', '@', 'A', 'B', 'C',  'D', 'E',  'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M',  'N', 'O',  'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W',  'X', 'Y',  'Z', '_', 'a',
    'b', 'c', 'd', 'e', 'f',  'g', 'h',  'i', 'j', 'k',
    'l', 'm', 'n', 'o', 'p',  'q', 'r',  's', 't', 'u',
    'v', 'w', 'x', 'y', 'z'
};

/* Base85 decoding lookup table. */
static uint8_t const decode85_table[] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0,  0,  0,  0,  58, 0,  59, 60, 61,
    62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
    82, 83, 84, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};


static unsigned long const pow85[] =
{
    85*85*85*85, 85*85*85, 85*85, 85, 1
};

STATIC int sm_encode85(uint8_t * dest, size_t dest_len, uint8_t const * const src, size_t const src_len)
{
    uint8_t buf[5];
    uint32_t tuple = 0;
    size_t src_count = 0;
    size_t dest_count = 0;
    size_t count = 0;
    size_t i;
    unsigned char *s;

    while (src_count < src_len)
    {
        unsigned const c = (*(src + src_count)) & 0xFF;

        switch (count++) {
        case 0:
            tuple |= (c << 24);
            break;
        case 1:
            tuple |= (c << 16);
            break;
        case 2:
            tuple |= (c <<  8);
            break;
        case 3:
            tuple |= c;
            i = 5;
            s = buf;
            do {
                *s++ = tuple % 85;
                tuple /= 85;
            } while (--i > 0);
            i = count;
            do {
                *dest++ = encode85_table[*--s];
                dest_count++;
                if (dest_count >= dest_len) {
                    return dest_count;
                }
            } while (i-- > 0);

            tuple = 0;
            count = 0;
            break;
        }
        src_count++;
    }

    /* Cleanup any remaining bytes... */
    if (count > 0) {
        i = 5;
        s = buf;
        do {
            *s++ = tuple % 85;
            tuple /= 85;
        } while (--i > 0);
        i = count;
        do {
            *dest++ = encode85_table[*--s];
            dest_count++;
            if (dest_count >= dest_len) {
                return dest_count;
            }
        } while (i-- > 0);
    }

    return dest_count;
}

STATIC int sm_decode85(uint8_t * dest, size_t dest_len, uint8_t const * const src, size_t const src_len)
{
    unsigned long tuple = 0;
    int c;
    size_t count = 0;
    size_t src_count = 0;
    size_t dest_count = 0;

    ASSERT_GOTO(dest_len >= (src_len * 4)/5, error);
    while (src_count < src_len) {
        c = (*(src + src_count)) & 0xFF;
        tuple += decode85_table[c] * pow85[count++];
        if (count == 5) {
            *dest++ = tuple >> 24;
            *dest++ = tuple >> 16;
            *dest++ = tuple >>  8;
            *dest++ = tuple;
            dest_count += 4;
            count = 0;
            tuple = 0;
        }
        src_count++;
    }

    /* Cleanup any remaining bytes... */
    if (count > 0) {
        count--;
        tuple += pow85[count];

        switch (count) {
        case 4:
            *dest++ = tuple >> 24;
            *dest++ = tuple >> 16;
            *dest++ = tuple >>  8;
            *dest++ = tuple;
            dest_count += 4;
            break;
        case 3:
            *dest++ = tuple >> 24;
            *dest++ = tuple >> 16;
            *dest++ = tuple >>  8;
            dest_count += 3;
            break;
        case 2:
            *dest++ = tuple >> 24;
            *dest++ = tuple >> 16;
            dest_count += 2;
            break;
        case 1:
            *dest++ = tuple >> 24;
            dest_count++;;
            break;
        }
    }

error:
    return dest_count;
}
#endif

#if (defined CONNECTOR_SM_ENCRYPTION)
STATIC connector_sm_transport_t sm_transport(connector_transport_t const connector_transport)
{
    connector_sm_transport_t sm_transport;

    switch (connector_transport)
    {
        case connector_transport_all:
            sm_transport = connector_sm_transport_edp;
            break;

#if (defined CONNECTOR_TRANSPORT_TCP)
        case connector_transport_tcp:
            sm_transport = connector_sm_transport_edp;
            break;
#endif
#if (defined CONNECTOR_TRANSPORT_UDP)
        case connector_transport_udp:
            sm_transport = connector_sm_transport_udp;
            break;
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
        case connector_transport_sms:
            sm_transport = connector_sm_transport_sms;
            break;
#endif
        default:
            ASSERT(connector_false);
            break;
    }

    return sm_transport;
}

STATIC connector_bool_t sm_configuration_load(connector_data_t * const connector_ptr, connector_transport_t const transport, connector_sm_encryption_data_type_t const type, void * const data, size_t const bytes_required)
{
    connector_status_t status;
    connector_sm_encryption_load_data_t load;

    load.transport = sm_transport(transport);
    load.type = type;
    load.data = data;
    load.bytes_required = bytes_required;

    status = sm_configuration_service_run_cb(connector_ptr, connector_request_id_sm_encryption_load_data, &load);
    return (status == connector_working);
}

STATIC connector_bool_t sm_configuration_store(connector_data_t * const connector_ptr, connector_transport_t const transport, connector_sm_encryption_data_type_t const type, void const * const data, size_t const bytes_used)
{
    connector_status_t status;
    connector_sm_encryption_store_data_t store;

    store.transport = sm_transport(transport);
    store.type = type;
    store.data = data;
    store.bytes_used = bytes_used;

    status = sm_configuration_service_run_cb(connector_ptr, connector_request_id_sm_encryption_store_data, &store);
    return (status == connector_working);
}

STATIC void sm_generate_iv(connector_data_t * const connector_ptr,
    uint8_t * const iv, size_t const length,
    connector_transport_t const transport, uint8_t const type, uint8_t const pool, uint16_t const request_id
    )
{
    memset(iv, 0, length);

    iv[0] = sm_transport(transport);
    iv[1] = (type | pool);
    iv[2] = HIGH8(request_id);
    iv[3] = LOW8(request_id);

    /* XOR in the final bytes of the device ID */
    {
        size_t const offset = (sizeof connector_ptr->device_id - length);
        uint8_t const * source = &connector_ptr->device_id[offset];

        for (size_t i = 0; i < length; i++)
        {
            iv[i] ^= source[i];
        }
    }
}

STATIC void sm_generate_aad(
    connector_data_t * const connector_ptr,
    uint8_t * const aad, size_t const length,
    uint8_t const info_byte, uint8_t const cs_byte
    )
{
    uint8_t * current = aad;
    size_t const device_id_space = length - 2;

    ASSERT(device_id_space == sizeof connector_ptr->device_id);
    memcpy(current, connector_ptr->device_id, device_id_space);
    current += device_id_space;

    *current = (info_byte & SM_AAD_INFO_BYTE_MASK);
    current++;
    *current = cs_byte;
}

STATIC connector_bool_t sm_reset_request_id(connector_data_t * connector_ptr, connector_sm_data_t * const data, connector_transport_t const transport, char const * const name)
{
    data->request.id = 0;
    if (!sm_configuration_store(connector_ptr, transport, connector_sm_encryption_data_type_id, &data->request.id, sizeof data->request.id))
    {
        connector_debug_line("unable to store the reset key for %s", name);
        return connector_false;
    }

    return connector_true;
}

STATIC connector_bool_t sm_encryption_set_key(connector_data_t * connector_ptr, uint8_t const * const key, size_t const length)
{
    connector_sm_encryption_data_t * const keyring = &connector_ptr->sm_encryption;

    if (length != sizeof keyring->current.key)
    {
        return connector_false;
    }

    if (!sm_configuration_store(connector_ptr, connector_transport_all, connector_sm_encryption_data_type_current_key, key, length))
    {
        connector_debug_line("unable to store new key");
        return connector_false;
    }

#if (defined CONNECTOR_TRANSPORT_UDP)
    if (!sm_reset_request_id(connector_ptr, &connector_ptr->sm_udp, connector_transport_udp, "udp"))
        return connector_false;
#endif

#if (defined CONNECTOR_TRANSPORT_SMS)
    if (!sm_reset_request_id(connector_ptr, &connector_ptr->sm_sms, connector_transport_sms, "sms"))
        return connector_false;
#endif

    memcpy(keyring->previous.key, keyring->current.key, sizeof keyring->previous.key);
    keyring->previous.valid = connector_true;
    sm_configuration_store(connector_ptr, connector_transport_all, connector_sm_encryption_data_type_previous_key, keyring->previous.key, sizeof keyring->previous.key);

    memcpy(keyring->current.key, key, length);
    keyring->current.valid = connector_true;

    return connector_true;
}

STATIC connector_bool_t sm_encryption_get_current_key(connector_data_t * connector_ptr, uint8_t const * * const key, size_t * const length)
{
    connector_sm_encryption_data_t * const keyring = &connector_ptr->sm_encryption;
    connector_bool_t const have_key = keyring->current.valid;

    if (have_key)
    {
        *key = keyring->current.key;
        *length = sizeof keyring->current.key;
    }

    return have_key;
}

STATIC connector_bool_t sm_encryption_get_previous_key(connector_data_t * connector_ptr, uint8_t const * * const key, size_t * const length)
{
    connector_sm_encryption_data_t * const keyring = &connector_ptr->sm_encryption;
    connector_bool_t const have_key = keyring->previous.valid;

    if (have_key)
    {
        *key = keyring->previous.key;
        *length = sizeof keyring->previous.key;
    }

    return have_key;
}

STATIC connector_bool_t sm_encrypt_block(
    connector_data_t * connector_ptr,
    connector_transport_t const transport,
    uint16_t const request_id, uint8_t const info_byte, int8_t cs_byte,
    uint8_t const * const plaintext, size_t const plaintext_length,
    uint8_t * const ciphertext, size_t const ciphertext_length,
    uint8_t * const tag, size_t const tag_length
    )
{
    uint8_t const type = SmIsResponse(info_byte) ? SM_IV_TYPE_RESPONSE : SM_IV_TYPE_REQUEST;
    uint8_t iv[SM_IV_LENGTH];
    uint8_t aad[SM_AAD_LENGTH];
    connector_sm_encrypt_gcm_t encrypt;

    if (plaintext_length != ciphertext_length)
    {
        connector_debug_line("sm_encrypt_block: buffer mismatch plain=%u, cipher=%u", plaintext_length, ciphertext_length);
        return connector_false;
    }

    connector_debug_line("sm_encrypt_block: pointers plain=%p, cipher=%p", plaintext, ciphertext);

    encrypt.message.length = plaintext_length;
    encrypt.message.input = plaintext;
    encrypt.message.output = ciphertext;

    encrypt.tag.length = tag_length;
    encrypt.tag.data = tag;

    encrypt.aad.length = sizeof aad;
    encrypt.aad.data = aad;
    sm_generate_aad(connector_ptr, aad, sizeof aad, info_byte, cs_byte);

    encrypt.iv.length = sizeof iv;
    encrypt.iv.data = iv;
    sm_generate_iv(connector_ptr, iv, sizeof iv, transport, type, SM_IV_POOL_DEVICE, request_id);

    if (!sm_encryption_get_current_key(connector_ptr, &encrypt.key.data, &encrypt.key.length))
    {
        connector_debug_line("sm_encrypt_block: invalid key");
        return connector_false;
    }

    {
        connector_status_t status = sm_configuration_service_run_cb(connector_ptr, connector_request_id_sm_encrypt_gcm, &encrypt);

        if (status != connector_working)
        {
            connector_debug_line("sm_encrypt_block: callback failed status=%u", status);
            return connector_false;
        }
    }

    return connector_true;
}

STATIC uint8_t * get_tracking(connector_data_t * const connector_ptr, connector_transport_t const transport)
{
    uint8_t * tracking;

    switch (transport)
    {
#if (defined CONNECTOR_TRANSPORT_UDP)
        case connector_transport_udp:
            tracking = connector_ptr->sm_udp.request.tracking;
            break;
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
        case connector_transport_sms:
            tracking = connector_ptr->sm_sms.request.tracking;
            break;
#endif
        default:
            tracking = NULL;
            break;
    }

    return tracking;
}

STATIC connector_bool_t sm_write_tracking_data(connector_data_t * const connector_ptr, connector_transport_t const transport)
{
    uint8_t const * const tracking = get_tracking(connector_ptr, transport);
    connector_bool_t const success = sm_configuration_store(connector_ptr, transport, connector_sm_encryption_data_type_tracking, tracking, SM_TRACKING_BYTES);

    if (!success)
    {
        connector_debug_line("sm_write_tracking_data: tracking write failed");
    }

    return success;
}

STATIC connector_bool_t sm_have_seen_request(connector_data_t * connector_ptr, connector_transport_t const transport, uint16_t const request_id)
{
    uint8_t const * const tracking = get_tracking(connector_ptr, transport);
    size_t const byte = (request_id / CHAR_BIT);
    size_t const bit = (request_id - (byte * CHAR_BIT));

    ASSERT(byte < SM_TRACKING_BYTES);
    ASSERT(bit < CHAR_BIT);

    return SmIsBitSet(tracking[byte], 1 << bit);
}

STATIC connector_bool_t sm_mark_request_seen(connector_data_t * connector_ptr, connector_transport_t const transport, uint16_t const request_id)
{
    uint8_t * const tracking = get_tracking(connector_ptr, transport);
    size_t const byte = (request_id / CHAR_BIT);
    size_t const bit = (request_id - (byte * CHAR_BIT));

    ASSERT(byte < SM_TRACKING_BYTES);
    ASSERT(bit < CHAR_BIT);

    SmBitSet(tracking[byte], 1 << bit);
    return sm_write_tracking_data(connector_ptr, transport);
}

STATIC connector_bool_t sm_clear_all_seen(connector_data_t * connector_ptr, connector_transport_t const transport)
{
    uint8_t * const tracking = get_tracking(connector_ptr, transport);

    memset(tracking, 0, SM_TRACKING_BYTES);
    return sm_write_tracking_data(connector_ptr, transport);
}

STATIC connector_bool_t sm_decrypt_block(
    connector_data_t * connector_ptr,
    connector_transport_t const transport,
    uint16_t const request_id, uint8_t const info_byte, int8_t cs_byte,
    uint8_t const * const ciphertext, size_t const ciphertext_length,
    uint8_t const * const tag, size_t const tag_length,
    uint8_t * const plaintext, size_t const plaintext_length
    )
{
    uint8_t const type = SmIsResponse(info_byte) ? SM_IV_TYPE_RESPONSE : SM_IV_TYPE_REQUEST;
    uint8_t iv[SM_IV_LENGTH];
    uint8_t aad[SM_AAD_LENGTH];
    connector_sm_decrypt_gcm_t decrypt;
    connector_status_t status;

    if (plaintext_length != ciphertext_length)
    {
        connector_debug_line("sm_decrypt_block: buffer mismatch plain=%u, cipher=%u", plaintext_length, ciphertext_length);
        return connector_false;
    }

    decrypt.message.length = ciphertext_length;
    decrypt.message.input = ciphertext;
    decrypt.message.output = plaintext;

    decrypt.tag.length = tag_length;
    decrypt.tag.data = (uint8_t *) tag;

    decrypt.aad.length = sizeof aad;
    decrypt.aad.data = aad;
    sm_generate_aad(connector_ptr, aad, sizeof aad, info_byte, cs_byte);

    decrypt.iv.length = sizeof iv;
    decrypt.iv.data = iv;
    sm_generate_iv(connector_ptr, iv, sizeof iv, transport, type, SM_IV_POOL_SERVER, request_id);

    if (!sm_encryption_get_current_key(connector_ptr, &decrypt.key.data, &decrypt.key.length))
    {
        connector_debug_line("sm_decrypt_block: invalid key");
        return connector_false;
    }

    status = sm_configuration_service_run_cb(connector_ptr, connector_request_id_sm_decrypt_gcm, &decrypt);
    if (status == connector_working)
    {
        connector_sm_encryption_data_t * const keyring = &connector_ptr->sm_encryption;

        /* new key works -- stop using (and tracking) the old one */
        if (keyring->previous.valid)
        {
            keyring->previous.valid = connector_false;
            sm_configuration_store(connector_ptr, transport, connector_sm_encryption_data_type_previous_key, "", 0);
            sm_clear_all_seen(connector_ptr, transport);
        }
    }
    else
    {
        connector_debug_line("sm_decrypt_block: callback failed current key status=%u", status);

        /* try the old key -- could be an old message in-flight */
        if (sm_encryption_get_previous_key(connector_ptr, &decrypt.key.data, &decrypt.key.length))
        {
            status = sm_configuration_service_run_cb(connector_ptr, connector_request_id_sm_decrypt_gcm, &decrypt);
            if (status != connector_working)
            {
                connector_debug_line("sm_decrypt_block: callback failed previous key status=%u", status);
                return connector_false;
            }
        }
    }

    if (sm_have_seen_request(connector_ptr, transport, request_id))
    {
        connector_debug_line("sm_decrypt_block:duplicate request %u", request_id);
        return connector_false;
    }

    return sm_mark_request_seen(connector_ptr, transport, request_id);
}

STATIC connector_status_t sm_get_request_id(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, uint16_t * const new_request_id)
{
    connector_status_t result = connector_pending;
    uint16_t const request_id = sm_ptr->request.id;

    if (sm_ptr->request.id == SM_REQUEST_ID_LAST)
        goto error; /* need a new key */

    sm_ptr->request.id++;
    if (!sm_configuration_store(connector_ptr, sm_ptr->network.transport, connector_sm_encryption_data_type_id, &sm_ptr->request.id, sizeof sm_ptr->request.id))
    {
        /* we can no longer trust our request ids */
        sm_ptr->request.id = SM_REQUEST_ID_LAST;
        goto error;
    }

    *new_request_id = request_id;
    result = connector_working;

error:
    return result;
}
#else
/* This function searches for the next valid request_id and leaves it in sm_ptr->request.id */
STATIC connector_status_t sm_get_request_id(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr, uint16_t * const new_request_id)
{
    connector_status_t result = connector_pending;
    uint16_t const request_id = sm_ptr->request.id;

    UNUSED_PARAMETER(connector_ptr);

    do
    {
        connector_sm_session_t * session = sm_ptr->session.head;

        sm_ptr->request.id++;
        sm_ptr->request.id &= SM_REQUEST_ID_MASK;

        while (session != NULL)
        {
            /* already used? */
            if (session->request_id == sm_ptr->request.id)
                break;

            session = session->next;
        }

        if (session == NULL)
            break;

    } while (request_id != sm_ptr->request.id);

    ASSERT_GOTO(request_id != sm_ptr->request.id, error);
    *new_request_id = sm_ptr->request.id;
    result = connector_working;

error:
    return result;
}
#endif
