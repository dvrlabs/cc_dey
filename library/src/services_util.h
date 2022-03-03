#ifndef SERVICES_UTIL_H
#define SERVICES_UTIL_H

#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include "ccapi/ccapi.h"

/* 
 * DRM imposes a timeout of 75 seconds for synchronous SCI requests, so
 * it does not make sense for the connector to wait any longer for
 * the response.
 */

/* TODO: Move to a DAL configuration option */
#define SOCKET_READ_TIMEOUT_SEC	75

const char *to_send_error_msg(ccapi_send_error_t error);

int read_uint32(int fd, uint32_t * const ret, struct timeval *timeout);
int write_uint32(int fd, const uint32_t value);

int read_string(int fd, char **string, size_t *length, struct timeval *timeout);
int write_string(int fd, const char *string );

int read_blob(int fd, void **buffer, size_t *length, struct timeval *timeout);
int write_blob(int fd, const void *data, size_t data_length);

int send_ok(int fd);
int send_error(int fd, const char *msg);

#endif
