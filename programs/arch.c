#ifndef NOLIBC
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#else

#include <sys/utsname.h>

int uname (struct utsname *buf) {
    return my_syscall1(__NR_uname, buf);
}

#endif

int main () {
	struct utsname u;
	if (uname(&u) < 0) {
		perror("uname");
		return EXIT_FAILURE;
	}
    puts(u.machine);
	return EXIT_SUCCESS;
}
