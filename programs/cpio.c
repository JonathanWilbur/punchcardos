/*
Copied from: https://github.com/rayae/minicpio/tree/master
License at the time of copying: GPL-3.0
Original work by Jeff Garzik

External file lists, symlink, pipe and fifo support by Thayne Harbaugh
Hard link support by Luciano Rocha

Modified by Jonathan M. Wilbur to build using nolibc, stream file to output
instead of buffering entirely into memory, and use fixed mtime for
reproducibility.

See documentation of this format here: https://www.kernel.org/doc/Documentation/early-userspace/buffer-format.txt

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
    return my_syscall3(__NR_readlink, pathname, buf, bufsiz);
}

#endif

#define xstr(s) #s
#define str(s) xstr(s)
#define DIRENT_BUFSIZ 32 * sizeof(struct linux_dirent64)

static unsigned int offset;
static unsigned int ino = 721; // I think this is just an arbitrary number.

/* Hard-coded for supreme reproducibility. */
const long int DEFAULT_MTIME = 1000000000;
int out;

static char to_hex_char (unsigned char byte) {
    if (byte < 10) {
        return byte + '0';
    } else {
        return (byte - 10) + 'A';
    }
}

static void uint32_to_hex (int* b, char* out, uint32_t offset) {
    /* No, this isn't a programming war crime at all! What are you talking about? */
    out[(*b)++] = to_hex_char((offset & 0xF0000000) >> 28);
    out[(*b)++] = to_hex_char((offset & 0x0F000000) >> 24);
    out[(*b)++] = to_hex_char((offset & 0x00F00000) >> 20);
    out[(*b)++] = to_hex_char((offset & 0x000F0000) >> 16);
    out[(*b)++] = to_hex_char((offset & 0x0000F000) >> 12);
    out[(*b)++] = to_hex_char((offset & 0x00000F00) >> 8);
    out[(*b)++] = to_hex_char((offset & 0x000000F0) >> 4);
    out[(*b)++] = to_hex_char((offset & 0x0000000F));
    return;
}

static void push_string(const char *name) {
    unsigned int name_len = strlen(name) + 1;
    write(out, name, name_len - 1);
    write(out, "\0", 1);
    offset += name_len;
}

static void push_pad(void) {
    while (offset & 3) {
        write(out, "\0", 1);
        offset++;
    }
}

static void push_rest(const char *name) {
    unsigned int name_len = strlen(name) + 1;
    unsigned int tmp_ofs;

    write(out, name, name_len - 1);
    write(out, "\0", 1);
    offset += name_len;

    tmp_ofs = name_len + 110;
    while (tmp_ofs & 3) {
        write(out, "\0", 1);
        offset++;
        tmp_ofs++;
    }
}

static void push_hdr(const char *s) {
    int len = strlen(s);
    if (write(out, s, len) != len) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    offset += 110;
}

const char *MAGIC = "070701";

const char *TRAILER_NAME = "TRAILER!!!";

const char *TRAILER_BYTES = "070701" /* magic */
    "00000000" /* ino */
    "00000000" /* mode */
    "00000000" /* uid */
    "00000000" /* gid */
    "00000001" /* nlink */
    "00000000" /* mtime */
    "00000000" /* filesize */
    "00000000" /* major */
    "00000000" /* minor */
    "00000000" /* rmajor */
    "00000000" /* rminor */
    "0000000B" /* namesize (strlen("TRAILER!!!") + 1)*/
    "00000000" /* cksum */
    ;

static void cpio_trailer(void) {
    push_hdr(TRAILER_BYTES);
    push_rest(TRAILER_NAME);
    while (offset % 512) {
        write(out, "\0", 1);
        offset++;
    }
}

