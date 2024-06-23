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
#include <libgen.h>
#else

/* This is probably not a correct implementation somehow, but it is surely
"close enough." */
static char *dirname (char *path) {
    int len = strlen(path);

    // Real dirname returns a period if supplied an empty string.
    if (len == 0) {
        return ".";
    }

    int i = len - 1;
    int prev_was_slash = 0;
    for (; i >= 0; i--) {
        if (path[i] == '/') {
            goto is_dir;
        }
    }

    // Real dirname returns a period if there are no slashes.
    return ".";

    is_dir:
    for (; i >= 0; i--) {
        if (path[i] != '/') {
            break;
        }
        path[i] = 0;
    }

    // If the string was all slashes, return a single slash.
    if (strlen(path) == 0) {
        return "/";
    }

    return path;
}

#endif

int
main(int argc, const char *argv[])
{
	char *str;

	if (argc == 1)
		return (EXIT_FAILURE);
	str = dirname((char *)argv[1]);
	puts(str);
	return (EXIT_SUCCESS);
}