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
 *	list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *	this list of conditions and the following disclaimer in the documentation
 *	and/or other materials provided with the distribution.
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
#include "compression.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <brotli/encode.h>

#include "http/strings.h"
#include "misc/default.h"
#include "misc/io.h"
#include "misc/options.h"

/* Brotli Parameters */
int					BrotliQuality =	BROTLI_DEFAULT_QUALITY;
int					BrotliWindow =	BROTLI_DEFAULT_WINDOW;
BrotliEncoderMode	BrotliMode =	BROTLI_DEFAULT_MODE;

/* Subroutines */
int		tryLoad(const char *, struct FCVersion *, const char *, time_t);
int		trySave(const char *, struct FCVersion *, const char *);
int		compressBrotli(const char *, struct FCEntry *);

int FCCompressionSetup(void) {
	return 1;
}

void FCCompressionDestroy(void) {
}

int FCCompressFile(const char *fileName, struct FCEntry *entry) {
	if (!compressBrotli(fileName, entry))
		return 0;

	return 1;
}

int compressBrotli(const char *fileName, struct FCEntry *entry) {
	size_t initialSize;
	char *newData;
	BROTLI_BOOL ret;

	entry->br.encoding = MTE_brotli;

	if (entry->uncompressed.size == 0)
		return 1;

	if (tryLoad(fileName, &entry->br, entry->br.encoding,
		entry->modificationDate)) {
		return 1;
	}

	initialSize = BrotliEncoderMaxCompressedSize(entry->uncompressed.size);
	entry->br.size = initialSize;
	if (initialSize == 0) {
		fprintf(stderr, ANSI_COLOR_RED"[Cache::compressBrotli] Size "
				"doesn't fit size_t! Input size is %zu"ANSI_COLOR_RESETLN,
				entry->uncompressed.size);
		return 0;
	}

	entry->br.data = malloc(entry->br.size);
	if (!entry->br.data) {
		entry->br.size = 0;
		fputs(ANSI_COLOR_RED"[Cache::compressBrotli] Allocation error."
				ANSI_COLOR_RESETLN, stderr);
		return 0;
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
		return 0;
	}

	if (initialSize != entry->br.size) {
		newData = realloc(entry->br.data, entry->br.size);
		if (!newData) {
			free(entry->br.data);
			entry->br.data = NULL;
			entry->br.size = 0;
			fputs(ANSI_COLOR_RED"[Cache::compressBrotli] Reallocation error."
				  ANSI_COLOR_RESETLN, stderr);
			return 0;
		}

		entry->br.data = newData;
	}

	if (!trySave(fileName, &entry->br, entry->br.encoding)) {
		fputs(ANSI_COLOR_RED"[Cache::compressBrotli] Failed to save compressed file."
			  ANSI_COLOR_RESETLN, stderr);
	}

	return 1;
}

int trySave(const char *fileName, struct FCVersion *version, const char *ext) {
	char buf[1024];
	char *changeCharacter;
	size_t cacheLocationSize, extSize, fileNameSize;
	int fd;
	const char *writeBuf;
	size_t len;
	ssize_t ret;

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
	changeCharacter[0] = '\0';

	/* Create directories */
	if (IOMkdirRecursive(buf) != 0) {
		fprintf(stderr, ANSI_COLOR_RED"[Cache::trySave] Failed to mkdir %s"
				ANSI_COLOR_RESETLN, buf);
		return 0;
	}

	/* Construct the name */
	*changeCharacter = '/';

	/* Save the file */
	fd = open(buf, O_WRONLY | O_CREAT);
	if (fd == -1) {
		perror(ANSI_COLOR_RED"[Cache::trySave] Failed to create the file");
		fputs(ANSI_COLOR_RESET, stdout);
		return 0;
	}

	writeBuf = version->data;
	len = version->size;
	do {
		ret = write(fd, writeBuf, len);
		if (ret == -1) {
			close(fd);
			fprintf(stderr, ANSI_COLOR_RED"[Cache::trySave] Failed to save %s"
					ANSI_COLOR_RESETLN, buf);
			return 0;
		}

		len -= ret;
		writeBuf += ret;
	} while (len != 0);

	close(fd);
	return 1;
}

/* Try load the compressed file from the filesystem cache. */
int tryLoad(const char *fileName, struct FCVersion *version, const char *ext,
			time_t modificationDate) {
	char buf[1024];
	size_t cacheLocationSize, extSize;
	int fd;
	char *readBuf;
	size_t len;
	ssize_t ret;
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
		return 0;
	}

	if (fstat(fd, &status) == -1) {
		perror(ANSI_COLOR_RED"[Cache::Compression::tryLoad] fstat() failure");
		fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, buf);
		close(fd);
		return 0;
	}

	if (status.st_mtime < modificationDate) {
		puts(ANSI_COLOR_RED"File was out of date:");
		fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, buf);
		close(fd);
		return 0;
	}

	version->size = status.st_size;
	version->data = malloc(status.st_size);
	if (version->data == NULL) {
		fprintf(stderr, ANSI_COLOR_RED"[Cache::Compression::tryLoad] Failed to"
				" allocate %zu octets for file '%s' on extension '%s'"
				ANSI_COLOR_RESETLN, status.st_size, buf, ext);
		close(fd);
		return 0;
	}

	len = version->size;
	readBuf = version->data;
	do {
		ret = read(fd, readBuf, len);

		if (ret == 0)
			break;

		if (ret == -1) {
			free(version->data);
			version->data = NULL;
			perror(ANSI_COLOR_RED"[Cache::Compression::tryLoad] Failed read");
			fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, buf);
			close(fd);
			return 0;
		}

		len -= ret;
		readBuf += ret;
	} while (len != 0);

	close(fd);
	return 1;
}
