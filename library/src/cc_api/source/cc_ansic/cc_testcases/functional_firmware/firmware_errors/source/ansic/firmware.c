/*
 * Copyright (c) 2013 Digi International Inc.
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
#include "connector_api.h"
#include "platform.h"



typedef struct {
    connector_firmware_version_t    version;
    char        * filespec;
    char        * description;
} firmware_list_t;

/* list of all supported firmware target info */
static firmware_list_t firmware_list[] = {
    /* version       name_spec          description */
    {{1,0,0,0}, ".*\\.[bB][iI][nN]", "Binary Image" },  /* any *.bin files */
    {{0,0,1,0}, ".*\\.a",            "Library Image"},   /* any *.a files */
    {{1,0,0,0}, ".*\\.txt",            "Text file"},   /* any *.txt files */
};

static int firmware_download_started = 0;
static size_t total_image_size = 0;

/******** Modified for testing firmware callback ********/
static connector_firmware_status_t test_case;
/******** End of Modifications for testing firmware callback ********/



static connector_callback_status_t app_firmware_target_count(connector_firmware_count_t * const target_info)
{
    connector_callback_status_t status = connector_callback_continue;

    target_info->count = ARRAY_SIZE(firmware_list);

    return status;
}

static connector_callback_status_t app_firmware_target_info(connector_firmware_info_t * const request_info)
{
    connector_callback_status_t status = connector_callback_continue;
    firmware_list_t * firmware_info;

    ASSERT(request_info->target_number <= ARRAY_SIZE(firmware_list));

    firmware_info = &firmware_list[request_info->target_number];

    request_info->version.major = firmware_info->version.major;
    request_info->version.minor = firmware_info->version.minor;
    request_info->version.revision = firmware_info->version.revision;
    request_info->version.build = firmware_info->version.build;

    request_info->description = firmware_info->description;
    request_info->filespec = firmware_info->filespec;

    return status;
}

static connector_callback_status_t app_firmware_download_request(connector_firmware_download_start_t * const download_info)
{
    connector_callback_status_t status = connector_callback_continue;

    if (firmware_download_started)
    {
        download_info->status = connector_firmware_status_device_error;
        goto done;
    }

    APP_DEBUG("target = %d\n",         download_info->target_number);
    APP_DEBUG("filename = %s\n",       download_info->filename);
    APP_DEBUG("code size = %d\n",      download_info->code_size);


    /******** Modified for testing firmware callback ********/


    /* Return an error for each test case */
    if( strcmp(download_info->filename, "test_01_firmware_error_download_denied.txt") == 0)
    {
        download_info->status = connector_firmware_status_download_denied;
    }
    else if( strcmp(download_info->filename, "test_02_firmware_error_download_invalid_size.txt") == 0)
    {
        download_info->status = connector_firmware_status_download_invalid_size;
    }
    else if( strcmp(download_info->filename, "test_03_firmware_error_download_invalid_version.txt") == 0)
    {
        download_info->status = connector_firmware_status_download_invalid_version;
    }
    else if( strcmp(download_info->filename, "test_04_firmware_error_download_unauthenticated.txt") == 0)
    {
        download_info->status = connector_firmware_status_download_unauthenticated;
    }
    else if( strcmp(download_info->filename, "test_05_firmware_error_download_not_allowed.txt") == 0)
    {
        download_info->status = connector_firmware_status_download_not_allowed;
    }
    else if( strcmp(download_info->filename, "test_06_firmware_error_download_configured_to_reject.txt") == 0)
    {
        download_info->status = connector_firmware_status_download_configured_to_reject;
    }
    else if( strcmp(download_info->filename, "test_07_firmware_error_encountered_error.txt") == 0)
    {
        download_info->status = connector_firmware_status_encountered_error;
    }
    else if( strcmp(download_info->filename, "test_08_firmware_status_device_error.txt") == 0)
    {
        download_info->status = connector_firmware_status_device_error;
    }
    else
    {
        firmware_download_started = 1;
    }

    /* These are test cases that we need to start to process, then we save the test in a global var */
    if( strcmp(download_info->filename, "test_09_firmware_status_hardware_error.txt") == 0)
    {
        test_case = connector_firmware_status_hardware_error;
    }
    else if( strcmp(download_info->filename, "test_10_firmware_status_invalid_data.txt") == 0)
    {
        test_case = connector_firmware_status_invalid_data;
    }
    else if( strcmp(download_info->filename, "test_11_firmware_status_invalid_offset.txt") == 0)
    {
        test_case = connector_firmware_status_invalid_offset;
    }
    else if( strcmp(download_info->filename, "test_12_firmware_status_user_abort.txt") == 0)
    {
        test_case = connector_firmware_status_user_abort;
    }

    /******** End of Modifications for testing firmware callback ********/


    total_image_size = 0;


done:
    return status;
}

