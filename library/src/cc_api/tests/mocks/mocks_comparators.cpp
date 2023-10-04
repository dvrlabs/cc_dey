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

#define ValueToStringFunction(type)     static SimpleString type##_ValueToString(void const * const object) {(void)object; return #type;}
#define compare_pointers(object1, object2)  do {if (object1 == NULL || object2 == NULL) return false; else if (object1 == object2) return true;} while(0)
#define compare_strings(string1, string2)   do {if (string1 != string2 && (string1 == NULL || string2 == NULL)) return false;\
                                                else if (strcmp(string1, string2) != 0) return false;                          } while(0)

ValueToStringFunction(ccimp_network_open_t)
ValueToStringFunction(ccimp_network_send_t)
ValueToStringFunction(ccimp_network_receive_t)
ValueToStringFunction(ccimp_network_close_t)
ValueToStringFunction(connector_transport_t)
ValueToStringFunction(connector_initiate_stop_request_t)
ValueToStringFunction(connector_request_data_service_send_t)
ValueToStringFunction(connector_sm_send_ping_request_t)
ValueToStringFunction(ccimp_fs_file_open_t)
ValueToStringFunction(ccimp_fs_file_read_t)
ValueToStringFunction(ccimp_fs_file_write_t)
ValueToStringFunction(ccimp_fs_file_seek_t)
ValueToStringFunction(ccimp_fs_file_close_t)
ValueToStringFunction(ccimp_fs_file_truncate_t)
ValueToStringFunction(ccimp_fs_file_remove_t)
ValueToStringFunction(ccimp_fs_dir_open_t)
ValueToStringFunction(ccimp_fs_dir_read_entry_t)
ValueToStringFunction(ccimp_fs_dir_entry_status_t)
ValueToStringFunction(ccimp_fs_dir_close_t)
ValueToStringFunction(ccimp_fs_get_hash_alg_t)
ValueToStringFunction(ccimp_fs_hash_file_t)
ValueToStringFunction(ccimp_fs_error_desc_t)
ValueToStringFunction(ccimp_fs_session_error_t)
ValueToStringFunction(connector_request_data_point_t)

static SimpleString ccimp_create_thread_info_t_ValueToString(void const * const object)
{
    return StringFrom(((ccimp_os_create_thread_info_t*)object)->type);
}

static bool ccimp_network_open_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_network_open_t * open_data_1 = (ccimp_network_open_t*)object1;
    ccimp_network_open_t * open_data_2 = (ccimp_network_open_t*)object2;

    compare_pointers(object1, object2);
    compare_strings(open_data_1->device_cloud.url, open_data_2->device_cloud.url);

    if (open_data_1->handle != open_data_2->handle)
        return false;

    return true;
}

static bool ccimp_network_send_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_network_send_t * send_data_1 = (ccimp_network_send_t*)object1;
    ccimp_network_send_t * send_data_2 = (ccimp_network_send_t*)object2;

    compare_pointers(object1, object2);

    if (send_data_1->bytes_available != send_data_2->bytes_available)
        return false;

    if (send_data_1->bytes_used != send_data_2->bytes_used)
        return false;

    if (send_data_1->handle != send_data_2->handle)
        return false;

    if (memcmp(send_data_1->buffer, send_data_2->buffer, send_data_1->bytes_available) != 0)
        return false;

    return true;
}

static bool ccimp_network_receive_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_network_receive_t * receive_data_1 = (ccimp_network_receive_t*)object1;
    ccimp_network_receive_t * receive_data_2 = (ccimp_network_receive_t*)object2;

    compare_pointers(object1, object2);

    if (receive_data_1->bytes_available != receive_data_2->bytes_available)
        return false;

    if (receive_data_1->bytes_used != receive_data_2->bytes_used)
        return false;

    if (receive_data_1->handle != receive_data_2->handle)
        return false;

    if (memcmp(receive_data_1->buffer, receive_data_2->buffer, receive_data_1->bytes_available) != 0)
        return false;

    return true;
}

static bool ccimp_network_close_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_network_close_t * close_data_1 = (ccimp_network_close_t*)object1;
    ccimp_network_close_t * close_data_2 = (ccimp_network_close_t*)object2;

    compare_pointers(object1, object2);
    if (close_data_1->handle != close_data_2->handle)
        return false;

    return true;
}

