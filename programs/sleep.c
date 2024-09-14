#ifndef NOLIBC
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#endif

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		return EXIT_FAILURE;
	}
    int seconds = atoi(argv[1]);
    if (seconds == 0)
    {
        return EXIT_FAILURE;
    }
#ifndef NOLIBC
	signal(SIGALRM, SIG_IGN);
#endif
	sleep(seconds);
	return EXIT_SUCCESS;
}
