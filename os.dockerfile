FROM ubuntu:latest
LABEL author="Jonathan M. Wilbur <jonathan@wilbur.space>"
LABEL version="0.0.1"
LABEL description="PunchcardOS: A minimal Unix-like distro for bootstrapping a trustworthy toolchain"

RUN mkdir /pgp
ADD /pgp/* /pgp
RUN apt update && apt install -y \
    git curl gpg build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev \
    texinfo libisl-dev openssl libssl-dev
# libcloog-isl-dev is not found.

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

# RUN apt update && apt upgrade -y && \
# apt install git-core gcc g++ gcc-multilib g++-multilib clang cmake \
# python3-minimal python3-serial python3-pip apt gawk unzip parted wget \
# curl qemu-system-x86 imagemagick -y
RUN git clone --depth 1 https://github.com/vvaltchev/tilck.git
# TODO: Check the commit GPG signature?

# NOTE: Tilck CANNOT be compiled with TinyCC.
# See: https://github.com/vvaltchev/tilck/discussions/93
# In addition, to GCC, it specifically requires the assember provided in
# GNU's binutils.

# This folder is only for source code used in pre-compiled binaries included in
# the image. This includes the cross-compiler, the kernel, and the shell.
RUN mkdir /tilck/sysroot/src
WORKDIR /tilck/sysroot/src

# I did not download GnuEFI from the official source because there doesn't seem
# to be any kind of scriptable download URL. Also, this version is distributed
# by the fellow that made Tilck anyway. The author also says that downloading
# directly from Sourceforge is too slow for CI builds.
# RUN curl -O https://sourceforge.net/projects/gnu-efi/files/gnu-efi-3.0.17.tar.bz2/download
RUN git clone --depth 1 https://github.com/vvaltchev/gnu-efi-fork.git gnuefi

# Following the instructions here to make a cross-compiler: https://wiki.osdev.org/GCC_Cross_Compiler

# Binutils is required to get the assembler for GCC.
RUN mkdir /tilck/sysroot/src/binutils
RUN curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
RUN curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz.sig
RUN gpg --verify binutils-2.41.tar.xz.sig binutils-2.41.tar.xz
RUN tar -C /tilck/sysroot/src/binutils --strip-components 1 -xvf binutils-2.41.tar.xz

# Note, GCC is only used to compile the kernel and the TinyCC. TinyCC is used in
# image for compiling programs.
RUN mkdir /tilck/sysroot/src/gcc
RUN curl -O https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz
RUN curl -O https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz.sig
RUN gpg --verify gcc-13.2.0.tar.xz.sig gcc-13.2.0.tar.xz
RUN tar -C /tilck/sysroot/src/gcc --strip-components 1 -xvf gcc-13.2.0.tar.xz

RUN mkdir /tilck/sysroot/src/musl
RUN curl -O https://musl.libc.org/releases/musl-1.2.4.tar.gz
RUN curl -O https://musl.libc.org/releases/musl-1.2.4.tar.gz.asc
RUN gpg --verify musl-1.2.4.tar.gz.asc musl-1.2.4.tar.gz
RUN tar -C /tilck/sysroot/src/musl --strip-components 1 -xvzf musl-1.2.4.tar.gz

RUN mkdir /tilck/sysroot/src/cmake
# You have to use the -L flag to follow redirects.
# RUN curl -OL https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1.tar.gz
# # TODO: Verify the hash is 15e94f83e647f7d620a140a7a5da76349fc47a1bfed66d0f5cdee8e7344079ad
# # RUN export CMAKE_DIGEST=$(openssl dgst -sha256 cmake-3.28.1.tar.gz)
# # RUN '[ ]'
# RUN tar -C /tilck/sysroot/src/cmake --strip-components 1 -xvzf cmake-3.28.1.tar.gz

RUN curl -OL https://github.com/Kitware/CMake/releases/download/v3.2.3/cmake-3.2.3.tar.gz
# TODO: Verify the hash is 15e94f83e647f7d620a140a7a5da76349fc47a1bfed66d0f5cdee8e7344079ad
# RUN export CMAKE_DIGEST=$(openssl dgst -sha256 cmake-3.28.1.tar.gz)
# RUN '[ ]'
RUN tar -C /tilck/sysroot/src/cmake --strip-components 1 -xvzf cmake-3.2.3.tar.gz

RUN mkdir /tilck/sysroot/src/mtools
RUN curl -O https://ftp.gnu.org/gnu/mtools/mtools-4.0.43.tar.bz2
RUN curl -O https://ftp.gnu.org/gnu/mtools/mtools-4.0.43.tar.bz2.sig
RUN gpg --verify mtools-4.0.43.tar.bz2.sig mtools-4.0.43.tar.bz2
RUN tar -C /tilck/sysroot/src/mtools --strip-components 1 -xvf mtools-4.0.43.tar.bz2

RUN mkdir /tilck/sysroot/src/zlib
RUN curl -O https://www.zlib.net/fossils/zlib-1.3.tar.gz
RUN curl -O https://www.zlib.net/zlib-1.3.tar.gz.asc
RUN gpg --verify zlib-1.3.tar.gz.asc zlib-1.3.tar.gz
RUN tar -C /tilck/sysroot/src/zlib --strip-components 1 -xvzf zlib-1.3.tar.gz

WORKDIR /tilck/sysroot/src/binutils
RUN ./configure --target=i686-elf --prefix=/tilck/sysroot --with-sysroot --disable-nls --disable-werror
RUN make
RUN make install

# For some stupid reason, GCC requires you to build it in a separate directory.
RUN mkdir /tilck/sysroot/src/gcc-build
WORKDIR /tilck/sysroot/src/gcc-build
RUN ../gcc/configure --target=i686-elf --prefix=/tilck/sysroot --disable-nls --enable-languages=c --without-headers
RUN make all-gcc
RUN make all-target-libgcc
RUN make install-gcc
RUN make install-target-libgcc

WORKDIR /tilck/sysroot/src/cmake
RUN ./bootstrap
RUN make
RUN make install

WORKDIR /tilck/sysroot/src/gnuefi
RUN make
RUN make install

WORKDIR /tilck/sysroot/src/mtools
RUN ./configure
RUN make
RUN make install

WORKDIR /tilck/sysroot/src/zlib
RUN ./configure --prefix=/tilck/toolchain2/i386/zlib/install --static
RUN make
RUN make install

# TCROOT=/tilck/toolchain2
# TC="$TCROOT"
# ARCH="i386"
# MUSL_INSTALL=$TC/$ARCH/musl/install, which is therefore /tilck/toolchain2/i386/musl/install
WORKDIR /tilck/sysroot/src/musl
ENV CC=/tilck/sysroot/bin/i686-elf-gcc
ENV CFLAGS="-B /tilck/sysroot/bin"
# RUN cp /tilck/sysroot/bin/i686-elf-ar /tilck/sysroot/bin/ar
ENV PATH="/tilck/sysroot/bin:$PATH"
# IDK why build should be x86_64
# I added --enable-wrapper=gcc, and I think I had to because I think the GCC
# compiler fails to build the temporary C file created in the configure script.
RUN ./configure \
    --target=i686-elf \
    --build=x86_64 \
    --disable-shared \
    --prefix=/tilck/toolchain2/i386/musl/install \
    --exec-prefix=/tilck/toolchain2/i386/musl/install \
    --enable-debug \
    --syslibdir=/tilck/toolchain2/i386/musl/install/lib \
    --enable-wrapper=gcc
RUN make
RUN make install

WORKDIR /tilck/toolchain2/i386/musl/install/bin
RUN cp musl-gcc musl-g++
RUN sed -i 's/-i386-gcc/-i386-g++/' /tilck/toolchain2/i386/musl/install/bin/musl-g++

WORKDIR /tilck/toolchain2/i386/musl/install/include
RUN ln -s /usr/include/linux .
RUN ln -s /usr/include/asm-generic .
RUN ln -s /usr/include/asm .

# GDB _might_ be added later, but I don't think it is required for this project.

# Note the period at the end. This avoids cloning the project name.
# RUN git clone --depth 1 https://github.com/JonathanWilbur/punchcardos.git .
# RUN git clone --depth 1 https://github.com/TinyCC/tinycc.git
# RUN git clone --depth 1 https://github.com/oriansj/M2-Planet.git
# RUN git clone --depth 1 https://github.com/oriansj/mescc-tools.git

# TODO: glibc-linked tcc for x86_64 cross-compiler, built with glibc-gcc
# TODO: musl-linked tcc, built with musl-gcc
# TODO: musl-linked shell, built with musl-gcc
# WORKDIR /tilck/sysroot/tinycc
# RUN ./configure --config-musl --extra-ldflags='-static -static-libgcc'
# RUN make
# RUN cp ./tcc /tilck/sysroot/bin/tcc

# RUN curl -O https://ftp.gnu.org/gnu/mes/mes-0.26.tar.gz
# RUN tar -xvzf mes-0.26.tar.gz
# TODO: Verify signature

# Make will copy all of the files in /tilck/sysroot to be the root of the
# disk image's filesystem.
WORKDIR /tilck
# ENV CFLAGS=""
ENV CFLAGS="-B /tilck/sysroot/bin"
# ENV CC=/tilck/sysroot/bin/i686-elf-gcc
ENV CC=""
ENV BUILD_ARCH="x86_64"
ENV ARCH="i386"
ENV HOST_ARCH="x86_64"
ENV TC="/tilck/toolchain2"
ENV TCROOT="$TC"
ENV GCC_TC_VER="13.2.0"
# ENV CMAKE=$TC/host_$HOST_ARCH/cmake/bin/cmake
ENV GCC_NAME_PFX="gcc_${GCC_TC_VER_}_${ARCH}"
RUN echo -n $GCC_TC_VER > echo -n $GCC_TC_VER > $TC/.gcc_tc_ver_$ARCH
RUN mkdir -p $TC/host_x86_64
WORKDIR /tilck/toolchain2
RUN ln -s host_${HOST_ARCH} host
WORKDIR /tilck/toolchain2/host
# RUN mkdir -p gcc_13_2_0_${ARCH}_musl/bin
# RUN mkdir -p gcc_13_2_0_${ARCH}_musl/libexec
RUN cp /tilck/sysroot/bin/i686-elf-gcc /tilck/toolchain2/host/gcc_13_2_0_i386_musl/bin/i686-linux-gcc
# RUN cp -r /tilck/sysroot/libexec/gcc/i686-elf/13.2.0 /tilck/toolchain2/host/gcc_13_2_0_i386_musl/libexec
# RUN ln -s host_${HOST_ARCH} host
RUN cp -r /tilck/sysroot /tilck/toolchain2/host
WORKDIR /tilck
# TODO: Put the GCC in the right location. /tilck/toolchain2/host/gcc_13_2_0_i386_musl/bin/i686-linux-gcc
RUN make
