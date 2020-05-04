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

#include "cache.h"

#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cache/compression.h"
#include "http/response_headers.h"
#include "http/strings.h"
#include "misc/default.h"
#include "misc/io.h"
#include "misc/options.h"

#define FCSTEP 8
#define FC_CALCULATE_USAGE

/* Path */
const char path[] = "/var/www/html";
const size_t pathLength = sizeof(path) / sizeof(path[0]) - 1;

/* State */
size_t fcCount = 0;
struct FCEntry **fcEntries = NULL;
char **fcNames = NULL;
size_t fcSize = 0;

/* Subroutines */
void
calculateUsage(void);

int
setFileContents(const char *, const char *, struct FCEntry *);


int
addFile(const char *directory, const char *fileName) {
	struct FCEntry **newEntries;
	char **newNames;
	char *name;
	struct FCEntry *entry;
	size_t lenDir;
	size_t lenFile;
	size_t lenPath;

	/* Allocate entry */
	lenDir = strlen(directory);
	lenFile = strlen(fileName);

	name = malloc(lenDir + 1 + lenFile + 1);
	if (!name) {
		fputs(ANSI_COLOR_RED"[Cache::addFile] Allocation failure on 'name'."
			  ANSI_COLOR_RESETLN, stderr);
		return 0;
	}

	entry = malloc(sizeof(struct FCEntry));
	if (!entry) {
		fputs(ANSI_COLOR_RED"[Cache::addFile] Allocation failure on 'entry'."
			  ANSI_COLOR_RESETLN, stderr);
		free(name);
		return 0;
	}

	/* Create name */
	memcpy(name, directory, lenDir);
	name[lenDir] = '/';
	memcpy(name + lenDir + 1, fileName, lenFile);
	name[lenDir + 1 + lenFile] = '\0';

	/* Create entry */
	memset(entry, 0, sizeof(struct FCEntry));
	if (!setFileContents(name, name + pathLength, entry)) {
		fputs(ANSI_COLOR_RED"[Cache::addFile] setFileContents() failed."
			  ANSI_COLOR_RESETLN, stderr);
		free(entry);
		free(name);
		return 0;
	}

	/* Reallocate if full */
	if (fcCount == fcSize) {
		fcSize += FCSTEP;

		newEntries = realloc(fcEntries, sizeof(struct FCEntry *) * fcSize);
		if (!newEntries) {
			fputs(ANSI_COLOR_RED
				  "[Cache::addFile::newEntriesRealloc] Allocation failure."
				  ANSI_COLOR_RESETLN, stderr);
			free(fcEntries);
			free(fcNames);
			free(entry);
			free(name);
			fcEntries = NULL;
			fcNames = NULL;
			return 0;
		}
		fcEntries = newEntries;

		newNames = realloc(fcNames, sizeof(char *) * fcSize);
		if (!newNames) {
			fputs(ANSI_COLOR_RED
				  "[Cache::addFile::newNamesRealloc] Allocation failure."
				  ANSI_COLOR_RESETLN, stderr);
			free(fcNames);
			free(fcEntries);
			free(entry);
			free(name);
			fcEntries = NULL;
			fcNames = NULL;
			return 0;
		}
		fcNames = newNames;
	}

	lenPath = strlen(path);
	memcpy(name, name + lenPath, strlen(name) - lenPath + 1);

	printf("Cache (name='%s')\n", name);
	/* Put entries in cache */
	fcNames[fcCount] = name;
	fcEntries[fcCount] = entry;

	fcCount += 1;
	return 1;
}

int
followDirectory(const char *name, int indent) {
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name))) {
		perror("Cache::followDirectory opendir() failure");
		printf("[Cache::followDirectory] Directory: '%s'\n", name);
		return 0;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_DIR) {
			char pathBuffer[1024];
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s", name, entry->d_name);

			if (!followDirectory(pathBuffer, indent + 2)) {
				closedir(dir);
				return 0;
			}
		} else if (!addFile(name, entry->d_name)) {
			fputs(ANSI_COLOR_RED
					"[Cache::followDirectory] addFile() failed."
					ANSI_COLOR_RESETLN, stderr);
			closedir(dir);
			return 0;
		}
	}

	closedir(dir);
	return 1;
}

int
FCSetup(void) {
	if (IOMkdirRecursive(OMCacheLocation) != 0) {
		fputs(ANSI_COLOR_RED"[Cache::FCSetup] Failed to create cache folder."
			  ANSI_COLOR_RESETLN, stderr);
		return 0;
	}

	if (!FCCompressionSetup()) {
		fputs(ANSI_COLOR_RED"[Cache::FCSetup] FCCompressionSetup() failed."
			  ANSI_COLOR_RESETLN, stderr);
		return 0;
	}

	if (!followDirectory("/var/www/html", 0)) {
		fputs(ANSI_COLOR_RED"[Cache::FCSetup] I/O error occurred."
			  ANSI_COLOR_RESETLN, stderr);
		FCCompressionDestroy();
		FCDestroy();
		return 0;
	}

#ifdef FC_CALCULATE_USAGE
	calculateUsage();
#endif /* FC_CALCULATE_USAGE */

	return 1;
}

