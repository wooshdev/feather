#!/usr/bin/python3

# BSD-2-Clause
# 
# Copyright (c) 2020 Tristan
# All Rights Reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS  SOFTWARE  IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND  ANY  EXPRESS  OR  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED  WARRANTIES  OF MERCHANTABILITY  AND FITNESS FOR A PARTICULAR PURPOSE
# ARE  DISCLAIMED.  IN  NO  EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE   FOR   ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL   DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE  GOODS  OR  SERVICES;  LOSS  OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION)  HOWEVER  CAUSED  AND  ON  ANY  THEORY OF LIABILITY, WHETHER IN
# CONTRACT,  STRICT  LIABILITY,  OR  TORT  (INCLUDING  NEGLIGENCE OR OTHERWISE)
# ARISING  IN  ANY  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import sys

class Colors:
	BLUE = "\033[34m"
	RED = "\033[31m"
	RESET = "\033[0m"
	YELLOW = "\033[33m"

if len(sys.argv) == 1:
	print(Colors.RED + "E: Please specify a file to lint!")
	print("Command Line Usage:%s %s <filename>" % (Colors.RESET, sys.argv[0]))
	sys.exit()

try:
	fd = open(sys.argv[1])
except (IOError, OSError) as e:
	print("%s%s" % (Colors.RED, e))
	print("Failed to open that file.%s" % Colors.RESET)
	sys.exit()

with fd:
	linenr = 0
	errors = 0
	for line in fd:
		linenr += 1
		count = 0
		for char in [*line]:
			if ord(char) == 0x9: # A tabulator character
				count += 4 - (count % 4)
			elif char != '\n':
				count += 1
		if count > 80:
			errors += 1
			print("Line %s%s:%i%s is too long: (%s%i%s characters)\n%s%s%s" % (
				Colors.BLUE, sys.argv[1], linenr, Colors.RESET,
				Colors.BLUE, count, Colors.RESET,
				Colors.YELLOW, line.strip(), Colors.RESET
			))
	print("File '%s' has %i error(s)" % (sys.argv[1], errors))
