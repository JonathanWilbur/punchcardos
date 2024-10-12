#ifndef NOLIBC
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#endif

const char* USAGE_MSG = "nm <file>";
const char* CRAZY_WORD_SIZE_MSG = "ELF File is neither 32-bit or 64-bit.";
const char* WEIRD_OVERLAP = "Weird (maybe not invalid) ELF file. Not supported.";
const char* WEIRD_HEADER_SIZE = "Unsupported program or section header size.";

char* sh_strs = NULL;

static char to_hex_char (unsigned char byte) {
    if (byte < 10) {
        return byte + '0';
    } else {
        return (byte - 10) + 'a';
    }
}


static void offset_to_hex (int* b, char* out, uint64_t offset) {
    /* No, this isn't a programming war crime at all! What are you talking about? */
    out[(*b)++] = to_hex_char((offset & 0xF000000000000000UL) >> 60);
    out[(*b)++] = to_hex_char((offset & 0x0F00000000000000UL) >> 56);
    out[(*b)++] = to_hex_char((offset & 0x00F0000000000000UL) >> 52);
    out[(*b)++] = to_hex_char((offset & 0x000F000000000000UL) >> 48);
    out[(*b)++] = to_hex_char((offset & 0x0000F00000000000UL) >> 44);
    out[(*b)++] = to_hex_char((offset & 0x00000F0000000000UL) >> 40);
    out[(*b)++] = to_hex_char((offset & 0x000000F000000000UL) >> 36);
    out[(*b)++] = to_hex_char((offset & 0x0000000F00000000UL) >> 32);
    out[(*b)++] = to_hex_char((offset & 0x00000000F0000000UL) >> 28);
    out[(*b)++] = to_hex_char((offset & 0x000000000F000000UL) >> 24);
    out[(*b)++] = to_hex_char((offset & 0x0000000000F00000UL) >> 20);
    out[(*b)++] = to_hex_char((offset & 0x00000000000F0000UL) >> 16);
    out[(*b)++] = to_hex_char((offset & 0x000000000000F000UL) >> 12);
    out[(*b)++] = to_hex_char((offset & 0x0000000000000F00UL) >> 8);
    out[(*b)++] = to_hex_char((offset & 0x00000000000000F0UL) >> 4);
    out[(*b)++] = to_hex_char((offset & 0x000000000000000FUL));
    out[(*b)++] = '\0';
    return;
}

/* This function assumes sym_fd is seeked to the section offset. */
static int handle_header64 (
    Elf64_Shdr *shdr,
    int sym_fd,
    int elf_fd,
    Elf64_Off shoff,
    Elf64_Half e_shentsize
) {
    Elf64_Sym sym;
    Elf64_Xword size_left = shdr->sh_size;
    Elf64_Shdr strtab_sxn;
    ssize_t rc;
    char *strings;
    ssize_t s; // Used to track how many bytes of strings were read into memory.
    char sym_char = '?';

calculate_strtab_section_header_offset:
    ;
    Elf64_Off strtab_offset = shoff + (shdr->sh_link * e_shentsize);
seek_to_strtab_section_header:
    if (lseek(elf_fd, strtab_offset, SEEK_SET) != strtab_offset) {
        perror("lseek to strtab");
        return EXIT_FAILURE;
    }
read_strtab_section_header:
    rc = read(elf_fd, (void *)&strtab_sxn, sizeof(strtab_sxn));
    if (rc < 0) {
        perror("read strings table header");
        return EXIT_FAILURE;
    }
    if (rc != sizeof(strtab_sxn))
        return EXIT_FAILURE;
    
seek_strtab_section_body:
    if (lseek(elf_fd, strtab_sxn.sh_offset, SEEK_SET) != strtab_sxn.sh_offset) {
        perror("lseek to strtab offset");
        return EXIT_FAILURE;
    }
alloc_strtab_in_memory:
    strings = malloc(strtab_sxn.sh_size);
    if (strings == NULL) {
        errno = ENOMEM;
        perror("Could not store strings in memory");
        return EXIT_FAILURE;
    }
read_strtab_into_memory:
    s = 0;
    size_left = strtab_sxn.sh_size;
    while (size_left > 0) {
        rc = read(elf_fd, &strings[s], size_left);
        if (rc < 0) {
            perror("read strings table");
            return EXIT_FAILURE;
        }
        if (rc == 0) // Is this right?
            return EXIT_FAILURE;
        s += rc;
        size_left -= rc;
    }

    size_left = shdr->sh_size;
    while (size_left >= 24) {
        rc = read(sym_fd, (void *)&sym, sizeof(sym));
        if (rc < 0) {
            perror("read symbols header");
            return EXIT_FAILURE;
        }
        if (rc != sizeof(sym))
            return EXIT_FAILURE;
        if (sym.st_name == 0)
            goto next_symbol;
        if (shdr->sh_entsize > sizeof(sym)) {
            if (lseek(sym_fd, (shdr->sh_entsize - sizeof(sym)), SEEK_CUR) < 0) {
                perror("seek to end of section header");
                return EXIT_FAILURE;
            }
        }
        char buf[24];
        int b = 0;
        offset_to_hex(&b, buf, sym.st_value);
        char *sym_name = &strings[sym.st_name];
        int sym_type = ELF64_ST_TYPE(sym.st_info);
        int sym_binding = ELF64_ST_BIND(sym.st_info);
        int is_global = (sym_binding == STB_GLOBAL);
        if (sym_binding == STB_WEAK)
            sym_char = is_global ? 'W' : 'w';
        if (sym.st_shndx == SHN_ABS) {
            sym_char = is_global ? 'A' : 'a';
        } else if (sym.st_shndx == SHN_COMMON) {
            sym_char = is_global ? 'C' : 'c';
        } else if (sym.st_shndx == SHN_UNDEF) {
            sym_char = 'U';
        } else { // Otherwise, we have to actually read the section.
            Elf64_Off sxn_offset = shoff + (sym.st_shndx * e_shentsize);
            uint32_t ref_to_section;
            if (lseek(elf_fd, sxn_offset, SEEK_SET) != sxn_offset) {
                perror("lseek to referred-to section");
                return EXIT_FAILURE;
            }
            rc = read(elf_fd, (void *)&ref_to_section, sizeof(ref_to_section));
            if (rc < 0) {
                perror("read referred-to section");
                return EXIT_FAILURE;
            }
            if (rc != sizeof(ref_to_section)) {
                puts("failed to read referred-to section type");
                return EXIT_FAILURE;
            }
            char* sxn_name = &sh_strs[ref_to_section];
            if (!strcmp(sxn_name, ".bss")) {
                sym_char = is_global ? 'B' : 'b';
            } else if (!strcmp(sxn_name, ".text")) {
                sym_char = is_global ? 'T' : 't';
            } else if (!strcmp(sxn_name, ".rodata")) {
                sym_char = is_global ? 'R' : 'r';
            } else if (!strcmp(sxn_name, ".data")) {
                sym_char = is_global ? 'D' : 'd';
            }
        }
        if (printf("%s %c %s\n", buf, sym_char, sym_name) < 0) {
            perror("print a symbol line");
            return EXIT_FAILURE;
        }

next_symbol:
        size_left -= shdr->sh_entsize ? shdr->sh_entsize : sizeof(sym);
    }

    return EXIT_SUCCESS;
}

