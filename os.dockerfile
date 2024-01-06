FROM ubuntu:latest

# Instructions for building the Linux Kernel

# bzip2 is needed to compress the kernel image
# git is needed to clone the Git repository
# vim is a text editor (maybe not needed?)
# gcc is the GNU compiler. The Linux Kernel specifically calls for this, but I believe others will work.
# libncurses-dev is just for the kernel configuration via `make menuconfig`
# flex is for generating lexical analyzers. I believe it is used by the Kernel, but IDK what for.
# bison is like flex, and again, IDK what it is used for.
# bc is a "basic calculator." It is required for the kernel build.
# cpio is for producing the archive used by the kernel for the initramfs, I believe.
# libelf-dev is for producing an ELF executable
# libssl-dev is used by the Kernel. Some Kernel functionality use OpenSSL, such as for signed kernel modules.
# syslinux is the bootloader
# dosfstools is for creating a FAT32 filesystem, which the UEFI boot requires.
# RUN apt update && apt install bzip2 git vim make gcc libncurses-dev flex bison bc cpio libelf-dev libssl-dev syslinux dosfstools

# "--depth 1" just means "clone only the most recent commit," since we don't need the whole history.
# RUN git clone --depth 1 https://github.com/torvalds/linux.git
# # TODO: Check the commit GPG signature?
# RUN cd linux
# RUN ./scripts/config --set-val CONFIG_OPTION y

# Building the Tilck Kernel instead

RUN apt update && apt install -y git curl
# RUN apt update && apt upgrade -y && \
# apt install git-core gcc g++ gcc-multilib g++-multilib clang cmake \
# python3-minimal python3-serial python3-pip apt gawk unzip parted wget \
# curl qemu-system-x86 imagemagick -y
RUN git clone --depth 1 https://github.com/vvaltchev/tilck.git
# TODO: Check the commit GPG signature?
# RUN cd tilck
WORKDIR /tilck
# See: https://github.com/vvaltchev/tilck/issues/177
# We don't want any of the interactive features of apt, so we just disable them.
# ENV DEBIAN_FRONTEND=noninteractive
ENV RUNNING_IN_CI=1
# ENV CI=1

# These options don't seem to do anything. I think the build script recurses
# into other scripts quickly, and this option does not recurse. That's my guess.
RUN set -o xtrace
RUN set -x
# This option, on the other hand, _does_ work, except the build breaks because,
# at some point, the output of a command is being piped into a file, and the
# trace output is making its way into that file.
# ENV SHELLOPTS=xtrace
# TODO: Explode this script
RUN ./scripts/build_toolchain

# This is the folder that becomes the root of the image.
WORKDIR /tilck/sysroot

# Note the period at the end. This avoids cloning the project name.
RUN git clone --depth 1 https://github.com/JonathanWilbur/punchcardos.git .
RUN git clone --depth 1 https://github.com/TinyCC/tinycc.git
RUN git clone --depth 1 https://github.com/oriansj/M2-Planet.git
RUN git clone --depth 1 https://github.com/oriansj/mescc-tools.git

# TODO: musl-linked tcc
# TODO: musl-linked shell
WORKDIR /tilck/sysroot/tinycc
RUN ./configure --config-musl --extra-ldflags='-static -static-libgcc'
RUN make
RUN cp ./tcc /tilck/sysroot/bin/tcc

# TODO: Copy outputs into Tilck

# Needed for the musl-gcc tool.
RUN git clone https://git.musl-libc.org/git/musl.git
# WORKDIR 

RUN curl -O https://ftp.gnu.org/gnu/mes/mes-0.26.tar.gz
RUN tar -xvzf mes-0.26.tar.gz
# TODO: Verify signature

RUN curl -O https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86/make-4.4.1-r2.apk
# TODO: Extract and install the binaries in the sysroot for Tilck.

# Make will copy all of the files in /tilck/sysroot to be the root of the
# disk image's filesystem.
RUN make
