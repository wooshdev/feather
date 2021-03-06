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

#ifndef HTTP2_FRAME_H
#define HTTP2_FRAME_H

#define H2_FRAME_DATA 0x0
#define H2_FRAME_HEADERS 0x1
#define H2_FRAME_PRIORITY 0x2
#define H2_FRAME_RST_STREAM 0x3
#define H2_FRAME_SETTINGS 0x4
#define H2_FRAME_PUSH_PROMISE 0x5
#define H2_FRAME_PING 0x6
#define H2_FRAME_GOAWAY 0x7
#define H2_FRAME_WINDOW_UPDATE 0x8
#define H2_FRAME_CONTINUATION 0x9
#define H2_FRAME_ALTSVC 0xA
#define H2_FRAME_ORIGIN 0xC

#include <stdbool.h>
#include <stdint.h>

/* RFC 7540 § 4.1 */
struct H2Frame {
	unsigned int  length : 24;
	uint8_t		  type;
	uint8_t		  flags;
	uint32_t	  stream;
	void		 *payload;
};

struct H2Session;

bool
H2ReadFrame(struct H2Session *, struct H2Frame *);

bool
H2SendFrame(struct H2Session *, struct H2Frame *);

#endif /* HTTP2_FRAME_H */
