/*
This is an abandoned attempt to create a barely-functioning text editor that
uses nolibc and does not perform any heap allocations. I wanted a program
whose code was so simple that it could be easily inspected and understood, but
in the end, I decided that I would use the code of the kilo editor and pare
down unwanted features and port it to nolibc. There's just not a good way to
write a text editor that doesn't use the heap: even a minimally functioning
one.

But for posterity's sake, here was how this was planned to work:

In main(), there would be an instantiated `editor` struct which would
contain the editor state. It would store the entire file contents in
compile-time stack memory, and there would also be, say, an array for 50
patches to the saved file. These patches would comprise a change type, and
the diff itself, which is a fixed-length character array. Upon saving, these
patches would be "squashed" just like Git commits are, and the final output
saved, and reloaded into the "saved file buffer."

These patches would be inserted into the array in the order that they are
written, but they would have pointer fields so as to construct a singly-linked
list whose nodes are ordered by index. Using this method, the file can by
displayed by writing the unchanged file contents to stdout, interleaved with
the changes processed by traversing the singly-linked list they form. This
methodology might have been successful if I had continued, but it required
nearly starting from scratch since it is so wildly different. There are so
many pitfalls with this approach that it simply wasn't worth it.

Copyright (c) 2024 Jonathan M. Wilbur.
Released under an MIT License.
*/
#ifndef NOLIBC
#include <sys/types.h>
#include <stdio.h>
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#endif

#define BUFSIZ 8192

// This can be customized to control stack size
// Stack size is largely NUM_CHUNKS * CHUNK_SIZE
#ifndef NUM_CHUNKS
#define NUM_CHUNKS 100
#endif

#define CHUNK_SIZE BUFSIZ

#ifndef MAX_FILE_SIZE
#define MAX_FILE_SIZE 50000
#endif

#define NUM_COLS 80

#define TERM_ROWS 24
#define TERM_COLS 80

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

// #define O_RDWR	    02
// #define O_CREAT	    0100

#define EXIT_SUCCESS    0
#define EXIT_USAGE      1
#define EXIT_OPEN       2
#define EXIT_GET_TERM   3
#define EXIT_SET_TERM   4
#define EXIT_READ       5
#define EXIT_RENDER     6
#define EXIT_INPUT      7
#define EXIT_CLEAR      8

#define BRKINT      0000002
#define INPCK       0000020
#define ISTRIP      0000040
#define ICRNL       0000400
#define IXON        0002000

#define OPOST       0000001
#define CS8         0000060

#define ICANON      0000002
#define ECHO        0000010
#define IEXTEN      0100000
#define ISIG        0000001

#define VMIN        6
#define VTIME       5

// I'm not even really sure this will work. This is for i386...
// #define TCGETS  0x00005401
// #define TCSETAF 0x00005408

#define ONLRET      0000040
#define ONLCR       0000004

#define CTRL_KEY(k) ((k) & 0x1f)

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 32
struct termios
  {
    tcflag_t c_iflag;		/* input mode flags */
    tcflag_t c_oflag;		/* output mode flags */
    tcflag_t c_cflag;		/* control mode flags */
    tcflag_t c_lflag;		/* local mode flags */
    cc_t c_line;			/* line discipline */
    cc_t c_cc[NCCS];		/* control characters */
    speed_t c_ispeed;		/* input speed */
    speed_t c_ospeed;		/* output speed */
#define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
#define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
};

const char* USAGE_MSG = "Usage: edit {file}";

#define PATCH_TYPE_ADD 0
#define PATCH_TYPE_DEL 1
typedef struct patch {
    size_t start;
    int type;
    union {
        char add[TERM_COLS + 2];
        size_t del;
    } d;
    struct patch *next;
} patch;

typedef struct editor {
    // char rows[NUM_ROWS][NUM_COLS];
    // char file_chunks[NUM_CHUNKS][CHUNK_SIZE];
    char saved_data[MAX_FILE_SIZE];
    size_t file_size;
    int rowpos;
    int colpos;
    int fd;
    struct termios orig_term; // For restoring upon exit.
    int dirty;
    patch changes[100]; // Only has entries if ed->dirty is 1.
} editor;

int tcgetattr (int fd, struct termios *term) {
    return ioctl(fd, TCGETS, term);
}

