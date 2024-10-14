#ifndef NOLIBC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

const char *USAGE_MSG = "Usage: printf <format-spec> [args...]";
static int print_usage () {
    puts(USAGE_MSG);
}

// Technically, all you need to support for the Linux kernel build is %s.
// I don't think it even uses any escape sequences.
// TODO: Support escape sequences
int printf_arg(char *fmt, char *arg) {
    char c;
    int in_format_sequence = 0;
    int chars_consumed = 0;
    char *orig_fmt = fmt;

    c = *fmt;
    while (c != '\0') {
        c = *fmt;
        if (!in_format_sequence) {
            if (c == '%') {
                in_format_sequence = 1;
            } else {
                if (write(STDOUT_FILENO, &c, 1) < 0) {
                    perror("write");
                    return -1;
                }
            }
            goto next_char;
        }
        if (c == 's') {
            if (arg == NULL)
                return -2;
            if (write(STDOUT_FILENO, arg, strlen(arg)) < 0) {
                perror("write");
                return -3;
            }
            return ((1 + fmt) - orig_fmt);
        } else {
            return -4;
        }

next_char:
        fmt++;
    }
    return 0;
}

// The Linux kernel build only uses %s
// However, it uses quite a lot of arguments.
// Curious usage: printf "fs/sysfs/%s " file.o dir.o symlink.o mount.o group.o
int main (int argc, char **argv) {
    char *fmt;
    char *fmtp;
    int rc;

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }
    fmt = argv[1];
    fmtp = fmt;

    for (int i = 2; i < argc; i++) {
        rc = printf_arg(fmtp, argv[i]);
        if (rc < 0)
            return EXIT_FAILURE;
        // Return 0 if the argument was unused and we have to wrap around.
        if (rc == 0) {
            fmtp = fmt;
            rc = printf_arg(fmtp, argv[i]);
            if (rc < 0)
                return EXIT_FAILURE;
            /* If the argument still could not be consumed after retrying, stop.
            Otherwise would cause an infinite loop.*/
            if (rc == 0)
                return EXIT_FAILURE;
        }
        fmtp += rc;
    }
    // Print the rest of the format string, if any.
    rc = printf_arg(fmtp, NULL);
    if (rc < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
