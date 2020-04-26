CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -I.
LDFLAGS = -pthread -lm

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
	bin/misc/io.so \
	bin/misc/options.so \
	bin/misc/statistics.so \
	bin/redir/client.so \
	bin/redir/server.so \

server: main.c bin/dirinfo $(BINARIES)
	$(CC) $(CFLAGS) -o $@ main.c $(BINARIES) $(LDFLAGS)

bin/dirinfo:
	@mkdir -p bin/base
	@touch bin/dirinfo
	@mkdir bin/cache
	@mkdir bin/core
	@mkdir bin/http
	@mkdir bin/misc
	@mkdir bin/redir

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

clean:
	rm -rf bin
	rm -f server

memory:
	valgrind --num-callers=100 --leak-resolution=high --leak-check=full --track-origins=yes --show-leak-kinds=all --track-fds=yes ./server
