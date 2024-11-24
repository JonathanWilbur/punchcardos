#ifndef NOLIBC
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#else

#define	F_OK	0		/* Test for existence.  */

struct utimbuf {
    time_t actime; /* Access time.  */
    time_t modtime; /* Modification time.  */
};

int access (const char *pathname, int mode) {
    return __sysret(my_syscall2(__NR_access, pathname, mode));
}

int utime(const char *filename, const struct utimbuf *times) {
    return __sysret(my_syscall2(__NR_utime, filename, times));
}

#endif

int main (int argc, char **argv) {
	if (argc < 2) {
		puts("touch {file-name}");	
		return EXIT_FAILURE;
	}
	
	for (int i = 1; i < argc; i++) {	
		if (!access(argv[i], F_OK)) {
			if (utime(argv[i], NULL) != 0) {
				perror("touch");
                return EXIT_FAILURE;
            }
		} else {	
			int fd = open(argv[i], O_CREAT | O_RDWR, 0644);
			if (fd < 0) {
				perror("touch");
                return EXIT_FAILURE;
            }
            close(fd);
		}
	}

	return EXIT_SUCCESS;
}