#ifndef NOLIBC
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#else
#define BUFSIZ 8192
#endif

const char* USAGE_MSG = "";
const char* VERSION = "1.0.0";

enum {
    Bytes,
    Lines,
};

/* This is a sloppy implementation that just attempts to allocate a buffer as
big as the entire read in one attempt. So be careful. */
int read_bytes (int fd, int bytes) {
    char* buf = malloc(bytes);
    if (!buf) {
        errno = ENOMEM;
        perror("head @ read_bytes");
        return EXIT_FAILURE;
    }
    size_t bytes_read = read(fd, buf, bytes);
    if (bytes_read < 0) {
        perror("head @ read_bytes");
        return EXIT_FAILURE;
    }
    if (write(STDIN_FILENO, buf, bytes) < -1) {
        perror("head @ read_bytes");
        return EXIT_FAILURE;
    }
    /* Note: this intentionally does not produce a newline at the end. */
    return EXIT_SUCCESS;
}

int read_lines (int fd, int lines, int zero_delim) {
    ssize_t n;
    int lines_remaining = lines;
    char delim = zero_delim ? '\0' : '\n';

    char buf[BUFSIZ];
    while ((n = read(fd, buf, BUFSIZ)) > 0) {
        for (int i = 0; i < n; i++) {
            if (buf[i] != delim) {
                continue;
            }
            lines_remaining--;
            if (lines_remaining != 0) {
                continue;
            }
            if (write(STDOUT_FILENO, buf, (size_t)i) != i) {
                perror("head @ read_lines");
                return EXIT_FAILURE;
            }
            puts(""); // This gives us the newline at the end
            return EXIT_SUCCESS;
        }

        if (write(STDOUT_FILENO, buf, (size_t)n) != n) {
            perror("head @ read_lines");
            return EXIT_FAILURE;
        }
    }
    puts(""); // This gives us the newline at the end
    return EXIT_SUCCESS;
}

int main (int argc, char **argv) {
    int next_arg = -1;
    int zero_delim = 0;
    int lines = 10;
    int bytes = 0;
    char* file_name = NULL;
    int fd = STDIN_FILENO;

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (next_arg == Bytes) {
            bytes = atoi(arg);
            next_arg = -1;
        }
        else if (next_arg == Lines) {
            lines = atoi(arg);
            next_arg = -1;
        }
        else if (strcmp(arg, "--bytes") == 0) {
            next_arg = Bytes;
        }
        else if (strcmp(arg, "-c") == 0) {
            next_arg = Bytes;
        }
        else if (strcmp(arg, "--lines") == 0) {
            next_arg = Lines;
        }
        else if (strcmp(arg, "-n") == 0) {
            next_arg = Lines;
        }
        else if (strcmp(arg, "--zero-delimited") == 0) {
            zero_delim = 1;
        }
        else if (strcmp(arg, "-z") == 0) {
            zero_delim = 1;
        }
        else if (strcmp(arg, "--version") == 0) {
            if (puts(VERSION) < 0)
                return EXIT_FAILURE;            
            return EXIT_SUCCESS;
        }
        else if (strlen(arg) > 0 && arg[0] != '-' && file_name == NULL) {
            file_name = arg;
        }
        else if (strlen(arg) == 1 && arg[0] != '-') {
            file_name = NULL; // Meaning: use standard input.
        }
        else {
            errno = EINVAL;
            perror("head @ main");
            puts(USAGE_MSG);
            return EXIT_FAILURE;
        }
    }

    if (file_name == NULL) {
        fd = STDIN_FILENO;
    } else {
        fd = open(file_name, O_RDONLY);
        if (fd <= 0) {
            errno = ENOENT;
            perror("head");
            return EXIT_FAILURE;
        }
    }

    if (bytes > 0) {
        return read_bytes(fd, bytes);
    }

    return read_lines(fd, lines, zero_delim);
}
