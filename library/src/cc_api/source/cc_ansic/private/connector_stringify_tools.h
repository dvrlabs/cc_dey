/*
 * Copyright (c) 2015 Digi International Inc.
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

#ifndef _CONNECTOR_STRINGIFY_TOOLS_H_
#define _CONNECTOR_STRINGIFY_TOOLS_H_

#if (defined CONNECTOR_SUPPORTS_64_BIT_INTEGERS)
#define largest_uint_t uint64_t
#define largest_int_t int64_t
#else
#define largest_uint_t uint32_t
#define largest_int_t int32_t
#endif

#define QUOTES_NEEDED_FLAG          UINT32_C(0x01)
#define LEADING_QUOTES_PUT_FLAG     UINT32_C(0x02)
#define TRAILING_QUOTES_PUT_FLAG    UINT32_C(0x04)
#define ESCAPE_FLAG                 UINT32_C(0x08)

#define IsBitSet(flag, bit)   (connector_bool(((flag) & (bit)) == (bit)))
#define IsBitClear(flag, bit) (connector_bool(((flag) & (bit)) == 0))
#define BitSet(flag, bit)    ((flag) |= (bit))
#define BitClear(flag, bit)  ((flag) &= ~(bit))

#define ClearAllFlags(flag)         ((flag) = 0)

#define QuotesNeeded(flag)          IsBitSet((flag), QUOTES_NEEDED_FLAG)
#define QuotesNotNeeded(flag)       IsBitClear((flag), QUOTES_NEEDED_FLAG)
#define SetQuotesNeeded(flag)       BitSet((flag), QUOTES_NEEDED_FLAG)
#define ClearQuotesNeeded(flag)     BitClear((flag), QUOTES_NEEDED_FLAG)

#define LeadingPut(flag)            IsBitSet((flag), LEADING_QUOTES_PUT_FLAG)
#define LeadingNotPut(flag)         IsBitClear((flag), LEADING_QUOTES_PUT_FLAG)
#define SetLeadingPut(flag)         BitSet((flag), LEADING_QUOTES_PUT_FLAG)
#define ClearLeadingPut(flag)       BitClear((flag), LEADING_QUOTES_PUT_FLAG)

#define TrailingPut(flag)           IsBitSet((flag), TRAILING_QUOTES_PUT_FLAG)
#define TrailingNotPut(flag)        IsBitClear((flag), TRAILING_QUOTES_PUT_FLAG)
#define SetTrailingPut(flag)        BitSet((flag), TRAILING_QUOTES_PUT_FLAG)
#define ClearTrailingPut(flag)      BitClear((flag), TRAILING_QUOTES_PUT_FLAG)

#define EscapeChar(flag)            IsBitSet((flag), ESCAPE_FLAG)
#define NotEscapeChar(flag)         IsBitClear((flag), ESCAPE_FLAG)
#define SetEscapeChar(flag)         BitSet((flag), ESCAPE_FLAG)
#define ClearEscapeChar(flag)       BitClear((flag), ESCAPE_FLAG)

#define character_needs_escaping(character) ((character) == '\\' || (character) == '\"' ? connector_true : connector_false)

typedef struct {
    largest_uint_t value;
    int figures;
    connector_bool_t negative;
    unsigned int base;
} int_info_t;

typedef struct {
    int_info_t integer;
    int_info_t fractional;
    connector_bool_t point_set;
} double_info_t;

typedef struct {
    char const * next_char;
    int quotes_info;
} string_info_t;

typedef struct {
    char * buffer;
    size_t bytes_available;
    size_t bytes_written;
} buffer_info_t;

STATIC void put_character(char const character, buffer_info_t * const buffer_info)
{
    size_t const offset = buffer_info->bytes_written;

    if (buffer_info->buffer != NULL)
    {
        buffer_info->buffer[offset] = character;
    }
    buffer_info->bytes_written += 1;
    buffer_info->bytes_available -= 1;
}

STATIC int count_int_ciphers(largest_uint_t const int_value)
{
    int length = 1;
    largest_uint_t value = int_value;

    while ((value /= 10) >= 1)
    {
        length++;
    }

    return length;
}

STATIC unsigned int get_next_cipher(largest_uint_t number, unsigned int const cipher_order, unsigned int const base)
{
    unsigned int cipher;
    unsigned int i;

    for (i = 0; i < cipher_order - 1; i++)
    {
        number /= base;
    }

    cipher = number % base;

    ASSERT(cipher <= base - 1);

    return cipher;
}

STATIC connector_bool_t process_integer(int_info_t * const int_info, buffer_info_t * const buffer_info)
{
    connector_bool_t done_processing = connector_false;

    if (int_info->negative)
    {
        if (buffer_info->bytes_available > 0)
        {
            put_character('-', buffer_info);
            int_info->negative = connector_false;
        }
        else
        {
            goto done;
        }
    }

    while (int_info->figures != 0 && buffer_info->bytes_available > 0)
    {
        unsigned int const cipher = get_next_cipher(int_info->value, int_info->figures--, int_info->base);

        if (cipher < 10)
        {
            put_character('0' + cipher, buffer_info);
        }
        else
        {
            put_character('A' + cipher - 10, buffer_info);
        }
    }

    if (int_info->figures == 0)
    {
        done_processing = connector_true;
    }

done:
    return done_processing;
}
#if (defined CONNECTOR_DATA_POINTS)

STATIC connector_bool_t string_needs_quotes(char const * const string)
{
    connector_bool_t need_quotes = connector_false;
    size_t index;

    if (string == NULL)
    {
        goto done;
    }

    for (index = 0; string[index] != '\0'; index++)
    {
        if (strchr("\"\\, \t\r\n", string[index]) != NULL)
        {
            need_quotes = connector_true;
            break;
        }
    }
done:
    return need_quotes;
}

STATIC connector_bool_t process_string(string_info_t * const string_info, buffer_info_t * const buffer_info)
{
    int quotes_flags = string_info->quotes_info;
    connector_bool_t done_processing = connector_false;

    if (string_info->next_char == NULL)
    {
        done_processing = connector_true;
        goto done;
    }

    if (QuotesNeeded(quotes_flags) && LeadingNotPut(quotes_flags))
    {
        if (buffer_info->bytes_available > 0)
        {
            put_character('\"', buffer_info);
            SetLeadingPut(quotes_flags);
        }
    }

    while (*string_info->next_char != '\0' && buffer_info->bytes_available > 0)
    {
        if (NotEscapeChar(quotes_flags) && character_needs_escaping(*string_info->next_char))
        {
            SetEscapeChar(quotes_flags);
            put_character('\\', buffer_info);
        }
        else
        {
            put_character(*string_info->next_char, buffer_info);
            string_info->next_char += 1;
            ClearEscapeChar(quotes_flags);
        }
    };

    if (*string_info->next_char == '\0')
    {
        if (QuotesNeeded(quotes_flags))
        {
            if (TrailingPut(quotes_flags))
            {
                done_processing = connector_true;
            }
            else
            {
                if (buffer_info->bytes_available > 0)
                {
                    put_character('\"', buffer_info);
                    SetTrailingPut(quotes_flags);
                    done_processing = connector_true;
                }
            }
        }
        else
        {
            done_processing = connector_true;
        }
    }

done:
    string_info->quotes_info = quotes_flags;
    return done_processing;
}

#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
STATIC connector_bool_t process_double(double_info_t * const double_info, buffer_info_t * const buffer_info)
{
    connector_bool_t done_processing = connector_false;

    while (!done_processing && buffer_info->bytes_available > 0)
    {
        connector_bool_t const done_processing_integer_part = process_integer(&double_info->integer, buffer_info);

        if (done_processing_integer_part)
        {
            if (!double_info->point_set)
            {
                if (buffer_info->bytes_available > 0)
                {
                    put_character('.', buffer_info);
                    double_info->point_set = connector_true;
                }
                else
                {
                    goto done;
                }
            }

            {
                connector_bool_t const done_processing_fractional_part = process_integer(&double_info->fractional, buffer_info);

                if (done_processing_fractional_part)
                {
                    done_processing = connector_true;
                }
            }
        }
    }

done:
    return done_processing;
}
#endif
#endif

STATIC void init_int_info(int_info_t * const int_info, largest_int_t const value, unsigned int const base)
{
    largest_uint_t const absolute_value = value >= 0 ? value : -value;

    int_info->value = absolute_value;
    int_info->base = base;
    int_info->figures = count_int_ciphers(absolute_value);
    int_info->negative = value < 0 ? connector_true : connector_false;
}

#if (defined CONNECTOR_DATA_POINTS)
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
STATIC long double_to_long_rounded(double const double_val)
{
    long long_value;

    if (double_val >= 0)
    {
        long_value = (long)(double_val + 0.5);
    }
    else
    {
        long_value = (long)(double_val - 0.5);
    }

    return long_value;
}

STATIC void init_double_info(double_info_t * const double_info, double const value)
{
    double const absolute_value = value >= 0 ? value : -value;
    long const integer_part = (long)absolute_value;
    double const double_fractional_part = (absolute_value - integer_part) * 1000000;
    long const fractional_part = double_to_long_rounded(double_fractional_part);

    init_int_info(&double_info->integer, integer_part, 10);
    init_int_info(&double_info->fractional, fractional_part, 10);
    double_info->fractional.figures = 6; /* Always add leading zeroes */

    if (value < 0)
    {
        double_info->integer.negative = connector_true;
    }
    double_info->point_set = connector_false;
}
#endif

STATIC void init_string_info(string_info_t * const string_info, char const * const string)
{
    ClearAllFlags(string_info->quotes_info);
    if (string_needs_quotes(string))
    {
        SetQuotesNeeded(string_info->quotes_info);
    }
    string_info->next_char = string;
}
#endif

#endif
