#!/bin/sh

set -e

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
gpg --import /pgp/gnu-keyring.gpg
gpg --import /pgp/musl.gpg
gpg --import /pgp/madler.gpg            # Zlib
gpg --import /pgp/bzip2.gpg
gpg --import /pgp/elfutils.gpg
gpg --import /pgp/lcollin.gpg           # XZ Utils
gpg --import /pgp/oriansj.gpg
gpg --import /pgp/gnupg.gpg
gpg --import /pgp/junio-hamano.gpg      # Git SCM
gpg --import /pgp/astikonas.asc         # Some bootstrapping things
gpg --import /pgp/wayne-davison.gpg     # Rsync
gpg --import /pgp/westes.asc            # Flex

# Used for QEMU.
# Obtained from: https://keys.openpgp.org/vks/v1/by-fingerprint/CEACC9E15534EBABB82D3FA03353C9CEF108B584
# Found by: https://www.qemu.org/download/
gpg --import /pgp/mroth.gpg

# Source for some of these: https://man.sr.ht/~oriansj/bootstrappable/live-bootstrap.md#stage0-posix

# Somebody online said you need:
# gcc, make, binutils, perl, bc, a shell, tar, cpio, gzip, util-linux, kmod,
# mkinitrd, squashfs-tools, and maybe flex, bison, and openssl.

# curl -L https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2 -o tcc.tar.bz2
curl -vL https://repo.or.cz/tinycc.git/snapshot/08a4c52de39b202f02d1ec525c64336d11ad9ccf.tar.gz -o tcc.tar.gz
# There is no signature to go along with this version.
# TODO: Verify hash never changes.

# I cannot find the damn keys for this guy.
curl -vL https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz -o flex.tar.gz
curl -vL https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz.sig -o flex.tar.gz.sig
gpg --verify flex.tar.gz.sig flex.tar.gz

# Signed by lcollin.gpg
curl -vL https://github.com/tukaani-project/xz/releases/download/v5.6.2/xz-5.6.2.tar.gz -o xz.tar.gz
curl -vL https://github.com/tukaani-project/xz/releases/download/v5.6.2/xz-5.6.2.tar.gz.sig -o xz.tar.gz.sig
gpg --verify xz.tar.gz.sig xz.tar.gz

# Signed by musl.gpg
curl -vL https://musl.libc.org/releases/musl-1.2.5.tar.gz -o musl.tar.gz
curl -vL https://musl.libc.org/releases/musl-1.2.5.tar.gz.asc -o musl.tar.gz.asc
gpg --verify musl.tar.gz.asc musl.tar.gz

# NOTE: Version 14 might not work. It introduced a lot of new stricter validation.
# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.xz -o gcc.tar.xz
curl -vL https://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.xz.sig -o gcc.tar.xz.sig
gpg --verify gcc.tar.xz.sig gcc.tar.xz

# Signed by madler.gpg
curl -vL https://www.zlib.net/zlib-1.3.1.tar.gz -o zlib.tar.gz
curl -vL https://www.zlib.net/zlib-1.3.1.tar.gz.asc -o zlib.tar.gz.asc
gpg --verify zlib.tar.gz.asc zlib.tar.gz

# NOTE: I believe this is a suitable stand-in for GNU's binutils.
# Signed by elfutils.gpg
curl -vL https://sourceware.org/elfutils/ftp/elfutils-latest.tar.bz2 -o elfutils.tar.bz2
curl -vL https://sourceware.org/elfutils/ftp/elfutils-latest.tar.bz2.sig -o elfutils.tar.bz2.sig
gpg --verify elfutils.tar.bz2.sig elfutils.tar.bz2

# See above.
# curl https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz
# curl https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz.sig

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz -o bash.tar.gz
curl -vL https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz.sig -o bash.tar.gz.sig
gpg --verify bash.tar.gz.sig bash.tar.gz

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/make/make-4.4.tar.gz -o make.tar.gz
curl -vL https://ftp.gnu.org/gnu/make/make-4.4.tar.gz.sig -o make.tar.gz.sig
gpg --verify make.tar.gz.sig make.tar.gz

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/automake/automake-1.16.tar.gz -o automake.tar.gz
curl -vL https://ftp.gnu.org/gnu/automake/automake-1.16.tar.gz.sig -o automake.tar.gz.sig
gpg --verify automake.tar.gz.sig automake.tar.gz

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/bc/bc-1.07.tar.gz -o bc.tar.gz
curl -vL https://ftp.gnu.org/gnu/bc/bc-1.07.tar.gz.sig -o bc.tar.gz.sig
gpg --verify bc.tar.gz.sig bc.tar.gz

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/bison/bison-3.8.tar.gz -o bison.tar.gz
curl -vL https://ftp.gnu.org/gnu/bison/bison-3.8.tar.gz.sig -o bison.tar.gz.sig
gpg --verify bison.tar.gz.sig bison.tar.gz

