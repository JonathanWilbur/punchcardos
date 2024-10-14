/* By Jonathan M. Wilbur */
#include <linux/limits.h>
#ifndef NOLIBC
// This means "yes, I want access to non-portable features of glibc"
// In our case, we want "environ" to be defined.
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#define BUFSIZ  8192
#define	X_OK	1

char *strcat(char *dest, char *src) {
    char *ret = dest;
    while (*dest)
        dest++;
    while (*dest++ = *src++)
        ;
    return ret;
}

int access(const char *pathname, int mode) {
    return __sysret(my_syscall2(__NR_access, pathname, mode));
}

#endif

const char *USAGE_MSG = "Usage: xargs [command [initial-arguments]]";
static void print_usage() {
    puts(USAGE_MSG);
}

// TODO: I may or may not implement these later.
#define FLAG_NULL (1 << 0)
#define FLAG_OPEN_TTY (1 << 1)
#define FLAG_INTERACTIVE (1 << 2)
#define FLAG_VERBOSE (1 << 3)
#define FLAG_EXIT (1 << 4)
#define FLAG_NO_RUN_IF_0 (1 << 5)

// TODO: I may or may not implement these later.
#define NEXT_ARG_NONE 0
#define NEXT_ARG_FILE 1
#define NEXT_ARG_DELIM 2
#define NEXT_ARG_EOF 3
#define NEXT_ARG_REPLACE 4
#define NEXT_ARG_MAX_LINES 5
#define NEXT_ARG_MAX_ARGS 6
#define NEXT_ARG_MAX_PROCS 7
#define NEXT_ARG_PROC_SLOT 8

static char linebuf[BUFSIZ];
static int linebufsize = 0;

/* An absolutely STINKY implementation for the sake of simplicity. */
static char *readline() {
    char c;
    char *line;

    while (1) {
        c = getchar();
        if (c == '\r')
            continue;
        if (c == '\n')
            break;
        if (c == EOF)
            return NULL;
        linebuf[linebufsize++] = (char)c;
    }
    line = malloc(linebufsize + 1);
    if (line == NULL)
        return NULL;
    memcpy(line, linebuf, linebufsize);
    line[linebufsize] = 0;  // Add null terminator.
    linebufsize = 0;
    return line;
}

char *resolve_executable_path (char *cmd, char *path_env) {
    char fullpath[PATH_MAX + 2];
    size_t path_env_len = strlen(path_env);
    int start = 0;

    for (int i = 0; i < path_env_len; i++) {
        if (path_env[i] != ':')
            continue;
        strncpy(fullpath, &path_env[start], (i - start)); // +2 for "/" and null
        fullpath[(i - start)] = '/';
        fullpath[(i - start) + 1] = 0;
        strcat(fullpath, cmd);
        if (access(fullpath, X_OK) == EXIT_SUCCESS)
            return strdup(fullpath);
        start = i + 1;
    }
    strcpy(fullpath, "./");
    strcat(fullpath, cmd);
    return strdup(fullpath);
}

char **append_arg(int argc, char **argv, char *addendum) {
    // +2: 1 for the new argument, and 1 for NULL
    char **new_argv = malloc((argc + 2) * sizeof(char *));
    if (new_argv == NULL) {
        perror("malloc");
        return NULL;
    }
    for (int i = 0; i < argc; i++)
        new_argv[i] = argv[i];
    new_argv[argc] = addendum;
    new_argv[argc + 1] = NULL;
    return new_argv;
}

/* The codes returned by this function are sourced from the man page on the
GNU xargs implementation. */
int launch(char **argv, char* path_env) {
    pid_t pid;
    int status;
    int exit_status;

    pid = fork();
    if (pid == 0) { // Child process
        char *cmdpath = resolve_executable_path(argv[0], path_env);
        if (cmdpath == NULL)
            return 127;
        if (execve(cmdpath, argv, environ) == -1) {
            perror("xargs execve");
            if (errno == ENOENT)
                return 127;
            return 126;
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) { // Error forking
        perror("xargs execve");
        return EXIT_FAILURE;
    } else { // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        if (WIFEXITED(status)) {
            exit_status = WEXITSTATUS(status);
            if (exit_status > 0 && exit_status <= 125)
                return 123;
            if (exit_status == 255)
                return 124;
        } else if (WIFSIGNALED(status)) {
            return 125;
        }
    }
    return EXIT_SUCCESS;
}

int xargs(int fd, int flags, int argc, char **argv) {
    char *item;
    int exit_code;
    /* This has to be strdup'd because strtok modifies its input. */
    char* path_env = strdup(getenv("PATH"));

    while ((item = readline()) != NULL) {
        if (strlen(item) == 0)
            continue;  // This is true of the GNU xargs too.
        char **new_argv = append_arg(argc, argv, item);
        if ((exit_code = launch(new_argv, path_env)) != EXIT_SUCCESS)
            return exit_code;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    char *arg;
    int flags = 0;
    int i;
    int fd;

    /* In theory, you could do "objcopy a.out", but it would do nothing at all,
    so we'll say less than three arguments is invalid. */
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (strlen(arg) == 0)
            continue;
        if (arg[0] != '-')
            break;  // We're moving on to parsing the command.
    }

    fd = STDIN_FILENO;
    return xargs(fd, flags, argc - i, &argv[i]);
}
