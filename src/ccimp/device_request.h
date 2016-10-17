/*
 * device_request.h
 *
 * Copyright (C) 2016 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi.
 * International Inc. By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered. This is an unpublished copyrighted work of Digi International
 * Inc. Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to restrictions
 * set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
 * Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and
 * (2) of the Commercial Computer Software - Restricted Rights at 48 CFR
 * 52.227-19, as applicable.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 *
 * Description: Device request header.
 *
 */

#ifndef DEVICE_REQUEST_H_
#define DEVICE_REQUEST_H_

#include "ccapi/ccapi.h"

ccapi_bool_t app_receive_default_accept_cb(char const * const target, ccapi_transport_t const transport);
void app_receive_default_data_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t const * const request_buffer_info, ccapi_buffer_info_t * const response_buffer_info);
void app_receive_default_status_cb(char const * const target, ccapi_transport_t const transport, ccapi_buffer_info_t * const response_buffer_info, ccapi_receive_error_t receive_error);

#endif /* DEVICE_REQUEST_H_ */
