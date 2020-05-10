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

#include "response_headers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cache/cache.h"
#include "http/strings.h"
#include "misc/default.h"

/* "Sun, 20 Jan 2020 01:00:00 +0000" is 31 characters + 1 NULL long */
#define DATE_HEADER_BUFFER_SIZE 32

static const char MediaTypeJavaScript[] = "application/javascript";

struct MediaType {
	const char	*ext;
	const char	*type;
};

const struct MediaType mediaTypes[] = {
	{ "css",	"text/css" },
	{ "gif",	"image/gif" },
	{ "html",	"text/html" },
	{ "ico",	"image/vnd.microsoft.icon" },
	{ "jfi",	"image/jpeg" },
	{ "jif",	"image/jpeg" },
	{ "jig",	"image/jpeg" },
	{ "jpe",	"image/jpeg" },
	{ "jpg",	"image/jpeg" },
	{ "jpeg",	"image/jpeg" },
	{ "js",		MediaTypeJavaScript },
	{ "md",		"text/markdown" },
	{ "otc",	"font/otf" },
	{ "otf",	"font/otf" },
	{ "png",	"image/png" },
	{ "svg",	"image/svg+xml" },
	{ "tif",	"image/tiff" },
	{ "tiff",	"image/tiff" },
	{ "ttc",	"font/otf" },
	{ "tte",	"font/ttf" },
	{ "ttf",	"font/ttf" },
	{ "webp",	"image/webp" },
	{ "woff",	"font/woff" },
	{ "woff2",	"font/woff2" },
};

char *
HTTPCreateDate(time_t inputTime) {
	size_t len;
	char *buf;

	buf = calloc(DATE_HEADER_BUFFER_SIZE, sizeof(char));
	if (buf == NULL)
		return NULL;

	len = strftime(buf, DATE_HEADER_BUFFER_SIZE, "%a, %d %h %Y %T %z",
				   localtime(&inputTime));

	return realloc(buf, len + 1);
}

char *
HTTPCreateDateCurrent(void) {
	return HTTPCreateDate(time(NULL));
}

/* This is a subroutine of HTTPGetMediaTypeProperties(). */
void
guessMediaCharset(const char *file, struct FCEntry *entry) {
	UNUSED(file);

	/* TODO Guess charset */
	entry->mediaCharset = MTC_utf8;
}

void
HTTPGetMediaTypeProperties(const char *file, struct FCEntry *entry) {
	size_t i;
	const char *last;

	last = strrchr(file, '.');
	if (last == NULL) {
		entry->mediaType = MT_octetstream;
		return;
	}

	last += 1;

	for (i = 0; i < sizeof(mediaTypes) / sizeof(mediaTypes[0]); i++) {
		if (strcasecmp(last, mediaTypes[i].ext) == 0) {
			entry->mediaType = mediaTypes[i].type;

			if (strncmp(mediaTypes[i].type, "text/", 5) == 0 ||
				mediaTypes[i].type == MediaTypeJavaScript)
				guessMediaCharset(file, entry);

			return;
		}
	}

	entry->mediaType = MT_octetstream;
	entry->mediaCharset = NULL;
}
