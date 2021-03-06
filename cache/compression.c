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
#include "compression.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <brotli/encode.h>
#include <brotli/types.h>

#include "cache/cache.h"
#include "http/strings.h"
#include "misc/default.h"
#include "misc/io.h"
#include "misc/options.h"

/* Brotli Parameters */
int					BrotliQuality =	BROTLI_DEFAULT_QUALITY;
int					BrotliWindow =	BROTLI_DEFAULT_WINDOW;
BrotliEncoderMode	BrotliMode =	BROTLI_DEFAULT_MODE;

/* Subroutines */
bool
tryLoad(const char *, struct FCVersion *, const char *, time_t);

bool
trySave(const char *, struct FCVersion *, const char *);

bool
compressBrotli(const char *, struct FCEntry *);


bool
FCCompressionSetup(void) {
	return true;
}

void
FCCompressionDestroy(void) {
}

bool
FCCompressFile(const char *fileName, struct FCEntry *entry) {
	if (!compressBrotli(fileName, entry))
		return false;

	return true;
}

bool
compressBrotli(const char *fileName, struct FCEntry *entry) {
	size_t initialSize;
	char *newData;
	BROTLI_BOOL ret;

	entry->br.encoding = MTE_brotli;

	if (entry->uncompressed.size == 0)
		return true;

	if (tryLoad(fileName, &entry->br, entry->br.encoding,
		entry->modificationDate)) {
		return true;
	}

	initialSize = BrotliEncoderMaxCompressedSize(entry->uncompressed.size);
	entry->br.size = initialSize;
	if (initialSize == 0) {
		fprintf(stderr, ANSI_COLOR_RED"[Cache::compressBrotli] Size "
				"doesn't fit size_t! Input size is %zu"ANSI_COLOR_RESETLN,
				entry->uncompressed.size);
		return false;
	}

	entry->br.data = malloc(entry->br.size);
	if (!entry->br.data) {
		entry->br.size = 0;
		fputs(ANSI_COLOR_RED"[Cache::compressBrotli] Allocation error."
				ANSI_COLOR_RESETLN, stderr);
		return false;
	}

	ret = BrotliEncoderCompress(BrotliQuality, BrotliWindow, BrotliMode,
								entry->uncompressed.size,
								(const uint8_t *) entry->uncompressed.data,
								&entry->br.size,
								(uint8_t *) entry->br.data);
	if (ret == BROTLI_FALSE) {
		fputs(ANSI_COLOR_RED"[Cache::compressBrotli] Compression failure"
			  ANSI_COLOR_RESETLN, stderr);
		free(entry->br.data);
		entry->br.size = 0;
		entry->br.data = NULL;
		return false;
	}

	if (initialSize != entry->br.size) {
		newData = realloc(entry->br.data, entry->br.size);
		if (!newData) {
			free(entry->br.data);
			entry->br.data = NULL;
			entry->br.size = 0;
			fputs(ANSI_COLOR_RED"[Cache::compressBrotli] Reallocation error."
				  ANSI_COLOR_RESETLN, stderr);
			return false;
		}

		entry->br.data = newData;
	}

	if (!trySave(fileName, &entry->br, entry->br.encoding)) {
		fprintf(stderr, ANSI_COLOR_YELLOW"[Cache::compressBrotli] Failed to "
				"save a compressed file with uncompressed name: '%s'"
				ANSI_COLOR_RESETLN, fileName);
	}

	return true;
}

bool
trySave(const char *fileName, struct FCVersion *version, const char *ext) {
	char buf[1024];
	char *changeCharacter;
	size_t cacheLocationSize, extSize, fileNameSize;
	int fd;
	const char *writeBuf;
	size_t len;

	cacheLocationSize = strlen(OMCacheLocation);
	extSize = strlen(ext);
	fileNameSize = strlen(fileName);

	/* Create directory */
	memcpy(buf, OMCacheLocation, cacheLocationSize);
	buf[cacheLocationSize] = '/';
	cacheLocationSize += 1;
	memcpy(buf + cacheLocationSize, ext, extSize);
	memcpy(buf + cacheLocationSize + extSize, fileName, fileNameSize + 1);
	changeCharacter = strrchr(buf, '/');

	if (changeCharacter == NULL) {
		fputs("Error: Invalid OMCacheLocation.", stderr);
		return false;
	}
	changeCharacter[0] = '\0';

	/* Create directories */
	if (IOMkdirRecursive(buf) != 0) {
		fprintf(stderr, ANSI_COLOR_RED"[Cache::trySave] Failed to mkdir %s"
				ANSI_COLOR_RESETLN, buf);
		return false;
	}

	/* Construct the name */
	*changeCharacter = '/';

	/* Save the file */
	fd = open(buf, O_WRONLY | O_CREAT);
	if (fd == -1) {
		perror(ANSI_COLOR_RED"[Cache::trySave] Failed to create the file");
		fputs(ANSI_COLOR_RESET, stdout);
		return false;
	}

	writeBuf = version->data;
	len = version->size;
	do {
		ssize_t ret;

		ret = write(fd, writeBuf, len);
		if (ret == -1) {
			close(fd);
			fprintf(stderr, ANSI_COLOR_RED"[Cache::trySave] Failed to save %s"
					ANSI_COLOR_RESETLN, buf);
			return false;
		}

		len -= ret;
		writeBuf += ret;
	} while (len != 0);

	close(fd);
	return true;
}

/* Try load the compressed file from the filesystem cache. */
bool
tryLoad(const char *fileName, struct FCVersion *version, const char *ext,
		time_t modificationDate) {
	char buf[1024];
	size_t cacheLocationSize, extSize;
	int fd;
	char *readBuf;
	size_t len;
	struct stat status;

	cacheLocationSize = strlen(OMCacheLocation);
	extSize = strlen(ext);

	/* Create path */
	memcpy(buf, OMCacheLocation, cacheLocationSize);
	buf[cacheLocationSize] = '/';
	cacheLocationSize += 1;
	memcpy(buf + cacheLocationSize, ext, extSize);
	strcpy(buf + cacheLocationSize + extSize, fileName);

	fd = open(buf, O_RDONLY);
	if (fd == -1) {
		perror("Failed to open file");
		printf("FileName is '%s'\n", buf);
		return false;
	}

	if (fstat(fd, &status) == -1) {
		perror(ANSI_COLOR_RED"[Cache::Compression::tryLoad] fstat() failure");
		fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, buf);
		close(fd);
		return false;
	}

	if (status.st_mtime < modificationDate) {
		puts(ANSI_COLOR_RED"File was out of date:");
		fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, buf);
		close(fd);
		return false;
	}

	version->size = status.st_size;
	version->data = malloc(status.st_size);
	if (version->data == NULL) {
		fprintf(stderr, ANSI_COLOR_RED"[Cache::Compression::tryLoad] Failed to"
				" allocate %lli octets for file '%s' on extension '%s'"
				ANSI_COLOR_RESETLN, (long long int) status.st_size, buf, ext);
		close(fd);
		return false;
	}

	len = version->size;
	readBuf = version->data;
	do {
		ssize_t ret;

		ret = read(fd, readBuf, len);

		if (ret == 0)
			break;

		if (ret == -1) {
			free(version->data);
			version->data = NULL;
			perror(ANSI_COLOR_RED"[Cache::Compression::tryLoad] Failed read");
			fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, buf);
			close(fd);
			return false;
		}

		len -= ret;
		readBuf += ret;
	} while (len != 0);

	close(fd);
	return true;
}