static int cpio_mkslink(const char *name, const char *target,
                        unsigned int mode, uid_t uid, gid_t gid) {
    char s[256];
    int i = strlen(MAGIC);

    if (name[0] == '/')
        name++;
    strcpy(s, MAGIC); /* magic */
    uint32_to_hex(&i, &s[0], ino++); /* ino */
    uint32_to_hex(&i, &s[0], S_IFLNK | mode); /* mode */
    uint32_to_hex(&i, &s[0], uid); /* uid */
    uint32_to_hex(&i, &s[0], gid); /* gid */
    uint32_to_hex(&i, &s[0], 1); /* nlink */
    uint32_to_hex(&i, &s[0], DEFAULT_MTIME); /* mtime */
    uint32_to_hex(&i, &s[0], strlen(target) + 1); /* filesize */
    uint32_to_hex(&i, &s[0], 3); /* major */
    uint32_to_hex(&i, &s[0], 1); /* minor */
    uint32_to_hex(&i, &s[0], 0); /* rmajor */
    uint32_to_hex(&i, &s[0], 0); /* rminor */
    uint32_to_hex(&i, &s[0], strlen(name) + 1); /* namesize */
    uint32_to_hex(&i, &s[0], 0); /* chksum */
    s[i] = 0;
    push_hdr(s);
    push_string(name);
    push_pad();
    push_string(target);
    push_pad();
    return 0;
}

static int cpio_mkslink_line(const char *filename) {
    char target[PATH_MAX + 1];
    ssize_t i = readlink(filename, target, sizeof(target));
    if (i < 0)
        return EXIT_FAILURE;
    target[i] = 0;
    unsigned int mode = 0777;
    int uid = 0;
    int gid = 0;
    return cpio_mkslink(filename, target, mode, uid, gid);
}

static int cpio_mkgeneric(const char *name, unsigned int mode,
                          uid_t uid, gid_t gid) {
    char s[256];
    int i = strlen(MAGIC);

    if (name[0] == '/')
        name++;

    strcpy(s, MAGIC); /* magic */
    uint32_to_hex(&i, &s[0], ino++); /* ino */
    uint32_to_hex(&i, &s[0], mode); /* mode */
    uint32_to_hex(&i, &s[0], uid); /* uid */
    uint32_to_hex(&i, &s[0], gid); /* gid */
    uint32_to_hex(&i, &s[0], 2); /* nlink */
    uint32_to_hex(&i, &s[0], DEFAULT_MTIME); /* mtime */
    uint32_to_hex(&i, &s[0], 0); /* filesize */
    uint32_to_hex(&i, &s[0], 3); /* major */
    uint32_to_hex(&i, &s[0], 1); /* minor */
    uint32_to_hex(&i, &s[0], 0); /* rmajor */
    uint32_to_hex(&i, &s[0], 0); /* rminor */
    uint32_to_hex(&i, &s[0], strlen(name) + 1); /* namesize */
    uint32_to_hex(&i, &s[0], 0); /* chksum */
    s[i] = 0;

    push_hdr(s);
    push_rest(name);
    return 0;
}

static int cpio_mkdir_line(const char *name) {
    unsigned int mode = 0775;
    int uid = 0;
    int gid = 0;
    mode |= S_IFDIR;
    return cpio_mkgeneric(name, mode, uid, gid);
}

/* Read from one file and append it to another. */
static int append (int infd, int outfd, long size, const char* name) {
    int retval;
    char filebuf[BUFSIZ];

    long bytes_to_read = size;
    /* We have to do read/write in loops, because either may be interrupted
    by signals, among other possible problems. */
    while (bytes_to_read > 0) {
        retval = read(infd, filebuf, sizeof(filebuf));
        if (retval < 0) {
            perror(name);
            return EXIT_FAILURE;
        }
        if (retval == 0)
            break;
        bytes_to_read -= retval;
        long bytes_to_write = retval;
        while (bytes_to_write > 0) {
            retval = write(out, filebuf, bytes_to_write);
            if (retval < 0) {
                fprintf(stderr, "writing filebuf failed\n");
                return EXIT_FAILURE;
            }
            bytes_to_write -= retval;
        }
    }
    return EXIT_SUCCESS;
}

