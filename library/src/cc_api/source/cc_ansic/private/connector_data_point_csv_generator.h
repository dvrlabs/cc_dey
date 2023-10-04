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

#ifndef _CONNECTOR_DATA_POINT_CSV_GENERATOR_H_
#define _CONNECTOR_DATA_POINT_CSV_GENERATOR_H_

#include "connector_stringify_tools.h"

/************************************************************************
** WARNING: Don't change the order of the state unless default         **
**          CSV format described in the Cloud documentation changes.   **
************************************************************************/
typedef enum {
    csv_data,
    csv_time,
    csv_quality,
    csv_description,
    csv_location,
    csv_type,
    csv_unit,
    csv_forward_to,
    csv_stream_id,
    csv_finished
} csv_field_t;

typedef enum {
    LOCATION_STATE_PUT_LEADING_QUOTE,
    LOCATION_STATE_INIT_LATITUDE,
    LOCATION_STATE_PUT_LATITUDE,
    LOCATION_STATE_PUT_1ST_COMMA,
    LOCATION_STATE_INIT_LONGITUDE,
    LOCATION_STATE_PUT_LONGITUDE,
    LOCATION_STATE_PUT_2ND_COMMA,
    LOCATION_STATE_INIT_ELEVATION,
    LOCATION_STATE_PUT_ELEVATION,
    LOCATION_STATE_PUT_TRAILING_QUOTE,
    LOCATION_STATE_FINISH
} location_state_t;

typedef enum {
    TIME_EPOCH_FRAC_STATE_SECONDS,
    TIME_EPOCH_FRAC_STATE_MILLISECONDS,
    TIME_EPOCH_FRAC_STATE_FINISH
} time_epoch_frac_state_t;

typedef struct {
    connector_data_stream_t const * current_data_stream;
    connector_data_point_t const * current_data_point;
    csv_field_t current_csv_field;

    struct {
        connector_bool_t init;
        union {
            int_info_t intg;
            double_info_t dbl;
            string_info_t str;
        } info;
        union {
            location_state_t location;
            time_epoch_frac_state_t time;
        } internal_state;
    } data;
} csv_process_data_t;

STATIC void terminate_csv_field(csv_process_data_t * const csv_process_data, buffer_info_t * const buffer_info, csv_field_t const next_field)
{
    if (buffer_info->bytes_available > 0)
    {
        csv_process_data->data.init = connector_false;
        put_character(',', buffer_info);
        csv_process_data->current_csv_field = next_field;
    }
}

