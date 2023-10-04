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

#include "device_id_utils.h"

#define DEVICE_ID_LENGTH                16
#define MAC_ADDR_LENGTH                 6
#define IMEI_LENGTH                     8
#define DEVICE_ID_FROM_IMEI_PREFIX      1
#define MEID_LENGTH                     8
#define DEVICE_ID_FROM_MEID_PREFIX      4
#define ESN_LENGTH                      4
#define DEVICE_ID_FROM_ESN_PREFIX       2

/* Converts the first digit char ('0' to '9') to a nibble starting at index and working backwards. */
static unsigned int digit_to_nibble(char const * const string, int * const index)
{
    unsigned int nibble = 0;
    int current;

    for (current = *index; current >= 0; current--)
    {
        int const ch = string[current];

        if (isxdigit(ch))
        {
            if (isdigit(ch))
                nibble = ch - '0';
            else
                nibble = toupper(ch) - 'A' + 0xA;
            break;
        }
    }
    *index = current - 1;

    return nibble;
}

/* Parse a string with non-digit characters into an array (i.e.: 0123456-78-901234-5 will be stored in {0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45}) */
static int string_to_byte_array(char const * const str, uint8_t * const array, size_t const array_size)
{
    int i;
    int const string_length = strlen(str);
    int index = string_length - 1;

    for (i = array_size - 1; i >= 0; i--)
    {
        unsigned int const ls_nibble = digit_to_nibble(str, &index);
        unsigned int const ms_nibble = digit_to_nibble(str, &index);

        array[i] = (ms_nibble << 4) + ls_nibble;
    }

    return 0;
}

void get_device_id_from_mac(uint8_t * const device_id, uint8_t const * const mac_addr)
{
    memset(device_id, 0x00, DEVICE_ID_LENGTH);
    device_id[8] = mac_addr[0];
    device_id[9] = mac_addr[1];
    device_id[10] = mac_addr[2];
    device_id[11] = 0xFF;
    device_id[12] = 0xFF;
    device_id[13] = mac_addr[3];
    device_id[14] = mac_addr[4];
    device_id[15] = mac_addr[5];
}

void get_device_id_from_imei(uint8_t * const device_id, uint8_t const * const imei)
{
    memset(device_id, 0x00, DEVICE_ID_LENGTH);
    device_id[1] = 0x01;

    device_id[8] = imei[0];
    device_id[9] = imei[1];
    device_id[10] = imei[2];
    device_id[11] = imei[3];
    device_id[12] = imei[4];
    device_id[13] = imei[5];
    device_id[14] = imei[6];
    device_id[15] = imei[7];
}

void get_device_id_from_meid(uint8_t * const device_id, uint8_t const * const meid)
{
    memset(device_id, 0x00, DEVICE_ID_LENGTH);
    device_id[1] = 0x04;

    device_id[8] = meid[0];
    device_id[9] = meid[1];
    device_id[10] = meid[2];
    device_id[11] = meid[3];
    device_id[12] = meid[4];
    device_id[13] = meid[5];
    device_id[14] = meid[6];
    device_id[15] = meid[7];
}

void get_device_id_from_esn(uint8_t * const device_id, uint8_t const * const esn)
{
    memset(device_id, 0x00, DEVICE_ID_LENGTH);
    device_id[1] = 0x02;

    device_id[12] = esn[0];
    device_id[13] = esn[1];
    device_id[14] = esn[2];
    device_id[15] = esn[3];
}

void get_mac_from_string(uint8_t * const mac_addr, char const * const mac_addr_string)
{
    string_to_byte_array(mac_addr_string, mac_addr, MAC_ADDR_LENGTH);
}

void get_imei_from_string(uint8_t * const imei, char const * const imei_string)
{
    string_to_byte_array(imei_string, imei, IMEI_LENGTH);
}

void get_meid_from_string(uint8_t * const meid, char const * const meid_string)
{
    string_to_byte_array(meid_string, meid, MEID_LENGTH);
}

void get_esn_from_string(uint8_t * const esn, char const * const esn_string)
{
    string_to_byte_array(esn_string, esn, ESN_LENGTH);
}

void get_ipv4_from_string(uint8_t * const ipv4, char const * const ipv4_string)
{
    unsigned int first, second, third, fourth;
    int const read = sscanf(ipv4_string, "%u.%u.%u.%u", &first, &second, &third, &fourth);

    if(read != 4)
    {
        printf("INVALID IPv4!!!\n");
        return;
    }

    ipv4[0] = (uint8_t)first;
    ipv4[1] = (uint8_t)second;
    ipv4[2] = (uint8_t)third;
    ipv4[3] = (uint8_t)fourth;
}

