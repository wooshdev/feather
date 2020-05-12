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

#include "frame.h"

#ifdef __FreeBSD__
	#include <sys/endian.h>
#else
	#include <endian.h>
#endif

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>

#include "http2/debugging.h"
#include "http2/session.h"

/* If this functions returns 0, the contents of frame are undefined (can be any
 * value) */
bool
H2ReadFrame(struct H2Session *session, struct H2Frame *frame) {
	uint8_t buf[4];

	/* Length */
	if (!CSSReadClient(session->client, (char *)buf, 3))
		return FALSE;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	frame->length = (buf[0] << 16) + (buf[1] << 8) + buf[2];
#endif

	/* Type */
	if (!CSSReadClient(session->client, (char *) &frame->type, 1))
		return FALSE;

	/* Flags */
	if (!CSSReadClient(session->client, (char *) &frame->flags, 1))
		return FALSE;

	/* R + Stream Identifier */
	if (!CSSReadClient(session->client, (char *) buf, 4))
		return FALSE;
	frame->stream = ntohs((uint32_t) *buf);

// 	puts("Frame information");
// 	printf("\t length = %x\n", frame->length);
// 	printf("\t type = %s\n", H2DFrameTypeNames[frame->type]);
// 	printf("\t flags = %x\n", frame->flags);
// 	printf("\t stream = %x\n", frame->stream);

	/* Payload */
	frame->payload = malloc(frame->length);
	if (frame->payload == NULL)
		return FALSE;

	if (!CSSReadClient(session->client, (char *)frame->payload,
			frame->length)) {
		free(frame->payload);
		return FALSE;
	}

	return TRUE;
}

bool
H2SendFrame(struct H2Session *session, struct H2Frame *frame) {
	char buf[9] = {
		frame->length >> 16,
		frame->length >> 8,
		frame->length & 0x0000FF,
		frame->type,
		frame->flags,
		frame->stream >> 24,
		frame->stream >> 16,
		frame->stream >> 8,
		frame->stream & 0x000000FF
	};

	if (!CSSWriteClient(session->client, buf, sizeof(buf)))
		return FALSE;

	if (frame->length != 0 &&
		frame->payload != NULL &&
		!CSSWriteClient(session->client, frame->payload, frame->length))
		return FALSE;

	return TRUE;
} 
