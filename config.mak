BINUTILS_VER = 2.33.1
GCC_VER = 9.4.0
MUSL_VER = 1.2.0
GMP_VER = 6.1.2
MPC_VER = 1.1.0
MPFR_VER = 4.0.2
# ISL_VER =
LINUX_VER = 4.19.90

BINUTILS_CONFIG += --disable-lto
# COMMON_CONFIG += CC="/bin/i686-linux-musl-gcc -static --static" CXX="/opt/static/bin/i686-linux-musl-g++ -static --static"
#COMMON_CONFIG += CFLAGS="-fPIC -g0 -Os" CXXFLAGS="-fPIC -g0 -Os"
COMMON_CONFIG += --disable-nls
COMMON_CONFIG += --enable-static=yes --enable-static
# COMMON_CONFIG += LDFLAGS=" -s --static"
DL_CMD = curl -k -C - -L -o
GCC_CONFIG += --disable-host-shared
GCC_CONFIG += --disable-lto
GCC_CONFIG += --disable-multilib
GCC_CONFIG += --enable-languages=c,c++