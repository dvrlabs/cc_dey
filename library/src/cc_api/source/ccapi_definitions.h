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

#if !(defined _CCAPI_DEFINITIONS_H_)
#define _CCAPI_DEFINITIONS_H_

#include <ctype.h>

#include "connector_config.h"
#include "ccapi/ccapi.h"
#include "ccimp/ccimp_hal.h"
#include "ccimp/ccimp_logging.h"
#include "ccimp/ccimp_os.h"
#include "ccimp/ccimp_network.h"
#include "ccimp/ccimp_filesystem.h"
#include "connector_api.h"

#define UNUSED_ARGUMENT(a)  (void)(a)
#define INVALID_ENUM(type)  ((type) -1)

#define ON_FALSE_DO_(cond, code)        do { if (!(cond)) {code;} } while (0)

#if (defined CCIMP_DEBUG_ENABLED)
#define ASSERT_MSG_GOTO(cond, label)   ON_FALSE_DO_((cond), \
                                           { \
                                               ccapi_logging_line(TMP_FATAL_PREFIX "Following condition '%s' failed in file '%s' line %d", #cond, __FILE__, __LINE__); \
                                               ccimp_hal_halt(); \
                                               goto label; \
                                           })
#define ASSERT_MSG(cond)               ON_FALSE_DO_((cond), \
                                           { \
                                               ccapi_logging_line(TMP_FATAL_PREFIX "Following condition '%s' failed in file '%s' line %d", #cond, __FILE__, __LINE__); \
                                               ccimp_hal_halt(); \
                                           })
#else
#define ASSERT_MSG_GOTO(cond, label)   ON_FALSE_DO_((cond), {goto label;})
#define ASSERT_MSG(cond)
#endif

#define reset_heap_ptr(pp) do { if (*(pp) != NULL) { ccapi_free(*(pp)); *(pp) = NULL; } } while (0)
#define CCAPI_BOOL(v)   (!!(v) ? CCAPI_TRUE : CCAPI_FALSE)
#define CCFSM_BOOL(c)   ((c) ? connector_true : connector_false)
#define CCAPI_RUNNING(c) ((c) != NULL && (c)->thread.connector_run->status == CCAPI_THREAD_RUNNING)

#define CCAPI_BOOL_TO_CONNECTOR_BOOL(a)     ((a) == CCAPI_TRUE ? connector_true : connector_false)
#define CCAPI_MAX_OF(a, b)          ((a) > (b) ? (a) : (b))
#define CCAPI_MIN_OF(a, b)          ((a) < (b) ? (a) : (b))
#define CCAPI_FS_DIR_SEPARATOR      '/'
#define CCAPI_FS_ROOT_PATH          "/"

#if !(defined CCIMP_IDLE_SLEEP_TIME_MS)
#define CCIMP_IDLE_SLEEP_TIME_MS 100
#endif

typedef struct {
    char * device_type;
    char * device_cloud_url;
    uint8_t device_id[16];
    uint32_t vendor_id;
    ccapi_bool_t cli_supported;
    ccapi_bool_t sm_supported;
    ccapi_bool_t receive_supported;
    ccapi_bool_t firmware_supported;
    ccapi_bool_t rci_supported;
    ccapi_bool_t filesystem_supported;
    ccapi_bool_t streaming_cli_supported;
	ccapi_bool_t sm_key_distribution_supported;
    ccapi_status_callback_t status_callback;
} ccapi_config_t;

typedef enum {
    CCAPI_THREAD_NOT_STARTED,
    CCAPI_THREAD_REQUEST_START,
    CCAPI_THREAD_RUNNING,
    CCAPI_THREAD_REQUEST_STOP
} ccapi_thread_status_t;

typedef struct {
    ccimp_os_create_thread_info_t ccimp_info;
    ccapi_thread_status_t status;
    void * lock;
} ccapi_thread_info_t;

typedef struct ccapi_fs_virtual_dir {
    char * virtual_dir;
    char * local_dir;
    size_t virtual_dir_length;
    size_t local_dir_length;
    struct ccapi_fs_virtual_dir * next;
} ccapi_fs_virtual_dir_t;

#if (defined CCIMP_DATA_SERVICE_ENABLED)
typedef struct ccapi_receive_target
{
    char * target;
    struct {
        ccapi_receive_data_cb_t data;
        ccapi_receive_status_cb_t status;
    } user_callback;
    size_t max_request_size;
    struct ccapi_receive_target * next;
} ccapi_receive_target_t;

