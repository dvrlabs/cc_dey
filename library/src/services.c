/*****************************************************************************
 * Copyright 2019 Digi International Inc., All Rights Reserved
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
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 ****************************************************************************/
 
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "ccapi/ccapi.h"
#include "cc_logging.h"
#include "service_device_request.h"
#ifdef DO_COMMAND_SUPPORT
#include "service_do_command.h"
#endif
#include "service_dp_upload.h"
#include "service_name_upload.h"
#include "services.h"
#include "services_util.h"

#define CONNECTOR_REQUEST_PORT	977
#define REQUEST_TAG_MAX_LENGTH	64

static bool received_sigterm = false;

static void sigterm_handler(int signum)
{
	received_sigterm = true;
}

static int setup_sigterm_handler()
{
	struct sigaction act;
	sigset_t set;

	memset(&act, 0, sizeof(act));
	act.sa_handler = sigterm_handler;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);

	if (sigaction(SIGTERM, &act, NULL))
	{
		log_error("%s", "Failed to install signal handler");
		return 1;
	}

	if (pthread_sigmask(SIG_UNBLOCK, &set, NULL))
	{
		log_error("%s", "Failed to unblock SIGTERM");
		return 1;
	}

	return 0;
}

typedef int (*request_handler_t)(int socket_fd);

struct handler_t {
	const char *request_tag;
	request_handler_t request_handler;
} request_handlers[] = {
	{
		REQ_TAG_DP_FILE_REQUEST,
		handle_datapoint_file_upload
	},
	{
		REQ_TAG_NAME_REQUEST,
		handle_name_upload
	},
#ifdef DO_COMMAND_SUPPORT
	{
		REQ_TAG_REGISTER_DC,
		handle_register_do_command
	},
	{
		REQ_TAG_UNREGISTER_DC,
		handle_unregister_do_command
	},
#endif
	{
		REQ_TAG_REGISTER_DR,
		handle_register_device_request
	},
	{
		REQ_TAG_UNREGISTER_DR,
		handle_unregister_device_request
	}
};

static void handle_requests(int fd)
{
	int request_sock;
	
	if (setup_sigterm_handler())
	{
		return;
	}

	while (!received_sigterm && (request_sock = accept4(fd, NULL, NULL, SOCK_CLOEXEC)) != -1)
	{
		char *request_tag = NULL;
		int i;
		bool handled = false;
		struct timeval timeout = {
			.tv_sec = 20,
			.tv_usec = 0
		};
		/* Read request tag (to select handler for this request) */
		if (read_string(request_sock, &request_tag, NULL, &timeout) < 0)
		{
			send_error(request_sock, "Failed to read request code");
			log_error("Error reading request tag, %s", strerror(errno));
			close(request_sock);
			continue;
		}
		
		/* Invoke the corresponding handler (only one handler per request */
		for (i = 0; i < ARRAY_SIZE(request_handlers); i++)
		{
			const struct handler_t *handler = &request_handlers[i];

			if (!strcmp(request_tag, handler->request_tag))
			{
				if (handler->request_handler(request_sock))
				{
					log_error("Error handling request tagged with: '%s'", request_tag);
				}
				handled = true;

				break;
			}
		}

		/* Error on requets that cannot be handled, and clean up */
		if (!handled)
		{
			send_error(request_sock, "Invalid request type");
		}

		if (close(request_sock) < 0)
		{
			log_warning("could not close service socket after attending request: %s", strerror(errno));
		}
		
		free(request_tag);
	}
}

void listen_for_requests()
{
	struct sockaddr_in addr;
	int fd;
	int n_options = 1;

	fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd == -1)
	{
		return;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &n_options, sizeof(n_options)) < 0)
	{
		log_warning("Failed to set SO_REUSE* on request serversocket: %s", strerror(errno));
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONNECTOR_REQUEST_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *) &addr, sizeof addr) || listen(fd, 3))
	{
		log_error("%s", "Failed to bind to local socket");
		goto done;
	}

	handle_requests(fd);

done:
	close(fd);
}
