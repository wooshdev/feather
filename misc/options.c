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

#include "options.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc/default.h"

/* Secure/recommended value by default: none */
enum OSILevel OMGSSystemInformationInServerHeader = OSIL_NONE;

const char	*OMSCertificateFile;
const char	*OMSCertificateChainFile;
const char	*OMSCertificatePrivateKeyFile;
const char	*OMSCipherList = "ECDHE-ECDSA-AES128-GCM-SHA256:"
	"ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:"
	"ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:"
	"ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:"
	"DHE-RSA-AES256-GCM-SHA384:"
	"TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384";
const char	*OMSCipherSuites = "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:"
"TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:TLS_AES_128_CCM_8_SHA256";

const char	*OMCacheLocation = "/var/www/cache";

char *internalCert;
char *internalChain;
char *internalPrivKey;

const char *internalPrefixPath = "/etc/letsencrypt/live/";
const char *internalSuffixCert = "/cert.pem";
const char *internalSuffixChain = "/chain.pem";
const char *internalSuffixPrivKey = "/privkey.pem";

int
internalSetupCertificatesLetsencrypt() {
	DIR *d;
	size_t genericSize, a, b;
	struct dirent *dir;

	d = opendir(internalPrefixPath);
	if (!d) {
		perror(ANSI_COLOR_RED"Failed to open Letsencrypt directory"
			   ANSI_COLOR_RESET);
		return 0;
	}

	while ((dir = readdir(d)) != NULL) {
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
			continue;

		a = strlen(internalPrefixPath);
		b = strlen(dir->d_name);
		genericSize = a + b + 1;
		internalCert = malloc(genericSize + strlen(internalSuffixCert));
		if (!internalCert)
			return -1;
		internalChain = malloc(genericSize + strlen(internalSuffixChain));
		if (!internalChain) {
			free(internalCert);
			return -2;
		}
		internalPrivKey = malloc(genericSize + strlen(internalSuffixPrivKey));
		if (!internalPrivKey) {
			free(internalChain);
			free(internalCert);
			return -2;
		}
		strcpy(internalCert, internalPrefixPath);
		strcpy(internalChain, internalPrefixPath);
		strcpy(internalPrivKey, internalPrefixPath);
		strcpy(internalCert + a, dir->d_name);
		strcpy(internalChain + a, dir->d_name);
		strcpy(internalPrivKey + a, dir->d_name);
		strcpy(internalCert + a + b, internalSuffixCert);
		strcpy(internalChain + a + b, internalSuffixChain);
		strcpy(internalPrivKey + a + b, internalSuffixPrivKey);

		OMSCertificateFile = internalCert;
		OMSCertificateChainFile = internalChain;
		OMSCertificatePrivateKeyFile = internalPrivKey;
		closedir(d);
		return 1;
	}

	closedir(d);
	return 0;
}

bool
OMSetup(void) {
	internalCert = NULL;
	internalChain = NULL;
	internalPrivKey = NULL;

	if (internalSetupCertificatesLetsencrypt() <= 0) {
		puts("Failed to setup certificate files using Letsencrypt.");
		return false;
	}

	return true;
}

void
OMDestroy(void) {
	free(internalCert);
	free(internalChain);
	free(internalPrivKey);
}
