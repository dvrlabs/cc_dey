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

static void test_cli_request_cb(ccapi_transport_t const transport, char const * const command, char const * * const output)
{
    (void)transport;
    (void)command;
    (void)output;

    return;
}

TEST_GROUP(test_ccapi_cli_init)
{
    void setup()
    {
        Mock_create_all();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(test_ccapi_cli_init, testBadRequestCallback)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    ccapi_cli_service_t cli_service = {NULL, NULL};

    th_fill_start_structure_with_good_parameters(&start);
    start.service.cli = &cli_service;

    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_INVALID_CLI_REQUEST_CALLBACK);
}

TEST(test_ccapi_cli_init, testInitOk)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;
    ccapi_cli_service_t cli_service = {test_cli_request_cb, NULL};

    th_fill_start_structure_with_good_parameters(&start);
    start.service.cli = &cli_service;

    error = ccapi_start(&start);

    CHECK(error == CCAPI_START_ERROR_NONE);

    {
        ccapi_stop_error_t stop_error;

        Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
        CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
        CHECK(ccapi_data_single_instance == NULL);
    }
}
