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
#include <unistd.h>
#include <semaphore.h>

#include "mocks.h"

static mock_connector_api_info_t mock_info[MAX_INFO];
static sem_t sem;

mock_connector_api_info_t * mock_connector_api_info_alloc(connector_handle_t connector_handle, ccapi_handle_t ccapi_handle)
{
    size_t i;

    sem_wait(&sem);

    for (i = 0; i < MAX_INFO; i++)
    {
        if (mock_info[i].used == connector_false)
        {
            mock_info[i].used = connector_true;
            mock_info[i].connector_handle = connector_handle;
            mock_info[i].ccapi_handle = ccapi_handle;	
            mock_info[i].connector_run_retval = connector_idle;
            mock_info[i].connector_initiate_transport_start_info.init_transport = CCAPI_TRUE;
            mock_info[i].connector_initiate_transport_stop_info.stop_transport = CCAPI_TRUE;
            mock_info[i].connector_initiate_send_data_info.out.data = NULL;
            mock_info[i].connector_initiate_send_data_info.in.chunk_size = 100;
            mock_info[i].connector_initiate_send_data_info.in.response = connector_data_service_send_response_t::connector_data_service_send_response_success; 
            mock_info[i].connector_initiate_send_data_info.in.hint = NULL;
            mock_info[i].connector_initiate_send_data_info.in.status = connector_data_service_status_t::connector_data_service_status_complete;
            mock_info[i].connector_initiate_send_ping_info.response = connector_sm_ping_response_t::connector_sm_ping_status_success; 

            sem_post(&sem);

            return &mock_info[i];
        }
    }

    sem_post(&sem);

    return NULL;
}

mock_connector_api_info_t * mock_connector_api_info_get_nosem(connector_handle_t connector_handle)
{
    size_t i;

    for (i = 0; i < MAX_INFO; i++)
    {
        if (mock_info[i].used == connector_true && mock_info[i].connector_handle == connector_handle)
            return &mock_info[i];
    }

    assert (0);

    return NULL;
}

mock_connector_api_info_t * mock_connector_api_info_get(connector_handle_t connector_handle)
{
    mock_connector_api_info_t * mock_info;

    sem_wait(&sem);

    mock_info = mock_connector_api_info_get_nosem(connector_handle);

    sem_post(&sem);

    return mock_info;
}

void mock_connector_api_info_free(connector_handle_t connector_handle)
{
    mock_connector_api_info_t * mock_info;

    sem_wait(&sem);

    mock_info = mock_connector_api_info_get_nosem(connector_handle);
    mock_info->used = connector_false;
    mock_info->ccapi_handle = NULL;

    sem_post(&sem);
}

union vp2fp 
{
    void* vp;
    connector_callback_status_t (*fp)(connector_class_id_t const class_id, connector_request_id_t const request_id, void * const data, void * const context);
};

void Mock_connector_init_create(void)
{
    memset(mock_info, 0, sizeof(mock_info));

    return;
}

void Mock_connector_init_destroy(void)
{
    mock("connector_init").checkExpectations();
    return;
}

/* context is ignored because its value is allocated by ccapi_start() and is not available from test context */
void Mock_connector_init_expectAndReturn(connector_callback_t const callback, connector_handle_t retval, void * const context)
{
    vp2fp u;
    u.fp = callback;
    UNUSED_ARGUMENT(context);

    mock("connector_init").expectOneCall("connector_init")
             .withParameter("callback", u.vp)
             .andReturnValue(retval);

    mock("connector_init").setData("behavior", MOCK_CONNECTOR_INIT_ENABLED);
}

connector_handle_t connector_init(connector_callback_t const callback, void * context)
{
    vp2fp u;
    u.fp = callback;
    uint8_t behavior;
    connector_handle_t connector_handle;

    behavior = mock("connector_init").getData("behavior").getIntValue();

    if (behavior == MOCK_CONNECTOR_INIT_ENABLED)
    {
        mock("connector_init").actualCall("connector_init").withParameter("callback", u.vp);
        connector_handle = mock("connector_init").returnValue().getPointerValue();
    }
    else
    {
        connector_handle = malloc(sizeof (int)); /* Return a different pointer each time. */
    } 
    
    if (connector_handle != NULL)
        mock_connector_api_info_alloc(connector_handle, (ccapi_handle_t)context);

    return connector_handle;
}

