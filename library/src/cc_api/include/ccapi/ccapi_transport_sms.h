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

#ifndef _CCAPI_TRANSPORT_SMS_H_
#define _CCAPI_TRANSPORT_SMS_H_

#define CCAPI_SM_SMS_MAX_SESSIONS_LIMIT 256
#define CCAPI_SM_SMS_MAX_SESSIONS_DEFAULT 20

#define CCAPI_SMS_START_WAIT_FOREVER ((uint8_t) 0)
#define CCAPI_SMS_RX_TIMEOUT_INFINITE ((size_t) 0)

typedef enum {
    CCAPI_SMS_START_ERROR_NONE,
    CCAPI_SMS_START_ERROR_ALREADY_STARTED,
    CCAPI_SMS_START_ERROR_CCAPI_STOPPED,
    CCAPI_SMS_START_ERROR_NULL_POINTER,
    CCAPI_SMS_START_ERROR_INIT,
    CCAPI_SMS_START_ERROR_MAX_SESSIONS,
    CCAPI_SMS_START_ERROR_INVALID_PHONE,
    CCAPI_SMS_START_ERROR_INVALID_SERVICE_ID,
    CCAPI_SMS_START_ERROR_INSUFFICIENT_MEMORY,
    CCAPI_SMS_START_ERROR_TIMEOUT
} ccapi_sms_start_error_t;

typedef enum {
    CCAPI_SMS_STOP_ERROR_NONE,
    CCAPI_SMS_STOP_ERROR_NOT_STARTED,
    CCAPI_SMS_STOP_ERROR_CCFSM
} ccapi_sms_stop_error_t;

typedef enum {
    CCAPI_SMS_CLOSE_DISCONNECTED,
    CCAPI_SMS_CLOSE_DATA_ERROR
} ccapi_sms_close_cause_t;

typedef ccapi_bool_t (* ccapi_sms_close_cb_t)(ccapi_sms_close_cause_t cause);

typedef struct {
    struct {
        ccapi_sms_close_cb_t close;
    } callback;
    uint8_t start_timeout;
    struct {
       char const * phone_number;
       char const * service_id;
    } cloud_config;
    struct {
        size_t max_sessions;
        size_t rx_timeout;
    } limit;
} ccapi_sms_info_t;


typedef struct {
    ccapi_transport_stop_t behavior;
} ccapi_sms_stop_t;


ccapi_sms_start_error_t ccapi_start_transport_sms(ccapi_sms_info_t const * const sms_start);
ccapi_sms_stop_error_t ccapi_stop_transport_sms(ccapi_sms_stop_t const * const sms_stop);

#endif