static int main64 (int fd, char* path) {
    Elf64_Ehdr header;
    Elf64_Half e_shnum;
    Elf64_Phdr phdr;
    Elf64_Shdr shdr;

read_elf_header:
    if (read(fd, (char *)&header, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr))
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

    // Get a new independent FD to the ELF file.
    int fd2 = open(path, O_RDONLY);
    if (fd2 < 0) {
        perror("could not open()");
        return EXIT_FAILURE;
    }
    int fd3 = open(path, O_RDONLY);
    if (fd3 < 0) {
        perror("could not open()");
        return EXIT_FAILURE;
    }

calculate_shstrs_header_offset:
    ;
    Elf64_Off start_shstrs = header.e_shoff
        + ((Elf64_Off)header.e_shstrndx * (Elf64_Off)header.e_shentsize);
seek_to_shstrs_header_offset:
    if (lseek(fd2, start_shstrs, SEEK_SET) != start_shstrs)
        return EXIT_FAILURE;
read_shstrs_header:
    if (read(fd2, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr))
        return EXIT_FAILURE;
check_if_its_actually_section_header_strtab:
    if (shdr.sh_type != SHT_STRTAB)
        return EXIT_FAILURE;
seek_to_shstrs_section_body:
    if (lseek(fd2, shdr.sh_offset, SEEK_SET) != shdr.sh_offset)
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

read_section_header_strings:
    if (read(fd2, sh_strs, shdr.sh_size) != shdr.sh_size)
        return EXIT_FAILURE;
seek_back_to_section_headers_start:
    if (lseek(fd, header.e_shoff, SEEK_SET) != header.e_shoff)
        return EXIT_FAILURE;

    e_shnum = header.e_shnum;
    while (e_shnum--) {
read_a_section_header:
        if (read(fd, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr))
            return EXIT_FAILURE;
ignore_empty_headers:
        if (shdr.sh_size == 0)
            continue;
ignore_non_symbol_headers:
        if (shdr.sh_type != SHT_SYMTAB && shdr.sh_type != SHT_DYNSYM)
            continue;
seek_to_start_of_symbol_section:
        if (lseek(fd2, shdr.sh_offset, SEEK_SET) != shdr.sh_offset)
            return EXIT_FAILURE;
        if (handle_header64(&shdr, fd2, fd3, header.e_shoff, header.e_shentsize) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }

    putchar('\n');
    return EXIT_SUCCESS;
}

static int main32 (int fd) {
    putchar('\n');
    return EXIT_FAILURE;
}

int main (int argc, char **argv) {
    int fd, is_32bit, is_64bit;
    char e_ident[EI_NIDENT];

    if (argc < 2) {
        puts(USAGE_MSG);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd <= 0) {
        errno = ENOENT;
        perror("readelf @ main");
        return EXIT_FAILURE;
    }
    if (read(fd, e_ident, EI_NIDENT) != EI_NIDENT) {
        errno = ENOEXEC;
        perror("readelf @ main");
        return EXIT_FAILURE;
    }
    if (lseek(fd, 0, SEEK_SET) != 0) {
        return EXIT_FAILURE;
    }
    is_32bit = e_ident[EI_CLASS] == ELFCLASS32;
    is_64bit = e_ident[EI_CLASS] == ELFCLASS64;
    if (!is_32bit && !is_64bit) {
        puts(CRAZY_WORD_SIZE_MSG);
        return EXIT_FAILURE;
    }
    else if (is_64bit) {
        return main64(fd, argv[1]);
    }
    else {
        return main32(fd);
    }
}