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
 * OM  is an abbreviation for Options Manager
 * OMS is an abbreviation for Security Options Manager
 */

#ifndef MISC_OPTIONS_H
#define MISC_OPTIONS_H

#include "misc/default.h"

/**
 * The system information level. You can choose to include information about
 * the host operating system on which the server is running, for debugging 
 * purposes.
 *
 * This information is sent among with requests in the form of a Server header,
 * e.g.: 'WFS (Linux localhost 5.4.0-28-generic x86_64)'
 *
 * It is highly discouraged to include this security-sensitive information in
 * production, since attackers can use this information to check to see if the
 * operating system has had any vulnerabilities and hasn't been updated since.
 *
 * This option is implemented by querying the operating system using a system
 * call to uname(2).
 *
 * The following bits of information can be included in the header, which are
 * collected by the uname(2) call.
 * sysname:  The operating system name.
 * nodename: The name of the node which is probably the domain name.
 * release:  The release level (i.e. the version of the operating system).
 * machine:  The name of hardware type (e.g. amd64, x86_64, i386, etc.)
 *
 * An example of the bits listed above:
 * sysname:  Linux
 * nodename: localhost
 * release:  5.4.0-28-generic
 * machine:  x86_64
 *
 * On Linux systems, more specific information about the distribution may be
 * welcome:
 * distname:        The name of the distribution
 * distversion:     The release/version of the distribution
 * distcodename:    The codename of the distribution
 * distdescription: The description of the distribution
 *
 * On Ubuntu 20.04, the above result in:
 * distname:        Ubuntu
 * distversion:     20.04
 * distcodename:    focal
 * distdescription: Ubuntu 20.04 focal
 *
 * On non-Linux systems, the dist* variables are ignore. The values will appear
 * in the order that they are listed here.
 *
 * For more information on the meaning of these bits, visit a standard such
 * as SUS or POSIX, e.g. at:
 * https://pubs.opengroup.org/onlinepubs/007908799/xsh/uname.html
 */
enum OSILevel {
	OSIL_NONE = 0,
	OSIL_SYSNAME = 1,
	OSIL_NODENAME = 2,
	OSIL_RELEASE = 4,
	OSIL_MACHINE = 8,
	OSIL_DISTNAME = 16,
	OSIL_DISTVERSION = 32,
	OSIL_DISTCODENAME = 64,
	OSIL_DISTDESCRIPTION = 128
};

extern const char	*OMSCertificateFile;
extern const char	*OMSCertificateChainFile;
extern const char	*OMSCertificatePrivateKeyFile;
extern const char	*OMSCipherList;
extern const char	*OMSCipherSuites;

extern const char	*OMCacheLocation;

extern enum OSILevel OMGSSystemInformationInServerHeader;

/* Functions */
void
OMDestroy(void);

bool
OMSetup(void);

#endif /* MISC_OPTIONS_H */
