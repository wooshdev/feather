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

#include "huffman.h"

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>

#include "http2/huffman/data.h"

/** Data Structures  **/
struct HuffmanNode {
	struct HuffmanNode *left;
	struct HuffmanNode *right;
	size_t value;
};

/** Function Prototypes **/
static void
destroyNode(struct HuffmanNode *);

/** Static Variables **/
static bool initialized = false;
static struct HuffmanNode rootNode;

void
HuffmanDestroy(void) {
	if (!initialized)
		return;
	destroyNode(&rootNode);
}

bool
HuffmanLookup(const uint8_t *source, size_t bitLength, uint8_t *output) {
	if (!initialized)
		return false;

	*output = 0xFAu;
	return true;
}

bool
HuffmanSetup(void) {
	struct HuffmanNode *currentNode;
	size_t i, j;
	uint32_t value;

	if (initialized)
		return true;

	for (i = 0; i < sizeof(huffmanSymbols) / sizeof(huffmanSymbols[0]); i++) {
		currentNode = &rootNode;
		value = huffmanSymbols[i].code;
		printf("\nValue: 0x%X (bits is %hhu): ", value, huffmanSymbols[i].bits);

		j = 0;
		while (j < huffmanSymbols[i].bits) {
			printf("%u", (uint32_t) (value & 0x10000000) > 8);

			j++;
			value = value << 1;
		}
	}

	puts("\nDone");

	initialized = true;
	return true;
}

static void
destroyNode(struct HuffmanNode *node) {
	if (node->left) {
		destroyNode(node->left);
		free(node->left);
	}

	if (node->right) {
		destroyNode(node->right);
		free(node->right);
	}
}
