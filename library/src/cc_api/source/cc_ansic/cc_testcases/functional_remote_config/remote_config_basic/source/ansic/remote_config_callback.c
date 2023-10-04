/*
 * Copyright (c) 2014 Digi International Inc.
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
#include "connector_config.h"
#include "connector_api.h"
#include "platform.h"

/* Get functions from each c file */
extern connector_callback_status_t app_string_types_group_init(connector_remote_config_t * const remote_config);
extern connector_callback_status_t app_string_types_group_get(connector_remote_config_t * const remote_config);
extern connector_callback_status_t app_string_types_group_set(connector_remote_config_t * const remote_config);
extern connector_callback_status_t app_string_types_group_end(connector_remote_config_t * const remote_config);
extern void app_string_types_group_cancel(connector_remote_config_t * const remote_config);











static connector_callback_status_t app_process_session_start(connector_remote_config_t * const remote_config)
{
    connector_callback_status_t status = connector_callback_continue;

    APP_DEBUG("app_process_session_start\n");

    UNUSED_ARGUMENT(remote_config);

    return status;
}

static connector_callback_status_t app_process_session_end(connector_remote_config_t * const remote_config)
{
    APP_DEBUG("app_process_session_end\n");
    UNUSED_ARGUMENT(remote_config);

    return connector_callback_continue;
}



static connector_callback_status_t app_process_action_start(connector_remote_config_t * const remote_config)
{
    APP_DEBUG("app_process_action_start\n");

    if (remote_config->action == connector_remote_action_query)
    {
        APP_DEBUG("connector_remote_action_query\n");
        APP_DEBUG("source=");
        switch (remote_config->attribute.source)
        {
            case rci_query_setting_attribute_source_current:
                APP_DEBUG("'rci_query_setting_attribute_source_current'\n");
                break;
            case rci_query_setting_attribute_source_stored:
                APP_DEBUG("'rci_query_setting_attribute_source_stored'\n");
                break;
            case rci_query_setting_attribute_source_defaults:
                APP_DEBUG("'rci_query_setting_attribute_source_defaults'\n");
                break;
        }
        APP_DEBUG("compare_to=");
        switch (remote_config->attribute.compare_to)
        {
            case rci_query_setting_attribute_compare_to_current:
                APP_DEBUG("'rci_query_setting_attribute_compare_to_current'\n");
                break;
            case rci_query_setting_attribute_compare_to_stored:
                APP_DEBUG("'rci_query_setting_attribute_compare_to_stored'\n");
                break;
            case rci_query_setting_attribute_compare_to_defaults:
                APP_DEBUG("'rci_query_setting_attribute_compare_to_defaults'\n");
                break;
            case rci_query_setting_attribute_compare_to_none:
                APP_DEBUG("'rci_query_setting_attribute_compare_to_none'\n");
                break;
        }
    }

    return connector_callback_continue;
}

static connector_callback_status_t app_process_action_end(connector_remote_config_t * const remote_config)
{
    APP_DEBUG("app_process_action_end\n");

    UNUSED_ARGUMENT(remote_config);
    return connector_callback_continue;
}


/************************************************************************************************/
/******************************** GROUP PROCESS *****************************************************/
static connector_callback_status_t app_process_group_start(connector_remote_config_t * const remote_config)
{
    connector_callback_status_t status = connector_callback_continue;

    APP_DEBUG("app_process_group_start\n");
    UNUSED_ARGUMENT(remote_config);


    switch (remote_config->group.type)
    {
        case connector_remote_group_setting:

            /* Call to each init function for the groups */
            switch(remote_config->group.id)
                {/* ConfigGenerator autogenerates different enum values for each group */
                    case connector_setting_string_types:
                        return app_string_types_group_init(remote_config);
                        break;

                    default:
                        ASSERT(0);
                        break;
                }

            break;

        case connector_remote_group_state:
            status = connector_callback_unrecognized;
            break;

        default:
            APP_DEBUG("app_process_group_start: group not found. type = %d \n", remote_config->group.type);
    }

    return status;
}

static connector_callback_status_t app_process_group_process(connector_remote_config_t * const remote_config)
{
    connector_callback_status_t status = connector_callback_continue;

    APP_DEBUG("app_process_group_process\n");
    UNUSED_ARGUMENT(remote_config);

    if ( remote_config->action == connector_remote_action_set )
    {
        APP_DEBUG("app_process_group_process: connector_remote_action_set\n");

        switch(remote_config->group.id)
        {/* ConfigGenerator autogenerates different enum values for each group */
            case connector_setting_string_types:
                APP_DEBUG("app_process_group_process: Request for group 'connector_setting_string_types'\n");
                return app_string_types_group_set(remote_config);
                break;

            default:
                ASSERT(0);
                break;
        }
    }
    else
    {
        APP_DEBUG("app_process_group_process: connector_remote_action_get\n");

        APP_DEBUG("app_process_group_process: Request Group ID %d\n", remote_config->group.id);
        APP_DEBUG("app_process_group_process: Request Element ID %d\n", remote_config->element.id);


        switch (remote_config->group.id)
        {/* ConfigGenerator autogenerates different enum values for each group */
            case connector_setting_string_types:
                APP_DEBUG("app_process_group_process: Request for group 'connector_setting_string_types'\n");
                return app_string_types_group_get(remote_config);
                break;

            default:
                ASSERT(0);
                break;
        }

    }


    return status;
}

static connector_callback_status_t app_process_group_end(connector_remote_config_t * const remote_config)
{
    connector_callback_status_t status = connector_callback_continue;

    APP_DEBUG("app_process_group_end\n");
    UNUSED_ARGUMENT(remote_config);

    return status;
}




static connector_callback_status_t app_process_session_cancel(connector_remote_config_cancel_t * const remote_config)
{
    connector_callback_status_t status = connector_callback_continue;

    APP_DEBUG("app_process_session_cancel\n");
    if (remote_config->user_context != NULL)
    {
        free(remote_config->user_context);
    }
    return status;
}







/********************************************/
/***** MAIN HANDLER FOR ALL RCI REQUEST *****/
/********************************************/
connector_callback_status_t app_remote_config_handler(connector_request_id_remote_config_t const request_id, void * const data)
{
    connector_callback_status_t status = connector_callback_continue;

    UNUSED_ARGUMENT(data);

    switch (request_id)
    {
        case connector_request_id_remote_config_session_start:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_session_start\n");
            status = app_process_session_start(data);
            break;

        case connector_request_id_remote_config_session_end:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_session_end\n");
            status = app_process_session_end(data);
            break;

        case connector_request_id_remote_config_action_start:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_action_start\n");
            status = app_process_action_start(data);
            break;

        case connector_request_id_remote_config_action_end:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_action_end\n");
            status = app_process_action_end(data);
            break;

        case connector_request_id_remote_config_group_start:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_group_start\n");
            status = app_process_group_start(data);
            break;

        case connector_request_id_remote_config_group_process:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_group_process\n");
            status = app_process_group_process(data);
//             {
//                 connector_remote_config_t * const remote_config = data;
//                 remote_config->response.element_value->string_value = "HOLA";
//             }
            break;

        case connector_request_id_remote_config_group_end:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_group_end\n");
            status = app_process_group_end(data);
            break;

        case connector_request_id_remote_config_session_cancel:
//             APP_DEBUG("app_remote_config_handler: connector_request_id_remote_config_session_cancel\n");
            status = app_process_session_cancel(data);
            break;

        default:
            APP_DEBUG("app_remote_config_handler: unknown request id %d\n", request_id);
            status = connector_callback_unrecognized;
            break;
    }

    return status;
}