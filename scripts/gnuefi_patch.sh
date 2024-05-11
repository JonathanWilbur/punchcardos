#!/bin/bash

# Run this inside the src directory of 'gnu-efi'
# HACK: patch the GNU-EFI library to force CHAR16 to always be an unsigned
# short, as the L"string" literals when -fshort-wchar is used. That is necessary
# because with the custom cross musl toolchain built for host=aarch64, wchar_t
# is always defined as "int".
 
old="typedef wchar_t CHAR16"
new="typedef unsigned short CHAR16"

for x in ia32 x86_64; do
    file="inc/${x}/efibind.h"
    if ! [ -f $file ]; then
        echo "ERROR: file $file not found!"
        exit 1
    fi
    sed -i "s/${old}/${new}/" $file
done