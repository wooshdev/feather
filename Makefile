# Makefile work is licensed under a Creative Commons Attribution-ShareAlike 4.0
# International License. See <https://creativecommons.org/licenses/by-sa/4.0/>.

.PHONY: clean

include UserVariables.mk

ADDITIONAL_CFLAGS ?=
ADDITIONAL_LDFLAGS ?=
CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -I. $(ADDITIONAL_CFLAGS)
LDFLAGS = -pthread -lm $(ADDITIONAL_LDFLAGS)
CXX = clang++
CXXFLAGS = -Wall -Wextra -Wpedantic -g -I. $(ADDITIONAL_CFLAGS)

# Dependencies using pkg-config (OpenSSL, brotli)
DEPENDENCIES = openssl libbrotlicommon libbrotlienc
CFLAGS += `pkg-config --cflags $(DEPENDENCIES)`
LDFLAGS += `pkg-config --static --libs $(DEPENDENCIES)`

BINARIES = \
	bin/base/global_state.so \
	bin/cache/cache.so \
	bin/cache/compression.so \
	bin/core/h1.so \
	bin/core/h2.so \
	bin/core/security.so \
	bin/core/server.so \
	bin/http/response_headers.so \
	bin/http/strings.so \
	bin/http/syntax.so \
	bin/http2/frames/goaway.so \
	bin/http2/frames/headers.so \
	bin/http2/frames/priority.so \
	bin/http2/frames/rst_stream.so \
	bin/http2/frames/settings.so \
	bin/http2/frames/window_update.so \
	bin/http2/huffman/huffman.so \
	bin/http2/debugging.so \
	bin/http2/frame.so \
	bin/misc/io.so \
	bin/misc/options.so \
	bin/misc/statistics.so \
	bin/redir/client.so \
	bin/redir/server.so \

TEST_BINARIES = \
	bin/tests/http2/huffman \
	bin/tests/redir

all: server $(BINARIES) $(TEST_BINARIES)

server: main.c bin/dirinfo $(BINARIES)
	$(CC) $(CFLAGS) -o $@ main.c $(BINARIES) $(LDFLAGS)

# This target will create the directory structure for the build files. The
# binaries should have approximately the same path. For example: 'foo/bar.c'
# should cast to 'bin/foo/bar.o'
bin/dirinfo:
	@mkdir -p bin/base
	@touch bin/dirinfo
	@mkdir bin/cache
	@mkdir bin/core
	@mkdir bin/http
	@mkdir bin/http2
	@mkdir bin/http2/frames
	@mkdir bin/http2/huffman
	@mkdir bin/misc
	@mkdir bin/redir
	@mkdir bin/tests
	@mkdir bin/tests/base
	@mkdir bin/tests/base/global_state
	@mkdir bin/tests/http2

bin/base/global_state.so: base/global_state.c \
	base/global_state.h
	$(CC) $(CFLAGS) -c -o $@ base/global_state.c

bin/cache/cache.so: cache/cache.c \
	cache/cache.h \
	http/strings.h
	$(CC) $(CFLAGS) -c -o $@ cache/cache.c

bin/cache/compression.so: cache/compression.c \
	cache/compression.h \
	http/strings.h
	$(CC) $(CFLAGS) -c -o $@ cache/compression.c

bin/core/h1.so: core/h1.c \
	core/h1.h \
	core/security.h
	$(CC) $(CFLAGS) -c -o $@ core/h1.c

bin/core/h2.so: core/h2.c \
	core/h2.h \
	core/security.h \
	http2/frame.h \
	http2/session.h \
	http2/stream.h \
	misc/default.h
	$(CC) $(CFLAGS) -c -o $@ core/h2.c

bin/core/security.so: core/security.c \
	core/security.h
	$(CC) $(CFLAGS) -c -o $@ core/security.c

bin/core/server.so: core/server.c \
	core/server.h
	$(CC) $(CFLAGS) -c -o $@ core/server.c

bin/http/response_headers.so: http/response_headers.c \
	http/response_headers.h
	$(CC) $(CFLAGS) -c -o $@ http/response_headers.c

bin/http/strings.so: http/strings.c \
	http/strings.h
	$(CC) $(CFLAGS) -c -o $@ http/strings.c

bin/http/syntax.so: http/syntax.c \
	http/syntax.h
	$(CC) $(CFLAGS) -c -o $@ http/syntax.c

bin/http2/frames/goaway.so: http2/frames/goaway.c \
	http2/frames/goaway.h \
	http2/frame.h \
	http2/session.h
	$(CC) $(CFLAGS) -c -o $@ http2/frames/goaway.c

