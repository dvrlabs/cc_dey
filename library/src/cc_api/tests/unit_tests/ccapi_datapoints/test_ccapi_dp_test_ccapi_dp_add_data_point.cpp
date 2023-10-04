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

#include "test_helper_functions.h"

TEST_GROUP(test_ccapi_dp_add_data_point)
{
    ccapi_dp_collection_handle_t dp_collection;
    ccapi_dp_collection_t * dp_collection_spy;

    void setup()
    {
        Mock_create_all();

        ccapi_dp_error_t dp_error;

        dp_error = ccapi_dp_create_collection(&dp_collection);
        CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

        dp_collection_spy = dp_collection;
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_dp_add_data_point, testDataPointAddInvalidArgument)
{
    ccapi_dp_error_t dp_error;

    dp_error = ccapi_dp_add(NULL, "stream1", NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);

    dp_error = ccapi_dp_add(dp_collection, NULL, NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_STREAM_ID, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "", NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_STREAM_ID, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "inexistent_stream", NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_STREAM_ID, dp_error);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddEpochInt32LocQual)
{
    ccapi_dp_error_t dp_error;
    ccapi_location_t location = {42.28, 2.26, 391.0};
    ccapi_timestamp_t timestamp;
    int32_t temperature = 300;
    int32_t quality = 100;

    timestamp.epoch.seconds = 1399550817;
    timestamp.epoch.milliseconds = 123;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_TS_EPOCH " " CCAPI_DP_KEY_DATA_INT32 " " CCAPI_DP_KEY_LOCATION " " CCAPI_DP_KEY_QUALITY);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    th_expect_malloc(sizeof (connector_data_point_t), TH_MALLOC_RETURN_NORMAL, false);
    dp_error = ccapi_dp_add(dp_collection, "stream1", &timestamp, temperature, &location, quality);

    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_native == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_native == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_local_epoch_fractional == ccfsm_data_point->time.source);
    CHECK_EQUAL(temperature, ccfsm_data_point->data.element.native.int_value);
    CHECK_EQUAL(quality, ccfsm_data_point->quality.value);
    CHECK(NULL == ccfsm_data_point->description);

    CHECK_EQUAL(location.latitude, ccfsm_data_point->location.value.native.latitude);
    CHECK_EQUAL(location.longitude, ccfsm_data_point->location.value.native.longitude);
    CHECK_EQUAL(location.elevation, ccfsm_data_point->location.value.native.elevation);

    CHECK_EQUAL(timestamp.epoch.seconds, ccfsm_data_point->time.value.since_epoch_fractional.seconds);
    CHECK_EQUAL(timestamp.epoch.milliseconds, ccfsm_data_point->time.value.since_epoch_fractional.milliseconds);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddInt64LocEpochMS)
{
    ccapi_dp_error_t dp_error;
    ccapi_location_t location = {42.28, 2.26, 391.0};
    ccapi_timestamp_t timestamp;
    int64_t integer = 40 * 1000 * 1000;

    timestamp.epoch_msec = 1399550817000;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_INT64 " " CCAPI_DP_KEY_LOCATION " " CCAPI_DP_KEY_TS_EPOCH_MS);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "stream1", integer, &location, &timestamp);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_native == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_ignore == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_local_epoch_whole == ccfsm_data_point->time.source);
    CHECK(NULL == ccfsm_data_point->description);

    CHECK(integer == ccfsm_data_point->data.element.native.long_value);

    CHECK_EQUAL(location.latitude, ccfsm_data_point->location.value.native.latitude);
    CHECK_EQUAL(location.longitude, ccfsm_data_point->location.value.native.longitude);
    CHECK_EQUAL(location.elevation, ccfsm_data_point->location.value.native.elevation);

    CHECK(timestamp.epoch_msec == ccfsm_data_point->time.value.since_epoch_whole.milliseconds);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddFloat)
{
    ccapi_dp_error_t dp_error;
    float flt_number = 1 / 3;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_FLOAT);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "stream1", flt_number);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_ignore == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_ignore == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_cloud == ccfsm_data_point->time.source);
    CHECK(NULL == ccfsm_data_point->description);

    CHECK_EQUAL(flt_number, ccfsm_data_point->data.element.native.float_value);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddDouble)
{
    ccapi_dp_error_t dp_error;
    double double_number = 1 / 3;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_DOUBLE);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "stream1", double_number);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_ignore == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_ignore == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_cloud == ccfsm_data_point->time.source);
    CHECK(NULL == ccfsm_data_point->description);

    CHECK_EQUAL(double_number, ccfsm_data_point->data.element.native.double_value);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddJson)
{
    ccapi_dp_error_t dp_error;
    char const * const json = "{Data:1}";

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_JSON);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "stream1", json);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;

    CHECK(connector_data_point_type_json == ccfsm_data_stream->type);
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_ignore == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_ignore == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_cloud == ccfsm_data_point->time.source);
    CHECK(NULL == ccfsm_data_point->description);

    STRCMP_EQUAL(json, ccfsm_data_point->data.element.native.string_value);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddGeoJson)
{
    ccapi_dp_error_t dp_error;
    char const * const geojson = "{Data:1}";

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_GEOJSON);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    dp_error = ccapi_dp_add(dp_collection, "stream1", geojson);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;

    CHECK(connector_data_point_type_geojson == ccfsm_data_stream->type);
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_ignore == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_ignore == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_cloud == ccfsm_data_point->time.source);
    CHECK(NULL == ccfsm_data_point->description);

    STRCMP_EQUAL(geojson, ccfsm_data_point->data.element.native.string_value);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddStringTimeISO)
{
    ccapi_dp_error_t dp_error;
    char string_data[] = "String data point";
    char const * const timestamp_iso = "2012-01-12T06:16:55.235Z";
    ccapi_timestamp_t timestamp;

    timestamp.iso8601 = timestamp_iso;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_STRING " " CCAPI_DP_KEY_TS_ISO8601);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    th_expect_malloc(sizeof (connector_data_point_t), TH_MALLOC_RETURN_NORMAL, false);
    th_expect_malloc(strlen(string_data) + 1, TH_MALLOC_RETURN_NORMAL, false);
    th_expect_malloc(strlen(timestamp_iso) + 1, TH_MALLOC_RETURN_NORMAL, false);

    dp_error = ccapi_dp_add(dp_collection, "stream1", string_data, &timestamp);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point->next == NULL);

    connector_data_stream_t * ccfsm_data_stream = dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream;
    connector_data_point_t * ccfsm_data_point = ccfsm_data_stream->point;
    CHECK(connector_data_point_t::data::connector_data_type_native == ccfsm_data_point->data.type);
    CHECK(connector_data_point_t::location::connector_location_type_ignore == ccfsm_data_point->location.type);
    CHECK(connector_data_point_t::quality::connector_quality_type_ignore == ccfsm_data_point->quality.type);
    CHECK(connector_data_point_t::time::connector_time_local_iso8601 == ccfsm_data_point->time.source);
    CHECK(NULL == ccfsm_data_point->description);

    STRCMP_EQUAL(string_data, ccfsm_data_point->data.element.native.string_value);
    STRCMP_EQUAL(timestamp_iso, ccfsm_data_point->time.value.iso8601_string);
}


