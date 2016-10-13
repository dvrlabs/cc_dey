/*
 * ccimp_hal.c
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
 * Description: Cloud Connector hal implementation.
 *
 */

#include "ccimp/ccimp_hal.h"

#include <stdio.h>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#if (defined UNIT_TEST)
#define ccimp_hal_halt			ccimp_hal_halt_real
#define ccimp_hal_reset			ccimp_hal_reset_real
#endif

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/

#if (defined CCIMP_DEBUG_ENABLED)
ccimp_status_t ccimp_hal_halt(void)
{
	printf("ccimp_hal_halt!!!!\n");

	assert(0);

	/* Should not get here */

	return CCIMP_STATUS_OK;
}
#endif

ccimp_status_t ccimp_hal_reset(void)
{
	printf("ccimp_hal_reset!!!!\n");

	/* Note: we must be running as the superuser to reboot the system */
	sync();
	reboot(LINUX_REBOOT_CMD_RESTART);

	/* Should not get here */
	return CCIMP_STATUS_OK;
}

