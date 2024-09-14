#ifndef NOLIBC
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#else

char* strcat(char* dest, char* src) {
    char* ret = dest;
    while (*dest)
	    dest++;
    while (*dest++ = *src++);
    return ret;
}

/* This is probably not a correct implementation somehow, but it is surely
"close enough." */
static char *basename (char *path) {
    int len = strlen(path);

    // Real basename returns an empty string if supplied one.
    if (len == 0) {
        return "";
    }

    // Real basename returns a single slash if supplied one.
    if (len == 1 && path[0] == '/') {
        return "/";
    }

    int i = len - 1;

    // Ignore trailing slashes, which is what the real basename does.
    for (; i > 0; i--) {
        if (path[i] != '/') {
            break;
        }
    }

    // Real basename returns a single slash if the input is all slashes.
    if (i == 0) {
        return "/";
    }

    // Set the trailing slash to a null character to cut the string off here.
    path[i+1] = 0;

    for (; i >= 0; i--) {
        if (path[i] == '/') {
            return &path[i+1];
        }
    }
    return path;
}

static int rename(const char *oldpath, const char *newpath) {
    return my_syscall2(__NR_rename, oldpath, newpath);
}

#endif

#define FLAG_NO_CLOBBER     (1)
#define FLAG_UPDATE         (1 << 1)
#define FLAG_VERBOSE        (1 << 2)
#define FLAG_BACKUP         (1 << 3)

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static int move (char* src, char* dest, int flags) {
    struct stat src_st;
    struct stat dest_st;
    char dest2[NAME_MAX + 2];
    size_t dest_name_len;

    dest_name_len = strlen(dest);
    if ((dest_name_len >= NAME_MAX) || (dest_name_len == 0)) {
        errno = EINVAL;
        perror(dest);
        return EXIT_FAILURE;
    }

    // ENOENT is ignored, because the destination may be the new name of the file.
    if ((stat(dest, &dest_st) < 0) && (errno != ENOENT)) {
        perror(src);
        return EXIT_FAILURE;
    }

    // If ENOENT: directly rename src to dest.
    if (errno == ENOENT) {
        if (rename(src, dest) < 0) {
            perror(src);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    } else if (S_ISDIR(dest_st.st_mode)) { // If the dest is a directory, move under it.
        strcpy(dest2, dest);
        if (dest[dest_name_len - 1] != '/')
            strcat(dest2, "/");
        strcat(dest2, basename(src));
        dest = dest2;
    }
    // Otherwise, the dest is a file: create backup, check for updates, etc.
    if (flags & FLAG_NO_CLOBBER)
        return EXIT_FAILURE;
    if (flags & FLAG_BACKUP) {
        strcpy(dest2, dest);
        dest2[dest_name_len] = '~';
        dest2[dest_name_len + 1] = 0;
        if (move(dest, dest2, (flags & FLAG_VERBOSE)) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }
    if (flags & FLAG_UPDATE) {
        if (stat(src, &src_st) < 0) {
            perror(src);
            return EXIT_FAILURE;
        }
        // TODO: Is this the right timestamp to use?
        if (src_st.st_mtime <= dest_st.st_mtime) {
            // The destination is newer than the source. Just exit.
            goto win;
        }
    }
    if (unlink(dest) < 0) {
        perror(src);
        return EXIT_FAILURE;
    }
    if (rename(src, dest) < 0) {
        perror(src);
        return EXIT_FAILURE;
    }

win:
    if (flags & FLAG_VERBOSE)
        printf("Moved %s to %s\n", src, dest);
    return EXIT_SUCCESS;
}

static void print_usage () {
    puts("mv [-v|-n|-u|-b]... SOURCE... DIRECTORY");
    puts("mv [-v|-n|-u|-b]... -t DIRECTORY SOURCE...");
}

int main (int argc, char **argv) {
    char *arg;
    char *dest;
    int flags = 0;
    int next_is_dest = 0;
    int i;

    if (argc <= 2) {
        print_usage();
        return EXIT_FAILURE;
    }
    dest = argv[argc - 1];

    // First pass: find the destination directory.
    // Multiple destinations is an undetected user error.
    for (i = 1; i < argc - 1; i++) {
        arg = argv[i];
        if (next_is_dest) {
            dest = arg;
            break;
        }
        if (!strcmp(arg, "-t")) {
            next_is_dest = 1;
            continue;
        }
        if (prefix("--target-directory=", arg)) {
            dest = &arg[sizeof("--target-directory=")];
            break;
        }
    }

    // Second pass: read all options
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (!strcmp(arg, "-n") || !strcmp(arg, "--no-clobber")) {
            flags |= FLAG_NO_CLOBBER;
            continue;
        }
        else if (!strcmp(arg, "-u") || !strcmp(arg, "--update")) {
            flags |= FLAG_UPDATE;
            continue;
        }
        else if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
            flags |= FLAG_VERBOSE;
            continue;
        }
        else if (!strcmp(arg, "-b") || !strcmp(arg, "--backup")) {
            flags |= FLAG_BACKUP;
            continue;
        }
        else if (arg[0] == '-') {
            printf("Option not understood: %s\n", arg);
            print_usage();
            return EXIT_FAILURE;
        }
        else {
            // We have reached the end of the options. Time to parse files.
            break;
        }
    }

    // Third pass: process all files
    for (; i < argc; i++) {
        arg = argv[i];
        if (!strcmp(arg, "-t")) {
            i++;
            continue;
        }
        if (prefix("--target-directory=", arg))
            continue;
        if (move(arg, dest, flags) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}