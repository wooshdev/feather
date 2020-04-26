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

#include "h1.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include "core/security.h"
#include "http/strings.h"

#include <string.h>

#include "cache/cache.h"
#include "core/timings.h"
#include "misc/default.h"
#include "http/syntax.h"

/**
 * I don't know how the character length from an integer can be derived, so I
 * use the following formula:
 * ceil(log(2^64 - 1))
 */
#define DOCUMENT_SIZE_CHARACTER_SIZE 19
#define METHOD_SIZE			64
#define PATH_SIZE			2048
#define VERSION_SIZE		8
#define HEADER_NAME_SIZE	64
#define HEADER_VALUE_SIZE	256
#define HEADER_STEP_SIZE 	8

#define TIME_FORMAT "%a, %d %b %Y %T GMT"

struct HTTPHeader {
	/* Even though a perfect-sized-buffer is better memory wise, doing a lot of
	 * allocations is worse CPU wise. */
	char name[HEADER_NAME_SIZE];
	char value[HEADER_VALUE_SIZE];
};

struct HTTPRequest {
	char	buffer[2];
	struct HTTPHeader	*headers;
	uint8_t headersSize;
	uint8_t headerCount;
	char	method[METHOD_SIZE];
	char	path[PATH_SIZE];
	char	version[VERSION_SIZE];
};

enum HTTPError {
	HTTP_ERROR_FILE_NOT_FOUND,
	HTTP_ERROR_HEADER_ALLOCATION_FAILURE,
	HTTP_ERROR_HEADER_EMPTY_NAME,
	HTTP_ERROR_HEADER_EMPTY_VALUE,
	HTTP_ERROR_HEADER_INVALID_NAME,
	HTTP_ERROR_HEADER_INVALID_VALUE,
	HTTP_ERROR_HEADER_NAME_TOO_LONG,
	HTTP_ERROR_HEADER_VALUE_TOO_LONG,
	HTTP_ERROR_METHOD_UNRECOGNIZED,
	HTTP_ERROR_METHOD_INVALID,
	HTTP_ERROR_METHOD_TOO_LONG,
	HTTP_ERROR_PATH_INVALID,
	HTTP_ERROR_PATH_TOO_LONG,
	HTTP_ERROR_VERSION_INVALID,
	HTTP_ERROR_VERSION_UNKNOWN,
	HTTP_ERROR_READ,
};

static const char messageFormat[] =
	"HTTP/1.1 %s\r\n"
	"Connection: keep-alive\r\n"
	"Content-Encoding: %s\r\n"
	"Content-Length: %zu\r\n"
	"Content-Type: %s\r\n"
	"Date: %s\r\n"
	"%s"
	"Referrer-Policy: no-referrer\r\n"
	"Server: WFS\r\n"
	"Strict-Transport-Security: max-age=31536000\r\n"
	"X-Content-Type-Options: nosniff\r\n"
	"\r\n";

static const char messageNotModified[] =
	"HTTP/1.1 304 Not Modified\r\n"
	"Connection: keep-alive\r\n"
	"Date: %s\r\n"
	"Referrer-Policy: no-referrer\r\n"
	"Server: WFS\r\n"
	"Strict-Transport-Security: max-age=31536000\r\n"
	"X-Content-Type-Options: nosniff\r\n"
	"\r\n";


int handleRequest(CSSClient);
int handleRequestStage2(CSSClient, struct HTTPRequest *, struct Timings *);
int recoverError(CSSClient, enum HTTPError, struct HTTPRequest *);
int writeResponse(CSSClient);

void CSHandleHTTP1(CSSClient client) {
	int status;
	clock_t after;
	clock_t before;

	do {
		before = clock();
		status = handleRequest(client);
		after = clock();
		printf("handleRequest> took %f ms\n", (after - before)*1e3/CLOCKS_PER_SEC);
	} while(status);
}

