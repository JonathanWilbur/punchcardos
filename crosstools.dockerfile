FROM ubuntu:latest AS base

RUN apt update && apt install -y \
    git curl gpg build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev \
    texinfo libisl-dev openssl libssl-dev

ENV MPFR_VER="4.1.1"
ENV GMP_VER="6.3.0"
ENV MPC_VER="1.2.1"
ENV GCC_VER="12.3.0"
ENV MUSL_VER="1.2.5"
ENV BINUTILS_VER="2.41"
ENV LINUX_HEADERS_VER="4.19.307"
ENV FILE_VER="5.45"

ENV DOWNLOADS=/downloads

ENV ARCH=x86_64
ENV GCC_ARCH=x86-64
ENV HOST=x86_64-cross-linux-gnu
ENV TARGET=x86_64-linux
ENV CROSS_TOOLS=/cross-tools
ENV TOOLS=/tools

RUN mkdir /pgp
ADD /pgp/* /pgp

RUN gpg --import /pgp/gnu-keyring.gpg
RUN gpg --import /pgp/gregkh.gpg
RUN gpg --import /pgp/torvalds.gpg
RUN gpg --import /pgp/musl.gpg
RUN gpg --import /pgp/madler.gpg

ENV PATH="${TOOLS}/bin:${CROSS_TOOLS}/bin:${PATH}"

FROM base AS downloads

RUN mkdir -p ${DOWNLOADS}/binutils
RUN curl -O https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.xz
RUN curl -O https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.xz.sig
RUN gpg --verify binutils-${BINUTILS_VER}.tar.xz.sig binutils-${BINUTILS_VER}.tar.xz
RUN tar -C ${DOWNLOADS}/binutils --strip-components 1 -xvf binutils-${BINUTILS_VER}.tar.xz

RUN mkdir -p ${DOWNLOADS}/linux
RUN curl -O https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-${LINUX_HEADERS_VER}.tar.xz
RUN tar -C ${DOWNLOADS}/linux --strip-components 1 -xvf linux-${LINUX_HEADERS_VER}.tar.xz

RUN mkdir -p ${DOWNLOADS}/gcc
RUN curl -O https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz
RUN curl -O https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz.sig
RUN gpg --verify gcc-${GCC_VER}.tar.xz.sig gcc-${GCC_VER}.tar.xz
RUN tar -C ${DOWNLOADS}/gcc --strip-components 1 -xvf gcc-${GCC_VER}.tar.xz

RUN mkdir ${DOWNLOADS}/musl
RUN curl -O https://musl.libc.org/releases/musl-${MUSL_VER}.tar.gz
RUN curl -O https://musl.libc.org/releases/musl-${MUSL_VER}.tar.gz.asc
RUN gpg --verify musl-${MUSL_VER}.tar.gz.asc musl-${MUSL_VER}.tar.gz
RUN tar -C ${DOWNLOADS}/musl --strip-components 1 -xvzf musl-${MUSL_VER}.tar.gz

RUN mkdir ${DOWNLOADS}/file
RUN curl -O http://ftp.astron.com/pub/file/file-${FILE_VER}.tar.gz
# RUN curl -O http://ftp.astron.com/pub/file/file-${FILE_VER}.tar.gz.asc
# RUN gpg --verify file-${FILE_VER}.tar.gz.asc file-${FILE_VER}.tar.gz
RUN tar -C ${DOWNLOADS}/file --strip-components 1 -xvzf file-${FILE_VER}.tar.gz

RUN mkdir ${DOWNLOADS}/gcc/mpfr
RUN curl -O https://ftp.gnu.org/gnu/mpfr/mpfr-${MPFR_VER}.tar.xz
RUN tar -C ${DOWNLOADS}/gcc/mpfr --strip-components 1 -xvf mpfr-${MPFR_VER}.tar.xz

RUN mkdir ${DOWNLOADS}/gcc/gmp
RUN curl -O https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VER}.tar.xz
RUN tar -C ${DOWNLOADS}/gcc/gmp --strip-components 1 -xvf gmp-${GMP_VER}.tar.xz

RUN mkdir ${DOWNLOADS}/gcc/mpc
RUN curl -O https://ftp.gnu.org/gnu/mpc/mpc-${MPC_VER}.tar.gz
RUN tar -C ${DOWNLOADS}/gcc/mpc --strip-components 1 -xvzf mpc-${MPC_VER}.tar.gz

FROM base AS cross

COPY --from=downloads ${DOWNLOADS} ${DOWNLOADS}

WORKDIR ${DOWNLOADS}/linux
RUN make mrproper
RUN make ARCH=${ARCH} INSTALL_HDR_PATH=${CROSS_TOOLS}/${TARGET} headers_install

RUN mkdir -v ${DOWNLOADS}/binutils/build
WORKDIR ${DOWNLOADS}/binutils/build
RUN ${DOWNLOADS}/binutils/configure \
    --prefix=${CROSS_TOOLS} \
    --target=${TARGET} \
    --with-sysroot=${CROSS_TOOLS}/${TARGET} \
    --disable-nls \
    --disable-multilib \
    --disable-werror \
    --enable-deterministic-archives \
    --disable-compressed-debug-sections 
RUN make configure-host
RUN make
RUN make install

RUN mkdir -v ${DOWNLOADS}/gcc/build
WORKDIR ${DOWNLOADS}/gcc/build
RUN ${DOWNLOADS}/gcc/configure \
    --prefix=${CROSS_TOOLS} \
    --build=${HOST} \
    --host=${HOST} \
    --target=${TARGET} \
    --with-sysroot=${CROSS_TOOLS}/${TARGET} \
    --with-native-system-header-dir=/include \
    --disable-nls \
    --with-newlib \
    --disable-libitm \
    --disable-libvtv \
    --disable-libssp \
    --disable-shared \
    --disable-libgomp \
    --without-headers \
    --disable-threads \
    --disable-multilib \
    --disable-libatomic \
    --disable-libstdcxx \
    --enable-languages=c \
    --disable-libquadmath \
    --disable-libsanitizer \
    --with-arch=${GCC_ARCH} \
    --disable-decimal-float \
    --enable-clocale=generic
RUN make all-gcc all-target-libgcc
RUN make install-gcc install-target-libgcc

WORKDIR ${DOWNLOADS}/musl
RUN ./configure \
    CROSS_COMPILE=${TARGET}- \
    --prefix=/ \
    --target=${TARGET}
RUN make && DESTDIR=${CROSS_TOOLS} make install

RUN mkdir -v ${CROSS_TOOLS}/usr
RUN ln -sv ../include ${CROSS_TOOLS}/usr/include
RUN rm -vf ${CROSS_TOOLS}/lib/ld-musl-${ARCH}.so.1
RUN ln -sv libc.so ${CROSS_TOOLS}/lib/ld-musl-${ARCH}.so.1

RUN ln -sv ../lib/ld-musl-${ARCH}.so.1 ${CROSS_TOOLS}/bin/ldd

RUN mkdir -pv ${CROSS_TOOLS}/etc
RUN echo "${CROSS_TOOLS}/lib\n${TOOLS}/lib" > ${CROSS_TOOLS}/etc/ld-musl-${ARCH}.path

RUN rm -rvf ${DOWNLOADS}/gcc/build && mkdir -v ${DOWNLOADS}/gcc/build
WORKDIR ${DOWNLOADS}/gcc/build
RUN AR=ar LDFLAGS="-Wl,-rpath,${CROSS_TOOLS}/lib" \
    ../configure \
    --prefix=${CROSS_TOOLS} \
    --build=${HOST} \
    --host=${HOST} \
    --target=${TARGET} \
    --disable-multilib \
    --with-sysroot=${CROSS_TOOLS} \
    --disable-nls \
    --enable-shared \
    --enable-languages=c,c++ \
    --enable-threads=posix \
    --enable-clocale=generic \
    --enable-libstdcxx-time \
    --enable-fully-dynamic-string \
    --disable-symvers \
    --disable-libsanitizer \
    --disable-lto-plugin \
    --disable-libssp
RUN make \
    AS_FOR_TARGET="${TARGET}-as" \
    LD_FOR_TARGET="${TARGET}-ld" 
RUN make install