#ifndef NOLIBC
// This means "yes, I want access to non-portable features of glibc"
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <dirent.h>

struct linux_dirent64 {
    ino64_t        d_ino;    /* 64-bit inode number */
    off64_t        d_off;    /* Not an offset; see getdents() */
    unsigned short d_reclen; /* Size of this dirent */
    unsigned char  d_type;   /* File type */
    char           d_name[]; /* Filename (null-terminated) */
};

#endif

#define DIRENT_BUFSIZ   32 * sizeof(struct linux_dirent64)

const char *USAGE_MSG = "Usage: ls [-l|-a] [path]";

// TODO: Different exit failure codes.

int ls (const char *dir, int op_a, int op_l)
{
    int fd;
    char dirbuf[DIRENT_BUFSIZ];
    size_t bytes_read;
    struct linux_dirent64 *d;

    fd = open(dir, O_RDONLY);
    if (fd == -1)
        return EXIT_FAILURE;
    while (1) {
        d = (struct linux_dirent64 *)&dirbuf;
        bytes_read = getdents64(fd, d, DIRENT_BUFSIZ);
        if (bytes_read <= 0) { // 0 means we're done, negative means failure.
            break;
        }
        for (size_t byte_pos = 0; byte_pos < bytes_read; ) {
            d = (struct linux_dirent64 *) (dirbuf + byte_pos);
            byte_pos += d->d_reclen;
            if (!op_a && d->d_name[0] == '.') {
                continue;
            }
            // TODO: directory highlighting?
            if (printf("%s%s", d->d_name, op_l ? "\n" : "  ") < 0)
                return EXIT_FAILURE;
        }
    }
    if (bytes_read < 0) {
        return EXIT_FAILURE;
    }
    if (!op_l) {
        puts(""); // Intentionally ignore error code.
    }
    // assert(rc == 0);
    return EXIT_SUCCESS;
}

int main (int argc, const char *argv[])
{
    int op_a = 0, op_l = 0;
    char *dir = ".";

	if (argc == 1) {
		return ls(".", 0, 0);
	}
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            if (puts(USAGE_MSG) == EOF)
                return EXIT_FAILURE;
            return EXIT_SUCCESS;
        }
        if (argv[i][0] == '-') {
            char *p = (char*)(argv[1] + 1);
            while (*p) {
                if (*p == 'a') {
                    op_a = 1;
                }
                else if (*p == 'l') {
                    op_l = 1;
                }
                else {
                    perror("Unrecognized option");
                    return EXIT_FAILURE;
                }
                p++;
            }
        } else {
            dir = (char *)argv[i];
        }
    }
    return ls(dir, op_a, op_l);
}