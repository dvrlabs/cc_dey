#include <stdio.h>
#include <stdarg.h>
#include "ccapi_xml_rci_handler.h"

//#define TEST_ERRORS

#define SET_GOOD_RESPONSE \
       "<set_setting>     \
          <serial>        \
            <baud/>       \
            <parity/>     \
            <xbreak/>     \
            <databits/>   \
          </serial>       \
          <ethernet>      \
            <ip/>         \
            <dhcp/>       \
          </ethernet>     \
        </set_setting>"

#if (defined TEST_ERRORS)
#define SET_BAD_RESPONSE  \
       "<set_setting>     \
          <serial>        \
            <baud>        \
               <error id=\"1\"> \
               <desc>linux error description while setting</desc> \
               <hint>linux error hint while setting</hint> \
               </error>   \
            </baud>       \
            <parity/>     \
            <xbreak/>     \
            <databits/>   \
          </serial>       \
          <ethernet>      \
            <ip/>         \
            <dhcp/>       \
          </ethernet>     \
        </set_setting>"
#endif

#define QUERY_SETTING_SERIAL_RESPONSE \
       "<query_setting>            \
          <serial index=\"2\">     \
            <baud>2400</baud>      \
            <parity>none</parity>  \
            <databits>5</databits> \
            <xbreak>on</xbreak>    \
            <txbytes>123</txbytes> \
          </serial>                \
        </query_setting>"

#define QUERY_SETTING_ETHERNET_RESPONSE \
       "<query_setting>            \
          <ethernet index=\"1\">   \
            <ip>0.0.0.0</ip>       \
            <subnet>0.0.0.0</subnet>     \
            <gateway>0.0.0.0</gateway>   \
            <dhcp>true</dhcp>            \
            <dns/>                       \
            <mac>00:00:00:00:00:00</mac> \
            <duplex>auto</duplex>        \
          </ethernet>                    \
        </query_setting>"

#define QUERY_SETTING_DEVICE_TIME_RESPONSE \
       "<query_setting>            \
          <device_time> \
            <curtime>2015-02-04T11:41:24+-100</curtime> \
          </device_time> \
        </query_setting>"

#define QUERY_SETTING_DEVICE_INFO_RESPONSE \
       "<query_setting>            \
          <device_info> \
            <version>0x2020000</version> \
            <product>Cloud Connector Product</product> \
            <model/> \
            <company>Digi International Inc.</company> \
            <desc>Cloud Connector Demo on Linux \
              with firmware upgrade, and remote configuration supports</desc> \
          </device_info> \
        </query_setting>"

#define QUERY_SETTING_SYSTEM_RESPONSE \
       "<query_setting>            \
          <system> \
            <description/> \
            <contact/> \
            <location/> \
          </system> \
        </query_setting>"

#define QUERY_SETTING_DEVICESECURITY_RESPONSE \
       "<query_setting>            \
          <devicesecurity> \
            <identityVerificationForm>simple</identityVerificationForm> \
          </devicesecurity> \
        </query_setting>"

#define QUERY_STATE_DEVICE_STATE_RESPONSE \
       "<query_state>            \
          <device_state> \
            <system_up_time>9</system_up_time> \
            <signed_integer>-10</signed_integer> \
            <float_value>3.56</float_value> \
          </device_state> \
        </query_state>"

#define QUERY_STATE_GPS_STATS_RESPONSE \
       "<query_state>            \
          <gps_stats> \
            <latitude>44.932017</latitude> \
            <longitude>-93.461594</longitude> \
          </gps_stats> \
        </query_state>"

#if (defined TEST_ERRORS)
#define QUERY_BAD_RESPONSE     \
       "<query_setting>        \
          <serial>             \
             <error id=\"1\">  \
             <desc>linux error description while querying</desc> \
             <hint>linux error hint while querying</hint> \
             </error>          \
          </serial>            \
        </query_setting>"
#endif

#if (defined RCI_LEGACY_COMMANDS)
#define REBOOT_GOOD_RESPONSE "<reboot/>"
#define SET_FACTORY_DEFAULTS_GOOD_RESPONSE "<set_factory_default/>"

#if (defined TEST_ERRORS)
#define REBOOT_BAD_RESPONSE \
       "<reboot>        \
             <error id=\"1\">  \
             <desc>linux error description while executing reboot</desc> \
             <hint>linux error hint while executing reboot</hint> \
             </error>          \
        </reboot>"

#define SET_FACTORY_DEFAULTS_BAD_RESPONSE \
       "<set_factory_default>        \
             <error id=\"1\">  \
             <desc>linux error description while executing set_factory_default</desc> \
             <hint>linux error hint while executing set_factory_default</hint> \
             </error>          \
        </set_factory_default>"
#endif
#endif

#define xstr(s) str(s)
#define str(s) #s
 