# curl -vL https://ftp.gnu.org/gnu/coreutils/coreutils-9.4.tar.gz
# curl -vL https://ftp.gnu.org/gnu/coreutils/coreutils-9.4.tar.gz.sig

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/cpio/cpio-2.14.tar.gz -o cpio.tar.gz
curl -vL https://ftp.gnu.org/gnu/cpio/cpio-2.14.tar.gz.sig -o cpio.tar.gz.sig
gpg --verify cpio.tar.gz.sig cpio.tar.gz

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/glibc/glibc-2.39.tar.gz -o glibc.tar.gz
curl -vL https://ftp.gnu.org/gnu/glibc/glibc-2.39.tar.gz.sig -o glibc.tar.gz.sig
gpg --verify glibc.tar.gz.sig glibc.tar.gz

# FYI: Autotools IS automake + autoconf + libtool.

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.xz -o automake.tar.xz
curl -vL https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.xz.sig -o automake.tar.xz.sig
gpg --verify automake.tar.xz.sig automake.tar.xz

# Signed by gnu-keyring.gpg
curl -vL https://ftp.gnu.org/gnu/autoconf/autoconf-2.72.tar.xz -o autoconf.tar.xz
curl -vL https://ftp.gnu.org/gnu/autoconf/autoconf-2.72.tar.xz.sig -o autoconf.tar.xz.sig
gpg --verify autoconf.tar.xz.sig autoconf.tar.xz

curl -vL https://ftp.gnu.org/gnu/libtool/libtool-2.4.7.tar.xz -o libtool.tar.xz
curl -vL https://ftp.gnu.org/gnu/libtool/libtool-2.4.7.tar.xz.sig -o libtool.tar.xz.sig
gpg --verify libtool.tar.xz.sig libtool.tar.xz

# TODO: It looks like I am going to have to consense this into a smaller
# sequence of steps.

# GnuPG 2.6 depends on the following GnuPG related packages:
# npth         (https://gnupg.org/ftp/gcrypt/npth/)
# libgpg-error (https://gnupg.org/ftp/gcrypt/libgpg-error/)
# libgcrypt    (https://gnupg.org/ftp/gcrypt/libgcrypt/)
# libksba      (https://gnupg.org/ftp/gcrypt/libksba/)
# libassuan    (https://gnupg.org/ftp/gcrypt/libassuan/)
curl -vL https://www.gnupg.org/ftp/gcrypt/gnupg/gnupg-2.4.5.tar.bz2 -o gnupg.tar.bz2
curl -vL https://www.gnupg.org/ftp/gcrypt/gnupg/gnupg-2.4.5.tar.bz2.sig -o gnupg.tar.bz2.sig
gpg --verify gnupg.tar.bz2.sig gnupg.tar.bz2

curl -vL https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.11.0.tar.bz2 -o libgcrypt.tar.bz2
curl -vL https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.11.0.tar.bz2.sig -o libgcrypt.tar.bz2.sig
gpg --verify libgcrypt.tar.bz2.sig libgcrypt.tar.bz2

curl -vL https://www.gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.50.tar.bz2 -o libgpg-error.tar.bz2
curl -vL https://www.gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.50.tar.bz2.sig -o libgpg-error.tar.bz2.sig
gpg --verify libgpg-error.tar.bz2.sig libgpg-error.tar.bz2

curl -vL https://www.gnupg.org/ftp/gcrypt/npth/npth-1.7.tar.bz2 -o npth.tar.bz2
curl -vL https://www.gnupg.org/ftp/gcrypt/npth/npth-1.7.tar.bz2.sig -o npth.tar.bz2.sig
gpg --verify npth.tar.bz2.sig npth.tar.bz2

curl -vL https://www.gnupg.org/ftp/gcrypt/libksba/libksba-1.6.7.tar.bz2 -o libksba.tar.bz2
curl -vL https://www.gnupg.org/ftp/gcrypt/libksba/libksba-1.6.7.tar.bz2.sig -o libksba.tar.bz2.sig
gpg --verify libksba.tar.bz2.sig libksba.tar.bz2

