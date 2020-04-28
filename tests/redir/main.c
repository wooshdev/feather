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
#include <sys/socket.h>
#include <sys/time.h>

#include <err.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct Test {
	int (*function)(int);
	const char *name;
};

int Test1(int);

int main(void) {
	int fd[2];
	size_t i;
	int ret;

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fd) == -1) {
		perror("E: socketpair() failure");
		return EXIT_FAILURE;
	}

	struct Test tests[] = {
		{ Test1, "Test1" }
	};

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		printf("Running test %s...", tests[i].name);
		ret = tests[i].function(0);

		if (ret) {
			puts("ok");
		} else {
			puts("failure");
			return EXIT_FAILURE;
		}
	}

	uint8_t buf0[2];
	uint8_t buf1[2];
	buf0[0] = '\0';
	buf1[0] = '\0';

	printf("write[0]: %zi\n", write(fd[0], "OK", 3));
	printf("write[1]: %zi\n", write(fd[1], "NO", 3));
	printf("read[0]: %zi\n", read(fd[0], buf0, 3));
	printf("read[1]: %zi\n", read(fd[1], buf1, 3));
	
	printf("buf0: %s\nbuf1: %s\n", buf0, buf1);

	close(fd[0]);
	close(fd[1]);

	return EXIT_SUCCESS;
}

int Test1(int sockfd) {
	(void)(sockfd);

	return 1;
}
