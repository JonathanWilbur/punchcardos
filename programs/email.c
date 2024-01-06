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

const char* CANNOT_OPEN_FILE_MSG = "cannot open file\n";
const char* CANNOT_WRITE_ERR_MSG = "write error\n";
const char* HELO_CMD = "HELO goofy.zip\r\n";

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

int prefix(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

int main () {
    // Not implemented.
    return 1;
}

// int main (int argc, char **argv) {
//     int fd;
//     int sockfd;
//     int port;
//     int n;
//     struct sockaddr_in addr;
//     struct hostent *server;
//     char *filename;
//     char *from_email;
//     char *to_email;
//     char *host;
//     char *port;
//     char *buf[BUFSIZ];

//     if (argc != 5)
//     {
//        fprintf(stderr, "usage: %s <file> <from> <to> <hostname> <port>\n", argv[0]);
//        return EXIT_FAILURE;
//     }

//     filename = argv[1];
//     from_email = argv[2];
//     to_email = argv[3];
//     host = argv[4];
//     port = argv[5];

//     if ((fd = open(filename, O_RDONLY)) == -1)
//     {
//         write(STDERR_FILENO, CANNOT_OPEN_FILE_MSG, sizeof(CANNOT_OPEN_FILE_MSG));
//         return EXIT_FAILURE;
//     }

//     sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd < 0)
//     {
//         perror("Could not open socket");
//         return EXIT_FAILURE;
//     }

//     server = gethostbyname(host);
//     if (server == NULL)
//     {
//         fprintf(stderr, "Could not resolve host: %s\n", host);
//         return EXIT_FAILURE;
//     }

//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
//     addr.sin_port = htons(port);

//     if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
//     {
//         perror("Could not connect to host");
//         return EXIT_FAILURE;
//     }

//     struct stat sbuf;
//     fstat(fd, &sbuf);
//     off_t size = sbuf.st_size;

//     FILE *sockfp = fdopen(sockfd, "wb+");
//     if (sockfp == NULL) {
//         perror("File descriptor error");
//         return EXIT_FAILURE;
//     }

//     // S: 220 smtp.example.com ESMTP Postfix
//     // C: HELO relay.example.org
//     // S: 250 Hello relay.example.org, I am glad to meet you
//     // C: MAIL FROM:<bob@example.org>
//     // S: 250 Ok
//     // C: RCPT TO:<alice@example.com>
//     // S: 250 Ok
//     // C: RCPT TO:<theboss@example.com>
//     // S: 250 Ok
//     // C: DATA
//     // S: 354 End data with <CR><LF>.<CR><LF>
//     fgets(&buf[0], sizeof(buf), sockfp);
//     if (strlen(&buf[0]) < 4)
//         return EXIT_FAILURE;
//     if (puts(&buf[0]) < 0)
//         return EXIT_FAILURE;
//     if (!prefix("220 ", &buf[0]))
//         return EXIT_FAILURE;
//     if (write(sockfd, HELO_CMD, strlen(HELO_CMD)) < strlen(HELO_CMD))
//         return EXIT_FAILURE;

//     fgets(buf, sizeof(buf), sockfp);
//     if (strlen(&buf[0]) < 4)
//         return EXIT_FAILURE;
//     if (puts(&buf[0]) < 0)
//         return EXIT_FAILURE;
//     if (!prefix("250 ", &buf[0]))
//         return EXIT_FAILURE;
//     if (fprintf(sockfp, "MAIL FROM:<%s>\n", from_email) < 0)
//         return EXIT_FAILURE;
//     if (fflush(sockfp) == EOF)
//         return EXIT_FAILURE;

//     print("mailed");
    
//     if (filecopy(fd, sockfd) < 0) {
//         return EXIT_FAILURE;
//     }
    
//     // I don't actually know if this is necessary.
//     sleep(1);

//     close(sockfd);
//     return 0;
// }