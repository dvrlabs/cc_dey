/*****************************************************************************
* Copyright (c) 2016 Digi International Inc., All Rights Reserved
*
* This software contains proprietary and confidential information of Digi
* International Inc.  By accepting transfer of this copy, Recipient agrees
* to retain this software in confidence, to prevent disclosure to others,
* and to make no use of this software other than that for which it was
* delivered.  This is an unpublished copyrighted work of Digi International
* Inc.  Except as permitted by federal law, 17 USC 117, copying is strictly
* prohibited.
*
* Restricted Rights Legend
*
* Use, duplication, or disclosure by the Government is subject to
* restrictions set forth in sub-paragraph (c)(1)(ii) of The Rights in
* Technical Data and Computer Software clause at DFARS 252.227-7031 or
* subparagraphs (c)(1) and (2) of the Commercial Computer Software -
* Restricted Rights at 48 CFR 52.227-19, as applicable.
*
* Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
*
****************************************************************************/

#ifndef _CUSTOM_CONNECTOR_CONFIG_H_
#define _CUSTOM_CONNECTOR_CONFIG_H_

/* Cloud Connector Configuration Categories */

/* Transports */
#undef  CCIMP_UDP_TRANSPORT_ENABLED
#undef  CCIMP_SMS_TRANSPORT_ENABLED

/* Services */
#define CCIMP_DATA_SERVICE_ENABLED
#define CCIMP_DATA_POINTS_ENABLED
#undef CCIMP_RCI_SERVICE_ENABLED
#undef CCIMP_FIRMWARE_SERVICE_ENABLED
#define CCIMP_FILE_SYSTEM_SERVICE_ENABLED

/* OS Features */
#define CCIMP_LITTLE_ENDIAN
#define CCIMP_COMPRESSION_ENABLED
#define CCIMP_64_BIT_INTEGERS_SUPPORTED
#define CCIMP_FLOATING_POINT_SUPPORTED

#define CCIMP_HAS_STDINT_HEADER

/* Debugging (Logging / Halt) */
#define CCIMP_DEBUG_ENABLED

/* Limits */
#define CCIMP_FILE_SYSTEM_MAX_PATH_LENGTH   256
#undef  CCIMP_FILE_SYSTEM_LARGE_FILES_SUPPORTED

#define CCIMP_SM_UDP_MAX_RX_SEGMENTS   256
#define CCIMP_SM_SMS_MAX_RX_SEGMENTS   256

#define CCIMP_IDLE_SLEEP_TIME_MS 100

#endif /* _CUSTOM_CONNECTOR_CONFIG_H_ */
