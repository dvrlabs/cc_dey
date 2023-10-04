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
TEST_GROUP(ccapi_ping_with_no_ccapi) 
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

TEST(ccapi_ping_with_no_ccapi, testCcapiNotStarted)
{
    ccapi_ping_error_t error;

    error = ccapi_send_ping(CCAPI_TRANSPORT_UDP);
    CHECK_EQUAL(CCAPI_PING_ERROR_CCAPI_NOT_RUNNING, error);
}

/* This group doesn't call ccapi_transport_udp_start function */
TEST_GROUP(ccapi_ping_with_no_transport_start) 
{
    void setup()
    {
        Mock_create_all();

        th_start_ccapi();
    }

    void teardown()
    {
        Mock_destroy_all();
    }
};

TEST(ccapi_ping_with_no_transport_start, testTransportNotValid)
{
    ccapi_ping_error_t error;

    error = ccapi_send_ping(CCAPI_TRANSPORT_TCP);
    CHECK_EQUAL(CCAPI_PING_ERROR_TRANSPORT_NOT_VALID, error);
}

TEST(ccapi_ping_with_no_transport_start, testTransportNotStarted)
{
    ccapi_ping_error_t error;

    error = ccapi_send_ping(CCAPI_TRANSPORT_UDP);
    CHECK_EQUAL(CCAPI_PING_ERROR_TRANSPORT_NOT_STARTED, error);
}

TEST_GROUP(test_ccapi_ping_common_sanity_checks)
{
    static ccapi_ping_error_t error;

    void setup()
    {
        Mock_create_all();

        th_start_ccapi();

        th_start_udp();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_ping_common_sanity_checks, testSmsTransportNotStarted)
{
    ccapi_ping_error_t error;

    error = ccapi_send_ping(CCAPI_TRANSPORT_SMS);
    CHECK_EQUAL(CCAPI_PING_ERROR_TRANSPORT_NOT_STARTED, error);

    error = ccapi_send_ping_with_reply(CCAPI_TRANSPORT_SMS, CCAPI_SEND_PING_WAIT_FOREVER);
    CHECK_EQUAL(CCAPI_PING_ERROR_TRANSPORT_NOT_STARTED, error);
}


TEST_GROUP(test_ccapi_ping_sanity_checks)
{
    static ccapi_ping_error_t error;

    void setup()
    {
        Mock_create_all();

        th_start_ccapi();

        th_start_udp();
    }

    void teardown()
    {
        th_stop_ccapi(ccapi_data_single_instance);

        Mock_destroy_all();
    }
};

TEST(test_ccapi_ping_sanity_checks, testOK)
{
    ccapi_ping_error_t error;

    error = ccapi_send_ping(CCAPI_TRANSPORT_UDP);
    CHECK_EQUAL(CCAPI_PING_ERROR_NONE, error);

    error = ccapi_send_ping_with_reply(CCAPI_TRANSPORT_UDP, CCAPI_SEND_PING_WAIT_FOREVER);
    CHECK_EQUAL(CCAPI_PING_ERROR_NONE, error);
}