#if (defined TEST_ERRORS)
static unsigned int rnd_set_response = 0;
static unsigned int rnd_query_response = 0;
#if (defined RCI_LEGACY_COMMANDS)
static unsigned int rnd_reboot_response = 0;
static unsigned int rnd_set_factory_default_response = 0;
#endif
#endif
#if (defined RCI_LEGACY_COMMANDS)
static unsigned int rnd_do_command_response = 0;
#endif

static char * local_xml_response_buffer = NULL;
static size_t local_xml_response_buffer_size = 0;

static int append_data_to_xml_response_buffer(char const * const data, ...)
{
    int error_id = 0;
    va_list arg_list_test;
    va_list arg_list_done;
    size_t new_buffer_len;
    int print_ret_test;

    va_start(arg_list_test, data);
    va_copy(arg_list_done, arg_list_test);

    print_ret_test = vsnprintf(NULL, 0, data, arg_list_test);

    if (local_xml_response_buffer == NULL)
    {
        new_buffer_len = print_ret_test;

        local_xml_response_buffer = malloc(new_buffer_len + 1);
    }
    else
    {
        new_buffer_len = local_xml_response_buffer_size + print_ret_test;

        local_xml_response_buffer = realloc(local_xml_response_buffer, new_buffer_len + 1);
    }

    if (local_xml_response_buffer == NULL)
    {
        assert(0);
        error_id = -1;
    }
    else
    {
        int print_ret_done;

        print_ret_done = vsprintf(&local_xml_response_buffer[local_xml_response_buffer_size], data, arg_list_done);

        assert(print_ret_test == print_ret_done);

        local_xml_response_buffer_size = new_buffer_len;
    }

    va_end(arg_list_test);
    va_end(arg_list_done);

    return error_id;
}

void xml_rci_request(char const * const xml_request_buffer, char const * * const xml_response_buffer)
{
    char * group_ptr = NULL;

    printf("    Called '%s'\n", __FUNCTION__);

    assert(local_xml_response_buffer == NULL);
    assert(local_xml_response_buffer_size == 0);

    /* process the xml_request_buffer and provide a response at xml_response_buffer */
    if (strncmp(xml_request_buffer, "<query_setting", sizeof("<query_setting") - 1) == 0)
    {
        group_ptr = strstr(xml_request_buffer, "   <");
        if (group_ptr != NULL)
        {
            group_ptr += sizeof("   ") - 1;
            if (strncmp(group_ptr, "<serial", sizeof("<serial") - 1) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_SETTING_SERIAL_RESPONSE);
            }
            else if (strncmp(group_ptr, "<ethernet", sizeof("<ethernet") -1 ) == 0)
            {
                /* Just a test: every two query request for the group 'ethernet' return an error */
#if (defined TEST_ERRORS)
                if (rnd_query_response++ % 2)
                    append_data_to_xml_response_buffer("%s", QUERY_BAD_RESPONSE);
                else
#endif
                    append_data_to_xml_response_buffer("%s", QUERY_SETTING_ETHERNET_RESPONSE);
            }
            else if (strncmp(group_ptr, "<device_time", sizeof("<device_time") -1 ) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_SETTING_DEVICE_TIME_RESPONSE);
            }
            else if (strncmp(group_ptr, "<device_info", sizeof("<device_info") -1 ) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_SETTING_DEVICE_INFO_RESPONSE);
            }
            else if (strncmp(group_ptr, "<system", sizeof("<system") -1 ) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_SETTING_SYSTEM_RESPONSE);
            }
            else if (strncmp(group_ptr, "<devicesecurity", sizeof("<devicesecurity") -1 ) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_SETTING_DEVICESECURITY_RESPONSE);
            }
        }
    }
    else if (strncmp(xml_request_buffer, "<query_state", sizeof("<query_state") - 1) == 0)
    {
        group_ptr = strstr(xml_request_buffer, "   <");
        if (group_ptr != NULL)
        {
            group_ptr += sizeof("   ") - 1;
            if (strncmp(group_ptr, "<device_state", sizeof("<device_state") - 1) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_STATE_DEVICE_STATE_RESPONSE);
            }
            else if (strncmp(group_ptr, "<gps_stats", sizeof("<gps_stats") -1 ) == 0)
            {
                append_data_to_xml_response_buffer("%s", QUERY_STATE_GPS_STATS_RESPONSE);
            }
        }
    }
    else if (strncmp(xml_request_buffer, "<set_setting", sizeof("<set_setting") - 1) == 0)
    {
        /* Don't mind the group set in the request... just provide a response (for 'ethernet' group for example)
           with or without 'error' tag randomly */
#if (defined TEST_ERRORS)
        if (rnd_set_response++ % 2)
            append_data_to_xml_response_buffer("%s", SET_BAD_RESPONSE);
        else
#endif
            append_data_to_xml_response_buffer("%s", SET_GOOD_RESPONSE);
    }
    else if (strncmp(xml_request_buffer, "<set_state", sizeof("<set_state") - 1) == 0)
    {
        /* Don't mind the group set in the request... just provide a response (for 'ethernet' group for example) */
        append_data_to_xml_response_buffer("%s", SET_GOOD_RESPONSE);
    }
