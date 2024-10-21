#ifndef NOLIBC
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#else
#define	F_OK	0

int access (const char *pathname, int mode) {
    return __sysret(my_syscall2(__NR_access, pathname, mode));
}

#endif

const char *USAGE_MSG = "";
static void print_usage () {
    puts(USAGE_MSG);
}

// TODO: None of these are supported except SYMBOLIC and FORCE
#define FLAG_FORCE          1
#define FLAG_INTERACTIVE    (1 << 1)
#define FLAG_LOGICAL        (1 << 2)
#define FLAG_NO_DEREF       (1 << 3)
#define FLAG_PHYSICAL       (1 << 4)
#define FLAG_RELATIVE       (1 << 5)
#define FLAG_SYMBOLIC       (1 << 6)
#define FLAG_NO_TARGET_DIR  (1 << 7)
#define FLAG_VERBOSE        (1 << 8)
#define FLAG_BACKUP         (1 << 9)

static int ln(char *target, char *link_name, int flags) {
    int ret;

    if (
        (flags & FLAG_FORCE)
        && (access(link_name, F_OK) == EXIT_SUCCESS)
        && (unlink(link_name) < 0)
    ) {
        perror("unlink");
        return EXIT_FAILURE;
    }

    if (flags & FLAG_SYMBOLIC) {
        ret = symlink(target, link_name);
    } else {
        ret = link(target, link_name);
    }
    if (ret) {
        perror("ln");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    int flags = 0;
    char *target;
    char *link_name;
    char *arg;
    int i;
    size_t arglen;

    if (argc < 3) {
        print_usage();
        return EXIT_FAILURE;
    }

parse_args:
    for (i = 1; i < argc - 1; i++) {
        arg = argv[i];
        arglen = strlen(arg);
        if (arglen == 0)
            continue;
parse_short_args:
        if (arglen >= 2 && arg[0] == '-' && arg[1] != '-') {
            for (int j = 1; j < arglen; j++) {
                switch (arg[j]) {
                case 'b':
                    flags |= FLAG_BACKUP;
                    break;
                case 'f':
                    flags |= FLAG_FORCE;
                    break;
                case 'i':
                    flags |= FLAG_INTERACTIVE;
                    break;
                case 'L':
                    flags |= FLAG_LOGICAL;
                    break;
                case 'n':
                    flags |= FLAG_NO_DEREF;
                    break;
                case 'P':
                    flags |= FLAG_PHYSICAL;
                    break;
                case 'r':
                    flags |= FLAG_RELATIVE;
                    break;
                case 's':
                    flags |= FLAG_SYMBOLIC;
                    break;
                case 'T':
                    flags |= FLAG_NO_TARGET_DIR;
                    break;
                case 'v':
                    flags |= FLAG_VERBOSE;
                    break;
                default: {
                    print_usage();
                    return EXIT_FAILURE;
                }
                }
            }
            continue;
        }
parse_long_args:
        if (arglen < 2 || arg[0] != '-' || arg[1] != '-')
            break;
        if (!strcmp(arg, "--force")) {
            flags |= FLAG_FORCE;
        }
        else if (!strcmp(arg, "--interactive")) {
            flags |= FLAG_INTERACTIVE;
        }
        else if (!strcmp(arg, "--logical")) {
            flags |= FLAG_LOGICAL;
        }
        else if (!strcmp(arg, "--no-dereference")) {
            flags |= FLAG_NO_DEREF;
        }
        else if (!strcmp(arg, "--physical")) {
            flags |= FLAG_PHYSICAL;
        }
        else if (!strcmp(arg, "--relative")) {
            flags |= FLAG_RELATIVE;
        }
        else if (!strcmp(arg, "--symbolic")) {
            flags |= FLAG_SYMBOLIC;
        }
        else if (!strcmp(arg, "--no-target-directory")) {
            flags |= FLAG_NO_TARGET_DIR;
        }
        else if (!strcmp(arg, "--verbose")) {
            flags |= FLAG_VERBOSE;
        }
    }

parse_target_and_link_name:
    target = argv[i++];
    if (i >= argc) {
        print_usage();
        return EXIT_FAILURE;
    }
    link_name = argv[i];

    return ln(target, link_name, flags);    
}