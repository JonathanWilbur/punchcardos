/*
Source: https://github.com/brenns10/lsh/tree/master
License: UNLICENSE at the time of copying.
*/
#include <sys/klog.h>        /* Definition of SYSLOG_* constants */
#include <sys/syscall.h>     /* Definition of SYS_* constants */
#ifndef NOLIBC
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __GLIBC__
#include <sys/klog.h>
#endif

#else

size_t strspn (const char *str, const char *accept) {
    size_t accept_len = strlen(accept);

    size_t i;
    for (i = 0; i < strlen(str); i++) {
        int match = 0;

        for (int j = 0; j < accept_len; j++) {
            if (str[i] == accept[j]) {
                match = 1;
                break;
            }
        }

        if (!match) {
            return i;
        }
    }
    return i;
}

size_t strcspn (const char *str, const char *reject) {
    size_t reject_len = strlen(reject);

    for (size_t i = 0; i < strlen(str); i++) {
        for (int j = 0; j < reject_len; j++) {
            if (str[i] == reject[j]) {
                return i;
            }
        }
    }
    return strlen(str); // No chars were rejected: the whole string is allowed.
}

// Copied from glibc directly. See licenses/glibc.license.txt.
char *strtok_r (char *s, const char *delim, char **save_ptr)
{
    char *end;

    if (s == NULL) {
        s = *save_ptr;
    }

    if (*s == '\0') {
        *save_ptr = s;
        return NULL;
    }

    /* Scan leading delimiters.  */
    s += strspn (s, delim);
    if (*s == '\0') {
        *save_ptr = s;
        return NULL;
    }

    /* Find the end of the token.  */
    end = s + strcspn (s, delim);
    if (*end == '\0') {
        *save_ptr = end;
        return s;
    }

    /* Terminate the token and make *SAVE_PTR point past it.  */
    *end = '\0';
    *save_ptr = end + 1;
    return s;
}

int syslog (int type, char *bufp, int len) {
    return my_syscall3(__NR_syslog, type, bufp, len);
}

#endif

/*
From the man page for syslog(2):
"The symbolic names are defined in the kernel source, but are not exported to
user space; you will either need to use the numbers, or define the names
yourself."
*/
#ifndef SYSLOG_ACTION_CONSOLE_OFF
#define SYSLOG_ACTION_CONSOLE_OFF   6
#endif

// TODO: Format this file.

static int last_exit = 0;

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;
  char *env[] = { NULL };

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execve(args[0], args, env) == -1) { // TODO: You could support arguments here.
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    if (WIFEXITED(status)) {
        last_exit = WEXITSTATUS(status);
    }
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  char *token_save_ptr;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok_r(line, LSH_TOK_DELIM, &token_save_ptr);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok_r(NULL, LSH_TOK_DELIM, &token_save_ptr);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("%d> ", last_exit);
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
    /* These next lines silence the kernel ring buffer messages from being
    displayed on the console. While annoying, it is not necessary to handle, so
    we just ignore the return value. */
#ifdef NOLIBC
    syslog(SYSLOG_ACTION_CONSOLE_OFF, NULL, 0);
#elif defined(__GLIBC__)
    klogctl(SYSLOG_ACTION_CONSOLE_OFF, NULL, 0);
#endif
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