typedef enum {
    CCAPI_RECEIVE_THREAD_IDLE,
    CCAPI_RECEIVE_THREAD_DATA_CB_READY,
    CCAPI_RECEIVE_THREAD_DATA_CB_QUEUED,
    CCAPI_RECEIVE_THREAD_DATA_CB_PROCESSED,
    CCAPI_RECEIVE_THREAD_FREE
} ccapi_receive_thread_status_t;

typedef struct
{
    char * target;
    connector_transport_t transport;
    ccapi_bool_t response_required;
    struct {
        ccapi_receive_data_cb_t data;
        ccapi_receive_status_cb_t status;
    } user_callback;
    size_t max_request_size;
    ccapi_buffer_info_t request_buffer_info;
    ccapi_buffer_info_t response_buffer_info;
    ccapi_bool_t response_handled_internally;
    ccapi_buffer_info_t response_processing;
    ccapi_receive_error_t receive_error;
    ccapi_receive_thread_status_t receive_thread_status;
} ccapi_svc_receive_t;
#endif

#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)

/* Pool of chunks allocated to process firmware update */
#if !(defined CCAPI_CHUNK_POOL_SIZE)
#define CCAPI_CHUNK_POOL_SIZE 2
#endif

typedef struct
{
    ccapi_bool_t in_use;
    uint32_t offset;
    uint8_t * data;
    size_t size;
    ccapi_bool_t last;
} ccapi_fw_chunk_info;
#endif

#if (defined CCIMP_RCI_SERVICE_ENABLED)
typedef enum {
    CCAPI_RCI_THREAD_IDLE,
    CCAPI_RCI_THREAD_CB_QUEUED,
    CCAPI_RCI_THREAD_CB_PROCESSED
} ccapi_rci_thread_status_t;
#endif

#if (defined CONNECTOR_SM_CLI)
typedef enum {
    CCAPI_CLI_THREAD_IDLE,
    CCAPI_CLI_THREAD_REQUEST_CB_READY,
    CCAPI_CLI_THREAD_REQUEST_CB_QUEUED,
    CCAPI_CLI_THREAD_REQUEST_CB_PROCESSED,
    CCAPI_CLI_THREAD_FREE
} ccapi_cli_thread_status_t;

typedef struct
{
    connector_transport_t transport;
    ccapi_bool_t response_required;
    ccapi_bool_t more_data;
    ccapi_string_info_t request_string_info;
    ccapi_string_info_t response_string_info;
    ccapi_bool_t response_handled_internally;
    ccapi_string_info_t response_processing;
    ccapi_cli_error_t cli_error;
    ccapi_cli_thread_status_t cli_thread_status;
} ccapi_svc_cli_t;
#endif

typedef struct {
    void * connector_handle;
    ccapi_config_t config;
    struct {
        ccapi_thread_info_t * connector_run;
        ccapi_thread_info_t * rci;
        ccapi_thread_info_t * receive;
        ccapi_thread_info_t * cli;
        ccapi_thread_info_t * firmware;
    } thread;
    void * initiate_action_lock;
    void * file_system_lock;
    struct {
        struct {
            ccapi_filesystem_service_t user_callback;
            ccapi_fs_virtual_dir_t * virtual_dir_list;
            void * imp_context;
        } file_system;
#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
        struct {
            ccapi_fw_service_t config;
            struct {
                ccapi_bool_t update_started;
                unsigned int target;
                uint32_t total_size;
                ccapi_fw_chunk_info chunk_pool[CCAPI_CHUNK_POOL_SIZE];
                uint8_t chunk_pool_head;
                uint8_t chunk_pool_tail;
                uint32_t head_offset;
                uint32_t tail_offset;
                uint32_t ccfsm_bytes_processed;
                ccapi_fw_data_error_t data_error;
            } processing;
        } firmware_update;
#endif
#if (defined CCIMP_DATA_SERVICE_ENABLED)
        struct {
            ccapi_receive_service_t user_callback;
            void * receive_lock;
            ccapi_receive_target_t * target_list;
            ccapi_svc_receive_t * svc_receive;
        } receive;
#endif
#if (defined CCIMP_RCI_SERVICE_ENABLED)
        struct {
            ccapi_rci_data_t const * rci_data;
            ccapi_rci_info_t rci_info;
            ccapi_rci_thread_status_t rci_thread_status;
            struct {
                enum {
                    ccapi_callback_type_none,
                    ccapi_callback_type_base,
                    ccapi_callback_type_lock,
                    ccapi_callback_type_element,
                    ccapi_callback_type_transform
                } type;
                union {
                    struct {
                        ccapi_rci_function_base_t function;
                    } base;
                    struct {
                        ccapi_rci_function_lock_t function;
                        ccapi_response_item_t * item;
                    } lock;
                    struct {
                        ccapi_rci_function_element_t function;
                        ccapi_element_value_t * value;
                    } element;
                    struct {
                        ccapi_rci_function_transform_t function;
                        ccapi_element_value_t * value;
                        char const ** transformed;
                    } transform;
                } as;
                unsigned int error;
#if (defined RCI_ENUMS_AS_STRINGS)
                struct {
                    connector_element_enum_t const * array;
                    unsigned int element_count;
                } enum_data;
#endif
            } callback;
        } rci;
#endif
#if (defined CONNECTOR_SM_CLI)
        struct {
            ccapi_cli_service_t user_callback;
            ccapi_svc_cli_t * svc_cli;
        } cli;
#endif
#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)
        struct {
            ccapi_sm_service_t user_callback;
        } sm;
#endif
#if (defined CCIMP_STREAMING_CLI_SERVICE_ENABLED)
        struct {
            ccapi_streaming_cli_service_t user_callback;
        } streaming_cli;
#endif
    } service;
    struct {
        ccapi_tcp_info_t * info;
        ccapi_bool_t connected;
    } transport_tcp;
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
    struct {
        ccapi_udp_info_t * info;
        ccapi_bool_t started;
    } transport_udp;
#endif
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
    struct {
        ccapi_sms_info_t * info;
        ccapi_bool_t started;
    } transport_sms;
#endif
} ccapi_data_t;

