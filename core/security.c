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

#include "security.h"

#include <sys/types.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>


#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <openssl/ssl3.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>

#include "misc/default.h"
#include "misc/io.h"
#include "misc/options.h"

/* 300 ms = 300000 μs */
#define CSS_POLL_TIMEOUT 300000

const SSL_METHOD *SSLMethod;
SSL_CTX *SSLContext;

static const unsigned char ALPN_HTTP1[] = {
	/* "http/1.1" */
	0x68, 0x74, 0x74, 0x70, 0x2f, 0x31, 0x2e, 0x31
};
static const unsigned char ALPN_HTTP2[] = {
	/* "http/2" */
	0x68, 0x32
};

#define ALPN_HTTP1_LEN 8
#define ALPN_HTTP2_LEN 2

static int 
alpnHandler(SSL *ssl, 
			const unsigned char **out, 
			unsigned char *outlen,
			const unsigned char *in,
			unsigned int inlen,
			void *arg);

int
CSSetupSecurityManager(void) {
	X509 *cert;

	SSLMethod = TLS_server_method();
	if (!SSLMethod)
		return 0;

	SSLContext = SSL_CTX_new(SSLMethod);
	if (!SSLContext) {
		return -1;
	}

	SSL_CTX_set_ecdh_auto(SSLContext, 1);
	SSL_CTX_set_min_proto_version(SSLContext, TLS1_2_VERSION);

	if (SSL_CTX_set_cipher_list(SSLContext, OMSCipherList) == 0) {
		puts(ANSI_COLOR_RED"E: Failed to set cipher list."ANSI_COLOR_RESETLN);
		ERR_print_errors_fp(stderr);
		CSDestroySecurityManager();
		return -2;
	}

/* LibreSSL doesn't have the 'SSL_CTX_set_ciphersuites' function */
#if defined(TLS_MAX_VERSION) && TLS_MAX_VERSION == TLS1_3_VERSION
	if (SSL_CTX_set_ciphersuites(SSLContext, OMSCipherSuites) == 0) {
		puts(ANSI_COLOR_RED"E: Failed to set ciphersuites."ANSI_COLOR_RESETLN);
		ERR_print_errors_fp(stderr);
		CSDestroySecurityManager();
		return -2;
	}
#endif

	/* Set certificate file. */
	if (SSL_CTX_use_certificate_file(SSLContext, OMSCertificateFile,
		SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		CSDestroySecurityManager();
		return -2;
	}

	/* Set chain file. */
	FILE *file = fopen(OMSCertificateChainFile, "r");
	cert = PEM_read_X509(file, NULL, 0, NULL);
	fclose(file);
	if (!SSL_CTX_add_extra_chain_cert(SSLContext, cert)) {
		ERR_print_errors_fp(stderr);
		CSDestroySecurityManager();
		return -3;
	}

	if (SSL_CTX_use_PrivateKey_file(SSLContext, OMSCertificatePrivateKeyFile,
		SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		CSDestroySecurityManager();
		return -4;
	}

	BIO *bio = BIO_new(BIO_s_file());
	if (!bio) {
		CSDestroySecurityManager();
		return -5;
	}
	if (!BIO_read_filename(bio, OMSCertificateChainFile)) {
		BIO_free(bio);
		CSDestroySecurityManager();
		return -6;
	}

	cert = X509_new();
	if (!cert) {
		BIO_free(bio);
		CSDestroySecurityManager();
		return -7;
	}

	/* TODO Check this return value ! */
	PEM_read_bio_X509(bio, &cert, 0, NULL);

	if (!SSL_CTX_add1_chain_cert(SSLContext, cert)) {
		BIO_free(bio);
		X509_free(cert);
		CSDestroySecurityManager();
		return -8;
	}
	BIO_free(bio);
	X509_free(cert);

	SSL_CTX_set_alpn_select_cb(SSLContext, alpnHandler, NULL);
	
	return 1;
}

void
CSDestroySecurityManager(void) {
	/* Clean our objects */
	SSL_CTX_free(SSLContext);

	/* Clean internal state */
	/*FIPS_mode_set(0); */
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_id_callback(NULL);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
}

void
CSSDestroyClient(CSSClient client) {
	int state;
	char unused[1];

	/* Check if we can still read, so we can perform a proper shutdown. */
	state = SSL_read(client, unused, 1);
	if (SSL_get_error(client, state) == SSL_ERROR_NONE)
		SSL_shutdown(client);

	SSL_free(client);
}

int
CSSSetupClient(int sockfd, CSSClient *client) {
	int ret;
	SSL *ssl;

	if (IOTimeoutAvailableData(sockfd, CSS_POLL_TIMEOUT) <= 0)
		return 0;

	ssl = SSL_new(SSLContext);
	if (!ssl)
		return -1;

	/* attach socket */
	if (!SSL_set_fd(ssl, sockfd)) {
		/* ERR_print_errors_fp(stderr); */
		CSSDestroyClient(ssl);
		return -2;
	}

	ret = SSL_accept(ssl);
	if (ret <= 0) {
		/*
		printf("SSL_get_error from SSL_accept is %i\n", 
			   SSL_get_error(ssl, ret));
		ERR_print_errors_fp(stderr);
		*/
		CSSDestroyClient(ssl);
		return -3;
	}

	*client = ssl;
	return 1;
}

/* TODO this implementation is blocking */
bool
CSSWriteClient(CSSClient client, const char *buf, size_t len) {
	do {
		ssize_t ret;

		ret = SSL_write(client, buf, len);

		if (ret <= 0) {
#ifdef CORE_SECURITY_FLAG_FIX_WRITE_ERRORS
			printf("errno is %i\n", errno);
			perror("SSL_write error");
			printf("Write error: %i\n", SSL_get_error(client, ret));
			ERR_print_errors_fp(stderr);
			long error;
			while ((error = ERR_get_error()) != 0) {
				printf("error: %s %s\n",
					   ERR_lib_error_string(error),
					   ERR_func_error_string(error));
			}
#endif /* CORE_SECURITY_FLAG_FIX_WRITE_ERRORS */
			return FALSE;
		}

		buf += ret;
		len -= ret;
	} while (len > 0);

	return TRUE;
}

bool
CSSReadClient(CSSClient client, char *buf, size_t len) {
	do {
		ssize_t ret;

		ret = SSL_read(client, buf, len);

		if (ret <= 0)
			return FALSE;

		buf += ret;
		len -= ret;
	} while (len > 0);

	return TRUE;
}

static int
compareALPN(const unsigned char *a,
			const unsigned char *b,
			size_t size) {
	size_t i;
	
	for (i = 0; i < size; i++)
		if (((unsigned char) a[i]) != b[i])
			return 0;
	return 1;
}

static int
alpnHandler(SSL *ssl, const unsigned char **out, unsigned char *outlen,
			const unsigned char *in, unsigned int inlen, void *arg) {
	UNUSED(ssl);
	UNUSED(arg);

	size_t pos = 0;
	unsigned char size;
	int wasHTTP1Found = 0;

	while (pos < inlen) {
		size = in[pos++];
		
		/* Check malformed data */
		if (size == 0 || size + pos > inlen || size + pos > 254)
			return SSL_TLSEXT_ERR_ALERT_FATAL;

		if (size == ALPN_HTTP1_LEN && 
			compareALPN(ALPN_HTTP1, in + pos, size)) {
			wasHTTP1Found = 1;
		}
#ifdef OPTIONS_ENABLE_HTTP2
		else if (size == ALPN_HTTP2_LEN && 
			compareALPN(ALPN_HTTP2, in + pos, size)) {
			*out = ALPN_HTTP2;
			*outlen = size;
			return SSL_TLSEXT_ERR_OK;
		}
#endif
		pos += size;
	}

	if (wasHTTP1Found) {
		*out = ALPN_HTTP1;
		*outlen = size;
		return SSL_TLSEXT_ERR_OK;
	}
	
	return SSL_TLSEXT_ERR_ALERT_FATAL;
}

enum CSProtocol
CSSGetProtocol(CSSClient client) {
	const unsigned char *data;
	unsigned int len;

	SSL_get0_alpn_selected(client, &data, &len);
	if (len == 0 || !data)
		return CSPROT_NONE;

	if (len == ALPN_HTTP1_LEN && compareALPN(data, ALPN_HTTP1, ALPN_HTTP1_LEN))
		return CSPROT_HTTP1;
	
	if (len == ALPN_HTTP2_LEN && compareALPN(data, ALPN_HTTP2, ALPN_HTTP2_LEN))
		return CSPROT_HTTP2;

	size_t i;
	for (i = 0; i < len; i++)
		putc(data[i], stdout);
	putc('\n', stdout);

	return CSPROT_ERROR;
}