TEST(test_ccapi_dp_add_data_point, testDataPointAddStringTimeISONoMemory4DataPoint)
{
    ccapi_dp_error_t dp_error;
    char string_data[] = "String data point";
    char const * const timestamp_iso = "2012-01-12T06:16:55.235Z";
    ccapi_timestamp_t timestamp;

    timestamp.iso8601 = timestamp_iso;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_STRING " " CCAPI_DP_KEY_TS_ISO8601);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    th_expect_malloc(sizeof (connector_data_point_t), TH_MALLOC_RETURN_NULL, false);

    dp_error = ccapi_dp_add(dp_collection, "stream1", string_data, &timestamp);
    CHECK_EQUAL(CCAPI_DP_ERROR_INSUFFICIENT_MEMORY, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddStringTimeISONoMemory4StringDP)
{
    ccapi_dp_error_t dp_error;
    char string_data[] = "String data point";
    char const * const timestamp_iso = "2012-01-12T06:16:55.235Z";
    ccapi_timestamp_t timestamp;

    timestamp.iso8601 = timestamp_iso;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_STRING " " CCAPI_DP_KEY_TS_ISO8601);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    th_expect_malloc(sizeof (connector_data_point_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(strlen(string_data) + 1, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(strlen(timestamp_iso) + 1, TH_MALLOC_RETURN_NULL, false);


    dp_error = ccapi_dp_add(dp_collection, "stream1", string_data, &timestamp);
    CHECK_EQUAL(CCAPI_DP_ERROR_INSUFFICIENT_MEMORY, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddStringTimeISONoMemory4TsISOString)
{
    ccapi_dp_error_t dp_error;
    char string_data[] = "String data point";
    char const * const timestamp_iso = "2012-01-12T06:16:55.235Z";
    ccapi_timestamp_t timestamp;

    timestamp.iso8601 = timestamp_iso;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream1", CCAPI_DP_KEY_DATA_STRING " " CCAPI_DP_KEY_TS_ISO8601);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    th_expect_malloc(sizeof (connector_data_point_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(strlen(string_data) + 1, TH_MALLOC_RETURN_NULL, false);

    dp_error = ccapi_dp_add(dp_collection, "stream1", string_data, &timestamp);
    CHECK_EQUAL(CCAPI_DP_ERROR_INSUFFICIENT_MEMORY, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
}

TEST(test_ccapi_dp_add_data_point, testDataPointAddTimeISOStringNoMemory4TsISOString)
{
    ccapi_dp_error_t dp_error;
    char string_data[] = "String data point";
    char const * const timestamp_iso = "2012-01-12T06:16:55.235Z";
    ccapi_timestamp_t timestamp;

    timestamp.iso8601 = timestamp_iso;

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "stream2", CCAPI_DP_KEY_TS_ISO8601 " " CCAPI_DP_KEY_DATA_STRING);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);

    th_expect_malloc(sizeof (connector_data_point_t), TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(strlen(timestamp_iso) + 1, TH_MALLOC_RETURN_NORMAL, true);
    th_expect_malloc(strlen(string_data) + 1, TH_MALLOC_RETURN_NULL, false);

    dp_error = ccapi_dp_add(dp_collection, "stream2", &timestamp, string_data);
    CHECK_EQUAL(CCAPI_DP_ERROR_INSUFFICIENT_MEMORY, dp_error);
    CHECK(dp_collection_spy->ccapi_data_stream_list != NULL);
    CHECK(dp_collection_spy->ccapi_data_stream_list->ccfsm_data_stream->point == NULL);
}