int
FCLookup(const char *path, struct FCResult *result, enum FCFlags flags) {
	UNUSED(path);
	UNUSED(result);

	struct FCEntry		*entry;
	size_t i;
	struct FCVersion	*version;

	/* FIXME */
	if (strcmp(path, "/") == 0)
		path = "/index.html";

	for (i = 0; i < fcCount; i++) {
		if (strcasecmp(path, fcNames[i]) == 0) {
			entry = fcEntries[i];

			result->mediaCharset = entry->mediaCharset;
			result->mediaType = entry->mediaType;
			result->modificationDate = entry->modificationDate;

			if (flags & FCF_BROTLI && entry->br.data)
				version = &entry->br;
			else if (flags & FCF_GZIP && entry->gzip.data)
				version = &entry->gzip;
			else
				version = &entry->uncompressed;

			result->data = version->data;
			result->encoding = version->encoding;
			result->size = version->size;

			return 1;
		}
	}

	return 0;
}

void
FCDestroy(void) {

	if (fcCount != 0) {
		size_t i;

		for (i = 0; i < fcCount; i++) {
			free(fcEntries[i]->uncompressed.data);
			free(fcEntries[i]->br.data);
			free(fcEntries[i]->gzip.data);
			free(fcEntries[i]);
			free(fcNames[i]);
		}
	}

	free(fcEntries);
	free(fcNames);

	FCCompressionDestroy();
}

int
setFileContents(const char *file, const char *fileNameAbsolute,
					struct FCEntry *entry) {
	char *buf;
	int fd;
	off_t len;
	struct stat status;

	fd = open(file, O_RDONLY);
	if (fd == -1) {
		perror(ANSI_COLOR_RED"[Cache::setFileContents] read() failure");
		fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, file);
		return 0;
	}

	if (fstat(fd, &status) == -1) {
		perror(ANSI_COLOR_RED"[Cache::setFileContents] fstat() failure");
		fprintf(stderr, "\tFile='%s'"ANSI_COLOR_RESETLN, file);
		close(fd);
		return 0;
	}

	entry->modificationDate = status.st_mtime;
	entry->uncompressed.encoding = MTE_none;
	entry->uncompressed.size = status.st_size;
	entry->uncompressed.data = malloc(status.st_size);
	if (entry->uncompressed.data == NULL) {
		fputs(ANSI_COLOR_RED"WARNING: setFileContents() malloc failed."
			  ANSI_COLOR_RESETLN, stderr);
		close(fd);
		return 0;
	}

	len = status.st_size;
	buf = entry->uncompressed.data;
	do {
		ssize_t ret;

		ret = read(fd, buf, len);
		if (ret == 0)
			break;

		if (ret == -1) {
			fputs(ANSI_COLOR_RED"[Cache::setFileContents] Failed to read()"
				  ANSI_COLOR_RESETLN, stderr);
			free(entry->uncompressed.data);
			entry->uncompressed.data = NULL;
			close(fd);
			return 0;
		}

		buf += ret;
		len -= ret;
	} while (len != 0);

	HTTPGetMediaTypeProperties(fileNameAbsolute, entry);

	if (!FCCompressFile(fileNameAbsolute, entry)) {
		fputs(ANSI_COLOR_RED
			  "[Cache::setFileContents] Failed to FCCompressFile()"
			  ANSI_COLOR_RESETLN, stderr);
		free(entry->uncompressed.data);
		entry->uncompressed.data = NULL;
		close(fd);
		return 0;
	}

	close(fd);
	return 1;
}

void
calculateUsage(void) {
	size_t i;
	size_t totalOctets;
	size_t totalObjects;
	size_t logarithmic;

	totalObjects = fcCount;
	totalOctets = 0;

	for (i = 0; i < fcCount; i++) {
		struct FCEntry *entry;

		entry = fcEntries[i];
		if (entry->br.size > 0)
			totalObjects += 1;
		if (entry->gzip.size > 0)
			totalObjects += 1;
		totalOctets += entry->uncompressed.size +
					   entry->br.size +
					   entry->gzip.size;
	}

	puts(ANSI_COLOR_BLUE" ======= "
		 ANSI_COLOR_MAGENTA"FileCache Usage"
		 ANSI_COLOR_BLUE" =======");
	printf(ANSI_COLOR_BLUE"Total Objects:\t"
		   ANSI_COLOR_GREY"%zu\n",
		   totalObjects);
	fputs(ANSI_COLOR_BLUE"Total Memory:\t"
		  ANSI_COLOR_GREY, stdout);

	/* IMO, the following is ugly code, but it is what it is. */
	if (totalOctets == 0)
		fputs("no memory used."ANSI_COLOR_RESETLN, stdout);
	else if (totalOctets < 1000)
		printf("%zu bytes."ANSI_COLOR_RESETLN, totalOctets);
	else {
		logarithmic = (size_t)(log(totalOctets) / log(1000));
		const char names[] = { 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
		printf("%.1f %cB"ANSI_COLOR_RESETLN,
			   totalOctets / pow(1000, logarithmic),
			   names[logarithmic - 1]);
	}
}