static int cpio_mkfile(const char *name, const char *location,
                       unsigned int mode, uid_t uid, gid_t gid,
                       unsigned int nlinks) {
    char s[256];
    struct stat buf;
    long size;
    int file = -1;
    int retval;
    int rc = EXIT_FAILURE;
    int namesize, i;
    unsigned int j;

    mode |= S_IFREG;

    file = open(location, O_RDONLY);
    if (file < 0) {
        fprintf(stderr, "File %s could not be opened for reading\n", location);
        goto error;
    }

    retval = stat(location, &buf);
    if (retval) {
        fprintf(stderr, "File %s could not be stat()'ed\n", location);
        goto error;
    }

    size = 0;
    for (j = 1; j <= nlinks; j++) {
        /* data goes on last link */
        if (j == nlinks)
            size = buf.st_size;

        if (name[0] == '/')
            name++;
        namesize = strlen(name) + 1;

        i = strlen(MAGIC);
        strcpy(s, MAGIC); /* magic */
        uint32_to_hex(&i, &s[0], ino); /* ino */
        uint32_to_hex(&i, &s[0], mode); /* mode */
        uint32_to_hex(&i, &s[0], uid); /* uid */
        uint32_to_hex(&i, &s[0], gid); /* gid */
        uint32_to_hex(&i, &s[0], nlinks); /* nlink */
        uint32_to_hex(&i, &s[0], buf.st_mtime); /* mtime */
        uint32_to_hex(&i, &s[0], size); /* filesize */
        uint32_to_hex(&i, &s[0], 3); /* major */
        uint32_to_hex(&i, &s[0], 1); /* minor */
        uint32_to_hex(&i, &s[0], 0); /* rmajor */
        uint32_to_hex(&i, &s[0], 0); /* rminor */
        uint32_to_hex(&i, &s[0], namesize); /* namesize */
        uint32_to_hex(&i, &s[0], 0); /* chksum */
        s[i] = 0;

        push_hdr(s);
        push_string(name);
        push_pad();

        /* This section just reads a file into memory and appends it to another. */
        if (size) {
            if (append(file, out, size, name) != EXIT_SUCCESS)
                goto error;
            offset += size;
            push_pad();
        }

        name += namesize;
    }
    ino++;
    rc = 0;

error:
    if (file >= 0)
        close(file);
    return rc;
}

static int cpio_mkfile_line(const char *filename) {
    unsigned int mode = 0644;
    int uid = 0;
    int gid = 0;
    int nlinks = 1;
    int end = 0, dname_len = 0;
    return cpio_mkfile(filename, filename, mode, uid, gid, nlinks);
}

static int usage() {
    fprintf(stderr, "mkcpio <input dir> <output file>\n");
    return EXIT_FAILURE;
}

#define LINE_SIZE (2 * PATH_MAX + 50)

static int is_ignored_entry(const char *name) {
    return (!strcmp(name, ".") || !strcmp(name, ".."));
}

static int iterate(const char *dir);

static int handle_entry(const char *parent, const struct linux_dirent64 *ent) {
    char fullpath[PATH_MAX + 1];
    strcpy(fullpath, parent);
    strcat(fullpath, "/");
    strcat(fullpath, (char *)ent->d_name);

    switch (ent->d_type) {
        case (DT_REG):
            return cpio_mkfile_line(fullpath);
        case (DT_DIR): {
            if (cpio_mkdir_line(fullpath) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            return iterate(fullpath);
        }
        case (DT_LNK):
            return cpio_mkslink_line(fullpath);
        default:
            break; // Everything else is ignored.
    }
    return EXIT_SUCCESS;
}

static int iterate(const char *dir) {
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
        if (bytes_read < 0)
            return EXIT_FAILURE;
        if (bytes_read == 0)  // 0 means we're done.
            break;
        for (size_t byte_pos = 0; byte_pos < bytes_read;) {
            d = (struct linux_dirent64 *)(dirbuf + byte_pos);
            byte_pos += d->d_reclen;
            if (is_ignored_entry(d->d_name))
                continue;
            if (handle_entry(dir, d) != EXIT_SUCCESS)
                return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    char *args, *type;
    const char *dirname;
    int isstd = 0;

    if (argv[1]) {
        dirname = argv[1];
    } else {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (!argv[2]) {
        isstd = 1;
        out = STDOUT_FILENO;
    } else {
        if (!(out = open(argv[2], O_WRONLY | O_CREAT, 0640))) {
            fprintf(stderr, "ERROR: unable to open '%s': %d\n\n", dirname, errno);
            if (!isstd)
                return usage();
        }
    }
    if (iterate(dirname) != EXIT_SUCCESS)
        return EXIT_FAILURE;
    cpio_trailer();
    if (!isstd)
        close(out);
    return EXIT_SUCCESS;
}
