# Simpler Core Utils

This repo contains lightweight, simple rewrites of a select few of the GNU core
utils for the purposes of implementing an extremely small, almost unusable,
Linux distro that is only capable enough to minimally write C code, compile it,
hash it, encrypt it, and send it over HTTP, SMTP, or FTP.

Every program in the `programs/` folder is a single-file. Using `ls` as an
example, Compilation is as simple as:

```bash
gcc programs/ls.c -o ls
```

This Linux-distro-in-progress is called "PunchcardOS," harkening back to the
bootstrapping of computing by the use of
[punchcard programs](https://en.wikipedia.org/wiki/Punched_card).

## PunchcardOS

Provided that it proves to be feasible, PunchcardOS will start with
[Tilck](https://github.com/vvaltchev/tilck) as a lighter alternative to Linux,
[ChibiCC](https://github.com/rui314/chibicc) as a minimal C compiler, possibly
[NoLibC](https://github.com/wtarreau/nolibc) as a C library,
[lsh](https://github.com/brenns10/lsh) as a shell, and
[kilo](https://github.com/antirez/kilo) as a minimal text editor. With these
pre-compiled binaries alone, it should be possible to write and compile
further C programs. [crun](https://github.com/containers/crun) may be a suitable
container runtime, but a smaller, simpler alternative would be preferred;
[this code](https://github.com/w-vi/diyC) seems like it could be the start of a
makeshift solution.

### Running Tilck on QEMU

```bash
qemu-system-i386 -rtc base=localtime -drive index=0,media=disk,format=raw,file=tilck.img
```

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