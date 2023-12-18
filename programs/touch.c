#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char* argv[])
{
	if (argc < 2)
	{
		puts("touch {file-name}");	
		return EXIT_FAILURE;
	}
	
	for (int i = 1; i < argc; i++)
	{	
		if (!access(argv[i], F_OK))
		{
			if (utime(argv[i], NULL) != 0)
            {
				perror("touch");
                return EXIT_FAILURE;
            }
		}
		else
		{	
			int fd = open(argv[i], O_CREAT | O_RDWR, 0777);
			if (fd > 0)
            {
				perror("touch");
                return EXIT_FAILURE;
            }
		}
	}

	return EXIT_SUCCESS;
}