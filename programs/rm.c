#ifndef NOLIBC
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#endif

/* This does not support the -r option, because that would require a LOT more
code. The -f option is implied. */
int main (int argc, char* argv[])
{
	if (argc == 1)
	{
		puts("usage: rm {file-path-to-delete}");
		return EXIT_FAILURE;
	}

    #ifndef NOLIBC
	if (access(argv[1], F_OK) != 0)
    { // The file does not exist.
		perror("rm");
        return EXIT_FAILURE;
    }
    #endif

    /* The unlink system call is how you delete a file on Linux. */
    if (unlink(argv[1]) != 0)
    {
        perror("rm");
        return EXIT_FAILURE;
    }
	
	return EXIT_SUCCESS;
}