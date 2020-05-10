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
 * MT is an abbreviation for Media Type
 * MTC is an abbreviation for Media Type Charset
 * MTE is an abbreviation for Media Type Encoding
 */

#ifndef HTTP_STRINGS_H
#define HTTP_STRINGS_H

/* Charsets */
extern const char *MTC_utf8;

/* Encodings */
extern const char *MTE_brotli;
extern const char *MTE_gzip;
extern const char *MTE_none;

/* Media Types */
extern const char *MT_css;
extern const char *MT_html;
extern const char *MT_octetstream;

/* Statuses */
extern const char *HTTPStatus200OK;
extern const char *HTTPStatus304NotModified;
extern const char *HTTPStatus400BadRequest;
extern const char *HTTPStatus404NotFound;
extern const char *HTTPStatus500NotImplemented;
extern const char *HTTPStatus505HTTPVersionNotSupported;


#endif /* HTTP_STRINGS_H */
