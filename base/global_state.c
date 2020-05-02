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

#include "global_state.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/utsname.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "misc/default.h"
#include "misc/io.h"
#include "misc/options.h"
#include "misc/statistics.h"

#define LINUX_DIST_FILE "/etc/lsb-release"

/* From the header file */
int GSMainLoop;

pthread_t GSCoreThread;
pthread_t GSRedirThread;

int GSCoreThreadState;
int GSRedirThreadState;

int GSCoreSocket;
int GSRedirSocket;

const size_t GSMaxPathSize = 256;
char GSServerHostNameInternal[1024];
const char *GSServerHostName = GSServerHostNameInternal;
char internalProductName[1024];
const char *GSServerProductName = internalProductName;

static const char *GSParentNames[] = {
	"Core",
	"Redirection",
};

/**
 * Child Scheduling 
 */
/* How many children, a.ka. threads may be made */
static const size_t GSChildSize = 500;
static struct GSThread *GSChildThreads;
static pthread_mutex_t GSChildMutex = PTHREAD_MUTEX_INITIALIZER;

/* Prototyping */
int GSPopulateProductName(void);

void GSDestroy(void) {
	size_t i;
	struct timespec time;

	/* Send signal to threads */
	pthread_mutex_lock(&GSChildMutex);
	for (i = 0; i < GSChildSize; i++)
		if (GSChildThreads[i].state)
			pthread_kill(GSChildThreads[i].thread, SIGINT);
	pthread_mutex_unlock(&GSChildMutex);

	/* wait */
	time.tv_sec = 0;
	time.tv_nsec = 100000000; /* 100 ms */
	nanosleep(&time, NULL);
	
	/* kill if they don't stop */
	for (i = 0; i < GSChildSize; i++) {
		if (GSChildThreads[i].sockfd > -1) {
			close(GSChildThreads[i].sockfd);
			GSChildThreads[i].sockfd = -1;
		}

		if (GSChildThreads[i].state) {
			pthread_cancel(GSChildThreads[i].thread);
			pthread_join(GSChildThreads[i].thread, NULL);
		}
	}

	free(GSChildThreads);

	if (GSRedirSocket > -1) {
		close(GSRedirSocket);
		GSRedirSocket = -1;
	}

	if (GSCoreSocket > -1) {
		close(GSCoreSocket);
		GSCoreSocket = -1;
	}
}

static void GSPopulateHostName(void) {
	struct addrinfo  hints;
	struct addrinfo *info;
	int gai_result;

	GSServerHostNameInternal[1023] = '\0';
	gethostname(GSServerHostNameInternal, 1023);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if ((gai_result = getaddrinfo(GSServerHostNameInternal, "http", &hints,
								  &info)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_result));
		fputs(ANSI_COLOR_RED "[GlobalState] Requesting the address information"
			  "failed. This most likely occurred to your hostname not looping"
			  "back to your domain. Check your DNS settings or change your '"
			  "/etc/hosts' file.", stderr);
		exit(1);
	}

	/* TODO info is a linked list, so which domain should we choose? */
	strcpy(GSServerHostNameInternal, info->ai_canonname);

	freeaddrinfo(info);
}

int GSInit(void) {
	size_t i;

	GSMainLoop = 1;
	GSCoreSocket = -1;
	GSCoreThreadState = 0;
	GSRedirSocket = -1;
	GSRedirThreadState = 0;

	GSPopulateHostName();

	if (!GSPopulateProductName()) {
		perror(ANSI_COLOR_RED"[GSInit] Failed to populate the 'Server' header"
			   ANSI_COLOR_RESETLN);
		return 0;
	}

	GSChildThreads = calloc(GSChildSize, sizeof(struct GSThread));
	if (GSChildThreads == NULL) {
		perror(ANSI_COLOR_RED"[GSInit] Failed to allocate"ANSI_COLOR_RESETLN);
		return 0;
	}

	for (i = 0; i < GSChildSize; i++)
		GSChildThreads[i].sockfd = -1;

	GSCoreSocket = IOCreateSocket(443, 1);

	if (GSCoreSocket < 0) {
		printf(ANSI_COLOR_RED"[GSInit] Failed to create GSCoreSocket: %s"
			ANSI_COLOR_RESETLN, IOErrors[-GSCoreSocket]);
		return 0;
	}

	GSRedirSocket = IOCreateSocket(80, 1);

	if (GSRedirSocket < 0) {
		printf(ANSI_COLOR_RED"[GSInit] Failed to create GSRedirSocket: %s"
			ANSI_COLOR_RESETLN, IOErrors[-GSRedirSocket]);
		return 0;
	}

	return 1;
}