static bool ccimp_create_thread_info_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_os_create_thread_info_t * create_thread_info1 = (ccimp_os_create_thread_info_t*)object1;
    ccimp_os_create_thread_info_t * create_thread_info2 = (ccimp_os_create_thread_info_t*)object2;

    compare_pointers(object1, object2);
    if (create_thread_info1->argument != create_thread_info2->argument)
        return false;
    if (create_thread_info1->type != create_thread_info2->type)
        return false;

    return true;
}

static bool connector_transport_t_IsEqual(void const * const object1, void const * const object2)
{
    connector_transport_t * connector_transport_1 = (connector_transport_t *)object1;
    connector_transport_t * connector_transport_2 = (connector_transport_t *)object2;

    compare_pointers(object1, object2);

    if (*connector_transport_1 != *connector_transport_2)
        return false;

    return true;
}

static bool connector_initiate_stop_request_t_IsEqual(void const * const object1, void const * const object2)
{
    connector_initiate_stop_request_t * connector_initiate_stop_request_1 = (connector_initiate_stop_request_t *)object1;
    connector_initiate_stop_request_t * connector_initiate_stop_request_2 = (connector_initiate_stop_request_t *)object2;

    compare_pointers(object1, object2);
    if (connector_initiate_stop_request_1->condition != connector_initiate_stop_request_2->condition)
        return false;

    if (connector_initiate_stop_request_1->transport != connector_initiate_stop_request_2->transport)
        return false;

    if (connector_initiate_stop_request_1->user_context != connector_initiate_stop_request_2->user_context)
        return false;

    return true;
}

static bool connector_request_data_service_send_t_IsEqual(void const * const object1, void const * const object2)
{
    connector_request_data_service_send_t * connector_request_data_service_send_t_1 = (connector_request_data_service_send_t *)object1;
    connector_request_data_service_send_t * connector_request_data_service_send_t_2 = (connector_request_data_service_send_t *)object2;

    compare_pointers(object1, object2);

    if (connector_request_data_service_send_t_1->transport != connector_request_data_service_send_t_2->transport)
        return false;
    if (strcmp(connector_request_data_service_send_t_1->path, connector_request_data_service_send_t_2->path) != 0)
        return false;
    if (connector_request_data_service_send_t_1->content_type != connector_request_data_service_send_t_2->content_type)
        return false;
    if (connector_request_data_service_send_t_1->option != connector_request_data_service_send_t_2->option)
        return false;
    if (connector_request_data_service_send_t_1->response_required != connector_request_data_service_send_t_2->response_required)
        return false;
    if (connector_request_data_service_send_t_1->timeout_in_seconds != connector_request_data_service_send_t_2->timeout_in_seconds)
        return false;
    if (connector_request_data_service_send_t_1->request_id != connector_request_data_service_send_t_2->request_id)
        return false;

    return true;
}

static bool connector_sm_send_ping_request_t_IsEqual(void const * const object1, void const * const object2)
{
    connector_sm_send_ping_request_t * connector_sm_send_ping_request_t_1 = (connector_sm_send_ping_request_t *)object1;
    connector_sm_send_ping_request_t * connector_sm_send_ping_request_t_2 = (connector_sm_send_ping_request_t *)object2;

    compare_pointers(object1, object2);

    if (connector_sm_send_ping_request_t_1->transport != connector_sm_send_ping_request_t_2->transport)
        return false;
    if (connector_sm_send_ping_request_t_1->response_required != connector_sm_send_ping_request_t_2->response_required)
        return false;
    if (connector_sm_send_ping_request_t_1->timeout_in_seconds != connector_sm_send_ping_request_t_2->timeout_in_seconds)
        return false;
    if (connector_sm_send_ping_request_t_1->request_id != connector_sm_send_ping_request_t_2->request_id)
        return false;

    return true;
}

static bool ccimp_fs_file_open_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_open_t * ccimp_fs_file_open_1 = (ccimp_fs_file_open_t *)object1;
    ccimp_fs_file_open_t * ccimp_fs_file_open_2 = (ccimp_fs_file_open_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_open_1->errnum != ccimp_fs_file_open_2->errnum)
        return false;
    if (ccimp_fs_file_open_1->imp_context != ccimp_fs_file_open_2->imp_context)
        return false;
    if (ccimp_fs_file_open_1->flags != ccimp_fs_file_open_2->flags)
        return false;
    if (ccimp_fs_file_open_1->handle != ccimp_fs_file_open_2->handle)
        return false;
    if (strcmp(ccimp_fs_file_open_1->path, ccimp_fs_file_open_2->path) != 0)
        return false;
    return true;
}

