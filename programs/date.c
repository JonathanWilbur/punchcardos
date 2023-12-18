#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main ()
{
	long now, time();
	char *ctime();
	time (&now);
	printf("%s", ctime (&now));
	return EXIT_SUCCESS;
}