/*
Copied from: https://github.com/michael105/minicore/blob/master/core/grep.c.
Modified extensively by Jonathan M. Wilbur to compile on nolibc and to add
support for:

-F      = Interpret PATTERNS as fixed strings, not regular expressions
-f      = Obtain  patterns  from  FILE, one per line.
-v      = Invert match
-q      = No output. Exit with 0 if match.
-i      = Case insensitive compare
\w      = Word Class
+       = One-or-more operator
?       = Optional operator

-F = --fixed-strings
-f = --file
-v = --invert-match
-q = --quiet, --silent
-i = --ignore-case

For this to work for building the Linux kernel, it MUST support these invocations:
grep -F -f ./scripts/head-object-list.txt
grep -v _NONE
grep -q " R_\w*_"

HOWEVER: The first invocation always returns nothing on x86-64, and the others
will be gone if you eliminate vDSO from the build. The latter two invocations
also return nothing in normal cases: it is just some kind of check. So I might
be able to get away with a grep program that merely understands the command line
arguments and does _nothing_.

*/
#ifndef NOLIBC
#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#else
#define BUFSIZ 8192

static int tolower (int c) {
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    else
        return c;
}

static char *strstr_ex (char *haystack, const char *needle, int caseign) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    // If needle is the empty string, the return value is always haystack itself.
    if (nlen == 0) {
        return haystack;
    }
    if (hlen == 0) {
        return NULL;
    }

    char needlechar0 = needle[0];
    for (size_t i = 0; i < hlen - nlen; i++) {
        if (caseign && isalpha(haystack[i])) {
            if (tolower(haystack[i]) != tolower(needlechar0))
                continue;
        } else {
            if (haystack[i] != needlechar0)
                continue;
        }
        if (memcmp(&haystack[i], needle, nlen) == 0) {
            return &haystack[i];
        }
    }
    return NULL;
}

static char *strstr (char *haystack, const char *needle) {
    return strstr_ex(haystack, needle, 0);
}

static char *strcasestr (char *haystack, const char *needle) {
    return strstr_ex(haystack, needle, 0);
}

#endif

#define READLINE_END        EXIT_SUCCESS
#define READLINE_EREAD      (-1)
#define READLINE_EMALLOC    (-2)
#define READLINE_ETOOBIG    (-3)

static char linebuf[BUFSIZ];
static int linebufsize = 0;

/*
An absolutely STINKY implementation for the sake of simplicity.
This implementation returns a negative number for problems, 0 to indicate end of
file, and a length + 1 otherwise.
*/
static ssize_t readline(int fd, char **line) {
    char c;
    ssize_t rc;
    int ended = 0;

    while ((rc = read(fd, &c, 1)) == 1) { // Read one character at a time.
        if (c == '\r')
            continue;
        if (c == '\n')
            break;
        if (linebufsize == BUFSIZ)
            return READLINE_ETOOBIG;
        linebuf[linebufsize++] = (char)c;
    }
    if (rc < 0) {
        perror("readline");
        return READLINE_EREAD;
    }
    if (rc == 0)
        ended = 1;
    *line = malloc(linebufsize + 1);
    if (*line == NULL) {
        perror("malloc");
        return READLINE_EMALLOC;
    }
    memcpy(*line, linebuf, linebufsize);
    (*line)[linebufsize] = 0; // Add null terminator.
    rc = linebufsize;
    linebufsize = 0;
    return ended ? 0 : rc + 1;
}

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9.

#define FLAG_INVERT         (1 << 0)
#define FLAG_QUIET          (1 << 1)
#define FLAG_FIXED          (1 << 2)
#define FLAG_CASEIGN        (1 << 3) // Not necessary, but should be easy to implement for ASCII at least.

static int match_char (int c, char *text, char char_class, int flags);
static int matchstar(int c, char *re, char *text, char char_class, int flags);
static int matchhere(char *re, char *text, char char_class, int flags);
static int matchplus(int c, char *re, char *text, char char_class, int flags);

#define IS_MATCH            1
#define IS_NOT_MATCH        0

#define EXIT_MATCH          0
#define EXIT_NOT_MATCH      1
#define EXIT_PROBLEM        2

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static int match_fixed (char *pattern, char *text, int flags) {
    if (flags & FLAG_CASEIGN) {
        return (strcasestr(text, pattern) != NULL)
            ? IS_MATCH
            : IS_NOT_MATCH;
    } else {
        return (strstr(text, pattern) != NULL)
            ? IS_MATCH
            : IS_NOT_MATCH;
    }
}

