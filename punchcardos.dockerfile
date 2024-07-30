# This currently must be ran with the --privileged flag, because you have to
# create a mount to create the image. Hopefully I can fix this with a tool of
# my own.
# TODO: Use alpine
FROM ubuntu:latest
LABEL author="Jonathan M. Wilbur <jonathan@wilbur.space>"
LABEL version="0.0.1"
LABEL description="PunchcardOS: A minimal Unix-like distro for bootstrapping a trustworthy toolchain"

# This doesn't seem to be mentioned, but the Kernel requires Perl to build.
# Perl is included by default in Ubuntu.

# TODO: Clean this up, so I can find the true minimum I need to bootstrap PunchcardOS.
# TODO: I think I can get rid of bc by writing my own.
# The only place where the Kernel uses it for building is here: kernel/time/timeconst.bc.
# (It is used a lot for tests, I think.)
# rsync is required to install headers, but not to build the Kernel itself.
ENV KERNEL_REQS="make gcc flex bison libelf-dev bc binutils rsync"
ENV KERNEL_OPT="libncurses-dev"
ENV KERNEL_UNK="cpio clang dwarves jfsutils reiserfsprogs xfsprogs btrfs-progs libssl-dev"
# Currently, all of the below are used, except maybe vim. IDK why I included it.
ENV OTHER_DEPS="git vim gpg curl dosfstools bzip2 xz-utils perl tar cpio syslinux"

# Linux Kernel requirements: https://docs.kernel.org/process/changes.html
# dwarves installed pahole
RUN apt update && apt install -y ${KERNEL_REQS} ${OTHER_DEPS} ${KERNEL_UNK}
# I am not sure what to do about udev; it seems like installing that could wreck my computer.
# mcelog ignored. I don't see how to install this.

# The initramfs: the in-memory file system the kernel loads at startup.
RUN mkdir -p /build/initramfs
# For files that will be included, but not part of the initramfs.
RUN mkdir -p /build/files
# Where needed pre-compiled executables will be built from source.
RUN mkdir -p /build/stage/src

