#ifndef NOLIBC
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

static void print_usage () {
    puts("usage: mkdir [-p] {dir-name}");
}

int main (int argc, char **argv)
{
    int parents = 0;
    char *arg;
    char *dir;
    int x;
    int dir_name_len;

	if (argc < 2) {
		print_usage();
		return EXIT_FAILURE;
	}
    dir = argv[argc - 1];

    for (int i = 1; i < argc - 1; i++) {
        arg = argv[i];
        if (!strcmp(arg, "-p") || !strcmp(arg, "--parents")) {
            parents = 1;
        }
        else {
            print_usage();
            return EXIT_FAILURE;
        }
    }

    if (!parents) {
        if (mkdir(dir, 0775)) {
            perror("mkdir");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    dir_name_len = strlen(dir);
    x = (dir[0] == '/'); // 1 if true.
    for (; x < strlen(dir); x++) {
        if (dir[x] != '/')
            continue;
        dir[x] = 0;
        if (mkdir(dir, 0775) && errno != EEXIST) {
            perror("mkdir");
            return EXIT_FAILURE;
        }
        dir[x] = '/';
    }
    if (mkdir(dir, 0775) && errno != EEXIST) {
        perror("mkdir");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}