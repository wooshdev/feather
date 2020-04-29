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
 *
 * CS(S) is an abbreviation for Core Server/Service (Security)
 */

#ifndef CORE_SECURITY_H
#define CORE_SECURITY_H

#include <stddef.h>

#include <openssl/ossl_typ.h>

typedef SSL *CSSClient;

enum CSProtocol {
	CSPROT_ERROR,
	CSPROT_HTTP1,
	CSPROT_HTTP2,
	CSPROT_NONE,
};

void
CSDestroySecurityManager(void);

int
CSSetupSecurityManager(void);

void
CSSDestroyClient(CSSClient);

enum CSProtocol
CSSGetProtocol(CSSClient);

int
CSSReadClient(CSSClient, char *, size_t); 

int
CSSSetupClient(int, CSSClient *);

int
CSSWriteClient(CSSClient, const char *, size_t);


#endif /* CORE_SECURITY_H */
