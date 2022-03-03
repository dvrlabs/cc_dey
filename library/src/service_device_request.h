#ifndef SERVICE_DEVICE_REQUEST_H
#define SERVICE_DEVICE_REQUEST_H

#define REQ_TAG_REGISTER_DR	"register_devicerequest"
#define REQ_TAG_UNREGISTER_DR	"unregister_devicerequest"

int handle_register_device_request(int fd);
int handle_unregister_device_request(int fd);

int import_devicerequests(const char *file_path);
int dump_devicerequests(const char *file_path);


void builtin_request_done(const char *target, ccapi_transport_t transport, ccapi_buffer_info_t *response_buffer_info, ccapi_receive_error_t receive_error);
ccapi_receive_error_t builtin_request_speedtest(const char *target, ccapi_transport_t transport, const ccapi_buffer_info_t *request_buffer_info, ccapi_buffer_info_t *response_buffer_info);
ccapi_receive_error_t builtin_request_modem_firmware_update(const char *target, ccapi_transport_t transport, const ccapi_buffer_info_t *request_buffer_info, ccapi_buffer_info_t *response_buffer_info);
ccapi_receive_error_t builtin_request_edp_certificate_update(const char *target, ccapi_transport_t transport, const ccapi_buffer_info_t *request_buffer_info, ccapi_buffer_info_t *response_buffer_info);
ccapi_receive_error_t builtin_request_subscriptions(const char *target, ccapi_transport_t transport, const ccapi_buffer_info_t *request_buffer_info, ccapi_buffer_info_t *response_buffer_info);

#endif
