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

#include <sys/resource.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base/global_state.h"
#include "cache/cache.h"
#include "core/security.h"
#include "core/server.h"
#include "misc/default.h"
#include "misc/options.h"
#include "misc/statistics.h"
#include "redir/server.h"

/* Clean up functions */
size_t cufIndex = 0;
void (*cleanUpFunctions[4])(void);

/* Prototypes */
static void CatchSignal(int);
static void StopWithError(const char *, const char *);

int main(void) {
	pthread_attr_t attribs;
	size_t lastCount;
	int ret;
	struct sigaction act;
	struct timespec time;

	cleanUpFunctions[cufIndex++] = GSDestroy;

	/* Setup GlobalState */
	if (!GSInit())
		StopWithError("GlobalState", "Failed to GSInit().");

	/* Setup Signal Catching */
	memset(&act, 0, sizeof(struct sigaction));
	if (sigemptyset(&act.sa_mask) == -1) {
		perror("[Main] sigemptyset() failed");
		StopWithError("SignalSetup", "Failed to set signal handler.");
	}

	act.sa_handler = CatchSignal;
	act.sa_flags = 0;

	if (sigaction(SIGINT,	&act, NULL) == -1 ||
		sigaction(SIGPIPE,	&act, NULL) == -1) {
		perror("[Main] sigaction() failed");
		StopWithError("SignalSetup", "Failed to set signal handler.");
	}

	/* Setup options/configuration */
	if (!OMSetup())
		StopWithError("OptionsManager", "Failed to OMSetup().");

	cleanUpFunctions[cufIndex++] = OMDestroy;

	/* Start security */
	if (CSSetupSecurityManager() <= 0)
		StopWithError("CoreSecurity", "Failed to setup the SecurityManager.");

	cleanUpFunctions[cufIndex++] = CSDestroySecurityManager;

	/* Start cache */
	if (!FCSetup())
		StopWithError("FileCache", "Failed to setup the FileCache (FCSetup).");

	cleanUpFunctions[cufIndex++] = FCDestroy;

	/* Start services. */
	if (pthread_attr_init(&attribs) != 0)
		StopWithError("Services", "pthread_attr_init failed.");

	if (pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE) != 0)
		StopWithError("Services", "pthread_attr_setdetachstate failed.");

	GSRedirThreadState = 1;
	if (pthread_create(&GSRedirThread, &attribs, &RSEntrypoint, NULL) != 0)
		StopWithError("Services", "Failed to start GSRedirThread.");

	GSCoreThreadState = 1;
	if (pthread_create(&GSCoreThread, &attribs, &CSEntrypoint, NULL) != 0) {
		pthread_cancel(GSRedirThread);
		StopWithError("Services", "Failed to start GSCoreThread.");
	}

	if (pthread_attr_destroy(&attribs) != 0)
		warn("[Main] W: Failed to destroy thread attributes.");

	lastCount = 0;

	SMBegin();
	fputs("[Main] Initialization was "ANSI_COLOR_GREEN"succesful"
		  ANSI_COLOR_RESET".\n", stdout);

	while (GSMainLoop) {
		size_t currentCount;

		time.tv_sec = 1;
		time.tv_nsec = 0;
		nanosleep(&time, NULL);

		currentCount = SMGetPageTraffic();
		if (currentCount != lastCount) {
			printf(ANSI_COLOR_CYAN"Traffic> "ANSI_COLOR_MAGENTA"%zu"
				   ANSI_COLOR_RESETLN, currentCount);
			lastCount = currentCount;
		}
	}

	/* Newline for ^C */
	putchar('\n');

	SMEnd();

	GSDestroy();

	/* Send signal to stop */
	ret = pthread_kill(GSCoreThread, SIGINT);
	if (ret != 0 && ret != ESRCH)
		fprintf(stderr, ANSI_COLOR_RED"[Main] [ShutdownProcedure] E: "
			   "CoreThreadKill=%i"ANSI_COLOR_RESETLN, ret);

	ret = pthread_kill(GSRedirThread, SIGINT);
	if (ret != 0 && ret != ESRCH)
		fprintf(stderr, ANSI_COLOR_RED"[Main] [ShutdownProcedure] E: "
			   "RedirThreadKill=%i"ANSI_COLOR_RESETLN, ret);

	/* Stop threads */
	time.tv_nsec = 100000000; /* 100 ms */
	nanosleep(&time, NULL);

	pthread_join(GSCoreThread, NULL);
	pthread_join(GSRedirThread, NULL);

	/* After all other threads have stopped: */
	CSDestroySecurityManager();
	FCDestroy();
	OMDestroy();

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	return EXIT_SUCCESS;
}

static void CatchSignal(int signo) {
	if (signo == SIGINT)
		GSNotify(GSA_INTERRUPT);
}

static void StopWithError(const char *section, const char *message) {
	size_t i;

	for (i = 0; i < cufIndex; i++)
		cleanUpFunctions[i]();

	fclose(stdin);
	fclose(stdout);

	fprintf(stderr, ANSI_COLOR_RED"[Main] [%s] %s"ANSI_COLOR_RESETLN, section,
			message);

	fclose(stderr);
	exit(EXIT_FAILURE);
}
