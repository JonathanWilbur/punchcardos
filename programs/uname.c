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
#include <sys/utsname.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#define CMD_M 0x1
#define CMD_N 0x2
#define CMD_R 0x4
#define CMD_S 0x8
#define CMD_V 0x10

static int
decode_flags(int argc, char *argv[])
{
	int opt;
	int flags;

	flags = 0;
	while ((opt = getopt(argc, argv, "amnrsv")) != -1)
	{
		switch (opt)
		{
		 case 'a':
			return (CMD_M | CMD_N | CMD_R | CMD_S | CMD_V);
		 case 'm':
			flags |= CMD_M;
			break;
		 case 'n':
			flags |= CMD_N;
			break;
		 case 'r':
			flags |= CMD_R;
			break;
		 case 's':
			flags |= CMD_S;
			break;
		 case 'v':
			flags |= CMD_V;
			break;
		 default:
			break;
		}
	}

	if (flags == 0)
		flags = CMD_S;
	return (flags);
}

static void
print_data(char *str)
{
	static char has_previous = 0;

	if (has_previous)
		putchar(' ');
	fputs(str, stdout);
	has_previous = 1;
}

int
main(int argc, const char *argv[])
{
	struct utsname u;
	int flags;

	flags = decode_flags(argc, (char **)argv);
	if (uname(&u) < 0)
	{
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
	putchar('\n');
	return (EXIT_SUCCESS);
}
