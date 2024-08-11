/* Minimal hexdump implementation by Jonathan M. Wilbur <jonathan@wilbur.space>.
This was written in such a way to maximize the number of functions so that the
compiled ELF binary is as auditable / understandable / inspectable as possible
(assuming symbols are included).
*/
#ifndef NOLIBC
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

int atoi (const char* s) {
    int ret = 0;
    for (int i = 0; i < strlen(s); i++) {
        char c = s[i];
        if (c < '0' || c > '9') {
            break;
        }
        ret = (ret * 10) + (c - '0');
    }
    return ret;
}

#else
#define BUFSIZ 8192
#endif

static char to_hex_char (unsigned char byte) {
    if (byte < 10) {
        return byte + '0';
    } else {
        return (byte - 10) + 'A';
    }
}

static void offset_to_hex (int* b, char* out, uint32_t offset) {
    /* No, this isn't a programming war crime at all! What are you talking about? */
    out[(*b)++] = to_hex_char((offset & 0xF0000000) >> 28);
    out[(*b)++] = to_hex_char((offset & 0x0F000000) >> 24);
    out[(*b)++] = to_hex_char((offset & 0x00F00000) >> 20);
    out[(*b)++] = to_hex_char((offset & 0x000F0000) >> 16);
    out[(*b)++] = to_hex_char((offset & 0x0000F000) >> 12);
    out[(*b)++] = to_hex_char((offset & 0x00000F00) >> 8);
    out[(*b)++] = to_hex_char((offset & 0x000000F0) >> 4);
    out[(*b)++] = to_hex_char((offset & 0x0000000F));
    out[(*b)++] = ' ';
    return;
}

static void print_ascii_column (int* b, char* out, unsigned char* bytes, size_t count) {
    out[(*b)++] = '|';
    for (int i = 0; i < count; i++) {
        if (isprint(bytes[i])) {
            out[(*b)++] = bytes[i];
        } else {
            out[(*b)++] = '.';
        }
    }
    out[(*b)++] = '|';
    return;
}

static int print_hexdump_line (
    unsigned char* bytes,
    size_t count,
    size_t offset,
    int outfd
) {
    char buf[100];
    int b = 2;
    buf[0] = '0';
    buf[1] = 'x';
    offset_to_hex(&b, buf, (uint32_t)offset);

    for (int i = 0; i < count; i++) {
        buf[b++] = to_hex_char((bytes[i] & '\xF0') >> 4);
        buf[b++] = to_hex_char(bytes[i] & '\x0F');
        buf[b++] = ' ';
    }
    /* Pad with spaces to the ASCII column is aligned even when there is a row
    not containing LINE_SIZE bytes. */
    while (b <= 60)
        buf[b++] = ' ';
    print_ascii_column(&b, buf, bytes, count);
    buf[b++] = '\n';
    write(outfd, buf, b);
    return EXIT_SUCCESS;
}

#define LINE_SIZE   16

static int hexdump (int infd, int outfd, size_t start, size_t numbytes) {
    char buf[BUFSIZ];
    char line_buf[LINE_SIZE];
    ssize_t bytes_read;
    size_t bytes_written = 0;
    int line_buf_size = 0;

    if (lseek(infd, start, SEEK_SET) != start) {
        return EXIT_FAILURE;
    }

    while ((bytes_read = read(infd, buf, BUFSIZ)) > 0) {
        for (int i = 0; i < bytes_read && (bytes_written < numbytes); i++) {
            line_buf[line_buf_size++] = buf[i];
            bytes_written++;
            if (line_buf_size == LINE_SIZE) {
                if (print_hexdump_line(
                    (unsigned char *)&line_buf,
                    LINE_SIZE,
                    start + bytes_written,
                    outfd) != EXIT_SUCCESS) {
                    return EXIT_FAILURE;
                }
                line_buf_size = 0;
            }
        }
    }

    if (bytes_read < 0) {
        perror("failed to read");
        return EXIT_FAILURE;
    }
    if (print_hexdump_line(
        (unsigned char *)&line_buf,
        line_buf_size,
        start + bytes_written,
        outfd) != EXIT_SUCCESS)
            return EXIT_FAILURE;

    close(outfd);

    return EXIT_SUCCESS;
}

int main (int argc, char** argv) {
    int infd = STDIN_FILENO;
    int outfd = STDOUT_FILENO;
    size_t start = 0;
    size_t numbytes = SIZE_MAX;

    for (int i = 1; i < argc; i++) {
        if (
            (argv[i][0] == '-')
            && (strlen(argv[i]) == 2)
            && (i < argc - 1)
        ) {
            switch(argv[i][1]) {
                case 's':
                    start = atoi(argv[i + 1]);
                    i++;
                    break;

                case 'n':
                    numbytes = atoi(argv[i + 1]);
                    i++;
                    break;
                          
                case 'o':
                    outfd = open(argv[i + 1], O_WRONLY | O_CREAT, 0600);
                    if (outfd < 0) {
                        perror("hexdump output file");
                        return EXIT_FAILURE;
                    }
                    i++;
                    break;
                default:
                    printf("Argument -%c not understood.", argv[i][1]);
                    return EXIT_FAILURE;
            }
            continue;
        }

        infd = open(argv[i], O_RDONLY);
        if (infd < 0) {
            perror("hexdump input file");
            return EXIT_FAILURE;
        }
    }

    return hexdump(infd, outfd, start, numbytes);
}