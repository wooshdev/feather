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

#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "base/global_state.h"
#include "http/response_headers.h"
#include "http/syntax.h"
#include "misc/default.h"
#include "misc/io.h"

const char redirFormat[] = "HTTP/1.1 301 Moved Permanently\r\n"
						   "Connection: close\r\n"
						   "Content-Length: 0\r\n"
						   "Date: %s\r\n"
						   "Location: https://%s%s\r\n"
						   "Server: %s\r\n"
						   "\r\n";

/* Prototypes */
static bool
ReadPath(int, char *);

void
RSChildHandler(int sockfd) {
	char *date;
	char *path;
	ssize_t state;

	if (!IOTimeoutAvailableData(sockfd, 10000)) {
		fputs(ANSI_COLOR_RED"[RSChildHandler] Timeout.\n", stderr);
		return;
	}

	path = malloc(GSMaxPathSize);
	if (!path)
		return;

	/* skip method + space */
	do {
		ssize_t ret;

		ret = read(sockfd, path, 1);

		if (ret != 1) {
			state = -1;
			break;
		}

		if (!HTTPIsTokenCharacter(path[0])) {
			if (path[0] == 0)
				state = 0; /* method is empty */
			else if (path[0] == ' ')
				state = 1;
			else
				state = -2; /* invalid character */
			break;
		}
	} while(1);

	if (state < 0) {
		/* I/O error */
		free(path);
		return;
	}

	if (!ReadPath(sockfd, path)) {
		free(path);
		return;
	}

	date = HTTPCreateDateCurrent();
	if (!date) {
		free(path);
		return;
	}

	dprintf(sockfd, redirFormat, date, GSServerHostName, path, 
			GSServerProductName);

	free(path);
	free(date);
}

static bool
ReadPath(int sockfd, char *buf) {
	do {
		ssize_t ret;

		ret = read(sockfd, buf, 1);

		if (ret != 1)
			return FALSE;

		if (buf[0] == ' ') {
			buf[0] = 0;
			return TRUE;
		}

		buf += 1;
	} while (TRUE);
}
