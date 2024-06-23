/* This program generates a One-Time Pad (OTP) and encrypts an input file with
it, outputting the encrypted file and the key used to encrypt it. The ciphertext
file will have the same name as the input file but with an ".enc" extension
added. The key file will have the same name as the input file, but with an
".otpkey" extension added. */
#ifndef NOLIBC
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#else

char* strcat(char* dest, char* src)
{
    char* ret = dest;
    while (*dest)
	    dest++;
    while (*dest++ = *src++);
    return ret;
}

#endif

/* We pick a weird size for the buffer used to read the CSPRNG, because, if
there are intentional weaknesses, it is slightly more likely that they were
designed to repeat at binary boundaries, such as 512 bytes, 1024 bytes, etc. */
#define READ_BUFFER_SIZE 497

const char* PROGRAM_NAME = "otp";
const char* USAGE_MSG = "Invalid arguments. Correct usage: otp {file-to-encrypt}\n";
const char* FILE_ERR_MSG = "Error reading /dev/urandom.\n";
const char* NOT_ENOUGH_RAND_MSG = "Did not read enough data from /dev/urandom.\n";
const char* END_OF_RAND_MSG = "Unexpected end of randomness from /dev/urandom.\n";
const char* CSPRNG_SOURCE = "/dev/urandom";
const char* ERR_READING = "Error reading plaintext file";
const char* ERR_WRITING_KEY = "Error writing to key file";
const char* ERR_WRITING_CIP = "Error writing to ciphertext file";

int main(int argc, char *argv[]) {
    if (argc != 2)
    {
        write(STDERR_FILENO, USAGE_MSG, strlen(USAGE_MSG));
        return EXIT_FAILURE;
    }

    int rfd = open(CSPRNG_SOURCE, O_RDONLY); // "rfd" = randomness file descriptor
    if (rfd < 0) { // If the file descriptor is -1, it is an error.
        perror("Error"); // perror() prints the error message indicated by errno.
        return EXIT_FAILURE;
    }
    char* plaintext_file_name = argv[1];

    /* We need two copies of the plaintext file name, because each is going to
    get concatenated with a different extension to produce a new file name. Most
    file systems only allow file paths of up to 255 bytes, so we do not need to
    malloc(). */
    char plaintext_file_name_2[255];
    strcpy(plaintext_file_name_2, plaintext_file_name);

    int pfd = open(argv[1], O_RDONLY); // "pfd" = plaintext file descriptor
    if (pfd < 0) { // If the file descriptor is -1, it is an error.
        perror("Error"); // perror() prints the error message indicated by errno.
        close(rfd); // We ignore any error this could return, because we are exiting anyway.
        return EXIT_FAILURE;
    }

    char* key_file_name = strcat(plaintext_file_name, ".otpkey");
    int kfd = open(key_file_name, O_WRONLY | O_CREAT, 0600); // "kfd" = key file descriptor
    if (kfd < 0) { // If the file descriptor is -1, it is an error.
        perror("Error"); // perror() prints the error message indicated by errno.
        close(pfd); // We ignore any error this could return, because we are exiting anyway.
        close(rfd); // We ignore any error this could return, because we are exiting anyway.
        return EXIT_FAILURE;
    }

    char* out_file_name = strcat(plaintext_file_name_2, ".enc");
    int cfd = open(out_file_name, O_WRONLY | O_CREAT, 0600); // "cfd" = ciphertext file descriptor
    if (cfd < 0) { // If the file descriptor is -1, it is an error.
        perror("Error"); // perror() prints the error message indicated by errno.
        close(kfd); // We ignore any error this could return, because we are exiting anyway.
        close(pfd); // We ignore any error this could return, because we are exiting anyway.
        close(rfd); // We ignore any error this could return, because we are exiting anyway.
        return EXIT_FAILURE;
    }

    int plaintext_bytes_read;
    int csprng_bytes_read;
    int bytes_written;
    char read_buf[READ_BUFFER_SIZE];
    char rand_buf[READ_BUFFER_SIZE];
    char ctxt_buf[READ_BUFFER_SIZE];
    while ((plaintext_bytes_read = read(pfd, read_buf, READ_BUFFER_SIZE)) > 0) {
        csprng_bytes_read = read(rfd, rand_buf, READ_BUFFER_SIZE);
        if (csprng_bytes_read == 0) {
            write(STDERR_FILENO, END_OF_RAND_MSG, strlen(END_OF_RAND_MSG));
            close(pfd); // We ignore any error this could return, because we are exiting anyway.
            close(rfd); // We ignore any error this could return, because we are exiting anyway.
            close(kfd); // We ignore any error this could return, because we are exiting anyway.
            close(cfd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }
        if (csprng_bytes_read < 0) {
            write(STDERR_FILENO, FILE_ERR_MSG, strlen(FILE_ERR_MSG));
            close(pfd); // We ignore any error this could return, because we are exiting anyway.
            close(rfd); // We ignore any error this could return, because we are exiting anyway.
            close(kfd); // We ignore any error this could return, because we are exiting anyway.
            close(cfd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }
        if (csprng_bytes_read != READ_BUFFER_SIZE) {
            write(STDERR_FILENO, NOT_ENOUGH_RAND_MSG, strlen(NOT_ENOUGH_RAND_MSG));
            close(pfd); // We ignore any error this could return, because we are exiting anyway.
            close(rfd); // We ignore any error this could return, because we are exiting anyway.
            close(kfd); // We ignore any error this could return, because we are exiting anyway.
            close(cfd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }

        /* It is possible that fewer bytes will be read from the plaintext file
        than from the cipher text. */
        int len = plaintext_bytes_read < csprng_bytes_read
            ? plaintext_bytes_read
            : csprng_bytes_read;

        // XOR the plaintext with the key.
        for (int i = 0; i < len; i++) {
            ctxt_buf[i] = read_buf[i] ^ rand_buf[i];
        }

        bytes_written = write(kfd, &rand_buf, len);
        if (bytes_written <= 0) {
            perror(ERR_WRITING_KEY);
            close(pfd); // We ignore any error this could return, because we are exiting anyway.
            close(rfd); // We ignore any error this could return, because we are exiting anyway.
            close(kfd); // We ignore any error this could return, because we are exiting anyway.
            close(cfd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }
        bytes_written = write(cfd, &ctxt_buf, len);
        if (bytes_written <= 0) {
            perror(ERR_WRITING_CIP);
            close(pfd); // We ignore any error this could return, because we are exiting anyway.
            close(rfd); // We ignore any error this could return, because we are exiting anyway.
            close(kfd); // We ignore any error this could return, because we are exiting anyway.
            close(cfd); // We ignore any error this could return, because we are exiting anyway.
            return EXIT_FAILURE;
        }
    }
    if (plaintext_bytes_read < 0) {
        perror(ERR_READING); // perror() prints the error message indicated by errno.
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}