static bool ccimp_fs_file_read_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_read_t * ccimp_fs_file_read_1 = (ccimp_fs_file_read_t *)object1;
    ccimp_fs_file_read_t * ccimp_fs_file_read_2 = (ccimp_fs_file_read_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_read_1->errnum != ccimp_fs_file_read_2->errnum)
        return false;
    if (ccimp_fs_file_read_1->imp_context != ccimp_fs_file_read_2->imp_context)
        return false;
    if (ccimp_fs_file_read_1->handle != ccimp_fs_file_read_2->handle)
        return false;
    if (ccimp_fs_file_read_1->bytes_available != ccimp_fs_file_read_2->bytes_available)
        return false;
    if (ccimp_fs_file_read_1->bytes_used != ccimp_fs_file_read_2->bytes_used)
        return false;

    return true;
}

static bool ccimp_fs_file_write_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_write_t * ccimp_fs_file_write_1 = (ccimp_fs_file_write_t *)object1;
    ccimp_fs_file_write_t * ccimp_fs_file_write_2 = (ccimp_fs_file_write_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_write_1->errnum != ccimp_fs_file_write_2->errnum)
        return false;
    if (ccimp_fs_file_write_1->imp_context != ccimp_fs_file_write_2->imp_context)
        return false;
    if (ccimp_fs_file_write_1->handle != ccimp_fs_file_write_2->handle)
        return false;
    if (ccimp_fs_file_write_1->buffer != ccimp_fs_file_write_2->buffer)
        return false;
    if (ccimp_fs_file_write_1->bytes_available != ccimp_fs_file_write_2->bytes_available)
        return false;
    if (ccimp_fs_file_write_1->bytes_used != ccimp_fs_file_write_2->bytes_used)
        return false;

    return true;
}

static bool ccimp_fs_file_seek_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_seek_t * ccimp_fs_file_seek_1 = (ccimp_fs_file_seek_t *)object1;
    ccimp_fs_file_seek_t * ccimp_fs_file_seek_2 = (ccimp_fs_file_seek_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_seek_1->errnum != ccimp_fs_file_seek_2->errnum)
        return false;
    if (ccimp_fs_file_seek_1->imp_context != ccimp_fs_file_seek_2->imp_context)
        return false;
    if (ccimp_fs_file_seek_1->handle != ccimp_fs_file_seek_2->handle)
        return false;
    if (ccimp_fs_file_seek_1->requested_offset != ccimp_fs_file_seek_2->requested_offset)
        return false;
    if (ccimp_fs_file_seek_1->resulting_offset != ccimp_fs_file_seek_2->resulting_offset)
        return false;
    if (ccimp_fs_file_seek_1->origin != ccimp_fs_file_seek_2->origin)
        return false;

    return true;
}

static bool ccimp_fs_file_close_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_close_t * ccimp_fs_file_close_1 = (ccimp_fs_file_close_t *)object1;
    ccimp_fs_file_close_t * ccimp_fs_file_close_2 = (ccimp_fs_file_close_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_close_1->errnum != ccimp_fs_file_close_2->errnum)
        return false;
    if (ccimp_fs_file_close_1->imp_context != ccimp_fs_file_close_2->imp_context)
        return false;
    if (ccimp_fs_file_close_1->handle != ccimp_fs_file_close_2->handle)
        return false;

    return true;
}

static bool ccimp_fs_file_truncate_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_truncate_t * ccimp_fs_file_truncate_1 = (ccimp_fs_file_truncate_t *)object1;
    ccimp_fs_file_truncate_t * ccimp_fs_file_truncate_2 = (ccimp_fs_file_truncate_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_truncate_1->errnum != ccimp_fs_file_truncate_2->errnum)
        return false;
    if (ccimp_fs_file_truncate_1->imp_context != ccimp_fs_file_truncate_2->imp_context)
        return false;
    if (ccimp_fs_file_truncate_1->handle != ccimp_fs_file_truncate_2->handle)
        return false;
    if (ccimp_fs_file_truncate_1->length_in_bytes != ccimp_fs_file_truncate_2->length_in_bytes)
        return false;

    return true;
}