static int match(char *re, char *text, int flags) {
    if (flags & FLAG_FIXED)
        return match_fixed(re, text, flags);
    if (re[0] == '^')
        return matchhere(re + 1, text, 0, flags);
    do {  // must look at empty string
        if (matchhere(re, text, 0, flags))
            return IS_MATCH;
    } while (*text++ != '\0');
    return IS_NOT_MATCH;
}

/*
matchhere: search for re at beginning of text

Note that this will ABSOLUTELY overflow the stack if the regular expression is
complicated enough.
*/
static int matchhere(char *re, char *text, char char_class, int flags) {
/* We use goto recurse instead of actual recursion to prevent a stack overflow. */
recurse:
    if (re[0] == '\0')
        return char_class == 0 ? IS_MATCH : IS_NOT_MATCH;
    if (re[1] == '*')
        return matchstar(re[0], re + 2, text, char_class, flags);
    if (re[1] == '+')
        return matchplus(re[0], re + 2, text, char_class, flags);
    if (re[1] == '?') {
        if (match_char(re[0], text, char_class, flags & FLAG_CASEIGN))
            text++;
        re += 2;
        char_class = 0; // char_class must reset here.
        goto recurse;
    }
    if (re[0] == '$' && re[1] == '\0')
        return *text == '\0';
    if (re[0] == '\\') {
        switch (re[1]) {
        case 'w':
            char_class = re[1];
            re++;
            goto recurse;
        case '.':
        case '+':
        case '?':
        case '^':
        case '$':
            if (re[1] != *text)
                return IS_NOT_MATCH;
            re += 2;
            text++;
            goto recurse;
        default: {
            printf("Not supported regex character class: %c\n", re[1]);
            exit(EXIT_FAILURE);
        }
        }
    }
    if (*text != '\0' && match_char(re[0], text, char_class, flags & FLAG_CASEIGN)) {
        re++;
        text++;
        char_class = 0; // REVIEW: I think char_class needs to reset to 0 here. Not sure.
        goto recurse;
    }
    return IS_NOT_MATCH;
}

static int match_char (int c, char *text, char char_class, int caseign) {
    if (char_class == 0) {
        if (
            caseign
            && isalpha(c)
            && tolower(c) == tolower(*text)
        )
            return IS_MATCH;
        else if (c == *text)
            return IS_MATCH;
        if (c == '.')
            return IS_MATCH;
        return IS_NOT_MATCH;
    }
    switch (char_class) {
    case 'w': return (isalnum(*text) || *text == '_');
    default:
        printf("Not supported regex character class: %c\n", c);
        exit(EXIT_FAILURE);
    }
}

// matchstar: search for c*re at beginning of text
static int matchstar(int c, char *re, char *text, char char_class, int flags) {
    do {  // a * matches zero or more instances
        if (matchhere(re, text, 0, flags)) // char_class must reset here.
            return IS_MATCH;
        if (*text == '\0')
            break;
    } while (match_char(c, text++, char_class, flags & FLAG_CASEIGN));
    return IS_MATCH;
}

static int matchplus(int c, char *re, char *text, char char_class, int flags) {
    int matched = 0;
    do { // a + matches one or more instances
        if (matchhere(re, text, 0, flags)) // char_class must reset here.
            return matched;
        matched = 1;
        if (*text == '\0')
            break;
    } while (match_char(c, text++, char_class, flags & FLAG_CASEIGN));
    return IS_NOT_MATCH;
}

static int grep(char *pattern, int fd, int flags) {
    int n, m;
    char *p, *q;
    char buf[BUFSIZ];
    int criterion = (flags & FLAG_INVERT) ? IS_NOT_MATCH : IS_MATCH;
    int any_line_matched = 0;

    m = 0;
    while ((n = read(fd, buf + m, sizeof(buf) - m - 1)) > 0) {
        m += n;
        buf[m] = '\0';
        p = buf;
        while ((q = strchr(p, '\n')) != 0) {
            *q = 0;
            if (match(pattern, p, flags) == criterion) {
                if (!(flags & FLAG_QUIET)) {
                    *q = '\n';
                    if (write(STDOUT_FILENO, p, q + 1 - p) < 0) {
                        perror("write");
                        return EXIT_PROBLEM;
                    }
                }
                any_line_matched = 1;
            }
            p = q + 1;
        }
        if (p == buf)
            m = 0;
        if (m > 0) {
            m -= p - buf;
            memmove(buf, p, m);
        }
    }
    if (n < 0) {
        perror("read");
        return EXIT_PROBLEM;
    }
    return any_line_matched ? EXIT_SUCCESS : EXIT_FAILURE;
}

const char *USAGE_MSG = "Usage: grep [options] <pattern> [files...]";
static void print_usage () {
    puts(USAGE_MSG);
}

