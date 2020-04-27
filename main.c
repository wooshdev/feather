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

#include <sys/resource.h>
#include <sys/time.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base/global_state.h"
#include "cache/cache.h"
#include "core/security.h"
#include "core/server.h"
#include "misc/default.h"
#include "misc/options.h"
#include "misc/statistics.h"
#include "redir/server.h"

static void CatchSignal(int, siginfo_t *, void *);

int main(void) {
	pthread_attr_t attribs;
	size_t currentCount;
	size_t lastCount;
	struct rusage afterCache;
	struct rusage beforeCache;
	struct sigaction act;
	struct timespec time;

	/* Setup GlobalState */
	if (!GSInit()) {
		GSDestroy();
		err(0, "Failed to initialize.");
	}

	/* Setup Signal Catching */
	memset(&act, 0, sizeof(struct sigaction));
	if (sigemptyset(&act.sa_mask) == -1) {
		GSDestroy();
		perror("[Main] sigemptyset() failed");
		err(0, "[Main] Failed to set signal handler.");
	}
  
	act.sa_sigaction = CatchSignal;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act, NULL) == -1 || sigaction(SIGPIPE, &act, NULL) == -1) {
		GSDestroy();
		perror("[Main] sigaction() failed");
		err(0, "Failed to set signal handler.");
	}

	/* Setup options/configuration */
	if (!OMSetup()) {
		GSDestroy();
		err(0, "Failed to setup the OptionsManager.");
	}

	/* Start security */
	if (CSSetupSecurityManager() <= 0) {
		OMDestroy();
		GSDestroy();
		err(0, "Failed to setup the SecurityManager.");
	}

	getrusage(RUSAGE_SELF, &beforeCache);
	/* Start cache */
	if (!FCSetup()) {
		OMDestroy();
		CSDestroySecurityManager();
		GSDestroy();
		err(0, "Failed to start GSRedirThread.");
	}
	getrusage(RUSAGE_SELF, &afterCache);
	printf(ANSI_COLOR_MAGENTA"Cache actually uses %zu octets."ANSI_COLOR_RESETLN,
		   afterCache.ru_maxrss * 1024L - beforeCache.ru_maxrss * 1024L);

	/* Start services. */
	pthread_attr_init(&attribs);
	pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);

	GSRedirThreadState = 1;
	if (pthread_create(&GSRedirThread, &attribs, &RSEntrypoint, NULL) != 0) {
		FCDestroy();
		OMDestroy();
		CSDestroySecurityManager();
		GSDestroy();
		err(0, "Failed to start GSRedirThread.");
	}

	GSCoreThreadState = 1;
	if (pthread_create(&GSCoreThread, &attribs, &CSEntrypoint, NULL) != 0) {
		FCDestroy();
		OMDestroy();
		CSDestroySecurityManager();
		pthread_cancel(GSRedirThread);
		GSDestroy();
		err(0, "Failed to start GSCoreThread.");
	}
	pthread_attr_destroy(&attribs);

	time.tv_sec = 0;
	lastCount = 0;

	fputs("[Main] Initialization was "ANSI_COLOR_GREEN"succesful"
		  ANSI_COLOR_RESET".\n", stdout);

	while (GSMainLoop) {
		time.tv_sec = 1;
		time.tv_nsec = 0;
		nanosleep(&time, NULL);

		currentCount = SMGetPageTraffic();
		if (currentCount != lastCount) {
			printf(ANSI_COLOR_CYAN"Traffic> "ANSI_COLOR_MAGENTA"%zu"ANSI_COLOR_RESETLN, currentCount);
			lastCount = currentCount;
		}
	}

	printf(ANSI_COLOR_CYAN"Traffic> Total was "ANSI_COLOR_MAGENTA"%zu"ANSI_COLOR_RESETLN, SMGetPageTraffic());

	GSDestroy();

	/* Stop threads */
	time.tv_nsec = 100000000; /* 100 ms */
	nanosleep(&time, NULL);

	pthread_join(GSCoreThread, NULL);
	pthread_join(GSRedirThread, NULL);

	/* After all other threads have stopped: */
	CSDestroySecurityManager();
	FCDestroy();
	OMDestroy();

	return EXIT_SUCCESS;
}

static void CatchSignal(int signo, siginfo_t *info, void *context) {
	UNUSED(info);
	UNUSED(context);

	if (signo == SIGINT)
		GSNotify(GSA_INTERRUPT);
}
