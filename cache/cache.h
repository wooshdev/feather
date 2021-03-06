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
 *
 * FC is an abbreviation for FileCache.
 */

#ifndef CACHE_CACHE_H
#define CACHE_CACHE_H

#include <stdbool.h>
#include <time.h>

struct FCVersion {
	char		*data;
	const char	*encoding;
	size_t		 size;
};

struct FCEntry {
	const char		*mediaCharset;
	const char		*mediaType;
	time_t			 modificationDate;
	struct FCVersion br;
	struct FCVersion gzip;
	struct FCVersion uncompressed;
};

enum FCFlags {
	FCF_BROTLI = 1,
	FCF_GZIP = 2
};

struct FCResult {
	const char	*data;
	const char	*encoding;
	const char	*mediaCharset;
	const char	*mediaType;
	time_t		 modificationDate;
	size_t		 size;
};

bool
FCSetup(void);

bool
FCLookup(const char *, struct FCResult *, enum FCFlags);

void
FCDestroy(void);

#endif /* CACHE_CACHE_H */
