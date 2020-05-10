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

#include "io.h"

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *IOErrors[] = {
	/*  0 */"no error",
	/*  1 */"failed to open socket",
	/*  2 */"failed to enable SO_REUSEADDR",
	/*  3 */"failed to get socket flags (non-blocking)",
	/*  4 */"failed to set socket flags (non-blocking)",
	/*  5 */"failed to bind socket to address",
	/*  6 */"failed to listen for connections on socket",
	/*  7 */"failed to create directory",
};

int
IOCreateSocket(uint16_t port, bool nonBlocking) {
	struct sockaddr_in addr;
	int flag;
	int sockfd;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		return -1;

	flag = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int))
			== -1) {
		perror("IOCreateSocket setsockopt");
		close(sockfd);
		return -2;
	}

	if (nonBlocking) {
		int status;

		flag = fcntl(sockfd, F_GETFL);
		if (flag == -1) {
			perror("IOCreateSocket fcntl_get");
			close(sockfd);
			return -3;
		}

		status = fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
		if (status == -1) {
			perror("IOCreateSocket fcntl_set");
			close(sockfd);
			return -4;
		}
	}

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror(ANSI_COLOR_RED"[IOCreateSocket] E: bind name to socket failure"
			ANSI_COLOR_RESET);

		fprintf(stderr, ANSI_COLOR_RED
			  "\nFailed to use the %hu port specified above.\n"
			  ANSI_COLOR_RESET
			  "One of the following problems may have occurred:\n\n"
			  ANSI_COLOR_BLUE" • You don't have sufficied permisions.\n"
			  ANSI_COLOR_RESET"   This webserver uses ports below"
			  " 1024, which are restricted.\n"
			  "   On Linux, check if the CAP_NET_BIND_SERVICE permission has "
			  "been granted.\n\n"
			  ANSI_COLOR_BLUE" • A web server is already running.\n"
			  ANSI_COLOR_RESET"   You can check if this is the case by "
			  "executing the following command: \n      sudo netstat -nlp | "
			  "grep :%hu\n   If nothing pops up, no service is using the port."
			  ANSI_COLOR_RESETLN"\n",
			  port, port);

		close(sockfd);
		return -5;
	}

	if (listen(sockfd, 1) == -1) {
		perror("IOCreateSocket listen");
		close(sockfd);
		return -6;
	}

	return sockfd;
}

bool
IOTimeoutAvailableData(int fd, size_t microTimeout) {
	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set);
	FD_SET(fd, &set);

	timeout.tv_sec = 0;
	timeout.tv_usec = microTimeout;

	return select(fd+1, &set, NULL, NULL, &timeout) > 0;
}

int
IOMkdirRecursive(const char *dir) {
	char buf[256];
	char *p;
	size_t len;

	snprintf(buf, sizeof(buf),"%s",dir);
	len = strlen(buf);
	if(buf[len - 1] == '/')
		buf[len - 1] = 0;
	for(p = buf + 1; *p; p++)
		if(p[0] == '/') {
			p[0] = 0;
			if (mkdir(buf, S_IRWXU) == -1 && errno != EEXIST)
				return -7;
			p[0] = '/';
		}

	if (mkdir(buf, S_IRWXU) == -1 && errno != EEXIST)
		return -7;

	return 0;
}
