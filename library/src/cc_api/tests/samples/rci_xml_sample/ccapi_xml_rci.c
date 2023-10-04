/*
* Copyright (c) 2016 Digi International Inc.
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

/* TODO:
*         - Split set requests to xml_rci_handler() in groups intead of a complete request (as it's done for queries)
*           to improve the error location.
*/

#include  <stdio.h>
#include  <stdarg.h>

#include  "ccapi_xml_rci.h"
#include  "ccapi_xml_rci_handler.h"

#define xstr(s) str(s)
#define str(s) #s

static char * xml_request_buffer = NULL;
static size_t xml_request_buffer_size = 0;

static char const * xml_query_response_buffer = NULL;
#if (defined RCI_LEGACY_COMMANDS)
static char * do_command_response = NULL;
#endif

static ccapi_global_error_id_t process_xml_error(ccapi_rci_info_t * const info, char const * xml_response_buffer)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;

    char * error_ptr = NULL;

    /* Parse xml_response looking for an 'error' tag */
    error_ptr = strstr(xml_response_buffer, "<error id=");
    if (error_ptr != NULL)
    {
        int scanf_ret;
        unsigned int response_error_id;

        char * response_error_desc = malloc(XML_MAX_ERROR_DESC_LENGTH + 1);
        char * response_error_hint = malloc(XML_MAX_ERROR_HINT_LENGTH + 1);

        error_id = CCAPI_GLOBAL_ERROR_XML_RESPONSE_FAIL;

        if (response_error_desc == NULL || response_error_hint == NULL)
        {
            goto done;
        }

        #define XML_MAX_ERROR_DESC_LENGTH_STR  xstr(XML_MAX_ERROR_DESC_LENGTH)
        #define XML_MAX_ERROR_HINT_LENGTH_STR  xstr(XML_MAX_ERROR_HINT_LENGTH)

        #define XML_ERROR_FORMAT "<error id=\"%u\">%*[^<]<desc>%" XML_MAX_ERROR_DESC_LENGTH_STR "[^<]</desc>%*[^<]<hint>%" XML_MAX_ERROR_HINT_LENGTH_STR "[^<]</hint>"

        scanf_ret = sscanf(error_ptr, XML_ERROR_FORMAT, &response_error_id, response_error_desc, response_error_hint);
        if (scanf_ret > 0)
        {
            static char const brci_error_prefix[] = "XML Error: id='%d', desc='%s', hint='%s'";
            static char response_error[sizeof(brci_error_prefix) + XML_MAX_ERROR_DESC_LENGTH + 1 + XML_MAX_ERROR_HINT_LENGTH + 1];

            sprintf(response_error, brci_error_prefix, response_error_id, response_error_desc, response_error_hint);

            /* Take the xml error 'id' + 'desc' + 'hint' as hint */
            info->error_hint = response_error;

            printf("%s\n", response_error);
        }

        free(response_error_desc);
        free(response_error_hint);
    }

done:
    return error_id;
}

static ccapi_global_error_id_t append_data_to_xml_request_buffer(char const * const data, ...)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;
    va_list arg_list_test;
    va_list arg_list_done;
    size_t new_buffer_len;
    int print_ret_test;

    va_start(arg_list_test, data);
    va_copy(arg_list_done, arg_list_test);

    print_ret_test = vsnprintf(NULL, 0, data, arg_list_test);

    if (xml_request_buffer == NULL)
    {
        new_buffer_len = print_ret_test;

        xml_request_buffer = malloc(new_buffer_len + 1);
    }
    else
    {
        new_buffer_len = xml_request_buffer_size + print_ret_test;

        xml_request_buffer = realloc(xml_request_buffer, new_buffer_len + 1);
    }

    if (xml_request_buffer == NULL)
    {
        assert(0);
        error_id = CCAPI_GLOBAL_ERROR_XML_REQUEST_FAIL;
    }
    else
    {
        int print_ret_done;

        print_ret_done = vsprintf(&xml_request_buffer[xml_request_buffer_size], data, arg_list_done);

        assert(print_ret_test == print_ret_done);

        xml_request_buffer_size = new_buffer_len;
    }

    va_end(arg_list_test);
    va_end(arg_list_done);

    return error_id;
}

static void write_group(ccapi_rci_info_t * const info)
{
    switch (info->group.type)
    {
        case CCAPI_RCI_GROUP_SETTING:
            append_data_to_xml_request_buffer("setting");
            break;
        case CCAPI_RCI_GROUP_STATE:
            append_data_to_xml_request_buffer("state");
            break;
    }

    return;
}

