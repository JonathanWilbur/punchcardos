#!/bin/bash

# Given an architecture name using Linux's convention, return the EFI name
# for the same architecture. The result is the same for almost all architectures
# except for "i386" which becomes "ia32".

arch=$1

if [[ "$arch" == i386 ]]; then
    arch="ia32"
fi

echo "$arch"