static connector_callback_status_t app_firmware_image_data(connector_firmware_download_data_t * const image_data)
{
    connector_callback_status_t status = connector_callback_continue;


    if (!firmware_download_started)
    {
        APP_DEBUG("app_firmware_image_data:no firmware download request started\n");
        image_data->status = connector_firmware_status_download_denied;
        goto done;
    }

    {
        size_t const max_bytes_to_print = 4;
        size_t const bytes_to_print = (image_data->image.bytes_used > max_bytes_to_print) ? max_bytes_to_print : image_data->image.bytes_used;
        size_t i;

        APP_DEBUG("target = %d\n", image_data->target_number);
        APP_DEBUG("offset = 0x%" PRIx32 "\n", image_data->image.offset);

        APP_DEBUG("data = ");
        for (i=0; i < bytes_to_print; i++)
        {
            APP_DEBUG("0x%02X ", image_data->image.data[i]);
        }
        APP_DEBUG("...\n");

        total_image_size += image_data->image.bytes_used;
        APP_DEBUG("length = %" PRIsize " (total = %" PRIsize ")\n", image_data->image.bytes_used, total_image_size);



        /******** Modified for testing firmware callback ********/
        switch (test_case)
        {
            case connector_firmware_status_hardware_error:
                /* test_09_firmware_status_hardware_error */
                APP_DEBUG("app_firmware_image_data: test_09_firmware_status_hardware_error\n");
                image_data->status = connector_firmware_status_hardware_error;
                firmware_download_started = 0;
                break;

            case connector_firmware_status_invalid_data:
                /* test_10_firmware_status_invalid_data */
                APP_DEBUG("app_firmware_image_data: test_10_firmware_status_invalid_data\n");
                image_data->status = connector_firmware_status_invalid_data;
                firmware_download_started = 0;
                break;

            case connector_firmware_status_invalid_offset:
                /* test_11_firmware_status_invalid_offset */
                APP_DEBUG("app_firmware_image_data: test_11_firmware_status_invalid_offset\n");
                image_data->status = connector_firmware_status_invalid_offset;
                firmware_download_started = 0;
                break;

            case connector_firmware_status_user_abort:
                /* test_11_firmware_status_invalid_offset */
                APP_DEBUG("app_firmware_image_data: test_12_firmware_status_user_abort\n");
                image_data->status = connector_firmware_status_user_abort;
                firmware_download_started = 0;
                break;

            default:
                ASSERT(connector_false);
                goto done;

        }
        /******** End of Modifications for testing firmware callback ********/

    }

done:
    return status;
}

static connector_callback_status_t app_firmware_download_complete(connector_firmware_download_complete_t * const download_complete)
{
    connector_callback_status_t status = connector_callback_continue;


    if (!firmware_download_started)
    {
        APP_DEBUG("app_firmware_download_complete:no firmware download request started\n");
        download_complete->status = connector_firmware_download_not_complete;
        goto done;
    }

    APP_DEBUG("app_firmware_download_complete: target    = %d\n",    download_complete->target_number);
    download_complete->status = connector_firmware_download_success;

    firmware_download_started = 0;

done:
    return status;
}

static connector_callback_status_t app_firmware_download_abort(connector_firmware_download_abort_t const * const abort_data)
{
    connector_callback_status_t   status = connector_callback_continue;

    /* Device Cloud is aborting firmware update */
    APP_DEBUG("app_firmware_download_abort: target = %d, status = %d\n", abort_data->target_number, abort_data->status);
    firmware_download_started = 0;

    return status;
}

static connector_callback_status_t app_firmware_reset(connector_firmware_reset_t const * const reset_data)
{
    connector_callback_status_t   status = connector_callback_continue;

    UNUSED_ARGUMENT(reset_data);
    /* Device Cloud requests firmware reboot */
    APP_DEBUG("app_firmware_reset\n");

    firmware_download_started = 0;

    return status;
}

connector_callback_status_t app_firmware_handler(connector_request_id_firmware_t const request_id, void * const data)
{
    connector_callback_status_t status = connector_callback_unrecognized;

    switch (request_id)
    {
    case connector_request_id_firmware_target_count:
        status = app_firmware_target_count(data);
        break;

    case connector_request_id_firmware_info:
        status = app_firmware_target_info(data);
        break;

    case connector_request_id_firmware_download_start:
        status = app_firmware_download_request(data);
        break;

    case connector_request_id_firmware_download_data:
        status = app_firmware_image_data(data);
        break;

    case connector_request_id_firmware_download_complete:
        status = app_firmware_download_complete(data);
        break;

    case connector_request_id_firmware_download_abort:
        status =  app_firmware_download_abort(data);
        break;

    case connector_request_id_firmware_target_reset:
        status =  app_firmware_reset(data);
        break;

    }

    return status;
}

