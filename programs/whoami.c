#ifndef NOLIBC
#include <stdlib.h>
#include <sys/acct.h>
#include <stdio.h>
#include <unistd.h>
#endif

int main (int argc, char **argv) {
    uid_t uid = geteuid();
    if (uid == -1) {
        return EXIT_FAILURE;
    }
    if (printf("%d\n", uid) <= 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
