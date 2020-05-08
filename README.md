# FeatherServer
FeatherServer is an open-source webserver that focuses on security and performance. Therefore, a very minimal configuration is used to reduce overhead.

## Feature Status
Currently the following features are present:
- An HTTP redirection server. This server requires secure connections. Non-secure connections will be upgraded by the RedirService.
- An HTTPS server, this is main component of the project.
- The industry standard HTTP/1.1 protocol is used to communicate with clients (webbrowsers et al.)
- Client-side Caching is enabled by handling conditional requests. This is achieved by sending a `Last-Modified` header and validating the future request header `If-Modified-Since`.
- File system overhead is reduced by caching files in memory.
- Proper system integration with `init`/`systemd` and the `/var/log/` system.

In the future, the following features can be expected:

- Automated Certificate Management Environment ([AMCE](https://en.wikipedia.org/wiki/Automated_Certificate_Management_Environment))
- [HTTP/2](https://en.wikipedia.org/wiki/HTTP/2)
- Online Certificate Status Protocol ([OCSP](https://en.wikipedia.org/wiki/Online_Certificate_Status_Protocol))
- A configure script (maybe Autotools, Meson, etc.)

## Platform Support <a name="platform-support"></a>
This code is designed to primarily use POSIX and C89 functions, but some minimal C99-standard functions and some libraries 
are used. This basically means that most UNIX-like systems are supported. Currently, the following platforms are supported:
- (GNU) Linux
- FreeBSD
- OpenBSD

Please refer to the build instructions for more details. Windows systems are not supported and will probably also not be
supported in the future.

## Dependencies
This project tries to keep the amount of dependencies used minimal, but some are simply mandatory.
### pkg-config
This project locates its' dependencies by using [`pkg-config`](https://www.freedesktop.org/wiki/Software/pkg-config/). On most systems, this is already installed. On some BSDs, it can be installed by running the following command:
```bash
$ pkg install pkgconf
```

### Transport Layer Security
FeatherServer's main component is the HTTPS server, which is a server that uses connections encrypted by the [TLS](https://en.wikipedia.org/wiki/Transport_Layer_Security) protocol. FeatherServer achieves this secure connection by using the [OpenSSL](https://github.com/openssl/openssl/) or one of its compatible forks, such as [LibreSSL](https://github.com/libressl-portable/portable). To quickly install OpenSSL, execute the following commands:
```bash
$ git clone https://github.com/openssl/openssl
$ cd openssl
$ ./config
$ make
$ sudo make install
```

If you prefer to use your package manager, please make sure you also have the developer package installed. On `apt` platforms (Debian/Ubuntu et al.), this is `libssl-dev`:
```bash
sudo apt install libssl-dev
```

### Brotli <a name="dependencies-brotli"></a>
To achieve fast transfers between the server and its' clients, FeatherServer uses [Brotli](github.com/google/brotli), an open-source compression library written in C and created by Google. To quick-install Brotli, execute the following commands:
```bash
$ git clone https://github.com/google/brotli
$ mkdir brotli/build && cd brotli/build
$ ../configure-cmake
$ make
$ sudo make install
```
To use your package manager, execute the following command:
```bash
sudo apt install libbrotli1 libbrotli-dev
```

## Getting the Code
To get a clone of the repository, run the following command:
```bash
$ git clone https://github.com/wooshdev/feather
```
This will create a new folder in your current working directory, which is named `feather`.

## Building FeatherServer
### Build Requirements
Make sure your system is supported. To check if your platform is supported, confirmt that it is listed in the [Platform Support](#platform-support) section.

A modern compiler is recommended, like [Clang](https://clang.llvm.org) or [GCC](https://gcc.gnu.org). There is currently no configure script, so the default compiler used is Clang. To change this, modify the `CC` variable in the [Makefile](Makefile). Changing the compiler can also be done by passing it to the `make` command, e.g. `make CC=gcc`.

## Compiling
Because of the lack of a configure script, this project contains a out-of-the-box working Makefile. Simply put: to build the project, execute the following:
```bash
$ make
```
If you wish to use a different compiler than the preconfigured `clang` compiler, such as the GCC compiler which is installed on most UNIX systems, do the following:
```bash
$ make CC=gcc
```
If you wish to enable the experimental HTTP/2 implementation, add the `ADDITIONAL_CFLAGS=-DOPTIONS_ENABLE_HTTP2` flag, e.g.:
```bash
$ make ADDITIONAL_CFLAGS=-DOPTIONS_ENABLE_HTTP2
```
This will set the `OPTIONS_ENABLE_HTTP2` macro and informs the compilation that HTTP/2 should be used. To combine options, simply concatenate:
```bash
$ make ADDITIONAL_CFLAGS=-DOPTIONS_ENABLE_HTTP2 CC=gcc
```
The above will use the `GCC` compiler and enable HTTP/2.

## Starting the server
The executable will compiled to `./server` in the current working directory, and isn't put in `/usr/bin`, `/usr/local/bin` or some other system directory. Also, no `init`/`systemd` intergration is available, thus running the server simply invoke the `./server` binary, either in tmux/screen or using jobs. Future integration with more systems may be expected.


## Testing
The project has been tested quite a bit. This section contains tools to test the code with. The Makefile contains targets useful for debugging, even though its primary use is building.
### Valgrind <a name="testing-valgrind"></a>
Valgrind is used for memory debugging, memory leak detection and system-resource leak detection. A useful make target is `memory`, which will run the executable in a controlled mode.
```bash
$ make memory
```
Note that this target does not (re)build the program.
### Cppcheck <a name="testing-cppcheck"></a>
[Cppcheck](http://cppcheck.sourceforge.net) is a static analysis tool. It can be used for finding bugs that weren't found or caught by human revision. To run the tool, make sure you have it installed (see the project page for more information). 
```bash
$ make cppcheck
```
### IWYU
[include-what-you-use](https://include-what-you-use.org) (IWYU) is a tool to find unnecessary `#include <...>`'s in code. Make sure you have [installed](https://github.com/include-what-you-use/include-what-you-use/blob/master/README.md#how-to-build) the tool before use. To run the tool, execute the following command:
```bash
$ make clean all -k CC=include-what-you-use
```
Explanation: The two `clean` and `all` targets will insure the complete codebase will be scanned. The `-k` option is there to insure the Makefile 'compilation' continues after warnings/errors occur. IWYU is a Clang plugin and acts like the compiler, therefore the `CC` variable is overwritten.
### clang-tidy
[clang-tidy](https://clang.llvm.org/extra/clang-tidy) is a linter tool that can – just like `cppcheck` – analyze code for possible bugs. The tool is a clang plugin, and runs as the compiler. `clang-tidy` doesn't recognise all the Clang/GCC CFLAGS/LDFLAGS as with normal compilation, so the `CC` variable can't be replaced. To run the tool, do the following:
```bash
$ clang-tidy -I . <compilation unit>
```
Where `<compilation unit>` is a .c file, for example `main.c`.
### Unit Tests
Standalone components should be tested using `Unit Tests`. These are tests that will use a certain component of the program and feed it various types of input and checks the result. The tests are located in the `tests` folder.
### Script Tests
The program can be roughly benchmarked using the following script:
```bash
$ time tests/redir.sh
```
This script will query the `Redirection Service` running on port 80 . It will perform 100 requests, and using the [`time`](https://man.openbsd.org/time) command the time of execution will be kept track off. Note that this script uses [`wget`](https://www.gnu.org/software/wget/), which is a tool that performs HTTP requests. It will probably be already installed on most Linux systems, but you can use `apt` if it doesn't. To install it on a BSD platform, execute the following command:

On NetBSD:
```bash
$ pkg install wget
```

On FreeBSD:
```bash
pkg_add wget
```

## Make targets
The following make targets are present in the Makefile:
| **Target** | **Description** |
|-|-|
| all | This is the default target, and will create the executable binary. |
| clean | This will delete all binaries created, and will restore the clone to the original state. |
| cppcheck| The cppcheck target analyzes the source code. For more information, see the [Cppcheck](#testing-cppcheck] test section. |
| memory | The memory target runs binary in an enclosed mode using Valgrind. For more information, see the [Valgrind](#testing-valgrind) test section. |

## License
This project is licensed under "BSD 2-Clause Simplified" [license](https://github.com/wooshdev/feather/blob/master/COPYING). This project uses the [Brotli](#dependencies-brotli) library for compression, which is [licensed](https://github.com/google/brotli/blob/master/LICENSE) under the MIT license. OpenSSL is mainly [licensed](https://github.com/openssl/openssl/blob/master/LICENSE) under the [Apache License v2](https://opensource.org/licenses/Apache-2.0). 
