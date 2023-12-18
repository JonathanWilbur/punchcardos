#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

int ls(const char *dir,int op_a,int op_l)
{
	struct dirent *d;
	DIR *dir_list = opendir(dir);
	if (!dir_list)
	{
		if (errno = ENOENT)
		{
			perror("No such directory");
		}
		else
		{
            // TODO: Is there some way you can print the error message?
			perror("Unable to read directory");
		}
		exit(EXIT_FAILURE);
	}

    /* Every time you call readdir on the list, it returns the next directory
    entry, or NULL if there are no more. */
	while ((d = readdir(dir_list)) != NULL)
	{
		// Skip over hidden files.
		if (!op_a && d->d_name[0] == '.')
        {
            continue;
        }
		printf("%s  ", d->d_name);
		if (op_l)
        {
            printf("\n");
        }
	}
	if (!op_l) {
	    printf("\n");
    }
    return EXIT_SUCCESS;
}

// TODO: Include help option.
int main(int argc, const char *argv[])
{
	if (argc == 1)
	{
		return ls(".", 0, 0);
	}
	else if (argc == 2)
	{
		if (argv[1][0] == '-')
		{
			int op_a = 0, op_l = 0;
			char *p = (char*)(argv[1] + 1);
			while(*p) {
				if (*p == 'a')
                {
                    op_a = 1;
                }
				else if (*p == 'l')
                {
                    op_l = 1;
                }
				else
                {
					perror("Unrecognized option");
					exit(EXIT_FAILURE);
				}
				p++;
			}
			return ls(".", op_a, op_l);
		}
	}
    // TODO: Handle multiple arguments.
	return EXIT_SUCCESS;
}