static ccapi_global_error_id_t call_xml_rci_request(ccapi_rci_info_t * const info, char const * * const xml_response_buffer)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;

    xml_rci_request(xml_request_buffer, xml_response_buffer);

    if (xml_request_buffer != NULL)
    {
        free(xml_request_buffer);
        xml_request_buffer = NULL;
        xml_request_buffer_size = 0;
    }

    assert(*xml_response_buffer != NULL);

    if (strlen(*xml_response_buffer) == 0)
    {
        error_id = CCAPI_GLOBAL_ERROR_XML_RESPONSE_FAIL;
        info->error_hint = "empty xml response";

        goto done;
    }

    error_id = process_xml_error(info, *xml_response_buffer);

done:
    return error_id;
}
 
ccapi_global_error_id_t ccapi_xml_rci_action_start(ccapi_rci_info_t * const info)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;

    switch(info->action)
    {
        case CCAPI_RCI_ACTION_SET:
        {
            assert(xml_request_buffer == NULL);
            assert(xml_request_buffer_size == 0);

            append_data_to_xml_request_buffer("<set_");
            write_group(info);
            append_data_to_xml_request_buffer(">");

            break;
        }
        case CCAPI_RCI_ACTION_QUERY:
        {
            break;
        }
#if (defined RCI_LEGACY_COMMANDS)
        case CCAPI_RCI_ACTION_DO_COMMAND:
            do_command_response = malloc(XML_MAX_DO_COMMAND_RESPONSE_LENGTH + 1);
            if (do_command_response == NULL)
            {
                printf("%s: Unable alloc memory to parse do_command response\n", __FUNCTION__);
                error_id = CCAPI_GLOBAL_ERROR_XML_REQUEST_FAIL;
                info->error_hint = "Unable alloc memory to parse do_command response";
                goto done;
            }
            /* Fall through */
        case CCAPI_RCI_ACTION_REBOOT:
        case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
            assert(xml_request_buffer == NULL);
            assert(xml_request_buffer_size == 0);

            break;
#endif
    }

done:
    return error_id;
}

ccapi_global_error_id_t ccapi_xml_rci_action_end(ccapi_rci_info_t * const info)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;

    switch(info->action)
    {
        case CCAPI_RCI_ACTION_SET:
        {
            char const * xml_set_response_buffer = NULL;

            append_data_to_xml_request_buffer("\n</set_");
            write_group(info);
            append_data_to_xml_request_buffer(">\n");

            error_id = call_xml_rci_request(info, &xml_set_response_buffer);

            xml_rci_finished((char *)xml_set_response_buffer);
            break;
        }
        case CCAPI_RCI_ACTION_QUERY:
        {
            break;
        }
#if (defined RCI_LEGACY_COMMANDS)
        case CCAPI_RCI_ACTION_DO_COMMAND:
        {
            if (do_command_response != NULL)
                free(do_command_response);
            break;
        }
        case CCAPI_RCI_ACTION_REBOOT:
        case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
        {
            break;
        }
#endif
    }

    return error_id;
}

int ccapi_xml_rci_group_start(ccapi_rci_info_t * const info)
{
    int error_id = CCAPI_GLOBAL_ERROR_NONE;

    switch(info->action)
    {
        case CCAPI_RCI_ACTION_SET:
        {
            append_data_to_xml_request_buffer("\n   <%s index=\"%d\">\n      ", info->group.name, info->group.instance);    
            break;
        }
        case CCAPI_RCI_ACTION_QUERY:
        {
            assert(xml_request_buffer == NULL);
            assert(xml_request_buffer_size == 0);
            append_data_to_xml_request_buffer("<query_");
            write_group(info);

            if (info->group.type == CCAPI_RCI_GROUP_SETTING)
            {
                static char const * const attribute_source[] = {"current", "stored", "defaults"};
                static char const * const attribute_compare_to[] = {"none", "current", "stored", "defaults"};

                append_data_to_xml_request_buffer(" source=\"%s\"", attribute_source[info->query_setting.attributes.source]);
                append_data_to_xml_request_buffer(" compare_to=\"%s\"", attribute_compare_to[info->query_setting.attributes.compare_to]);
            }
            append_data_to_xml_request_buffer(">");
            append_data_to_xml_request_buffer("\n   <%s index=\"%d\"/>", info->group.name, info->group.instance);    
            append_data_to_xml_request_buffer("\n</query_");
            write_group(info);
            append_data_to_xml_request_buffer(">\n");

            error_id = call_xml_rci_request(info, &xml_query_response_buffer);

            break;
        }
#if (defined RCI_LEGACY_COMMANDS)
        case CCAPI_RCI_ACTION_DO_COMMAND:
        case CCAPI_RCI_ACTION_REBOOT:
        case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
        {
            break;
        }
#endif
    }

    return error_id;
}