STATIC connector_bool_t process_csv_data(csv_process_data_t * const csv_process_data, buffer_info_t * const buffer_info)
{
    connector_data_point_t const * const current_data_point = csv_process_data->current_data_point;
    connector_data_stream_t const * const current_data_stream = csv_process_data->current_data_stream;
    connector_bool_t done_processing = connector_false;

    if (!csv_process_data->data.init)
    {
        csv_process_data->data.init = connector_true;

        switch (current_data_point->data.type)
        {
            case connector_data_type_text:
            {
                init_string_info(&csv_process_data->data.info.str, current_data_point->data.element.text);
                break;
            }
            case connector_data_type_native:
            {
                switch (current_data_stream->type)
                {
                    case connector_data_point_type_string:
                    case connector_data_point_type_geojson:
                    case connector_data_point_type_json:
                    case connector_data_point_type_binary:
                    {


                        init_string_info(&csv_process_data->data.info.str, current_data_point->data.element.native.string_value);
                        break;
                    }
                    case connector_data_point_type_double:
                    {
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                        init_double_info(&csv_process_data->data.info.dbl, current_data_point->data.element.native.double_value);
#else
                        connector_debug_line("CONNECTOR_SUPPORTS_FLOATING_POINT not defined");
                        ASSERT(current_data_stream->type != connector_data_point_type_double);
#endif
                        break;
                    }
                    case connector_data_point_type_float:
                    {
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                        init_double_info(&csv_process_data->data.info.dbl, current_data_point->data.element.native.float_value);
#else
                        connector_debug_line("CONNECTOR_SUPPORTS_FLOATING_POINT not defined");
                        ASSERT(current_data_stream->type != connector_data_point_type_float);
#endif
                        break;
                    }
                    case connector_data_point_type_integer:
                    {
                        init_int_info(&csv_process_data->data.info.intg, current_data_point->data.element.native.int_value, 10);
                        break;
                    }
                    case connector_data_point_type_long:
                    {
#if (defined CONNECTOR_SUPPORTS_64_BIT_INTEGERS)
                        init_int_info(&csv_process_data->data.info.intg, current_data_point->data.element.native.long_value, 10);
                        break;
#else
                        connector_debug_line("CONNECTOR_SUPPORTS_64_BIT_INTEGERS not defined");
                        ASSERT(current_data_stream->type != connector_data_point_type_long);
#endif
                    }

                }
                break;
            }
        }
    }

    switch (current_data_point->data.type)
    {
        case connector_data_type_text:
            done_processing = process_string(&csv_process_data->data.info.str, buffer_info);
            break;
        case connector_data_type_native:
            switch (current_data_stream->type)
            {
                case connector_data_point_type_string:
                case connector_data_point_type_geojson:
                case connector_data_point_type_json:
                case connector_data_point_type_binary:
                    done_processing = process_string(&csv_process_data->data.info.str, buffer_info);
                    break;
                case connector_data_point_type_double:
                case connector_data_point_type_float:
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                    done_processing = process_double(&csv_process_data->data.info.dbl, buffer_info);
#else
                    connector_debug_line("CONNECTOR_SUPPORTS_FLOATING_POINT not defined");
                    ASSERT(connector_false);
#endif
                    break;
                case connector_data_point_type_long:
#if !(defined CONNECTOR_SUPPORTS_64_BIT_INTEGERS)
                    connector_debug_line("CONNECTOR_SUPPORTS_64_BIT_INTEGERS not defined");
                    ASSERT(connector_false);
#endif
                    /* Intentional fall through */
                case connector_data_point_type_integer:
                    done_processing = process_integer(&csv_process_data->data.info.intg, buffer_info);
                    break;
            }
            break;
    }

    return done_processing;
}

STATIC connector_bool_t process_csv_location(csv_process_data_t * const csv_process_data, buffer_info_t * const buffer_info)
{
    connector_data_point_t const * const current_data_point = csv_process_data->current_data_point;
    connector_bool_t done_processing = connector_false;

    switch (current_data_point->location.type)
    {
        case connector_location_type_ignore:
        {
            done_processing = connector_true;
            break;
        }
        case connector_location_type_text:
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
        case connector_location_type_native:
#endif
        {
            if (!csv_process_data->data.init)
            {
                csv_process_data->data.init = connector_true;
                csv_process_data->data.internal_state.location = LOCATION_STATE_PUT_LEADING_QUOTE;
            }
            switch (csv_process_data->data.internal_state.location)
            {
                case LOCATION_STATE_PUT_LEADING_QUOTE:
                case LOCATION_STATE_PUT_TRAILING_QUOTE:
                {
                    if (buffer_info->bytes_available > 0)
                    {
                        put_character('\"', buffer_info);
                        csv_process_data->data.internal_state.location++;
                    }
                    break;
                }
                case LOCATION_STATE_INIT_LATITUDE:
                {
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                    if (current_data_point->location.type == connector_location_type_native)
                    {
                        init_double_info(&csv_process_data->data.info.dbl, current_data_point->location.value.native.latitude);
                    }
                    else
                    {
                        init_string_info(&csv_process_data->data.info.str, current_data_point->location.value.text.latitude);
                        ClearQuotesNeeded(csv_process_data->data.info.str.quotes_info);
                    }
#else
                    init_string_info(&csv_process_data->data.info.str, current_data_point->location.value.text.latitude);
                    ClearQuotesNeeded(csv_process_data->data.info.str.quotes_info);
#endif
                    csv_process_data->data.internal_state.location++;
                    break;
                }
                case LOCATION_STATE_INIT_LONGITUDE:
                {
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                    if (current_data_point->location.type == connector_location_type_native)
                    {
                        init_double_info(&csv_process_data->data.info.dbl, current_data_point->location.value.native.longitude);
                    }
                    else
                    {
                        init_string_info(&csv_process_data->data.info.str, current_data_point->location.value.text.longitude);
                        ClearQuotesNeeded(csv_process_data->data.info.str.quotes_info);
                    }
#else
                    init_string_info(&csv_process_data->data.info.str, current_data_point->location.value.text.longitude);
                    ClearQuotesNeeded(csv_process_data->data.info.str.quotes_info);
#endif
                    csv_process_data->data.internal_state.location++;
                    break;
                }
                case LOCATION_STATE_INIT_ELEVATION:
                {
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                    if (current_data_point->location.type == connector_location_type_native)
                    {
                        init_double_info(&csv_process_data->data.info.dbl, current_data_point->location.value.native.elevation);
                    }
                    else
                    {
                        init_string_info(&csv_process_data->data.info.str, current_data_point->location.value.text.elevation);
                        ClearQuotesNeeded(csv_process_data->data.info.str.quotes_info);
                    }
#else
                    init_string_info(&csv_process_data->data.info.str, current_data_point->location.value.text.elevation);
                    ClearQuotesNeeded(csv_process_data->data.info.str.quotes_info);
#endif
                    csv_process_data->data.internal_state.location++;
                    break;
                }
                case LOCATION_STATE_PUT_LATITUDE:
                case LOCATION_STATE_PUT_LONGITUDE:
                case LOCATION_STATE_PUT_ELEVATION:
                {
#if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                    connector_bool_t field_done;

                    if (current_data_point->location.type == connector_location_type_native)
                    {
                        field_done = process_double(&csv_process_data->data.info.dbl, buffer_info);
                    }
                    else
                    {
                        field_done = process_string(&csv_process_data->data.info.str, buffer_info);
                    }
#else
                    connector_bool_t const field_done = process_string(&csv_process_data->data.info.str, buffer_info);
#endif

                    if (field_done)
                    {
                        csv_process_data->data.internal_state.location++;
                    }
                    break;
                }
                case LOCATION_STATE_PUT_1ST_COMMA:
                case LOCATION_STATE_PUT_2ND_COMMA:
                {
                    if (buffer_info->bytes_available > 0)
                    {
                        put_character(',', buffer_info);
                        csv_process_data->data.internal_state.location++;
                    }
                    break;
                }
                case LOCATION_STATE_FINISH:
                    done_processing = connector_true;
                    break;
            }
            break;
        }
    }

    return done_processing;
}

