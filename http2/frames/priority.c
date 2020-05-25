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

#include "priority.h"

#include <arpa/inet.h>

#include "http2/frame.h"
#include "http2/session.h"

#include "http2/frames/goaway.h"
#include "http2/frames/rst_stream.h"
#include "http2/error.h"

static const char errorInfoStream0[] = "Priority on stream 0 is invalid";
static const char errorInfoLength[] = "Priority frames are 5 octets of length";

bool
H2HandlePriority(struct H2Session *session, struct H2Frame *frame) {
	if (frame->stream == 0) {
		H2SendGoaway(session, 0, H2E_PROTOCOL_ERROR,
					 sizeof(errorInfoStream0), errorInfoStream0);
		return false;
	}

	if (frame->length != 5) {
		H2SendGoaway(session, 0, H2E_FRAME_SIZE_ERROR,
					 sizeof(errorInfoLength), errorInfoLength);
		return false;
	}

	/* This information is not used at the moment. */

	return true;
}
