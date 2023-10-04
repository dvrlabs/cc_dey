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

/* This group doesn't call ccapi_start/stop functions */
TEST_GROUP(ccapi_tcp_start_with_no_ccapi) {};
TEST(ccapi_tcp_start_with_no_ccapi, testNotStarted)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};

    IGNORE_ALL_LEAKS_IN_TEST(); /* TODO: if CCAPI is not started it detects memory leaks */

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_CCAPI_STOPPED, error);
}

TEST_GROUP(test_ccapi_tcp_start_sanity_checks)
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

TEST(test_ccapi_tcp_start_sanity_checks, testNullPointer)
{
    ccapi_tcp_start_error_t error;

    error = ccapi_start_transport_tcp(NULL);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NULL_POINTER, error);
    CHECK(ccapi_data_single_instance->transport_tcp.info == NULL);
}

TEST(test_ccapi_tcp_start_sanity_checks, testBadKeepalivesRx)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};

    tcp_start.keepalives.rx = CCAPI_KEEPALIVES_RX_MAX + 1;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_KEEPALIVES, error);

    tcp_start.keepalives.rx = CCAPI_KEEPALIVES_RX_MIN - 1;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_KEEPALIVES, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testBadKeepalivesTx)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};

    tcp_start.keepalives.tx = CCAPI_KEEPALIVES_TX_MAX + 1;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_KEEPALIVES, error);

    tcp_start.keepalives.tx = CCAPI_KEEPALIVES_TX_MIN - 1;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_KEEPALIVES, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testBadKeepalivesWaitCount)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};

    tcp_start.keepalives.wait_count = CCAPI_KEEPALIVES_WCNT_MAX + 1;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_KEEPALIVES, error);

    tcp_start.keepalives.wait_count = CCAPI_KEEPALIVES_WCNT_MIN - 1;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_KEEPALIVES, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testBadIP)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */
    uint8_t invalid_ipv4[] = {0x00, 0x00, 0x00, 0x00};

    tcp_start.connection.type = CCAPI_CONNECTION_LAN;
    tcp_start.connection.ip.type = CCAPI_IPV4;
    memcpy(tcp_start.connection.ip.address.ipv4, invalid_ipv4, sizeof tcp_start.connection.ip.address.ipv4);
    memcpy(tcp_start.connection.info.lan.mac_address, mac, sizeof tcp_start.connection.info.lan.mac_address);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_IP, error);

    tcp_start.connection.ip.type = CCAPI_IPV6;
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_IP, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testLANIpv4)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */

    tcp_start.connection.type = CCAPI_CONNECTION_LAN;
    tcp_start.connection.ip.type = CCAPI_IPV4;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);
    memcpy(tcp_start.connection.info.lan.mac_address, mac, sizeof tcp_start.connection.info.lan.mac_address);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK(memcmp(ipv4, ccapi_data_single_instance->transport_tcp.info->connection.ip.address.ipv4, sizeof ipv4) == 0);
    CHECK(memcmp(mac, ccapi_data_single_instance->transport_tcp.info->connection.info.lan.mac_address, sizeof mac) == 0);
}

TEST(test_ccapi_tcp_start_sanity_checks, testLANIpv6)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv6[] = {0x00, 0x00, 0x00, 0x00, 0xFE, 0x80, 0x00, 0x00, 0x02, 0x25, 0x64, 0xFF, 0xFE, 0x9B, 0xAF, 0x03}; /* fe80::225:64ff:fe9b:af03 */
    uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */

    tcp_start.connection.type = CCAPI_CONNECTION_LAN;
    tcp_start.connection.ip.type = CCAPI_IPV6;
    memcpy(tcp_start.connection.ip.address.ipv6, ipv6, sizeof tcp_start.connection.ip.address.ipv6);
    memcpy(tcp_start.connection.info.lan.mac_address, mac, sizeof tcp_start.connection.info.lan.mac_address);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK(memcmp(ipv6, ccapi_data_single_instance->transport_tcp.info->connection.ip.address.ipv6, sizeof ipv6) == 0);
    CHECK(memcmp(mac, ccapi_data_single_instance->transport_tcp.info->connection.info.lan.mac_address, sizeof mac) == 0);
    CHECK(ccapi_data_single_instance->transport_tcp.info->connection.password == NULL);
}

TEST(test_ccapi_tcp_start_sanity_checks, testLANZeroMAC)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* 000000:000000 */
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */

    tcp_start.connection.type = CCAPI_CONNECTION_LAN;
    tcp_start.connection.ip.type = CCAPI_IPV4;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);
    memcpy(tcp_start.connection.info.lan.mac_address, mac, sizeof tcp_start.connection.info.lan.mac_address);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_INVALID_MAC, error);
    CHECK(ccapi_data_single_instance->transport_tcp.info == NULL);
}

TEST(test_ccapi_tcp_start_sanity_checks, testPassword)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char password[] = "Hello, World!";

    tcp_start.connection.password = password;
    tcp_start.connection.type = CCAPI_CONNECTION_LAN;
    tcp_start.connection.ip.type = CCAPI_IPV4;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);
    memcpy(tcp_start.connection.info.lan.mac_address, mac, sizeof tcp_start.connection.info.lan.mac_address);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    /* If both pointers are the same, then ccapi_data_single_instance is holding a pointer to a stack variable */
    CHECK(tcp_start.connection.password != ccapi_data_single_instance->transport_tcp.info->connection.password);
    STRCMP_EQUAL(tcp_start.connection.password, ccapi_data_single_instance->transport_tcp.info->connection.password);
}

