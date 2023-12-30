#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

const char* HTTP_VERB = "POST";
const char* CANNOT_OPEN_FILE_MSG = "cannot open file\n";
const char* CANNOT_WRITE_ERR_MSG = "write error\n";

int error_writing () {
    perror("Could not write to remote host");
    return EXIT_FAILURE;
}

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

int main (int argc, char **argv) {
    int fd;
    int sockfd;
    int port;
    int n;
    struct sockaddr_in addr;
    struct hostent *server;
    char *filename;
    char *hostname;
    char *path;

    if (argc != 5)
    {
       fprintf(stderr, "usage: %s <file> <hostname> <port> <path>\n", argv[0]);
       return EXIT_FAILURE;
    }

    filename = argv[1];
    hostname = argv[2];
    port = atoi(argv[3]);
    path = argv[4];

    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        write(STDERR_FILENO, CANNOT_OPEN_FILE_MSG, sizeof(CANNOT_OPEN_FILE_MSG));
        return EXIT_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Could not open socket");
        return EXIT_FAILURE;
    }

    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "Could not resolve host: %s\n", hostname);
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        perror("Could not connect to host");
        return EXIT_FAILURE;
    }

    struct stat buf;
    fstat(fd, &buf);
    off_t size = buf.st_size;

    FILE *sockfp = fdopen(sockfd, "wb+");
    if (sockfp == NULL) {
        perror("File descriptor error");
        return EXIT_FAILURE;
    }
    n = fprintf(
        sockfp,
        "%s %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n",
        HTTP_VERB,
        path,
        hostname,
        size
    );
    if (n < 0)
    {
        return error_writing();
    }
    fflush(sockfp);
    
    if (filecopy(fd, sockfd) < 0) {
        return EXIT_FAILURE;
    }
    
    // I don't actually know if this is necessary.
    sleep(1);

    close(sockfd);
    return 0;
}