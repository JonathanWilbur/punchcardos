#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char* argv[])
{
	if (argc != 2)
	{
		puts("usage: mkdir {dir-name}");
		return EXIT_FAILURE;
	}
	if (mkdir(argv[1], 0775))
    {
		perror("mkdir");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}