void Mock_connector_run_create(void)
{
    if (sem_init(&sem, 0, 1) == -1)
    {
        printf("Mock_connector_run_create: sem_init error\n");
    }

    return;
}

void Mock_connector_run_destroy(void)
{
    size_t i;
    connector_bool_t finished;

    do
    {
        finished = connector_true;

        sem_wait(&sem);

        for (i = 0; i < MAX_INFO; i++)
        {
            if (mock_info[i].used == connector_true)
            {
                mock_info[i].connector_run_retval = connector_device_terminated;

                sem_post(&sem);

                usleep(100);

                finished = connector_false;
                break;
            }
        }
    } while (finished == connector_false);

    if (sem_destroy(&sem) == -1) printf("ccimp_os_lock_destroy error\n");

    return;
}

connector_status_t connector_run(connector_handle_t const handle)
{
    connector_status_t connector_run_retval;

    /* printf("+connector_run: handle=%p\n",(void*)handle);	*/
    do {
        mock_connector_api_info_t * mock_info = mock_connector_api_info_get(handle);
        connector_run_retval = mock_info != NULL? mock_info->connector_run_retval:connector_idle;

        if (connector_run_retval == connector_idle || connector_run_retval == connector_working || connector_run_retval == connector_pending || connector_run_retval == connector_active || connector_run_retval == connector_success)
        {
            ccimp_os_yield();
        }
    } while (connector_run_retval == connector_idle || connector_run_retval == connector_working || connector_run_retval == connector_pending || connector_run_retval == connector_active || connector_run_retval == connector_success);
    /* printf("-connector_run: handle=%p\n",(void*)handle); */

    switch (connector_run_retval)
    {
        case connector_device_terminated:
        case connector_init_error:
        case connector_abort:
            mock_connector_api_info_free(handle);
            break;
        default:
            break;
    }

    return connector_run_retval;
}

/* * * * * * * * * * connector_initiate_action() * * * * * * * * * */
void Mock_connector_initiate_action_create(void)
{
    mock().installComparator("connector_transport_t", connector_transport_t_comparator);
    mock().installComparator("connector_initiate_stop_request_t", connector_initiate_stop_request_t_comparator);
    mock().installComparator("connector_request_data_service_send_t", connector_request_data_service_send_t_comparator);
    mock().installComparator("connector_sm_send_ping_request_t", connector_sm_send_ping_request_t_comparator);
    mock().installComparator("connector_request_data_point_t", connector_request_data_point_t_comparator);
    return;
}

void Mock_connector_initiate_action_destroy(void)
{
    mock("connector_initiate_action").checkExpectations();
}

void Mock_connector_initiate_action_expectAndReturn(connector_handle_t handle, connector_initiate_request_t request, void * request_data, connector_status_t retval)
{
    switch (request)
    {
        case connector_initiate_transport_stop:
            mock("connector_initiate_action").expectOneCall("connector_initiate_action")
                     .withParameter("handle", handle)
                     .withParameter("request", request)
                     .withParameterOfType("connector_initiate_stop_request_t", "request_data", request_data)
                     .andReturnValue(retval);
            break;
        case connector_initiate_transport_start:
            mock("connector_initiate_action").expectOneCall("connector_initiate_action")
                     .withParameter("handle", handle)
                     .withParameter("request", request)
                     .withParameterOfType("connector_transport_t", "request_data", request_data)
                     .andReturnValue(retval);
            break;
        case connector_initiate_send_data:
            mock("connector_initiate_action").expectOneCall("connector_initiate_action")
                     .withParameter("handle", handle)
                     .withParameter("request", request)
                     .withParameterOfType("connector_request_data_service_send_t", "request_data", request_data)
                     .andReturnValue(retval);

            mock("connector_initiate_action").setData("connector_request_data_service_send_t_behavior", MOCK_CONNECTOR_SEND_DATA_ENABLED);
            break;
#ifdef CONNECTOR_SHORT_MESSAGE
        case connector_initiate_ping_request:
            mock("connector_initiate_action").expectOneCall("connector_initiate_action")
                     .withParameter("handle", handle)
                     .withParameter("request", request)
                     .withParameterOfType("connector_sm_send_ping_request_t", "request_data", request_data)
                     .andReturnValue(retval);

            mock("connector_initiate_action").setData("connector_sm_send_ping_request_t_behavior", MOCK_CONNECTOR_SEND_PING_ENABLED);
            break;
        case connector_initiate_session_cancel:
        case connector_initiate_session_cancel_all:
            break;
#endif
#ifdef CONNECTOR_DATA_POINTS
        case connector_initiate_data_point_binary:
            break;
        case connector_initiate_data_point:
            mock("connector_initiate_action").expectOneCall("connector_initiate_action")
                     .withParameter("handle", handle)
                     .withParameter("request", request)
                     .withParameterOfType("connector_request_data_point_t", "request_data", request_data)
                     .andReturnValue(retval);
            break;
#endif
        case connector_initiate_terminate:
            mock("connector_initiate_action").expectOneCall("connector_initiate_action")
                     .withParameter("handle", handle)
                     .withParameter("request", request)
                     .withParameter("request_data", request_data)
                     .andReturnValue(retval);
            break;
    }

}

