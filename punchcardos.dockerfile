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
ENV KERNEL_REQS="make gcc flex bison libelf-dev bc binutils"
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

# You might ask yourself: why did I hard-code the GPG keys into this repository?
# The reason is that the keys are distributed from the same source as the source
# tarballs, so if the source tarballs are compromised, the keys could be
# compromised as well. In other words, there is no point in getting a tarball
# from source X, then verifying them using the keys from source X.
#
# On the other hand, embedding the keys in the repository ensures that two
# distribution points must be compromised to compromise the installation.
#
# GPG keys are not supposed to change often, but if they do legitimately change,
# you can just create a PR to update the keys, or fork this repo.
RUN gpg --import /pgp/gnu-keyring.gpg
RUN gpg --import /pgp/gregkh.gpg
RUN gpg --import /pgp/torvalds.gpg
RUN gpg --import /pgp/musl.gpg
RUN gpg --import /pgp/madler.gpg

# Used for QEMU.
# Obtained from: https://keys.openpgp.org/vks/v1/by-fingerprint/CEACC9E15534EBABB82D3FA03353C9CEF108B584
# Found by: https://www.qemu.org/download/
RUN gpg --import /pgp/mroth.gpg

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

# TODO: Refine this further
RUN ./scripts/config --set-val CONFIG_NET n
RUN ./scripts/config --set-val CONFIG_ELF_CORE y
RUN ./scripts/config --set-val CONFIG_AUTOFS_FS y
RUN ./scripts/config --set-val CONFIG_IKCONFIG y
RUN ./scripts/config --set-val CONFIG_IKCONFIG_PROC y

RUN ./scripts/config --set-val CONFIG_BLK_DEV y
RUN ./scripts/config --set-val CONFIG_BLK_DEV_INITRD y

RUN ./scripts/config --set-val FAT_FS y
RUN ./scripts/config --set-val MSDOS_FS y
RUN ./scripts/config --set-val VFAT_FS y
RUN ./scripts/config --set-val EXFAT_FS y

# This might be needed for initramfs to work.
RUN ./scripts/config --set-val CONFIG_BLK_DEV_RAM y
RUN ./scripts/config --set-val CONFIG_BLK_DEV_RAM_COUNT 1
# The number is in unit of 1014B. Currently set to 128MB.
RUN ./scripts/config --set-val CONFIG_BLK_DEV_RAM_SIZE 131072

RUN ./scripts/config --set-val CONFIG_TTY y
RUN ./scripts/config --set-val CONFIG_SERIAL_8250 y
RUN ./scripts/config --set-val CONFIG_SERIAL_8250_CONSOLE y

RUN ./scripts/config --set-val CONFIG_BINFMT_ELF y
RUN ./scripts/config --set-val CONFIG_COMPAT_BINFMT_ELF y
RUN ./scripts/config --set-val CONFIG_BINFMT_SCRIPT y

RUN ./scripts/config --set-val CONFIG_CRYPTO_MANAGER_DISABLE_TESTS y

# I believe some of these 
RUN ./scripts/config --set-val CONFIG_PCI y
RUN ./scripts/config --set-val CONFIG_ATA y
RUN ./scripts/config --set-val CONFIG_BLK_DEV_GENERIC y
RUN ./scripts/config --set-val CONFIG_ATA_PIIX y
RUN ./scripts/config --set-val CONFIG_IDE y
RUN ./scripts/config --set-val CONFIG_BLK_DEV_IDE y
RUN ./scripts/config --set-val CONFIG_BLK_DEV_IDEPCI y
RUN ./scripts/config --set-val CONFIG_64BIT y
RUN ./scripts/config --set-val CONFIG_BLK_DEV_SD y

# Disabled because I was getting kernel panics in QEMU from some sort of tracing
# self-tests.
RUN ./scripts/config --set-val CONFIG_FTRACE_SELFTEST n
RUN ./scripts/config --set-val CONFIG_FTRACE_STARTUP_TEST n
RUN ./scripts/config --set-val CONFIG_EVENT_TRACE_STARTUP_TEST n
RUN ./scripts/config --set-val CONFIG_FTRACE_SORT_STARTUP_TEST n
RUN ./scripts/config --set-val CONFIG_MMIOTRACE_TEST n

# Add these lines to Kbuild for reproducibility
RUN echo 'KBUILD_BUILD_USER=a' >> Kbuild
RUN echo 'KBUILD_BUILD_HOST=b' >> Kbuild
RUN echo 'KBUILD_BUILD_TIMESTAMP="Sat 22 Jun 2024 03:54:20 PM EDT"' >> Kbuild

# This is a Reproducible Builds standard
ENV SOURCE_DATE_EPOCH=1719086180
RUN make -j 4

RUN cp arch/x86/boot/bzImage /build

RUN mv /linux.tar.xz /build/files
RUN mv /linux.tar.sign /build/files

# Add software licenses
RUN mkdir -p /build/initramfs/lic
ADD /licenses/* /build/initramfs/lic

# Add minimal programs
RUN mkdir -p /build/initramfs/src/punchcard
ADD /programs/* /build/initramfs/src/punchcard

ENV NOLIBC_INC="-include /build/stage/src/linux/tools/include/nolibc/nolibc.h"
ENV NOLIBC_FLAGS="-static -fno-asynchronous-unwind-tables -fno-ident -s -Os -nostdlib"
WORKDIR /build
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/cat /build/initramfs/src/punchcard/cat.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/sh /build/initramfs/src/punchcard/sh.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/ls /build/initramfs/src/punchcard/ls.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/c4 /build/initramfs/src/punchcard/c4.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o initramfs/kilo /build/initramfs/src/punchcard/kilo.c

RUN mkdir -p initramfs/sbin
RUN mkdir -p initramfs/usr/include/nolibc
RUN cp /build/stage/src/linux/tools/include/nolibc/* initramfs/usr/include/nolibc

# TODO: Somebody online said you need:
# gcc, make, binutils, perl, bc, a shell, tar, cpio, gzip, util-linux, kmod,
# mkinitrd, squashfs-tools, and maybe flex, bison, and openssl.

# TODO: Fetch and build: https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2
# TODO: musl
# TODO: glibc
# TODO: gcc
# TODO: bash
# TODO: bc
# TODO: minimake
# TODO: elfutils
# TODO: flex
# TODO: bison
# TODO: cpio
# TODO: dwarves
# TODO: perl
# TODO: git
# TODO: gpg
# TODO: dosfstools
# TODO: bzip2
# TODO: xz-utils
# TODO: tar
# TODO: syslinux
# TODO: util-linux
# TODO: kmod? (Isn't that part of the kernel?)
# TODO: disassembler

# /init is hard-coded into the Linux kernel to run automatically when the
# initramfs is loaded. We are using the shell as our init process.
RUN cp initramfs/sh initramfs/init

RUN chmod +x initramfs/init

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