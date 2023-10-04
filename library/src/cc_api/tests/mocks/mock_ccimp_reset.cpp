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

#include <pthread.h>

#define CCAPI_CONST_PROTECTION_UNLOCK
#include "mocks.h"

void Mock_ccimp_hal_reset_create(void)
{
    return;
}

void Mock_ccimp_hal_reset_destroy(void)
{
    mock("ccimp_hal_reset").checkExpectations();
}

void Mock_ccimp_hal_reset_expectAndReturn(void * retval)
{
    mock("ccimp_hal_reset").expectOneCall("ccimp_hal_reset")
            .andReturnValue(retval);

    /* If we are calling expectations, then override default malloc */
    mock("ccimp_hal_reset").setData("behavior", MOCK_RESET_ENABLED);
}

extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "ccapi_definitions.h"

ccimp_status_t ccimp_hal_reset(void)
{
    int const behavior = mock_scope_c("ccimp_hal_reset")->getData("behavior").value.intValue;
    ccimp_status_t ccimp_status;
    switch (behavior)
    {
        case MOCK_RESET_DISABLED:
            ccimp_status = /* ccimp_hal_reset_real(); */ CCIMP_STATUS_OK;
            break;
        default:
            mock_scope_c("ccimp_hal_reset")->actualCall("ccimp_hal_reset");
            ccimp_status = (ccimp_status_t)mock_scope_c("ccimp_hal_reset")->returnValue().value.intValue;
            break;
    }

    return ccimp_status;
}

}