curl -vL https://www.gnupg.org/ftp/gcrypt/libassuan/libassuan-3.0.1.tar.bz2 -o libassuan.tar.bz2
curl -vL https://www.gnupg.org/ftp/gcrypt/libassuan/libassuan-3.0.1.tar.bz2.sig -o libassuan.tar.bz2.sig
gpg --verify libassuan.tar.bz2.sig libassuan.tar.bz2

# NOTE: This is signed the way the kernel is above.
# It was really difficult to even find this info, but these are signed by
# Junio C. Hamano <gitster@pobox.com> (junio-hamano.gpg).
curl -vL https://mirrors.edge.kernel.org/pub/software/scm/git/git-2.46.0.tar.xz -o git.tar.xz
curl -vL https://mirrors.edge.kernel.org/pub/software/scm/git/git-2.46.0.tar.sign -o git.tar.sign
xz -cd git.tar.xz | gpg --verify git.tar.sign -

# Signed by bzip2.gpg
curl -vL https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz -o bzip2.tar.gz
curl -vL https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz.sig -o bzip2.tar.gz.sig
gpg --verify bzip2.tar.gz.sig bzip2.tar.gz

# This is the dwarves package, or at least what we need out of it.
curl -vL https://git.kernel.org/pub/scm/devel/pahole/pahole.git/snapshot/pahole-1.27.tar.gz -o pahole.tar.gz
# TODO: Verify hash

# It appears that these are not GPG signed.
curl -vL https://www.cpan.org/src/5.0/perl-5.40.0.tar.gz -o perl.tar.gz
# TODO: Verify hash

# PEG/LEG: Needed to build minias
# curl -vL https://www.piumarta.com/software/peg/peg-0.1.19.tar.gz -o peg.tar.gz
# TODO: Verify hash

# TODO: I don't know where the key is
curl -vL https://github.com/dosfstools/dosfstools/releases/download/v4.2/dosfstools-4.2.tar.gz -o dosfstools.tar.gz
curl -vL https://github.com/dosfstools/dosfstools/releases/download/v4.2/dosfstools-4.2.tar.gz.sig -o dosfstools.tar.gz.sig
# gpg --verify dosfstools.tar.gz.sig dosfstools.tar.gz

# TODO: I don't know where these keys are.
curl -vL https://mirrors.edge.kernel.org/pub/linux/utils/boot/syslinux/syslinux-6.03.zip -o syslinux.zip
curl -vL https://mirrors.edge.kernel.org/pub/linux/utils/boot/syslinux/syslinux-6.03.zip.sign -o syslinux.zip.sign
# gpg --verify syslinux.zip.sign syslinux.zip

# TODO: This may be replaceable with a single-file program.
curl -vL https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz -o m4.tar.xz
curl -vL https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz.sig -o m4.tar.xz.sig
gpg --verify m4.tar.xz.sig m4.tar.xz

curl -vL https://www.mpfr.org/mpfr-current/mpfr-4.2.1.tar.xz -o mpfr.tar.xz
curl -vL https://www.mpfr.org/mpfr-current/mpfr-4.2.1.tar.xz.asc -o mpfr.tar.xz.asc
gpg --verify mpfr.tar.xz.asc mpfr.tar.xz

# Needed to build MPC.
curl -vL https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz -o gmp.tar.xz
curl -vL https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz.sig -o gmp.tar.xz.sig
gpg --verify gmp.tar.xz.sig gmp.tar.xz

# Needs MPFR and GMP.
curl -vL https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz -o mpc.tar.gz
curl -vL https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz.sig -o mpc.tar.gz.sig
gpg --verify mpc.tar.gz.sig mpc.tar.gz

curl -vL https://ftp.gnu.org/gnu/mes/mes-0.27.tar.gz -o mes.tar.gz
curl -vL https://ftp.gnu.org/gnu/mes/mes-0.27.tar.gz.sig -o mes.tar.gz.sig
gpg --verify mes.tar.gz.sig mes.tar.gz

curl -vL https://github.com/oriansj/bootstrap-seeds/archive/refs/tags/Release_1.3.0.tar.gz-o -o bootstrap-seeds
# TODO: Verify hash

