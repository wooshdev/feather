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

#include <sys/socket.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "redir/client.h"

typedef enum { FALSE, TRUE } bool;

struct Test {
	bool (*function)(void);
	const char *name;
};

bool TestUsual(void);
bool TestShort(void);
bool TestHTTP0_9(void);
bool PayloadTest(const void *, size_t, void *, size_t);

int main(void) {
	size_t i;

	struct Test tests[] = {
		{ TestUsual, "UsualRequest" },
		{ TestShort, "ShortRequest" },
		{ TestHTTP0_9, "HTTP/0.9" },
	};

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		bool ret;

		printf("Running test %s...", tests[i].name);
		ret = tests[i].function();

		if (ret)
			puts("ok");
		else {
			printf("\rRunning test %s...failed", tests[i].name);
			close(2);
			close(1);
			close(0);
			return EXIT_FAILURE;
		}
	}

	close(2);
	close(1);
	close(0);

	return EXIT_SUCCESS;
}

bool TestUsual(void) {
	const unsigned char payload[] =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	unsigned char result[128];

	/* Yes, - sizeof(payload[0]) is intended; it removes the NULL character */
	return PayloadTest(payload, sizeof(payload) - sizeof(payload[0]),
					   result, 128);
}

bool TestShort(void) {
	const unsigned char payload[] =
		"GET / HTTP/1.1\r\n"
		"\r\n";

	unsigned char result[128];

	/* Yes, - sizeof(payload[0]) is intended; it removes the NULL character */
	return PayloadTest(payload, sizeof(payload) - sizeof(payload[0]),
					   result, 128);
}

bool TestHTTP0_9(void) {
	const unsigned char payload[] = "GET / HTTP/1.1\r\n";

	unsigned char result[128];

	/* Yes, - sizeof(payload[0]) is intended; it removes the NULL character */
	return PayloadTest(payload, sizeof(payload) - sizeof(payload[0]),
					   result, 128);
}

bool PayloadTest(const void *inBuf, size_t inBufSize, void *outBuf,
				 size_t outBufSize) {
	int fd[2];
	/* fd[0] is for our simulated remote endpoint; the client
	 * fd[1] is for our simulated local endpoint; the server
	 */

	const uint8_t *buf;
	size_t len;
	char *path;
	char *result;
	ssize_t ret;

	buf = inBuf;
	result = outBuf;

	/* Setup communication */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fd) == -1) {
		perror("[PayloadTest] Error: socketpair() failure");
		return 0;
	}

	/* Write payload */
	len = inBufSize;
	do {
		ret = write(fd[0], buf, len);
		if (ret == -1) {
			perror("[PayloadTest] Failed to write payload");
			close(fd[0]);
			close(fd[1]);
			return 0;
		}

		buf += ret;
		len -= ret;
	} while (len != 0);

	path = malloc(256); /* Same as in base/global_state.c */
	if (!path) {
		close(fd[0]);
		close(fd[1]);
		return 0;
	}

	/* Execute */
	RSChildHandler(fd[1], path);

	/* Check result */
	/* Example: "HTTP/1.1 301 Moved Permanently" */
	ret = read(fd[0], result, outBufSize);

	if (ret < 13) {
		fprintf(stderr, "[PayloadTest] [Error] Malformed Response! read() "
				"retval: %zi\n", ret);
		close(fd[0]);
		close(fd[1]);
		return 0;
	}

	/* Check if status wasn't a 3xx code */
	if (result[9] != '3') {
		unsigned char debugInfo[4];
		debugInfo[0] = result[9];
		debugInfo[1] = result[10];
		debugInfo[2] = result[11];
		debugInfo[3] = '\0';

		fprintf(stderr, "[PayloadTest] [Error] Non 3xx status code! Status "
				"code was '%s'\n", debugInfo);
		close(fd[0]);
		close(fd[1]);
		return 0;
	}

	/* Clean Up */
	ret = close(fd[1]);
	if (ret == -1) {
		fputs("[PayloadTest] [Error] RSChildHandler isn't supposed to close "
			  "the socket!\n", stderr);
		return 0;
	}

	close(fd[0]);

	return 1;
}
