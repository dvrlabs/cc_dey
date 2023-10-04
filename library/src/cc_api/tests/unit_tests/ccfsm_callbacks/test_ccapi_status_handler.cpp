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

TEST_GROUP(test_ccapi_status_handler)
{
    void setup()
    {
        Mock_create_all();

        th_start_ccapi();
        th_start_tcp_wan_ipv4_with_callbacks();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_status_handler, testStatusTCPStarted)
{
    connector_request_id_t request;
    connector_status_tcp_event_t tcp_status = {connector_tcp_communication_started};
    connector_callback_status_t connector_status;

    request.status_request = connector_request_id_status_tcp;
    connector_status = ccapi_connector_callback(connector_class_id_status, request, &tcp_status, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_data_single_instance->transport_tcp.connected);
}

TEST(test_ccapi_status_handler, testStatusTCPKeepaliveMissed)
{
    connector_request_id_t request;
    connector_status_tcp_event_t tcp_status = {connector_tcp_keepalive_missed};
    connector_callback_status_t connector_status;

    request.status_request = connector_request_id_status_tcp;
    connector_status = ccapi_connector_callback(connector_class_id_status, request, &tcp_status, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_tcp_keepalives_cb_called);
    CHECK_EQUAL(CCAPI_KEEPALIVE_MISSED, ccapi_tcp_keepalives_cb_argument);
}

TEST(test_ccapi_status_handler, testStatusTCPKeepaliveRestored)
{
    connector_request_id_t request;
    connector_status_tcp_event_t tcp_status = {connector_tcp_keepalive_restored};
    connector_callback_status_t connector_status;

    request.status_request = connector_request_id_status_tcp;
    connector_status = ccapi_connector_callback(connector_class_id_status, request, &tcp_status, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_data_single_instance->transport_tcp.connected);
    CHECK_EQUAL(CCAPI_TRUE, ccapi_tcp_keepalives_cb_called);
    CHECK_EQUAL(CCAPI_KEEPALIVE_RESTORED, ccapi_tcp_keepalives_cb_argument);
}

TEST(test_ccapi_status_handler, testStatusStop)
{
    connector_request_id_t request;
    connector_initiate_stop_request_t stop_status = {connector_transport_tcp, connector_stop_immediately, NULL};
    connector_callback_status_t connector_status;

    request.status_request = connector_request_id_status_stop_completed;
    connector_status = ccapi_connector_callback(connector_class_id_status, request, &stop_status, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, connector_status);
}