int ccapi_xml_rci_group_end(ccapi_rci_info_t * const info)
{
    switch(info->action)
    {
        case CCAPI_RCI_ACTION_SET:
        {
            append_data_to_xml_request_buffer("\n   </%s>", info->group.name);    
            break;
        }
        case CCAPI_RCI_ACTION_QUERY:
        {
            xml_rci_finished((char *)xml_query_response_buffer);

            break;
        }
#if (defined RCI_LEGACY_COMMANDS)
        case CCAPI_RCI_ACTION_DO_COMMAND:
        case CCAPI_RCI_ACTION_REBOOT:
        case CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS:
        {
            break;
        }
#endif
    }

    return CCAPI_GLOBAL_ERROR_NONE;
}

int ccapi_xml_rci_group_set(ccapi_rci_info_t * const info, ...)
{
    va_list arg_list;

    va_start(arg_list, info);

    switch(info->element.type)
    {
        case CCAPI_RCI_ELEMENT_TYPE_NOT_SET:
            break;
#if (defined RCI_PARSER_USES_STRINGS)
#if (defined RCI_PARSER_USES_STRING)
        case CCAPI_RCI_ELEMENT_TYPE_STRING:
#endif
#if (defined RCI_PARSER_USES_MULTILINE_STRING)
        case CCAPI_RCI_ELEMENT_TYPE_MULTILINE_STRING:
#endif
#if (defined RCI_PARSER_USES_PASSWORD)
        case CCAPI_RCI_ELEMENT_TYPE_PASSWORD:
#endif
#if (defined RCI_PARSER_USES_FQDNV4)
        case CCAPI_RCI_ELEMENT_TYPE_FQDNV4:
#endif
#if (defined RCI_PARSER_USES_FQDNV6)
        case CCAPI_RCI_ELEMENT_TYPE_FQDNV6:
#endif
#if (defined RCI_PARSER_USES_DATETIME)
        case CCAPI_RCI_ELEMENT_TYPE_DATETIME:
#endif
#if (defined RCI_PARSER_USES_IPV4)
        case CCAPI_RCI_ELEMENT_TYPE_IPV4:
#endif
#if (defined RCI_PARSER_USES_MAC_ADDR)
        case CCAPI_RCI_ELEMENT_TYPE_MAC:
#endif
#if (defined RCI_PARSER_USES_ENUM)
        case CCAPI_RCI_ELEMENT_TYPE_ENUM:
#endif
        {
            char const * const p_string = va_arg(arg_list, char const * const);

            append_data_to_xml_request_buffer("<%s>%s</%s>", info->element.name, p_string, info->element.name);

            break;
        }
#endif
#if (defined RCI_PARSER_USES_INT32)
        case CCAPI_RCI_ELEMENT_TYPE_INT32:
        {
            int32_t const * const p_int32_t = va_arg(arg_list, int32_t const * const);

            append_data_to_xml_request_buffer("<%s>%d</%s>", info->element.name, *p_int32_t, info->element.name);    

            break;
        }
#endif
#if (defined RCI_PARSER_USES_UNSIGNED_INTEGER)
#if (defined RCI_PARSER_USES_UINT32)
        case CCAPI_RCI_ELEMENT_TYPE_UINT32:
#endif
#if (defined RCI_PARSER_USES_HEX32)
        case CCAPI_RCI_ELEMENT_TYPE_HEX32:
#endif
#if (defined RCI_PARSER_USES_0X_HEX32)
        case CCAPI_RCI_ELEMENT_TYPE_0X32:
#endif
        {
            uint32_t const * const p_uint32_t = va_arg(arg_list, uint32_t const * const);

            append_data_to_xml_request_buffer("<%s>%d</%s>", info->element.name, *p_uint32_t, info->element.name);    

            break;
        }
#endif
#if (defined RCI_PARSER_USES_FLOAT)
        case CCAPI_RCI_ELEMENT_TYPE_FLOAT:
        {
            float const * const p_float = va_arg(arg_list, float const * const);

            append_data_to_xml_request_buffer("<%s>%f</%s>", info->element.name, *p_float, info->element.name);    

            break;
        }
#endif
#if (defined RCI_PARSER_USES_ON_OFF)
        case CCAPI_RCI_ELEMENT_TYPE_ON_OFF:
        {
            ccapi_on_off_t const * const p_ccapi_on_off_t = va_arg(arg_list, ccapi_on_off_t const * const);

            append_data_to_xml_request_buffer("<%s>%s</%s>", info->element.name, *p_ccapi_on_off_t ? "on":"off", info->element.name);    

            break;
        }
#endif
#if (defined RCI_PARSER_USES_BOOLEAN)
        case CCAPI_RCI_ELEMENT_TYPE_BOOL:
        {
            ccapi_bool_t const * const p_ccapi_bool_t = va_arg(arg_list, ccapi_bool_t const * const);

            append_data_to_xml_request_buffer("<%s>%s</%s>", info->element.name, *p_ccapi_bool_t ? "true":"false", info->element.name);    

            break;
        }        
#endif
    }

    va_end(arg_list);

    return CCAPI_GLOBAL_ERROR_NONE;
}

