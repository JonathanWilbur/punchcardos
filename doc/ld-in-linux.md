# Linker Usage in Linux

I replaced `/usr/bin/ld` with a fake program that logs the command-line
invocation, and then executes the real `ld` with those options. I did this so I
could see where and how LD is getting called during the Linux kernel build, so
I can see the minimal feature set that would be needed in a minimal `ld` to be
able to build the Linux kernel.

## Complete List of Invocations

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments -v` (This just prints the version)

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/cc38kQVJ.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o scripts/basic/fixdep /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/cchu9kep.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/cctSR7Sy.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o arch/x86/tools/relocs /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. arch/x86/tools/relocs_32.o arch/x86/tools/relocs_64.o arch/x86/tools/relocs_common.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/ccshcu0E.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o scripts/kallsyms /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/ccprJN8h.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/ccgSJZ93.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o scripts/sorttable /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/ccbPzVwT.o -lpthread -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `ld -v` (This just prints the version)

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/ccS1DsCP.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o scripts/mod/mk_elfconfig /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/cc9aQCFP.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/ccWkR8Y6.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o scripts/mod/modpost /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. scripts/mod/modpost.o scripts/mod/file2alias.o scripts/mod/sumversion.o scripts/mod/symsearch.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --eh-frame-hdr -v`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --eh-frame-hdr -v`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --eh-frame-hdr -v`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --eh-frame-hdr -v`

- `ld -o arch/x86/entry/vdso/vdso32.so.dbg -shared --hash-style=both --build-id=sha1 --eh-frame-hdr -Bsymbolic -z noexecstack -m elf_i386 -soname linux-gate.so.1 -T arch/x86/entry/vdso/vdso32/vdso32.lds arch/x86/entry/vdso/vdso32/note.o arch/x86/entry/vdso/vdso32/system_call.o arch/x86/entry/vdso/vdso32/sigreturn.o arch/x86/entry/vdso/vdso32/vclock_gettime.o arch/x86/entry/vdso/vdso32/vgetcpu.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/cca6Lv6d.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o arch/x86/entry/vdso/vdso2c /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/cc6118LA.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments -m elf_i386 --emit-relocs -T arch/x86/realmode/rm/realmode.lds arch/x86/realmode/rm/header.o arch/x86/realmode/rm/trampoline_32.o arch/x86/realmode/rm/stack.o arch/x86/realmode/rm/reboot.o -o arch/x86/realmode/rm/realmode.elf`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/cchznbn0.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o lib/gen_crc32table /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/ccCYVHRy.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/cc7akoXu.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o drivers/tty/vt/conmakehash /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/cc0qt3lg.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments -r -o vmlinux.o --whole-archive vmlinux.a --no-whole-archive --start-group lib/lib.a arch/x86/lib/lib.a --end-group`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --build-id=sha1 --orphan-handling=warn --script=./arch/x86/kernel/vmlinux.lds --strip-debug -o .tmp_vmlinux1 --whole-archive vmlinux.a init/version-timestamp.o --no-whole-archive --start-group lib/lib.a arch/x86/lib/lib.a --end-group .tmp_vmlinux0.kallsyms.o`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --build-id=sha1 --orphan-handling=warn --script=./arch/x86/kernel/vmlinux.lds --strip-debug -o .tmp_vmlinux2 --whole-archive vmlinux.a init/version-timestamp.o --no-whole-archive --start-group lib/lib.a arch/x86/lib/lib.a --end-group .tmp_vmlinux1.kallsyms.o`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments --build-id=sha1 --orphan-handling=warn --script=./arch/x86/kernel/vmlinux.lds -o vmlinux --whole-archive vmlinux.a init/version-timestamp.o --no-whole-archive --start-group lib/lib.a arch/x86/lib/lib.a --end-group .tmp_vmlinux2.kallsyms.o`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/ccWv1d4V.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o arch/x86/boot/mkcpustr /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/ccdkUB4y.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `ld -m elf_i386 --no-ld-generated-unwind-info -v` (Just prints the version)

- `ld -m elf_i386 --no-ld-generated-unwind-info --no-dynamic-linker -v` (Just prints the version)

- `ld -m elf_i386 --no-ld-generated-unwind-info --no-warn-rwx-segments -v` (Just prints the version)

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/ccb6biom.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o arch/x86/boot/compressed/mkpiggy /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/cce0UZWC.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

- `ld -m elf_i386 --no-ld-generated-unwind-info -pie --no-dynamic-linker --orphan-handling=warn -z noexecstack --no-warn-rwx-segments -T arch/x86/boot/compressed/vmlinux.lds arch/x86/boot/compressed/kernel_info.o arch/x86/boot/compressed/head_32.o arch/x86/boot/compressed/misc.o arch/x86/boot/compressed/string.o arch/x86/boot/compressed/cmdline.o arch/x86/boot/compressed/error.o arch/x86/boot/compressed/piggy.o arch/x86/boot/compressed/cpuflags.o arch/x86/boot/compressed/early_serial_console.o -o arch/x86/boot/compressed/vmlinux`

- `ld -m elf_i386 -z noexecstack --no-warn-rwx-segments -m elf_i386 -z noexecstack -T arch/x86/boot/setup.ld arch/x86/boot/a20.o arch/x86/boot/bioscall.o arch/x86/boot/cmdline.o arch/x86/boot/copy.o arch/x86/boot/cpu.o arch/x86/boot/cpuflags.o arch/x86/boot/cpucheck.o arch/x86/boot/early_serial_console.o arch/x86/boot/edd.o arch/x86/boot/header.o arch/x86/boot/main.o arch/x86/boot/memory.o arch/x86/boot/pm.o arch/x86/boot/pmjump.o arch/x86/boot/printf.o arch/x86/boot/regs.o arch/x86/boot/string.o arch/x86/boot/tty.o arch/x86/boot/video.o arch/x86/boot/video-mode.o arch/x86/boot/version.o arch/x86/boot/video-vga.o arch/x86/boot/video-vesa.o arch/x86/boot/video-bios.o -o arch/x86/boot/setup.elf`

- `/usr/bin/ld -plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so -plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper -plugin-opt=-fresolution=/tmp/cc63dBy0.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o arch/x86/boot/tools/build /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/13/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/13 -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/13/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/13/../../.. /tmp/ccdFpst0.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/13/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/crtn.o`

## De-Duplicated List of Options Used

| Option                        | Description                                                       | Can be ignored?                                                   |
|-------------------------------|-------------------------------------------------------------------|-------------------------------------------------------------------|
| `-m elf_i386` or `elf_x86_64` | Designate architecture                                            | Could be hard-coded                                               |
| `-z noexecstack`              | Not requiring executable stack                                    | Maybe, but probably easy                                          |
| `--no-warn-rwx-segments`      | Obvious to me                                                     | Not even supported by my `ld`. Didn't seem to matter.             |
| `--eh-frame-hdr`              | `.eh_frame_hdr` section & ELF `PT_GNU_EH_FRAME` segment header.*  | This seems like it could be complicated, but maybe not necessary  |
| `-v`                          | Display version number                                            | No, but its easy.                                                 |
| `-plugin`                     | Kill me                                                           | Not sure.                                                         |
| `-plugin-opt`                 | Does not appear to be supported on my `ld`.                       | Maybe not necessary?                                              |
| `--build-id`                  | Generate a build identifier section in the ELF file               | Probably not necessary, but also might not be too hard.           |
| `--hash-style=gnu`            | Hashing mechanism for ELF.                                        | It sounds like hashing is not needed entirely.                    |
| `--as-needed`                 | I think this means the shared libraries get linked as needed.     | I might not even support dynamic linking.                         |
| `-dynamic-linker`             | Specify the dynamic linker                                        | Might not support.                                                |
| `-pie`                        | Position Independent Executable                                   | Seems only important for shared libraries.                        |
| `-z now`                      | Resolve dynamic symbols at start time rather than function calls  | Only shared libraries                                             |
| `-z relro`                    | Create an ELF "PT_GNU_RELRO" segment header in the object.        | Only shared libraries                                             |
| `-l`                          | Library to link                                                   | No                                                                |
| `-shared`                     | Create a shared library                                           | Probably not going to do this.                                    |
| `-Bsymbolic`                  | Sounds like "use defs of globals from shlib instead"              | Probably not                                                      |
| `--whole-archive`             | Include every object in the archive instead of cherry-picking     | Could be a default                                                |
| `-r`                          | Generate relocatable output                                       | No                                                                |
| `--emit-relocs`               | Leave relocation sections in executables                          | Probably could be ignored, but also probably easy to do.          |
| `-T` or `--script`            | Specify linker script                                             | No                                                                |
| `--strip-debug`               | Omit debug symbols                                                | Yes                                                               |
| `--ld-generated-unwind-info`  | Something about `.eh_frame` section.                              | Yes                                                               |
| `--no-dynamic-linker`         | Omit the dynamic linker                                           | Yes                                                               |
| `--orphan-handling=warn`      | Warn about orphaned sections.                                     | Yes                                                               |
| `--start-group` `--end-group` | Not supported by my linker. Probably doesn't matter.              | Yes                                                               |
| `--push-state` `--pop-state`  | Push and pop linker flags                                         | No, but not too hard.                                             |
| `-soname`                     | Set the internal shared object name `SONAME`                      | Shared libraries: maybe not                                       |

\* https://refspecs.linuxfoundation.org/LSB_1.3.0/gLSB/gLSB/ehframehdr.html

Plugin Options Used:

```
-plugin /usr/libexec/gcc/x86_64-linux-gnu/13/liblto_plugin.so
-plugin-opt=/usr/libexec/gcc/x86_64-linux-gnu/13/lto-wrapper
-plugin-opt=-fresolution=/tmp/cc38kQVJ.res
-plugin-opt=-pass-through=-lgcc
-plugin-opt=-pass-through=-lgcc_s
-plugin-opt=-pass-through=-lc
-plugin-opt=-pass-through=-lgcc
-plugin-opt=-pass-through=-lgcc_s
```

In conclusion, these seem to be the flags I will definitely need to support:

- `-l`
- `-m`
- `-v`
- `-o`
- `-T`
- `--push-state` and `--pop-state`

## Binaries Produced

These are only the binaries produced using `/usr/bin/ld`. There may be more.

| Name                          | Description                                                                       |
|-------------------------------|-----------------------------------------------------------------------------------|
| `fixdep`                      | "Optimize" a list of dependencies as spit out by gcc -MD for the build framework. |
| `kallsyms`                    | Generate assembler source containing symbol information                           |
| `relocs`                      | ???                                                                               |
| `sorttable`                   | Sort the `vmlinux` table in place.                                                |
| `mk_elfconfig`                | Create just a few C headers from an ELF binary being piped to stdin               | 
| `modpost`                     | Something to do with kernel modules                                               |
| `vdso32.so.dbg`               | VDSO Shared Library                                                               |
| `vdso2c`                      | This looks like it generates C bindings for VDSO.                                 |
| `gen_crc32table`              | This looks like it generates a CRC-32 table in a C header                         |
| `conmakehash`                 | Something used by `vt`. Not clear at all. Generates `consolemap_deftbl.c`         |
| `mkcpustr`                    | Generates `cpustr.h`                                                              |
| `mkpiggy`                     | Generates some kind of assembly wrapper called `piggy.S`                          |
| `vmlinux`                     | The Linux Kernel                                                                  |
| `setup.elf`                   | Used to make `setup.bin`: something about setting up before the kernel is loaded  |
| `build`                       | Combines `setup.bin`, `vmlinux`, and `zoffset.h` to produce `bzImage`             |

## Reducing the footprint

`CONFIG_VDSO=n` should get rid of the VDSO builds.

Could `CONFIG_KERNEL_UNCOMPRESSED=y` be used to produce a simpler build process?
And perhaps not require bzip2?

### Getting Rid of VDSO entirely

I removed Make's recursion into the VDSO subdirectories in the Linux repo, and
I got all these errors:

```
  CC      init/version-timestamp.o
  KSYMS   .tmp_vmlinux0.kallsyms.S
  AS      .tmp_vmlinux0.kallsyms.o
  LD      .tmp_vmlinux1
