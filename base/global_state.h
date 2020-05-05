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
 *
 * GS is an of abbreviation Global State.
 */

#ifndef BASE_GLOBAL_STATE_H
#define BASE_GLOBAL_STATE_H

#include <pthread.h>
#include <stddef.h>

#include "misc/default.h"

struct GSThread {
	int			 state;
	pthread_t	 thread;
	int			 sockfd;
};

enum GSAction {
	/* ^C was pressed */
	GSA_INTERRUPT,
};

enum GSThreadParent {
	GSTP_CORE,
	GSTP_REDIR
};

/* Boolean */
extern int GSMainLoop;

extern pthread_t GSCoreThread;
extern pthread_t GSRedirThread;

extern int GSCoreThreadState;
extern int GSRedirThreadState;

extern int GSCoreSocket;
extern int GSRedirSocket;

/* Some options (maybe these should be moved to another file) */
extern const size_t	 GSMaxPathSize;
extern const char	*GSServerHostName;
extern const char	*GSServerProductName;

void
GSChildThreadRelease(struct GSThread *);

void
GSDestroy(void);

bool
GSInit(void);

void
GSNotify(enum GSAction);

bool
GSScheduleChildThread(enum GSThreadParent, void *(*) (void *), int);

#endif /* BASE_GLOBAL_STATE_H */
 