#define BRACKET_SIZE 2 /* Size of '<' + '>' */
#define NULL_TERM_SIZE 1

static int get_xml_value(ccapi_rci_info_t * const info, char const * const xml_response_buffer, char const * * xml_value)
{
    int error_id = CCAPI_GLOBAL_ERROR_XML_RESPONSE_FAIL;
    char * element_ptr = NULL;
    char element_name[RCI_ELEMENT_NAME_MAX_SIZE + BRACKET_SIZE + NULL_TERM_SIZE];

    assert(xml_response_buffer != NULL);

    /* Firs look for empty answer */
    sprintf(element_name, "<%s/>", info->element.name);
    element_ptr = strstr(xml_response_buffer, element_name);
    if (element_ptr != NULL)
    {
        *xml_value = "";

        error_id = CCAPI_GLOBAL_ERROR_NONE;
        goto done;
    }

    sprintf(element_name, "<%s>", info->element.name);

    element_ptr = strstr(xml_response_buffer, element_name);
    if (element_ptr != NULL)
    {
        int scanf_ret;
        static char value[XML_MAX_VALUE_LENGTH + 1];

        #define XML_MAX_VALUE_LENGTH_STR  xstr(XML_MAX_VALUE_LENGTH)

        #define XML_VALUE_FORMAT "%*[^>]>%" XML_MAX_VALUE_LENGTH_STR "[^<]</"
        scanf_ret = sscanf(element_ptr, XML_VALUE_FORMAT, value);
        if (scanf_ret > 0)
        {
            /* printf("element xml_value='%s'\n", value); */
            *xml_value = value;

            error_id = CCAPI_GLOBAL_ERROR_NONE;
            goto done;
        }
    }

done:
    return error_id;
}

