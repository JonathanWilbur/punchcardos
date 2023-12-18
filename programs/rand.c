/* This program generates a specified number of random bytes and appends them to
a specified file, creating the output file if it does not exist. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

/* We pick a weird size for the buffer used to read the CSPRNG, because, if
there are intentional weaknesses, it is slightly more likely that they were
designed to repeat at binary boundaries, such as 512 bytes, 1024 bytes, etc. */
#define READ_BUFFER_SIZE 497

const char* PROGRAM_NAME = "rand";
const char* USAGE_MSG = "Invalid arguments. Correct usage: rand {number-of-bytes} {output-file-path}\n";
const char* FILE_ERR_MSG = "Error reading /dev/urandom.\n";
const char* END_OF_RAND_MSG = "Unexpected end of randomness from /dev/urandom.\n";
const char* CSPRNG_SOURCE = "/dev/urandom";
const char* ERR_WRITING = "Error writing to output file";

int main(int argc, char *argv[]) {
    if (argc != 3)
    {
        write(STDERR_FILENO, USAGE_MSG, strlen(USAGE_MSG));
        return EXIT_FAILURE;
    }
    // atoi() = Parse an integer from bytes, returning 0 from an invalid string.
    int bytes_left_to_read = atoi(argv[1]);

    /* Zero is either a sign that the second argument was invalid, or the user
    actually chose zero bytes, which is invalid. Either way display usage and
    exit. */
    if (bytes_left_to_read == 0) {
        write(STDERR_FILENO, USAGE_MSG, strlen(USAGE_MSG));
        return EXIT_FAILURE;
    }

    int ifd = open(CSPRNG_SOURCE, O_RDONLY);
    if (ifd < 0) { // If the file descriptor is -1, it is an error.
        perror("Error"); // perror() prints the error message indicated by errno.
        return EXIT_FAILURE;
    }
    int ofd = open(argv[2], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (ofd < 0) { // If the file descriptor is -1, it is an error.
        perror("Error"); // perror() prints the error message indicated by errno.
        close(ifd); // We ignore any error this could return, because we are exiting anyway.
        return EXIT_FAILURE;
    }

    char read_buf[READ_BUFFER_SIZE];
    while (bytes_left_to_read > 0) {
        ssize_t bytes_read = read(ifd, &read_buf, READ_BUFFER_SIZE);
        if (bytes_read < 0) { // -1 means the file does not exist or some other error.
            write(STDERR_FILENO, FILE_ERR_MSG, strlen(FILE_ERR_MSG));
            close(ifd); // We ignore any error this could return, because we are exiting anyway.
            close(ofd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }
        // 0 means that we reached the end of the file, which should never happen with /dev/urandom
        if (bytes_read == 0) {
            write(STDERR_FILENO, END_OF_RAND_MSG, strlen(END_OF_RAND_MSG));
            close(ifd); // We ignore any error this could return, because we are exiting anyway.
            close(ofd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }
        if (bytes_left_to_read > READ_BUFFER_SIZE) {
            ssize_t bytes_written = write(ofd, &read_buf, READ_BUFFER_SIZE);
            if (bytes_written < 0) {
                perror(ERR_WRITING);
                close(ifd); // We ignore any error this could return, because we are exiting anyway.
                close(ofd); // We ignore any error this could return, because we are exiting anyway.
                return EXIT_FAILURE;
            }
            bytes_left_to_read -= READ_BUFFER_SIZE;
        } else {
            ssize_t bytes_written = write(ofd, &read_buf, bytes_left_to_read);
            if (bytes_written < 0) {
                perror(ERR_WRITING);
                close(ifd); // We ignore any error this could return, because we are exiting anyway.
                close(ofd); // We ignore any error this could return, because we are exiting anyway.
                return EXIT_FAILURE;
            }
            // bytes_left_to_read -= bytes_left_to_read
            break;
        }
    }
    return EXIT_SUCCESS;
}