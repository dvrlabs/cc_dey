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

TEST_GROUP(test_ccapi_dp_collection_dp_count)
{
    void setup()
    {
        Mock_create_all();
        th_start_ccapi();
        th_start_tcp_lan_ipv4();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_dp_collection_dp_count, testInvalidArgument)
{
    ccapi_dp_error_t dp_error;
    ccapi_dp_collection_handle_t dp_collection;

    dp_error = ccapi_dp_get_collection_points_count(NULL, NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);

    dp_error = ccapi_dp_get_collection_points_count(dp_collection, NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);

    dp_error = ccapi_dp_create_collection(&dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    dp_error = ccapi_dp_get_collection_points_count(dp_collection, NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);
}

TEST(test_ccapi_dp_collection_dp_count, testCount)
{
    ccapi_dp_error_t dp_error;
    ccapi_dp_collection_handle_t dp_collection;
    uint32_t points_count = 0xFF;

    dp_error = ccapi_dp_create_collection(&dp_collection);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    dp_error = ccapi_dp_get_collection_points_count(dp_collection, NULL);
    CHECK_EQUAL(CCAPI_DP_ERROR_INVALID_ARGUMENT, dp_error);

    dp_error = ccapi_dp_get_collection_points_count(dp_collection, &points_count);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    CHECK_EQUAL(0, points_count);

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "int_stream", CCAPI_DP_KEY_DATA_INT32);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    th_check_collection_dp_count(dp_collection, 0);

    dp_error = ccapi_dp_add_data_stream_to_collection(dp_collection, "float_stream", CCAPI_DP_KEY_DATA_FLOAT);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    th_check_collection_dp_count(dp_collection, 0);

    dp_error = ccapi_dp_add(dp_collection, "int_stream", 1);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    th_check_collection_dp_count(dp_collection, 1);

    dp_error = ccapi_dp_add(dp_collection, "int_stream", 1);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    th_check_collection_dp_count(dp_collection, 2);

    dp_error = ccapi_dp_add(dp_collection, "float_stream", 1.0);
    CHECK_EQUAL(CCAPI_DP_ERROR_NONE, dp_error);
    th_check_collection_dp_count(dp_collection, 3);
}