#if (defined CCIMP_DATA_SERVICE_ENABLED)
typedef struct
{
    ccapi_data_t * ccapi_data;
    ccapi_bool_t sending_file;
    void * next_data;
    ccimp_fs_file_handle_t file_handler;
    size_t bytes_remaining;
    void * send_lock;
    ccapi_send_error_t request_error;
    ccapi_send_error_t response_error;
    ccapi_send_error_t status_error;
    ccapi_string_info_t * hint;
} ccapi_svc_send_t;

ccapi_receive_target_t * * get_pointer_to_target_entry(ccapi_data_t * const ccapi_data, char const * const target);
#endif

#if (defined CCIMP_DATA_POINTS_ENABLED)
typedef enum {
    CCAPI_DP_ARG_DATA_INT32,
    CCAPI_DP_ARG_DATA_INT64,
    CCAPI_DP_ARG_DATA_FLOAT,
    CCAPI_DP_ARG_DATA_DOUBLE,
    CCAPI_DP_ARG_DATA_STRING,
    CCAPI_DP_ARG_DATA_JSON,
    CCAPI_DP_ARG_DATA_GEOJSON,
    CCAPI_DP_ARG_TS_EPOCH,
    CCAPI_DP_ARG_TS_EPOCH_MS,
    CCAPI_DP_ARG_TS_ISO8601,
    CCAPI_DP_ARG_LOCATION,
    CCAPI_DP_ARG_QUALITY,
    CCAPI_DP_ARG_INVALID
} ccapi_dp_argument_t;

typedef struct ccapi_dp_data_stream {
    connector_data_stream_t * ccfsm_data_stream;
    struct {
        ccapi_dp_argument_t * list;
        unsigned int count;
    } arguments;
    struct ccapi_dp_data_stream * next;
} ccapi_dp_data_stream_t;

typedef struct stream_seen {
    char * stream_id;
    struct stream_seen * next;
} stream_seen_t;

typedef struct ccapi_dp_collection {
    ccapi_dp_data_stream_t * ccapi_data_stream_list;
    uint32_t dp_count;
    void * lock;
} ccapi_dp_collection_t;

typedef struct {
    void * lock;
    ccapi_string_info_t * hint;
    ccapi_dp_error_t response_error;
    ccapi_dp_error_t status;
} ccapi_dp_transaction_info_t;
#endif

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
typedef enum {
    CCAPI_FS_INTERNAL_ERROR_ACCESS_DENIED,
    CCAPI_FS_INTERNAL_ERROR_INVALID_PATH
} ccapi_fs_internal_error_t;

typedef struct {
    ccapi_bool_t error_is_internal;
    union {
        ccimp_fs_errnum_t ccimp_error;
        ccapi_fs_internal_error_t ccapi_error;
    } error;
} ccapi_fs_error_handle_t;
#endif

#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)
typedef struct
{
    void * ping_lock;
    ccapi_ping_error_t response_error;
} ccapi_svc_ping_t;
#endif

extern ccapi_data_t * ccapi_data_single_instance;
#if (defined CCIMP_DEBUG_ENABLED)
extern unsigned int logging_lock_users;
extern void * logging_lock;
#endif

