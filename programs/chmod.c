/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/chmod.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define USED __attribute__((used))
#elif defined __GNUC__
#define USED __attribute__((unused))
#else
#define USED
#endif
#if defined(SUS)
static const char sccsid[] USED = "@(#)chmod_sus.sl	1.10 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)chmod.sl	1.10 (gritter) 5/29/05";
#endif
/*
 * chmod [-R] [ugoa][+-=][rwxXlstugo] files
 *  change mode of files
 */
#include <linux/limits.h>
#ifndef NOLIBC
// TODO: It might be possible to clean some of these up.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


struct linux_dirent64 {
    ino64_t        d_ino;    /* 64-bit inode number */
    off64_t        d_off;    /* Not an offset; see getdents() */
    unsigned short d_reclen; /* Size of this dirent */
    unsigned char  d_type;   /* File type */
    char           d_name[]; /* Filename (null-terminated) */
};

#else
#include <syscall.h>
#include <sys/syscall.h>

int lstat (const char *pathname, struct stat *statbuf) {
    return __sysret(my_syscall2(__NR_lstat, pathname, statbuf));
}

char *strcat(char *dest, char *src) {
    char *ret = dest;
    while (*dest)
        dest++;
    while (*dest++ = *src++)
        ;
    return ret;
}

#endif

#define DIRENT_BUFSIZ   32 * sizeof(struct linux_dirent64)

#define USER 05700  /* user's bits */
#define GROUP 02070 /* group's bits */
#define OTHER 00007 /* other's bits */
#define ALL 07777   /* all */

#define READ 00444   /* read permit */
#define WRITE 00222  /* write permit */
#define EXEC 00111   /* exec permit */
#define SETID 06000  /* set[ug]id */
#define STICKY 01000 /* sticky bit */

#ifndef S_ENFMT
#define S_ENFMT 02000 /* mandatory locking bit */
#endif

static mode_t um;
static int Rflag;

static mode_t newmode(const char *, const mode_t, const char *);
static void corresp(const char *);
static void execreq(const char *);
static int change_mode(const char *ms, const char *full_path);

