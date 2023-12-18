#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

const char* CANNOT_OPEN_FILE_MSG = "cannot open file\n";
const char* CANNOT_WRITE_ERR_MSG = "write error\n";

int filecopy (int ifd, int ofd) {
    ssize_t n;
    char buf[BUFSIZ];
    while ((n = read(ifd, buf, BUFSIZ)) > 0) {
        if (write(ofd, buf, (size_t)n) != n) {
            write(STDERR_FILENO, CANNOT_WRITE_ERR_MSG, sizeof(CANNOT_WRITE_ERR_MSG));
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    int fd;
    if (argc == 1) {
        return filecopy(STDIN_FILENO, STDOUT_FILENO);
    }
    while (--argc > 0) {
        if ((fd = open(*++argv, O_RDONLY)) == -1) {
            write(STDERR_FILENO, CANNOT_OPEN_FILE_MSG, sizeof(CANNOT_OPEN_FILE_MSG));
        }
        else
        {
            int rc = filecopy(fd, STDOUT_FILENO);
            (void)close(fd);
            return rc;
        }
    }
    return EXIT_SUCCESS;
}