static bool ccimp_fs_file_remove_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_file_remove_t * ccimp_fs_file_remove_1 = (ccimp_fs_file_remove_t *)object1;
    ccimp_fs_file_remove_t * ccimp_fs_file_remove_2 = (ccimp_fs_file_remove_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_file_remove_1->errnum != ccimp_fs_file_remove_2->errnum)
        return false;
    if (ccimp_fs_file_remove_1->imp_context != ccimp_fs_file_remove_2->imp_context)
        return false;
    if (strcmp(ccimp_fs_file_remove_1->path, ccimp_fs_file_remove_2->path) != 0)
        return false;

    return true;
}

static bool ccimp_fs_dir_open_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_dir_open_t * ccimp_fs_dir_open_1 = (ccimp_fs_dir_open_t *)object1;
    ccimp_fs_dir_open_t * ccimp_fs_dir_open_2 = (ccimp_fs_dir_open_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_dir_open_1->errnum != ccimp_fs_dir_open_2->errnum)
        return false;
    if (ccimp_fs_dir_open_1->imp_context != ccimp_fs_dir_open_2->imp_context)
        return false;
    if (strcmp(ccimp_fs_dir_open_1->path, ccimp_fs_dir_open_2->path) != 0)
        return false;
    if (ccimp_fs_dir_open_1->handle != ccimp_fs_dir_open_2->handle)
        return false;

    return true;
}

static bool ccimp_fs_dir_read_entry_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_dir_read_entry_t * ccimp_fs_dir_read_entry_1 = (ccimp_fs_dir_read_entry_t *)object1;
    ccimp_fs_dir_read_entry_t * ccimp_fs_dir_read_entry_2 = (ccimp_fs_dir_read_entry_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_dir_read_entry_1->errnum != ccimp_fs_dir_read_entry_2->errnum)
        return false;
    if (ccimp_fs_dir_read_entry_1->imp_context != ccimp_fs_dir_read_entry_2->imp_context)
        return false;
    if (ccimp_fs_dir_read_entry_1->handle != ccimp_fs_dir_read_entry_2->handle)
        return false;
    if (ccimp_fs_dir_read_entry_1->entry_name != ccimp_fs_dir_read_entry_2->entry_name)
        return false;
    if (ccimp_fs_dir_read_entry_1->bytes_available != ccimp_fs_dir_read_entry_2->bytes_available)
        return false;
    return true;
}

static bool ccimp_fs_dir_entry_status_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_dir_entry_status_t * ccimp_fs_dir_entry_status_1 = (ccimp_fs_dir_entry_status_t *)object1;
    ccimp_fs_dir_entry_status_t * ccimp_fs_dir_entry_status_2 = (ccimp_fs_dir_entry_status_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_dir_entry_status_1->errnum != ccimp_fs_dir_entry_status_2->errnum)
        return false;
    if (ccimp_fs_dir_entry_status_1->imp_context != ccimp_fs_dir_entry_status_2->imp_context)
        return false;
    if (strcmp(ccimp_fs_dir_entry_status_1->path, ccimp_fs_dir_entry_status_2->path) != 0)
        return false;
    if (ccimp_fs_dir_entry_status_1->status.file_size != ccimp_fs_dir_entry_status_2->status.file_size)
        return false;
    if (ccimp_fs_dir_entry_status_1->status.last_modified != ccimp_fs_dir_entry_status_2->status.last_modified)
        return false;
    if (ccimp_fs_dir_entry_status_1->status.type != ccimp_fs_dir_entry_status_2->status.type)
        return false;
    return true;
}

static bool ccimp_fs_dir_close_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_dir_close_t * ccimp_fs_dir_close_1 = (ccimp_fs_dir_close_t *)object1;
    ccimp_fs_dir_close_t * ccimp_fs_dir_close_2 = (ccimp_fs_dir_close_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_dir_close_1->errnum != ccimp_fs_dir_close_2->errnum)
        return false;
    if (ccimp_fs_dir_close_1->imp_context != ccimp_fs_dir_close_2->imp_context)
        return false;
    if (ccimp_fs_dir_close_1->handle != ccimp_fs_dir_close_2->handle)
        return false;
    return true;
}

