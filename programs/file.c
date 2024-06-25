#ifndef NOLIBC
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#endif

int main (int argc, char* argv[])
{
	struct stat sb;

	if (argc != 2)
	{
		puts("usage: file {file-name}");
		return EXIT_FAILURE;
	}

	if (stat(argv[1], &sb) == -1)
	{
		perror("stat");
		return EXIT_FAILURE;
	}

	printf("%s: ", argv[1]);
    if (S_ISREG(sb.st_mode)) {
        puts("regular file");
    }
    else if (S_ISDIR(sb.st_mode)) {
        puts("directory");
    }
    else if (S_ISCHR(sb.st_mode)) {
        puts("character device");
    }
    else if (S_ISBLK(sb.st_mode)) {
        puts("block device");
    }
    else if (S_ISFIFO(sb.st_mode)) {
        puts("fifo / pipe");
    }
    else if (S_ISLNK(sb.st_mode)) {
        puts("symlink");
    }
    else if (S_ISSOCK(sb.st_mode)) {
        puts("socket");
    }
    else {
        puts("unknown");
    }
	return EXIT_SUCCESS;
}

