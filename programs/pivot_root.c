#ifndef NOLIBC
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Defined in nolibc, but not in glibc.
int pivot_root(
    const char *new_root,
    const char *put_old
) {
    return syscall(__NR_pivot_root, new_root, put_old);
}

#endif

const char* USAGE_MSG = "Usage: pivot_root {new_root} {put_old}";

int main (int argc, char** argv) {
    if (argc != 3) {
        puts(USAGE_MSG);
        return EXIT_FAILURE;
    }
    int rc = pivot_root(argv[1], argv[2]);
    if (rc != 0) {
        perror("mount");
        return rc;
    }
    return EXIT_SUCCESS;
}
