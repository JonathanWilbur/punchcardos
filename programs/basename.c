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
#include <string.h>
#include <libgen.h>
#else

/* This is probably not a correct implementation somehow, but it is surely
"close enough." */
static char *basename (char *path) {
    int len = strlen(path);

    // Real basename returns an empty string if supplied one.
    if (len == 0) {
        return "";
    }

    // Real basename returns a single slash if supplied one.
    if (len == 1 && path[0] == '/') {
        return "/";
    }

    int i = len - 1;

    // Ignore trailing slashes, which is what the real basename does.
    for (; i > 0; i--) {
        if (path[i] != '/') {
            break;
        }
    }

    // Real basename returns a single slash if the input is all slashes.
    if (i == 0) {
        return "/";
    }

    // Set the trailing slash to a null character to cut the string off here.
    path[i+1] = 0;

    for (; i >= 0; i--) {
        if (path[i] == '/') {
            return &path[i+1];
        }
    }
    return path;
}

#endif

static void
remove_suffix(char *str, char *suffix)
{
	char *suffix_end;
	char *str_end;

	suffix_end = suffix + strlen(suffix);
	str_end = str + strlen(str);

	for (; suffix_end > suffix && str_end > str; suffix_end--, str_end--)
	{
		if (*suffix_end != *str_end)
		{
			return;
		}
	}

	if (str_end > str)
	{
		*str_end = '\0';
	}
}

int
main(int argc, const char *argv[])
{
	char *str;

	if (argc == 1)
	{
		return (EXIT_FAILURE);
	}
	str = basename((char *)argv[1]);
	if (argc > 2)
	{
		remove_suffix(str, (char *)argv[2]);
	}
	puts(str);
	return (EXIT_SUCCESS);
}