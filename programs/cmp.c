/* By Jonathan M. Wilbur. */
#ifndef NOLIBC
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

const char *USAGE_MSG = "Usage: cmp [-b|-v|-s|-v] <file1> [file2]";
static int print_usage () {
    puts(USAGE_MSG);
}

#define EXIT_SAME           0
#define EXIT_DIFFERENT      1
#define EXIT_TROUBLE        2

#define FLAG_PRINT_BYTES    1
#define FLAG_VERBOSE        (1 << 1)
#define FLAG_SILENT         (1 << 2)

static int cmp (char *file1, char *file2, int flags) {
    char c1;
    char c2;
    int rc;
    long line = 1;
    long byte = 1;
    int fd1, fd2;
#ifndef NO_STAT_SHORTCUT
    struct stat st1;
    struct stat st2;
#endif

set_up_fd1:
    fd1 = !strcmp(file2, "-")
        ? STDIN_FILENO
        : open(file1, O_RDONLY);
    if (fd1 < 0) {
        perror(file1);
        return EXIT_TROUBLE;
    }

set_up_fd2:
    fd2 = (file2 == NULL || !strcmp(file2, "-"))
        ? STDIN_FILENO
        : open(file2, O_RDONLY);
    if (fd2 < 0) {
        perror(file1);
        return EXIT_TROUBLE;
    }

#ifndef NO_STAT_SHORTCUT
do_stats:
    if (fd1 != STDIN_FILENO && fd2 != STDIN_FILENO) {
        if (stat(file1, &st1) < 0) {
            perror("stat file1");
            return EXIT_TROUBLE;
        }
        if (stat(file2, &st2) < 0) {
            perror("stat file2");
            return EXIT_TROUBLE;
        }
        if (st1.st_size != st2.st_size)
            goto differ;
    }
#endif

read_and_compare_loop:
    while ((rc = read(fd1, &c1, 1)) == 1) {
        byte++;
        if (rc == 0) {
            rc = read(fd2, &c2, 1);
            if (rc == 0) // We are at the end of both files.
                break;
            goto differ; // Otherwise, one file is larger than the other.
        }
        rc = read(fd2, &c2, 1);
        if (rc < 0)
            break;
        if (rc == 0 || c1 != c2)
            goto differ; // One file is larger than the other or different.
        if (c1 == '\n')
            line++;
    }
end_of_read_and_compare_loop:
    if (rc < 0) {
        perror("read");
        return EXIT_TROUBLE;
    }
    if (read(fd2, &c2, 1) == 1)
        goto differ; // There were still more characters in file2.
    return EXIT_SAME;

differ:
    if (!(flags & FLAG_SILENT)) {
        file1 = (!strcmp(file1, "-"))
            ? "<stdin>"
            : file1;
        file2 = (file2 == NULL || !strcmp(file2, "-"))
            ? "<stdin>"
            : file2;
        /* Little bit of a hack, but we check if c1 and c2 are non-zero to know
        if we ever even read a character. We can't print bytes if the files
        differ by size, because we found that out using stat(), not read(). */
        if ((flags & FLAG_PRINT_BYTES) && c1 && c2) {
            if (printf("%s %s differ: byte %ld, line %ld is %d %c %d %c\n",
                       file1, file2, byte, line, c1, c1, c2, c2) < 10) {
                perror("printf");
                return EXIT_TROUBLE;
            }
        } else {
            if (printf("%s %s differ: byte %ld, line %ld\n", file1, file2, byte, line) < 10) {
                perror("printf");
                return EXIT_TROUBLE;
            }
        }
    }
    return EXIT_DIFFERENT;
}

int main (int argc, char **argv) {
    int flags = 0;
    char *file1;
    char *file2;
    char *arg;
    size_t arglen;
    int i = 0;

parse_args:
    for (i = 1; i < argc - 1; i++) {
        arg = argv[i];
        arglen = strlen(arg);
        if (arglen == 0)
            continue;
parse_short_args:
        if (arglen >= 2 && arg[0] == '-' && arg[1] != '-') {
            for (int j = 1; j < arglen; j++) {
                switch (arg[j]) {
                case 'b':
                    flags |= FLAG_PRINT_BYTES;
                    break;
                case 'l': // This is not a mistake. -l is verbose.
                    flags |= FLAG_VERBOSE;
                    break;
                case 's':
                    flags |= FLAG_SILENT;
                    break;
                default: {
                    print_usage();
                    return EXIT_TROUBLE;
                }
                }
            }
            continue;
        }
parse_long_args:
        if (arglen < 2 || arg[0] != '-' || arg[1] != '-')
            break;
        if (!strcmp(arg, "--print-bytes")) {
            flags |= FLAG_PRINT_BYTES;
        }
        else if (!strcmp(arg, "--verbose")) {
            flags |= FLAG_VERBOSE;
        }
        else if (!strcmp(arg, "--silent") || !strcmp(arg, "--quiet")) {
            flags |= FLAG_SILENT;
        }
        else {
            printf("Argument not understood: %s\n", arg);
            return EXIT_TROUBLE;
        }
    }

    if (flags & FLAG_VERBOSE) {
        puts("--verbose not supported");
        return EXIT_TROUBLE;
    }

parse_file_names:
    file1 = argv[i++];
    if (i >= argc) {
        file2 = NULL;
    } else {
        file2 = argv[i];
    }

    return cmp(file1, file2, flags);
}