void GSNotify(enum GSAction action) {
	switch (action) {
		case GSA_INTERRUPT:
			GSMainLoop = 0;
			break;
		default:
			printf("[GSNotify] Unknown GSAction: %X\n", action);
	}
}

int GSScheduleChildThread(enum GSThreadParent parent,
						  void *(*routine) (void *), int sockfd) {
	int	state;
	struct GSThread *thread;

	/* Statistics */
	SMNotifyRequest();

	pthread_mutex_lock(&GSChildMutex);
	{
		size_t i;

		for (i = 0; i < GSChildSize; i++) {
			thread = &GSChildThreads[i];
			if (!thread->state)
				break;
		}

		if (thread->state) {
			fputs(ANSI_COLOR_RED"[GSScheduleChildThread] All threads are in"
				" use at the moment."ANSI_COLOR_RESETLN, stderr);
			return 0;
		}

		thread->sockfd = sockfd;
		thread->state = 1;
	}
	pthread_mutex_unlock(&GSChildMutex);


	state = pthread_create(&thread->thread, NULL, routine, thread);
	if (state != 0) {
		char *buf;

		buf = malloc(256);
		if (buf != NULL) {
			strerror_r(state, buf, 256);
			fprintf(stderr, ANSI_COLOR_RED"[GSScheduleChildThread] Failed to"
				" create thread (parent: %s): %s!"ANSI_COLOR_RESETLN, buf,
				GSParentNames[parent]);
			free(buf);
		}

		pthread_mutex_lock(&GSChildMutex);
		{
			thread->sockfd = -1;
			thread->state = 0;
		}
		pthread_mutex_unlock(&GSChildMutex);

		return 0;
	}

	pthread_detach(thread->thread);

	return 1;
}


void GSChildThreadRelease(struct GSThread *thread) {
	pthread_mutex_lock(&GSChildMutex);

	if (thread->sockfd > -1) {
		close(thread->sockfd);
		thread->sockfd = -1;
	}

	thread->state = 0;

	pthread_mutex_unlock(&GSChildMutex);
}

int GSPopulateProductName(void) {
	memcpy(internalProductName, "WFS", 4);

	if (OMGSSystemInformationInServerHeader == OSIL_NONE)
		return 1;

	char	*str;
	size_t	 len;
	size_t	 sizeLeft;
	int		 warn; /* bool */

	struct utsname systemName;

	internalProductName[3] = ' ';
	internalProductName[4] = '(';
	str = internalProductName + 5;

	sizeLeft = sizeof(internalProductName) / sizeof(internalProductName[0]) - 5;
	warn = 0;

	if (uname(&systemName) == -1) {
		perror("uname");
		fputs(ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] Failed to "
			  "retrieve system information."ANSI_COLOR_RESETLN, stdout);
		return 0;
	}

	if (OMGSSystemInformationInServerHeader & OSIL_SYSNAME) {
		len = strlen(systemName.sysname);
		if (len + 1 > sizeLeft - 1) {
			fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E:"
				   " The variable 'sysname' doesn't fit! value='%s' (%zu"
					" characters)"ANSI_COLOR_RESETLN, systemName.sysname, len);
			return 0;
		}

		memcpy(str, systemName.sysname, len);
		str[len] = ' ';
		str += len + 1;
	}

	if (OMGSSystemInformationInServerHeader & OSIL_NODENAME) {
		len = strlen(systemName.nodename);
		if (len + 1 > sizeLeft - 1) {
			fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E:"
				   " The variable 'nodename' doesn't fit! value='%s' (%zu"
					" characters)"ANSI_COLOR_RESETLN, systemName.nodename, len);
			return 0;
		}

		memcpy(str, systemName.nodename, len);
		str[len] = ' ';
		str += len + 1;
	}

	if (OMGSSystemInformationInServerHeader & OSIL_RELEASE) {
		warn = 1;
		len = strlen(systemName.release);
		if (len + 1 > sizeLeft - 1) {
			fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E:"
				   " The variable 'release' doesn't fit! value='%s' (%zu"
					" characters)"ANSI_COLOR_RESETLN, systemName.release, len);
			return 0;
		}

		memcpy(str, systemName.release, len);
		str[len] = ' ';
		str += len + 1;
	}

	if (OMGSSystemInformationInServerHeader & OSIL_MACHINE) {
		warn = 1;
		len = strlen(systemName.machine);
		if (len + 1 > sizeLeft - 1) {
			fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E: "
				   "'machine' is too big! machine='%s' (%zu characters)"
					ANSI_COLOR_RESETLN, systemName.machine, len);
			return 0;
		}

		memcpy(str, systemName.machine, len);
		str[len] = ' ';
		str += len + 1;
	}

