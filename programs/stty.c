#ifndef NOLIBC
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#else

/* `c_cc' member of 'struct termios' structure can be disabled by
   using the value _POSIX_VDISABLE.  */
#define _POSIX_VDISABLE '\0'

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

int tcsetattr(int fd, int optional_actions, struct termios *termios_p) {
    /* TCSETSF = change terminal settings after flushing */
    /* TCSETAF = change terminal settings after flushing (legacy version) */
    return ioctl(fd, TCSETSF, termios_p);
}

#define CBAUD	000000010017
#define CBAUDEX 000000010000

int tcgetattr (int fd, struct termios *term) {
    int rc = ioctl(fd, TCGETS, term);
    term->c_ispeed = term->c_cflag & (CBAUD | CBAUDEX);
    term->c_ospeed = term->c_cflag & (CBAUD | CBAUDEX);
    return rc;
}

/* c_cflag bit meaning */
#define  B0	        0000000		/* hang up */
#define  B50	    0000001
#define  B75	    0000002
#define  B110	    0000003
#define  B134	    0000004
#define  B150	    0000005
#define  B200	    0000006
#define  B300	    0000007
#define  B600	    0000010
#define  B1200	    0000011
#define  B1800	    0000012
#define  B2400	    0000013
#define  B4800	    0000014
#define  B9600	    0000015
#define  B19200	    0000016
#define  B38400	    0000017
#define  B57600     0010001
#define  B115200    0010002
#define  B230400    0010003
#define  B460800    0010004
#define  B500000    0010005
#define  B576000    0010006
#define  B921600    0010007
#define  B1000000   0010010
#define  B1152000   0010011
#define  B1500000   0010012
#define  B2000000   0010013
#define  B2500000   0010014
#define  B3000000   0010015
#define  B3500000   0010016
#define  B4000000   0010017
#define __MAX_BAUD  B4000000

/* c_cflag bits.  */
#define CSIZE	0000060
#define CS5	    0000000
#define CS6	    0000020
#define CS7	    0000040
#define CS8	    0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define PARENB	0000400
#define PARODD	0001000
#define HUPCL	0002000
#define CLOCAL	0004000

/* c_cc characters */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_iflag bits */
#define IGNBRK	0000001  /* Ignore break condition.  */
#define BRKINT	0000002  /* Signal interrupt on break.  */
#define IGNPAR	0000004  /* Ignore characters with parity errors.  */
#define PARMRK	0000010  /* Mark parity and framing errors.  */
#define INPCK	0000020  /* Enable input parity check.  */
#define ISTRIP	0000040  /* Strip 8th bit off characters.  */
#define INLCR	0000100  /* Map NL to CR on input.  */
#define IGNCR	0000200  /* Ignore CR.  */
#define ICRNL	0000400  /* Map CR to NL on input.  */
#define IUCLC	0001000  /* Map uppercase characters to lowercase on input (not in POSIX).  */
#define IXON	0002000  /* Enable start/stop output control.  */
#define IXANY	0004000  /* Enable any character to restart output.  */
#define IXOFF	0010000  /* Enable start/stop input control.  */
#define IMAXBEL	0020000  /* Ring bell when input queue is full (not in POSIX).  */
#define IUTF8	0040000  /* Input is UTF8 (not in POSIX).  */

