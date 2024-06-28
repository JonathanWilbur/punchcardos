/*

Sourced from: https://github.com/kohi-gnu/unix-utils

BSD 3-Clause License

Copyright (c) 2020, d0p1
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
#ifndef NOLIBC
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#else

#include <sys/utsname.h>

int uname (struct utsname *buf) {
    return my_syscall1(__NR_uname, buf);
}

#endif

#define CMD_M   0x1
#define CMD_N   0x2
#define CMD_R   0x4
#define CMD_S   0x8
#define CMD_V   0x10
#define CMD_P   0x20
#define CMD_I   0x40
#define CMD_O   0x80

static int decode_flags(int argc, char *argv[])
{
	int opt;
	int flags;

	flags = 0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "--all") == 0 || strcmp(arg, "-a") == 0)
            return (CMD_M | CMD_N | CMD_R | CMD_S | CMD_V);
        else if (strcmp(arg, "--kernel-name") == 0 || strcmp(arg, "-s") == 0) {
            flags |= CMD_S;
        }
        else if (strcmp(arg, "--nodename") == 0 || strcmp(arg, "-n") == 0) {
            flags |= CMD_N;
        }
        else if (strcmp(arg, "--kernel-release") == 0 || strcmp(arg, "-r") == 0) {
            flags |= CMD_R;
        }
        else if (strcmp(arg, "--kernel-version") == 0 || strcmp(arg, "-v") == 0) {
            flags |= CMD_V;
        }
        else if (strcmp(arg, "--machine") == 0 || strcmp(arg, "-m") == 0) {
            flags |= CMD_M;
        }
        else if (strcmp(arg, "--processor") == 0 || strcmp(arg, "-p") == 0) {
            flags |= CMD_P;
        }
        else if (strcmp(arg, "--hardware-platform") == 0 || strcmp(arg, "-i") == 0) {
            flags |= CMD_I;
        }
        else if (strcmp(arg, "--operating-system") == 0 || strcmp(arg, "-o") == 0) {
            flags |= CMD_O;
        }
    }

	if (flags == 0)
		flags = CMD_S;
	return (flags);
}

static void print_data (char *str)
{
	static char has_previous = 0;

	if (has_previous)
		putchar(' ');
	fputs(str, stdout);
	has_previous = 1;
}

// TODO: Compare with the real uname. It is not exactly the same...
int main (int argc, const char *argv[])
{
	struct utsname u;
	int flags;

	flags = decode_flags(argc, (char **)argv);
	if (uname(&u) < 0) {
		perror("uname");
		return (EXIT_FAILURE);
	}

	if (flags & CMD_S)
		print_data(u.sysname);
	if (flags & CMD_N)
		print_data(u.nodename);
	if (flags & CMD_R)
		print_data(u.release);
	if (flags & CMD_V)
		print_data(u.version);
	if (flags & CMD_M)
		print_data(u.machine);
    if (flags & CMD_P)
        print_data(u.machine); // FIXME:
    if (flags & CMD_I)
        print_data(u.machine); // FIXME:
    if (flags & CMD_O)
        print_data(u.machine); // FIXME:

	putchar('\n');
	return (EXIT_SUCCESS);
}
