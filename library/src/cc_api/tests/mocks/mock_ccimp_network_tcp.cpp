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

#include "mocks.h"

void Mock_ccimp_network_tcp_open_create(void)
{
    mock().installComparator("ccimp_network_open_t", ccimp_network_open_t_comparator);
    return;
}

void Mock_ccimp_network_tcp_open_destroy(void)
{
    mock("ccimp_network_tcp_open").checkExpectations();
}

void Mock_ccimp_network_tcp_open_expectAndReturn(ccimp_network_open_t * expect, ccimp_status_t retval)
{
    mock("ccimp_network_tcp_open").expectOneCall("ccimp_network_tcp_open")
            .withParameterOfType("ccimp_network_open_t", "open_data", expect)
            .andReturnValue(retval);
}

void Mock_ccimp_network_tcp_send_create(void)
{
    mock().installComparator("ccimp_network_send_t", ccimp_network_send_t_comparator);

    return;
}

void Mock_ccimp_network_tcp_send_destroy(void)
{
    mock("ccimp_network_tcp_send").checkExpectations();
}

void Mock_ccimp_network_tcp_send_expectAndReturn(ccimp_network_send_t * expect, ccimp_status_t retval)
{
    mock("ccimp_network_tcp_send").expectOneCall("ccimp_network_tcp_send")
            .withParameterOfType("ccimp_network_send_t", "send_data", expect)
            .andReturnValue(retval);
}

void Mock_ccimp_network_tcp_receive_create(void)
{
    mock().installComparator("ccimp_network_receive_t", ccimp_network_receive_t_comparator);

    return;
}

void Mock_ccimp_network_tcp_receive_destroy(void)
{
    mock("ccimp_network_tcp_receive").checkExpectations();
}

void Mock_ccimp_network_tcp_receive_expectAndReturn(ccimp_network_receive_t * expect, ccimp_status_t retval)
{
    mock("ccimp_network_tcp_receive").expectOneCall("ccimp_network_tcp_receive")
            .withParameterOfType("ccimp_network_receive_t", "receive_data", expect)
            .andReturnValue(retval);
}

void Mock_ccimp_network_tcp_close_create(void)
{
    mock().installComparator("ccimp_network_close_t", ccimp_network_close_t_comparator);

    return;
}

void Mock_ccimp_network_tcp_close_destroy(void)
{
    mock("ccimp_network_tcp_close").checkExpectations();
}

void Mock_ccimp_network_tcp_close_expectAndReturn(ccimp_network_close_t * expect, ccimp_status_t retval)
{
    mock("ccimp_network_tcp_close").expectOneCall("ccimp_network_tcp_close")
            .withParameterOfType("ccimp_network_close_t", "close_data", expect)
            .andReturnValue(retval);
}

extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "ccapi_definitions.h"

ccimp_status_t ccimp_network_tcp_open(ccimp_network_open_t * const open_data)
{
    mock_scope_c("ccimp_network_tcp_open")->actualCall("ccimp_network_tcp_open")->withParameterOfType("ccimp_network_open_t", "open_data", (void *)open_data);

    return (ccimp_status_t)mock_scope_c("ccimp_network_tcp_open")->returnValue().value.intValue;
}

ccimp_status_t ccimp_network_tcp_send(ccimp_network_send_t * const send_data)
{
    mock_scope_c("ccimp_network_tcp_send")->actualCall("ccimp_network_tcp_send")->withParameterOfType("ccimp_network_send_t", "send_data", (void *)send_data);

    return (ccimp_status_t)mock_scope_c("ccimp_network_tcp_send")->returnValue().value.intValue;
}

ccimp_status_t ccimp_network_tcp_receive(ccimp_network_receive_t * const receive_data)
{
    mock_scope_c("ccimp_network_tcp_receive")->actualCall("ccimp_network_tcp_receive")->withParameterOfType("ccimp_network_receive_t", "receive_data", (void *)receive_data);

    return (ccimp_status_t)mock_scope_c("ccimp_network_tcp_receive")->returnValue().value.intValue;
}

ccimp_status_t ccimp_network_tcp_close(ccimp_network_close_t * const close_data)
{
    mock_scope_c("ccimp_network_tcp_close")->actualCall("ccimp_network_tcp_close")->withParameterOfType("ccimp_network_close_t", "close_data", (void *)close_data);

    return (ccimp_status_t)mock_scope_c("ccimp_network_tcp_close")->returnValue().value.intValue;
}
}
