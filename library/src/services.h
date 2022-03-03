#ifndef SERVICES_H
#define SERVICES_H

extern int sms_is_enabled(void);
void listen_for_requests(void);
unsigned short drm_port(void);
int cli_local_auth_is_enabled(void);
int proxy_is_enabled(void);
char const * const proxy_host(void);
unsigned short proxy_port(void);

#endif