/* c_oflag bits */
#define OPOST	0000001  /* Post-process output.  */
#define OLCUC	0000002  /* Map lowercase characters to uppercase on output. (not in POSIX).  */
#define ONLCR	0000004  /* Map NL to CR-NL on output.  */
#define OCRNL	0000010  /* Map CR to NL on output.  */
#define ONOCR	0000020  /* No CR output at column 0.  */
#define ONLRET	0000040  /* NL performs CR function.  */
#define OFILL	0000100  /* Use fill characters for delay.  */
#define OFDEL	0000200  /* Fill is DEL.  */
#if defined __USE_MISC || defined __USE_XOPEN
# define NLDLY	0000400  /* Select newline delays:  */
# define   NL0	0000000  /* Newline type 0.  */
# define   NL1	0000400  /* Newline type 1.  */
# define CRDLY	0003000  /* Select carriage-return delays:  */
# define   CR0	0000000  /* Carriage-return delay type 0.  */
# define   CR1	0001000  /* Carriage-return delay type 1.  */
# define   CR2	0002000  /* Carriage-return delay type 2.  */
# define   CR3	0003000  /* Carriage-return delay type 3.  */
# define TABDLY	0014000  /* Select horizontal-tab delays:  */
# define   TAB0	0000000  /* Horizontal-tab delay type 0.  */
# define   TAB1	0004000  /* Horizontal-tab delay type 1.  */
# define   TAB2	0010000  /* Horizontal-tab delay type 2.  */
# define   TAB3	0014000  /* Expand tabs to spaces.  */
# define BSDLY	0020000  /* Select backspace delays:  */
# define   BS0	0000000  /* Backspace-delay type 0.  */
# define   BS1	0020000  /* Backspace-delay type 1.  */
# define FFDLY	0100000  /* Select form-feed delays:  */
# define   FF0	0000000  /* Form-feed delay type 0.  */
# define   FF1	0100000  /* Form-feed delay type 1.  */
#endif
#define VTDLY	0040000  /* Select vertical-tab delays:  */
#define   VT0	0000000  /* Vertical-tab delay type 0.  */
#define   VT1	0040000  /* Vertical-tab delay type 1.  */

#ifdef __USE_MISC
# define XTABS	0014000
#endif

/* c_lflag bits */
#define ISIG	0000001   /* Enable signals.  */
#define ICANON	0000002   /* Canonical input (erase and kill processing).  */
#if defined __USE_MISC || (defined __USE_XOPEN && !defined __USE_XOPEN2K)
# define XCASE	0000004
#endif
#define ECHO	0000010   /* Enable echo.  */
#define ECHOE	0000020   /* Echo erase character as error-correcting
			     backspace.  */
#define ECHOK	0000040   /* Echo KILL.  */
#define ECHONL	0000100   /* Echo NL.  */
#define NOFLSH	0000200   /* Disable flush after interrupt or quit.  */
#define TOSTOP	0000400   /* Send SIGTTOU for background output.  */
#ifdef __USE_MISC
# define ECHOCTL 0001000  /* If ECHO is also set, terminal special characters
			     other than TAB, NL, START, and STOP are echoed as
			     ^X, where X is the character with ASCII code 0x40
			     greater than the special character
			     (not in POSIX).  */
# define ECHOPRT 0002000  /* If ICANON and ECHO are also set, characters are
			     printed as they are being erased
			     (not in POSIX).  */
# define ECHOKE	 0004000  /* If ICANON is also set, KILL is echoed by erasing
			     each character on the line, as specified by ECHOE
			     and ECHOPRT (not in POSIX).  */
# define FLUSHO	 0010000  /* Output is being flushed.  This flag is toggled by
			     typing the DISCARD character (not in POSIX).  */
# define PENDIN	 0040000  /* All characters in the input queue are reprinted
			     when the next character is read
			     (not in POSIX).  */
#endif
#define IEXTEN	0100000   /* Enable implementation-defined input processing. */
#ifdef __USE_MISC
# define EXTPROC 0200000
#endif

#endif

#ifdef __GLIBC__

static char itoa_buffer[21];

// Intentionally non-reentrant, like nolibc's implementation, specifically so
// we do not have to free memory each time we stringify an integer.
char *itoa(long in) {
    snprintf(itoa_buffer, sizeof(itoa_buffer), "%ld", in);
    return itoa_buffer;
}

#endif

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

#define DEF_FLAG_SETTER(arg, name, field, constant)   if (arg[0] == '-' && !strcmp(&arg[1], name)) {\
    field &= ~constant;\
    continue;\
} else if (!strcmp(arg, name)) {\
    field |= constant;\
    continue;\
}

#define DEF_CC_SETTER(arg, name, constant) if (!strcmp(arg, name)) { setting_cc = constant; continue; } 

#define CHAR_ON     '+'
#define CHAR_OFF    '-'

#define DEF_FLAG_PRINT(field, name, constant) \
    printf("%s[%c] ", name, (field & constant) ? CHAR_ON : CHAR_OFF);

#define DEF_CC_PRINT(name, c) if (c != _POSIX_VDISABLE) \
        if (isgraph(c)) \
            printf("%s=%c ", name, c); \
        else \
            printf("%s=0x%02x ", name, c); \
    else \
        printf("%s=<Unset> ", name);


