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

#ifndef HTTP2_ERROR_H
#define HTTP2_ERROR_H

/**
 * RFC 7540 § 11.4 states that these errors can be found at IANA's website:
 * https://www.iana.org/assignments/http2-parameters/http2-parameters.xhtml
 */
#define H2E_NO_ERROR 0x0
#define H2E_PROTOCOL_ERROR 0x1
#define H2E_INTERNAL_ERROR 0x2
#define H2E_FLOW_CONTROL_ERROR 0x3
#define H2E_SETTINGS_TIMEOUT 0x4
#define H2E_STREAM_CLOSED 0x5
#define H2E_FRAME_SIZE_ERRO 0x6
#define H2E_REFUSED_STREAM 0x7
#define H2E_CANCEL 0x8
#define H2E_COMPRESSION_ERROR 0x9
#define H2E_CONNECT_ERROR 0xA
#define H2E_ENHANCE_YOUR_CALM 0xB
#define H2E_INADEQUATE_SECURITY 0xC
#define H2E_HTTP_1_1_REQUIRED 0xD

#endif /* HTTP2_ERROR_H */
