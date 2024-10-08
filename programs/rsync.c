/*
rsync that is minimally good enough to copy Linux kernel headers per the Linux
install_headers Make target.

Theoretically, this could be even simpler, since it could be hard-coded to
perform only the specific invocation used by the install_headers Make target,
but it seemed worthwhile to make it just a little more flexible by actually
supporting the three options used as well as glob expressions and multiple
sources.

Note that the glob expressions implementation is probably incorrect. This is
just barely meant to work for one particular use case anyway.

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

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    return __sysret(my_syscall3(__NR_readlink, pathname, buf, bufsiz));
}

// NOTE: loff_t was replaced by long int.
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
#define FLAG_PRUNE      (1 << 1)
#define FLAG_SYMLINKS   (1 << 2)

#define INEXARGSZ   sizeof("--exclude=")

typedef struct options_st {
    char *incls[64];
    char *excls[64];
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
    if (dfd == -1) {
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

// if  the pattern starts with a / then it is anchored to a particular spot in the hierarchy of files, otherwise it is matched against the end of the pathname.  This is similar to
//  a leading ^ in regular expressions.  Thus "/foo" would match a name of "foo" at either the "root of the transfer" (for a global rule) or in the merge-file’s  directory  (for  a
//  per-directory  rule).   An  unqualified  "foo" would match a name of "foo" anywhere in the tree because the algorithm is applied recursively from the top down; it behaves as if
//  each path component gets a turn at being the end of the filename.  Even the unanchored "sub/foo" would match at any point in the hierarchy where a "foo" was found within a  di‐
//  rectory named "sub".  See the section on ANCHORING INCLUDE/EXCLUDE PATTERNS for a full discussion of how to specify a pattern that matches at the root of the transfer.

// if  the pattern contains a / (not counting a trailing /) or a "**", then it is matched against the full pathname, including any leading directories. If the pattern doesn’t con‐
// tain a / or a "**", then it is matched only against the final component of the filename.  (Remember that the algorithm is applied recursively so "full filename" can actually be
// any portion of a path from the starting directory on down.)

// a  trailing  "dir_name/***" will match both the directory (as if "dir_name/" had been specified) and everything in the directory (as if "dir_name/**" had been specified).  This
// behavior was added in version 2.6.7.

/* Return TRUE if the expr glob expression matches path, otherwise FALSE. */
static bool match (const char *expr, const char *path, bool is_dir) {
    char* p = (char *)path;
    char* e = (char *)expr;
    char match_until = 0;
    unsigned int asterisk = 0;

    while (e[0] && p[0]) {
        switch (e[0]) {
        case ('*'):
            asterisk++;
            break;
        case ('?'):
            if (asterisk > 0)
                goto next; // If ? follows * or **, just ignore it.
            if (p[0] == '/')
                return false;
            p++;
            break;
        case ('\\'):
            /* This is technically incorrect: backslash should be treated
            literally if there are no wildcards in the expression. */
            e++;
            if (!e[0])
                return false; // Corrupted expression.
            if (p[0] != e[0])
                return false;
            p++;
            break;
        default: {
            if (asterisk == 1) {
                match_until = '/';
            } else if (asterisk >= 2) {
                match_until = e[0];
            }

            if (match_until) {
                while (p[0] && p[0] != match_until)
                    p++;
                match_until = 0;
            } else if (e[0] != p[0]) { // Otherwise, match characters literally
                return false;
            }
            break;
        }
        }

        next:
            e++;
    }

    if (e[0] == '/') {
        if (!is_dir)
            return false;
        e++;
    }

    return (!e[0] && !p[0]);
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
    bool excluded = false;
    bool not_excluded = false;
    char srcpath[PATH_MAX + 1];
    char destpath[PATH_MAX + 1];
    char relpath[PATH_MAX + 1];
    char linkpath[PATH_MAX + 1];
    char *excl = opts->excls[0];
    char *incl = opts->incls[0];
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

    while (excl) {
        if (match(compared_name, excl, ent->d_type == DT_DIR)) {
            excluded = true;
            break;
        }
        excl++;
    }

    while (incl) {
        if (match(compared_name, incl, ent->d_type == DT_DIR)) {
            not_excluded = true;
            break;
        }
        incl++;
    }

    if (excluded && !not_excluded) {
        // The entry is ignored.
        return EXIT_SUCCESS;
    }

    switch (ent->d_type) {
        case (DT_REG):
            return copy_file(srcpath, destpath);
        case (DT_DIR): {
            if (stat(srcpath, &st) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
            if (mkdir(destpath, st.st_mode))
                return EXIT_FAILURE;
            if (opts->flags & FLAG_RECURSE)
                return iterate(srcpath, opts, root_len);
            else
                return EXIT_SUCCESS;
        }
        case (DT_LNK):
            rc = readlink(srcpath, linkpath, sizeof(linkpath));
            if (rc < 0) {
                perror(srcpath);
                return EXIT_FAILURE;
            }
            linkpath[rc] = 0;
            if (symlink(linkpath, destpath) < 0) {
                perror(srcpath);
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        default:
            break; // Everything else is ignored.
    }
    return EXIT_SUCCESS;
}

#define DIRENT_BUFSIZ 32 * sizeof(struct linux_dirent64)

// root_len is so the root directory can be easily obtained from the src.
static int iterate(const char *src, const options *opts, int root_len) {
    int fd;
    char dirbuf[DIRENT_BUFSIZ];
    size_t bytes_read;
    struct linux_dirent64 *d;

    fd = open(src, O_RDONLY);
    if (fd == -1)
        return EXIT_FAILURE;
    while (1) {
        d = (struct linux_dirent64 *)&dirbuf;
        bytes_read = getdents64(fd, d, DIRENT_BUFSIZ);
        if (bytes_read < 0)
            return EXIT_FAILURE;
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

/*
    This command has to be capable of matching the following capability:
    -m = prune empty directories
    -r = recursive
    -l = copy symlinks as symlinks

"if the pattern ends with a / then it will only match a directory, not a regular file, symlink, or device."
*/
// rsync -mrl --include='*/' --include='*\.h' --exclude='*' usr/include $(INSTALL_HDR_PATH)
// rsync [OPTION...] SRC... [DEST]
int main (int argc, char **argv) {
    int n = 0;
    int x = 0;
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
        if (!strcmp(arg, "--recursive") || !strcmp(arg, "-r"))
            opts.flags |= FLAG_RECURSE;
        else if (!strcmp(arg, "--prune-empty-dirs") || !strcmp(arg, "-m"))
            opts.flags |= FLAG_PRUNE;
        else if (!strcmp(arg, "--links") || !strcmp(arg, "-l"))
            opts.flags |= FLAG_SYMLINKS;
        else if (prefix("--include=", arg) && strlen(arg) > INEXARGSZ) {
            arg = &arg[INEXARGSZ];
            if (arg[INEXARGSZ] == '\'' || arg[INEXARGSZ] == '"') {
                quote = arg[INEXARGSZ];
                arg++;
                len = strlen(arg);
                if (len == 0 || arg[len - 1] != quote)
                    return EXIT_FAILURE;
                arg[len - 1] = 0; // Effectively delete the trailing quote.
            }
            opts.incls[n++] = arg;
        }
        else if (prefix("--exclude=", arg) && strlen(arg) > INEXARGSZ) {
            arg = &arg[INEXARGSZ];
            if (arg[INEXARGSZ] == '\'' || arg[INEXARGSZ] == '"') {
                quote = arg[INEXARGSZ];
                arg++;
                len = strlen(arg);
                if (len == 0 || arg[len - 1] != quote)
                    return EXIT_FAILURE;
                arg[len - 1] = 0; // Effectively delete the trailing quote.
            }
            opts.excls[x++] = arg;
        }
        else if (prefix("--", arg)) {
            printf("Unrecognized option: %s\nAborting.\n", arg);
            return EXIT_FAILURE;
        }
        // TODO: Support grouped options (e.g. "-mrl")
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
