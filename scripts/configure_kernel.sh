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

# Smaller, simpler build
./scripts/config --set-val CONFIG_KERNEL_UNCOMPRESSED y
./scripts/config --set-val CONFIG_VDSO n
./scripts/config --set-val CONFIG_HAVE_GENERIC_VDSO n

# Include debug symbols in the kernel
./scripts/config --set-val CONFIG_DEBUG_INFO y

# I don't think 32-bit mode is necessary.
./scripts/config --set-val CONFIG_IA32_EMULATION n
./scripts/config --set-val CONFIG_X86_32 n

# We turn this off so that `vdso32_enabled` is not needed.
# ./scripts/config --set-val CONFIG_PROC_SYSCTL n

./scripts/config --set-val CONFIG_X86_SKIP_VDSO y

# This mitigates undefined symbols introduced with CONFIG_X86_SKIP_VDSO when
# building an x86_64_defconfig build.
./scripts/config --set-val CONFIG_TIME_NS n

# These options being turned off should prevent inat tables from being built,
# which mitigates our need for an awk implementation with regex support.
./scripts/config --set-val CONFIG_KVM n
./scripts/config --set-val CONFIG_X86_EMULATION n
./scripts/config --set-val CONFIG_BPF_JIT n
./scripts/config --set-val CONFIG_BPF_JIT_ALWAYS_ON n
./scripts/config --set-val CONFIG_FUNCTION_TRACER n
./scripts/config --set-val CONFIG_X86_DECODER_SELFTEST n
./scripts/config --set-val CONFIG_KGDB n
./scripts/config --set-val CONFIG_FRAME_POINTER n
./scripts/config --set-val CONFIG_FTRACE n
./scripts/config --set-val CONFIG_X86_XSAVE n
./scripts/config --set-val CONFIG_X86_AVX512 n

# This mitigates the need for one awk, sed, sha1sum, and cut in orc_hash.sh.
./scripts/config --set-val CONFIG_UNWINDER_ORC n

# Add these lines to Kbuild for reproducibility
echo 'KBUILD_BUILD_USER=a' >> Kbuild
echo 'KBUILD_BUILD_HOST=b' >> Kbuild
echo 'KBUILD_BUILD_TIMESTAMP="Sat 22 Jun 2024 03:54:20 PM EDT"' >> Kbuild
