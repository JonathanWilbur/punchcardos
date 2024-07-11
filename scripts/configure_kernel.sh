#!/bin/sh

# To be executed within the Linux Kernel's source tree.

# TODO: Refine this further
./scripts/config --set-val CONFIG_NET n
./scripts/config --set-val CONFIG_ELF_CORE y
./scripts/config --set-val CONFIG_AUTOFS_FS y
./scripts/config --set-val CONFIG_IKCONFIG y
./scripts/config --set-val CONFIG_IKCONFIG_PROC y

./scripts/config --set-val CONFIG_BLK_DEV y
./scripts/config --set-val CONFIG_BLK_DEV_INITRD y

./scripts/config --set-val FAT_FS y
./scripts/config --set-val MSDOS_FS y
./scripts/config --set-val VFAT_FS y
./scripts/config --set-val EXFAT_FS y

# This might be needed for initramfs to work.
./scripts/config --set-val CONFIG_BLK_DEV_RAM y
./scripts/config --set-val CONFIG_BLK_DEV_RAM_COUNT 1
# The number is in unit of 1014B. Currently set to 128MB.
./scripts/config --set-val CONFIG_BLK_DEV_RAM_SIZE 131072

./scripts/config --set-val CONFIG_TTY y
./scripts/config --set-val CONFIG_SERIAL_8250 y
./scripts/config --set-val CONFIG_SERIAL_8250_CONSOLE y

./scripts/config --set-val CONFIG_BINFMT_ELF y
./scripts/config --set-val CONFIG_COMPAT_BINFMT_ELF y
./scripts/config --set-val CONFIG_BINFMT_SCRIPT y

./scripts/config --set-val CONFIG_CRYPTO_MANAGER_DISABLE_TESTS y

# I believe some of these 
./scripts/config --set-val CONFIG_PCI y
./scripts/config --set-val CONFIG_ATA y
./scripts/config --set-val CONFIG_BLK_DEV_GENERIC y
./scripts/config --set-val CONFIG_ATA_PIIX y
./scripts/config --set-val CONFIG_IDE y
./scripts/config --set-val CONFIG_BLK_DEV_IDE y
./scripts/config --set-val CONFIG_BLK_DEV_IDEPCI y
./scripts/config --set-val CONFIG_64BIT y
./scripts/config --set-val CONFIG_BLK_DEV_SD y

# Disabled because I was getting kernel panics in QEMU from some sort of tracing
# self-tests.
./scripts/config --set-val CONFIG_FTRACE_SELFTEST n
./scripts/config --set-val CONFIG_FTRACE_STARTUP_TEST n
./scripts/config --set-val CONFIG_EVENT_TRACE_STARTUP_TEST n
./scripts/config --set-val CONFIG_FTRACE_SORT_STARTUP_TEST n
./scripts/config --set-val CONFIG_MMIOTRACE_TEST n

# Add these lines to Kbuild for reproducibility
echo 'KBUILD_BUILD_USER=a' >> Kbuild
echo 'KBUILD_BUILD_HOST=b' >> Kbuild
echo 'KBUILD_BUILD_TIMESTAMP="Sat 22 Jun 2024 03:54:20 PM EDT"' >> Kbuild