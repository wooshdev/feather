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

#include <sys/utsname.h>

#include "misc/options.c"

#define GS_NO_POPULATE_PRODUCTNAME_WARNINGS

/* UNAME SETTINGS */
bool US_return_err = FALSE;
struct utsname US_info;

struct TestModule {
	const char	*name;
	bool (*func)(void);
};

int
FUNC_IMPL_uname(struct utsname *buf);

void
HintStdoutCarriageReturn(void);

bool
TestA(void);
bool
TestB(void);
bool
TestC(void);
bool
TestD(void);
bool
TestE(void);

#define FUNC_UNAME FUNC_IMPL_uname
#include <base/global_state.c>

int
main(void) {
	size_t	amt;
	size_t	i;
	size_t	j;
	size_t	longest;
	bool	ret;
	size_t	temp;

	strcpy(US_info.sysname, "undefined");
	strcpy(US_info.nodename, "undefined");
	strcpy(US_info.release, "undefined");
	strcpy(US_info.machine, "undefined");

	OMGSSystemInformationInServerHeader = OSIL_SYSNAME | OSIL_NODENAME |
										  OSIL_RELEASE | OSIL_MACHINE;

	struct TestModule modules[] = {
		{ "Self-test", TestA },
		{ "Failed call to uname()", TestB },
		{ "Empty Struct", TestC },
		{ "Long Values", TestD },
		{ "Regular Test", TestE },
	};

	/* Calculate amount of modules */
	amt = sizeof(modules) / sizeof(modules[0]);

	/* Calculate the longest string for formatting */
	longest = 0;
	for (i = 0; i < amt; i++) {
		temp = strlen(modules[i].name);
		if (longest < temp)
			longest = temp;
	}
	longest += 1;

	printf("Running "ANSI_COLOR_BLUE"%zu"ANSI_COLOR_RESET" tests.\n", amt);

	for (i = 0; i < amt; i++) {
		printf(ANSI_COLOR_MAGENTA"%s"ANSI_COLOR_RESET" running",
			   modules[i].name);
		fflush(stdout);
		ret = modules[i].func();
		printf(ANSI_COLOR_MAGENTA"\r%s "ANSI_COLOR_RESET, modules[i].name);
		for (j = strlen(modules[i].name); j < longest; j++)
			putchar('.');
		puts((ret ? ANSI_COLOR_GREEN" passed"ANSI_COLOR_RESET :
					ANSI_COLOR_RED" failed"ANSI_COLOR_RESET));
	}

	puts("\nFinished.");

	return EXIT_FAILURE;
}

int
FUNC_IMPL_uname(struct utsname *buf) {
	errno = EAGAIN;
	if (buf == NULL) return -1;
	if (US_return_err) return -1;
	errno = 0;

	memcpy(buf, &US_info, sizeof(struct utsname));

	return 0;
}

bool
TestA(void) {
	return TRUE;
}

bool
TestB(void) {
	bool ret;

	HintStdoutCarriageReturn();

	US_return_err = TRUE;
	ret = GSPopulateProductName();
	US_return_err = FALSE;

	return ret == FALSE;
}

bool
TestC(void) {
	size_t	i;
	size_t	len;

	len = sizeof(US_info.sysname) / sizeof(US_info.sysname[0]);
	for (i = 0; i < len; i++)
		US_info.sysname[i] = ((i + 1) == len) ? 0 : 'A';

	len = sizeof(US_info.nodename) / sizeof(US_info.nodename[0]);
	for (i = 0; i < len; i++)
		US_info.nodename[i] = ((i + 1) == len) ? 0 : 'B';

	len = sizeof(US_info.release) / sizeof(US_info.release[0]);
	for (i = 0; i < len; i++)
		US_info.release[i] = ((i + 1) == len) ? 0 : 'C';

	len = sizeof(US_info.machine) / sizeof(US_info.machine[0]);
	for (i = 0; i < len; i++)
		US_info.machine[i] = ((i + 1) == len) ? 0 : 'D';

	HintStdoutCarriageReturn();
	return GSPopulateProductName();
}

bool
TestD(void) {
	US_info.sysname[0] = '\0';
	US_info.nodename[0] = '\0';
	US_info.release[0] = '\0';
	US_info.machine[0] = '\0';

	HintStdoutCarriageReturn();

	return GSPopulateProductName();
}

bool
TestE(void) {
	strcpy(US_info.sysname, "FeatherOS");
	strcpy(US_info.nodename, "localhost.example");
	strcpy(US_info.release, "v1.0");
	strcpy(US_info.machine, "RISC-V");

	HintStdoutCarriageReturn();

	return GSPopulateProductName();
}

/* Carriage return stdout to make error printing from GSPopulateProductName
 * better formatted. */
void
HintStdoutCarriageReturn(void) {
	fputs("\r                                                     \r", stdout);
	fflush(stdout);
}
