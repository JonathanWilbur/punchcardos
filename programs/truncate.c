#ifndef NOLIBC
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#else

int truncate(const char *path, off_t length) {
    return __sysret(my_syscall2(__NR_truncate, path, length));
}

#endif

#define PARSE_SIZE_FAIL     -1

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static long parse_size (const char* cp) {
    long value;
    if (!isdigit(*cp))
        return PARSE_SIZE_FAIL;

    value = 0;
    while (isdigit(*cp))
        value = value * 10 + *cp++ - '0';
    switch (*cp++) {
        case 'G':
            value *= (1024 * 1024 * 1024);
            break;
        case 'M':
            value *= (1024 * 1024);
            break;
        
        case 'K':
            value *= 1024;
            break;

        case 'b':
            value *= 512;
            break;

        case 'w':
            value *= 2;
            break;

        case '\0':
            return value;

        default:
            return PARSE_SIZE_FAIL;
    }
    if (*cp)
        return PARSE_SIZE_FAIL;
    return value;
}

static int do_truncate (const char *file, char op, off_t size, int no_create, int io_blocks) {
    struct stat st;
    off_t current_size;
    off_t modulus;
    int new_fd;

    if ((stat(file, &st) < 0) && (no_create || (errno != ENOENT))) {
        perror(file);
        return EXIT_FAILURE;
    }

    if (errno == ENOENT) {
        // This is equivalent to calling creat().
        new_fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (new_fd < 0) {
            perror(file);
            return EXIT_FAILURE;
        }
        close(new_fd);
        current_size = 0;
    } else {
        current_size = st.st_size;
    }

    // TODO: Is this right?
    if (io_blocks)
        size *= st.st_blksize;

    switch (op) {
        case '=': break;
        case '+':
            size += current_size;
            break;
        case '-':
            size = size > current_size ? 0 : (current_size - size);
            break;
        case '<': // "at most"
            size = current_size < size ? current_size : size;
            break;
        case '>': // "at least"
            size = current_size > size ? current_size : size;
            break;
        case '/':
            modulus = size;
            if (current_size % modulus)
                size = current_size - (current_size % modulus);
            break;
        case '%':
            modulus = size;
            if (current_size % modulus)
                size = current_size + (modulus - (current_size % modulus));
            break;
        default: {
            printf("Unsupported operand: %c\n", op);
            return EXIT_FAILURE;
        }
    }
    return (truncate(file, size) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

const char *USAGE = "truncate [-c|-o] files...";
static void print_usage () {
    puts(USAGE);
}

#define EXPECT_OPT_NONE     0
#define EXPECT_OPT_SIZE     1
#define EXPECT_OPT_REF      2

int main (int argc, char **argv) {
    char *arg;
    int no_create = 0;
    int io_blocks = 0;
    long size = 0;
    char op = '=';
    struct stat ref_st;
    int expect_opt = EXPECT_OPT_NONE;

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;

        if (!strcmp(arg, "--help") || !strcmp(arg, "-h") || !strcmp(arg, "-?")) {
            print_usage();
            return EXIT_SUCCESS;
        }
        else if (expect_opt == EXPECT_OPT_SIZE) {
            if (!isdigit(arg[0])) {
                op = arg[0];
                arg++;
            }
            size = parse_size(arg);
            if (size == PARSE_SIZE_FAIL)
                return EXIT_FAILURE;
            expect_opt = EXPECT_OPT_NONE;
            continue;
        }
        else if (expect_opt == EXPECT_OPT_REF) {
            if (stat(arg, &ref_st) < 0) {
                perror(arg);
                return EXIT_FAILURE;
            }
            size = ref_st.st_size;
            expect_opt = EXPECT_OPT_NONE;
            continue;
        }
        else if (!strcmp(arg, "--no-create") || !strcmp(arg, "-c")) {
            no_create = 1;
            continue;
        }
        else if (!strcmp(arg, "--io-blocks") || !strcmp(arg, "-o")) {
            io_blocks = 1;
            continue;
        }
        else if (!strcmp(arg, "-s")) {
            expect_opt = EXPECT_OPT_SIZE;
            continue;
        }
        else if (!strcmp(arg, "-r")) {
            expect_opt = EXPECT_OPT_REF;
            continue;
        }
        else if (prefix("--size=", arg)) {
            arg = &arg[7];
            if (!isdigit(arg[0])) {
                op = arg[0];
                arg++;
            }
            size = parse_size(arg);
            if (size == PARSE_SIZE_FAIL)
                return EXIT_FAILURE;
            continue;
        }
        else if (prefix("--reference=", arg)) {
            arg = &arg[12];
            if (stat(arg, &ref_st) < 0) {
                perror(arg);
                return EXIT_FAILURE;
            }
            size = ref_st.st_size;
            continue;
        }
        else if (prefix("-", arg)) {
            printf("Argument not understood: %s\n", arg);
            return EXIT_FAILURE;
        }
        else if (do_truncate(arg, op, size, no_create, io_blocks) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
