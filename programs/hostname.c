/*
This hyper-minimal implementation of the hostname command only operates on the
short hostname: no FQDNs, YP, NIS, IP addresses etc. are supported.

The license (MIT) for this program is commented out at the bottom of this file.
*/
#include <limits.h>
#ifndef NOLIBC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#else

static long __syscall_ret (unsigned long r) {
	if (r > -4096UL) {
		errno = -r;
		return -1;
	}
	return r;
}

// There is no gethostname system call. You use uname for this.
int gethostname (char *name, size_t len) {
    struct utsname buf;
    if (uname(&buf) != 0)
        return -1;
    // nodename is null-terminated, so this should be fine.
    strncpy(name, buf.nodename, len);
    return 0;
}

int sethostname (const char *name, size_t len) {
    return __syscall_ret(my_syscall2(__NR_sethostname, name, len));
}

#endif

const char *USAGE = "hostname [desired hostname]";
static void print_usage () {
    puts(USAGE);
}

int main (int argc, char **argv) {
    char *arg;
    char *desired_name = NULL;
    char hostname[HOST_NAME_MAX + 1];
    hostname[HOST_NAME_MAX] = 0;

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;
        if (!strcmp(arg, "--short") || !strcmp(arg, "-s"))
            continue; // This is what it does anyway.
        if (arg[0] == '-') { // Anything else is not understood.
            printf("Option not understood: %s\n", arg);
            print_usage();
            return EXIT_FAILURE;
        }
        if (desired_name != NULL) { // The host name was already chosen.
            print_usage();
            return EXIT_FAILURE;
        }
        desired_name = arg;
    }

    if (desired_name != NULL) {
        return sethostname(desired_name, strlen(desired_name)) == 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    } else {
        if (gethostname(hostname, HOST_NAME_MAX) != 0) {
            perror("hostname");
            return EXIT_FAILURE;
        }
        if (puts(hostname) == EOF)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
}

/*
MIT License

Copyright (c) 2024 Jonathan M. Wilbur <jonathan@wilbur.space>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/