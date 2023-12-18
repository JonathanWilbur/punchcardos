#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

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
	signal(SIGALRM, SIG_IGN);
	sleep(seconds);
	return EXIT_SUCCESS;
}