# Where we store out PGP keys.
RUN mkdir /pgp
ADD /pgp/* /pgp

RUN gpg --import /pgp/gregkh.gpg
RUN gpg --import /pgp/torvalds.gpg

# We are skipping Rust for now.
# TODO: Add --profile minimal
# RUN curl -sSf https://sh.rustup.rs | sh -s -- -y
# RUN . "$HOME/.cargo/env"
# RUN $HOME/.cargo/bin/cargo install bindgen-cli

ENV LINUX_VERSION=6.9.6

RUN curl https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${LINUX_VERSION}.tar.xz -o linux.tar.xz
RUN curl https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${LINUX_VERSION}.tar.sign -o linux.tar.sign

# The TAR archive is signed, not the compressed XZ file.
RUN xz -cd linux.tar.xz | gpg --verify linux.tar.sign -

RUN mkdir /build/stage/src/linux
RUN tar -C /build/stage/src/linux --strip-components 1 -xvf linux.tar.xz

WORKDIR /build/stage/src/linux
RUN make mrproper
ADD mods mods
ENV LSMOD=/build/stage/src/linux/mods
# IDK why, but it seems that you have to first have a config before
# localyesconfig will work.
# RUN make allnoconfig
# RUN make localyesconfig
RUN make ARCH=x86_64 x86_64_defconfig
# RUN make allmodconfig

ADD ./scripts/configure_kernel.sh ./configure_kernel.sh
RUN chmod +x configure_kernel.sh \
    && ./configure_kernel.sh \
    && rm configure_kernel.sh

# This is a Reproducible Builds standard
RUN SOURCE_DATE_EPOCH=1719086180 make -j 4

RUN mkdir /build/stage/linux-headers
RUN make headers_install ARCH=x86 INSTALL_HDR_PATH=/build/initramfs/usr

RUN cp arch/x86/boot/bzImage /build

RUN mv /linux.tar.xz /build/files
RUN mv /linux.tar.sign /build/files

# Add software licenses
RUN mkdir -p /build/initramfs/lic
ADD /licenses/* /build/initramfs/lic

# Add minimal programs
RUN mkdir -p /build/initramfs/src/punchcard
ADD /programs/* /build/initramfs/src/punchcard

# Build minimal programs to use our minimal system call interface as well as straplibc.
ENV NOLIBC_INC="-include /build/stage/src/linux/tools/include/nolibc/nolibc.h"
ENV NOLIBC_FLAGS="-static -Os -nostdlib"
WORKDIR /build
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/cat /build/initramfs/src/punchcard/cat.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/sh /build/initramfs/src/punchcard/sh.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/ls /build/initramfs/src/punchcard/ls.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/c4 /build/initramfs/src/punchcard/c4.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/kilo /build/initramfs/src/punchcard/kilo.c

RUN mkdir -p initramfs/sbin
RUN mkdir -p initramfs/usr/include
RUN cp -R /build/stage/src/linux/tools/include/nolibc initramfs/usr/include

################################################################################
#                                                                              #
# In the following section, we pre-fetch the minimal dependencies we need to   #
# build another Punchcard OS. Since the first pass of PunchcardOS will not     #
# have any networking, it will be unable to fetch these dependencies remotely. #
# Using these, dependencies, we can build a second pass of PunchcardOS that    #
# does have networking.                                                        #
#                                                                              #
################################################################################

# TODO: Build sha256sum and confirm hashes for all of the below.
WORKDIR /build/initramfs/src
ADD ./scripts/fetch_sources.sh ./fetch_sources.sh
RUN chmod +x ./fetch_sources.sh \
    && ./fetch_sources.sh \
    && rm ./fetch_sources.sh

# Build our minimal system calls interface
RUN mkdir -p /build/initramfs/lib
RUN gcc /build/initramfs/src/libsyscall/syscall-x86-64.s -c -o /build/stage/syscall.o
RUN ar rcs /build/initramfs/lib/libsyscall.a /build/stage/syscall.o

RUN mkdir /build/initramfs/src/tcc
RUN tar --strip-components=1 -C /build/initramfs/src/tcc -xvf tcc.tar.gz
WORKDIR /build/initramfs/src/tcc
ADD ./tinycc_undef_fn_macros.patch ./tinycc_undef_fn_macros.patch
RUN git apply tinycc_undef_fn_macros.patch
ADD ./tinycc_exec_fns.patch ./tinycc_exec_fns.patch
RUN git apply tinycc_exec_fns.patch

# TODO: This statically links glibc! There is no escape!
RUN ./configure --enable-static --extra-cflags=-static --extra-ldflags=-static
RUN make
RUN mv ./tcc /build/initramfs/cc
WORKDIR /build
# /init is hard-coded into the Linux kernel to run automatically when the
# initramfs is loaded. We are using the shell as our init process.
RUN cp initramfs/sh initramfs/init

RUN chmod +x initramfs/init

# TODO: Investigate why this works, but the static library (libsyscall.a) does not.
RUN cp /build/stage/syscall.o /build/initramfs/lib/syscall.o

# NOTE: You have to cd into the initramfs folder so that `find .` does not
# return a list of entries that begin with `./initramfs/`, otherwise, your
# entire initramfs is going to be in a folder called `initramfs` when extracted!
WORKDIR /build/initramfs
RUN find . | cpio -o -H newc > ../init.cpio
WORKDIR /build
# RUN dd if=/dev/zero of=boot bs=1M count=1000
# Truncate has an advantage of creating a sparse file on supporting systems,
# which means that this won't really take up this much space until something is
# actually stored in it.
RUN truncate -s 1GB boot
RUN mkfs -t fat boot

# Syslinux makes this into a bootable partition.
RUN syslinux boot

# Run these steps with the --privileged flag.
# RUN mkdir m
# TODO: Write something that can do this without an actual mount.
# RUN mount boot m
# RUN cp bzImage init.cpio m
# RUN umount m

RUN echo 'Build complete. Output files named bzImage and init.cpio.'