static bool ccimp_fs_get_hash_alg_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_get_hash_alg_t * ccimp_fs_hash_status_1 = (ccimp_fs_get_hash_alg_t *)object1;
    ccimp_fs_get_hash_alg_t * ccimp_fs_hash_status_2 = (ccimp_fs_get_hash_alg_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_hash_status_1->errnum != ccimp_fs_hash_status_2->errnum)
        return false;
    if (ccimp_fs_hash_status_1->imp_context != ccimp_fs_hash_status_2->imp_context)
        return false;
    if (strcmp(ccimp_fs_hash_status_1->path, ccimp_fs_hash_status_2->path) != 0)
        return false;
    if (ccimp_fs_hash_status_1->hash_alg.actual != ccimp_fs_hash_status_2->hash_alg.actual)
        return false;
    if (ccimp_fs_hash_status_1->hash_alg.requested != ccimp_fs_hash_status_2->hash_alg.requested)
        return false;
    return true;
}

static bool ccimp_fs_hash_file_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_hash_file_t * ccimp_fs_hash_file_1 = (ccimp_fs_hash_file_t *)object1;
    ccimp_fs_hash_file_t * ccimp_fs_hash_file_2 = (ccimp_fs_hash_file_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_hash_file_1->errnum != ccimp_fs_hash_file_2->errnum)
        return false;
    if (ccimp_fs_hash_file_1->imp_context != ccimp_fs_hash_file_2->imp_context)
        return false;
    if (strcmp(ccimp_fs_hash_file_1->path, ccimp_fs_hash_file_2->path) != 0)
        return false;
    if (ccimp_fs_hash_file_1->hash_algorithm != ccimp_fs_hash_file_2->hash_algorithm)
        return false;
    if (ccimp_fs_hash_file_1->hash_value != ccimp_fs_hash_file_2->hash_value)
        return false;
    if (ccimp_fs_hash_file_1->bytes_requested != ccimp_fs_hash_file_2->bytes_requested)
        return false;
    return true;
}

static bool ccimp_fs_error_desc_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_error_desc_t * ccimp_fs_error_desc_1 = (ccimp_fs_error_desc_t *)object1;
    ccimp_fs_error_desc_t * ccimp_fs_error_desc_2 = (ccimp_fs_error_desc_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_error_desc_1->errnum != ccimp_fs_error_desc_2->errnum)
        return false;
    if (ccimp_fs_error_desc_1->imp_context != ccimp_fs_error_desc_2->imp_context)
        return false;
    if (ccimp_fs_error_desc_1->error_string != ccimp_fs_error_desc_2->error_string)
        return false;
    if (ccimp_fs_error_desc_1->bytes_available != ccimp_fs_error_desc_2->bytes_available)
        return false;
    if (ccimp_fs_error_desc_1->bytes_used != ccimp_fs_error_desc_2->bytes_used)
        return false;
    if (ccimp_fs_error_desc_1->error_status != ccimp_fs_error_desc_2->error_status)
        return false;
    return true;
}

static bool ccimp_fs_session_error_t_IsEqual(void const * const object1, void const * const object2)
{
    ccimp_fs_session_error_t * ccimp_fs_session_error_1 = (ccimp_fs_session_error_t *)object1;
    ccimp_fs_session_error_t * ccimp_fs_session_error_2 = (ccimp_fs_session_error_t *)object2;

    compare_pointers(object1, object2);
    if (ccimp_fs_session_error_1->imp_context != ccimp_fs_session_error_2->imp_context)
        return false;
    if (ccimp_fs_session_error_1->session_error != ccimp_fs_session_error_2->session_error)
        return false;
    return true;
}

static bool connector_request_data_point_t_IsEqual(void const * const object1, void const * const object2)
{
    connector_request_data_point_t * connector_request_data_point_1 = (connector_request_data_point_t *)object1;
    connector_request_data_point_t * connector_request_data_point_2 = (connector_request_data_point_t *)object2;

    compare_pointers(object1, object2);

    if (connector_request_data_point_1->request_id != connector_request_data_point_2->request_id)
        return false;
    if (connector_request_data_point_1->response_required != connector_request_data_point_2->response_required)
        return false;
    if (connector_request_data_point_1->stream != connector_request_data_point_2->stream)
        return false;
    if (connector_request_data_point_1->timeout_in_seconds != connector_request_data_point_2->timeout_in_seconds)
        return false;
    if (connector_request_data_point_1->transport != connector_request_data_point_2->transport)
        return false;
    if (connector_request_data_point_1->user_context != connector_request_data_point_2->user_context)
        return false;
    return true;
}

