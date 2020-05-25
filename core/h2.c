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

#include "h2.h"

#ifdef __FreeBSD__
	#include <sys/endian.h>
#else
	#include <endian.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/security.h"
#include "http2/debugging.h"
#include "http2/frames/goaway.h"
#include "http2/frames/priority.h"
#include "http2/frames/settings.h"
#include "http2/frames/window_update.h"
#include "http2/frame.h"
#include "http2/session.h"
#include "http2/stream.h"
#include "misc/default.h"

const char HTTP2Preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

/* Prototypes */
bool
checkPreface(struct H2Session *);

void CSHandleHTTP2(CSSClient client) {
	bool (*frameHandlers[])(struct H2Session *, struct H2Frame *) = {
		NULL, /* DATA */
		NULL, /* HEADERS */
		H2HandlePriority,
		NULL, /* RST_STREAM */
		H2HandleSettings,
		NULL, /* PUSH_PROMISE */
		NULL, /* PING */
		H2HandleGoaway,
		H2HandleWindowUpdate,
	};

	bool ret;
	struct H2Session *session;

	session = malloc(sizeof(*session));
	if (session == NULL) {
		fputs(ANSI_COLOR_RED"[H2] Failed to allocate data for session"
			  ANSI_COLOR_RESETLN, stderr);
		return;
	}

	session->client = client;
	session->streamCount = 0;
	session->streams = NULL;
	session->windowSize = 65535;

	if (!checkPreface(session)) {
		free(session);
		fputs(ANSI_COLOR_RED"[H2] Preface comparison failed"ANSI_COLOR_RESETLN,
			  stderr);
		return;
	}

	/* Server preface = (potentially empty) settings frame */
	if (!H2SendSettings(session, NULL, 0)) {
		fputs(ANSI_COLOR_RED"[H2] Failed to send settings"ANSI_COLOR_RESETLN,
			  stderr);
		return;
	}

	while (true) {
		if (!H2ReadFrame(session, &session->frameBuffer))
			break;

		printf("[#%u] Frame: %s\n", session->frameBuffer.stream,
			   H2DFrameTypeNames[session->frameBuffer.type]);

		if (session->frameBuffer.type
				< sizeof(frameHandlers) / sizeof(frameHandlers[0])
			&& frameHandlers[session->frameBuffer.type] != NULL) {
			ret = frameHandlers[session->frameBuffer.type]
								(session, &session->frameBuffer);
			if (!ret) {
				/* TODO */
			}
		} else
			printf(ANSI_COLOR_YELLOW"W: Ignored Frame %s"ANSI_COLOR_RESETLN,
				   H2DFrameTypeNames[session->frameBuffer.type]);

		/* TODO: The frameBuffer payload is literally re-malloc'ed, maybe use a
		 * size_t + realloc solution? */
		free(session->frameBuffer.payload);
	}

	free(session);
	puts(ANSI_COLOR_MAGENTA " === Connection Closed ===" ANSI_COLOR_RESET);
}

bool
checkPreface(struct H2Session *session) {
	char buf[24];

	if (!CSSReadClient(session->client, buf, 24))
		return 0;

	return memcmp(buf, HTTP2Preface, 24) == 0;
}