int ccapi_xml_rci_group_get(ccapi_rci_info_t * const info, ...)
{
    va_list arg_list;
    int error_id;
    char const * xml_value = NULL;

    va_start(arg_list, info);

    error_id = get_xml_value(info, xml_query_response_buffer, &xml_value);
    if (error_id != CCAPI_GLOBAL_ERROR_NONE)
    {
        goto done;
    }

    switch(info->element.type)
    {
        case CCAPI_RCI_ELEMENT_TYPE_NOT_SET:
            break;
#if (defined RCI_PARSER_USES_STRINGS)
#if (defined RCI_PARSER_USES_STRING)
        case CCAPI_RCI_ELEMENT_TYPE_STRING:
#endif
#if (defined RCI_PARSER_USES_MULTILINE_STRING)
        case CCAPI_RCI_ELEMENT_TYPE_MULTILINE_STRING:
#endif
#if (defined RCI_PARSER_USES_PASSWORD)
        case CCAPI_RCI_ELEMENT_TYPE_PASSWORD:
#endif
#if (defined RCI_PARSER_USES_FQDNV4)
        case CCAPI_RCI_ELEMENT_TYPE_FQDNV4:
#endif
#if (defined RCI_PARSER_USES_FQDNV6)
        case CCAPI_RCI_ELEMENT_TYPE_FQDNV6:
#endif
#if (defined RCI_PARSER_USES_DATETIME)
        case CCAPI_RCI_ELEMENT_TYPE_DATETIME:
#endif
#if (defined RCI_PARSER_USES_IPV4)
        case CCAPI_RCI_ELEMENT_TYPE_IPV4:
#endif
#if (defined RCI_PARSER_USES_MAC_ADDR)
        case CCAPI_RCI_ELEMENT_TYPE_MAC:
#endif
#if (defined RCI_PARSER_USES_ENUM)
        case CCAPI_RCI_ELEMENT_TYPE_ENUM:
#endif
        {
            char const * * const p_string = va_arg(arg_list, char const * * const);

            *p_string = xml_value;

            printf("string_value='%s'\n", *p_string);

            break;
        }
#endif
#if (defined RCI_PARSER_USES_INT32)
        case CCAPI_RCI_ELEMENT_TYPE_INT32:
        {
            int32_t * const p_int32_t = va_arg(arg_list, int32_t * const);

            *p_int32_t = (int32_t)atoi(xml_value);

            printf("integer_value='%d'\n", *p_int32_t);

            break;
        }
#endif
#if (defined RCI_PARSER_USES_UNSIGNED_INTEGER)
#if (defined RCI_PARSER_USES_UINT32)
        case CCAPI_RCI_ELEMENT_TYPE_UINT32:
#endif
#if (defined RCI_PARSER_USES_HEX32)
        case CCAPI_RCI_ELEMENT_TYPE_HEX32:
#endif
#if (defined RCI_PARSER_USES_0X_HEX32)
        case CCAPI_RCI_ELEMENT_TYPE_0X32:
#endif
        {
            uint32_t * const p_uint32_t = va_arg(arg_list, uint32_t * const);

            *p_uint32_t = (uint32_t)atoi(xml_value);

            printf("unsigned_integer_value='%u'\n", *p_uint32_t);

            break;
        }
#endif
#if (defined RCI_PARSER_USES_FLOAT)
        case CCAPI_RCI_ELEMENT_TYPE_FLOAT:
        {
            float * const p_float = va_arg(arg_list, float * const);

            *p_float = (float)atof(xml_value);

            printf("float_value='%f'\n", *p_float);

            break;
        }
#endif
#if (defined RCI_PARSER_USES_ON_OFF)
        case CCAPI_RCI_ELEMENT_TYPE_ON_OFF:
        {
            ccapi_on_off_t * const p_ccapi_on_off_t = va_arg(arg_list, ccapi_on_off_t * const);

            /* boolean value should be 0, 1, on, off, true or false */
            if (!strcmp(xml_value, "0") || !strcmp(xml_value, "off") || !strcmp(xml_value, "false"))
            {
                *p_ccapi_on_off_t = CCAPI_OFF;
            }
            else if (!strcmp(xml_value, "1") || !strcasecmp(xml_value, "on") || !strcasecmp(xml_value, "true"))
            {
                *p_ccapi_on_off_t = CCAPI_ON;
            }
            else
            {
                error_id = CCAPI_GLOBAL_ERROR_XML_RESPONSE_FAIL;
                goto done;
            }

            printf("ccapi_on_off_value='%u'\n", *p_ccapi_on_off_t);

            break;
        }    
#endif
#if (defined RCI_PARSER_USES_BOOLEAN)
        case CCAPI_RCI_ELEMENT_TYPE_BOOL:
        {
            ccapi_bool_t * const p_ccapi_bool_t = va_arg(arg_list, ccapi_bool_t * const);

            /* boolean value should be 0, 1, on, off, true or false */
            if (!strcmp(xml_value, "0") || !strcmp(xml_value, "off") || !strcmp(xml_value, "false"))
            {
                *p_ccapi_bool_t = CCAPI_FALSE;
            }
            else if (!strcmp(xml_value, "1") || !strcasecmp(xml_value, "on") || !strcasecmp(xml_value, "true"))
            {
                *p_ccapi_bool_t = CCAPI_TRUE;
            }
            else
            {
                error_id = CCAPI_GLOBAL_ERROR_XML_RESPONSE_FAIL;
                goto done;
            }

            printf("ccapi_bool_value='%u'\n", *p_ccapi_bool_t);

            break;
        }        
#endif
    }

done:
    va_end(arg_list);

    return error_id;
}

