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

#include <cstdio>
#include <cstdlib>
#include <cstddef>

#include <vector>

extern "C" {
#include "http2/huffman/huffman.h"
}

struct InputData {
	std::vector<uint8_t> data;
	uint8_t bits;
	uint8_t expected;
};

std::vector<InputData> inputs = {
	{ { 0x70 }, 7, 'T' },
	{ { 0x3f, 0x8 }, 10, '!' },
};

int
main(void) {
	size_t i;
	uint8_t output;

	if (!HuffmanSetup()) {
		fputs("Failed to setup huffman!\n", stderr);
		return EXIT_FAILURE;
	}

	for (i = 0; i < inputs.size(); i++) {
		if (!HuffmanLookup(inputs[i].data.data(), inputs[i].bits, &output)) {
			fprintf(stderr, "[Test #%zu] Huffman lookup failed!\n", i);
			HuffmanDestroy();
			return EXIT_FAILURE;
		}

		if (output != inputs[i].expected) {
			fprintf(stderr, "[Test #%zu] Invalid value: output=0x%hhX expected"
							"=0x%hhX\n", i, output, inputs[i].expected);
			HuffmanDestroy();
			return EXIT_FAILURE;
		}

		printf("[Test #%zu] Passed\n", i);
	}

	HuffmanDestroy();
	return EXIT_SUCCESS;
}