const char *USAGE = "";
static void print_usage () {
    puts(USAGE);
}

#define INVALID_SPEED   -2

static long speedtol (speed_t speed) {
    switch (speed) {
        case B0: return 0;
        case B50: return 50;
        case B75: return 75;
        case B110: return 110;
        case B134: return 134;
        case B150: return 150;
        case B200: return 200;
        case B300: return 300;
        case B600: return 600;
        case B1200: return 1200;
        case B1800: return 1800;
        case B2400: return 2400;
        case B4800: return 4800;
        case B9600: return 9600;
        case B19200: return 19200;
        case B38400: return 38400;
        case B57600: return 57600;
        case B115200: return 115200;
        case B230400: return 230400;
        case B460800: return 460800;
        case B500000: return 500000;
        case B576000: return 576000;
        case B921600: return 921600;
        case B1000000: return 1000000;
        case B1152000: return 1152000;
        case B1500000: return 1500000;
        case B2000000: return 2000000;
        case B2500000: return 2500000;
        case B3000000: return 3000000;
        case B3500000: return 3500000;
        case B4000000: return 4000000;
        default: return INVALID_SPEED;
    }
}

static speed_t ltospeed (long l) {
    switch (l) {
        case 0: return B0;
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 500000: return B500000;
        case 576000: return B576000;
        case 921600: return B921600;
        case 1000000: return B1000000;
        case 1152000: return B1152000;
        case 1500000: return B1500000;
        case 2000000: return B2000000;
        case 2500000: return B2500000;
        case 3000000: return B3000000;
        case 3500000: return B3500000;
        case 4000000: return B4000000;
        return INVALID_SPEED;
    }
}