int handleRequest(CSSClient client) {
	float diff;
	struct HTTPHeader *newHeaders;
	size_t pos;
	size_t ret;
	struct HTTPRequest *request;
	struct Timings timings;
	const char *timingsBufferingUnit;
	size_t timingsBufferingValue;

	request = malloc(sizeof(struct HTTPRequest));
	if (!request)
		return 0;

	request->headers = NULL;
	request->headerCount = 0;
	request->headersSize = 0;

	timings.flags = 0;

	/* Buffering */
	/* Try to put some data into the internal buffer, for timings' sake. */
	timings.buffering.before = clock();
	ret = CSSReadClient(client, request->method, 1);
	timings.buffering.after = clock();
	if (!ret) {
		request->method[0] = '\0';
		return recoverError(client, HTTP_ERROR_READ, request);
	}

	/* Read method */
	timings.readMethod.before = clock();
	pos = 0;
	do {
		if (request->method[pos] == ' ') {
			request->method[pos] = '\0';
			break;
		}

		if (!HTTPIsTokenCharacter(request->method[pos])) {
			request->method[pos] = '\0';
			return recoverError(client, HTTP_ERROR_METHOD_INVALID, request);
		}

		if (++pos > METHOD_SIZE) {
			request->method[pos - 1] = '\0';
			return recoverError(client, HTTP_ERROR_METHOD_TOO_LONG, request);
		}

		ret = CSSReadClient(client, request->method + pos, 1);
		if (!ret) {
			request->method[pos] = '\0';
			return recoverError(client, HTTP_ERROR_READ, request);
		}
	} while(1);
	timings.readMethod.after = clock();

	/* Read path */
	timings.readPath.before = clock();
	pos = 0;
	do {
		ret = CSSReadClient(client, request->path + pos, 1);
		if (!ret) {
			request->path[pos] = '\0';
			return recoverError(client, HTTP_ERROR_READ, request);
		}

		if (request->path[pos] == ' ') {
			request->path[pos] = '\0';
			break;
		}

		/* TODO Verify request-target syntax conforming to RFC 7230 § 5.3 */
		if (request->path[pos] == '\0')
			return recoverError(client, HTTP_ERROR_PATH_INVALID, request);

		if (++pos > PATH_SIZE) {
			request->path[pos - 1] = '\0';
			return recoverError(client, HTTP_ERROR_PATH_TOO_LONG, request);
		}
	} while(1);
	timings.readPath.after = clock();

	/* Read version */
	timings.readVersion.before = clock();
	if (!CSSReadClient(client, request->version, 8))
		return recoverError(client, HTTP_ERROR_READ, request);
	if (strncmp("HTTP/", request->version, 5) != 0)
		return recoverError(client, HTTP_ERROR_VERSION_INVALID, request);
	if (request->version[6] != '.')
		return recoverError(client, HTTP_ERROR_VERSION_INVALID, request);
	if (request->version[5] != '1' && request->version[7] != '1')
		return recoverError(client, HTTP_ERROR_VERSION_UNKNOWN, request);
	timings.readVersion.after = clock();

	/* Consume CRLF */
	if (!CSSReadClient(client, request->buffer, 2))
		return recoverError(client, HTTP_ERROR_READ, request);

	timings.readHeaders.before = clock();
	/* Parse headers */
	do {
		/* Check first two bytes to see if it is a CRLF token. */
		if (!CSSReadClient(client, request->buffer, 2))
			return recoverError(client, HTTP_ERROR_READ, request);
		if (request->buffer[0] == '\r' && request->buffer[1] == '\n')
			break;

		/* Check size of buffer */
		if (request->headerCount == request->headersSize) {
			request->headersSize += HEADER_STEP_SIZE;
			newHeaders = realloc(request->headers, request->headersSize 
									* sizeof(struct HTTPHeader));
			if (!newHeaders) {
				free(request->headers);
				request->headers = NULL;
				return recoverError(client, 
									HTTP_ERROR_HEADER_ALLOCATION_FAILURE,
									request);
			}
			request->headers = newHeaders;
		}

		/* Read field-name */
		request->headers[request->headerCount].name[0] = request->buffer[0];
		request->headers[request->headerCount].name[1] = request->buffer[1];

		if (!HTTPIsTokenCharacter(request->buffer[0]) || 
			!HTTPIsTokenCharacter(request->buffer[1]))
			return recoverError(client, HTTP_ERROR_HEADER_INVALID_NAME,
								request);

		pos = 2;
		while (1) {
			if (!CSSReadClient(client, request->buffer, 1))
				return recoverError(client, HTTP_ERROR_READ, request);
			if (request->buffer[0] == ':')
				break;
			if (!HTTPIsTokenCharacter(request->buffer[0]))
				return recoverError(client, HTTP_ERROR_HEADER_INVALID_NAME, 
									request);
			request->headers[request->headerCount].name[pos++] =
				request->buffer[0];
			if (pos == HEADER_NAME_SIZE) {
				request->headers[request->headerCount].name[pos - 1] = '\0';
				return recoverError(client, HTTP_ERROR_HEADER_NAME_TOO_LONG,
								request);
			}
		}
		request->headers[request->headerCount].name[pos] = '\0';

		if (pos == 0)
			return recoverError(client, HTTP_ERROR_HEADER_EMPTY_NAME, request);

		/* Consume OWS */
		while (1) {
			if (!CSSReadClient(client, request->buffer, 1))
				return recoverError(client, HTTP_ERROR_READ, request);
			if (request->buffer[0] != '\t' && request->buffer[0] != ' ')
				break;
		}

		/* Consume field-value */
		pos = 0;
		do {
			if (request->buffer[0] == '\r') {
				if (!CSSReadClient(client, request->buffer, 1))
					return recoverError(client, HTTP_ERROR_READ, request);
				if (request->buffer[0] != '\n')
					return recoverError(client, HTTP_ERROR_HEADER_INVALID_VALUE, request);
				request->headers[request->headerCount].value[pos] = '\0';
				break;
			}

			if (request->buffer[0] != '\t' && request->buffer[0] != ' ' && 
				(request->buffer[0] < 0x21 || request->buffer[0] > 0x7E)) {
				request->headers[request->headerCount].value[pos - 1] = '\0';
				return recoverError(client, HTTP_ERROR_HEADER_INVALID_VALUE, 
									request);
			}
			request->headers[request->headerCount].value[pos++] =
				request->buffer[0];

			if (!CSSReadClient(client, request->buffer, 1)) {
				request->headers[request->headerCount].value[pos - 1] = '\0';
				return recoverError(client, HTTP_ERROR_READ, request);
			}
		} while (1);

		/* Trim end of OWS token:
		 * OWS = *( SP / HTAB ) */
		char *lastTab = strrchr(
			request->headers[request->headerCount].value, '\t');
		char *lastSpace = strrchr(
			request->headers[request->headerCount].value, '\t');

		if (lastTab)
			if (lastSpace)
				if (lastSpace < lastTab)
					*lastSpace = '\0';
				else
					*lastTab = '\0';
			else
				*lastTab = '\0';
		else if (lastSpace)
			*lastSpace = '\0';

		/* Check if the value was empty */
		if (request->headers[request->headerCount].value[0] == '\0')
			return recoverError(client, HTTP_ERROR_HEADER_EMPTY_VALUE,
								request);
		request->headerCount += 1;
	} while (1);
	timings.readHeaders.after = clock();

	strncpy(timings.path, request->path, 256);

	/* TODO Check if there was a [ message-body ] */

	/* Handling */
	timings.handling.before = clock();
	ret = handleRequestStage2(client, request, &timings);
	timings.handling.after = clock();

	diff = (timings.buffering.after - timings.buffering.before)
			* 1.0 / CLOCKS_PER_SEC;

	if (diff > 1e-9) {
		if (diff > 1e-6) {
			if (diff > 1e-3) {
				if (diff > 1) {
					timingsBufferingUnit = "s";
					timingsBufferingValue = diff;
				} else {
					timingsBufferingUnit = "ms";
					timingsBufferingValue = diff * 1e3;
				}
			} else {
				timingsBufferingUnit = "μs";
				timingsBufferingValue = diff * 1e6;
			}
		} else {
			timingsBufferingUnit = "ns";
			timingsBufferingValue = diff * 1e9;
		}
	} else {
		timingsBufferingUnit = "ps";
		timingsBufferingValue = diff * 1e12;
	}

	printf(ANSI_COLOR_MAGENTA"Timing>"ANSI_COLOR_GREEN" Path(\"%s\") "
		ANSI_COLOR_BLUE"Buffering(%zu %s) ReadMethod(%zu μs) ReadPath(%zu μs) "
		"ReadVersion(%zu μs) ReadHeaders (%zu μs) Handling(%zu μs)"
		ANSI_COLOR_YELLOW"%s%s%s"ANSI_COLOR_RESETLN, timings.path,
		timingsBufferingValue, timingsBufferingUnit,
		(size_t) ((timings.readMethod.after - timings.readMethod.before) *
		(size_t) 1e6 / CLOCKS_PER_SEC),
		(size_t) ((timings.readPath.after - timings.readPath.before) * 
		(size_t) 1e6 / CLOCKS_PER_SEC),
		(size_t) ((timings.readVersion.after - timings.readVersion.before) * 
		(size_t) 1e6 / CLOCKS_PER_SEC),
		(size_t) ((timings.readHeaders.after - timings.readHeaders.before) * 
		(size_t) 1e6 / CLOCKS_PER_SEC),
		(size_t) ((timings.handling.after - timings.handling.before) * 
		(size_t) 1e6 / CLOCKS_PER_SEC),
		(timings.flags & TF_CLIENT_CACHED ? ANSI_COLOR_BLUE";"ANSI_COLOR_YELLOW
			" client-cached" : ""),
		(timings.flags & TF_COMPRESSED ? ANSI_COLOR_BLUE";"ANSI_COLOR_YELLOW
			" compressed" : ""),
		(timings.flags & TF_NOT_FOUND ? ANSI_COLOR_BLUE";"ANSI_COLOR_YELLOW
			" not-found" : "")
	);

	return ret;
}

