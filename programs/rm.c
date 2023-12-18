#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char* argv[])
{
	if (argc == 1)
	{
		puts("usage: rm {file-path-to-delete}");
		return EXIT_FAILURE;
	}

	if (access(argv[1], F_OK) != 0)
    { // The file does not exist.
		perror("rm");
        return EXIT_FAILURE;
    }

    if (unlink(argv[1]) != 0)
    {
        perror("rm");
        return EXIT_FAILURE;
    }
	
	return EXIT_SUCCESS;
}