bin/http2/frames/headers.so: http2/frames/headers.c \
	http2/frames/headers.h \
	http2/frame.h \
	http2/session.h
	$(CC) $(CFLAGS) -c -o $@ http2/frames/headers.c

bin/http2/frames/priority.so: http2/frames/priority.c \
	http2/frames/priority.h \
	http2/frame.h \
	http2/session.h
	$(CC) $(CFLAGS) -c -o $@ http2/frames/priority.c

bin/http2/frames/rst_stream.so: http2/frames/rst_stream.c \
	http2/frames/rst_stream.h \
	http2/frame.h \
	http2/session.h
	$(CC) $(CFLAGS) -c -o $@ http2/frames/rst_stream.c

bin/http2/frames/settings.so: http2/frames/settings.c \
	http2/frames/settings.h \
	http2/settings.h \
	core/security.h \
	misc/default.h
	$(CC) $(CFLAGS) -c -o $@ http2/frames/settings.c

bin/http2/frames/window_update.so: http2/frames/window_update.c \
	http2/frames/window_update.h \
	http2/frame.h \
	http2/session.h
	$(CC) $(CFLAGS) -c -o $@ http2/frames/window_update.c

bin/http2/huffman/huffman.so: http2/huffman/huffman.c \
	http2/huffman/huffman.h \
	http2/huffman/data.h
	$(CC) $(CFLAGS) -c -o $@ http2/huffman/huffman.c

bin/http2/debugging.so: http2/debugging.c \
	http2/debugging.h
	$(CC) $(CFLAGS) -c -o $@ http2/debugging.c

bin/http2/frame.so: http2/frame.c \
	http2/frame.h \
	core/security.h
	$(CC) $(CFLAGS) -c -o $@ http2/frame.c

bin/misc/io.so: misc/io.c \
	misc/io.h
	$(CC) $(CFLAGS) -c -o $@ misc/io.c

bin/misc/options.so: misc/options.c \
	misc/options.h
	$(CC) $(CFLAGS) -c -o $@ misc/options.c

bin/misc/statistics.so: misc/statistics.c \
	misc/statistics.h
	$(CC) $(CFLAGS) -c -o $@ misc/statistics.c

bin/redir/client.so: redir/client.c \
	redir/server.h
	$(CC) $(CFLAGS) -c -o $@ redir/client.c

bin/redir/server.so: redir/server.c \
	redir/server.h
	$(CC) $(CFLAGS) -c -o $@ redir/server.c

# Tests
bin/tests/redir: tests/redir/main.c \
	redir/client.h
	$(CC) $(CFLAGS) -o $@ tests/redir/main.c -lpthread bin/redir/client.so \
		bin/base/global_state.so bin/http/syntax.so bin/misc/io.so \
		bin/http/response_headers.so bin/misc/statistics.so \
		bin/misc/options.so bin/http/strings.so

bin/tests/http2/huffman: tests/http2/huffman/huffman.cpp \
	bin/http2/huffman/huffman.so \
	http2/huffman/huffman.c \
	http2/huffman/huffman.h \
	http2/huffman/data.h
	$(CXX) $(CXXFLAGS) -o $@ tests/http2/huffman/huffman.cpp bin/http2/huffman/huffman.so

bin/tests/base/global_state/gspopulateproductname.so: \
	tests/base/global_state/gspopulateproductname.c \
	base/global_state.c \
	base/global_state.h \
	bin/misc/options.so
	$(CC) $(CFLAGS) -o $@ tests/base/global_state/gspopulateproductname.c \
		bin/misc/io.so \
		bin/misc/statistics.so \
		$(LDFLAGS)


# Destroys ALL build files, but will leave the source files intact.
clean:
	rm -rf bin
	rm -f server

## Tools

# the 'memory' target will invoke Valgrind, which will run the executable and
# can track memory usage. Memory leaks, double free()'s, use-after-free,
# uninitialised values, etc. can be found by using this tool.
memory:
	valgrind --num-callers=100 \
		 --leak-resolution=high \
		 --leak-check=full \
		 --track-origins=yes \
		 --show-leak-kinds=all \
		 --track-fds=yes \
		 ./server

# The 'Infer' target will run the infer program. This program statically
# analyzes code which helps in tracking down null pointer dereferences and
# errors of the kind. For more information, visit the official website:
# https://fbinfer.com/
infer:
	infer -r run -- make clean all

# the 'cppcheck' target will invoke the cppcheck program. This program 
# statically analyzes the code.
cppcheck:
	cppcheck -I. -q --std=c99 --enable=all --suppress=missingIncludeSystem .
