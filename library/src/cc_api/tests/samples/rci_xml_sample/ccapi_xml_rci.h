#ifndef ccapi_xml_rci_h
#define ccapi_xml_rci_h

#include  "ccapi/ccapi.h"
#include  "ccapi_rci_functions.h"

ccapi_global_error_id_t ccapi_xml_rci_action_start(ccapi_rci_info_t * const info);
ccapi_global_error_id_t ccapi_xml_rci_action_end(ccapi_rci_info_t * const info);
int ccapi_xml_rci_group_start(ccapi_rci_info_t * const info);
int ccapi_xml_rci_group_end(ccapi_rci_info_t * const info);
int ccapi_xml_rci_group_get(ccapi_rci_info_t * const info, ...);
int ccapi_xml_rci_group_set(ccapi_rci_info_t * const info, ...);

#endif
