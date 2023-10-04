/*
 * rci_test_helpers.cpp
 *
 *  Created on: Jan 19, 2015
 *      Author: spastor
 */

#include "test_helper_functions.h"

static char const * called_function = NULL;
static ccapi_rci_info_t received_ccapi_rci_info;
static ccapi_rci_info_t returned_ccapi_rci_info;
static unsigned int return_value;
static void const * p_value;

extern "C" {

extern ccapi_bool_t ccapi_rci_lock_cb;

unsigned int th_rci_called_function(char const * const function_name, ccapi_rci_info_t * const info)
{
    ASSERT(called_function == NULL);
    called_function = function_name;

    received_ccapi_rci_info = *info;
    *info = returned_ccapi_rci_info;

    while (ccapi_rci_lock_cb)
    {
        sched_yield();
    }

    return return_value;
}

void th_set_value_ptr(void const * const value)
{
    p_value = value;
}

}

void th_rci_returnValues(unsigned int const retval, ccapi_rci_info_t const * const expected_rci_info)
{
    return_value = retval;
    returned_ccapi_rci_info = *expected_rci_info;
}

void const * th_rci_get_value_ptr(void)
{
    return p_value;
}

char const * th_rci_last_function_called(void)
{
    return called_function;
}

void th_rci_checkExpectations(char const * const function_name, ccapi_rci_info_t const * const expected_received_rci_info)
{
    STRCMP_EQUAL(function_name, called_function);
    called_function = NULL;

    CHECK_EQUAL(received_ccapi_rci_info.action, expected_received_rci_info->action);
    STRCMP_EQUAL(received_ccapi_rci_info.error_hint, expected_received_rci_info->error_hint);

    CHECK_EQUAL(received_ccapi_rci_info.group.instance, expected_received_rci_info->group.instance);
    CHECK_EQUAL(received_ccapi_rci_info.group.type, expected_received_rci_info->group.type);

    CHECK_EQUAL(received_ccapi_rci_info.query_setting.attributes.compare_to, expected_received_rci_info->query_setting.attributes.compare_to);
    CHECK_EQUAL(received_ccapi_rci_info.query_setting.attributes.source, expected_received_rci_info->query_setting.attributes.source);
    CHECK_EQUAL(received_ccapi_rci_info.query_setting.matches, expected_received_rci_info->query_setting.matches);
    CHECK_EQUAL(received_ccapi_rci_info.element.type, expected_received_rci_info->element.type);
    CHECK_EQUAL(received_ccapi_rci_info.user_context, expected_received_rci_info->user_context);
}