int recoverError(CSSClient client, enum HTTPError error,
				 struct HTTPRequest *request) {
	char *buf;
	char date[32];
	size_t formattedBufSize;
	int ret;
	time_t actualTime;
	struct tm brokenDownTime;

	/* The connection has probably been closed, so in this case we shouldn't
	 * try to prepare and send a special error message. */
	if (error == HTTP_ERROR_READ) {
		puts("ERROR: FAILED TO READ");
		free(request->headers);
		free(request);
		return 0;
	}

	const char *const names[] = {
		"HTTP_ERROR_FILE_NOT_FOUND",
		"HTTP_ERROR_HEADER_ALLOCATION_FAILURE",
		"HTTP_ERROR_HEADER_EMPTY_NAME",
		"HTTP_ERROR_HEADER_EMPTY_VALUE",
		"HTTP_ERROR_HEADER_INVALID_NAME",
		"HTTP_ERROR_HEADER_INVALID_VALUE",
		"HTTP_ERROR_HEADER_NAME_TOO_LONG",
		"HTTP_ERROR_HEADER_VALUE_TOO_LONG",
		"HTTP_ERROR_METHOD_UNRECOGNIZED",
		"HTTP_ERROR_METHOD_INVALID",
		"HTTP_ERROR_METHOD_TOO_LONG",
		"HTTP_ERROR_PATH_INVALID",
		"HTTP_ERROR_PATH_TOO_LONG",
		"HTTP_ERROR_VERSION_INVALID",
		"HTTP_ERROR_VERSION_UNKNOWN",
		"HTTP_ERROR_READ"
	};

	printf("error was: %s (client is %p, request is %p)\n", names[error], 
		   (void *) client, (void *) request);

	/* We're done with request handling and after this we're just preparing the
	 * response, so the request object isn't needed anymore and can be released
	 */
	free(request->headers);
	free(request);
	request = NULL;

	/* TODO Create a 'personalized' error message for each error. */
	const char document[] = 
		"<!doctype html>"
		"<html>"
		"<head>"
		"<title>404 Not Found</title>"
		"</head>"
		"<body>"
		"<h1>File Not Found</h1>"
		"</body>"
		"</html>";

	/* Create Media Information (MIME) */
	const char mediaType[] = "text/html;charset=utf-8";
	const char encoding[] = "identity";

	/* Create Date header value */
	time(&actualTime);
	localtime_r(&actualTime, &brokenDownTime);
	strftime(date, 32, TIME_FORMAT, localtime(&actualTime));

	buf = malloc(
		strlen(messageFormat) +
		strlen(HTTPStatus404NotFound) +
		sizeof(encoding) / sizeof(encoding[0]) - 1 +
		DOCUMENT_SIZE_CHARACTER_SIZE +
		sizeof(mediaType) / sizeof(mediaType[0]) - 1 +
		1
	);

	if (!buf) {
		perror("Allocation failure");
		return 0;
	}

	formattedBufSize = sprintf(buf, messageFormat,
		HTTPStatus404NotFound,
		encoding,
		sizeof(document) / sizeof(document[0]) - 1,
		mediaType,
		date,
		""
	);

	ret = CSSWriteClient(client, buf, formattedBufSize);
	free(buf);

	if (!ret)
		return 0;

	ret = CSSWriteClient(client, document, 
						 sizeof(document) / sizeof(document[0]) - 1);

	if (!ret)
		return 0;

	switch (error) {
		/* Errors that don't affect the connection should return 1, and errors
		 * that do should return 0. */
		case HTTP_ERROR_FILE_NOT_FOUND:
			return 1;
		default:
			return 0;
	}
}

