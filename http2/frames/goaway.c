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

#include "goaway.h"

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http2/session.h"
#include "http2/frame.h"

bool
H2HandleGoaway(struct H2Session *session, struct H2Frame *frame) {
	(void) session;
	(void) frame;
	return false;
}

bool
H2SendGoaway(struct H2Session *session,
			 uint32_t lastStreamID, uint32_t errorCode,
			 size_t additionalDebugDataSize, const void *additionalDebugData) {
	bool ret;
	uint8_t *buf;
	struct H2Frame frame;

	buf = malloc(additionalDebugDataSize + 8);

	if (!buf)
		return false;

	frame.length = additionalDebugDataSize + 8;
	frame.type = H2_FRAME_GOAWAY;
	frame.flags = 0;
	frame.stream = 0;
	frame.payload = buf;

	*((uint32_t *) buf) = ntohl(lastStreamID);
	*((uint32_t *) &buf[4]) = ntohl(errorCode);
	if (additionalDebugData && additionalDebugDataSize > 0)
		memcpy(buf + 8, additionalDebugData, additionalDebugDataSize);

	ret = H2SendFrame(session, &frame);
	free(buf);

	return ret;
}