# I think Jeremiah Orians uses the GNU Keychain.
curl -vL https://github.com/oriansj/stage0-posix/releases/download/Release_1.6.0/stage0-posix-1.6.0.tar.gz -o stage0-posix.tar.gz
curl -vL https://github.com/oriansj/stage0-posix/releases/download/Release_1.6.0/stage0-posix-1.6.0.tar.gz.asc -o stage0-posix.tar.gz.asc
gpg --verify stage0-posix.tar.gz.asc stage0-posix.tar.gz

curl -vL https://github.com/oriansj/M2-Planet/releases/download/Release_1.11.0/m2-planet-1.11.0.tar.gz -o m2-planet.tar.gz
curl -vL https://github.com/oriansj/M2-Planet/releases/download/Release_1.11.0/m2-planet-1.11.0.tar.gz.asc -o m2-planet.tar.gz.asc
gpg --verify m2-planet.tar.gz.asc m2-planet.tar.gz

curl -vL https://github.com/oriansj/mescc-tools/releases/download/Release_1.5.2/mescc-tools-1.5.2.tar.gz -o mescc-tools.tar.gz
curl -vL https://github.com/oriansj/mescc-tools/releases/download/Release_1.5.2/mescc-tools-1.5.2.tar.gz.asc -o mescc-tools.tar.gz.asc
gpg --verify mescc-tools.tar.gz.asc mescc-tools.tar.gz

curl -vL https://github.com/oriansj/mescc-tools-extra/releases/download/Release_1.3.0/mescc-tools-extra-1.3.0.tar.gz -o mescc-tools-extra.tar.gz
curl -vL https://github.com/oriansj/mescc-tools-extra/releases/download/Release_1.3.0/mescc-tools-extra-1.3.0.tar.gz.asc -o mescc-tools-extra.tar.gz.asc
gpg --verify mescc-tools-extra.tar.gz.asc mescc-tools-extra.tar.gz

# I have no idea whose key signs this.
# Tool for loading and managing kernel modules. I am not sure what this is really needed for at build time.
curl -vL https://mirrors.edge.kernel.org/pub/linux/utils/kernel/kmod/kmod-32.tar.xz -o kmod.tar.xz
curl -vL https://mirrors.edge.kernel.org/pub/linux/utils/kernel/kmod/kmod-32.tar.sign -o kmod.tar.sign
# TODO: xz -cd kmod.tar.xz | gpg --verify kmod.tar.sign -

# This isn't needed to build the Linux Kernel, but it _is_ needed to install the headers
# TODO: It might be possible to write a tool that does what this does. It is only used in one place.
# These packages are signed by the wayne-davison.gpg key.
curl -vL https://download.samba.org/pub/rsync/src/rsync-3.3.0.tar.gz -o rsync.tar.gz
curl -vL https://download.samba.org/pub/rsync/src/rsync-3.3.0.tar.gz.asc -o rsync.tar.gz.asc
gpg --verify rsync.tar.gz.asc rsync.tar.gz

git clone https://github.com/oriansj/stage0.git
cd /build/initramfs/src/stage0
git checkout a5ba3acfde3111bda674212361e6e3e5f9379c4c
# TODO: Verify hash
cd /build/initramfs/src

git clone https://github.com/oriansj/stage0-posix-x86.git
cd /build/initramfs/src/stage0-posix-x86
git checkout 105aebe1e5c38dcd5fd87723f7e49966ad6c436a
# TODO: Verify hash
cd /build/initramfs/src

git clone https://github.com/andrewchambers/minias.git
cd /build/initramfs/src/minias
git checkout cd21b90feaa9b66296b8614023349fae7d314b4b
# TODO: Verify hash
cd /build/initramfs/src

git clone https://github.com/oriansj/M2libc
cd /build/initramfs/src/M2libc
git checkout 3a700010872697c4be9e3fab3cf707fce706741e
# TODO: Verify hash
cd /build/initramfs/src

git clone https://github.com/JonathanWilbur/straplibc.git
cd /build/initramfs/src/straplibc
# TODO: Check out specific hash and verify it
cd /build/initramfs/src

git clone https://github.com/JonathanWilbur/libsyscall.git
cd /build/initramfs/src/libsyscall
# TODO: Check out specific hash and verify it
cd /build/initramfs/src

# TODO: Documented here: https://go.dev/doc/install/source
# TODO: https://dl.google.com/go/go1.4-bootstrap-20171003.tar.gz
# TODO: https://go.dev/src/bootstrap.bash

# ~~util-linux~~ I am going to consider ./programs/mount "good enough."
#