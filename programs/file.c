#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char* argv[])
{
	struct stat sb;

	if (argc != 2) 
	{
		puts("usage: file {file-name}");
		return EXIT_FAILURE;
	}

	if (lstat(argv[1], &sb) == -1)
	{
		perror("stat");
		return EXIT_FAILURE;
	}

	printf("File type: ");
	switch (sb.st_mode & S_IFMT)
	{
		case S_IFBLK:
            printf("block device\n");      
            break;
		case S_IFCHR:
            printf("character device\n");    
            break;
		case S_IFDIR:
            printf("directory\n");             
            break;
		case S_IFIFO:
            printf("fifo / pipe\n");           
            break;
		case S_IFLNK:
            printf("symlink\n");             
            break;
		case S_IFREG:
            printf("regular file\n");      
            break;
		case S_IFSOCK:
            printf("socket\n");               
            break;
		default:
            printf("unknown\n");                
            break;
	}
	return EXIT_SUCCESS;
}

