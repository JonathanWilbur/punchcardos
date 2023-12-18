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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* XXX: fix me */
#ifndef PATH_MAX
# define PATH_MAX 4096
#endif /*!PATH_MAX*/

int
logical_pwd(void)
{
	char *pwd;

	pwd = getenv("PWD");
	if (pwd == NULL)
	{
		return (0);
	}

	if (*pwd != '/' || strlen(pwd) > PATH_MAX)
	{
		return (0);
	}

	if (strstr(pwd, "/..") != NULL)
	{
		return (0);
	}
	puts(pwd);
	return (1);
}

int
main(int argc, char *argv[])
{
	char buff[PATH_MAX];
	int idx;
	int logical; /* -L */

	logical = 1; /* POSIX default to -L */
	if (argc > 1)
	{
		for (idx = 1; idx < argc; idx++)
		{
			if (strcmp(argv[idx], "-L") == 0)
			{
				logical = 1;
			}
			else if (strcmp(argv[idx], "-P") == 0)
			{
				logical = 0;
			}
		}
	}

	if (logical && logical_pwd())
	{
		return (EXIT_SUCCESS);
	}

	if (getcwd(buff, sizeof(buff)) == NULL)
	{
		perror("pwd");
		return (EXIT_FAILURE);
	}
	puts(buff);
	return (EXIT_SUCCESS);
}
