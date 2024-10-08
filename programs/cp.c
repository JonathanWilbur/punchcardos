/*
Copyright (C) 2024 by Jonathan M. Wilbur <jonathan@wilbur.space>.
Licensed under an MIT license.
*/
#ifndef NOLIBC
// This means "yes, I want access to non-portable features of glibc"
#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <syscall.h>

struct linux_dirent64 {
    ino64_t d_ino;           /* 64-bit inode number */
    off64_t d_off;           /* Not an offset; see getdents() */
    unsigned short d_reclen; /* Size of this dirent */
    unsigned char d_type;    /* File type */
    char d_name[];           /* Filename (null-terminated) */
};

#else

#define BUFSIZ 8192

char* strcat(char* dest, char* src) {
    char* ret = dest;
    while (*dest)
	    dest++;
    while (*dest++ = *src++);
    return ret;
}

// NOTE: loff_t was replaced by long int.
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    return __sysret(my_syscall3(__NR_readlink, pathname, buf, bufsiz));
}

ssize_t copy_file_range(int fd_in, long int *off_in,
                        int fd_out, long int *off_out,
                        size_t len, unsigned int flags) {
    return __sysret(my_syscall6(__NR_copy_file_range, fd_in, off_in, fd_out, off_out, len, flags));
}

#endif

#define bool    int
#define true    1
#define false   0

#define FLAG_RECURSE    (1)

#define INEXARGSZ   sizeof("--exclude=")

typedef struct options_st {
    char *srcs[64];
    char *dest;
    int flags;
} options;

static int copy_file (const char* src, const char *dest) {
    char buf[BUFSIZ];
    struct stat st;
    int rc = 1;
    int sfd, dfd;

    if (stat(src, &st) != EXIT_SUCCESS)
        return EXIT_FAILURE;
    sfd = open(src, O_RDONLY);
    if (sfd < 0) {
        perror(src);
        return EXIT_FAILURE;
    }
    dfd = open(dest, O_WRONLY | O_CREAT, st.st_mode);
    if (dfd < 0) {
        if (errno == EEXIST) { // If the file already exists, truncate it before writing.
            /* In this case, it could be possible that the existing file has an
            undesirable mode, but we will ignore this edge case for now. */
            dfd = open(dest, O_WRONLY | O_TRUNC);
            if (dfd < 0) {
                perror(dest);
                close(sfd);
                close(dfd);
                return EXIT_FAILURE;
            }
        } else {
            perror(dest);
            return EXIT_FAILURE;
        }
    }
    if (st.st_size > 0)
        rc = copy_file_range(sfd, 0, dfd, 0, UINT64_MAX, 0);
    close(sfd);
    close(dfd);
    return rc > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int is_ignored_entry(const char *name) {
    return (!strcmp(name, ".") || !strcmp(name, ".."));
}

static int iterate(const char *dir, const options *opts, int root_len);

static int handle_entry(
    const char *parent,
    const struct linux_dirent64 *ent,
    const options *opts,
    int root_len
) {
    char srcpath[PATH_MAX + 1];
    char destpath[PATH_MAX + 1];
    char relpath[PATH_MAX + 1];
    char linkpath[PATH_MAX + 1];
    char *compared_name = (char *)ent->d_name;
    ssize_t rc;
    struct stat st;

    strcpy(srcpath, parent);
    strcat(srcpath, "/");
    strcat(srcpath, (char *)ent->d_name);

    strncpy(relpath, srcpath + root_len + 1, strlen(srcpath) - root_len - 1);

    strcpy(destpath, opts->dest);
    strcat(destpath, "/");
    strcat(destpath, (char *)relpath);

    switch (ent->d_type) {
        case (DT_REG):
            return copy_file(srcpath, destpath);
        case (DT_DIR): {
            if (stat(srcpath, &st) != EXIT_SUCCESS)
                goto err;
            // We can just ignore the error if the directory already exists.
            // In theory, it might not have the same mode, but we'll ignore this
            // edge case for right now.
            if (mkdir(destpath, st.st_mode) && (errno != EEXIST))
                goto err;
            if (opts->flags & FLAG_RECURSE)
                return iterate(srcpath, opts, root_len);
            else
                return EXIT_SUCCESS;
        }
        case (DT_LNK):
            rc = readlink(srcpath, linkpath, sizeof(linkpath));
            if (rc < 0)
                goto err;
            linkpath[rc] = 0;
            if (symlink(linkpath, destpath) < 0)
                goto err;
            return EXIT_SUCCESS;
        default:
            break; // Everything else is ignored.
    }
    return EXIT_SUCCESS;

err:
    perror(srcpath);
    return EXIT_FAILURE;
}

#define DIRENT_BUFSIZ 32 * sizeof(struct linux_dirent64)

// root_len is so the root directory can be easily obtained from the src.
static int iterate(const char *src, const options *opts, int root_len) {
    int fd;
    char dirbuf[DIRENT_BUFSIZ];
    size_t bytes_read;
    struct linux_dirent64 *d;

    fd = open(src, O_RDONLY);
    if (fd == -1) {
        perror(src);
        return EXIT_FAILURE;
    }
    while (1) {
        d = (struct linux_dirent64 *)&dirbuf;
        bytes_read = getdents64(fd, d, DIRENT_BUFSIZ);
        if (bytes_read < 0) {
            perror(src);
            return EXIT_FAILURE;
        }
        if (bytes_read == 0) // 0 means we're done.
            break;
        for (size_t byte_pos = 0; byte_pos < bytes_read;) {
            d = (struct linux_dirent64 *)(dirbuf + byte_pos);
            byte_pos += d->d_reclen;
            if (is_ignored_entry(d->d_name))
                continue;
            if (handle_entry(src, d, opts, root_len) != EXIT_SUCCESS)
                return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

int main (int argc, char **argv) {
    int s = 0;
    int len;
    char *arg, *src;
    options opts;
    char quote;

    if (argc < 3) {
        return EXIT_FAILURE;
    }

    /* Zeroing the struct is important, because we are using null pointers to
    determine the ends of the incls, excls, and srcs arrays. */
    memset(&opts, 0, sizeof(opts));
    opts.dest = argv[argc - 1];

    for (int i = 1; i < argc - 1; i++) {
        arg = argv[i];
        if (!strcmp(arg, "--recursive") || !strcmp(arg, "-R"))
            opts.flags |= FLAG_RECURSE;
        else if (prefix("--", arg)) {
            printf("Unrecognized option: %s\nAborting.\n", arg);
            return EXIT_FAILURE;
        }
        else {
            opts.srcs[s++] = arg;
        }
    }

    s = 0;
    while (src = opts.srcs[s++]) {
        if (iterate(src, &opts, strlen(src)) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