#define NEXT_ARG_NONE       0
#define NEXT_ARG_FILE       1

int main(int argc, char *argv[]) {
    int fd, i;
    char *pattern = NULL;
    char *pattern_file = NULL;
    int flags = 0;
    int next_arg = NEXT_ARG_NONE;
    char *arg;
    size_t arglen;

    if (argc <= 1) {
        puts("usage: grep pattern [file ...]\n");
        return EXIT_PROBLEM;
    }

parse_args:
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        arglen = strlen(arg);
        if (arglen == 0)
            continue;
        switch (next_arg) {
        case NEXT_ARG_NONE:
            break;
        case NEXT_ARG_FILE:
            pattern_file = arg;
            break;
        default:
            puts("Internal error when parsing command line arguments.");
            return EXIT_PROBLEM;
        }
        if (next_arg != NEXT_ARG_NONE) {
            next_arg = NEXT_ARG_NONE;
            continue; // We already handled it.
        }
parse_short_args:
        if (arglen >= 2 && arg[0] == '-' && arg[1] != '-') {
            for (int j = 1; j < arglen; j++) {
                switch (arg[j]) {
                case 'F':
                    flags |= FLAG_FIXED;
                    break;
                case 'v':
                    flags |= FLAG_INVERT;
                    break;
                case 'q':
                    flags |= FLAG_QUIET;
                    break;
                case 'y':
                case 'i':
                    flags |= FLAG_CASEIGN;
                    break;
                case 'f':
                    next_arg = NEXT_ARG_FILE;
                    break;
                default: {
                    print_usage();
                    return EXIT_PROBLEM;
                }
                }
            }
            continue;
        }
parse_long_args:
        // if (!strcmp(arg, "-")) {
        //     flags |= FLAG_IGNORE;
        //     continue;
        // }
        if (arglen < 2 || arg[0] != '-' || arg[1] != '-')
            break;
        if (!strcmp(arg, "--fixed-strings")) {
            flags |= FLAG_FIXED;
        }
        else if (!strcmp(arg, "--invert-match")) {
            flags |= FLAG_INVERT;
        }
        else if (!strcmp(arg, " --quiet") || !strcmp(arg, "--silent")) {
            flags |= FLAG_QUIET;
        }
        else if (!strcmp(arg, "--ignore-case")) {
            flags |= FLAG_CASEIGN;
        }
        else if (!strcmp(arg, "--file")) {
            next_arg = NEXT_ARG_FILE;
        }
        else if (prefix("--file=", arg)) {
            pattern_file = &arg[strlen("--file=")];
        }
        else {
            printf("Option not understood: %s\n", arg);
            return EXIT_PROBLEM;
        }
    }

    if (next_arg != NEXT_ARG_NONE) {
        print_usage();
        return EXIT_PROBLEM;
    }

    int pfd = 0;
    if (pattern_file == NULL) {
        pattern = argv[i++];
    } else {
        pfd = open(pattern_file, O_RDONLY);
        if (pfd < 0) {
            perror("read pattern file");
            return EXIT_PROBLEM;
        }
        int rlrc = readline(pfd, &pattern);
        if (rlrc < 0)
            return EXIT_PROBLEM;
        if (pattern == NULL) {
            puts("Patterns file was empty");
            return EXIT_FAILURE; // This is GNU grep's behavior.
        }
    }

    int start_of_files = i;
    int any_matched = 0;
    int rc;
    do {
        if (i >= argc) {
            rc = grep(pattern, STDIN_FILENO, flags);
        } else {
            rc = EXIT_FAILURE;
            for (int j = start_of_files; j < argc; j++) {
                fd = (strlen(argv[j]) == 1 && argv[j][0] == '-')
                    ? STDIN_FILENO
                    : open(argv[j], O_RDONLY);
                if (fd < 0) {
                    printf("grep: cannot open %s\n", argv[i]);
                    return EXIT_PROBLEM;
                }
                rc = grep(pattern, fd, flags);
                close(fd);
            }
        }
        if (rc == EXIT_SUCCESS)
            any_matched = 1;
        if (rc == EXIT_PROBLEM && !(flags & FLAG_QUIET))
            return EXIT_PROBLEM;

        pattern = NULL;
        if (pattern_file) {
            int rlrc = readline(pfd, &pattern);
            if (rlrc < 0)
                return EXIT_PROBLEM;
            if (rlrc == 0)
                break;

            // Ignore blank lines and lines starting with #.
            if (rlrc == 1 || pattern[0] == '#')
                continue;
        }
    } while (pattern != NULL);

    if (pfd > 0)
        close(pfd);
    return any_matched == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
