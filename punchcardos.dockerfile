FROM ubuntu:latest
LABEL author="Jonathan M. Wilbur <jonathan@wilbur.space>"
LABEL version="0.0.1"
LABEL description="PunchcardOS: A minimal Unix-like distro for bootstrapping a trustworthy toolchain"

# This doesn't seem to be mentioned, but the Kernel requires Perl to build.
# Perl is included by default in Ubuntu.

# TODO: I think I can get rid of bc by writing my own.
# The only place where the Kernel uses it for building is here: kernel/time/timeconst.bc.
# (It is used a lot for tests, I think.)
ENV KERNEL_REQS="make gcc flex bison libelf-dev bc binutils"
ENV KERNEL_OPT="libncurses-dev"
ENV KERNEL_UNK="flex bison cpio clang dwarves jfsutils reiserfsprogs xfsprogs btrfs-progs libssl-dev"
# Currently, all of the below are used, except maybe vim. IDK why I included it.
ENV OTHER_DEPS="git vim gpg curl dosfstools bzip2 xz-utils perl tar"

# Linux Kernel requirements: https://docs.kernel.org/process/changes.html
# dwarves installed pahole
RUN apt update && apt install -y ${KERNEL_REQS} ${OTHER_DEPS}
# I am not sure what to do about udev; it seems like installing that could wreck my computer.
# mcelog ignored. I don't see how to install this.

RUN mkdir -p /build/initramfs/src
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

# This puts the Linux kernel source in the OS's own initramfs
RUN mkdir /build/initramfs/src/linux
RUN curl https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.9.6.tar.xz -o linux.tar.xz
RUN curl https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.9.6.tar.sign -o linux.tar.sign

# The TAR archive is signed, not the compressed XZ file.
RUN xz -cd linux.tar.xz | gpg --verify linux.tar.sign -
RUN tar -C /build/initramfs/src/linux --strip-components 1 -xvf linux.tar.xz

WORKDIR /build/initramfs/src/linux

# RUN make mrproper
ADD mods mods
ENV LSMOD=/build/initramfs/src/linux/mods
RUN make allnoconfig
RUN make localyesconfig

# TODO: Refine this further
RUN ./scripts/config --set-val CONFIG_NET n
RUN ./scripts/config --set-val CONFIG_EXT3_FS y
RUN ./scripts/config --set-val CONFIG_EXT3_FS_POSIX_ACL y
RUN ./scripts/config --set-val CONFIG_EXT3_FS_SECURITY y
RUN ./scripts/config --set-val CONFIG_ELF_CORE y
RUN ./scripts/config --set-val CONFIG_BINFMT_ELF y
RUN ./scripts/config --set-val CONFIG_AUTOFS_FS y
RUN ./scripts/config --set-val CONFIG_IKCONFIG y
RUN ./scripts/config --set-val CONFIG_IKCONFIG_PROC y
RUN ./scripts/config --set-val CONFIG_DM_CRYPT y

# Add these lines to Kbuild for reproducibility
RUN echo 'KBUILD_BUILD_USER=a' >> Kbuild
RUN echo 'KBUILD_BUILD_HOST=b' >> Kbuild
RUN echo 'KBUILD_BUILD_TIMESTAMP="Sat 22 Jun 2024 03:54:20 PM EDT"' >> Kbuild

# This is a Reproducible Builds standard
ENV SOURCE_DATE_EPOCH=1719086180
RUN make -j 6

RUN cp arch/x86/boot/bzImage /build

# Add minimal programs
RUN mkdir -p /build/initramfs/src/punchcard
ADD /programs/* /build/initramfs/src/punchcard

ENV NOLIBC_INC="-include /build/initramfs/src/linux/tools/include/nolibc/nolibc.h"
ENV NOLIBC_FLAGS="-static -fno-asynchronous-unwind-tables -fno-ident -s -Os -nostdlib"
# TODO: Do I need to build glibc?
WORKDIR /build
# RUN gcc -o c4 /build/initramfs/src/punchcard/c4.c
# RUN gcc -o sh /build/initramfs/src/punchcard/sh.c
# RUN gcc -o cat /build/initramfs/src/punchcard/cat.c
# RUN gcc -o kilo /build/initramfs/src/punchcard/kilo.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o cat /build/initramfs/src/punchcard/cat.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o sh /build/initramfs/src/punchcard/sh.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o ls /build/initramfs/src/punchcard/ls.c
RUN gcc ${NOLIBC_FLAGS} ${NOLIBC_INC} -o c4 /build/initramfs/src/punchcard/c4.c

# This successfully produces a .o file.
# gcc ./programs/arch.c -lgcc

RUN cp c4 sh cat cat2 kilo initramfs

RUN find ./initramfs | cpio -o -H newc > ./init.cpio
RUN dd if=/dev/zero of=boot bs=1M count=50
RUN mkfs -t fat boot
RUN syslinux boot
RUN mkdir m
RUN mount boot m
RUN cp bzImage init.cpio m
RUN umount m

RUN echo 'Build complete. Output file named boot.'