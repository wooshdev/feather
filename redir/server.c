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

#include <errno.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "base/global_state.h"
#include "misc/default.h"
#include "client.h"

void *RSChildEntrypoint(void *threadParameter) {
	struct GSThread *thread = threadParameter;

	RSChildHandler(thread->sockfd);
	close(thread->sockfd);

	GSChildThreadRelease(thread);

	return NULL;
}

void *RSEntrypoint(void *threadParameter) {
	UNUSED(threadParameter);

	while (GSMainLoop) {
		int sockfd;

		sockfd = accept(GSRedirSocket, NULL, 0);

		if (sockfd == -1) {
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||
				errno == ECONNABORTED
			) {
				if (sched_yield() == -1) {
					perror(ANSI_COLOR_RED
						   "[RedirService] sched_yield() failed"
						   ANSI_COLOR_RESET);
				}
				continue;
			}

			perror(ANSI_COLOR_RED
				   "[RedirService] [CRITICAL] Socket I/O error occurred"
				   ANSI_COLOR_RESET);
			break;
		}

		/* printf("Sockfd (accepted): %i\n", sockfd); */
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