connector_status_t connector_initiate_action(connector_handle_t const handle, connector_initiate_request_t const request, void const * const request_data)
{
    mock_connector_api_info_t * mock_info = mock_connector_api_info_get(handle);
    ccapi_data_t * ccapi_data = (ccapi_data_t *)mock_info->ccapi_handle;
    assert(ccapi_data != NULL);
    switch (request)
    {
        case connector_initiate_transport_stop:
        {
            connector_initiate_stop_request_t * closedata = (connector_initiate_stop_request_t *)request_data;
            mock("connector_initiate_action").actualCall("connector_initiate_action")
                    .withParameter("handle", handle)
                    .withParameter("request", request)
                    .withParameterOfType("connector_initiate_stop_request_t", "request_data", (void *)request_data);
            switch(closedata->transport)
            {
                case connector_transport_tcp:
                    if (ccapi_data->transport_tcp.connected && mock_info->connector_initiate_transport_stop_info.stop_transport)
                    {
                        connector_request_id_t request_id;
                        connector_initiate_stop_request_t stop_status = {connector_transport_tcp, connector_wait_sessions_complete, NULL};

                        request_id.status_request = connector_request_id_status_stop_completed;
                        ccapi_connector_callback(connector_class_id_status, request_id, &stop_status, (void *)ccapi_data);
                    }
                    break;
#if (defined CONNECTOR_TRANSPORT_UDP)
                case connector_transport_udp:
                    if ((ccapi_data->transport_udp.started) && mock_info->connector_initiate_transport_stop_info.stop_transport)
                    {
                        connector_request_id_t request_id;
                        connector_initiate_stop_request_t stop_status = {connector_transport_udp, connector_wait_sessions_complete, NULL};

                        request_id.status_request = connector_request_id_status_stop_completed;
                        ccapi_connector_callback(connector_class_id_status, request_id, &stop_status, (void *)ccapi_data);
                    }
                    break;
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
                case connector_transport_sms:
                    if ((ccapi_data->transport_sms.started) && mock_info->connector_initiate_transport_stop_info.stop_transport)
                    {
                        connector_request_id_t request_id;
                        connector_initiate_stop_request_t stop_status = {connector_transport_sms, connector_wait_sessions_complete, NULL};

                        request_id.status_request = connector_request_id_status_stop_completed;
                        ccapi_connector_callback(connector_class_id_status, request_id, &stop_status, (void *)ccapi_data);
                    }
                    break;
#endif
                case connector_transport_all:
                    break;
            }
            break;
        }
        case connector_initiate_transport_start:
        {
            connector_transport_t * transport = (connector_transport_t *)request_data;
            mock("connector_initiate_action").actualCall("connector_initiate_action")
                    .withParameter("handle", handle)
                    .withParameter("request", request)
                    .withParameterOfType("connector_transport_t", "request_data", (void *)request_data);
            switch(*transport)
            {
                case connector_transport_tcp:
                    if (!ccapi_data->transport_tcp.connected && mock_info->connector_initiate_transport_start_info.init_transport)
                    {
                        connector_request_id_t request_id;
                        connector_status_tcp_event_t tcp_status = {connector_tcp_communication_started};

                        request_id.status_request = connector_request_id_status_tcp;
                        ccapi_connector_callback(connector_class_id_status, request_id, &tcp_status, (void *)ccapi_data);
                    }
                    break;
#if (defined CONNECTOR_TRANSPORT_UDP)
                case connector_transport_udp:
                    if (!ccapi_data->transport_udp.started && mock_info->connector_initiate_transport_start_info.init_transport)
                    {
                        ccapi_data->transport_udp.started = CCAPI_TRUE;
                    }
                    break;
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
                case connector_transport_sms:
                    if (!ccapi_data->transport_sms.started && mock_info->connector_initiate_transport_start_info.init_transport)
                    {
                        ccapi_data->transport_sms.started = CCAPI_TRUE;
                    }
                    break;
#endif
                case connector_transport_all:
                    break;
            }
            break;
        }
        case connector_initiate_terminate:
        {
            mock("connector_initiate_action").actualCall("connector_initiate_action")
                    .withParameter("handle", handle)
                    .withParameter("request", request)
                    .withParameter("request_data", (void *)request_data);

            ccapi_data->thread.connector_run->status = CCAPI_THREAD_REQUEST_STOP;
            mock_info->connector_run_retval = connector_device_terminated;

            break;
        }
        case connector_initiate_send_data:
        {

            uint8_t behavior = mock("connector_initiate_action").getData("connector_request_data_service_send_t_behavior").getIntValue();
            if (behavior == MOCK_CONNECTOR_SEND_DATA_ENABLED)
            {
                mock("connector_initiate_action").actualCall("connector_initiate_action")
                    .withParameter("handle", handle)
                    .withParameter("request", request)
                    .withParameterOfType("connector_request_data_service_send_t", "request_data", (void *)request_data);
            }

            /* Call data callback */
            {
                 connector_callback_status_t connector_status;
                 connector_request_id_t request_id;
                 connector_request_data_service_send_t * header = (connector_request_data_service_send_t *)request_data;

                 uint8_t * buffer = NULL;
                 size_t buffer_total_size = 0;
                 size_t buffer_chunk_size = mock_info->connector_initiate_send_data_info.in.chunk_size;
                 connector_bool_t more_data;

                 do
                 {
                     buffer_total_size += buffer_chunk_size;
                     buffer = (uint8_t *)realloc(buffer, buffer_total_size);
                     assert(buffer != NULL);

                     connector_data_service_send_data_t data_service_send = {
                                                           header->transport, 
                                                           header->user_context,
                                                           &buffer[buffer_total_size-buffer_chunk_size],
                                                           buffer_chunk_size,
                                                           0,
                                                           connector_false
                                                           };

                     request_id.data_service_request = connector_request_id_data_service_send_data;
                     connector_status = ccapi_connector_callback(connector_class_id_data_service, request_id, &data_service_send, (void *)ccapi_data);

                     mock_info->connector_initiate_send_data_info.out.data = (void*)buffer;
                     more_data = data_service_send.more_data;
                 } while (more_data == connector_true && connector_status == connector_callback_continue);

            }
            /* Call response callback */
            {
                 connector_request_id_t request_id;
                 connector_request_data_service_send_t * header = (connector_request_data_service_send_t *)request_data;
                 if (header->response_required == connector_true)
                 {
                     request_id.data_service_request = connector_request_id_data_service_send_response;

                     #define HANDLE_RESPONSE(response) \
                             { \
                                 case (response): \
                                     connector_data_service_send_response_t data_service_response = { \
                                                               header->transport, header->user_context, \
                                                               (response), \
                                                               mock_info->connector_initiate_send_data_info.in.hint \
                                                               }; \
                                     ccapi_connector_callback(connector_class_id_data_service, request_id, &data_service_response, (void *)ccapi_data); \
                                     break; \
                             }

                     switch (mock_info->connector_initiate_send_data_info.in.response)
                     {
                         HANDLE_RESPONSE(connector_data_service_send_response_t::connector_data_service_send_response_success);
                         HANDLE_RESPONSE(connector_data_service_send_response_t::connector_data_service_send_response_bad_request);
                         HANDLE_RESPONSE(connector_data_service_send_response_t::connector_data_service_send_response_unavailable);
                         HANDLE_RESPONSE(connector_data_service_send_response_t::connector_data_service_send_response_cloud_error);
                         HANDLE_RESPONSE(connector_data_service_send_response_t::connector_data_service_send_response_COUNT);
                     }
                }
            }
            /* Call status callback */
            {
                 connector_request_id_t request_id;
                 connector_request_data_service_send_t * header = (connector_request_data_service_send_t *)request_data;


                 request_id.data_service_request = connector_request_id_data_service_send_status;


                 #define HANDLE_STATUS(status) \
                         { \
                             case (status): \
                                 connector_data_service_status_t data_service_status = { \
                                                           header->transport, header->user_context, \
                                                           (status), \
                                                           connector_session_error_none \
                                                           }; \
                                 ccapi_connector_callback(connector_class_id_data_service, request_id, &data_service_status, (void *)ccapi_data); \
                                 break; \
                         }

                 switch (mock_info->connector_initiate_send_data_info.in.status)
                 {
                     HANDLE_STATUS(connector_data_service_status_t::connector_data_service_status_complete);
                     HANDLE_STATUS(connector_data_service_status_t::connector_data_service_status_cancel);
                     HANDLE_STATUS(connector_data_service_status_t::connector_data_service_status_timeout);
                     HANDLE_STATUS(connector_data_service_status_t::connector_data_service_status_session_error);
                     HANDLE_STATUS(connector_data_service_status_t::connector_data_service_status_COUNT);
                 }
            }
            break;
        }
#ifdef CONNECTOR_SHORT_MESSAGE
        case connector_initiate_ping_request:
        {

            uint8_t behavior = mock("connector_initiate_action").getData("connector_sm_send_ping_request_t_behavior").getIntValue();
            if (behavior == MOCK_CONNECTOR_SEND_PING_ENABLED)
            {
                mock("connector_initiate_action").actualCall("connector_initiate_action")
                    .withParameter("handle", handle)
                    .withParameter("request", request)
                    .withParameterOfType("connector_sm_send_ping_request_t", "request_data", (void *)request_data);
            }

            /* Call response callback */
            {
                 connector_request_id_t request_id;
                 connector_sm_send_ping_request_t * header = (connector_sm_send_ping_request_t *)request_data;


                 request_id.sm_request = connector_request_id_sm_ping_response;


                 #define HANDLE_PING_RESPONSE(response) \
                         { \
                             case (response): \
                                 connector_sm_ping_response_t ping_response = { \
                                                           header->transport, header->user_context, \
                                                           (response), \
                                                           }; \
                                 ccapi_connector_callback(connector_class_id_short_message, request_id, &ping_response, (void *)ccapi_data); \
                                 break; \
                         }

                 switch (mock_info->connector_initiate_send_ping_info.response)
                 {
                     HANDLE_PING_RESPONSE(connector_sm_ping_response_t::connector_sm_ping_status_success);
                     HANDLE_PING_RESPONSE(connector_sm_ping_response_t::connector_sm_ping_status_complete);
                     HANDLE_PING_RESPONSE(connector_sm_ping_response_t::connector_sm_ping_status_cancel);
                     HANDLE_PING_RESPONSE(connector_sm_ping_response_t::connector_sm_ping_status_timeout);
                     HANDLE_PING_RESPONSE(connector_sm_ping_response_t::connector_sm_ping_status_error);
                 }
            }
            break;
        }
        case connector_initiate_session_cancel:
        case connector_initiate_session_cancel_all:
            break;
#endif
#ifdef CONNECTOR_DATA_POINTS
        case connector_initiate_data_point:
        {
            connector_request_id_t request_id;

            mock("connector_initiate_action").actualCall("connector_initiate_action")
                    .withParameter("handle", handle)
                    .withParameter("request", request)
                    .withParameterOfType("connector_request_data_point_t", "request_data", (connector_request_data_point_t *)request_data);
            {
                request_id.data_point_request = connector_request_id_data_point_response;
                ccapi_connector_callback(connector_class_id_data_point, request_id, mock_info->connector_initiate_data_point.ccfsm_response, (void *)ccapi_data);
                request_id.data_point_request = connector_request_id_data_point_status;
                ccapi_connector_callback(connector_class_id_data_point, request_id, mock_info->connector_initiate_data_point.ccfsm_status, (void *)ccapi_data);
            }
            break;
        }
        case connector_initiate_data_point_binary:
            break;
#endif
    }


    return (connector_status_t)mock("connector_initiate_action").returnValue().getIntValue();
}
