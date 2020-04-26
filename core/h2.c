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

#include "h2.h"

#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>

/* BSD originated header */
#include <endian.h>

#include "http2/frame.h"
#include "http2/settings.h"
#include "misc/default.h"

const char HTTP2Preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

int checkPreface(CSSClient);
int readFrame(CSSClient, struct H2Frame *);
int sendFrame(CSSClient, struct H2Frame *);
int sendSettings(CSSClient, struct H2Setting *, size_t);

void CSHandleHTTP2(CSSClient client) {
	struct H2Frame frame;

	puts("[CSHandleHTTP2] new client");

	if (!checkPreface(client)) {
		fputs(ANSI_COLOR_RED"[H2] Preface comparison failed"ANSI_COLOR_RESETLN,
			  stderr);
		return;
	}

	/* Server preface = (potentially empty) settings frame */
	sendSettings(client, NULL, 0);

	while (1) {
		if (!readFrame(client, &frame))
			break;
		free(frame.payload);
	}
}

/* If this functions returns 0, the contents of frame are undefined (can be any
 * value) */
int readFrame(CSSClient client, struct H2Frame *frame) {
	uint8_t buf[4];

	/* Length */
	if (!CSSReadClient(client, (char *)buf, 3))
		return 0;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	frame->length = (buf[0] << 16) + (buf[1] << 8) + buf[2];
#endif

	/* Type */
	if (!CSSReadClient(client, (char *) &frame->type, 1))
		return 0;

	/* Flags */
	if (!CSSReadClient(client, (char *) &frame->flags, 1))
		return 0;

	/* R + Stream Identifier */
	if (!CSSReadClient(client, (char *) buf, 4))
		return 0;
	frame->stream = ntohs((uint32_t) *buf);

	puts("Frame information");
	printf("\t length = %x\n", frame->length);
	printf("\t type = %x\n", frame->type);
	printf("\t flags = %x\n", frame->flags);
	printf("\t stream = %x\n", frame->stream);


	/* Payload */
	frame->payload = malloc(frame->length);
	if (frame->payload == NULL)
		return 0;

	if (!CSSReadClient(client, (char *)frame->payload, frame->length)) {
		free(frame->payload);
		return 0;
	}

	return 1;
}

int sendFrame(CSSClient client, struct H2Frame *frame) {
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

	if (!CSSWriteClient(client, buf, 9))
		return 0;

	if (frame->length != 0 &&
		frame->payload != NULL &&
		!CSSWriteClient(client, frame->payload, frame->length))
		return 0;

	return 1;
}

int checkPreface(CSSClient client) {
	char buf[24];

	if (!CSSReadClient(client, buf, 24))
		return 0;

	return memcmp(buf, HTTP2Preface, 24) == 0;	
}

int sendSettings(CSSClient client, struct H2Setting *settings, size_t count) {
	struct H2Frame frame = {
		count * 6,
		H2_FRAME_SETTINGS,
		0,
		0,
		settings
	};

	return sendFrame(client, &frame);
}
