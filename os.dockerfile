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

RUN apt update && apt install -y git
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
ENV DEBIAN_FRONTEND=noninteractive
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
# Future versions may have to clone deeper for reproduceability.
RUN git clone --depth 1 https://github.com/urutech/nanocc.git
RUN git clone --depth 1 https://github.com/TinyCC/tinycc.git
RUN git clone --depth 1 https://github.com/rui314/chibicc.git
RUN git clone --depth 1 https://github.com/rswier/c4.git
RUN git clone --depth 1 https://github.com/Battelle/movfuscator.git
RUN git clone --depth 1 https://github.com/thepowersgang/mrustc.git
RUN git clone --depth 1 https://github.com/TomiG06/legolas/tree/main
RUN git clone --depth 1 https://github.com/pommicket/bootstrap
RUN git clone --depth 1 https://github.com/openjdk/jdk.git
RUN git clone --depth 1 https://github.com/openwrt/openwrt.git
RUN git clone --depth 1 https://github.com/openssl/openssl.git
RUN git clone --depth 1 https://github.com/git/git.git
RUN git clone --depth 1 https://github.com/strace/strace.git
RUN git clone --depth 1 https://github.com/westes/flex.git
RUN git clone --depth 1 https://github.com/golang/go.git
RUN git clone --depth 1 https://github.com/rust-lang/rust.git
RUN git clone --depth 1 https://github.com/rust-lang/cargo.git
RUN git clone --depth 1 https://git.openwrt.org/openwrt/openwrt.git
RUN git clone --depth 1 https://github.com/nanopb/nanopb.git
RUN git clone --depth 1 https://github.com/protocolbuffers/protobuf.git
RUN git clone --depth 1 https://gitlab.com/libvirt/libvirt.git
RUN git clone --depth 1 git://git.kernel.org/pub/scm/virt/kvm/kvm.git
RUN git clone --depth 1 https://github.com/neovim/neovim.git
RUN git clone --depth 1 https://github.com/tdlib/td.git
RUN git clone --depth 1 https://github.com/NationalSecurityAgency/ghidra.git
RUN git clone --depth 1 https://github.com/brave/brave-browser.git
RUN git clone --depth 1 https://github.com/curl/curl.git
RUN git clone --depth 1 https://github.com/telegramdesktop/tdesktop.git
RUN git clone --depth 1 https://github.com/oriansj/M2-Planet.git
RUN git clone --depth 1 https://github.com/oriansj/mescc-tools.git

# - [ ] ash
# - [ ] Noisemaker (my idea)
# - [ ] wget
# - [ ] websurfx CLI (my idea)
# - [ ] Lynx https://github.com/kurtchen/Lynx

# TODO: Extract and verify 
RUN curl https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz.sig
RUN curl https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz

RUN curl https://ftp.gnu.org/gnu/make/make-4.4.tar.gz
RUN curl https://ftp.gnu.org/gnu/make/make-4.4.tar.gz.sig

# Required to make OpenJDK
RUN curl https://ftp.gnu.org/gnu/automake/automake-1.16.tar.gz
RUN curl https://ftp.gnu.org/gnu/automake/automake-1.16.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/bc/bc-1.07.tar.gz
RUN curl https://ftp.gnu.org/gnu/bc/bc-1.07.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz
RUN curl https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/bison/bison-3.8.tar.gz
RUN curl https://ftp.gnu.org/gnu/bison/bison-3.8.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/coreutils/coreutils-9.4.tar.gz
RUN curl https://ftp.gnu.org/gnu/coreutils/coreutils-9.4.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/cpio/cpio-2.14.tar.gz
RUN curl https://ftp.gnu.org/gnu/cpio/cpio-2.14.tar.gz.sig

# NOTE: There was no gzipped version.
RUN curl https://ftp.gnu.org/gnu/diffutils/diffutils-3.10.tar.xz
RUN curl https://ftp.gnu.org/gnu/diffutils/diffutils-3.10.tar.xz.sig

RUN curl https://download.libsodium.org/libsodium/releases/libsodium-1.0.19.tar.gz
RUN curl https://download.libsodium.org/libsodium/releases/libsodium-1.0.19.tar.gz.sig

RUN curl https://download.qemu.org/qemu-8.2.0.tar.xz
RUN curl https://download.qemu.org/qemu-8.2.0.tar.xz.sig

RUN curl https://download.libguestfs.org/1.52-stable/libguestfs-1.52.0.tar.gz
RUN curl https://download.libguestfs.org/1.52-stable/libguestfs-1.52.0.tar.gz.sig

RUN curl https://downloads.xenproject.org/release/xen/4.18.0/xen-4.18.0.tar.gz
RUN curl https://downloads.xenproject.org/release/xen/4.18.0/xen-4.18.0.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/libtasn1/libtasn1-4.19.0.tar.gz
RUN curl https://ftp.gnu.org/gnu/libtasn1/libtasn1-4.19.0.tar.gz.sig

RUN curl https://ftp.gnu.org/gnu/gzip/gzip-1.13.tar.xz
RUN curl https://ftp.gnu.org/gnu/gzip/gzip-1.13.tar.xz.sig

RUN curl https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.xz
RUN curl https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.xz.sig

RUN curl https://ftp.gnu.org/gnu/gdb/gdb-14.1.tar.gz
RUN curl https://ftp.gnu.org/gnu/gdb/gdb-14.1.tar.gz.sig

# There seems to be no sig
RUN curl https://invisible-island.net/datafiles/release/ncurses.tar.gz

# TODO: Verify using these keys: https://www.python.org/downloads/
RUN curl https://www.python.org/ftp/python/3.12.1/Python-3.12.1.tgz

# This is the LTS release
RUN curl https://nodejs.org/dist/v20.10.0/node-v20.10.0.tar.gz


# From: https://gcc.gnu.org/
# "The GNU Compiler Collection includes front ends for C, C++, Objective-C,
# Fortran, Ada, Go, and D, as well as libraries for these languages..."
RUN git clone --depth 1 git clone git://gcc.gnu.org/git/gcc.git

# TODO: Extract this archive
# See: https://go.dev/doc/install/source#bootstrapFromSource
RUN curl https://dl.google.com/go/go1.4-bootstrap-20171003.tar.gz


# Steps here: https://www.wireguard.com/compilation/
RUN apt install -y libelf-dev linux-headers-$(uname -r) build-essential pkg-config
RUN git clone --depth 1 https://git.zx2c4.com/wireguard-linux-compat
RUN git clone --depth 1 https://git.zx2c4.com/wireguard-tools
RUN make -C wireguard-linux-compat/src -j$(nproc)
# RUN make -C wireguard-linux-compat/src install
RUN make -C wireguard-tools/src -j$(nproc)
# RUN make -C wireguard-tools/src install

# TODO: Clone other repos

# Make will copy all of the files in /tilck/sysroot to be the root of the
# disk image's filesystem.
RUN make
