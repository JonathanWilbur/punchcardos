# PunchcardOS

Punchcard is a hyper-minimal Linux distro that contains just enough pre-compiled
code to compile and run C programs for the purposes of bootstrap a trustworthy
toolchain. Unless you compile from source, you don't really know that there
aren't backdoors, unwanted telemetry, malware, etc. in the code you are running.
This repository is meant to provide a minimal set of tools for "bootstrapping"
other bigger, more complex tools like the GNU C compiler without using a
pre-compiled GNU C compiler and glibc (among many other things).

## Namesake

"PunchcardOS" harkens back to the bootstrapping of computing by the use of
[punchcard programs](https://en.wikipedia.org/wiki/Punched_card).

## Details

PunchcardOS will start with a pared-down Linux kernel, built specifically to run
only on whatever hardware it is going to run on, but with no networking
capability (by default, at least), and with other features added or removed to
minimize code size while still accepting some security features. Ideally, this
kernel will use no hardware-specific cryptography features. Can you really trust
Intel to generate your keys without backdoors?

The binaries included in `./programs` will be written so as to compile using
`nolibc`, meaning that they can run as static binaries on a system that has no
libc implementation. Every program will be small, understandable, and
single-file, so that very little tooling is needed to build it.

For this system to be usable, you will at least need a pre-compiled shell and a
C compiler. These will be pre-compiled and included in the distro so you can
bootstrap a system. (Maybe we could even do away with the shell in the future by
making some small C compiler compile the shell, then run it.)

You won't even have an `ls` command, a text editor, `cat`, `less`, or anything
like that until you compile it in this constrained environment.

Contenders for the minimal C compiler are:

- [ChibiCC](https://github.com/rui314/chibicc)
- [c4](https://github.com/rswier/c4)

The shell will be based on [lsh](https://github.com/brenns10/lsh). All of the
above will be modified to compile with `nolibc`, possibly with reduced
functionality.

Source for the programs in the `./programs` folder will be included, which you
will need to compile in this constrained environment to use as a part of the
bootstrapping.

These programs include:

- A text editor based on [kilo](https://github.com/antirez/kilo)
- [An HTTP server](https://github.com/Francesco149/nolibc-httpd)
- Tools for encrypting and hashing
- Tools for making an HTTP POST request
- Common unix utilities, such as `ls` and `rmdir`
- Tools for inspecting assembly and binaries

(Obviously some of these won't work if you have no networking.)

The distro will also include clones of the
[`stage0` source](https://savannah.nongnu.org/projects/stage0) and
[GNU Mes](https://www.gnu.org/software/mes/), among other repositories that
contain common build tooling useful for bootstrapping other projects. This
project isn't meant to compete with these, but rather, complement them: to
provide a trustworthy, minimal distro on which you can use these tools for more
bootstrapping.

## nolibc

### About nolibc

`nolibc` was a feature snuck into the Linux kernel without much fanfare.
Basically, it is a few headers that implement just a subset of a proper libc
implementation so applications do not have to link against a third-party libc.
This is a libc that comes with the Linux kernel! You can read more about it
[here](https://lwn.net/Articles/920158/). The root of all the headers includes
documentation in the comments
[here](https://github.com/torvalds/linux/blob/55027e689933ba2e64f3d245fb1ff185b3e7fc81/tools/include/nolibc/nolibc.h).

You have to do a little work of your own to implement any libc functionality
you are not getting from these headers, but small programs can function without.

This is the perfect solution for PunchcardOS, since it cuts out the amount of
software needed to bootstrap a trustworthy system.

### Compiling with nolibc headers

First, your code has to be written to actually use `nolibc`. To do this, wrap
the headers you normally include with `#ifndef NOLIBC` and `#endif`. `nolibc`
defines `NOLIBC`. Doing this prevents duplicate definitions coming from `glibc`
or whatever other libc you use by default. If there are any unimplemented
standard library things you need, define them in an `#else` block.

To compile a static binary that is linked against `nolibc` (as described in the
header of `nolibc.h` in the Linux Kernel), run:

```bash
gcc -static -fno-asynchronous-unwind-tables -fno-ident -s -Os -nostdlib \
 -include PATH_TO_LINUX_SRC/tools/include/nolibc/nolibc.h \
 -o YOUR_BIN_NAME ./programs/YOUR_SRC_NAME -lgcc
```

(After replacing `YOUR_BIN_NAME`, `YOUR_SRC_NAME`, and `PATH_TO_LINUX_SRC`
obviously.) I don't know if the ordering of arguments matters, but it doesn't
seem like it.

You can see a good example of this in `./programs/basename.c`. The standard
library doesn't have a `basename` function, so I had to implement one that is
used only when compiling the `nolibc` variant.

## Tests

There is setup involved for these to work, but you can run the tests under
`test/` like so: `node ./test/enc.mjs`.

## License

Some of the software in this repository was copied from other repositories, and
are governed by a different result. Unless otherwise noted, all software in
this repo is licensed under the MIT License whose text here:

```
MIT License

Copyright 2023 Jonathan M. Wilbur <jonathan@wilbur.space>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## Bootstrapping Order

These are kind of my chicken-scratchy notes. I will clean these up as I figure
out how this is going to work.

### Get a trustworthy GCC

In order (I think):

- Get an M2-Planet compiler via stage0
- Re-compile shell and libc with M2-Planet?
- Build `gzip` https://github.com/wkoszek/mini_gzip
- Build hashing tools
- Use the M2-Planet compiler produced to bootstrap GNU Mes
- Make (https://github.com/waltje/MiniMake/tree/main)
- Use GNU Mes to build TinyCC (could be built using either Make or manually running all commands)
- Compile `oksh` https://github.com/ibara/oksh
  - This uses a shell script `./configure`, but it looks simple enough where you
    could manually perform those steps via other means.
- Use TinyCC and `oksh` to build Bash (Apparently no other shell will work with GCC)
  - Building Bash requires a shell, which does not have to be bash
- Use TinyCC to build GNU Make
- Build https://github.com/onetrueawk/awk
  - // TODO: Requires `bison`
  - Maybe this will work instead? https://github.com/raygard/wak/blob/main/monosrc/mono.c
- // TODO: GNU Binutils _might_ be required. Review host-specific instructions for GCC.
- // TODO: Perl _might_ be required, but it is unclear.
- You need `libgmp`, but this might suffice: https://github.com/evdenis/mini-gmp
- Build MPFR (The real deal, but it does not look outrageously complicated.)
- Build MPC like normal
- Build `libzstd` like normal: https://github.com/facebook/zstd
- It is unclear whether ISL is universally required for GCC...
- It is also unclear whether Python will be reuqired for GCC
- Use TinyCC to build GCC (https://gcc.gnu.org/install/prerequisites.html)
- Flex (https://github.com/ronpinkas/simplex)
- libc (not sure which version, but you need one to build GCC) (https://github.com/pitust/shimlibc)

The above steps will be done:

1. Once, quickly and sloppily, just to make sure that it's possible.
2. Again, but carefully reading all of the code before compiling.
3. Again, after implementing hardware obfuscation.
4. Again, on different hardware.

### Get a trustworthy Linux Kernel

For some reason, this page is where the requirements to build are found:

https://docs.kernel.org/process/changes.html

## See Also

- https://github.com/keiranrowan/tiny-core
- https://bootstrappable.org/
- https://reproducible-builds.org/
- https://savannah.nongnu.org/projects/stage0
- https://nixos.org/

## To Install

- https://github.com/xypron/usign