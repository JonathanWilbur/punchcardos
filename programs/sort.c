/*
Copied from: https://github.com/michael105/minicore/blob/master/porting/minutils/src/sort.c
Modified by Jonathan M. Wilbur to be able to compile with nolibc, as well as
some formatting nits, then modified again to be `uniq` instead of `sort`.

BSD 3-Clause license at the time of copy. See license at the bottom.
*/
#include <linux/limits.h>
#ifndef NOLIBC
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#else
#define BUFSIZ  8192
#endif

typedef int (*CMP_FN) (char *a, char *b);

void swap (char** A, int i, int j) {
    char* temp;
    temp = A[i];
    A[i] = A[j];
    A[j] = temp;
}

static int partition (char** A, int p, int r, CMP_FN compare) {
    int i, j;
    char* x = A[r];
    i = p - 1;
    for (j = p; j < r; j++) {
        if (compare(A[j], x) <= 0) {
            i++;
            swap(A, i, j);
        }
    }
    swap(A, i + 1, r);
    return(i + 1);
}

void quick_sort (char** A, int p, int r, CMP_FN compare) {
    int q;
    if (p < r){
        q = partition(A, p, r, compare);
        quick_sort(A, p, q - 1, compare);
        quick_sort(A, q + 1, r, compare);
    }
}

static struct {
	char **array;
	size_t len;
	size_t cap;
} lines;

static char linebuf[BUFSIZ];
static size_t linebufsize = 0;

#define READLINE_OK         EXIT_SUCCESS
#define READLINE_EREAD      (-1)
#define READLINE_EMALLOC    (-2)
#define READLINE_ETOOBIG    (-3)

/* An absolutely STINKY implementation for the sake of simplicity. */
static ssize_t readline (int fd, char **line) {
    char c;
    ssize_t rc;

    while ((rc = read(fd, &c, 1)) == 1) { // Read one character at a time.
        if (c == '\r')
            continue;
        if (c == '\n')
            break;
        if (linebufsize == BUFSIZ)
            return READLINE_ETOOBIG;
        linebuf[linebufsize++] = (char)c;
    }
    if (rc < 0)
        return READLINE_EREAD;
    *line = malloc(linebufsize + 1);
    if (*line == NULL)
        return READLINE_EMALLOC;
    memcpy(*line, linebuf, linebufsize);
    (*line)[linebufsize] = 0; // Add null terminator.
    rc = linebufsize;
    linebufsize = 0;
    return rc;
}

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static int lines_init () {
	lines.len = 0;
	lines.cap = 64;
	lines.array = malloc(lines.cap * sizeof(char *));
	if (!lines.array) {
		fprintf(stderr, "sort: unable to allocate array: %d\n", errno);
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}

static void lines_resize () {
	void *ptr;
	size_t new_cap;

	new_cap = lines.cap * 2;
	ptr = realloc(lines.array, new_cap * sizeof(char *));
	if (!ptr) {
		fprintf(stderr, "sort: unable to resize array: %d\n", errno);
		exit(1);
	}

	lines.array = ptr;
}

static void lines_append (char *line) {
	if (lines.len >= lines.cap)
		lines_resize();
	lines.array[lines.len] = line;
	lines.len++;
}

static int compare_normal (char *x, char *y) {
	return strcmp(x, y);
}

static int compare_reverse (char *x, char *y) {
	return -strcmp(x, y);
}

static int read_lines (int fd) {
	char *line;
	size_t n;

	while (1) {
		n = readline(fd, &line);
        if (n < 0) // TODO: You could do more logging here.
            return EXIT_FAILURE;
        if (n == 0) {
            break;
        }
		lines_append(line);
	}
    return EXIT_SUCCESS;
}

static void print_all () {
	size_t i;

	for (i = 0; i < lines.len; i++) {
		fputs(lines.array[i], stdout);
        fputc('\n', stdout);
		free(lines.array[i]);
	}
}

static void print_unique () {
	size_t i;

	fputs(lines.array[0], stdout);
    fputc('\n', stdout);
	for (i = 1; i < lines.len; i++) {
		if (strcmp(lines.array[i], lines.array[i - 1])) {
			fputs(lines.array[i], stdout);
            fputc('\n', stdout);
        }
		free(lines.array[i - 1]);
	}
	free(lines.array[lines.len - 1]);
}

#define FLAG_NONE           0
#define FLAG_UNIQUE         1

int main (int argc, char **argv) {
	size_t i;
	int ch;
    int flags = FLAG_NONE;
    char *arg;
    int fd;
    CMP_FN cmp = compare_normal;

parse_args:
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (!strcmp(arg, "--help") || !strcmp(arg, "-h") || !strcmp(arg, "-?")) {
            return EXIT_SUCCESS;
        }
        else if (!strcmp(arg, "--unique") || !strcmp(arg, "-u")) {
            flags |= FLAG_UNIQUE;
        }
        else if (!strcmp(arg, "--reverse") || !strcmp(arg, "-r")) {
            cmp = compare_reverse;
        }
        else if (!strcmp(arg, "-")) {
            break;
        }
        else if (prefix("-", arg)) {
            printf("Option not understood: %s\n", arg);
            return EXIT_FAILURE;
        }
        else {
            break;
        }
    }

	if (lines_init() != EXIT_SUCCESS)
        return EXIT_FAILURE;

open_files:
	if (argc == i) {
        if (read_lines(STDIN_FILENO) != EXIT_SUCCESS)
            return EXIT_FAILURE;
        close(STDIN_FILENO);
	} else {
        for (; i < argc; i++) {
            arg = argv[i];
            if (!strcmp(arg, "-")) {
                if (read_lines(STDIN_FILENO) != EXIT_SUCCESS)
                    return EXIT_FAILURE;
                continue;
            }

            fd = open(arg, O_RDONLY);
            if (fd < 0) {
                perror(arg);
                return EXIT_FAILURE;
            }
            if (read_lines(fd) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            close(fd);
        }
	}

    if (lines.len == 0)
        return EXIT_SUCCESS;

    quick_sort(lines.array, 0, lines.len - 1, cmp);

	if (flags & FLAG_UNIQUE)
		print_unique();
	else
		print_all();

	free(lines.array);
	return EXIT_SUCCESS;
}

/*
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