int set_tty (int fd, struct termios *termios_p) {
    /* TCSETSF = change terminal settings after flushing */
    /* TCSETAF = change terminal settings after flushing (legacy version) */
    return ioctl(fd, TCSETSF, termios_p);
}

int configure_tty (int fd, struct termios *curr_term) {
    struct termios raw = *curr_term; /* Modify by value, not reference. */

    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // NOTE: this differs from kilo. I am not sure what the implications are.
    /* output modes - disable post processing */
    // raw.c_oflag &= ~(OPOST);
    // raw.c_oflag |= ONLCR;

    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - echoing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */
    return set_tty(fd, &raw);
}

#define MIN(a, b) a <= b ? a : b

// Returns non-zero on error.
int render (struct editor *ed) {
    int write_rc;
    size_t write_size;
    patch* next_change;
    size_t i; // Index into the original file

    /* If no changes, just display the old file. */
    if (ed->dirty == 0) {
        write_rc = write(STDOUT_FILENO, &ed->saved_data, ed->file_size);
        if (write_rc == -1) {
            return -1;
        }
        return 0;
    }

    next_change = &ed->changes[0];
    i = 0;

    // TODO: I just realized this is not keeping output within line limits.
    while (next_change != NULL) {
        write_size = next_change->start - i;
        if (write_size > 0) {
            write_rc = write(STDOUT_FILENO, &ed->saved_data[i], write_size);
            if (write_rc == -1) {
                return -1;
            }
            i += write_rc;
        }

        if (next_change->type == PATCH_TYPE_ADD) {
            write_size = strlen(next_change->d.add);
            write_rc = write(STDOUT_FILENO, next_change->d.add, write_size);
            if (write_rc == -1) {
                return -1;
            }
        } else if (next_change->type == PATCH_TYPE_DEL) {
            i += next_change->d.del;
        }
        next_change = next_change->next;
    }

    write_size = ed->file_size - i;
    if (write_size > 0) {
        write_rc = write(STDOUT_FILENO, &ed->saved_data[i], write_size);
        if (write_rc == -1) {
            return -1;
        }
        i += write_rc;
    }

    return 0;
}

char read_keystroke () {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            // TODO: exit() instead?
            return 0;
        }
    }
    return c;
}

int main(int argc, char **argv) {
    /* Why yes, this large data structure is allocated on the stack. Sue me! */
    editor ed;
    ssize_t n;
    char inputc = '\0';

    if (argc != 2) {
        puts(USAGE_MSG);
        return EXIT_USAGE;
    }
    ed.dirty = 0;
    ed.rowpos = 0;
    ed.colpos = 0;
    char* file_name = argv[1];

    ed.fd = open(file_name, O_RDWR | O_CREAT);
    if (ed.fd == -1) {
        return EXIT_OPEN;
    }

    // Get current terminal settings
    if (tcgetattr(STDIN_FILENO, &ed.orig_term) == -1) {
        if (errno == EBADF) {
            puts("Bad file descriptor\n");
        }
        if (errno == EFAULT) {
            puts("Inaccessible memory\n");
        }
        if (errno == ENOTTY) {
            puts("Not a terminal\n");
        }
        if (errno == EINVAL) {
            puts("Invalid ioctl() argument\n");
        }
        return EXIT_GET_TERM;
    }

    // Sets Raw Mode, among other things
    if (configure_tty(STDIN_FILENO, &ed.orig_term) != 0)
        return EXIT_SET_TERM;

    // Populate the original buffer.
    n = read(ed.fd, &ed.saved_data, MAX_FILE_SIZE);
    if (n == -1)
        return EXIT_READ;
    ed.file_size = n;
    
    while (1) {
        // Clear screen
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        // if (puts("\e[1;1H\e[2J") == EOF) {
        //     return EXIT_CLEAR;
        // }
        // if (puts("\x1b[?25l") == EOF) {
        //     return EXIT_CLEAR;
        // }

        if (render(&ed)) {
            return EXIT_RENDER;
        }

        // Wait for keystrokes
        inputc = read_keystroke();
        if (inputc == 0) {
            return EXIT_INPUT;
        }
        if (inputc == CTRL_KEY('q')) break;
    };
    
    if (set_tty(STDIN_FILENO, &ed.orig_term) != 0)
        return EXIT_SET_TERM;

    // FIXME: Seems like the cursor is still hidden after this.
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    return EXIT_SUCCESS;
}