#include  <stdio.h>
#include  "ccapi/ccapi.h"
#include  "ccapi_rci_functions.h"

#include  "ccapi_xml_rci.h"

ccapi_global_error_id_t rci_session_start_cb(ccapi_rci_info_t * const info)
{
    UNUSED_PARAMETER(info);
    printf("    Called '%s'\n", __FUNCTION__);
    return CCAPI_GLOBAL_ERROR_NONE;
}

ccapi_global_error_id_t rci_session_end_cb(ccapi_rci_info_t * const info)
{
    UNUSED_PARAMETER(info);
    printf("    Called '%s'\n", __FUNCTION__);
    return CCAPI_GLOBAL_ERROR_NONE;
}

ccapi_global_error_id_t rci_action_start_cb(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_action_start(info);
}

ccapi_global_error_id_t rci_action_end_cb(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);

    return ccapi_xml_rci_action_end(info);
}

ccapi_setting_serial_error_id_t rci_setting_serial_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_setting_serial_error_id_t rci_setting_serial_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_setting_serial_error_id_t rci_setting_serial_baud_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_baud_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_parity_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_parity_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_databits_get(ccapi_rci_info_t * const info, uint32_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_databits_set(ccapi_rci_info_t * const info, uint32_t const *const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_xbreak_get(ccapi_rci_info_t * const info, ccapi_on_off_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_xbreak_set(ccapi_rci_info_t * const info, ccapi_on_off_t const *const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_serial_error_id_t rci_setting_serial_txbytes_get(ccapi_rci_info_t * const info, uint32_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_ip_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_ip_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_subnet_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_subnet_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_gateway_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_gateway_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_dhcp_get(ccapi_rci_info_t * const info, ccapi_bool_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_dhcp_set(ccapi_rci_info_t * const info, ccapi_bool_t const *const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_dns_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_dns_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_mac_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_mac_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_duplex_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_ethernet_error_id_t rci_setting_ethernet_duplex_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_device_time_error_id_t rci_setting_device_time_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_setting_device_time_error_id_t rci_setting_device_time_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_setting_device_time_error_id_t rci_setting_device_time_curtime_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_device_time_error_id_t rci_setting_device_time_curtime_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_version_get(ccapi_rci_info_t * const info, uint32_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_product_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_product_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_model_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_model_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_company_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_company_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_desc_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_device_info_error_id_t rci_setting_device_info_desc_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_system_error_id_t rci_setting_system_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_setting_system_error_id_t rci_setting_system_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_setting_system_error_id_t rci_setting_system_description_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_system_error_id_t rci_setting_system_description_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_system_error_id_t rci_setting_system_contact_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_system_error_id_t rci_setting_system_contact_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_system_error_id_t rci_setting_system_location_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_system_error_id_t rci_setting_system_location_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_devicesecurity_error_id_t rci_setting_devicesecurity_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_setting_devicesecurity_error_id_t rci_setting_devicesecurity_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_setting_devicesecurity_error_id_t rci_setting_devicesecurity_identityVerificationForm_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_setting_devicesecurity_error_id_t rci_setting_devicesecurity_identityVerificationForm_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_setting_devicesecurity_error_id_t rci_setting_devicesecurity_password_set(ccapi_rci_info_t * const info, char const * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_state_device_state_error_id_t rci_state_device_state_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_state_device_state_error_id_t rci_state_device_state_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_state_device_state_error_id_t rci_state_device_state_system_up_time_get(ccapi_rci_info_t * const info, uint32_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_state_device_state_error_id_t rci_state_device_state_signed_integer_get(ccapi_rci_info_t * const info, int32_t * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_state_device_state_error_id_t rci_state_device_state_signed_integer_set(ccapi_rci_info_t * const info, int32_t const *const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_state_device_state_error_id_t rci_state_device_state_float_value_get(ccapi_rci_info_t * const info, float * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_state_device_state_error_id_t rci_state_device_state_float_value_set(ccapi_rci_info_t * const info, float const *const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_set(info, value);
}

ccapi_state_gps_stats_error_id_t rci_state_gps_stats_start(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_start(info);
}

ccapi_state_gps_stats_error_id_t rci_state_gps_stats_end(ccapi_rci_info_t * const info)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_end(info);
}

ccapi_state_gps_stats_error_id_t rci_state_gps_stats_latitude_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}

ccapi_state_gps_stats_error_id_t rci_state_gps_stats_longitude_get(ccapi_rci_info_t * const info, char const * * const value)
{
    printf("    Called '%s'\n", __FUNCTION__);
    return ccapi_xml_rci_group_get(info, value);
}
