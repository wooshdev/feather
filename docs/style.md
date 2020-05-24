# Style
This file contains the styling rules for writing C code. All code should comply to these rules.

## File Format

#### Licensing
The license should be included in all code files and be the first part of every file. The license can also be found in the `/COPYING` file.

#### Include Files
These are the formatting rules for `#include`'s:
- Kernel include's first
- Files from `/usr/include`, `/usr/local/include` and other global directories second (i.e. STL, POSIX, \*NIX, dependencies)
- Local files lastly

These three sections should be seperated by a blank line. Sorting per major category for `/usr/include` files is also good practice.

An example is shown here:
```c
#include <sys/time.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include "base/global_state.h"
#include "http/syntax.h"
#include "misc/default.h"
#include "misc/io.h"
#include "redir/server.h"
```

## General Formatting Rules
#### Line width
All code should fit in 80 columns. This rule is there so that users using command
line utilities can view the code without having to scroll vertically too much,
and most of the times it just looks better.

#### End of line
Lines shouldn't be ending with unnecessary invisible characters, like spaces and
 tabulators.

#### Indentation
Code indentation should almost always be a tabulator. Exceptions can be made for
 comments, and things like multidimensional array initialization.

#### Comments
Important comments or comments explaining the purpose of the current
 file/function/struct etc. should be written in the following form:

```c
/**
 * Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et
 * dolore magna aliqua.
 */
```

Small comments may be a single line:
```c
/* The following code makes the program run fast. */
```