TEST(test_ccapi_tcp_start_sanity_checks, testWAN)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "+54-3644-421921";

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK(tcp_start.connection.info.wan.link_speed == ccapi_data_single_instance->transport_tcp.info->connection.info.wan.link_speed);
    /* If both pointers are the same, then ccapi_data_single_instance is holding a pointer to a stack variable */
    CHECK(tcp_start.connection.info.wan.phone_number != ccapi_data_single_instance->transport_tcp.info->connection.info.wan.phone_number);
    STRCMP_EQUAL(tcp_start.connection.info.wan.phone_number, ccapi_data_single_instance->transport_tcp.info->connection.info.wan.phone_number);
}

TEST(test_ccapi_tcp_start_sanity_checks, testWANEmptyPhone)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "";

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK(tcp_start.connection.info.wan.link_speed == ccapi_data_single_instance->transport_tcp.info->connection.info.wan.link_speed);
    /* If both pointers are the same, then ccapi_data_single_instance is holding a pointer to a stack variable */
    CHECK(tcp_start.connection.info.wan.phone_number != ccapi_data_single_instance->transport_tcp.info->connection.info.wan.phone_number);
    STRCMP_EQUAL(tcp_start.connection.info.wan.phone_number, ccapi_data_single_instance->transport_tcp.info->connection.info.wan.phone_number);
}

TEST(test_ccapi_tcp_start_sanity_checks, testWANPhoneNull)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = NULL;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_PHONE, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testKeepaliveDefaults)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "+54-3644-421921";

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK_EQUAL(tcp_start.connection.type, ccapi_data_single_instance->transport_tcp.info->connection.type);
    CHECK_EQUAL(CCAPI_KEEPALIVES_RX_DEFAULT, ccapi_data_single_instance->transport_tcp.info->keepalives.rx);
    CHECK_EQUAL(CCAPI_KEEPALIVES_TX_DEFAULT, ccapi_data_single_instance->transport_tcp.info->keepalives.tx);
    CHECK_EQUAL(CCAPI_KEEPALIVES_WCNT_DEFAULT, ccapi_data_single_instance->transport_tcp.info->keepalives.wait_count);
}

TEST(test_ccapi_tcp_start_sanity_checks, testMaxSessions)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "+54-3644-421921";

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    tcp_start.connection.max_transactions = 10;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK_EQUAL(tcp_start.connection.max_transactions, ccapi_data_single_instance->transport_tcp.info->connection.max_transactions);
}

TEST(test_ccapi_tcp_start_sanity_checks, testDefaultMaxSessions)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "+54-3644-421921";

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    tcp_start.connection.max_transactions = 0;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK_EQUAL(1, ccapi_data_single_instance->transport_tcp.info->connection.max_transactions);
}

TEST(test_ccapi_tcp_start_sanity_checks, testTcpInfoNoMemory)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    void * malloc_for_ccapi_tcp = NULL;

    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_tcp_info_t), malloc_for_ccapi_tcp);
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_INSUFFICIENT_MEMORY, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testPasswordNoMemory)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t mac[] = {0x00, 0x04, 0x9D, 0xAB, 0xCD, 0xEF}; /* 00049D:ABCDEF */
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char password[] = "Hello, World!";
    void * malloc_for_ccapi_tcp = malloc(sizeof (ccapi_tcp_info_t));
    void * malloc_for_password = NULL;

    tcp_start.connection.password = password;
    tcp_start.connection.type = CCAPI_CONNECTION_LAN;
    tcp_start.connection.ip.type = CCAPI_IPV4;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);
    memcpy(tcp_start.connection.info.lan.mac_address, mac, sizeof tcp_start.connection.info.lan.mac_address);

    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_tcp_info_t), malloc_for_ccapi_tcp);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof password, malloc_for_password);
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_INSUFFICIENT_MEMORY, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testPhoneNoMemory)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "+54-3644-421921";
    void * malloc_for_ccapi_tcp = malloc(sizeof (ccapi_tcp_info_t));
    void * malloc_for_phone = NULL;

    tcp_start.connection.password = NULL;
    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    Mock_ccimp_os_malloc_expectAndReturn(sizeof (ccapi_tcp_info_t), malloc_for_ccapi_tcp);
    Mock_ccimp_os_malloc_expectAndReturn(sizeof phone_number, malloc_for_phone);
    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_INSUFFICIENT_MEMORY, error);
}

TEST(test_ccapi_tcp_start_sanity_checks, testCallbacksAreCopied)
{
    ccapi_tcp_start_error_t error;
    ccapi_tcp_info_t tcp_start = {{0}};
    uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01}; /* 192.168.1.1 */
    char phone_number[] = "+54-3644-421921";

    tcp_start.connection.type = CCAPI_CONNECTION_WAN;
    tcp_start.connection.info.wan.phone_number = phone_number;
    tcp_start.connection.info.wan.link_speed = 115200;
    memcpy(tcp_start.connection.ip.address.ipv4, ipv4, sizeof tcp_start.connection.ip.address.ipv4);

    connector_transport_t connector_transport = connector_transport_tcp;
    Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_transport_start, &connector_transport, connector_success);

    error = ccapi_start_transport_tcp(&tcp_start);
    CHECK_EQUAL(CCAPI_TCP_START_ERROR_NONE, error);
    CHECK_EQUAL(tcp_start.callback.close, ccapi_data_single_instance->transport_tcp.info->callback.close);
    CHECK_EQUAL(tcp_start.callback.keepalive, ccapi_data_single_instance->transport_tcp.info->callback.keepalive);
}
