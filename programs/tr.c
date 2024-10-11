#ifndef NOLIBC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#else

#define BUFSIZ      8192

int getchar() {
    int c;
    if (read(STDIN_FILENO, &c, 1) <= 0)
        return EOF;
    return c;
}

int putchar (int c) {
    return write(STDOUT_FILENO, &c, 1) == 1
        ? c
        : EOF;
}

#endif

#define FLAG_COMPLEMENT     1
#define FLAG_DELETE         (1 << 1)
#define FLAG_SQUEEZE        (1 << 2)
#define FLAG_TRUNCATE_SET1  (1 << 3)

#define MAX_LEN 256

// TODO: These are not needed to compile Linux, but it would be nice to fix them.
// You would have a 100% functional tr, if so.
static int unimplemented (const char *thing) {
    write(STDERR_FILENO, "Unimplemented: ", strlen("Unimplemented: "));
    write(STDERR_FILENO, thing, strlen(thing));
    write(STDERR_FILENO, "\n", 1);
    return EXIT_FAILURE;
}

static int tr(const char *set1, const char *set2, int flags) {
    char map[MAX_LEN];
    size_t i;
    int c;
    size_t len1 = strlen(set1);
    size_t len2 = strlen(set2);

    // Initialize map to identity (characters map to themselves)
    for (i = 0; i < MAX_LEN; i++) {
        map[i] = i;
    }

    // Populate translation map based on set1 -> set2 correspondence
    for (i = 0; i < len1; i++) {
        if (i < len2) {
            map[(unsigned char)set1[i]] = set2[i];
        } else {
            map[(unsigned char)set1[i]] = '\0'; // If set2 is shorter, map to null character
        }
    }

    while ((c = getchar()) != EOF) {
        if (putchar(map[c]) == EOF) {
            perror("write");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

const char *USAGE_MSG = "Usage: tr [-c|-d|-s|-t] SET1 [SET2]";
static int print_usage () {
    puts(USAGE_MSG);
}

/*
-c, -C, --complement        use the complement of SET1
-d, --delete                delete characters in SET1, do not translate
-s, --squeeze-repeats       replace each sequence of a repeated character that is listed in the last specified SET, with a single occurrence of that character
-t, --truncate-set1         first truncate SET1 to length of SET2
*/
int main (int argc, char **argv) {
    int flags = 0;
    char *arg;
    char *set1 = NULL;
    char *set2 = NULL;
    int i;

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc - 1; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;
        if (!strcmp(arg, "--complement") || !strcmp(arg, "-c") || !strcmp(arg, "-C")) {
            flags |= FLAG_COMPLEMENT;
            unimplemented(arg);
            continue;
        }
        else if (!strcmp(arg, "--delete") || !strcmp(arg, "-d")) {
            flags |= FLAG_DELETE;
            unimplemented(arg);
            continue;
        }
        else if (!strcmp(arg, "--squeeze-repeats") || !strcmp(arg, "-s")) {
            flags |= FLAG_SQUEEZE;
            unimplemented(arg);
            continue;
        }
        else if (!strcmp(arg, "--truncate-set1 ") || !strcmp(arg, "-t")) {
            flags |= FLAG_TRUNCATE_SET1;
            unimplemented(arg);
            continue;
        }
        else if (arg[0] == '-') {
            printf("Unrecognized flag: %s\n", arg);
            print_usage();
            return EXIT_FAILURE;
        }
        else {
            break;
        }
    }

    set1 = argv[i++];
    set2 = (i < argc)
        ? argv[i++]
        : "";
    if (i < argc) {
        print_usage();
        return EXIT_FAILURE;
    }

    return tr(set1, set2, flags);
}
