/* Minimal readelf implementation. */
#include "elf.h"
#ifndef NOLIBC
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#ifdef __GLIBC__
#include <stdio.h>

// WARNING: This is non-reentrant.
char* itoa (long long in) {
    static char buf[100];
    sprintf(buf, "%lld", in);
    return &buf[0];
}

#endif

#endif

// FIXME: I don't think this handles endianness correctly...

const char* USAGE_MSG = "readelf <file>";
const char* CRAZY_WORD_SIZE_MSG = "ELF File is neither 32-bit or 64-bit.";
const char* WEIRD_OVERLAP = "Weird (maybe not invalid) ELF file. Not supported.";
const char* WEIRD_HEADER_SIZE = "Unsupported program or section header size.";

char* sh_strs = NULL;

// Function to reverse an array of arbitrary bytes
void memrev(char *array, size_t length) {
    size_t start = 0;
    size_t end = length - 1;
    unsigned char temp;

    while (start < end) {
        // Swap the elements
        temp = array[start];
        array[start] = array[end];
        array[end] = temp;

        // Move towards the middle
        start++;
        end--;
    }
}

static char to_hex_char (unsigned char byte) {
    if (byte < 10) {
        return byte + '0';
    } else {
        return (byte - 10) + 'A';
    }
}

static int print_hexdump_line (unsigned char* bytes, size_t count, size_t indent) {
    char buf[80];
    int b = 0;
    int indent2 = indent;

    while (indent2--)
        buf[b++] = ' ';
    for (int i = 0; i < count; i++) {
        buf[b++] = to_hex_char((bytes[i] & '\xF0') >> 4);
        buf[b++] = to_hex_char(bytes[i] & '\x0F');
        buf[b++] = ' ';
    }
    // FIXME: Somehow an aberrant character is making its way in.
    while (b++ < 47 + indent)
        buf[b] = ' ';
    buf[b++] = '|';
    for (int i = 0; i < count; i++) {
        if (isprint(bytes[i])) {
            buf[b++] = bytes[i];
        } else {
            buf[b++] = '.';
        }
    }
    buf[b++] = '|';
    buf[b++] = '\n';
    write(STDOUT_FILENO, buf, b);
    return EXIT_SUCCESS;
}

