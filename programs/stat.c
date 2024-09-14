/*
Copied from: https://github.com/michael105/minicore/blob/2e5b2594246a79e9bc0ead73869014831c2c9c8f/porting/minutils/src/stat.c
Modified by Jonathan M. Wilbur to compile with nolibc.
Licensed under BSD-3-Clause License at the time of copying. See minicore.license.txt.
*/
#ifndef NOLIBC
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <pwd.h>
#include <grp.h>

#ifdef __GLIBC__
char* itoa (long in) {
    static char buf[100];
    sprintf(buf, "%ld", in);
    return &buf[0];
}
#endif

#else

#define bool    int
#define true    1
#define false   0

char* strcat(char* dest, char* src) {
    char* ret = dest;
    while (*dest)
	    dest++;
    while (*dest++ = *src++);
    return ret;
}

#endif

enum file_type {
	FILE_REGULAR,
	FILE_DIRECTORY,
	FILE_CHARACTER,
	FILE_BLOCK,
	FILE_FIFO,
	FILE_SYMLINK,
	FILE_SOCKET,
	FILE_UNKNOWN
};

static enum file_type get_file_type(mode_t mode) {
	if (S_ISREG(mode))
		return FILE_REGULAR;
	else if (S_ISDIR(mode))
		return FILE_DIRECTORY;
	else if (S_ISCHR(mode))
		return FILE_CHARACTER;
	else if (S_ISBLK(mode))
		return FILE_BLOCK;
	else if (S_ISFIFO(mode))
		return FILE_FIFO;
	else if (S_ISLNK(mode))
		return FILE_SYMLINK;
	else if (S_ISSOCK(mode))
		return FILE_SOCKET;
	else
		return FILE_UNKNOWN;
}

static char get_sticky(mode_t mode) {
	if (mode & S_ISVTX) {
		if ((mode & S_IXUSR) &&
		    (mode & S_IXGRP) &&
		    (mode & S_IXOTH)) {
			return 't';
		} else {
			return 'T';
		}
	} else if (mode & S_IXOTH) {
		return 'x';
	} else {
		return '-';
	}
}

static void get_time(char *buf, size_t len, time_t t) {
#ifdef NOLIBC
    // We have no time printing functions in nolibc: just print the epoch as decimal.
    strncpy(buf, itoa(t), len);
#else
	const struct tm *tm;

	tm = localtime(&t);
	strftime(buf, len, "%Y-%m-%d %H:%M:%S %Z", tm);
#endif
}

static void print_statblock(const char *path, const struct stat *stbuf) {
	const char *typename;
	enum file_type type;
	char mode[11], deviceinf[21];
	char atime[32], mtime[32], ctime[32];
#ifndef NOLIBC
    struct passwd *pwd;
	struct group *grp;

	pwd = getpwuid(stbuf->st_uid);
	grp = getgrgid(stbuf->st_gid);
#endif

	type = get_file_type(stbuf->st_mode);
	switch (type) {
	case FILE_REGULAR:
		typename = "regular file";
		mode[0] = '-';
		break;
	case FILE_DIRECTORY:
		typename = "directory";
		mode[0] = 'd';
		break;
	case FILE_CHARACTER:
		typename = "character special file";
		mode[0] = 'c';
		break;
	case FILE_BLOCK:
		typename = "block special file";
		mode[0] = 'b';
		break;
	case FILE_FIFO:
		typename = "fifo";
		mode[0] = 'p';
		break;
	case FILE_SYMLINK:
		typename = "symbolic link";
		mode[0] = 'l';
		break;
	case FILE_SOCKET:
		typename = "socket";
		mode[0] = 's';
		break;
	case FILE_UNKNOWN:
		typename = "unknown";
		mode[0] = '?';
	}
	if (type == FILE_CHARACTER || type == FILE_BLOCK) {
#ifndef NOLIBC
		sprintf(deviceinf, "Device type: %d,%d", major(stbuf->st_dev), minor(stbuf->st_dev));
#else // If using nolibc, we don't have access to device ID macros. Just print decimal.
        strcpy(deviceinf, "Device type: ");
        strcat(deviceinf, itoa(stbuf->st_dev));
#endif
	} else {
		deviceinf[0] = '\0';
	}

	mode[1] = (stbuf->st_mode & S_IRUSR) ? 'r' : '-';
	mode[2] = (stbuf->st_mode & S_IWUSR) ? 'w' : '-';
	mode[3] = (stbuf->st_mode & S_ISUID) ? 's' : ((stbuf->st_mode & S_IXUSR) ? 'x' : '-');
	mode[4] = (stbuf->st_mode & S_IRGRP) ? 'r' : '-';
	mode[5] = (stbuf->st_mode & S_IWGRP) ? 'w' : '-';
	mode[6] = (stbuf->st_mode & S_ISGID) ? 's' : ((stbuf->st_mode & S_IXGRP) ? 'x' : '-');
	mode[7] = (stbuf->st_mode & S_IROTH) ? 'r' : '-';
	mode[8] = (stbuf->st_mode & S_IWOTH) ? 'w' : '-';
	mode[9] = get_sticky(stbuf->st_mode);
	mode[10] = '\0';

	get_time(atime, sizeof(atime), stbuf->st_atime);
	get_time(mtime, sizeof(mtime), stbuf->st_mtime);
	get_time(ctime, sizeof(ctime), stbuf->st_ctime);

	printf("  File: %s\n"
	       "  Size: %ld\t\tBlocks: %ld\t   IO Block: %ld   %s\n"
	       "Device: %s/%ud\tInode: %lu\t   Links: %ld\t%s\n"
#ifndef NOLIBC
	       "Access: (%04o/%s)  Uid: (%5d/%8s)   Gid: (%5d/%8s)\n"
#else // nolibc doesn't support printf octal formatting, so we print the mode as hex.
           "Access: (0x%04x/%s)  Uid: (%5d/%8s)   Gid: (%5d/%8s)\n"
#endif
	       "Access: %s\n"
	       "Modify: %s\n"
	       "Change: %s\n"
	       " Birth: %s\n",
	       path,
	       (long)stbuf->st_size,
	       (long)stbuf->st_blocks,
	       (long)stbuf->st_blksize,
	       typename,
	       "???",
	       (unsigned)stbuf->st_dev,
	       (unsigned long)stbuf->st_ino,
	       (long)stbuf->st_nlink,
	       deviceinf,
	       stbuf->st_mode & 07777,
	       mode,
	       stbuf->st_uid,
#ifndef NOLIBC
	       (pwd) ? pwd->pw_name : "???",
#else
           "???",
#endif
	       stbuf->st_gid,
#ifndef NOLIBC
	       (grp) ? grp->gr_name : "???",
#else
           "???",
#endif
	       atime,
	       mtime,
	       ctime,
	       "-");
}

static int do_stat(const char *path) {
	struct stat stbuf;

	if (stat(path, &stbuf))
		return EXIT_FAILURE;
    print_statblock(path, &stbuf);
	return EXIT_SUCCESS;
}

const char *USAGE = "stat files...";
static void print_usage () {
    puts(USAGE);
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
        if (
            !strcmp(argv[i], "-h")
            || !strcmp(argv[i], "--help")
            || !strcmp(argv[i], "?")
        ) {
            print_usage();
            return EXIT_SUCCESS;
        }
		if (do_stat(argv[i]) != EXIT_SUCCESS) {
			fprintf(stderr, "%s: %s: %d\n", argv[0], argv[i], errno);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/* LICENSE AT THE TIME OF COPYING:
BSD 3-Clause License

Copyright (c) 2020, Michael (misc) Myer
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
