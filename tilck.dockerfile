FROM ubuntu:latest
RUN apt update && apt install -y git git-core gcc g++ gcc-multilib \
    g++-multilib clang cmake python3-minimal python3-serial python3-pip apt \
    gawk unzip parted wget curl qemu-system-x86 imagemagick
RUN git clone --depth 1 https://github.com/vvaltchev/tilck.git
# TODO: Check the commit GPG signature?
WORKDIR /tilck
# See: https://github.com/vvaltchev/tilck/issues/177
# We don't want any of the interactive features of apt, so we just disable them.
ENV DEBIAN_FRONTEND=noninteractive
ENV RUNNING_IN_CI=1
# ENV CI=1
RUN ./scripts/build_toolchain
RUN make
