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

#include "statistics.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "misc/default.h"

static pthread_mutex_t MSTrafficMutex = PTHREAD_MUTEX_INITIALIZER;
static size_t MSTrafficCount = 0;
static clock_t MSBeginTime = -1;

size_t
SMGetPageTraffic(void) {
	size_t result;

	pthread_mutex_lock(&MSTrafficMutex);
	{
		result = MSTrafficCount;
	}
	pthread_mutex_unlock(&MSTrafficMutex);

	return result;
}

void
SMNotifyRequest(void) {
	pthread_mutex_lock(&MSTrafficMutex);
	{
		MSTrafficCount += 1;
	}
	pthread_mutex_unlock(&MSTrafficMutex);
}

void
SMBegin(void) {
	MSBeginTime = clock();
}

void
SMEnd(void) {
	clock_t endTime;
	int hasPrinted; /* boolean */

	endTime = clock();
	hasPrinted = 0;

	printf(ANSI_COLOR_CYAN"Traffic> "ANSI_COLOR_GREY"Got %zu requests.\n"
		   ANSI_COLOR_MAGENTA"Uptime> "ANSI_COLOR_GREY, SMGetPageTraffic());

	if (MSBeginTime == -1)
		puts("error (begin time was 0)"ANSI_COLOR_RESET);
	else {
		size_t amount, timeBetween;

		/* XSI requires that CLOCKS_PER_SEC is equal to 1e6, which pratically
		 * makes clock() return microseconds */
		timeBetween = (endTime - MSBeginTime) / 1000; /* is now milliseconds */

		fputs("Ran for ", stdout);

		if (timeBetween > 6048e5) {
			amount = timeBetween / 6048e5;
			timeBetween -= amount * 6048e5;
			printf("%s%zu week%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
			hasPrinted = 1;
		}

		if (timeBetween > 864e5) {
			amount = timeBetween / 864e5;
			timeBetween -= amount * 864e5;
			printf("%s%zu day%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
			hasPrinted = 1;
		}

		if (timeBetween > 36e5) {
			amount = timeBetween / 36e5;
			timeBetween -= amount * 36e5;
			printf("%s%zu hour%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
			hasPrinted = 1;
		}

		if (timeBetween > 6e4) {
			amount = timeBetween / 6e4;
			timeBetween -= amount * 6e4;
			printf("%s%zu minute%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
			hasPrinted = 1;
		}

		if (timeBetween > 1e3) {
			amount = timeBetween / 1e3;
			timeBetween -= amount * 1e3;
			printf("%s%zu second%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
			hasPrinted = 1;
		}

		if (timeBetween > 0) {
			amount = timeBetween;
			printf("%s%zu millisecond%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
			hasPrinted = 1;
		}

		timeBetween = endTime - MSBeginTime;
		timeBetween %= 1000;
		if (timeBetween > 0) {
			amount = timeBetween;
			printf("%s%zu microsecond%s", (hasPrinted ? ", " : ""), amount,
				   (amount == 1 ? "" : "s"));
		}

		puts(ANSI_COLOR_RESET);
	}
}