static int recurse_dir (const char *ms, const char *dir) {
    int fd;
    char dirbuf[DIRENT_BUFSIZ];
    size_t bytes_read;
    struct linux_dirent64 *d;
    char full_entry[NAME_MAX];

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
            if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
                continue;
            strcpy(full_entry, dir);
            strcat(full_entry, "/");
            strcat(full_entry, d->d_name);
            change_mode(ms, full_entry);
        }
    }
    if (bytes_read < 0) {
        perror("recurse_dir");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int change_mode(const char *ms, const char *full_path) {
    int symlink;
    struct stat st;
    int fd;
    char dirbuf[DIRENT_BUFSIZ];
    size_t bytes_read;
    struct linux_dirent64 *d;

	if (lstat(full_path, &st) < 0
        || (symlink = ((st.st_mode & S_IFMT) == S_IFLNK))
        && stat(full_path, &st) < 0) {
		printf("can't access %s", full_path);
		return EXIT_FAILURE;
	}

    if (chmod(full_path, newmode(ms, st.st_mode, full_path)) < 0) {
        printf("can't change %s", full_path);
        return EXIT_FAILURE;
    }

    if (Rflag && symlink == 0 && (st.st_mode & S_IFMT) == S_IFDIR) {
        if (recurse_dir(ms, full_path) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static mode_t parse_octal(const char **ms) {
    register int c, i;

    i = 0;
    while ((c = *(*ms)++) >= '0' && c <= '7')
        i = (i << 3) + (c - '0');
    (*ms)--;
    return i;
}

static mode_t parse_who(const char **ms, mode_t *mp) {
    register int m;

    m = 0;
    *mp = 0;
    while (1)
        switch (*(*ms)++) {
            case 'u':
                m |= USER;
                continue;
            case 'g':
                m |= GROUP;
                continue;
            case 'o':
                m |= OTHER;
                continue;
            case 'a':
                m |= ALL;
                continue;
            default:
                (*ms)--;
                if (m == 0) {
                    m = ALL;
                    *mp = um;
                }
                return m;
        }
}

static int parse_operation(const char **ms) {
    switch (**ms) {
        case '+':
        case '-':
        case '=':
            return *(*ms)++;
    }
    return 0;
}


static mode_t parse_where(const char **ms, mode_t om, int *lock, int *copy, const mode_t pm) {
    register mode_t m;

    m = 0;
    *copy = 0;
    switch (**ms) {
        case 'u':
            m = (om & USER) >> 6;
            goto dup;
        case 'g':
            m = (om & GROUP) >> 3;
            goto dup;
        case 'o':
            m = (om & OTHER);
        dup:
            *copy = 1;
            m &= (READ | WRITE | EXEC);
            m |= (m << 3) | (m << 6);
            ++(*ms);
            return m;
    }
    for (;;) switch (*(*ms)++) {
            case 'r':
                m |= READ;
                continue;
            case 'w':
                m |= WRITE;
                continue;
            case 'x':
                m |= EXEC;
                continue;
            case 'X':
                if ((pm & S_IFMT) == S_IFDIR || (pm & EXEC))
                    m |= EXEC;
                continue;
            case 'l':
                if ((pm & S_IFMT) != S_IFDIR)
                    *lock |= 04;
                continue;
            case 's':
                m |= SETID;
                continue;
            case 't':
                m |= STICKY;
                continue;
            default:
                (*ms)--;
                return m;
        }
}

static mode_t newmode(const char *ms, const mode_t pm, const char *fn) {
    register mode_t o, m, b;
    int lock, setsgid = 0, didprc = 0, cleared = 0, copy = 0;
    mode_t nm, om, mm;

    nm = om = pm;
    m = parse_octal(&ms);
    if (!*ms) {
        nm = m;
        goto out;
    }
    if ((lock = (nm & S_IFMT) != S_IFDIR && (nm & (S_ENFMT | S_IXGRP)) == S_ENFMT) == 01)
        nm &= ~(mode_t)S_ENFMT;
    do {
        m = parse_who(&ms, &mm);
        while (o = parse_operation(&ms)) {
            b = parse_where(&ms, nm, &lock, &copy, pm);
            switch (o) {
                case '+':
                    nm |= b & m & ~mm;
                    if (b & S_ISGID)
                        setsgid = 1;
                    if (lock & 04)
                        lock |= 02;
                    break;
                case '-':
                    nm &= ~(b & m & ~mm);
                    if (b & S_ISGID)
                        setsgid = 1;
                    if (lock & 04)
                        lock = 0;
                    break;
                case '=':
                    nm &= ~m;
                    nm |= b & m & ~mm;
                    lock &= ~01;
                    if (lock & 04)
                        lock |= 02;
                    om = 0;
                    if (copy == 0)
                        cleared = 1;
                    break;
            }
            lock &= ~04;
        }
    } while (*ms++ == ',');
    if (*--ms) {
        puts("invalid mode");
        exit(255);
    }
    if (lock) {
        if (om & S_IXGRP) {
            printf("Locking not permitted on %s, a group executable file", fn);
            return pm;
        } else if ((lock & 02) && (nm & S_IXGRP)) {
            puts("Group execution and locking not permitted together");
            exit(3);
        }
        else if ((lock & 01) && (nm & S_IXGRP)) {
            printf("Group execution not permitted on %s, a lockable file", fn);
            return pm;
        } else if ((lock & 02) && (nm & S_ISGID)) {
            printf("Set-group-ID and locking not permitted together");
            exit(3);
        }
        else if ((lock & 01) && (nm & S_ISGID)) {
            printf("Set-group-ID not permitted on %s, a lockable file", fn);
            return pm;
        }
        nm |= S_ENFMT;
    } else /* lock == 0 */
        if ((pm & S_IFMT) != S_IFDIR) {
            if ((om & (S_IXGRP | S_ISGID)) == (S_IXGRP | S_ISGID) &&
                (nm & (S_IXGRP | S_ISGID)) == S_ISGID) {
                corresp(fn);
                didprc = 1;
                nm &= ~(mode_t)S_ISGID;
            } else if ((nm & (S_ISGID | S_IXGRP)) == S_ISGID) {
                if (fn || cleared) {
                    execreq(fn);
                    return pm;
                }
            }
        }
    if ((pm & S_IFMT) != S_IFDIR) {
        if ((om & (S_IXUSR | S_ISUID)) == (S_IXUSR | S_ISUID) &&
            (nm & (S_IXUSR | S_ISUID)) == S_ISUID) {
            if (didprc == 0)
                corresp(fn);
            nm &= ~(mode_t)S_ISUID;
        } else if ((nm & (S_ISUID | S_IXUSR)) == S_ISUID) {
            if (fn || cleared) {
                execreq(fn);
                return pm;
            }
        }
    }
out:
    if ((pm & S_IFMT) == S_IFDIR) {
        if ((pm & S_ISGID) && setsgid == 0)
            nm |= S_ISGID;
        else if ((nm & S_ISGID) && setsgid == 0)
            nm &= ~(mode_t)S_ISGID;
    }
    return (nm);
}

static void corresp (const char *fn) {
    printf("Corresponding set-ID also disabled on %s since set-ID requires execute permission", fn);
}

static void execreq (const char *fn) {
    if (fn)
        printf("Execute permission required for set-ID on execution for %s", fn);
    else {
        puts("Execute permission required for set-ID on execution");
        exit(EXIT_FAILURE);
    }
}

const char *USAGE = "Usage: chmod [-R|--recurse] [ugoa][+-=][rwxXlstugo] FILES...";
static void print_usage (void) {
    puts(USAGE);
}

int main (int argc, char **argv) {
    int i;
    const char *mode_string;
    char *arg;

    if (argc < 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc - 2; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;

        if (!strcmp(arg, "--help") || !strcmp(arg, "-h") || !strcmp(arg, "-?")) {
            print_usage();
            return EXIT_SUCCESS;
        }
        else if (!strcmp(arg, "--recurse") || !strcmp(arg, "-R")) {
            Rflag = 1;
        }
        else if (arg[0] != '-') {
            break;
        }
        else {
            print_usage();
            return EXIT_FAILURE;
        }
    }

#ifdef SUS
    umask(um = umask(0));
#endif /* SUS */
    mode_string = argv[i++];
    newmode(mode_string, 0, NULL);
    /* Everything that follows is a file name. */
    for (; i < argc; i++)
        if (change_mode(mode_string, argv[i]) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    return EXIT_SUCCESS;
}