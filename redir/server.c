/**
 * BSD-2-Clause
 *
 * Copyright (c) 2020 Tristan
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS  SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND  ANY  EXPRESS  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED  WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE  DISCLAIMED.  IN  NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE   FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,  PROCUREMENT  OF
 * SUBSTITUTE  GOODS  OR  SERVICES;  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION)  HOWEVER  CAUSED  AND  ON  ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT,  STRICT  LIABILITY,  OR  TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING  IN  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "server.h"

#include <errno.h>
#include <poll.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "base/global_state.h"
#include "misc/default.h"
#include "client.h"

static void *
RSChildEntrypoint(void *);

void *
RSEntrypoint(void *threadParameter) {
	UNUSED(threadParameter);

	struct pollfd pollInfo;
	pollInfo.fd = GSRedirSocket;

	while (GSMainLoop) {
		int ret;
		int sockfd;

		pollInfo.events = POLLIN;
		pollInfo.revents = 0;

		ret = poll(&pollInfo, 1, -1);

		if (ret < 0) {
			perror(ANSI_COLOR_RED"[RedirService] poll failed"ANSI_COLOR_RESET);
			break;
		} else if (ret == 0)
			continue;

		/* The socket was closed: */
		if (pollInfo.revents == POLLNVAL)
			break;

		sockfd = accept(GSRedirSocket, NULL, 0);

		if (sockfd == -1) {
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||
				errno == ECONNABORTED
			) {
				fputs(ANSI_COLOR_RED
					  "[RedirService] Poll success but accept() failed."
					  ANSI_COLOR_RESETLN, stderr);
				if (sched_yield() == -1) {
					perror(ANSI_COLOR_RED
						   "[RedirService] sched_yield() failed"
						   ANSI_COLOR_RESET);
				}
				continue;
			}

			/* The socket was closed: */
			if (errno == EBADF)
				break;

			perror(ANSI_COLOR_RED
				   "[RedirService] [CRITICAL] Socket I/O error occurred"
				   ANSI_COLOR_RESET);

			printf("errno was %i\n", errno);
			break;
		}

		if (!GSScheduleChildThread(GSTP_REDIR, RSChildEntrypoint, sockfd)) {
			close(sockfd);
			fputs(
				ANSI_COLOR_RED
				"[RedirService] [CRITICAL] Failed to schedule child thread."
				ANSI_COLOR_RESETLN,
				stderr
			);
			break;
		}
	}

	return NULL;
}

static void *
RSChildEntrypoint(void *threadParameter) {
	char *path;
	struct GSThread *thread;

	path = malloc(GSMaxPathSize);
	if (!path)
		return NULL;

	thread = threadParameter;

	RSChildHandler(thread->sockfd, path);
	close(thread->sockfd);
	free(path);

	GSChildThreadRelease(thread);

	return NULL;
}
