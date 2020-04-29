/**
 * BSD-2-Clause
 * 
 * Copyright (c) 2020 Tristan
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include "server.h"

#include <sys/socket.h>

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>

#include "base/global_state.h"
#include "core/h1.h"
#include "core/h2.h"
#include "core/security.h"
#include "misc/default.h"

void *CSChildEntrypoint(void *threadParameter) {
	struct GSThread *thread = threadParameter;
	CSSClient client = NULL;
	enum CSProtocol protocol;
	int ret;

	ret = CSSSetupClient(thread->sockfd, &client);

	if (ret <= 0) {
		printf("Failed to setup client: %i\n", ret);
	} else if (client == NULL) {
		printf("Client was NULL: %i\n", ret);
	} else {
		protocol = CSSGetProtocol(client);
		fputs("protocol is: ", stdout);
		switch (protocol) {
			case CSPROT_ERROR:
				puts("CSPROT_ERROR");
				break;
			case CSPROT_HTTP1:
				puts("CSPROT_HTTP1");
				CSHandleHTTP1(client);
				break;
			case CSPROT_HTTP2:
				puts("CSPROT_HTTP2");
				CSHandleHTTP2(client);
				break;
			case CSPROT_NONE:
				puts("CSPROT_NONE");
				CSHandleHTTP1(client);
				break;
		}

		/* TODO Check ALPN */
		CSSDestroyClient(client);
	}

	GSChildThreadRelease(thread);
	return NULL;
}

void *CSEntrypoint(void *threadParameter) {
	UNUSED(threadParameter);

	while (GSMainLoop) {
		int sockfd;

		sockfd = accept(GSCoreSocket, NULL, 0);

		if (sockfd == -1) {
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||
				errno == ECONNABORTED
			) {
				if (sched_yield() == -1) {
					perror(ANSI_COLOR_RED
						   "[CoreService] sched_yield() failed"
						   ANSI_COLOR_RESET);
				}
				continue;
			}

			perror(ANSI_COLOR_RED
				   "[CoreService] [CRITICAL] Socket I/O error occurred"
				   ANSI_COLOR_RESET);
			break;
		}

		if (!GSScheduleChildThread(GSTP_REDIR, CSChildEntrypoint, sockfd)) {
			fputs(
				ANSI_COLOR_RED
				"[CoreService] [CRITICAL] Failed to schedule child thread."
				ANSI_COLOR_RESETLN,
				stderr
			);
			break;
		}
	}

	GSCoreThreadState = 0;
	pthread_exit(NULL);
	return NULL;
}