ld: arch/x86/entry/common.o: in function `do_fast_syscall_32':
common.c:(.noinstr.text+0x14e): undefined reference to `vdso_image_32'
ld: arch/x86/kernel/signal_32.o: in function `ia32_setup_frame':
signal_32.c:(.text+0x39a): undefined reference to `vdso_image_32'
ld: arch/x86/kernel/signal_32.o: in function `ia32_setup_rt_frame':
signal_32.c:(.text+0x627): undefined reference to `vdso_image_32'
ld: arch/x86/kernel/traps.o: in function `math_error':
traps.c:(.text+0x167): undefined reference to `fixup_vdso_exception'
ld: arch/x86/kernel/traps.o: in function `do_trap':
traps.c:(.text+0x281): undefined reference to `fixup_vdso_exception'
ld: arch/x86/kernel/traps.o: in function `exc_general_protection':
traps.c:(.noinstr.text+0x7e0): undefined reference to `fixup_vdso_exception'
ld: arch/x86/kernel/tsc.o: in function `tsc_cs_enable':
tsc.c:(.text+0x11): undefined reference to `vclocks_used'
ld: tsc.c:(.text+0x19): undefined reference to `vclocks_used'
ld: arch/x86/mm/fault.o: in function `__bad_area_nosemaphore.constprop.0':
fault.c:(.text+0xab1): undefined reference to `fixup_vdso_exception'
ld: arch/x86/mm/fault.o: in function `exc_page_fault':
fault.c:(.noinstr.text+0x3ad): undefined reference to `fixup_vdso_exception'
ld: kernel/sysctl.o:(.data+0x1a4): undefined reference to `vdso32_enabled'
ld: kernel/entry/syscall_user_dispatch.o: in function `syscall_user_dispatch':
syscall_user_dispatch.c:(.text+0xdb): undefined reference to `arch_syscall_is_vdso_sigreturn'
make[2]: *** [scripts/Makefile.vmlinux:34: vmlinux] Error 1
make[1]: *** [/home/jonathan/repos/linux/Makefile:1175: vmlinux] Error 2
make: *** [Makefile:240: __sub-make] Error 2
```

Symbols missing:

- `vdso_image_32`
  - Invocation of `ia32_setup_frame` in `setup_rt_frame` can be conditionally
    compiled only if `CONFIG_X86_32` or `CONFIG_IA32_EMULATION`, then
    `ia32_setup_frame` can be excluded as well.
  - The only thing I have to figure out now is `do_fast_syscall_32`:
  - And `do_SYSENTER_32`
- `fixup_vdso_exception`
- `vclocks_used` (This should be super easy to replace)
- `vdso32_enabled` (This should also be easy to replace)
  - Curiously, `CONFIG_COMPAT_VDSO` sets this value to `false`
- `arch_syscall_is_vdso_sigreturn`
  - Trivially replaced

It almost looks like you could disable usage of these by setting
`CONFIG_X86_32` and `CONFIG_IA32_EMULATION` to `n`. I think almost everything
should work just fine if you do this.

~~Actually, setting `CONFIG_X86_32` to `n` gets undone automatically.~~
Actually, it appears that you have to explicitly set `ARCH=x86_64` when
invoking `make`, even on an `x86_64` build host.

I have now fixed this with a patch to the Linux source.

Strangely, I see these two entries:

```
ld /tmp/tmp.FRtGZ34AAw.o -shared -Bsymbolic --pack-dyn-relocs=relr -o /tmp/tmp.FRtGZ34AAw 

ld /tmp/tmp.FRtGZ34AAw.o -shared -Bsymbolic -z pack-relative-relocs -o /tmp/tmp.FRtGZ34AAw 
```

These come from `scripts/tools-support-relr.sh`. It looks like the above is just
a test to see if your tooling supports "relr." I don't know what would happen if
it does not.
