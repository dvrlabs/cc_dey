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

#ifndef _MOCK_CONNECTOR_API_H_
#define _MOCK_CONNECTOR_API_H_

#define MOCK_CONNECTOR_INIT_ENABLED 1
#define MOCK_CONNECTOR_RUN_ENABLED 1
#define MOCK_CONNECTOR_SEND_DATA_ENABLED 1
#define MOCK_CONNECTOR_SEND_PING_ENABLED 1

#define WAIT_FOR_ASSERT()   {do ccimp_os_yield(); while (assert_buffer == NULL);}

#define ASSERT_IF_NOT_HIT_DO(label, file, function, code) \
                       ON_FALSE_DO_(assert_buffer != NULL && \
                                    (!strcmp(assert_buffer, label) && (!strcmp(assert_file, file))) \
                                    , {printf("Didn't hit assert: %s\n", label); code;})
#define ASSERT_CLEAN()                      assert_buffer = NULL

#define MAX_INFO 10

typedef struct  {
    connector_bool_t used;
    ccapi_handle_t ccapi_handle;
    connector_handle_t connector_handle;
    connector_status_t connector_run_retval;
    struct {
        ccapi_bool_t init_transport;
    } connector_initiate_transport_start_info;
    struct {
        ccapi_bool_t stop_transport;
    } connector_initiate_transport_stop_info;
    struct {
        struct {
            size_t chunk_size;
            int status;
            int response;
            const char * hint;
        } in;
        struct {
            void * data;
        } out;
    } connector_initiate_send_data_info;
    struct {
        int response;
    } connector_initiate_send_ping_info;
    struct {
        connector_data_point_response_t * ccfsm_response;
        connector_data_point_status_t * ccfsm_status;
    } connector_initiate_data_point;
} mock_connector_api_info_t;

mock_connector_api_info_t * mock_connector_api_info_get(connector_handle_t connector_handle);
void mock_connector_api_info_free(connector_handle_t connector_handle);

void Mock_connector_init_create(void);
void Mock_connector_init_destroy(void);
void Mock_connector_init_expectAndReturn(connector_callback_t const callback, connector_handle_t retval, void * const context);

void Mock_connector_run_create(void);
void Mock_connector_run_destroy(void);

void Mock_connector_initiate_action_create(void);
void Mock_connector_initiate_action_destroy(void);
void Mock_connector_initiate_action_expectAndReturn(connector_handle_t handle, connector_initiate_request_t request, void * request_data, connector_status_t retval);

#endif /* MOCK_CONNECTOR_API_H_ */
