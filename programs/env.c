#include <linux/limits.h>
#ifndef NOLIBC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#else

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

#define FLAG_IGNORE     (1 << 0)
#define FLAG_NULL       (1 << 1)
#define FLAG_DEBUG      (1 << 2)

const char *USAGE_MSG = "env [OPTION]... [-] [NAME=VALUE]... [COMMAND [ARG]...]";
static void print_usage () {
    puts(USAGE_MSG);
}

static int prefix (const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static void remove_env_var(char **envp, const char *varname) {
    size_t len = strlen(varname);
    char **env = envp;

    while (*env) {
        if (strncmp(*env, varname, len) == 0 && (*env)[len] == '=') {
            char **next = env;
            do {
                next[0] = next[1];
                next++;
            } while (*next);
            // Optional: Set the last pointer to NULL (already handled by the shift)
            return;
        }
        env++;
    }
}

static int print_env (char **envp, int null_delim) {
    while ((*envp) != NULL) {
        if (null_delim) {
            if (write(STDOUT_FILENO, *envp, strlen(*envp)) < 0) {
                perror("write");
                return EXIT_FAILURE;
            }
            if (write(STDOUT_FILENO, "\0", 1) < 0) {
                perror("write");
                return EXIT_FAILURE;
            }
        } else {
            if (puts(*envp) < 0) {
                perror("puts");
                return EXIT_FAILURE;
            }
        }
        envp++;
    }
    return EXIT_SUCCESS;
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

int launch (char **cmd, char **envp, char *dir) {
    if (dir != NULL && chdir(dir) < 0) {
        perror("env @ chdir");
    }
    if (execve(cmd[0], cmd, envp) < 0) {
        perror("env @ execve");
    }
    return EXIT_FAILURE;
}

static int env (int argc, char **cmd, char **envp, int flags, char *dir, char *path_var) {
    char **empty_env = { NULL };

    if (path_var != NULL) // FIXME: Replicate to xargs
        cmd[0] = resolve_executable_path(cmd[0], path_var);
    if (argc == 0)
        return print_env(envp, flags & FLAG_NULL);
    if (flags & FLAG_IGNORE) {
        return launch(cmd, empty_env, dir);
    } else {
        return launch(cmd, envp, dir);
    }
}

#define NEXT_ARG_NONE       0
#define NEXT_ARG_UNSET      1
#define NEXT_ARG_CHDIR      2
#define NEXT_ARG_SPLIT_STR  3
// TODO: Handle split string

/*
-i, --ignore-environment    start with an empty environment
-0, --null                  end each output line with NUL, not newline
-u, --unset=NAME            remove variable from the environment
-C, --chdir=DIR             change working directory to DIR
-S, --split-string=S        process and split S into separate arguments; used to pass multiple arguments on shebang lines
-v, --debug                 print verbose information for each processing step
--help                      display this help and exit
--version                   output version information and exit
*/
int main (int argc, char **argv, char **envp) {
    int flags = 0;
    int i = 0;
    int next_arg = NEXT_ARG_NONE;
    char *arg;
    char *change_to_dir = NULL;
    size_t arglen;
    char *path_var = getenv("PATH");

parse_args:
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        arglen = strlen(arg);
        if (arglen == 0)
            continue;
        switch (next_arg) {
        case NEXT_ARG_NONE:
            break;
        case NEXT_ARG_CHDIR:
            change_to_dir = arg;
            break;
        case NEXT_ARG_UNSET:
            remove_env_var(envp, arg);
            break;
        case NEXT_ARG_SPLIT_STR:
            break;
        default:
            puts("Internal error when parsing command line arguments.");
            return EXIT_FAILURE;
        }
        if (next_arg != NEXT_ARG_NONE) {
            next_arg = NEXT_ARG_NONE;
            continue; // We already handled it.
        }
parse_short_args:
        if (arglen >= 2 && arg[0] == '-' && arg[1] != '-') {
            for (int j = 1; j < arglen; j++) {
                switch (arg[j]) {
                case 'i':
                    flags |= FLAG_IGNORE;
                    break;
                case '0':
                    flags |= FLAG_NULL;
                    break;
                case 'v':
                    flags |= FLAG_DEBUG;
                    break;
                case 'u':
                    next_arg = NEXT_ARG_UNSET;
                    break;
                case 'C':
                    next_arg = NEXT_ARG_CHDIR;
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
        if (!strcmp(arg, "-")) {
            flags |= FLAG_IGNORE;
            continue;
        }
        if (arglen < 2 || arg[0] != '-' || arg[1] != '-')
            break;
        if (!strcmp(arg, "--ignore-environment")) {
            flags |= FLAG_IGNORE;
        }
        else if (!strcmp(arg, "--null")) {
            flags |= FLAG_NULL;
        }
        else if (!strcmp(arg, "--debug")) {
            flags |= FLAG_DEBUG;
        }
        else if (!strcmp(arg, "--unset") || !strcmp(arg, "-u")) {
            next_arg = NEXT_ARG_UNSET;
        }
        else if (!strcmp(arg, "--chdir") || !strcmp(arg, "-C")) {
            next_arg = NEXT_ARG_CHDIR;
        }
        else if (prefix("--unset=", arg)) {
            remove_env_var(envp, &arg[strlen("--unset=")]);
        }
        else if (prefix("--chdir=", arg)) {
            change_to_dir = &arg[strlen("--chdir=")];
        }
        else {
            printf("Option not understood: %s\n", arg);
            return EXIT_FAILURE;
        }
    }

    if (next_arg != NEXT_ARG_NONE) {
        print_usage();
        return EXIT_FAILURE;
    }

    return env(argc - i, &argv[i], envp, flags, change_to_dir, path_var);
}