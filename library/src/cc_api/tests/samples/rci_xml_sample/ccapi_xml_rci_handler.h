#ifndef ccapi_xml_rci_handler_h
#define ccapi_xml_rci_handler_h

#include  "ccapi/ccapi.h"

#define XML_MAX_ERROR_DESC_LENGTH 100
#define XML_MAX_ERROR_HINT_LENGTH 100

#define XML_MAX_VALUE_LENGTH 100
#define XML_MAX_DO_COMMAND_RESPONSE_LENGTH 250

void xml_rci_request(char const * const xml_request_buffer, char const * * const xml_response_buffer);
void xml_rci_finished(char * const xml_response_buffer);

#endif