#ifdef __linux__
	if (OMGSSystemInformationInServerHeader & OSIL_DISTNAME ||
		OMGSSystemInformationInServerHeader & OSIL_DISTVERSION ||
		OMGSSystemInformationInServerHeader & OSIL_DISTCODENAME ||
		OMGSSystemInformationInServerHeader & OSIL_DISTDESCRIPTION
	) {
		char	*buf;
		char	*chrEq;
		char	*chrLn;
		int		 fd;
		size_t	 llen, pos, valueSize;
		struct stat fileStatus;

		fd = open(LINUX_DIST_FILE, O_RDONLY);

		if (fd == -1) {
			perror(ANSI_COLOR_RED"Failed to open file"ANSI_COLOR_RESET);
			fputs(ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E: Failed to"
				  " open the distribution information file: "LINUX_DIST_FILE
				  ANSI_COLOR_RESETLN, stderr);
			return 0;
		}

		/* Stat() the file to retrieve the file size which is used to allocate
		 * the buffer 'buf'. */
		errno = 0;
		if (fstat(fd, &fileStatus) == -1 || !S_ISREG(fileStatus.st_mode)) {
			if (errno == 0)
				fputs(ANSI_COLOR_RED"Failed to access file because it isn't a "
					  "regular file."ANSI_COLOR_RESET, stderr);
			else
				perror(ANSI_COLOR_RED"Failed to stat file"ANSI_COLOR_RESET);

			close(fd);
			fputs(ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E: Failed to"
				  " check the status of the distribution information file: "
				  LINUX_DIST_FILE""ANSI_COLOR_RESETLN, stderr);
			return 0;
		}

		/* Allocate the buffer 'buf'. In this buffer the contents of the file
		 * will be stored. */
		buf = malloc(fileStatus.st_size + 1);
		if (buf == NULL) {
			perror(ANSI_COLOR_RED"Failed to malloc"ANSI_COLOR_RESET);
			fputs(ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E: Failed to"
				  " allocate memory for the distribution information file: "
				  LINUX_DIST_FILE""ANSI_COLOR_RESET, stderr);
			close(fd);
			return 0;
		}
		buf[fileStatus.st_size] = '\0';

		llen = fileStatus.st_size;
		pos = 0;

		/* Load the file into the buffer 'buf' */
		do {
			ssize_t ret;

			ret = read(fd, buf + pos, llen);
			if (ret < 0) {
				perror(ANSI_COLOR_RED"Failed to read()"ANSI_COLOR_RESET);
				fputs(ANSI_COLOR_RED"[GSInit] [GSPopulateProductName] E: "
					  "Failed to read from the distribution information file: "
					  LINUX_DIST_FILE""ANSI_COLOR_RESET, stderr);
				close(fd);
				free(buf);
				return 0;
			}

			llen -= ret;
			pos += ret;
		} while (llen > 0);

		close(fd);
		fd = 0;

		llen = fileStatus.st_size;
		pos = 0;

		/* Parse and append */
		do {
			chrEq = strchr(buf + pos, '=');

			if (chrEq == NULL)
				break;
			*chrEq = '\0';

			if (OMGSSystemInformationInServerHeader & OSIL_DISTNAME &&
				strcmp(buf + pos, "DISTRIB_ID") == 0) {
				chrLn = strchr(chrEq + 1, '\n');
				if (chrLn == NULL)
					break;
				valueSize = chrLn - chrEq - 1;
				if (len + valueSize > sizeLeft - 1) {
					free(buf);
					fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProduct"
							"Name] E: The variable 'distname' doesn't fit! %zu"
							" characters"ANSI_COLOR_RESETLN, valueSize);
					return 0;
				}

				memcpy(str, chrEq + 1, valueSize);

				str[valueSize] = ' ';
				valueSize += 1;

				str += valueSize;
				sizeLeft -= valueSize;
				pos = chrLn - buf + 1;
				llen -= chrLn - buf + 1 - pos;
			} else if (OMGSSystemInformationInServerHeader & OSIL_DISTVERSION
			 && strcmp(buf + pos, "DISTRIB_RELEASE") == 0) {
				chrLn = strchr(chrEq + 1, '\n');
				if (chrLn == NULL)
					break;
				valueSize = chrLn - chrEq - 1;
				if (len + valueSize > sizeLeft - 1) {
					free(buf);
					fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProduct"
							"Name] E: The variable 'distversion' doesn't fit!"
							" %zu characters"ANSI_COLOR_RESETLN, valueSize);
					return 0;
				}

				memcpy(str, chrEq + 1, valueSize);

				str[valueSize] = ' ';
				valueSize += 1;

				str += valueSize;
				sizeLeft -= valueSize;
				pos = chrLn - buf + 1;
				llen -= chrLn - buf + 1 - pos;
			} else if (OMGSSystemInformationInServerHeader & OSIL_DISTCODENAME
			 && strcmp(buf + pos, "DISTRIB_CODENAME") == 0) {
				chrLn = strchr(chrEq + 1, '\n');
				if (chrLn == NULL)
					break;
				valueSize = chrLn - chrEq - 1;
				if (len + valueSize > sizeLeft - 1) {
					free(buf);
					fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProduct"
							"Name] E: The variable 'distversion' doesn't fit!"
							" %zu characters"ANSI_COLOR_RESETLN, valueSize);
					return 0;
				}

				memcpy(str, chrEq + 1, valueSize);

				str[valueSize] = ' ';
				valueSize += 1;

				str += valueSize;
				sizeLeft -= valueSize;
				pos = chrLn - buf + 1;
				llen -= chrLn - buf + 1 - pos;
			} else if (OMGSSystemInformationInServerHeader & OSIL_DISTDESCRIPTION
			 && strcmp(buf + pos, "DISTRIB_DESCRIPTION") == 0) {
				int	isQuoted; /* bool */

				isQuoted = chrEq[1] == '"';

				chrLn = strchr(chrEq + 1, '\n');
				if (chrLn == NULL)
					break;

				valueSize = chrLn - chrEq - (isQuoted ? 3 : 1);
				if (len + valueSize > sizeLeft - 1) {
					free(buf);
					fprintf(stderr, ANSI_COLOR_RED"[GSInit] [GSPopulateProduct"
							"Name] E: The variable 'distversion' doesn't fit!"
							" %zu characters"ANSI_COLOR_RESETLN, valueSize);
					return 0;
				}

				memcpy(str, chrEq + 1 + (isQuoted ? 1 : 0), valueSize);

				str[valueSize] = ' ';
				valueSize += 1;

				str += valueSize;
				sizeLeft -= valueSize;
				pos = chrLn - buf + 1;
				llen -= chrLn - buf + 1 - pos;
			} else {
				chrLn = strchr(buf, '\n');
				if (chrLn == NULL)
					break;
				pos = chrLn - buf;
				llen -= chrLn - buf - pos;
			}
		} while (llen > 0);

		free(buf);
	}
#endif

	if (str != GSServerHostNameInternal) {
		str[-1] = ')';
		*str = '\0';
	} else {
		*str = ')';
		str[1] = '\0';
	}

	if (warn) {
		printf(ANSI_COLOR_YELLOW"[Warning] Using sensitive information about "
			   "your system is discouraged due to security reasons. It can "
			   "provide attackers with details about your system that they "
			   "can abuse. For example, if you were to forget to update your "
			   "system and a vulnerability was published, an attacker could "
			   "notice that your system is out-of-date, and use the "
			   "vulnerability maliciously. If you are running this server in a"
			   " restricted testing environment, you could to choose to ignore"
			   " this warning."ANSI_COLOR_RESETLN);
	}

	printf("GSPopulateProductName: '%s'\n", GSServerProductName);

	return 1;
}
