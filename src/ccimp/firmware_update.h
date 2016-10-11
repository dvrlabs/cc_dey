/*
 * firmware_update.h
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
 * Description: Firmware update header.
 *
 */

#ifndef FIRMWARE_UPDATE_H_
#define FIRMWARE_UPDATE_H_

#include "ccapi/ccapi.h"

ccapi_fw_request_error_t app_fw_request_cb(unsigned int const target, char const * const filename, size_t const total_size);
ccapi_fw_data_error_t app_fw_data_cb(unsigned int const target, uint32_t offset, void const * const data, size_t size, ccapi_bool_t last_chunk);
void app_fw_cancel_cb(unsigned int const target, ccapi_fw_cancel_error_t cancel_reason);
void app_fw_reset_cb(unsigned int const target, ccapi_bool_t * system_reset, ccapi_firmware_target_version_t * version);

#endif /* FIRMWARE_UPDATE_H_ */
