# awk in Linux

These are the invocations of `awk` made in the process of building a Linux
kernel using `make ARCH=x86_64 x86_64_defconfig`:

```
awk 
/^#define ORC_(REG|TYPE)_/ { print }
/^struct orc_entry {$/ { p=1 }
p { print }
/^}/ { p=0 }

awk -f ../arch/x86/tools/gen-insn-attr-x86.awk ../arch/x86/lib/x86-opcode-map.txt 

awk !x[$0]++ { print("fs/efivarfs/"$0) } 

awk -f ./arch/x86/tools/gen-insn-attr-x86.awk ./arch/x86/lib/x86-opcode-map.txt 

awk !x[$0]++ { print("drivers/thermal/intel/"$0) } 
```

The two scripts used above definitely make use of regex, but I think this can be
circumvented by just pregenerating the `inat-tables.c` files.

This is the result after using `make ARCH=x86_64 allnoconfig`:

```
awk 
/^#define ORC_(REG|TYPE)_/ { print }
/^struct orc_entry {$/ { p=1 }
p { print }
/^}/ { p=0 }

awk -f ../arch/x86/tools/gen-insn-attr-x86.awk ../arch/x86/lib/x86-opcode-map.txt 

awk -f ./arch/x86/tools/gen-insn-attr-x86.awk ./arch/x86/lib/x86-opcode-map.txt
```

See: `scripts/Makefile.build` for the two `!x[$0]++` invocations. One is turned
off by disabling `EFIVAR_FS`. The other requires turning off thermal drivers
which is probably not a good idea for a bootstrapping OS that will be compiling
a lot of software. Either way, these are easy to reproduce if you just want to
implement them.