int handleRequestStage2(CSSClient client, struct HTTPRequest *request,
						struct Timings *timings) {
	/* TODO Improve variable naming. */
	char *buf;
	char date[32]; /* "Sun, 06 Nov 1994 08:49:37 GMT" = 29 characters */
	char dateLastModified[64];
	size_t formattedBufSize;
	size_t i;
	int isUnchanged = 0;
	size_t len;
	char mediaInfo[128];
	struct FCResult result;
	int ret;

	time_t actualTime;
	struct tm brokenDownTime;

	memset(&result, 0, sizeof(struct FCResult));

	/* TODO Parse 'Accept-Encoding' */
	ret = FCLookup(request->path, &result, FCF_BROTLI | FCF_GZIP);

	if (!ret) {
		timings->flags |= TF_NOT_FOUND;
		return recoverError(client, HTTP_ERROR_FILE_NOT_FOUND, request);
	}

	/* Create Media Information (MIME) */
	len = strlen(result.mediaType);
	memcpy(mediaInfo, result.mediaType, len);
	if (result.mediaCharset != NULL) {
		memcpy(mediaInfo + len, ";charset=", 9);
		strcpy(mediaInfo + len + 9, result.mediaCharset);
	} else {
		mediaInfo[len] = 0;
	}

	/* Create Date header value */
	time(&actualTime);
	localtime_r(&actualTime, &brokenDownTime);
	strftime(date, 32, TIME_FORMAT, localtime(&actualTime));

	/* Create Last-Modified Date header value */
	actualTime = result.modificationDate;
	localtime_r(&actualTime, &brokenDownTime);
	strcpy(dateLastModified, "Last-Modified: ");
	len = strftime(dateLastModified + 15, 64 - 15 - 2,
				   TIME_FORMAT,
				   localtime(&actualTime));

	/* Check if it is unchanged */
	for (i = 0; i < request->headerCount; i++) {
		if (strcasecmp(request->headers[i].name, "If-Modified-Since") == 0) {
			if (strcmp(request->headers[i].value, dateLastModified+15) == 0) {
				isUnchanged = 1;
			} else {
				printf("WARNING: current '%s' is NOT equal to client's '%s'\n",
					   dateLastModified + 15, request->headers[i].value);
			}
			break;
		}
	}

	/* Done with request handling, so resources can be released: */
	free(request->headers);
	free(request);

	if (result.encoding != MTE_none) {
		timings->flags |= TF_COMPRESSED;
	}

	if (isUnchanged) {
		timings->flags |= TF_CLIENT_CACHED;

		buf = malloc(strlen(messageNotModified) + strlen(date) + 1);
		if (!buf) {
			perror("Allocation failure");
			return 0;
		}

		formattedBufSize = sprintf(buf, messageNotModified, date);
		
		ret = CSSWriteClient(client, buf, formattedBufSize);
		free(buf);
		return ret;
	}

	/* Follow-up for the dateLastModified header */
	len += 15; /* starting position of header value. */
	dateLastModified[len] = '\r';
	dateLastModified[len + 1] = '\n';
	dateLastModified[len + 2] = '\0';

	/* DYNAMICALLY allocate a buffer for the headers */
	buf = malloc(
		strlen(messageFormat) +
		strlen(HTTPStatus200OK) +
		strlen(result.encoding) +
		DOCUMENT_SIZE_CHARACTER_SIZE +
		strlen(mediaInfo) + 
		strlen(date) + 
		strlen(dateLastModified) + 
		1
	);

	if (!buf) {
		perror("Allocation failure");
		return 0;
	}

	formattedBufSize = sprintf(buf, messageFormat,
		HTTPStatus200OK,
		result.encoding,
		result.size,
		mediaInfo,
		date,
		dateLastModified
	);

	ret = CSSWriteClient(client, buf, formattedBufSize);
	free(buf);

	if (!ret)
		return 0;

	ret = CSSWriteClient(client, result.data, result.size);
	printf("CSSWriteClient written %zu octets!\n", result.size);

	return ret;
}
