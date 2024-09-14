/*
Copied from: https://github.com/michael105/minicore/blob/2e5b2594246a79e9bc0ead73869014831c2c9c8f/porting/minutils/src/tty.c
Modified by Jonathan M. Wilbur to compile with nolibc.
License (BSD 3-Clause) commented out at the bottom.
*/
#include <limits.h>
#ifndef NOLIBC
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#else

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    return (ssize_t)my_syscall3(__NR_readlink, pathname, buf, bufsiz);
}

void procfdname (char *buf, unsigned fd) {
	unsigned i, j;
	for (i=0; (buf[i] = "/proc/self/fd/"[i]); i++);
	if (!fd) {
		buf[i] = '0';
		buf[i+1] = 0;
		return;
	}
	for (j=fd; j; j/=10, i++);
	buf[i] = 0;
	for (; fd; fd/=10) buf[--i] = '0' + fd%10;
}


int ttyname_r (int fd, char *name, size_t size) {
	char procname[sizeof "/proc/self/fd/" + 3*sizeof(int) + 2];
	ssize_t l;
	procfdname(procname, fd);
	l = readlink(procname, name, size);
	if (l < 0) return errno;
	else if (l == size) return ERANGE;
	name[l] = 0;
	return 0;
}

char *ttyname (int fd) {
	static char buf[TTY_NAME_MAX];
	int result;
	if ((result = ttyname_r(fd, buf, sizeof buf))) {
		errno = result;
		return NULL;
	}
	return buf;
}

#endif

/* Usage: tty [-s] */
int main(int argc, const char *argv[])
{
	const char *tty;
	int verbose, i;

	verbose = 1;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			verbose = 0;
		} else {
			fprintf(stderr, "%s: invalid argument: %s\n",
				argv[0], argv[i]);
			return 2;
		}
	}
	tty = ttyname(STDIN_FILENO);
	if (verbose) {
		if (tty)
			puts(tty);
		else
			puts("not a tty");
	}
	return tty == NULL;
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