#if (defined RCI_LEGACY_COMMANDS)
    /* do_command without target */
    #define DO_COMMAND_NO_TARGET "<do_command>"
    else if (strncmp(xml_request_buffer, DO_COMMAND_NO_TARGET, sizeof(DO_COMMAND_NO_TARGET) - 1) == 0)
    {
#if (defined TEST_ERRORS)
        if (rnd_do_command_response % 3 == 2)
        {

            append_data_to_xml_response_buffer("<do_command> \
                                         <error id=\"1\">  \
                                            <desc>linux error description while executing do_command</desc> \
                                            <hint>linux error hint while executing do_command without target</hint> \
                                         </error>          \
                                      </do_command>");
        }
        else
#endif
        {
            if (rnd_do_command_response % 3 == 1)
            {
                /* Empty response */
                append_data_to_xml_response_buffer("<do_command/>");
            }
            else
            {
                append_data_to_xml_response_buffer("<do_command>do command without target good response</do_command>");
            }
        }
        rnd_do_command_response++;
    }
    /* do_command with target */
    #define DO_COMMAND_TARGET "<do_command target=\""
    else if (strncmp(xml_request_buffer, DO_COMMAND_TARGET, sizeof(DO_COMMAND_TARGET) - 1) == 0)
    {
        char const * const target_ptr = &xml_request_buffer[sizeof(DO_COMMAND_TARGET) - 1];
        int scanf_ret;
        char target[RCI_COMMANDS_ATTRIBUTE_MAX_LEN + 1];

        #define RCI_COMMANDS_ATTRIBUTE_MAX_LEN_STR  xstr(RCI_COMMANDS_ATTRIBUTE_MAX_LEN)

        #define TARGET_FORMAT "%" RCI_COMMANDS_ATTRIBUTE_MAX_LEN_STR "[^\"]>"
        scanf_ret = sscanf(target_ptr, TARGET_FORMAT, target);
        if (scanf_ret == 0)
        {
            goto done;
        }

        /* printf("target='%s'\n", target); */

#if (defined TEST_ERRORS)
        if (rnd_do_command_response % 3 == 2)
        {
            append_data_to_xml_response_buffer("<do_command target=\"%s\"> \
                                         <error id=\"1\">  \
                                            <desc>linux error description while executing do_command</desc> \
                                            <hint>linux error hint while executing do_command with target</hint> \
                                         </error>          \
                                      </do_command>", target);
        }
        else
#endif
        {
            if (rnd_do_command_response % 3 == 1)
            {
                /* Empty response */
                append_data_to_xml_response_buffer("<do_command target=\"%s\"/>", target);
            }
            else
            {
                append_data_to_xml_response_buffer("<do_command target=\"%s\">do command with target good response</do_command>", target);
            }
        }
        rnd_do_command_response++;
    }
    else if (strncmp(xml_request_buffer, "<set_factory_default", sizeof("<set_factory_default") - 1) == 0)
    {
        /* Just provide a response with or without 'error' tag randomly */
#if (defined TEST_ERRORS)
        if (rnd_set_factory_default_response++ % 2)
            append_data_to_xml_response_buffer("%s", SET_FACTORY_DEFAULTS_BAD_RESPONSE);
        else
#endif
            append_data_to_xml_response_buffer("%s", SET_FACTORY_DEFAULTS_GOOD_RESPONSE);
    }
    else if (strncmp(xml_request_buffer, "<reboot", sizeof("<reboot") - 1) == 0)
    {
        /* Just provide a response with or without 'error' tag randomly */
#if (defined TEST_ERRORS)
        if (rnd_reboot_response++ % 2)
            append_data_to_xml_response_buffer("%s", REBOOT_BAD_RESPONSE);
        else
#endif
            append_data_to_xml_response_buffer("%s", REBOOT_GOOD_RESPONSE);
    }
#endif
    else
    {
        printf("Unsupported command!!!!!\n");
        assert(0);
    }

    *xml_response_buffer = local_xml_response_buffer;

done:
    return;
}

void xml_rci_finished(char * const xml_response_buffer)
{
    assert(local_xml_response_buffer == xml_response_buffer);

    if (local_xml_response_buffer != NULL)
    {
        free(local_xml_response_buffer);
        local_xml_response_buffer = NULL;
        local_xml_response_buffer_size = 0;
    }

    return;
}

