/*
Minimal objcopy implementation needed to compile the Linux kernel by
Jonathan M. Wilbur. This compiles with nolibc.
This explanation of binary files was instrumental to implementing this:
https://software-dl.ti.com/ccs/esd/documents/sdto_cgt_an_introduction_to_binary_files.html

DO NOT USE THIS FOR UNTRUSTWORTHY BINARIES. IT ALMOST CERTAINLY HAS SECURITY
VULNERABILITIES.

When using the elf64 target, this will not produce the same exact output as
objcopy. It seems that objcopy reorders the headers by putting the .shstrtab
header at the end. Note that this also affects the ELF header. I might fix this,
since I would then be able to verify that this implementation produces
byte-for-byte equivalent outputs to the GNU objcopy.

## License

Copyright 2024 Jonathan M. Wilbur <jonathan@wilbur.space>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
// Needs to work with these use cases to build the Linux kernel:
// objcopy -S --remove-section __ex_table  arch/x86/entry/vdso/vdso64.so.dbg arch/x86/entry/vdso/vdso64.so
// objcopy -O binary arch/x86/realmode/rm/realmode.elf arch/x86/realmode/rm/realmode.bin
// objcopy -j .modinfo -O binary vmlinux.o modules.builtin.modinfo
// objcopy -R .comment -S vmlinux arch/x86/boot/compressed/vmlinux.bin
// objcopy -O binary -R .note -R .comment -S arch/x86/boot/compressed/vmlinux arch/x86/boot/vmlinux.bin
// objcopy -O binary arch/x86/boot/setup.elf arch/x86/boot/setup.bin
// - O = output format
// - R = remove from output
// - S = Do not copy relocation and symbol information from the source file
// - j = Only section
#define _GNU_SOURCE
#ifndef NOLIBC
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#else

int fstat (int fd, struct stat *statbuf) {
    return __sysret(my_syscall2(__NR_fstat, fd, statbuf));
}

int ftruncate (int fd, off_t length) {
    return __sysret(my_syscall2(__NR_ftruncate, path, length));
}

// This is a Linux-specific system call
int fallocate(int fd, int mode, off_t offset, off_t len) {
    return __sysret(my_syscall4(__NR_fallocate, fd, mode, offset, len));
}

ssize_t copy_file_range(int fd_in, long int *off_in,
                        int fd_out, long int *off_out,
                        size_t len, unsigned int flags) {
    return __sysret(my_syscall6(__NR_copy_file_range, fd_in, off_in, fd_out, off_out, len, flags));
}

#endif

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

#define REMOVE_BUFSIZ       32
#define ONLY_BUFSIZ         32

const char* CRAZY_WORD_SIZE_MSG = "ELF File is neither 32-bit or 64-bit.";
const char* WEIRD_OVERLAP = "Weird (maybe not invalid) ELF file. Not supported.";
const char* WEIRD_HEADER_SIZE = "Unsupported program or section header size.";
const char* USAGE_MSG = "objcopy [-O binary|-S|-R <section>|-j <section>] src [dest]";

static void print_usage () {
    puts(USAGE_MSG);
}

/* We will need to sort by virtual address prior to writing the sections. */
void bubble_sort_section_headers (Elf64_Shdr *arr, int n) {
    int i, j;
    Elf64_Shdr temp;

    for (i = 0; i < n - 1; i++) {
        // Last i elements are already sorted
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j].sh_addr > arr[j+1].sh_addr) {
                // Swap arr[j] and arr[j+1]
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

char* sh_strs = NULL;

static char* get_section_header_name (uint32_t offset) { // Offset is 32 bits, even in 64-bit ELF.
    return &sh_strs[offset];
}

static int load_section_header_strings (int fd, Elf64_Ehdr *elf_header) {
    Elf64_Shdr shdr;

    Elf64_Off start_shstrs = elf_header->e_shoff
        + ((Elf64_Off)elf_header->e_shstrndx * (Elf64_Off)elf_header->e_shentsize);
seek_to_shstrs_header_offset:
    if (lseek(fd, start_shstrs, SEEK_SET) != start_shstrs)
        return EXIT_FAILURE;
read_shstrs_header:
    if (read(fd, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr))
        return EXIT_FAILURE;
check_if_its_actually_section_header_strtab:
    if (shdr.sh_type != SHT_STRTAB)
        return EXIT_FAILURE;
seek_to_shstrs_section_body:
    if (lseek(fd, shdr.sh_offset, SEEK_SET) != shdr.sh_offset)
        return EXIT_FAILURE;
allocate_room_for_section_header_strings:
    sh_strs = malloc(shdr.sh_size + 1);
    if (sh_strs == NULL) {
        errno = ENOMEM;
        perror("could not store section header strings table");
        return EXIT_FAILURE;
    }
    /* Set the last byte of the section header strings to NULL, just in case it
    is malformed and has no terminator. */
    sh_strs[shdr.sh_size] = 0;

    _header_strings:
    if (read(fd, sh_strs, shdr.sh_size) != shdr.sh_size)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int is_allowed (
    Elf64_Shdr *shdr,
    int strip_all,
    char *remove[REMOVE_BUFSIZ],
    char *only[ONLY_BUFSIZ]
) {
    char* name;
    char** list_name;

    if (
        strip_all
        && (
            shdr->sh_type == SHT_SYMTAB
            || shdr->sh_type == SHT_DYNSYM
            || shdr->sh_type == SHT_REL
            || shdr->sh_type == SHT_RELA
            || shdr->sh_type == SHT_SYMTAB_SHNDX
        )
    )
        return 0;

    name = get_section_header_name(shdr->sh_name);
    if (only[0] != NULL) { // If we are whitelisting.
        list_name = &only[0];
        while ((*list_name) != NULL) {
            if (!strcmp(*list_name, name))
                return 1;
            list_name++;
        }
    } else if (remove[0] != NULL) { // Otherwise, we are blacklisting
        list_name = &remove[0];
        while ((*list_name) != NULL) {
            if (!strcmp(*list_name, name))
                return 0;
            list_name++;
        }
    }

    return 1;
}

static int objcopy_bin64 (
    int ifd,
    int ofd,
    int strip_all,
    char *remove[REMOVE_BUFSIZ],
    char *only[ONLY_BUFSIZ],
    Elf64_Ehdr *elf_header
) {
    Elf64_Shdr *section_headers;
    Elf64_Shdr shdr;
    int s = 0;
    off_t offset;

seek_to_start_of_section_headers:
    if (lseek(ifd, elf_header->e_shoff, SEEK_SET) != elf_header->e_shoff) {
        perror("lseek input file to start of section headers");
        return EXIT_FAILURE;
    }

    section_headers = malloc(elf_header->e_shnum * sizeof(Elf64_Shdr));
    if (section_headers == NULL) {
        errno = ENOMEM;
        perror("failed to allocate memory for section headers");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < elf_header->e_shnum; i++) {
        if (read(ifd, (char *)&section_headers[s++], sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr)) {
            perror("read section header");
            return EXIT_FAILURE;
        }
        if (
            elf_header->e_shentsize > sizeof(Elf64_Shdr)
            && (lseek(ifd, (elf_header->e_shentsize - sizeof(Elf64_Shdr)), SEEK_CUR) < 0)
        ) {
            perror("seek to end of section header");
            return EXIT_FAILURE;
        }
    }
    bubble_sort_section_headers(section_headers, elf_header->e_shnum);

    Elf64_Addr last_byte_written = 0;
    for (int i = 0; i < elf_header->e_shnum; i++) {
        // Subtract the smallest address from all addresses
        shdr = section_headers[i];
        shdr.sh_addr -= section_headers[0].sh_addr;
        if (!is_allowed(&shdr, strip_all, remove, only))
            continue;
        if (shdr.sh_size == 0 || !(shdr.sh_flags & SHF_ALLOC))
            continue;
        // Fill with zeroes
        if (i > 0
            && (last_byte_written < shdr.sh_addr)
            && (fallocate(ofd, FALLOC_FL_ZERO_RANGE, last_byte_written, shdr.sh_addr - last_byte_written) < 0)) {
            perror("fallocate gap-fill");
            printf("%ld %ld\n", shdr.sh_addr, last_byte_written);
            return EXIT_FAILURE;
        }
        if (copy_file_range(ifd, &shdr.sh_offset, ofd, &shdr.sh_addr, shdr.sh_size, 0) < 0) {
            perror("copy section");
            return EXIT_FAILURE;
        }
        last_byte_written = shdr.sh_addr;
    }
    return EXIT_SUCCESS;
}

static int objcopy_elf64 (
    int ifd,
    int ofd,
    int strip_all,
    char *remove[REMOVE_BUFSIZ],
    char *only[ONLY_BUFSIZ],
    Elf64_Ehdr *elf_header
) {
    struct stat st;
    Elf64_Half e_phnum;
    Elf64_Half e_shnum;
    Elf64_Phdr phdr;
    Elf64_Shdr shdr;
    // Two pointers, because they get clobbered by copy_file_range.
    Elf64_Off e_phoff1 = elf_header->e_phoff;
    Elf64_Off e_phoff2 = elf_header->e_phoff;
    // Two pointers, because they get clobbered by copy_file_range.
    loff_t off1;
    loff_t off2;
    size_t len;
    size_t prog_headers_size = elf_header->e_phentsize * elf_header->e_phnum;
    char** list_name;
    unsigned int headers_removed;

get_input_file_size:
    if (fstat(ifd, &st) < 0) {
        perror("fstat input");
        return EXIT_FAILURE;
    }
    
set_output_file_size_same_as_input:
    if (ftruncate(ofd, st.st_size) < 0) {
        perror("ftruncate output");
        return EXIT_FAILURE;
    }

write_all_elf_header_bytes:
    /* sizeof(Elf64_Ehdr) is not always == e_ehsize. You have to copy the whole
    ELF header, even though you may have only read / used part of it. This will
    initially have the wrong number of section headers (assuming you used
    objcopy to remove headers), but it will get overwritten at the end with just
    the modified in-memory Elf64_Ehdr. Search for d4d30907-605a-4776-93e6-97488f8fb318. */
    off1 = 0;
    off2 = 0;
    if (copy_file_range(ifd, &off1, ofd, &off2, elf_header->e_ehsize, 0) < 0) {
        perror("copy_file_range for ELF header");
        return EXIT_FAILURE;
    }

copy_each_program_header_exactly:
    if (
        e_phoff1 > 0
        && copy_file_range(ifd, &e_phoff1, ofd, &e_phoff2, prog_headers_size, 0) < 0) {
        perror("copy_file_range for program headers");
        return EXIT_FAILURE;
    }

seek_to_start_of_section_headers:
    if (lseek(ifd, elf_header->e_shoff, SEEK_SET) != elf_header->e_shoff) {
        perror("lseek input file to start of section headers");
        return EXIT_FAILURE;
    }
    if (lseek(ofd, elf_header->e_shoff, SEEK_SET) != elf_header->e_shoff) {
        perror("lseek output file to start of section headers");
        return EXIT_FAILURE;
    }

    headers_removed = 0;
    for (int i = 0; i < elf_header->e_shnum; i++) {
read_a_section_header:
        if (read(ifd, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr)) {
            perror("read section header");
            return EXIT_FAILURE;
        }
        if (elf_header->e_shentsize > sizeof(Elf64_Shdr)
            && lseek(ifd, elf_header->e_shentsize - sizeof(Elf64_Shdr), SEEK_CUR) < 0) {
            perror("seek to end of section header");
            return EXIT_FAILURE;
        }
determine_if_section_header_is_allowed:
        if (!is_allowed(&shdr, strip_all, remove, only)) {
            headers_removed++;
            continue;
        }
        if (i == elf_header->e_shstrndx)
            elf_header->e_shstrndx -= headers_removed;
write_section_header:
        if (shdr.sh_link > 0)
            shdr.sh_link -= headers_removed;
        if (write(ofd, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr)) {
            perror("write section header");
            return EXIT_FAILURE;
        }
        if (elf_header->e_shentsize > sizeof(Elf64_Shdr)) {
            len = elf_header->e_shentsize - sizeof(Elf64_Shdr);
            if (copy_file_range(ifd, NULL, ofd, NULL, len, 0) < 0) {
                perror("copy_file_range for section headers");
                return EXIT_FAILURE;
            }
        }

copy_section_contents:
        off1 = shdr.sh_offset;
        off2 = off1;
        if (shdr.sh_size > 0 && copy_file_range(ifd, &off1, ofd, &off2, shdr.sh_size, 0) < 0) {
            perror("copy section header content");
            return EXIT_FAILURE;
        }
    }

seek_to_start_of_output_file:
    if (lseek(ofd, 0, SEEK_SET) != 0) {
        perror("lseek output file to start of output file");
        return EXIT_FAILURE;
    }

overwrite_elf_header:
    /* This is where the original (possibly now incorrect) ELF header gets
    overwritten by one with the correct number of section headers. */
    elf_header->e_shnum -= headers_removed;
    if (write(ofd, (char *)elf_header, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)) {
        perror("write ELF header");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#define OUTPUT_BIN      0
#define OUTPUT_ELF64    1

#define NEXT_ARG_NONE       0
#define NEXT_ARG_OUTPUT     1
#define NEXT_ARG_REMOVE     2
#define NEXT_ARG_ONLY       3

static int objcopy64 (
    int ifd,
    int ofd,
    int strip_all,
    char *remove[REMOVE_BUFSIZ],
    char *only[ONLY_BUFSIZ],
    int output_type
) {
    Elf64_Ehdr header;
    int rc = EXIT_FAILURE;

read_elf_header:
    if (read(ifd, (char *)&header, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr))
        return EXIT_FAILURE;
    if (header.e_ehsize != 64)
        return EXIT_FAILURE;

    // Very unusual file layout where the program headers or section headers
    // overlap with the ELF header.
    if (
        (header.e_phoff > 0 && header.e_phoff < 0x40)
        || (header.e_shoff > 0 && header.e_shoff < 0x40)
    ) {
        puts(WEIRD_OVERLAP);
        return EXIT_FAILURE;
    }

    // Program or section headers are too small to fill in our structs.
    if (
        (header.e_phentsize > 0 && header.e_phentsize < sizeof(Elf64_Phdr))
        || (header.e_shentsize > 0 && header.e_shentsize < sizeof(Elf64_Shdr))
    ) {
        puts(WEIRD_HEADER_SIZE);
        return EXIT_FAILURE;
    }

    if (load_section_header_strings(ifd, &header) != EXIT_SUCCESS) {
        puts("Failed to load section header strings.");
        return EXIT_FAILURE;
    }

    if (output_type == OUTPUT_BIN) {
        rc = objcopy_bin64(ifd, ofd, strip_all, remove, only, &header);
    } else if (output_type == OUTPUT_ELF64) {
        rc = objcopy_elf64(ifd, ofd, strip_all, remove, only, &header);
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int objcopy (
    int ifd,
    int ofd,
    int strip_all,
    char *remove[REMOVE_BUFSIZ],
    char *only[ONLY_BUFSIZ],
    int output_type
) {
    int is_32bit, is_64bit;
    char e_ident[EI_NIDENT];

read_elf_ident:
    if (read(ifd, e_ident, EI_NIDENT) != EI_NIDENT) {
        errno = ENOEXEC;
        perror("readelf @ main");
        return EXIT_FAILURE;
    }

seek_back_to_zero_offset:
    if (lseek(ifd, 0, SEEK_SET) != 0) {
        perror("lseek to 0");
        return EXIT_FAILURE;
    }

    is_32bit = e_ident[EI_CLASS] == ELFCLASS32;
    is_64bit = e_ident[EI_CLASS] == ELFCLASS64;
    if (!is_32bit && !is_64bit) {
        puts(CRAZY_WORD_SIZE_MSG);
        return EXIT_FAILURE;
    }
    else if (is_64bit) {
        return objcopy64(ifd, ofd, strip_all, remove, only, output_type);
    }
    else {
        puts("32-bit ELF objcopy unsupported.");
        return EXIT_FAILURE;
    }
}

int main (int argc, char **argv) {
    char *arg;
    int next_slot;
    int strip_all = 0;
    int next_arg = NEXT_ARG_NONE;
    int output = OUTPUT_ELF64;
    // To be clear, this fills these arrays with null bytes.
    char *remove[REMOVE_BUFSIZ] = { 0 };
    char *only[ONLY_BUFSIZ] = { 0 };
    int a;
    char *src = NULL;
    char *dest = NULL;
    int ifd;
    int ofd;
    int rc = EXIT_FAILURE;

    /* In theory, you could do "objcopy a.out", but it would do nothing at all,
    so we'll say less than three arguments is invalid. */
    if (argc < 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    a = 1;
    for (; a < argc - 1; a++) {
        arg = argv[a];
        if (strlen(arg) == 0)
            continue;
        switch (next_arg) {
        case NEXT_ARG_OUTPUT:
            if (!strcmp(arg, "elf64")) {
                output = OUTPUT_ELF64;
            } else if (!strcmp(arg, "binary")) {
                output = OUTPUT_BIN;
            } else {
                print_usage();
                return EXIT_FAILURE;
            }
            next_arg = NEXT_ARG_NONE;
            continue;
        case NEXT_ARG_REMOVE:
            next_slot = 0;
            while (remove[next_slot] != NULL && next_slot < ONLY_BUFSIZ)
                next_slot++;
            remove[next_slot] = arg;
            next_arg = NEXT_ARG_NONE;
            continue;
        case NEXT_ARG_ONLY:
            next_slot = 0;
            while (only[next_slot] != NULL && next_slot < ONLY_BUFSIZ)
                next_slot++;
            only[next_slot] = arg;
            next_arg = NEXT_ARG_NONE;
            continue;
        }

        if (!strcmp(arg, "--strip-all") || !strcmp(arg, "-S")) {
            strip_all = 1;
            continue;
        }
        else if (!strcmp(arg, "-O")) {
            next_arg = NEXT_ARG_OUTPUT;
            continue;
        }
        else if (!strcmp(arg, "-j")) {
            next_arg = NEXT_ARG_ONLY;
            continue;
        }
        else if (!strcmp(arg, "-R")) {
            next_arg = NEXT_ARG_REMOVE;
            continue;
        }
        else if (prefix("--output-target=", arg)) {
            arg = &arg[strlen("--output-target=")];
            if (!strcmp(arg, "elf64")) {
                output = OUTPUT_ELF64;
            } else if (!strcmp(arg, "binary")) {
                output = OUTPUT_BIN;
            } else {
                print_usage();
                return EXIT_FAILURE;
            }
            continue;
        }
        else if (prefix("--only-section=", arg)) {
            arg = &arg[strlen("--only-section=")];
            next_slot = 0;
            while (only[next_slot] != NULL && next_slot < ONLY_BUFSIZ)
                next_slot++;
            only[next_slot] = arg;
            next_arg = NEXT_ARG_NONE;
            continue;
        }
        else if (prefix("--remove-section=", arg)) {
            arg = &arg[strlen("--remove-section=")];
            next_slot = 0;
            while (remove[next_slot] != NULL && next_slot < ONLY_BUFSIZ)
                next_slot++;
            remove[next_slot] = arg;
            next_arg = NEXT_ARG_NONE;
            continue;
        }
        else if (prefix("-", arg)) {
            printf("Unrecognized option: %s\n", arg);
            print_usage();
            return EXIT_FAILURE;
        }
        else {
            break;
        }
    }

    for (; a < argc; a++) {
        arg = argv[a];
        if (src == NULL) {
            src = arg;
            continue;
        }
        if (dest == NULL) {
            dest = arg;
            continue;
        }
        print_usage();
        return EXIT_FAILURE;
    }

    if (next_arg != NEXT_ARG_NONE) {
        print_usage();
        return EXIT_FAILURE;
    }

    if (dest == NULL)
        dest = src;

    // TODO: I think you need to handle src == dest differently.
    ifd = open(src, O_RDONLY);
    ofd = open(dest, O_WRONLY | O_CREAT, 0550);

    rc = objcopy(ifd, ofd, strip_all, remove, only, output);

    // TODO: I think you need to handle src == dest differently.
    close(ifd);
    close(ofd);
    return rc;
}
