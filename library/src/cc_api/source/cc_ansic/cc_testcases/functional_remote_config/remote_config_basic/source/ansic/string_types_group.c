/*
 * Copyright (c) 2013 Digi International Inc.
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

#include <malloc.h>
#include "connector_types.h"
#include "connector_api.h"
#include "connector_config.h"
#include "platform.h"
#include <string.h>


#define SYSTEM_STRING_LENGTH 64

typedef struct {
    char test_field_string[SYSTEM_STRING_LENGTH];
    char test_field_multiline_string[SYSTEM_STRING_LENGTH];
    char test_field_password[SYSTEM_STRING_LENGTH];
} system_data_t;

system_data_t initial_string_types_data = {"ONE\0", "TWO\0", "THREE\0"};


/* Callback to initialize the settings with initial values */
connector_callback_status_t app_string_types_group_init(connector_remote_config_t * const remote_config)
{
//     APP_DEBUG("app_string_types_group_init\n");

    /* Initialize return value */
    connector_callback_status_t status = connector_callback_continue;

    /* Save in the user context the pointer to the data structure where save the data */
    remote_config->user_context = &initial_string_types_data;

    return status;
}

connector_callback_status_t app_string_types_group_get(connector_remote_config_t * const remote_config)
{
//     APP_DEBUG("app_string_types_group_get\n");

    /* Initialize return value */
    connector_callback_status_t status = connector_callback_continue;

    /* Get the data pointer from the user_context */
    system_data_t * string_types_data = remote_config->user_context;


    switch (remote_config->element.id)
    {
        /* Set the return value with the stored data value */
        case connector_setting_string_types_test_field_string:
            remote_config->response.element_value->string_value = string_types_data->test_field_string;
            break;
        case connector_setting_string_types_test_field_multiline_string:
            remote_config->response.element_value->string_value = string_types_data->test_field_multiline_string;
            break;
        case connector_setting_string_types_test_field_password:
            remote_config->response.element_value->string_value = string_types_data->test_field_password;
            break;
        default:
            ASSERT(0);
            break;
    }

    return status;
}




connector_callback_status_t app_string_types_group_set(connector_remote_config_t * const remote_config)
{
//     APP_DEBUG("app_string_types_group_set\n");

    /* Initialize return value */
    connector_callback_status_t status = connector_callback_continue;

    /* Get the data pointer from the user_context */
    system_data_t * string_types_data = remote_config->user_context;

    /* Create auxiliar pointer to manage the different fields */
    char * new_value_pointer = NULL;

    ASSERT(remote_config->element.type == connector_element_type_string);

    switch (remote_config->element.id)
    {   /* Save in the auxiliar pointer the stored data value */
        case connector_setting_string_types_test_field_string:
            new_value_pointer = string_types_data->test_field_string;
            break;

        case connector_setting_string_types_test_field_multiline_string:
            new_value_pointer = string_types_data->test_field_multiline_string;
            break;

        case connector_setting_string_types_test_field_password:
            new_value_pointer = string_types_data->test_field_password;
            break;

        default:
            ASSERT(0);
            break;
    }

    /* Write the new value in the data memory */
    if (new_value_pointer != NULL)
    {
        size_t const length = strlen(remote_config->element.value->string_value);
        memcpy(new_value_pointer, remote_config->element.value->string_value, length);
        new_value_pointer[length] = '\0';
    }

    return status;
}



connector_callback_status_t app_string_types_group_end(connector_remote_config_t * const remote_config)
{
    APP_DEBUG("app_string_types_group_end\n");

    UNUSED_ARGUMENT(remote_config);

    /* Initialize return value */
    connector_callback_status_t status = connector_callback_continue;

    return status;
}



void app_string_types_group_cancel(connector_remote_config_cancel_t * const remote_config)
{
    APP_DEBUG("app_string_types_group_cancel\n");

    UNUSED_ARGUMENT(remote_config);
}
