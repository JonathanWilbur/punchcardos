#!/bin/sh

set -ex

docker build . -t pcosbuild -f ./punchcardos.dockerfile
sudo docker run --privileged pcosbuild bash -c 'mkdir m && mount boot m && cp bzImage init.cpio syslinux.cfg files/* m && umount m'
CONTAINER_ID=$(docker ps --format 'table {{.ID}} {{.Image}}' -a | grep pcosbuild | head -n1 | cut -d ' ' -f1)
docker cp $CONTAINER_ID:/build/boot pcos.img
qemu-system-x86_64 -drive file=pcos.img,format=raw,index=0 -m 1G