STATIC connector_bool_t process_csv_time(csv_process_data_t * const csv_process_data, buffer_info_t * const buffer_info)
{
    connector_data_point_t const * const current_data_point = csv_process_data->current_data_point;
    connector_bool_t done_processing = connector_false;

    switch (current_data_point->time.source)
    {
        case connector_time_cloud:
            done_processing = connector_true;
            break;
        case connector_time_local_epoch_fractional:
        {
            if (!csv_process_data->data.init)
            {
                csv_process_data->data.init = connector_true;
                init_int_info(&csv_process_data->data.info.intg, current_data_point->time.value.since_epoch_fractional.seconds, 10);
                csv_process_data->data.internal_state.time = TIME_EPOCH_FRAC_STATE_SECONDS;
            }

            switch (csv_process_data->data.internal_state.time)
            {
                case TIME_EPOCH_FRAC_STATE_SECONDS:
                case TIME_EPOCH_FRAC_STATE_MILLISECONDS:
                {
                    connector_bool_t const field_done = process_integer(&csv_process_data->data.info.intg, buffer_info);

                    if (field_done)
                    {
                        init_int_info(&csv_process_data->data.info.intg, current_data_point->time.value.since_epoch_fractional.milliseconds, 10);
                        csv_process_data->data.info.intg.figures = 3; /* Always add leading zeroes, i.e. 1 millisecond must be "001" */
                        csv_process_data->data.internal_state.time++;
                    }
                    break;
                }
                case TIME_EPOCH_FRAC_STATE_FINISH:
                    done_processing = connector_true;
                    break;
            }
            break;
        }
#if (defined CONNECTOR_SUPPORTS_64_BIT_INTEGERS)
        case connector_time_local_epoch_whole:
        {
            if (!csv_process_data->data.init)
            {
                csv_process_data->data.init = connector_true;
                init_int_info(&csv_process_data->data.info.intg, current_data_point->time.value.since_epoch_whole.milliseconds, 10);
            }

            done_processing = process_integer(&csv_process_data->data.info.intg, buffer_info);
            break;
        }
#endif
        case connector_time_local_iso8601:
        {
            if (!csv_process_data->data.init)
            {
                csv_process_data->data.init = connector_true;
                init_string_info(&csv_process_data->data.info.str, current_data_point->time.value.iso8601_string);
            }
            done_processing = process_string(&csv_process_data->data.info.str, buffer_info);
            break;
        }
    }

    return done_processing;
}

