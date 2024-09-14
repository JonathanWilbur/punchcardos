# PunchcardOS

Punchcard is a hyper-minimal Linux distro that contains just enough pre-compiled
code to compile and run C programs for the purposes of bootstrap a trustworthy
toolchain. Unless you compile from source, you don't really know that there
aren't backdoors, unwanted telemetry, malware, etc. in the code you are running,
and even if you inspect the source, the compiler, or another tool you use could
be inserting malware into its outputs (Supply Chain Attack).

This repository is meant to provide a minimal set of tools for "bootstrapping"
other bigger, more complex tools like the GNU C compiler without using a
pre-compiled GNU C compiler and glibc (among many other things) so that you can
have a trustworthy system to build up from.

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

The minimal C compiler will be based on
[ChibiCC](https://github.com/rui314/chibicc), and the shell will be based on
[lsh](https://github.com/brenns10/lsh). All of the above will be modified to
compile with `nolibc`, possibly with reduced functionality. The assembler will
be based on [minias](https://github.com/andrewchambers/minias). Maybe
[this](https://github.com/stfsux/rld/tree/master) will work as a small linker,
or perhaps [this](https://github.com/ushitora-anqou/aqcc/tree/master/ld)?
ELF utils is just absolutely huge, so it would be preferable to avoid it.

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

## The Goal

The goals of this project is to create a distro that is small enough where all
of the code could reasonably be reviewed and understood, but which can also
bootstrap itself, using only a pre-compiled Linux kernel, C toolchain, and
shell: the bare minimum to bootstrap.

Once PunchcardOS itself has been bootstrapped using its own small, trustworthy
tooling, you could assume that its pre-compiled kernel, C toolchain, and shell
are also trustworthy, and therefore you have a fully trustworthy system. On the
second pass, the kernel will be compiled with networking support, so that source
archives can be fetched from remote sources to bootstrap any software.

In short:

1. Build PunchcardOS (using untrustworthy tooling)
2. Use PunchcardOS to bootstrap PunchcardOS (thereby producing a trustworthy PunchcardOS)
3. Use PunchcardOS (now trustworthy) to bootstrap any software
4. Publish the hashes of your build outputs so others can verify, or publish the
   build outputs themselves.

## Executable Standards

All executables in this project will be able to be built against nolibc, consist
of only a single file, and will use hard-coded timestamps (except the `date`
command).

### Hard-Coded Timestamps

We want the executables produced using the tools in this repository to have
consistent outputs, regardless of the machine or the time in which they were
created. A given set of inputs and tooling, should always produce a
byte-for-byte equal output, so that they (or hashes of them) can be compared
between bootstrapping attempts on different machines.

As such, source files in this repository, if taken from other projects, will be
modified so that they use a fixed timestamp where they would normally use the
real timestamp.

Areas where code was modified to hard-code a timestamp will be annotated with a
comment that contains `HARDCODED_TIMESTAMP`, in that exact casing, so that you
can search for these and change it, if desired.

The timestamp to which these will be set is to be decided upon, but it will be
documented here.

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
gcc -static -nostdlib -include PATH_TO_LINUX_SRC/tools/include/nolibc/nolibc.h \
 -o YOUR_BIN_NAME ./programs/YOUR_SRC_NAME -lgcc
```

(After replacing `YOUR_BIN_NAME`, `YOUR_SRC_NAME`, and `PATH_TO_LINUX_SRC`
obviously.) I don't know if the ordering of arguments matters, but it doesn't
seem like it.

You can see a good example of this in `./programs/basename.c`. The standard
library doesn't have a `basename` function, so I had to implement one that is
used only when compiling the `nolibc` variant.

## Building PunchcardOS

```bash
# Define which kernel modules are minimally required to run your hardware.
lsmod > mods

# Actual build of the distro.
docker build . -t pcosbuild -f ./punchcardos.dockerfile

# Privileged step required to mount filesystem.
sudo docker run --privileged pcosbuild bash \
-c 'mkdir m && mount boot m && cp bzImage init.cpio syslinux.cfg files/* m && umount m'

# This gets the ID of the most recent pcosbuild container.
CONTAINER_ID=$(docker ps --format 'table {{.ID}} {{.Image}}' -a | grep pcosbuild | head -n1 | cut -d ' ' -f1)

docker cp $CONTAINER_ID:/build/boot pcos.img
```

## Running PunchcardOS

### On QEMU

At minimum, you must run this:

```bash
qemu-system-x86_64 -drive file=pcos.img,format=raw,index=0
```

It is recommended that you add `-serial stdio` at the end of that, but I am not
your boss.

When it boots up, you should see a Syslinux prompt. This is because there is no
configuration to tell Syslinux _what_ to boot and _how_ to boot it. At this
prompt, type in:

```
/bzImage -initrd=/init.cpio root=/dev/sda
```

If you included `-serial stdio` on the QEMU command, add ` console=ttyS0` to the
end of the above Syslinux prompt, and hit `ENTER`. You should see a shell
appear in either the graphical QEMU window, or in the terminal where you ran
QEMU. Type `ls` and hit `ENTER`. If you see a listing of entries, it booted!

## Compiling within PunchcardOS

Since PunchcardOS only comes with a minimally working C compiler, you will need
to compile a C program to build anything else. As an example, you can compile a
program using the Tiny C compiler using a command like:

```bash
cc -nostdlib -static -g -include /src/straplibc/src/nolibc.h \
  /lib/syscall.o /src/tcc/lib/va_list.o /src/punchcard/ls.c
```

You might need to include `/src/tcc/lib/va_list.o`. The compiled binary does not
have access to the `__va_arg` symbol without this for some reason (to be
investigated).

`/lib/syscall.o` is a pre-compiled ELF object allowing you to make system calls
using `straplibc`. `straplibc` is an even smaller `nolibc` that was specifically
tailed for bootstrapping purposes.

### On Physical Hardware

PunchcardOS is yet to have been tested on physical hardware.

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

## Other Notes

### Ambiguous Error with Docker

After adding the steps to the dockerfile to fetch the tarballs from remote
sources, I was getting an error that said something to the effect of

```bash
failed to solve: failed to prepare $SOMETHING_1 as $SOMETHING_2: max depth exceeded
```

As it turns out, this absolutely spectacular and unambiguous error message means
that you have too many steps in your Dockerfile, and you need to condense steps.
I got by it by collapsing simliar steps into the scripts
`./scripts/configure_kernel.sh` and `./scripts/fetch_sources.sh`.

### Getting the distro booting

Building Linux 6.9.6 on an 11th Gen Intel(R) Core(TM) i7 with 16 GB RAM using
the `allmodconfig` build followed by the config changes in the
`punchcardos.dockerfile` and using `make -j4` took almost exactly two hours.
I tried this because the kernel was panicking, and I assumed that I was
probably missing some driver Qemu needed, so compiling absolutely everything
should make it work, right?

(I do wonder if the compiled kernel modules would even be available in
`initramfs`, so this might not have fixed my problem anyway. I don't really know
how kernel modules work, but I think they are stored on disk, so I don't know
how the kernel would be able to read them before there are any disks mounted!)

After using the `allmodconfig`, the distro still failed to start up, but
differently this time. It seemed to be panicking on some sort of tracing
self-test. I tried disabling kernel configs related to this and I don't remember
what the result was, but since I was going to have to do another two-hour long
`allmodconfig` build again to see if that fixed it, I just gave up on this
avenue of troubleshooting.

After trying to get this to work for hours, I finally figured out how to get the
kernel logs to display in `stdout` using Qemu (specifically so I can scroll up
and see earlier kernel ring buffer messages that are otherwise lost when they go
off-screen). You have to add `-serial stdio` to your `qemu-system-x86_64`
command like so:

```bash
qemu-system-x86_64 -drive file=pcos.img,format=raw,index=0,media=disk -serial stdio
```

**and** when the Syslinux prompt appears, you have to add `console=ttyS0` to the
kernel parameters, like so:

```
/bzImage -initrd=/init.cpio console=ttyS0
```

The former tells Qemu to use `stdin` and `stdout` for the serial terminal, and
the latter tells the Linux kernel to use this serial terminal as its console.
It seems like some kernel messages will still get displayed in the Qemu window,
still, but they are sparse enough to not be a problem.

This was useful for figuring out why I couldn't boot: the Linux kernel was
panicking because I did not specify a root filesystem via the `root` parameter.
I didn't know that was required. I still haven't figured out the fix to this,
but I have gotten further.

After testing all options for the `root` parameter, I found that the Linux
kernel was successfully mounting a disk, but not finding `/sbin/init`. It turned
out that, because I was using the `find` command in the directory above the
initramfs to create the CPIO archive, this archive had all of its contents in an
`initramfs` folder, making the correct path `/initramfs/sbin/init`. I haven't
fixed this yet, but I figured I should document it as soon as I realized it.
You can view the contents of CPIO archives using `cpio -itv < archive.cpio`.

After getting past that hurdle, the only way I could get the disk to be "found"
by the kernel was to remove the `media=disk` parameter from Qemu options. It is
weird that this worked, because apparently this is the default behavior.

After that, it could not find `/sbin/init`. I changed the build so that this was
placed at `/init`. That fixed it and now it works, but I have no idea why. The
documentation specifically lists where the kernel searches for init programs,
and this is not one of those places!

I had another problem later on: after adding the downloading of tarballs from
online into the `init.cpio`, and after refactoring this into a Bash script
outside of the Dockerfile, the Syslinux boot prompt would complain that there
was "No such file or directory" as `init.cpio`, even though copying this file
into the final image succeeded. Once again, a piece of shit program is reporting
a misleading error message: the problem was that the system didn't have enough
memory to load the 224M initrd filesystem. I changed it to 1G, which got it to
boot up successfully.

### A C Compiler

As it turns out `c4` is not viable as a bootstrapping compiler. It basically
only works for compiling itself, and its codebase is so abstruse that it is not
suitable for a "bootstrapping" compiler. ChibiCC seems more promising. I think
I will leave `c4` in the distro, though. If somebody can make it work for their
needs, it's fine to have another C compiler lying around.

UPDATE: After experimenting with ChibiCC, I don't think that will work either.
Specifically, nolibc uses too many GCC-specific features where I don't think I
can make nolibc work. See comments in `./programs/chibicc.c`. I _might_ be able
to make this work by using a modified nolibc. The biggest thing I need to
eliminate is the GCC-style inline assembly.

### Segfault in `strlen` when using nolibc

Make sure you are using recognized format specifiers in printf and the like. The
segfault was happening because I was using `%X`, when only `%x` was supported.
I need to report this. I don't think it should do this.

## See Also

- https://github.com/keiranrowan/tiny-core
- https://github.com/rofl0r/hardcore-utils
- https://bootstrappable.org/
- https://reproducible-builds.org/
- https://savannah.nongnu.org/projects/stage0
- https://nixos.org/

## To Install

- https://github.com/xypron/usign

## To Do

- [ ] MVP
  - [x] ~~`minias`~~
  - [ ] `elfread`
    - [ ] Print symbols
    - [ ] Print symbols in executable code
    - [ ] Print relocations
  - [ ] `ld`
  - [x] `minibash`
  - [x] ~~`peg/leg`~~
  - [x] `hexdump`
  - [ ] `hunt`
  - [ ] `peck`
  - [ ] `pex` (Peck, but for Hex)
  - [ ] `ar`
  - [x] `minised`
  - [x] ~~`miniawk`~~
    - This is too heavily dependent on regex and math. It will have to compile against `musl`.
  - [x] `cpio`
  - [ ] A bootloader (https://github.com/owenson/tiny-linux-bootloader/tree/master)
  - [x] ~~`syslinux`~~
  - [x] ~~`mtools`~~
  - [x] `m4`
  - [x] `dd`
  - [x] `truncate`
  - [ ] `chmod`*
  - [ ] `chown`*
  - [ ] `find`
  - [ ] `grep`
  - [ ] `httpget`
  - [ ] `stty`
  - [x] `sleep`
  - [ ] `dmesg`
  - [x] `stat`
  - [x] `touch`
  - [ ] `which`
  - [x] `hostname`
  - [ ] `strings` (https://github.com/michael105/minicore/blob/master/porting/minutils/src/strings.c)
  - [ ] `tty` ()
  - [ ] `[` (https://raw.githubusercontent.com/michael105/minicore/master/porting/minutils/src/%5B.c)
  - [x] `mv`
  - [ ] `cp`
  - [ ] Some hashing program
  - [ ] C runtime that drops capabilities, changes root, landlock?
- [ ] Mount initramfs as read-only
- [ ] Is it possible to make single-file `binutils` / `elfutils` commands?
- [ ] Tools for blacklisting or removing modules?
- [x] Syslinux configuration
- [x] Clear kernel ring buffer messages from screen
- [x] Get a working C compiler
- [ ] Shuffle kernel syscall numbers so malicious hardware is clueless
- [ ] Make a linker
- [ ] Make an assembler
- [ ] Container building via [Earthly](https://github.com/earthly/earthly)
- [ ] https://github.com/oriansj/torture_c/tree/main
- [ ] https://github.com/jart/sectorlisp
- [ ] https://github.com/realchonk/microcoreutils/tree/master
- [ ] I still haven't found a suitable ELF / x86-64 disassembler.
- [ ] Signed commits
- [ ] Signed releases
- [ ] Signed container images
- [ ] Use `seccomp()`
- [ ] Use `chroot()`
- [ ] Document special / non-standard programs
- [ ] Use TPM for some RNG
- [ ] Configure Integrity Management Architecture (IMA)?
- [ ] [`no_new_privs`](https://docs.kernel.org/userspace-api/no_new_privs.html)
- [ ] Use Landlock
- [ ] Drop capabilities
- [ ] `setcap` / `getcap` programs
- [ ] Executable minification
- [ ] Programs
  - [x] `arch`
  - [ ] `asn1parse`
  - [ ] `awk`
  - [x] `basename`
  - [x] `bc4linux`
  - [ ] `bc`
  - [ ] `[`
  - [ ] `c4`
  - [x] `cat`
  - [ ] `chgrp`
  - [ ] `chibicc`
  - [ ] `chmod`
  - [ ] `chown`
  - [ ] `chroot`
  - [ ] `cksum`
  - [x] `clear`
  - [ ] `cmp`
  - [ ] `col`
  - [ ] `colrm`
  - [ ] `comm`
  - [x] `cowsay`
  - [ ] `cp`
  - [ ] `cpio`
  - [ ] `cut`
  - [x] `date`
  - [ ] `dd`
  - [ ] `diff3`
  - [ ] `diff`
  - [x] `dirname`
  - [ ] `dmesg`
  - [x] `echo`
  - [ ] `edit`
  - [ ] `enc`
  - [ ] `env`
  - [ ] `expand`
  - [ ] `expr`
  - [x] `false`
  - [ ] `file`
  - [ ] `find`
  - [ ] `fmt`
  - [ ] `fold`
  - [ ] `getcap`
  - [ ] `getconf`
  - [ ] `getopt`
  - [ ] `grep`
  - [ ] `groff`
  - [ ] `groups`
  - [ ] `halt`
  - [x] `head`
  - [x] `hexdump`
  - [ ] `hostid`
  - [x] `hostname`
  - [ ] `httppost`
  - [ ] `hunt`
  - [ ] `id`
  - [ ] `installkernel`
  - [ ] `ischroot`
  - [ ] `join`
  - [ ] `kill`
  - [x] `kilo`
  - [ ] `ld`
  - [ ] `less`
  - [ ] `lisp`
  - [ ] `logger`
  - [ ] `look`
  - [x] `ls`
  - [ ] `m4`
  - [x] `make`
  - [ ] `man`
  - [ ] `minias`
  - [ ] `minilisp`
  - [ ] `mkdir`
  - [x] `mount`
  - [x] `mv`
  - [ ] `nl`
  - [ ] `otp`
  - [ ] `patch`
  - [ ] `pbget`
  - [ ] `pbput`
  - [x] `pivot_root`
  - [ ] `pmap`
  - [ ] `poweroff`
  - [ ] `ps`
  - [x] `pwd`
  - [ ] `rand`
  - [ ] `readelf`
  - [ ] `reboot`
  - [ ] `reset`
  - [x] `rm`
  - [x] `rmdir`
  - [ ] `sdiff`
  - [ ] `sed`
  - [ ] `seq`
  - [ ] `setcap`
  - [ ] `sha256`
  - [ ] `sha3`
  - [x] `sh`
  - [ ] `shuf`
  - [ ] `shutdown`
  - [x] `sleep`
  - [ ] `sort`
  - [ ] `split`
  - [x] `stat`
  - [ ] `stty`
  - [ ] `sum`
  - [ ] `tabs`
  - [ ] `tac`
  - [ ] `tail`
  - [ ] `tar`
  - [ ] `tee`
  - [ ] `test`
  - [ ] `time`
  - [ ] `timeout`
  - [ ] `tinylisp`
  - [x] `touch`
  - [ ] `tr`
  - [ ] `tree`
  - [x] `true`
  - [x] `truncate`
  - [ ] `tset`
  - [ ] `tty`
  - [x] `uname`
  - [ ] `unexpand`
  - [ ] `uniq`
  - [ ] `uptime`
  - [ ] `users`
  - [ ] `uuidgen`
  - [ ] `wc`
  - [ ] `whereis`
  - [ ] `which`
  - [x] `whoami`
  - [ ] `xargs`
