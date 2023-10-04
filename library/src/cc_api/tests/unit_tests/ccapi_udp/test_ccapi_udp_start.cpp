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

TEST_GROUP(test_ccapi_udp_start)
{
    void setup()
    {
        Mock_create_all();

        th_start_ccapi();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);
        Mock_destroy_all();
    }
};

static ccapi_bool_t ccapi_udp_close_cb(ccapi_udp_close_cause_t cause)
{
    UNUSED_ARGUMENT(cause);
    return CCAPI_TRUE;
}


TEST(test_ccapi_udp_start, testConnectorInitiateActionOK)
{
    ccapi_udp_start_error_t error;
    ccapi_udp_info_t udp_start = {{0}};

    udp_start.callback.close = ccapi_udp_close_cb;

    connector_transport_t connector_transport = connector_transport_udp;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_NONE, error);
}

TEST(test_ccapi_udp_start, testUDPAlreadyStarted)
{
    ccapi_udp_start_error_t error;
    ccapi_udp_info_t udp_start = {{0}};

    udp_start.callback.close = ccapi_udp_close_cb;

    connector_transport_t connector_transport = connector_transport_udp;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_NONE, error);
    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_ALREADY_STARTED, error);
}

TEST(test_ccapi_udp_start, testConnectorInitiateActionInitError)
{
    ccapi_udp_start_error_t error;
    ccapi_udp_info_t udp_start = {{0}};
    connector_transport_t connector_transport = connector_transport_udp;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_init_error);
    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_INIT, error);
    CHECK_EQUAL(CCAPI_FALSE, ccapi_data_single_instance->transport_udp.started);
    CHECK(ccapi_data_single_instance->transport_udp.info == NULL);

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_invalid_data);
    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_INIT, error);

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_service_busy);
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_invalid_data);
    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_INIT, error);
}

TEST(test_ccapi_udp_start, testConnectorInitiateActionUnknownError)
{
    ccapi_udp_start_error_t error;
    ccapi_udp_info_t udp_start = {{0}};
    connector_transport_t connector_transport = connector_transport_udp;
    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
    mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport,
            connector_abort);
    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_INIT, error);
}


TEST(test_ccapi_udp_start, testUDPConnectionTimeout)
{
    ccapi_udp_start_error_t error;
    ccapi_udp_info_t udp_start = {{0}};
    connector_transport_t connector_transport = connector_transport_udp;

    udp_start.start_timeout = 10;

    {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(ccapi_data_single_instance->connector_handle);
        mock_info->connector_initiate_transport_start_info.init_transport = CCAPI_FALSE;
    }

    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    Mock_ccimp_os_get_system_time_return(0); /* Start time */
    Mock_ccimp_os_get_system_time_return(5);
    Mock_ccimp_os_get_system_time_return(10);
    Mock_ccimp_os_get_system_time_return(15);

    error = ccapi_start_transport_udp(&udp_start);
    CHECK_EQUAL(CCAPI_UDP_START_ERROR_TIMEOUT, error);
    CHECK(ccapi_data_single_instance->transport_udp.info == NULL);
}