void ccapi_rci_thread(void * const argument);
void ccapi_receive_thread(void * const argument);
void ccapi_cli_thread(void * const argument);
void ccapi_firmware_thread(void * const argument);
void ccapi_connector_run_thread(void * const argument);
void * ccapi_malloc(size_t size);
#define ccapi_free_const(ptr)   ccapi_free((void *) (ptr))
ccimp_status_t ccapi_free(void * ptr);
char * ccapi_strdup(char const * const string);

void * ccapi_lock_create(void);
void * ccapi_lock_create_and_release(void);
ccimp_status_t ccapi_lock_acquire(void * lock);
ccimp_status_t ccapi_lock_release(void * lock);
ccimp_status_t ccapi_lock_destroy(void * lock);
connector_status_t ccapi_initiate_transport_stop(ccapi_data_t * const ccapi_data, ccapi_transport_t transport, ccapi_transport_stop_t behavior);
ccimp_status_t ccapi_open_file(ccapi_data_t * const ccapi_data, char const * const local_path, int const flags, ccimp_fs_file_handle_t * file_handler);
ccimp_status_t ccapi_read_file(ccapi_data_t * const ccapi_data, ccimp_fs_file_handle_t const file_handler, void * const data, size_t const bytes_available, size_t * const bytes_used);
ccimp_status_t ccapi_close_file(ccapi_data_t * const ccapi_data, ccimp_fs_file_handle_t const file_handler);
connector_transport_t ccapi_to_connector_transport(ccapi_transport_t const ccapi_transport);
ccimp_status_t ccapi_get_dir_entry_status(ccapi_data_t * const ccapi_data, char const * const local_path, ccimp_fs_stat_t * const fs_status);
connector_status_t connector_initiate_action_secure(ccapi_data_t * const ccapi_data, connector_initiate_request_t const request, void const * const request_data);

connector_callback_status_t connector_callback_status_from_ccimp_status(ccimp_status_t const ccimp_status);

connector_callback_status_t ccapi_connector_callback(connector_class_id_t const class_id, connector_request_id_t const request_id, void * const data, void * const context);
ccapi_fs_virtual_dir_t * * get_pointer_to_dir_entry_from_virtual_dir_name(ccapi_data_t * const ccapi_data, char const * const virtual_dir, unsigned int virtual_dir_length);

#if (defined CCIMP_FILE_SYSTEM_SERVICE_ENABLED)
connector_callback_status_t ccapi_filesystem_handler(connector_request_id_file_system_t filesystem_request, void * const data, ccapi_data_t * const ccapi_data);
#endif
#if (defined CCIMP_FIRMWARE_SERVICE_ENABLED)
connector_callback_status_t ccapi_firmware_service_handler(connector_request_id_firmware_t const firmware_service_request, void * const data, ccapi_data_t * const ccapi_data);
#endif
#if (defined CCIMP_DATA_SERVICE_ENABLED)
connector_callback_status_t ccapi_data_service_handler(connector_request_id_data_service_t const data_service_request, void * const data, ccapi_data_t * const ccapi_data);
#endif
#if (defined CCIMP_UDP_TRANSPORT_ENABLED || defined CCIMP_SMS_TRANSPORT_ENABLED)
connector_callback_status_t ccapi_sm_service_handler(connector_request_id_sm_t const sm_service_request, void * const data, ccapi_data_t * const ccapi_data);
#endif
#if (defined CCIMP_RCI_SERVICE_ENABLED)
connector_callback_status_t ccapi_rci_handler(connector_request_id_remote_config_t const request_id, void * const data, ccapi_data_t * const ccapi_data);
#endif
#if (defined CCIMP_STREAMING_CLI_SERVICE_ENABLED)
connector_callback_status_t ccapi_streaming_cli_handler(connector_request_id_streaming_cli_service_t const request, void * const data, ccapi_data_t * const ccapi_data);
#endif

void ccxapi_asynchronous_stop(ccapi_data_t * const ccapi_data);

void free_transport_tcp_info(ccapi_tcp_info_t * const tcp_info);
#if (defined CCIMP_UDP_TRANSPORT_ENABLED)
void free_transport_udp_info(ccapi_udp_info_t * const sms_info);
#endif
#if (defined CCIMP_SMS_TRANSPORT_ENABLED)
void free_transport_sms_info(ccapi_sms_info_t * const sms_info);
#endif

void ccapi_logging_line(char const * const format, ...);
void ccapi_logging_print_buffer(char const * const label, void const * const buffer, size_t const length);

#endif