size_t dp_generate_csv(csv_process_data_t * const csv_process_data, buffer_info_t * const buffer_info)
{
    while (buffer_info->bytes_available && csv_process_data->current_data_point != NULL)
    {
        connector_data_point_t const * const current_data_point = csv_process_data->current_data_point;
        connector_data_stream_t const * const current_data_stream = csv_process_data->current_data_stream;

        switch (csv_process_data->current_csv_field)
        {
            case csv_data:
            {
                connector_bool_t const done_processing = process_csv_data(csv_process_data, buffer_info);

                if (done_processing)
                {
                    terminate_csv_field(csv_process_data, buffer_info, csv_time);
                }
                break;
            }
            case csv_time:
            {
                connector_bool_t const done_processing = process_csv_time(csv_process_data, buffer_info);

                if (done_processing)
                {
                    terminate_csv_field(csv_process_data, buffer_info, csv_quality);
                }
                break;
            }
            case csv_quality:
            {
                connector_bool_t done_processing = connector_false;

                switch (current_data_point->quality.type)
                {
                    case connector_quality_type_ignore:
                    {
                        done_processing = connector_true;
                        break;
                    }
                    case connector_quality_type_native:
                    {
                        if (!csv_process_data->data.init)
                        {
                            csv_process_data->data.init = connector_true;
                            init_int_info(&csv_process_data->data.info.intg, current_data_point->quality.value, 10);
                        }

                        done_processing = process_integer(&csv_process_data->data.info.intg, buffer_info);
                        break;
                    }
                }

                if (done_processing)
                {
                    terminate_csv_field(csv_process_data, buffer_info, csv_description);
                }
                break;
            }

            case csv_location:
            {
                connector_bool_t const done_processing = process_csv_location(csv_process_data, buffer_info);

                if (done_processing)
                {
                    terminate_csv_field(csv_process_data, buffer_info, csv_type);
                }
                break;
            }

            case csv_type:
            case csv_description:
            case csv_unit:
            case csv_forward_to:
            case csv_stream_id:
            {
                connector_bool_t done_processing;

                if (!csv_process_data->data.init)
                {
                    char const * string = NULL;

                    switch (csv_process_data->current_csv_field)
                    {
                        case csv_description:
                            string = current_data_point->description;
                            break;
                        case csv_type:
                        {
                            static char const * const type_list[] = {"INTEGER", "LONG", "FLOAT", "DOUBLE", "STRING", "BINARY", "JSON", "GEOJSON"};
                            string = type_list[csv_process_data->current_data_stream->type];
                            break;
                        }
                        case csv_unit:
                            string = current_data_stream->unit;
                            break;
                        case csv_forward_to:
                            string = current_data_stream->forward_to;
                            break;
                        case csv_stream_id:
                            string = current_data_stream->stream_id;
                            break;
                        default:
                            ASSERT(0);
                            break;
                    }

                    csv_process_data->data.init = connector_true;
                    init_string_info(&csv_process_data->data.info.str, string);
                }

                done_processing = process_string(&csv_process_data->data.info.str, buffer_info);

                if (done_processing && buffer_info->bytes_available > 0)
                {
                    csv_process_data->data.init = connector_false;
                    csv_process_data->current_csv_field++;

                    ASSERT(csv_process_data->current_csv_field <= csv_finished);

                    if (csv_process_data->current_csv_field != csv_finished)
                    {
                        put_character(',', buffer_info);
                    }
                }
                break;
            }

            case csv_finished:
            {
                if (buffer_info->bytes_available > 0)
                {
                    put_character('\n', buffer_info);
                    csv_process_data->current_data_point = current_data_point->next;

                    if (csv_process_data->current_data_point == NULL)
                    {
                        csv_process_data->current_data_stream = csv_process_data->current_data_stream->next;

                        if (csv_process_data->current_data_stream != NULL)
                        {
                            csv_process_data->current_data_point = csv_process_data->current_data_stream->point;
                        }
                    }

                    csv_process_data->current_csv_field = csv_data;
                    csv_process_data->data.init = connector_false;
                }
                break;
            }
        }
    }

    return buffer_info->bytes_written;
}
#endif