static int print_term (struct termios *term) {
    puts("===== CHARACTERISTICS ====");
    long ispeed = speedtol(term->c_ispeed);
    long ospeed = speedtol(term->c_ospeed);
    if (ispeed == INVALID_SPEED) {
        puts("Input Speed / Symbol Rate (in Baud): INVALID");
    } else {
        printf("Input Speed / Symbol Rate (in Baud): %ld \n", ispeed);
    }
    if (ospeed == INVALID_SPEED) {
        puts("Output Speed / Symbol Rate (in Baud): INVALID");
    } else {
        printf("Output Speed / Symbol Rate (in Baud): %ld \n", ospeed);
    }

    puts("===== INPUT PARAMETERS =====");
    DEF_FLAG_PRINT(term->c_iflag, "ignbrk", IGNBRK)
    DEF_FLAG_PRINT(term->c_iflag, "brkint", BRKINT)
    DEF_FLAG_PRINT(term->c_iflag, "ignpar", IGNPAR)
    DEF_FLAG_PRINT(term->c_iflag, "parmrk", PARMRK)
    DEF_FLAG_PRINT(term->c_iflag, "inpck", INPCK)
    DEF_FLAG_PRINT(term->c_iflag, "istrip", ISTRIP)
    DEF_FLAG_PRINT(term->c_iflag, "inlcr", INLCR)
    DEF_FLAG_PRINT(term->c_iflag, "igncr", IGNCR)
    puts("");
    DEF_FLAG_PRINT(term->c_iflag, "icrnl", ICRNL)
    DEF_FLAG_PRINT(term->c_iflag, "iuclc", IUCLC)
    DEF_FLAG_PRINT(term->c_iflag, "ixon", IXON)
    DEF_FLAG_PRINT(term->c_iflag, "ixany", IXANY)
    DEF_FLAG_PRINT(term->c_iflag, "ixoff", IXOFF)
    DEF_FLAG_PRINT(term->c_iflag, "imaxbel", IMAXBEL)
    DEF_FLAG_PRINT(term->c_iflag, "iutf8", IUTF8)

    puts("\n\n===== OUTPUT PARAMETERS =====");
    DEF_FLAG_PRINT(term->c_oflag, "opost", OPOST)
    DEF_FLAG_PRINT(term->c_oflag, "olcuc", OLCUC)
    DEF_FLAG_PRINT(term->c_oflag, "onlcr", ONLCR)
    DEF_FLAG_PRINT(term->c_oflag, "ocrnl", OCRNL)
    DEF_FLAG_PRINT(term->c_oflag, "onocr", ONOCR)
    DEF_FLAG_PRINT(term->c_oflag, "onlret", ONLRET)
    DEF_FLAG_PRINT(term->c_oflag, "ofill", OFILL)
    DEF_FLAG_PRINT(term->c_oflag, "ofdel", OFDEL)
#if defined __USE_MISC || defined __USE_XOPEN
    puts("");
    DEF_FLAG_PRINT(term->c_oflag, "nldly", NLDLY)
    DEF_FLAG_PRINT(term->c_oflag, "nl0", NL0)
    DEF_FLAG_PRINT(term->c_oflag, "nl1", NL1)
    DEF_FLAG_PRINT(term->c_oflag, "crdly", CRDLY)
    DEF_FLAG_PRINT(term->c_oflag, "cr0", CR0)
    DEF_FLAG_PRINT(term->c_oflag, "cr1", CR1)
    DEF_FLAG_PRINT(term->c_oflag, "cr2", CR2)
    DEF_FLAG_PRINT(term->c_oflag, "cr3", CR3)
    DEF_FLAG_PRINT(term->c_oflag, "tabdly", TABDLY)
    puts("");
    DEF_FLAG_PRINT(term->c_oflag, "tab0", TAB0)
    DEF_FLAG_PRINT(term->c_oflag, "tab1", TAB1)
    DEF_FLAG_PRINT(term->c_oflag, "tab2", TAB2)
    DEF_FLAG_PRINT(term->c_oflag, "tab3", TAB3)
    DEF_FLAG_PRINT(term->c_oflag, "bsdly", BSDLY)
    DEF_FLAG_PRINT(term->c_oflag, "bs0", BS0)
    DEF_FLAG_PRINT(term->c_oflag, "bs1", BS1)
    DEF_FLAG_PRINT(term->c_oflag, "ffdly", FFDLY)
    puts("");
    DEF_FLAG_PRINT(term->c_oflag, "ff0", FF0)
    DEF_FLAG_PRINT(term->c_oflag, "ff1", FF1)
#endif
#ifdef __USE_MISC
    DEF_FLAG_PRINT(term->c_oflag, "xtabs", XTABS)
#endif

    puts("\n\n===== CONTROL PARAMETERS =====");
    DEF_FLAG_PRINT(term->c_cflag, "csize", CSIZE)
    DEF_FLAG_PRINT(term->c_cflag, "cs5", CS5)
    DEF_FLAG_PRINT(term->c_cflag, "cs6", CS6)
    DEF_FLAG_PRINT(term->c_cflag, "cs7", CS7)
    DEF_FLAG_PRINT(term->c_cflag, "cs8", CS8)
    DEF_FLAG_PRINT(term->c_cflag, "cstopb", CSTOPB)
    DEF_FLAG_PRINT(term->c_cflag, "cread", CREAD)
    DEF_FLAG_PRINT(term->c_cflag, "parenb", PARENB)
    DEF_FLAG_PRINT(term->c_cflag, "parodd", PARODD)
    puts("");
    DEF_FLAG_PRINT(term->c_cflag, "hupcl", HUPCL)
    DEF_FLAG_PRINT(term->c_cflag, "clocal", CLOCAL)

    puts("\n\n===== LOCAL CONTROL PARAMETERS =====");
    DEF_FLAG_PRINT(term->c_lflag, "isig", ISIG)
    DEF_FLAG_PRINT(term->c_lflag, "icanon", ICANON)
    DEF_FLAG_PRINT(term->c_lflag, "echo", ECHO)
    DEF_FLAG_PRINT(term->c_lflag, "echoe", ECHOE)
    DEF_FLAG_PRINT(term->c_lflag, "echok", ECHOK)
    DEF_FLAG_PRINT(term->c_lflag, "echonl", ECHONL)
    DEF_FLAG_PRINT(term->c_lflag, "noflsh", NOFLSH)
    DEF_FLAG_PRINT(term->c_lflag, "tostop", TOSTOP)
    puts("");
    DEF_FLAG_PRINT(term->c_lflag, "iexten", IEXTEN)
    DEF_FLAG_PRINT(term->c_lflag, "vtdly", VTDLY)
    DEF_FLAG_PRINT(term->c_lflag, "vt0", VT0)
    DEF_FLAG_PRINT(term->c_lflag, "vt1", VT1)
#ifdef __USE_MISC
    DEF_FLAG_PRINT(term->c_lflag, "echoctl", ECHOCTL)
    DEF_FLAG_PRINT(term->c_lflag, "echoprt", ECHOPRT)
    DEF_FLAG_PRINT(term->c_lflag, "echoke", ECHOKE)
    DEF_FLAG_PRINT(term->c_lflag, "flusho", FLUSHO)
    puts("");
    DEF_FLAG_PRINT(term->c_lflag, "pendin", PENDIN)
    DEF_FLAG_PRINT(term->c_lflag, "extproc", EXTPROC)
#endif

    puts("\n\n===== CONTROL CHARACTERS =====");
    DEF_CC_PRINT("vintr", term->c_cc[VINTR])
    DEF_CC_PRINT("vquit", term->c_cc[VQUIT])
    DEF_CC_PRINT("verase", term->c_cc[VERASE])
    DEF_CC_PRINT("vkill", term->c_cc[VKILL])
    DEF_CC_PRINT("veof", term->c_cc[VEOF])
    DEF_CC_PRINT("vtime", term->c_cc[VTIME])
    puts("");
    DEF_CC_PRINT("vmin", term->c_cc[VMIN])
    DEF_CC_PRINT("vswtc", term->c_cc[VSWTC])
    DEF_CC_PRINT("vstart", term->c_cc[VSTART])
    DEF_CC_PRINT("vstop", term->c_cc[VSTOP])
    DEF_CC_PRINT("vsusp", term->c_cc[VSUSP])
    DEF_CC_PRINT("veol", term->c_cc[VEOL])
    puts("");
    DEF_CC_PRINT("vreprint", term->c_cc[VREPRINT])
    DEF_CC_PRINT("vdiscard", term->c_cc[VDISCARD])
    DEF_CC_PRINT("vwerase", term->c_cc[VWERASE])
    DEF_CC_PRINT("vlnext", term->c_cc[VLNEXT])
    DEF_CC_PRINT("veol2", term->c_cc[VEOL2])

    puts("");
    return EXIT_SUCCESS;
}

