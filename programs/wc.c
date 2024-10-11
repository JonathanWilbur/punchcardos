#ifndef NOLIBC
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#else

#define BUFSIZ 8192

#endif

#ifdef __GLIBC__

static char itoa_buffer[21];

// Intentionally non-reentrant, like nolibc's implementation, specifically so
// we do not have to free memory each time we stringify an integer.
char *itoa(long in)
{
    snprintf(itoa_buffer, sizeof(itoa_buffer), "%ld", in);
    return itoa_buffer;
}

#endif

#define WC_TYPE_WORDS   0
#define WC_TYPE_BYTES   1
#define WC_TYPE_LINES   2
#define WC_TYPE_CHARS   3

static ssize_t wc (int type, char *filename, int print) {
    ssize_t count = 0;
    ssize_t rc = 0;
    char buf[BUFSIZ];
    int fd;
    
    fd = !strcmp(filename, "-")
        ? STDIN_FILENO
        : open(filename, O_RDONLY);

    if (fd < 0) {
        perror(filename);
        return -1;
    }

    while ((rc = read(fd, &buf, BUFSIZ)) != 0) {
        if (rc < 0) {
            perror("read");
            return -1;
        }
        if (type == WC_TYPE_BYTES) {
            count += rc;
        }
        else if (type == WC_TYPE_LINES) {
            for (int i = 0; i < rc; i++) {
                if (buf[i] == '\n')
                    count++;
            }
        }
        else if (type == WC_TYPE_WORDS) {
            for (int i = 0; i < rc; i++) {
                if (isspace(buf[i]) && buf[i] != '\n')
                    count++;
            }
        }
        else if (type == WC_TYPE_CHARS) {
            for (int i = 0; i < rc; i++) {
                if ((buf[i] & 0xC0) != 0x80) // Skip continuation bytes
                     count++;
            }
        }
        else {
            return -1;
        }
    }
    if (print && printf("%ld %s\n", count, filename) <= 0) {
        perror("printf");
        return -1;
    }

    close(fd);
    return count;
}

const char *USAGE_MSG = "wc [OPTION]... [FILE]...";
static void print_usage () {
    puts(USAGE_MSG);
}

/*
-c, --bytes             print the byte counts
-m, --chars             print the character counts
-l, --lines             print the newline counts
--files0-from=F         read input from the files specified by NUL-terminated names in file F; If F is - then read names from standard input
-L, --max-line-length   print the maximum display width
-w, --words             print the word counts
--help                  display this help and exit
--version               output version information and exit
*/
int main (int argc, char **argv) {
    char *arg;
    int type = WC_TYPE_WORDS;
    int i = 0;
    ssize_t count = 0;
    ssize_t rc;

    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;
        if (!strcmp(arg, "--bytes") || !strcmp(arg, "-c")) {
            type = WC_TYPE_BYTES;
            continue;
        }
        if (!strcmp(arg, "--chars") || !strcmp(arg, "-m")) {
            type = WC_TYPE_CHARS;
            continue;
        }
        if (!strcmp(arg, "--lines") || !strcmp(arg, "-l")) {
            type = WC_TYPE_LINES;
            continue;
        }
        if (!strcmp(arg, "--words") || !strcmp(arg, "-w")) {
            type = WC_TYPE_WORDS;
            continue;
        }
        if (arg[0] == '-' && strlen(arg) > 1) {
            printf("Option not understood: %s\n", arg);
            print_usage();
            return EXIT_FAILURE;
        }
        break;
    }

    if (i == argc) {
        arg = argv[i];
        rc = wc(type, "-", 0);
        if (rc < 0)
            return EXIT_FAILURE;
        if (puts(itoa(rc)) <= 0) {
            perror("puts");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    int files = argc - i;
    for (; i < argc; i++) {
        arg = argv[i];
        rc = wc(type, arg, 1);
        if (rc < 0)
            return EXIT_FAILURE;
        count += rc;
    }

    /* Total is only displayed if there are more than one file. */
    if (files > 1 && printf("%ld %s\n", count, "total") <= 0) {
        perror("printf");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
