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

#include <stdio.h>
#include <unistd.h>
#include "ccapi/ccapi.h"

#define DEVICE_TYPE      "DEY Device"
#define DEVICE_CLOUD_URL "devicecloud.digi.com"
#define VENDOR_ID        0x03000001

#define DEVICE_ID_LENGTH	16
#define MAC_ADDR_LENGTH		6

const uint8_t ipv4[] = {0xC0, 0xA8, 0x01, 0x01};            /* 192.168.1.1 */
const uint8_t mac[] = {0x00, 0x40, 0x9D, 0x7D, 0xCD, 0x4D}; /* 00049D:ABCDEF */

static ccapi_bool_t stop = CCAPI_FALSE;

static void get_device_id_from_mac(uint8_t * const device_id, uint8_t const * const mac_addr)
{
    memset(device_id, 0x00, DEVICE_ID_LENGTH);
    device_id[8] = mac_addr[0];
    device_id[9] = mac_addr[1];
    device_id[10] = mac_addr[2];
    device_id[11] = 0xFF;
    device_id[12] = 0xFF;
    device_id[13] = mac_addr[3];
    device_id[14] = mac_addr[4];
    device_id[15] = mac_addr[5];
}

void fill_start_structure(ccapi_start_t * start)
{   
    get_device_id_from_mac(start->device_id, mac);
    start->vendor_id = VENDOR_ID;
    start->device_cloud_url = DEVICE_CLOUD_URL;
    start->device_type = DEVICE_TYPE;

    start->status = NULL;

    start->service.cli = NULL;
    start->service.sm = NULL;
    start->service.receive = NULL;
    start->service.file_system = NULL;
    start->service.firmware = NULL;
    start->service.rci = NULL;
}

static ccapi_bool_t ccapi_tcp_close_cb(ccapi_tcp_close_cause_t cause)
{
    ccapi_bool_t reconnect;
    switch (cause)
    {
        case CCAPI_TCP_CLOSE_DISCONNECTED:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_DISCONNECTED\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_REDIRECTED:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_REDIRECTED\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_NO_KEEPALIVE:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_NO_KEEPALIVE\n");
            reconnect = CCAPI_TRUE;
            break;
        case CCAPI_TCP_CLOSE_DATA_ERROR:
            printf("ccapi_tcp_close_cb cause CCAPI_TCP_CLOSE_DATA_ERROR\n");
            reconnect = CCAPI_TRUE;
            break;
    }
    return reconnect;
}

static ccapi_start_error_t app_start_ccapi(void)
{
    ccapi_start_t start = {0};
    ccapi_start_error_t error;

    fill_start_structure(&start);

    error = ccapi_start(&start);
    if (error == CCAPI_START_ERROR_NONE)
    {
        printf("ccapi_start success\n");
    }
    else
    {
        printf("ccapi_start error %d\n", error);
    }

    return error;
}

static ccapi_tcp_start_error_t app_start_tcp_transport(void)
{
    ccapi_tcp_start_error_t tcp_start_error;
    ccapi_tcp_info_t tcp_info = {{0}};

    tcp_info.connection.type = CCAPI_CONNECTION_LAN;
    memcpy(tcp_info.connection.ip.address.ipv4, ipv4, sizeof tcp_info.connection.ip.address.ipv4);
    memcpy(tcp_info.connection.info.lan.mac_address, mac, sizeof tcp_info.connection.info.lan.mac_address);

    tcp_info.callback.close = ccapi_tcp_close_cb;
    tcp_info.callback.keepalive = NULL;
    tcp_start_error = ccapi_start_transport_tcp(&tcp_info);
    if (tcp_start_error == CCAPI_TCP_START_ERROR_NONE)
    {
        printf("ccapi_start_transport_tcp success\n");
    }
    else
    {
        printf("ccapi_start_transport_tcp failed with error %d\n", tcp_start_error);
        goto done;
    }
done:
    return tcp_start_error;
}

static ccapi_bool_t check_stop(void)
{
    ccapi_stop_error_t stop_error = CCAPI_STOP_ERROR_NONE;

    if (stop == CCAPI_TRUE)
    {
        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);

        if (stop_error == CCAPI_STOP_ERROR_NONE)
        {
            printf("ccapi_stop success\n");
        }
        else
        {
            printf("ccapi_stop error %d\n", stop_error);
        }
    }

    return stop;
}

int main (void)
{
    if (app_start_ccapi() != CCAPI_START_ERROR_NONE)
    {
        goto done;
    }

    if (app_start_tcp_transport() != CCAPI_TCP_START_ERROR_NONE)
    {
        goto done;
    }

    printf("Waiting for ever\n");
    do
    {
        sleep(1);
    } while (check_stop() != CCAPI_TRUE);

done:
    return 0;
}