#define NEXT_ARG_NONE   0
#define NEXT_ARG_FILE   1

// TODO: Line Discipline?
int main (int argc, char **argv) {
    char *arg;
    struct termios term;
    int next_arg = NEXT_ARG_NONE;
    int fd = STDIN_FILENO;
    long term_speed;
    int setting_cc = -1;

    if (tcgetattr(fd, &term) != 0) {
        perror("stty @ tcgetattr()");
        return EXIT_FAILURE;
    }

    if (argc == 1)
        return print_term(&term);

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;
        if (setting_cc >= 0) {
            term.c_cc[setting_cc] = arg[0];
            setting_cc = -1;
        }
        if (next_arg == NEXT_ARG_FILE) {
            fd = open(arg, O_RDONLY);
            if (fd < 0) {
                perror("stty @ open");
                return EXIT_FAILURE;
            }
            next_arg = NEXT_ARG_NONE;
        }
        if (!strcmp(arg, "--all") || !strcmp(arg, "-a"))
            return print_term(&term);
        if (prefix("--file=", arg)) {
            fd = open(&arg[7], O_RDONLY);
            if (fd < 0) {
                perror("stty @ open");
                return EXIT_FAILURE;
            }
            if (tcgetattr(fd, &term) != 0) {
                perror("stty @ tcgetattr()");
                return EXIT_FAILURE;
            }
            continue;
        }
        if (!strcmp(arg, "-F")) {
            next_arg = NEXT_ARG_FILE;
            continue;
        }

        if (isdigit(arg[0])) {
            for (int i = 0; i < strlen(arg); i++) {
                if (!isdigit(arg[i])) {
                    print_usage();
                    return EXIT_FAILURE;
                }
                term_speed = (term_speed * 10) + (arg[i] - '0');
            }
            term.c_ispeed = ltospeed(term_speed);
            term.c_ospeed = term.c_ispeed;
            if (term.c_ispeed == INVALID_SPEED) {
                puts("Invalid speed.");
                return EXIT_FAILURE;
            }
        }

        // Input flags
        DEF_FLAG_SETTER(arg, "ignbrk", term.c_iflag, IGNBRK)
        DEF_FLAG_SETTER(arg, "brkint", term.c_iflag, BRKINT)
        DEF_FLAG_SETTER(arg, "ignpar", term.c_iflag, IGNPAR)
        DEF_FLAG_SETTER(arg, "parmrk", term.c_iflag, PARMRK)
        DEF_FLAG_SETTER(arg, "inpck", term.c_iflag, INPCK)
        DEF_FLAG_SETTER(arg, "istrip", term.c_iflag, ISTRIP)
        DEF_FLAG_SETTER(arg, "inlcr", term.c_iflag, INLCR)
        DEF_FLAG_SETTER(arg, "igncr", term.c_iflag, IGNCR)
        DEF_FLAG_SETTER(arg, "icrnl", term.c_iflag, ICRNL)
        DEF_FLAG_SETTER(arg, "iuclc", term.c_iflag, IUCLC)
        DEF_FLAG_SETTER(arg, "ixon", term.c_iflag, IXON)
        DEF_FLAG_SETTER(arg, "ixany", term.c_iflag, IXANY)
        DEF_FLAG_SETTER(arg, "ixoff", term.c_iflag, IXOFF)
        DEF_FLAG_SETTER(arg, "imaxbel", term.c_iflag, IMAXBEL)
        DEF_FLAG_SETTER(arg, "iutf8", term.c_iflag, IUTF8)

        // Output flags
        DEF_FLAG_SETTER(arg, "opost", term.c_oflag, OPOST)
        DEF_FLAG_SETTER(arg, "olcuc", term.c_oflag, OLCUC)
        DEF_FLAG_SETTER(arg, "onlcr", term.c_oflag, ONLCR)
        DEF_FLAG_SETTER(arg, "ocrnl", term.c_oflag, OCRNL)
        DEF_FLAG_SETTER(arg, "onocr", term.c_oflag, ONOCR)
        DEF_FLAG_SETTER(arg, "onlret", term.c_oflag, ONLRET)
        DEF_FLAG_SETTER(arg, "ofill", term.c_oflag, OFILL)
        DEF_FLAG_SETTER(arg, "ofdel", term.c_oflag, OFDEL)
#if defined __USE_MISC || defined __USE_XOPEN
        DEF_FLAG_SETTER(arg, "nldly", term.c_oflag, NLDLY)
        DEF_FLAG_SETTER(arg, "nl0", term.c_oflag, NL0)
        DEF_FLAG_SETTER(arg, "nl1", term.c_oflag, NL1)
        DEF_FLAG_SETTER(arg, "crdly", term.c_oflag, CRDLY)
        DEF_FLAG_SETTER(arg, "cr0", term.c_oflag, CR0)
        DEF_FLAG_SETTER(arg, "cr1", term.c_oflag, CR1)
        DEF_FLAG_SETTER(arg, "cr2", term.c_oflag, CR2)
        DEF_FLAG_SETTER(arg, "cr3", term.c_oflag, CR3)
        DEF_FLAG_SETTER(arg, "tabdly", term.c_oflag, TABDLY)
        DEF_FLAG_SETTER(arg, "tab0", term.c_oflag, TAB0)
        DEF_FLAG_SETTER(arg, "tab1", term.c_oflag, TAB1)
        DEF_FLAG_SETTER(arg, "tab2", term.c_oflag, TAB2)
        DEF_FLAG_SETTER(arg, "tab3", term.c_oflag, TAB3)
        DEF_FLAG_SETTER(arg, "bsdly", term.c_oflag, BSDLY)
        DEF_FLAG_SETTER(arg, "bs0", term.c_oflag, BS0)
        DEF_FLAG_SETTER(arg, "bs1", term.c_oflag, BS1)
        DEF_FLAG_SETTER(arg, "ffdly", term.c_oflag, FFDLY)
        DEF_FLAG_SETTER(arg, "ff0", term.c_oflag, FF0)
        DEF_FLAG_SETTER(arg, "ff1", term.c_oflag, FF1)
#endif
#ifdef __USE_MISC
        DEF_FLAG_SETTER(arg, "xtabs", term.c_oflag, XTABS)
#endif

        // Control flag bits
        DEF_FLAG_SETTER(arg, "csize", term.c_cflag, CSIZE)
        DEF_FLAG_SETTER(arg, "cs5", term.c_cflag, CS5)
        DEF_FLAG_SETTER(arg, "cs6", term.c_cflag, CS6)
        DEF_FLAG_SETTER(arg, "cs7", term.c_cflag, CS7)
        DEF_FLAG_SETTER(arg, "cs8", term.c_cflag, CS8)
        DEF_FLAG_SETTER(arg, "cstopb", term.c_cflag, CSTOPB)
        DEF_FLAG_SETTER(arg, "cread", term.c_cflag, CREAD)
        DEF_FLAG_SETTER(arg, "parenb", term.c_cflag, PARENB)
        DEF_FLAG_SETTER(arg, "parodd", term.c_cflag, PARODD)
        DEF_FLAG_SETTER(arg, "hupcl", term.c_cflag, HUPCL)
        DEF_FLAG_SETTER(arg, "clocal", term.c_cflag, CLOCAL)

        // Local control flags
        DEF_FLAG_SETTER(arg, "isig", term.c_lflag, ISIG)
        DEF_FLAG_SETTER(arg, "icanon", term.c_lflag, ICANON)
        DEF_FLAG_SETTER(arg, "echo", term.c_lflag, ECHO)
        DEF_FLAG_SETTER(arg, "echoe", term.c_lflag, ECHOE)
        DEF_FLAG_SETTER(arg, "echok", term.c_lflag, ECHOK)
        DEF_FLAG_SETTER(arg, "echonl", term.c_lflag, ECHONL)
        DEF_FLAG_SETTER(arg, "noflsh", term.c_lflag, NOFLSH)
        DEF_FLAG_SETTER(arg, "tostop", term.c_lflag, TOSTOP)
        DEF_FLAG_SETTER(arg, "iexten", term.c_lflag, IEXTEN)
        DEF_FLAG_SETTER(arg, "vtdly", term.c_lflag, VTDLY)
        DEF_FLAG_SETTER(arg, "vt0", term.c_lflag, VT0)
        DEF_FLAG_SETTER(arg, "vt1", term.c_lflag, VT1)
#ifdef __USE_MISC
        DEF_FLAG_SETTER(arg, "echoctl", term.c_lflag, ECHOCTL)
        DEF_FLAG_SETTER(arg, "echoprt", term.c_lflag, ECHOPRT)
        DEF_FLAG_SETTER(arg, "echoke", term.c_lflag, ECHOKE)
        DEF_FLAG_SETTER(arg, "flusho", term.c_lflag, FLUSHO)
        DEF_FLAG_SETTER(arg, "pendin", term.c_lflag, PENDIN)
        DEF_FLAG_SETTER(arg, "extproc", term.c_lflag, EXTPROC)
#endif

        // Control Characters
        DEF_CC_SETTER(arg, "vintr", VINTR)
        DEF_CC_SETTER(arg, "vquit", VQUIT)
        DEF_CC_SETTER(arg, "verase", VERASE)
        DEF_CC_SETTER(arg, "vkill", VKILL)
        DEF_CC_SETTER(arg, "veof", VEOF)
        DEF_CC_SETTER(arg, "vtime", VTIME)
        DEF_CC_SETTER(arg, "vmin", VMIN)
        DEF_CC_SETTER(arg, "vswtc", VSWTC)
        DEF_CC_SETTER(arg, "vstart", VSTART)
        DEF_CC_SETTER(arg, "vstop", VSTOP)
        DEF_CC_SETTER(arg, "vsusp", VSUSP)
        DEF_CC_SETTER(arg, "veol", VEOL)
        DEF_CC_SETTER(arg, "vreprint", VREPRINT)
        DEF_CC_SETTER(arg, "vdiscard", VDISCARD)
        DEF_CC_SETTER(arg, "vwerase", VWERASE)
        DEF_CC_SETTER(arg, "vlnext", VLNEXT)
        DEF_CC_SETTER(arg, "veol2", VEOL2)
    }

    if (tcsetattr(fd, 0, &term) != 0) {
        perror("stty @ tcsetattr()");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
