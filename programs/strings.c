/*
Copied from: https://github.com/michael105/minicore/blob/master/porting/minutils/src/strings.c
Modified by Jonathan M. Wilbur to compile with nolibc, as well as some
formatting nits.

Licensed under BSD 3-Clause license at the time of copy.
See license commented out at the bottom.
*/
#ifndef NOLIBC
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#endif

static int isprint_lookahead(const char *buf, size_t index, size_t len, unsigned int min_len) {
	unsigned int i;

	for (i = 1; i < min_len; i++) {
		if (index + i >= len)
			break;
		if (!isprint(buf[index + i]))
			return 0;
	}
	return 1;
}

static int strings(int fd, unsigned int min_len) {
	char buf[4096];
	size_t i, len;
	int wip;

	wip = 0;
	while (1) {
		len = read(fd, buf, sizeof(buf));
        if (len == 0)
            break;
		if (len < 0) {
            perror("strings");
            return EXIT_FAILURE;
		}
		for (i = 0; i < len; i++) {
			if (isprint(buf[i])) {
				if (!wip && isprint_lookahead(buf, i, len, min_len))
					wip = 1;
				if (wip)
					putchar(buf[i]);
			} else if (wip) {
				putchar('\n');
				wip = 0;
			}
		}
	}
	if (wip)
		putchar('\n');
	return EXIT_SUCCESS;
}

/* Usage: strings [-MIN-LENGTH] FILE... */
int main(int argc, const char *argv[]) {
	int i, fd, ret;
	unsigned int min_len = 2;

	i = 1;
	if (i < argc && argv[i][0] == '-') {
		min_len = atoi(argv[i] + 1);
		if (!min_len) {
			fprintf(stderr, "%s: invalid number: %s\n", argv[0], argv[i]);
			return EXIT_FAILURE;
		}
		i++;
	}

	if (i == argc)
		return strings(STDIN_FILENO, min_len);

    for (; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "%s: unable to open '%s': %d\n", argv[0], argv[i], errno);
			return EXIT_FAILURE;
		}
		ret = strings(fd, min_len);
		close(fd);
		if (ret)
			return EXIT_FAILURE;
	}
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