#if (defined RCI_LEGACY_COMMANDS)
#define XML_MAX_DO_COMMAND_RESPONSE_LENGTH_STR  xstr(XML_MAX_DO_COMMAND_RESPONSE_LENGTH)
#define XML_DO_COMMAND_RESPONSE_FORMAT "%*[^>]>%" XML_MAX_DO_COMMAND_RESPONSE_LENGTH_STR "[^<]</"
static int get_xml_do_command_response(ccapi_rci_info_t * const info, char const * const xml_response_buffer, char const * * xml_do_command_response)
{
    int error_id = CCAPI_GLOBAL_ERROR_XML_RESPONSE_FAIL;
    char * do_command_ptr = NULL;
    char do_command_tag[sizeof("do_command target=\"\"") + RCI_COMMANDS_ATTRIBUTE_MAX_LEN + BRACKET_SIZE + NULL_TERM_SIZE];

    assert(xml_response_buffer != NULL);

    /* 1: look for empty answer without target */
    sprintf(do_command_tag, "<do_command/>");
    do_command_ptr = strstr(xml_response_buffer, do_command_tag);
    if (do_command_ptr != NULL)
    {
        *xml_do_command_response = "";

        error_id = CCAPI_GLOBAL_ERROR_NONE;
        goto done;
    }

    /* 2: look for filled answer without target */
    sprintf(do_command_tag, "<do_command>");
    do_command_ptr = strstr(xml_response_buffer, do_command_tag);
    if (do_command_ptr != NULL)
    {
        if (sscanf(do_command_ptr, XML_DO_COMMAND_RESPONSE_FORMAT, do_command_response) > 0)
        {
            printf("xml_do_command_response='%s'\n", do_command_response);
            *xml_do_command_response = do_command_response;

            error_id = CCAPI_GLOBAL_ERROR_NONE;
            goto done;
        }
    }

    /* 3: look for empty answer with target */
    sprintf(do_command_tag, "<do_command target=\"%s\"/>", info->do_command.target);
    do_command_ptr = strstr(xml_response_buffer, do_command_tag);
    if (do_command_ptr != NULL)
    {
        *xml_do_command_response = "";

        error_id = CCAPI_GLOBAL_ERROR_NONE;
        goto done;
    }

    /* 4: look for filled answer with target */
    sprintf(do_command_tag, "<do_command target=\"%s\">", info->do_command.target);
    do_command_ptr = strstr(xml_response_buffer, do_command_tag);
    if (do_command_ptr != NULL)
    {
        if (sscanf(do_command_ptr, XML_DO_COMMAND_RESPONSE_FORMAT, do_command_response) > 0)
        {
            printf("xml_do_command_response='%s'\n", do_command_response);
            *xml_do_command_response = do_command_response;

            error_id = CCAPI_GLOBAL_ERROR_NONE;
            goto done;
        }
    }

done:
    return error_id;
}

ccapi_global_error_id_t rci_do_command_cb(ccapi_rci_info_t * const info)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;
    char const * xml_set_response_buffer = NULL;

    printf("    Called '%s'\n", __FUNCTION__);

    append_data_to_xml_request_buffer("<do_command");
    if (info->do_command.target != NULL)
    {
        append_data_to_xml_request_buffer(" target=\"%s\"", info->do_command.target);
    }
    append_data_to_xml_request_buffer(">%s</do_command>", info->do_command.request);

    error_id = call_xml_rci_request(info, &xml_set_response_buffer);
    if (error_id == CCAPI_GLOBAL_ERROR_NONE)
    {
        char const * xml_do_command_response = NULL;

        error_id = get_xml_do_command_response(info, xml_set_response_buffer, &xml_do_command_response);
        if (error_id == CCAPI_GLOBAL_ERROR_NONE)
        {
            *info->do_command.response = xml_do_command_response;
        }
    }

    xml_rci_finished((char *)xml_set_response_buffer);

    return error_id;
}

ccapi_global_error_id_t rci_set_factory_defaults_cb(ccapi_rci_info_t * const info)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;
    char const * xml_set_response_buffer = NULL;

    printf("    Called '%s'\n", __FUNCTION__);

    append_data_to_xml_request_buffer("<set_factory_default/>");

    error_id = call_xml_rci_request(info, &xml_set_response_buffer);

    xml_rci_finished((char *)xml_set_response_buffer);

    return error_id;
}

ccapi_global_error_id_t rci_reboot_cb(ccapi_rci_info_t * const info)
{
    ccapi_global_error_id_t error_id = CCAPI_GLOBAL_ERROR_NONE;
    char const * xml_set_response_buffer = NULL;

    printf("    Called '%s'\n", __FUNCTION__);

    append_data_to_xml_request_buffer("<reboot/>");

    error_id = call_xml_rci_request(info, &xml_set_response_buffer);

    xml_rci_finished((char *)xml_set_response_buffer);

    return error_id;
}
#endif