MockFunctionComparator ccimp_network_open_t_comparator(ccimp_network_open_t_IsEqual, ccimp_network_open_t_ValueToString);
MockFunctionComparator ccimp_network_send_t_comparator(ccimp_network_send_t_IsEqual, ccimp_network_send_t_ValueToString);
MockFunctionComparator ccimp_network_receive_t_comparator(ccimp_network_receive_t_IsEqual, ccimp_network_receive_t_ValueToString);
MockFunctionComparator ccimp_network_close_t_comparator(ccimp_network_close_t_IsEqual, ccimp_network_close_t_ValueToString);
MockFunctionComparator ccimp_create_thread_info_t_comparator(ccimp_create_thread_info_t_IsEqual, ccimp_create_thread_info_t_ValueToString);
MockFunctionComparator connector_transport_t_comparator(connector_transport_t_IsEqual, connector_transport_t_ValueToString);
MockFunctionComparator connector_initiate_stop_request_t_comparator(connector_initiate_stop_request_t_IsEqual, connector_initiate_stop_request_t_ValueToString);
MockFunctionComparator connector_request_data_service_send_t_comparator(connector_request_data_service_send_t_IsEqual, connector_request_data_service_send_t_ValueToString);
MockFunctionComparator connector_sm_send_ping_request_t_comparator(connector_sm_send_ping_request_t_IsEqual, connector_sm_send_ping_request_t_ValueToString);
MockFunctionComparator ccimp_fs_file_open_t_comparator(ccimp_fs_file_open_t_IsEqual, ccimp_fs_file_open_t_ValueToString);
MockFunctionComparator ccimp_fs_file_read_t_comparator(ccimp_fs_file_read_t_IsEqual, ccimp_fs_file_read_t_ValueToString);
MockFunctionComparator ccimp_fs_file_write_t_comparator(ccimp_fs_file_write_t_IsEqual, ccimp_fs_file_write_t_ValueToString);
MockFunctionComparator ccimp_fs_file_seek_t_comparator(ccimp_fs_file_seek_t_IsEqual, ccimp_fs_file_seek_t_ValueToString);
MockFunctionComparator ccimp_fs_file_close_t_comparator(ccimp_fs_file_close_t_IsEqual, ccimp_fs_file_close_t_ValueToString);
MockFunctionComparator ccimp_fs_file_truncate_t_comparator(ccimp_fs_file_truncate_t_IsEqual, ccimp_fs_file_truncate_t_ValueToString);
MockFunctionComparator ccimp_fs_file_remove_t_comparator(ccimp_fs_file_remove_t_IsEqual, ccimp_fs_file_remove_t_ValueToString);
MockFunctionComparator ccimp_fs_dir_open_t_comparator(ccimp_fs_dir_open_t_IsEqual, ccimp_fs_dir_open_t_ValueToString);
MockFunctionComparator ccimp_fs_dir_read_entry_t_comparator(ccimp_fs_dir_read_entry_t_IsEqual, ccimp_fs_dir_read_entry_t_ValueToString);
MockFunctionComparator ccimp_fs_dir_entry_status_t_comparator(ccimp_fs_dir_entry_status_t_IsEqual, ccimp_fs_dir_entry_status_t_ValueToString);
MockFunctionComparator ccimp_fs_dir_close_t_comparator(ccimp_fs_dir_close_t_IsEqual, ccimp_fs_dir_close_t_ValueToString);
MockFunctionComparator ccimp_fs_get_hash_alg_t_comparator(ccimp_fs_get_hash_alg_t_IsEqual, ccimp_fs_get_hash_alg_t_ValueToString);
MockFunctionComparator ccimp_fs_hash_file_t_comparator(ccimp_fs_hash_file_t_IsEqual, ccimp_fs_hash_file_t_ValueToString);
MockFunctionComparator ccimp_fs_error_desc_t_comparator(ccimp_fs_error_desc_t_IsEqual, ccimp_fs_error_desc_t_ValueToString);
MockFunctionComparator ccimp_fs_session_error_t_comparator(ccimp_fs_session_error_t_IsEqual, ccimp_fs_session_error_t_ValueToString);
MockFunctionComparator connector_request_data_point_t_comparator(connector_request_data_point_t_IsEqual, connector_request_data_point_t_ValueToString);

