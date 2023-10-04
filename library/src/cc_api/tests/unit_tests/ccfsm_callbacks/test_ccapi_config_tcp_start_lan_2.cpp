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

TEST_GROUP(test_ccapi_config_tcp_start_lan_2)
{
    /* This groups starts with LAN and IPv6, Password enabled, max transactions = 10 and Keepalives RX=90, TX=100, WC=10 */
    void setup()
    {
        Mock_create_all();

        th_start_ccapi();
        th_start_tcp_lan_ipv6_password_keepalives();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_config_tcp_start_lan_2, testConfigIPv6)
{
    connector_request_id_t request;
    connector_config_ip_address_t connector_ip_addr = {NULL, connector_ip_address_ipv4};
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_ip_addr;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &connector_ip_addr, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(connector_ip_address_ipv6, connector_ip_addr.ip_address_type);
    CHECK_EQUAL(0, memcmp(connector_ip_addr.address, &ccapi_data_single_instance->transport_tcp.info->connection.ip.address.ipv6, sizeof ccapi_data_single_instance->transport_tcp.info->connection.ip.address.ipv6));
}

TEST(test_ccapi_config_tcp_start_lan_2, testIdVerificationPassword)
{
    connector_request_id_t request;
    connector_config_identity_verification_t connector_id_verification = {connector_identity_verification_simple};
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_identity_verification;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &connector_id_verification, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(connector_identity_verification_password, connector_id_verification.type);
}

TEST(test_ccapi_config_tcp_start_lan_2, testPassword)
{
    connector_request_id_t request;
    connector_config_pointer_string_t password = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_password;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &password, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(strlen(ccapi_data_single_instance->transport_tcp.info->connection.password), password.length);
    CHECK_EQUAL(ccapi_data_single_instance->transport_tcp.info->connection.password, password.string);
    STRCMP_EQUAL(ccapi_data_single_instance->transport_tcp.info->connection.password, password.string);
}

TEST(test_ccapi_config_tcp_start_lan_2, testMaxTransactions)
{
    connector_request_id_t request;
    connector_config_max_transaction_t max_transaction = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_max_transaction;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &max_transaction, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(ccapi_data_single_instance->transport_tcp.info->connection.max_transactions, max_transaction.count);
}

TEST(test_ccapi_config_tcp_start_lan_2, testRxKeepalives)
{
    connector_request_id_t request;
    connector_config_keepalive_t rx_keepalive = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_rx_keepalive;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &rx_keepalive, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(ccapi_data_single_instance->transport_tcp.info->keepalives.rx, rx_keepalive.interval_in_seconds);
}

TEST(test_ccapi_config_tcp_start_lan_2, testTxKeepalives)
{
    connector_request_id_t request;
    connector_config_keepalive_t tx_keepalive = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_tx_keepalive;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &tx_keepalive, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(ccapi_data_single_instance->transport_tcp.info->keepalives.tx, tx_keepalive.interval_in_seconds);
}

TEST(test_ccapi_config_tcp_start_lan_2, testWcKeepalives)
{
    connector_request_id_t request;
    connector_config_wait_count_t wait_count = { 0 };
    connector_callback_status_t callback_status;

    request.config_request = connector_request_id_config_wait_count;
    callback_status = ccapi_connector_callback(connector_class_id_config, request, &wait_count, ccapi_data_single_instance);
    CHECK_EQUAL(connector_callback_continue, callback_status);
    CHECK_EQUAL(ccapi_data_single_instance->transport_tcp.info->keepalives.wait_count, wait_count.count);
}
