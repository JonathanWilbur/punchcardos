#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc,char* argv[])
{
	if (argc != 2)
	{
		puts("usage: rmdir {dir-to-delete}");
		return EXIT_FAILURE;
	}
	if (rmdir(argv[1]))
	{
		perror("rmdir");
        return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}