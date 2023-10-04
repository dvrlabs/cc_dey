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

TEST_GROUP(test_ccapi_config_tcp_start_lan_1)
{
    /* This groups starts with LAN and IPv4, No password */
    void setup()
    {

        Mock_create_all();

        th_start_ccapi();
        th_start_tcp_lan_ipv4();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);
        Mock_destroy_all();
    }
};

TEST(test_ccapi_config_tcp_start_lan_1, testConfigConnectionTypeLAN)
{
    connector_request_id_t request;
    connector_config_connection_type_t connection_type = { connector_connection_type_wan };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_connection_type;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &connection_type, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(connection_type.type, connector_connection_type_lan);
}

TEST(test_ccapi_config_tcp_start_lan_1, testConfigMAC)
{
    connector_request_id_t request;
    connector_config_pointer_data_t connector_mac_addr = {NULL, sizeof ccapi_data_single_instance->transport_tcp.info->connection.info.lan.mac_address};
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_mac_addr;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &connector_mac_addr, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(0, memcmp(connector_mac_addr.data, ccapi_data_single_instance->transport_tcp.info->connection.info.lan.mac_address, sizeof ccapi_data_single_instance->transport_tcp.info->connection.info.lan.mac_address));
}

TEST(test_ccapi_config_tcp_start_lan_1, testConfigIPv4)
{
    connector_request_id_t request;
    connector_config_ip_address_t connector_ip_addr = {NULL, connector_ip_address_ipv6};
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_ip_addr;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &connector_ip_addr, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(connector_ip_address_ipv4, connector_ip_addr.ip_address_type);
    CHECK_EQUAL(0, memcmp(connector_ip_addr.address, &ccapi_data_single_instance->transport_tcp.info->connection.ip.address.ipv4, sizeof ccapi_data_single_instance->transport_tcp.info->connection.ip.address.ipv4));
}

TEST(test_ccapi_config_tcp_start_lan_1, testIdVerificationSimple)
{
    connector_request_id_t request;
    connector_config_identity_verification_t connector_id_verification = {connector_identity_verification_password};
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_identity_verification;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &connector_id_verification, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(connector_identity_verification_simple, connector_id_verification.type);
}