static int print_hexdump (char *bytes, size_t count, size_t indent) {
    size_t line_size;
    for (size_t i = 0; i < count; i += 16) {
        line_size = ((count - i) >= 16) ? 16 : count - i;
        if (print_hexdump_line(&bytes[i], line_size, indent) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// TODO: Elf64_Addr	e_entry;		/* Entry point virtual address */
// TODO: Elf64_Word	e_flags;		/* Processor-specific flags */
static int print_ehdr64 (Elf64_Ehdr header) {
    if (header.e_ident[EI_VERSION] != 1 || header.e_version != 1) {
        // No other version is defined.
        return EXIT_FAILURE;
    }

    if (header.e_ident[EI_DATA] == ELFDATA2LSB) {
        fputs("ELF64LEv1@", stdout);
    }
    else if (header.e_ident[EI_DATA] == ELFDATA2MSB) {
        fputs("ELF64BEv1@", stdout);
    } 
    else {
        return EXIT_FAILURE;
    }
    
    if (header.e_ident[EI_OSABI] == ELFOSABI_NONE) {
        fputs("SysVv", stdout);
    } else {
        fputs("?v", stdout);
    }

    fputs(itoa(header.e_ident[EI_ABIVERSION]), stdout);
    fputs(": ", stdout);
    switch (header.e_type) {
    case ET_NONE:
        fputs("NONE", stdout);
        break;
    case ET_REL:
        fputs("REL", stdout);
        break;
    case ET_EXEC:
        fputs("EXEC", stdout);
        break;
    case ET_DYN:
        fputs("DYN", stdout);
        break;
    case ET_CORE:
        fputs("CORE", stdout);
        break;
    case ET_NUM:
        fputs("NUM", stdout);
        break;
    case ET_LOOS:
        fputs("LOOS", stdout);
        break;
    case ET_HIOS:
        fputs("HIOS", stdout);
        break;
    case ET_LOPROC:
        fputs("LOPROC", stdout);
        break;
    case ET_HIPROC:
        fputs("HIPROC", stdout);
        break;
    default: break;
    }

    switch (header.e_machine) {
        case EM_386:
            fputs(" x86", stdout);
            break;
        case EM_X86_64:
            fputs(" x86-64", stdout);
            break;
        default: break;
    }

    putchar('\n');

    return EXIT_SUCCESS;
}

static int print_phdr64 (Elf64_Phdr header) {
    fputs("  PH: ", stdout);
    switch (header.p_type) {
    case PT_NULL:
        fputs("NULL", stdout);
        break;
    case PT_LOAD:
        fputs("LOAD", stdout);
        break;
    case PT_DYNAMIC:
        fputs("DYNAMIC", stdout);
        break;
    case PT_INTERP:
        fputs("INTERP", stdout);
        break;
    case PT_NOTE:
        fputs("NOTE", stdout);
        break;
    case PT_SHLIB:
        fputs("SHLIB", stdout);
        break;
    case PT_PHDR:
        fputs("PHDR", stdout);
        break;
    case PT_TLS:
        fputs("TLS", stdout);
        break;
    case PT_NUM:
        fputs("NUM", stdout);
        break;
    case PT_LOOS:
        fputs("LOOS", stdout);
        break;
    case PT_GNU_EH_FRAME:
        fputs("GNU_EH_FRAME", stdout);
        break;
    case PT_GNU_STACK:
        fputs("GNU_STACK", stdout);
        break;
    case PT_GNU_RELRO:
        fputs("GNU_RELRO", stdout);
        break;
    case PT_GNU_PROPERTY:
        fputs("GNU_PROPERTY", stdout);
        break;
    case PT_LOSUNW:
        fputs("LOSUNW", stdout);
        break;
    case PT_SUNWSTACK:
        fputs("SUNWSTACK", stdout);
        break;
    case PT_HIOS:
        fputs("HIOS", stdout);
        break;
    case PT_LOPROC:
        fputs("LOPROC", stdout);
        break;
    case PT_HIPROC:
        fputs("HIPROC", stdout);
        break;
    default: fputs(itoa(header.p_type), stdout);
    }
    putchar(' ');
    putchar('[');
    /* If the section header flags use any OS or processor-specific flags,
    just display the whole thing as an integer. */
    if (header.p_flags & (PF_MASKOS | PF_MASKPROC)) {
        printf("0x%X", header.p_flags);
    } else {
        if (header.p_flags & PF_X)
            putchar('X');
        if (header.p_flags & PF_W)
            putchar('W');
        if (header.p_flags & PF_R)
            putchar('R');
    }
    putchar(']');
    printf(" phys=0x%lX virt=0x%lX", header.p_paddr, header.p_vaddr);
    printf(" memory=%ld align=%ld", header.p_memsz, header.p_align);
    // TODO: Print segment contents.
    // Elf64_Off	    p_offset;   /* Segment file offset */
    // Elf64_Xword	    p_filesz;   /* Segment size in file */

    putchar('\n');
    return EXIT_SUCCESS;
}

static int print_shdr64 (Elf64_Shdr header) {
    char* name = header.sh_name ? &sh_strs[header.sh_name] : "";
    printf("  SH: \"%s\" ", name);
    switch (header.sh_type) {
        case SHT_NULL:
            fputs("NULL", stdout);
            break;
        case SHT_PROGBITS:
            fputs("PROGBITS", stdout);
            break;
        case SHT_SYMTAB:
            fputs("SYMTAB", stdout);
            break;
        case SHT_STRTAB:
            fputs("STRTAB", stdout);
            break;
        case SHT_RELA:
            fputs("RELA", stdout);
            break;
        case SHT_HASH:
            fputs("HASH", stdout);
            break;
        case SHT_DYNAMIC:
            fputs("DYNAMIC", stdout);
            break;
        case SHT_NOTE:
            fputs("NOTE", stdout);
            break;
        case SHT_NOBITS:
            fputs("NOBITS", stdout);
            break;
        case SHT_REL:
            fputs("REL", stdout);
            break;
        case SHT_SHLIB:
            fputs("SHLIB", stdout);
            break;
        case SHT_DYNSYM:
            fputs("DYNSYM", stdout);
            break;
        case SHT_INIT_ARRAY:
            fputs("INIT_ARRAY", stdout);
            break;
        case SHT_FINI_ARRAY:
            fputs("FINI_ARRAY", stdout);
            break;
        case SHT_PREINIT_ARRAY:
            fputs("PREINIT_ARRAY", stdout);
            break;
        case SHT_GROUP:
            fputs("GROUP", stdout);
            break;
        case SHT_SYMTAB_SHNDX:
            fputs("SYMTAB_SHNDX", stdout);
            break;
        case SHT_RELR:
            fputs("RELR", stdout);
            break;
        case SHT_NUM:
            fputs("NUM", stdout);
            break;
        case SHT_LOOS:
            fputs("LOOS", stdout);
            break;
        case SHT_GNU_ATTRIBUTES:
            fputs("GNU_ATTRIBUTES", stdout);
            break;
        case SHT_GNU_HASH:
            fputs("GNU_HASH", stdout);
            break;
        case SHT_GNU_LIBLIST:
            fputs("GNU_LIBLIST", stdout);
            break;
        case SHT_CHECKSUM:
            fputs("CHECKSUM", stdout);
            break;
        case SHT_LOSUNW:
            fputs("LOSUNW", stdout);
            break;
        // case SHT_SUNW_move:
        //     fputs("SUNW_move", stdout);
        //     break;
        case SHT_SUNW_COMDAT:
            fputs("SUNW_COMDAT", stdout);
            break;
        case SHT_SUNW_syminfo:
            fputs("SUNW_syminfo", stdout);
            break;
        case SHT_GNU_verdef:
            fputs("GNU_verdef", stdout);
            break;
        case SHT_GNU_verneed:
            fputs("GNU_verneed", stdout);
            break;
        // case SHT_GNU_versym:
        //     fputs("GNU_versym", stdout);
        //     break;
        // case SHT_HISUNW:
        //     fputs("HISUNW", stdout);
        //     break;
        case SHT_HIOS:
            fputs("HIOS", stdout);
            break;
        case SHT_LOPROC:
            fputs("LOPROC", stdout);
            break;
        case SHT_HIPROC:
            fputs("HIPROC", stdout);
            break;
        case SHT_LOUSER:
            fputs("LOUSER", stdout);
            break;
        case SHT_HIUSER:
            fputs("HIUSER", stdout);
            break;
        default: fputs(itoa(header.sh_type), stdout);
    }

    putchar(' ');
    putchar('[');
    // Unrecognized flags? Print as hex.
    if (header.sh_flags & (SHF_MASKOS | SHF_MASKPROC)) {
        printf("0x%lX", header.sh_flags);
    } else {
        if (header.sh_flags & SHF_WRITE)
            putchar('W');
        if (header.sh_flags & SHF_ALLOC)
            putchar('A');
        if (header.sh_flags & SHF_EXECINSTR)
            putchar('X');
        if (header.sh_flags & SHF_MERGE)
            putchar('M');
        if (header.sh_flags & SHF_STRINGS)
            putchar('S');
        if (header.sh_flags & SHF_INFO_LINK)
            putchar('I');
        if (header.sh_flags & SHF_LINK_ORDER)
            putchar('L');
        if (header.sh_flags & SHF_OS_NONCONFORMING)
            putchar('Z');
        if (header.sh_flags & SHF_GROUP)
            putchar('G');
        if (header.sh_flags & SHF_TLS)
            putchar('T');
        if (header.sh_flags & SHF_COMPRESSED)
            putchar('C');
        if (header.sh_flags & SHF_GNU_RETAIN)
            putchar('R');
        if (header.sh_flags & SHF_ORDERED)
            putchar('O');
        if (header.sh_flags & SHF_EXCLUDE)
            putchar('E');
    }
    putchar(']');

    printf(" addr=0x%lX align=0x%lX link=0x%X info=0x%X entsize=%lu",
        header.sh_addr,
        header.sh_addralign,
        header.sh_link,
        header.sh_info,
        header.sh_entsize
    );

    putchar('\n');
    return EXIT_SUCCESS;
}

// TODO: Validate e_ehsize against actual ELF header.
static int main64 (int fd, char* path) {
    Elf64_Ehdr header;
    Elf32_Half e_phnum;
    Elf32_Half e_shnum;
    Elf64_Phdr phdr;
    Elf64_Shdr shdr;
    char* section_bytes;

    if (read(fd, (char *)&header, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)) {
        return EXIT_FAILURE;
    }
    if (header.e_ehsize != 64) {
        return EXIT_FAILURE;
    }
    if (print_ehdr64(header) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    // Very unusual file layout where the program headers or section headers
    // overlap with the ELF header.
    if ((header.e_phoff < 0x40) || (header.e_shoff < 0x40)) {
        puts(WEIRD_OVERLAP);
        return EXIT_FAILURE;
    }

    // Program or section headers are too small to fill in our structs.
    if (
        (header.e_phentsize < sizeof(Elf64_Phdr))
        || (header.e_shentsize < sizeof(Elf64_Shdr))
    ) {
        puts(WEIRD_HEADER_SIZE);
        return EXIT_FAILURE;
    }

    e_phnum = header.e_phnum;
    if (lseek(fd, header.e_phoff, SEEK_SET) != header.e_phoff) {
        return EXIT_FAILURE;
    }
    while (e_phnum--) {
        if (read(fd, (char *)&phdr, sizeof(Elf64_Phdr)) != sizeof(Elf64_Phdr)) {
            return EXIT_FAILURE;
        }
        if (print_phdr64(phdr) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        // TODO: Print segment data bytes.
    }

    // Get a new independent FD to the ELF file.
    int fd2 = open(path, O_RDONLY);
    if (fd2 < 0) {
        perror("could not open()");
        return EXIT_FAILURE;
    }

    // Define the start of the strings section header.
    Elf64_Off start_shstrs = header.e_shoff
        + ((Elf64_Off)header.e_shstrndx * (Elf64_Off)header.e_shentsize);
    // Seek to the start of the strings section header.
    if (lseek(fd2, start_shstrs, SEEK_SET) != start_shstrs) {
        return EXIT_FAILURE;
    }
    // Read the section header.
    if (read(fd2, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr)) {
        return EXIT_FAILURE;
    }
    // TODO: Assert that the header is of type SHT_STRTAB?
    if (lseek(fd2, shdr.sh_offset, SEEK_SET) != shdr.sh_offset) {
        return EXIT_FAILURE;
    }
    // Allocate room for the section header strings.
    sh_strs = malloc(shdr.sh_size + 1);
    if (sh_strs == NULL) {
        errno = ENOMEM;
        perror("could not store section header strings table");
        return EXIT_FAILURE;
    }
    /* Set the last byte of the section header strings to NULL, just in case it
    is malformed and has no terminator. */
    sh_strs[shdr.sh_size] = 0;
    if (read(fd2, sh_strs, shdr.sh_size) != shdr.sh_size) {
        return EXIT_FAILURE;
    }

    // TODO: Bail if 0
    e_shnum = header.e_shnum;
    if (lseek(fd, header.e_shoff, SEEK_SET) != header.e_shoff) {
        return EXIT_FAILURE;
    }

    while (e_shnum--) {
        if (read(fd, (char *)&shdr, sizeof(Elf64_Shdr)) != sizeof(Elf64_Shdr)) {
            return EXIT_FAILURE;
        }
        if (print_shdr64(shdr) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        if (shdr.sh_size == 0)
            continue;
        if (lseek(fd2, shdr.sh_offset, SEEK_SET) != shdr.sh_offset) {
            return EXIT_FAILURE;
        }
        section_bytes = malloc(shdr.sh_size);
        if (section_bytes == NULL) {
            errno = ENOMEM;
            perror("could not allocate space for section data");
            return EXIT_FAILURE;
        }

        if (read(fd2, section_bytes, shdr.sh_size) != shdr.sh_size) {
            return EXIT_FAILURE;
        }
        print_hexdump(section_bytes, shdr.sh_size, 4);
        free(section_bytes);
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