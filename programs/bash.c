/*
Copied from: https://github.com/fortytwobytes/minibash/tree/main
Licensed under an MIT license at the time of copying.

Jonathan M. Wilbur modified this code a lot:
- Making it not depend on an external readline library
- Putting all source in one file
- Porting to work with nolibc

*/
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static char linebuf[BUFSIZ];
static int linebufsize = 0;

/* An absolutely STINKY implementation for the sake of simplicity. */
static char* readline (char* prompt) {
    char c;
    char *line;

    write(STDOUT_FILENO, prompt, strlen(prompt)); // Intentionally ignoring errors.

    while ((c = getchar()) != EOF) {
        if (c == '\r')
            continue;
        if (c == '\n')
            break;
        linebuf[linebufsize++] = (char)c;
    }
    
    line = malloc(linebufsize + 1);
    if (line == NULL) {
        return NULL;
    }
    memcpy(line, linebuf, linebufsize);
    line[linebufsize] = 0; // Add null terminator.
    linebufsize = 0;

    return line;
}

#define TRUE 1
#define FALSE 0

#define PROMPT "minibash $> "

#define READ 0
#define WRITE 1

#define ADD 1
#define APPEND 2

typedef struct s_global t_global;
extern t_global g_global;
typedef struct s_envs t_envs;
typedef struct s_cmd t_cmd;

struct s_envs {
    char *name;
    char *value;
    int index;
    t_envs *next;
};

struct s_global {
    int exit_status;
    int heredoc_flag;
    t_envs *envs;
};

void shell_loop(void);

int size_of_env(void);
void index_envs(void);
int is_export_valid(char *exp);
int cd(char **args, int fd);
void pwd(char **args, int fd);
void echo(char **args, int fd);
void env(char **args, int fd);
int unset(char **args, int fd);
int export(char **args, int fd);
int shell_exit(char **args);
int is_builtins(char *cmd);
int exec_builtins(char **args, int outfile);
char *get_name_(char *args);
char *get_value_(int index, char *str);

typedef struct s_envs t_envs;
typedef struct s_cmd t_cmd;

void shell_loop(void);
void execute(t_cmd *head);
int exec_single_cmd(t_cmd *head, t_cmd *cmd);
void command_not_found(t_cmd *cmd);
int is_updated(char *name, char *value, int option);
void add_env(t_envs **envs, char *name, char *value);
void pop_env(char *name);
char *ft_getenv(char *name);
char *get_env_value(char *name);
t_envs *envs_init(char **env);
void ft_setenv(char *name, char *value);
void close_all_fds(t_cmd *head);

void set_exit_status(int sig);
char **dynamic_env(void);
char *here_doc_name(void);
char *ft_getpath(char *cmd);
void update_env(char *name, char *value);

void default_signals(void);
void handle_signals(void);
void ignore_signals(void);
void sigint_heredoc(void);
void change_flag(int s);

int ft_open(char *path, int oflag, int mode);
void ft_close(int fildes);
void ft_pipe(int fd[2]);
void ft_dup(int fildes);
void ft_wait(int *stat_loc);
void ft_dup2(int fildes, int fildes2);
void ft_execve(char *path, char **argv);
void ft_waitpid(pid_t pid);
pid_t ft_fork(void);

#define EXIT_SYNTAX 258

#define OPERATOR -1
#define PIPE 0
#define REDIRECTION 1
#define WORD 2
#define HEREDOC 3
#define LIMITER 4
#define FILE 5

#define READ_END 0
#define WRITE_END 1

typedef struct s_token {
    char *token;
    int type;
    int pipe[2];
    struct s_token *next;
} t_token;

typedef struct s_cmd {
    char *cmd;
    char **args;
    char *path;
    int infile;
    int outfile;
    struct s_cmd *next;
} t_cmd;

int parse(t_token *tokens);
t_cmd *parse_line(char *line);
t_token *split_by_operator(char **words);
int next_quote(int i, char quote, char *line);
t_token *expand_tokens(t_token *tokens);
int is_env_name(char c);
t_cmd *convert_to_cmds(t_token *tokens);
char *parameter_expansion(char *token);
char *get_value(char *s);
char *quotes_removal(char *token);
void add_back(t_token **head, char *s);
void add_middle(t_token *token, char *word);
void print_list(t_token *head);
void free_tokens(t_token *tokens);
void tokenise_heredoc(t_token *token);
int is_operator(char c);
t_token *word_spliting(t_token *token);
void free_cmd(t_cmd *head);
void close_fds(t_cmd *cmd);
t_token *add_cmd(t_cmd **cmds, t_token *tokens);
int handle_heredoc(t_cmd *cmd, char *limiter, char *file);
t_token *next_pipe(t_token *tokens);
t_token *next_pipe(t_token *tokens);
void handle_pipes(t_cmd *cmd, t_token *tokens);
int handle_redirection(t_cmd *cmd, t_token *tokens);
void handle_cmd(t_cmd *cmd, t_token *tokens);
char *get_next_line(int fd);
t_token *next_pipe(t_token *tokens);
void open_pipes(t_token *tokens);
void expands_dollars_dollars(char *token);
void free_token_word(t_token *token, char *word);
void check_and_redirect(int *inf_out, int fd);
void free_all(t_token *tokens);
int handle_heredocs(t_cmd *cmd, t_token *tokens);
int get_name_len(char *token, int i);
char *heredoc_expansion(char *line);

/* ---------- MISC ------------ */
int ft_atoi(char *str);
int is_char_in_str(char *str, char c);

/* ---------- ERRORS ------------ */
void ft_exit(int status, char *msg);
void fatal(char *cmd, char *msg);

/* ---------- STRINGS ------------ */
int ft_strcmp(char *s1, char *s2);
char *ft_strdup(char *s1);
char **ft_split(char *s, char c);
void *ft_calloc(size_t size);
size_t ft_strlen(char *str);
void free_split(char **s);
char *ft_substr(char *s, size_t start, size_t end);
int is_upper(char c);
int is_lower(char c);
int is_num(char c);
char *ft_strjoin(char *s1, char *s2);
char *ft_strjoin_sep(char *s1, char *s2, char sep);
int contains(char *s, char c);
char *ft_itoa(int n);
void *ft_memcpy(void *dst, void *src, size_t size);

/* ---------- PRINTS ------------ */
void ft_putchar_fd(char c, int fd);
void ft_putstr_fd(char *s, int fd);
void ft_putnbr_fd(long nbr, int fd);
void ft_puthex_fd(unsigned long nbr, int fd, char form);

static int home_case(void);
static int args_len(char **args);
static void update_pwd(char *new_path);

int cd(char **args, int fd) {
    int i;
    int exit_status;

    i = args_len(args);
    if (i == 1) {
        exit_status = home_case();
        ft_putchar_fd(0, fd);
        update_pwd(ft_getenv("HOME"));
        return (exit_status);
    } else {
        if (chdir(args[1])) {
            fatal("cd", "no such file or directory");
            return (1);
        }
    }
    ft_putchar_fd(0, fd);
    update_pwd(args[1]);
    return (0);
}

static int args_len(char **args) {
    int i;

    i = 0;
    while (args[i])
        i++;
    return (i);
}

static void update_pwd(char *new_path) {
    char *new_wd;

    update_env("OLDPWD", ft_getenv("PWD"));
    new_wd = getcwd(NULL, 0);
    if (new_wd == NULL) {
        new_wd = ft_strjoin_sep(ft_getenv("OLDPWD"), new_path, '/');
        update_env("PWD", new_wd);
        fatal("cd", "error retrieving current directory");
    } else
        update_env("PWD", new_wd);
    free(new_wd);
}

static int home_case(void) {
    char *home;

    home = ft_getenv("HOME");
    if (!home) {
        fatal("cd", "HOME not set");
        return (1);
    }
    if (chdir(home) != 0) {
        fatal("cd", "no such file or directory");
        return (1);
    }
    return (0);
}

static int is_flag_n(char *flag);
static void echo_args(int *n, char **args, int fd);

void echo(char **args, int fd) {
    int new_line;
    int i;

    new_line = TRUE;
    i = 1;
    if (args[i] == NULL) {
        ft_putstr_fd("\n", fd);
        return;
    }
    while (args[i] && is_flag_n(args[i])) {
        new_line = FALSE;
        i++;
    }
    echo_args(&new_line, args + i, fd);
    if (new_line == TRUE)
        ft_putstr_fd("\n", fd);
}

static int is_flag_n(char *flag) {
    int i;

    i = 0;
    if (!ft_strcmp(flag, "-n"))
        return (TRUE);
    if (flag[0] != '-')
        return (FALSE);
    i++;
    while (flag && flag[i]) {
        if (flag[i] != 'n')
            return (FALSE);
        i++;
    }
    return (TRUE);
}

static void echo_args(int *n, char **args, int fd) {
    int i;

    (void)n;
    i = 0;
    while (args && args[i]) {
        ft_putstr_fd(args[i], fd);
        if (args[i + 1])
            ft_putchar_fd(' ', fd);
        i++;
    }
}

void env(char **args, int fd) {
    t_envs *tmp;

    (void)args;
    tmp = g_global.envs;
    while (tmp) {
        if (tmp->value == NULL) {
            tmp = tmp->next;
            continue;
        }
        ft_putstr_fd(tmp->name, fd);
        ft_putchar_fd('=', fd);
        ft_putstr_fd(tmp->value, fd);
        ft_putchar_fd('\n', fd);
        tmp = tmp->next;
    }
}

static int print_export(int fd);
static void export_conditions(int idx, char *arg);
static void print_export_helper(int fd, int counter);

int export(char **args, int fd) {
    int idx;
    int exit_status;

    exit_status = 1;
    if (*(args + 1) == NULL)
        return (print_export(fd));
    args++;
    while (*args) {
        idx = is_export_valid(*args);
        if (idx == -1) {
            fatal("export", "not a valid identifier");
            args++;
            continue;
        }
        exit_status = 0;
        export_conditions(idx, *args);
        args++;
    }
    ft_putchar_fd(0, fd);
    return (exit_status);
}

static void export_conditions(int idx, char *arg) {
    char *name;
    char *value;
    int is_modified;

    name = get_name_(arg);
    idx = ft_strlen(name);
    value = get_value_(idx, arg);
    if (*(arg + idx) == 0)
        is_modified = FALSE;
    else if (*(arg + idx) == '+')
        is_modified = is_updated(name, value, APPEND);
    else
        is_modified = is_updated(name, value, ADD);
    if (is_modified == FALSE)
        add_env(&g_global.envs, name, value);
    free(name);
    free(value);
}

static void print_export_helper(int fd, int counter) {
    t_envs *tmp;

    tmp = g_global.envs;
    while (tmp) {
        if (counter == tmp->index) {
            if (!ft_strcmp(tmp->name, "_"))
                break;
            ft_putstr_fd("declare -x ", fd);
            ft_putstr_fd(tmp->name, fd);
            if (tmp->value != NULL) {
                ft_putchar_fd('=', fd);
                ft_putchar_fd('\"', fd);
                ft_putstr_fd(tmp->value, fd);
                ft_putchar_fd('\"', fd);
            }
            ft_putchar_fd('\n', fd);
            break;
        }
        tmp = tmp->next;
    }
}

static int print_export(int fd) {
    int size;
    int counter;

    counter = 0;
    size = size_of_env();
    index_envs();
    while (counter < size) {
        print_export_helper(fd, counter);
        counter++;
    }
    return (0);
}

char *get_name_(char *args) {
    char *buffer;
    int i;

    i = 0;
    buffer = NULL;
    while (args[i]) {
        if (args[i] == '=')
            break;
        if (args[i] == '+' && args[i + 1] == '=')
            break;
        i++;
    }
    buffer = ft_calloc(i + 1);
    ft_memcpy(buffer, args, i);
    return (buffer);
}

char *get_value_(int index, char *str) {
    int len;
    char *buffer;

    len = ft_strlen(str);
    if (str[index] == 0)
        return (NULL);
    if (str[index] == '+')
        index++;
    else if (str[index + 1] == 0)
        return (ft_strdup(""));
    buffer = ft_calloc(len - index + 1);
    ft_memcpy(buffer, str + index + 1, len - index);
    return (buffer);
}

void pwd(char **args, int fd) {
    char *pwdir;

    (void)args;
    pwdir = ft_getenv("PWD");
    if (pwdir == NULL) {
        pwdir = getcwd(NULL, 0);
        if (pwdir == NULL)
            fatal("pwd", "error retrieving current directory");
        else
            ft_putstr_fd(pwdir, fd);
        free(pwdir);
    } else
        ft_putstr_fd(pwdir, fd);
    ft_putchar_fd('\n', fd);
}

int is_all_num(char *s) {
    while (*s) {
        if (!is_num(*s))
            return (0);
        s++;
    }
    return (1);
}

// If n is omitted, the exit status is that of the last command executed
int shell_exit(char **args) {
    ft_putstr_fd("exit\n", 2);
    if (args[1] == NULL)
        exit(g_global.exit_status);
    if (is_all_num(args[1])) {
        if (args[2] == NULL)
            exit(ft_atoi(args[1]));
        else {
            ft_putstr_fd("too many arguments\n", 2);
            return (1);
        }
    } else {
        ft_putstr_fd("numeric argument required\n", 2);
        exit(255);
    }
    return (0);
}

static int unset_regex(char *s);

int unset(char **args, int fd) {
    if (*(args + 1) == NULL)
        return (0);
    args++;
    while (*args) {
        if (unset_regex(*args)) {
            fatal("unset", "invalid identifier");
            return (1);
        }
        if (!ft_strcmp(*args, "_")) {
            args++;
            continue;
        }
        pop_env(*args);
        args++;
    }
    ft_putchar_fd(0, fd);
    return (0);
}

static int unset_regex(char *s) {
    if (!is_lower(*s) && !is_upper(*s) && *s != '_')
        return (1);
    s++;
    while (*s) {
        if (!is_env_name(*s))
            return (1);
        s++;
    }
    return (0);
}

int is_builtins(char *cmd) {
    if (!ft_strcmp(cmd, "cd"))
        return (1);
    if (!ft_strcmp(cmd, "pwd"))
        return (1);
    if (!ft_strcmp(cmd, "echo"))
        return (1);
    if (!ft_strcmp(cmd, "export"))
        return (1);
    if (!ft_strcmp(cmd, "unset"))
        return (1);
    if (!ft_strcmp(cmd, "env"))
        return (1);
    if (!ft_strcmp(cmd, "exit"))
        return (1);
    return (0);
}

int exec_builtins(char **args, int outfile) {
    if (!ft_strcmp(args[0], "cd"))
        return (cd(args, outfile));
    if (!ft_strcmp(args[0], "pwd"))
        pwd(args, outfile);
    if (!ft_strcmp(args[0], "echo"))
        echo(args, outfile);
    if (!ft_strcmp(args[0], "export"))
        return (export(args, outfile));
    if (!ft_strcmp(args[0], "unset"))
        return (unset(args, outfile));
    if (!ft_strcmp(args[0], "env"))
        env(args, outfile);
    if (!ft_strcmp(args[0], "exit"))
        shell_exit(args);
    return (0);
}

void index_envs(void) {
    t_envs *tmp;
    t_envs *tmp1;

    tmp = g_global.envs;
    while (tmp) {
        tmp->index = 0;
        tmp = tmp->next;
    }
    tmp = g_global.envs;
    while (tmp) {
        tmp1 = g_global.envs;
        while (tmp1) {
            if (ft_strcmp(tmp->name, tmp1->name) > 0)
                tmp->index++;
            tmp1 = tmp1->next;
        }
        tmp = tmp->next;
    }
}

int size_of_env(void) {
    int i;
    t_envs *tmp;

    i = 0;
    tmp = g_global.envs;
    while (tmp) {
        i++;
        tmp = tmp->next;
    }
    return (i);
}

int is_export_valid(char *exp) {
    int i;

    i = 0;
    if (!is_lower(*exp) && !is_upper(*exp) && *exp != '_')
        return (-1);
    while (exp[i]) {
        if (exp[i] == '=')
            return (i + 1);
        if (exp[i] == '+' && exp[i + 1] == '=')
            return (i + 2);
        if (!is_env_name(exp[i]))
            return (-1);
        i++;
    }
    return (i);
}

void add_env(t_envs **envs, char *name, char *value) {
    t_envs *new;
    t_envs *tmp;

    new = ft_calloc(sizeof(t_envs));
    new->name = ft_strdup(name);
    if (value == NULL)
        new->value = NULL;
    else
        new->value = ft_strdup(value);
    new->index = 0;
    new->next = NULL;
    if (!envs || !*envs) {
        *envs = new;
        return;
    }
    tmp = *envs;
    while (tmp->next)
        tmp = tmp->next;
    tmp->next = new;
}

void pop_env(char *name) {
    t_envs *tmp;
    t_envs *prev;

    tmp = g_global.envs;
    prev = NULL;
    while (tmp) {
        if (!ft_strcmp(tmp->name, name)) {
            if (prev)
                prev->next = tmp->next;
            else
                g_global.envs = tmp->next;
            free(tmp->name);
            free(tmp->value);
            free(tmp);
            return;
        }
        prev = tmp;
        tmp = tmp->next;
    }
}

t_envs *envs_init(char **env) {
    int i;
    char *name;
    char *value;
    char **sp;
    t_envs *envs;

    i = 0;
    envs = NULL;
    while (env && env[i]) {
        sp = ft_split(env[i], '=');
        name = sp[0];
        value = sp[1];
        add_env(&envs, name, value);
        free_split(sp);
        i++;
    }
    return (envs);
}

char *ft_getenv(char *name) {
    t_envs *tmp;

    tmp = g_global.envs;
    while (tmp) {
        if (!ft_strcmp(tmp->name, name))
            return (tmp->value);
        tmp = tmp->next;
    }
    return (NULL);
}

char *get_env_value(char *name) {
    t_envs *tmp;

    tmp = g_global.envs;
    if (*name == '?')
        return (ft_itoa(g_global.exit_status));
    if (!tmp)
        return (NULL);
    while (tmp) {
        if (!ft_strcmp(tmp->name, name)) {
            if (tmp->value)
                return (tmp->value);
            return ("");
        }
        tmp = tmp->next;
    }
    return ("");
}

void close_all_fds(t_cmd *head) {
    t_cmd *tmp;

    tmp = head;
    while (tmp) {
        if (tmp->outfile != 0)
            ft_close(tmp->outfile);
        if (tmp->infile != 0)
            ft_close(tmp->infile);
        tmp = tmp->next;
    }
}

// if fork fails the exit status is 1
int exec_single_cmd(t_cmd *head, t_cmd *cmd) {
    pid_t pid;
    int exit_status;

    pid = ft_fork();
    if (pid == -1)
        return (-1);
    if (pid == 0) {
        default_signals();
        if (cmd->infile != 0)
            ft_dup2(cmd->infile, 0);
        if (cmd->outfile != 0)
            ft_dup2(cmd->outfile, 1);
        close_all_fds(head);
        if (is_builtins(cmd->cmd)) {
            exit_status = exec_builtins(cmd->args, 1);
            exit(exit_status);
        }
        cmd->path = ft_getpath(cmd->cmd);
        if (cmd->path == NULL)
            command_not_found(cmd);
        ft_execve(cmd->path, cmd->args);
    }
    return (pid);
}

// WTERMSIG(status) returns the number of the signal
void wait_all_childs(int last_pid) {
    int pid;
    int status;
    int tmp_pid;

    pid = 0;
    tmp_pid = last_pid;
    if (last_pid == 0)
        return;
    while (pid != -1) {
        pid = wait(&status);
        if (last_pid == pid) {
            if (WIFSIGNALED(status))
                g_global.exit_status = 128 + WTERMSIG(status);
            else
                g_global.exit_status = WEXITSTATUS(status);
        }
    }
    if (tmp_pid == -1)
        g_global.exit_status = 1;
}

int one_builtin(t_cmd *head) {
    if (is_builtins(head->cmd) && !head->next) {
        if (head->outfile)
            g_global.exit_status = exec_builtins(head->args, head->outfile);
        else
            g_global.exit_status = exec_builtins(head->args, 1);
        close_all_fds(head);
        free_cmd(head);
        return (1);
    }
    return (0);
}

void execute(t_cmd *head) {
    t_cmd *tmp;
    int last_pid;

    tmp = head;
    last_pid = 0;
    if (!head)
        return;
    if (one_builtin(head))
        return;
    while (tmp) {
        if (tmp->cmd)
            last_pid = exec_single_cmd(head, tmp);
        if (last_pid == -1)
            break;
        tmp = tmp->next;
    }
    close_all_fds(head);
    wait_all_childs(last_pid);
    free_cmd(head);
}

static char *get_id(char *name);

char *here_doc_name(void) {
    char *name;
    char *tty_name;

    tty_name = ttyname(0);
    name = get_id(tty_name);
    return (name);
}

static char *get_id(char *name) {
    char *id;
    static int i;
    char *id2;
    char *tmp;

    if (!name)
        return (NULL);
    while (*name && !is_num(*name))
        name++;
    while (TRUE) {
        id2 = ft_itoa(i);
        tmp = ft_strjoin_sep(name, id2, '_');
        id = ft_strjoin("/tmp/.heredoc_", tmp);
        free(id2);
        free(tmp);
        i++;
        if (access(id, F_OK) == -1)
            break;
        free(id);
    }
    return (id);
}

void command_not_found(t_cmd *cmd) {
    write(2, "command not found: ", 20);
    write(2, cmd->cmd, strlen(cmd->cmd));
    write(2, "\n", 1);
    exit(127);
}

int is_updated(char *name, char *value, int option) {
    t_envs *tmp;
    char *holder;

    tmp = g_global.envs;
    if (value == NULL)
        return (FALSE);
    while (tmp) {
        if (!ft_strcmp(tmp->name, name)) {
            holder = tmp->value;
            if (option == ADD)
                tmp->value = ft_strdup(value);
            else if (option == APPEND)
                tmp->value = ft_strjoin(holder, value);
            free(holder);
            return (TRUE);
        }
        tmp = tmp->next;
    }
    return (FALSE);
}

char **dynamic_env(void) {
    int i;
    int j;
    char **env;
    t_envs *tmp;

    i = 0;
    tmp = g_global.envs;
    while (tmp) {
        tmp = tmp->next;
        i++;
    }
    env = ft_calloc(sizeof(char *) * (i + 1));
    j = 0;
    tmp = g_global.envs;
    while (j < i) {
        if (tmp->value == NULL)
            tmp->value = "";
        env[j] = ft_strjoin_sep(tmp->name, tmp->value, '=');
        tmp = tmp->next;
        j++;
    }
    return (env);
}

// the value should be allocated before calling ft_setenv
void ft_setenv(char *name, char *value) {
    t_envs *tmp;

    tmp = g_global.envs;
    while (tmp) {
        if (!ft_strcmp(tmp->name, name)) {
            free(tmp->value);
            tmp->value = value;
            return;
        }
        tmp = tmp->next;
    }
}

void update_env(char *name, char *value) {
    t_envs *tmp;
    char *holder;

    tmp = g_global.envs;
    if (!name || !value)
        return;
    while (tmp) {
        if (!ft_strcmp(tmp->name, name)) {
            holder = tmp->value;
            tmp->value = ft_strdup(value);
            free(holder);
            return;
        }
        tmp = tmp->next;
    }
}

char *ft_getpath(char *cmd) {
    int i;
    char *tmp;
    char *paths;
    char **sp_paths;

    i = -1;
    if (is_char_in_str(cmd, '/'))
        return (ft_strdup(cmd));
    paths = ft_getenv("PATH");
    if (!paths)
        return (NULL);
    sp_paths = ft_split(paths, ':');
    while (sp_paths[++i]) {
        tmp = ft_strjoin_sep(sp_paths[i], cmd, '/');
        if (access(tmp, X_OK) == -1)
            free(tmp);
        else
            return (free_split(sp_paths), tmp);
    }
    free_split(sp_paths);
    return (NULL);
}

int is_char_in_str(char *str, char c) {
    while (str && *str) {
        if (*str == c)
            return (1);
        str++;
    }
    return (0);
}

static int is_space(char c);
static void ft_add_history(char *line);

void shell_loop(void) {
    char *line;
    t_cmd *cmd;

    while (TRUE) {
        handle_signals();
        line = readline(PROMPT);
        if (!line)
            break;
        ft_add_history(line);
        cmd = parse_line(line);
        if (g_global.heredoc_flag) {
            g_global.heredoc_flag = 0;
            close_all_fds(cmd);
            free_cmd(cmd);
            free(line);
            continue;
        }
        ignore_signals();
        execute(cmd);
        free(line); // TODO: Will this be a problem with readline?
    }
}

// 512 history items is enough.
static char *history[512];
static int history_size = 0;

// Jonathan Wilbur's sick invention.
static void add_history (char *line) {
    history[history_size] = line;
}

static void ft_add_history(char *line) {
    char *ptr;

    if (!line)
        return;
    ptr = line;
    while (*line && is_space(*line)) {
        line++;
    }
    if (*line != 0)
        add_history(ptr);
}

static int is_space(char c) {
    return (c == ' ' || (c >= '\t' && c <= '\r'));
}

// rl_on_new_line  : tells that we moved to a nl
// rl_replace_line : replace the content of the rl buffer
// rl_redisplay    : change what is on the screen with
//  the actual content of the buffer
void handle_parent_sigint(int sig) {
    (void)sig;
    write(1, "\n", 1);
    g_global.exit_status = 1;
    // FIXME: This might be broken.
    // rl_on_new_line();
    // rl_replace_line("", 0);
    // rl_redisplay();
}

void handle_signals(void) {
    struct sigaction sa_sigint;
    struct sigaction sa_sigquit;

    sa_sigint.sa_handler = &handle_parent_sigint;
    sa_sigquit.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa_sigint, NULL);
    sigaction(SIGQUIT, &sa_sigquit, NULL);
}

void default_signals(void) {
    struct sigaction sa_sigint;
    struct sigaction sa_sigquit;

    sa_sigint.sa_handler = SIG_DFL;
    sa_sigquit.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa_sigint, NULL);
    sigaction(SIGQUIT, &sa_sigquit, NULL);
}

void ignore_signals(void) {
    struct sigaction sa_sigint;
    struct sigaction sa_sigquit;

    sa_sigint.sa_handler = SIG_IGN;
    sa_sigquit.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa_sigint, NULL);
    sigaction(SIGQUIT, &sa_sigquit, NULL);
}

void sigint_heredoc(void) {
    struct sigaction sa_sigint;

    sa_sigint.sa_handler = &change_flag;
    sigaction(SIGINT, &sa_sigint, NULL);
}

void ft_close(int fd) {
    if (close(fd) == -1)
        ft_exit(errno, strerror(errno));
}

void ft_dup(int fd) {
    if (dup(fd) == -1)
        ft_exit(errno, strerror(errno));
}

void ft_dup2(int oldfd, int newfd) {
    if (dup2(oldfd, newfd) == -1)
        ft_exit(errno, strerror(errno));
}

void ft_execve(char *path, char **argv) {
    if (execve(path, argv, dynamic_env()) == -1) {
        ft_putstr_fd("rmshell: ", STDERR_FILENO);
        ft_putstr_fd(argv[0], STDERR_FILENO);
        ft_putstr_fd(": No such file or directory\n", STDERR_FILENO);
        exit(errno);
    }
}

int ft_fork(void) {
    pid_t pid;

    pid = fork();
    if (pid == -1)
        perror("");
    return (pid);
}

int ft_open(char *path, int flags, int mode) {
    int fd;

    fd = open(path, flags, mode);
    if (fd == -1)
        perror(path);
    return (fd);
}

void ft_pipe(int fd[2]) {
    if (pipe(fd) == -1)
        ft_exit(errno, strerror(errno));
}

void ft_waitpid(pid_t pid) {
    int status;

    if (waitpid(pid, &status, 0) == -1)
        ft_exit(WEXITSTATUS(status), strerror(errno));
}

void ft_wait(int *status) {
    if (wait(status) == -1)
        ft_exit(errno, strerror(errno));
}

void ft_wait3(int *status, int options, struct rusage *rusage) {
    if (wait3(status, options, rusage) == -1)
        ft_exit(errno, strerror(errno));
}

void ft_wait4(pid_t pid, int *status, int options, struct rusage *rusage) {
    if (wait4(pid, status, options, rusage) == -1)
        ft_exit(errno, strerror(errno));
}

int is_upper(char c) {
    if (c >= 'A' && c <= 'Z')
        return (1);
    return (0);
}

int is_lower(char c) {
    if (c >= 'a' && c <= 'z')
        return (1);
    return (0);
}

int is_num(char c) {
    if (c >= '0' && c <= '9')
        return (1);
    return (0);
}

int contains(char *s, char c) {
    int i;

    i = 0;
    while (s[i]) {
        if (s[i] == c)
            return (1);
        i++;
    }
    return (0);
}

void fatal(char *cmd, char *msg) {
    ft_putstr_fd("rmshell: ", STDERR_FILENO);
    ft_putstr_fd(cmd, STDERR_FILENO);
    ft_putstr_fd(": ", STDERR_FILENO);
    ft_putstr_fd(msg, STDERR_FILENO);
    ft_putchar_fd('\n', STDERR_FILENO);
}

int ft_atoi(char *str) {
    int i;
    int sign;
    int result;

    i = 0;
    sign = 1;
    result = 0;
    while (str[i] == ' ' || (str[i] >= 9 && str[i] <= 13))
        i++;
    if (str[i] == '-' || str[i] == '+') {
        if (str[i] == '-')
            sign = -1;
        i++;
    }
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return (result * sign);
}

static void ft_bzero(void *p, size_t bytes);

void *ft_calloc(size_t bytes) {
    void *p;

    p = malloc(bytes);
    if (!p)
        ft_exit(EXIT_FAILURE, "malloc error");
    ft_bzero(p, bytes);
    return (p);
}

static void ft_bzero(void *p, size_t bytes) {
    size_t i;

    i = 0;
    while (i < bytes) {
        *((char *)p + i) = 0;
        i++;
    }
}

void ft_exit(int status, char *msg) {
    if (msg) {
        ft_putstr_fd(msg, 2);
        ft_putstr_fd("\n", 2);
    }
    exit(status);
}

// a function that free split return : a 2d array of string terminated by NULL;
void free_split(char **s) {
    size_t i;

    i = 0;
    if (!s)
        return;
    while (s[i]) {
        free(s[i]);
        i++;
    }
    free(s);
}

static int ft_num_of_characters(int n, int *signe) {
    int num_of_characters;

    num_of_characters = 2;
    while (n / 10) {
        num_of_characters++;
        n /= 10;
    }
    *signe = 1;
    if (n < 0) {
        *signe = -1;
        num_of_characters++;
    }
    return (num_of_characters);
}

char *ft_itoa(int n) {
    int num_of_characters;
    int signe;
    char *sol;

    if (n == -2147483648)
        return (ft_strdup("-2147483648"));
    num_of_characters = ft_num_of_characters(n, &signe);
    if (signe == -1)
        n *= -1;
    sol = ft_calloc(num_of_characters);
    --num_of_characters;
    while (n / 10) {
        sol[--num_of_characters] = n % 10 + '0';
        n /= 10;
    }
    sol[--num_of_characters] = n % 10 + '0';
    if (num_of_characters == 1)
        sol[0] = '-';
    return (sol);
}

void *ft_memcpy(void *dst, void *src, size_t n) {
    size_t index;

    index = -1;
    if (dst == src || !n)
        return (dst);
    while (++index < n)
        *(char *)(dst + index) = *(char *)(src + index);
    return (dst);
}

static char *allocate_word(char *str, char c);
static size_t ft_strlen_sep(char *str, char c);
static size_t count_strings(char *str, char c);

char **ft_split(char *s, char c) {
    char **str;
    size_t i;

    if (!s)
        return (0);
    i = 0;
    str = ft_calloc(sizeof(char *) * (count_strings((char *)s, c) + 1));
    while (*(char *)s) {
        while (*(char *)s && *(char *)s == c)
            s++;
        if (*(char *)s) {
            *(str + i) = allocate_word((char *)s, c);
            i++;
        }
        while (*(char *)s && *(char *)s != c)
            s++;
    }
    return (str);
}

static size_t count_strings(char *str, char c) {
    size_t i;
    size_t count;

    count = 0;
    i = 0;
    while (*(str + i)) {
        while (*(str + i) && *(str + i) == c)
            i++;
        if (*(str + i))
            count++;
        while (*(str + i) && *(str + i) != c)
            i++;
    }
    return (count);
}

static char *allocate_word(char *str, char c) {
    size_t len_word;
    size_t i;
    char *word;

    i = 0;
    len_word = ft_strlen_sep(str, c);
    word = ft_calloc(sizeof(char) * (len_word + 1));
    while (i < len_word) {
        *(word + i) = *(str + i);
        i++;
    }
    return (word);
}

static size_t ft_strlen_sep(char *str, char c) {
    size_t i;

    i = 0;
    while (*(str + i) && *(str + i) != c)
        i++;
    return (i);
}

static int get_return(char a, char b);

int ft_strcmp(char *s1, char *s2) {
    if (!s1 && !s2)
        return (0);
    if (!s1)
        return (-1);
    if (!s2)
        return (1);
    while (*s1 && *s2) {
        if (*s1 != *s2)
            return (get_return(*s1, *s2));
        s1++;
        s2++;
    }
    return (get_return(*s1, *s2));
}

static int get_return(char a, char b) {
    if (a == b)
        return (0);
    if (a > b)
        return (1);
    return (-1);
}

char *ft_strdup(char *s1) {
    char *dup;
    int i;

    i = 0;
    dup = ft_calloc(sizeof(char) * (ft_strlen(s1) + 1));
    while (s1 && s1[i]) {
        dup[i] = s1[i];
        i++;
    }
    return (dup);
}

char *ft_strjoin(char *s1, char *s2) {
    int i;
    int j;
    char *buffer;

    i = 0;
    i = -1;
    if (!s1 && !s2)
        return (NULL);
    if (!s1)
        return (ft_strdup(s2));
    if (!s2)
        return (ft_strdup(s1));
    buffer = ft_calloc(sizeof(char) * (ft_strlen(s1) + ft_strlen(s2) + 1));
    while (s1[++i])
        buffer[i] = s1[i];
    j = i;
    i = -1;
    while (s2[++i])
        buffer[j++] = s2[i];
    return (buffer);
}

char *ft_strjoin_sep(char *s1, char *s2, char sep) {
    int i;
    int j;
    char *buffer;

    i = -1;
    if (!s1 || !s2)
        return (NULL);
    buffer = ft_calloc(sizeof(char) * (ft_strlen(s1) + ft_strlen(s2) + 2));
    while (s1[++i])
        buffer[i] = s1[i];
    buffer[i] = sep;
    j = i + 1;
    i = -1;
    while (s2[++i])
        buffer[j++] = s2[i];
    return (buffer);
}

size_t ft_strlen(char *s) {
    size_t i;

    i = 0;
    if (!s)
        return (0);
    while (s[i])
        i++;
    return (i);
}

// TODO: It seems like a lot of these functions can be replaced with libc ones.
char *ft_substr(char *s, size_t start, size_t end) {
    size_t len;
    size_t i;
    char *word;

    len = end - start;
    i = 0;
    word = ft_calloc(len + 1);
    while (i < len) {
        word[i] = s[start + i];
        i++;
    }
    return (word);
}

static void put_nbr_base(unsigned long nbr, char *base, int base_len, int fd);

void ft_putchar_fd(char c, int fd) {
    write(fd, &c, 1);
}

void ft_putstr_fd(char *s, int fd) {
    if (!s)
        ft_putstr_fd("(null)", fd);
    else
        while (*s)
            ft_putchar_fd(*s++, fd);
}

void ft_putnbr_fd(long nbr, int fd) {
    if (nbr < 0) {
        nbr *= -1;
        ft_putchar_fd('-', fd);
    }
    put_nbr_base(nbr, "0123456789", 10, fd);
}

void ft_puthex_fd(unsigned long nbr, int fd, char form) {
    char *low_hex;
    char *upp_hex;

    low_hex = "0123456789abcdef";
    upp_hex = "0123456789ABCDEF";
    if (form == 'x')
        put_nbr_base((unsigned int)nbr, low_hex, 16, fd);
    else if (form == 'X')
        put_nbr_base((unsigned int)nbr, upp_hex, 16, fd);
    else
        put_nbr_base((unsigned long)nbr, low_hex, 16, fd);
}

static void put_nbr_base(unsigned long nbr,
                         char *base,
                         int baseLen,
                         int fd) {
    if (nbr >= (unsigned long)baseLen)
        put_nbr_base(nbr / baseLen, base, baseLen, fd);
    ft_putchar_fd(base[nbr % baseLen], fd);
}

// cat | ls | grep
// during exucution infile and outfile should be closee
// in the child and parent process
void handle_pipes(t_cmd *cmd, t_token *tokens) {
    if (tokens->type == PIPE)
        cmd->infile = tokens->pipe[READ_END];
    tokens = next_pipe(tokens);
    if (tokens)
        cmd->outfile = tokens->pipe[WRITE_END];
}

int redirect(t_cmd *cmd, char *type, char *file) {
    int fd;

    if (!ft_strcmp(type, ">>")) {
        fd = ft_open(file, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd == -1)
            return (0);
        check_and_redirect(&cmd->outfile, fd);
    }
    if (!ft_strcmp(type, ">")) {
        fd = ft_open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1)
            return (0);
        check_and_redirect(&cmd->outfile, fd);
    }
    if (!ft_strcmp(type, "<")) {
        fd = ft_open(file, O_RDONLY, 0);
        if (fd == -1)
            return (0);
        check_and_redirect(&cmd->infile, fd);
    }
    return (1);
}

// if a redirection fails we stop and don't execute the command
int handle_redirection(t_cmd *cmd, t_token *tokens) {
    int status;

    while (tokens && tokens->type != PIPE) {
        if (tokens->type == REDIRECTION) {
            if (!tokens->next)
                return (0);
            status = redirect(cmd, tokens->token, (tokens->next)->token);
            if (!status)
                return (status);
            tokens = tokens->next;
            tokens->type = FILE;
        } else
            tokens = tokens->next;
    }
    return (1);
}

// we check if fds are different to 0 because
// by default they are initialized to 0 by calloc

void handle_cmd(t_cmd *cmd, t_token *tokens) {
    int wc;
    int i;
    char **options;
    t_token *head;

    wc = 0;
    head = tokens;
    while (tokens && tokens->type != PIPE) {
        if (tokens->type == WORD)
            wc++;
        tokens = tokens->next;
    }
    options = ft_calloc(sizeof(char *) * (wc + 1));
    i = 0;
    while (head && head->type != PIPE) {
        if (head->type == WORD)
            options[i++] = head->token;
        head = head->next;
    }
    cmd->args = options;
    cmd->cmd = options[0];
}

t_cmd *convert_to_cmds(t_token *tokens) {
    t_token *head;
    t_cmd *cmds;

    head = tokens;
    cmds = NULL;
    open_pipes(head);
    while (tokens)
        tokens = add_cmd(&cmds, tokens);
    return (cmds);
}

void free_cmd(t_cmd *head) {
    t_cmd *tmp;

    while (head) {
        free_split(head->args);
        tmp = head->next;
        free(head);
        head = tmp;
    }
}

void close_fds(t_cmd *cmd) {
    if (cmd->infile != 0)
        close(cmd->infile);
    if (cmd->outfile != 0)
        close(cmd->outfile);
}

void add(t_cmd **cmds, t_cmd *new) {
    t_cmd *last;

    last = *cmds;
    if (!last)
        *cmds = new;
    else {
        while (last->next)
            last = last->next;
        last->next = new;
    }
}

t_token *free_cmds(t_cmd *new, t_token *tokens) {
    t_token *tmp;

    free(new);
    if (!tokens)
        return (NULL);
    if (tokens->type == PIPE)
        tokens = tokens->next;
    while (tokens && tokens->type != PIPE) {
        tmp = tokens;
        tokens = tokens->next;
        if (tmp->type == WORD)
            free(tmp->token);
    }
    return (tokens);
}

t_token *add_cmd(t_cmd **cmds, t_token *tokens) {
    t_cmd *new;
    int status;

    new = ft_calloc(sizeof(t_cmd));
    handle_pipes(new, tokens);
    if (tokens->type == PIPE)
        tokens = tokens->next;
    status = (handle_heredocs(new, tokens) && handle_redirection(new, tokens));
    if (!status) {
        close_fds(new);
        g_global.exit_status = 1;
        return (free_cmds(new, tokens));
    } else
        g_global.exit_status = 0;
    handle_cmd(new, tokens);
    add(cmds, new);
    return (next_pipe(tokens));
}

int is_env_name(char c) {
    if (is_lower(c) || is_upper(c) || is_num(c) || c == '_')
        return (1);
    return (0);
}

int handle_heredocs(t_cmd *cmd, t_token *tokens) {
    int status;

    while (tokens && tokens->type != PIPE) {
        if (tokens->type == HEREDOC) {
            tokens = tokens->next;
            status = handle_heredoc(cmd, tokens->token, here_doc_name());
            if (!status)
                return (status);
        } else
            tokens = tokens->next;
    }
    return (1);
}

// this function copies the token string into a new
// allocated string without copying the -1 occurenccense=
char *trim_quotes(char *token, int quotes_len) {
    char *trimed_token;
    int len;
    int i;
    int j;

    len = ft_strlen(token);
    trimed_token = ft_calloc(len - quotes_len + 1);
    i = 0;
    j = 0;
    while (token[i]) {
        if (token[i] != -1) {
            trimed_token[j] = token[i];
            j++;
        }
        i++;
    }
    free(token);
    return (trimed_token);
}

// this function removes the outer quotes of the passed string ,
// by marking the quotes that needs to be removed by -1
// and passing the token string to the trim quotes function
char *quotes_removal(char *token) {
    int i;
    char c;
    int quotes_len;

    i = 0;
    quotes_len = 0;
    while (token[i]) {
        if (token[i] == '"' || token[i] == '\'') {
            c = token[i];
            token[i] = -1;
            i = next_quote(i + 1, c, token);
            token[i] = -1;
            quotes_len++;
        }
        i++;
    }
    if (!quotes_len)
        return (token);
    return (trim_quotes(token, quotes_len));
}

// we will loop the linked list and remove empty strings that resulted from
// an unknown variable expansion .
t_token *remove_empty_tokens(t_token *tokens, t_token *head, t_token *prev) {
    while (tokens) {
        if (*(tokens->token) == 0) {
            if (prev == NULL) {
                head = tokens->next;
                free_token_word(tokens, tokens->token);
                tokens = head;
            } else {
                prev->next = tokens->next;
                free_token_word(tokens, tokens->token);
                tokens = prev->next;
            }
        } else {
            prev = tokens;
            tokens = tokens->next;
        }
    }
    return (head);
}

// in the expansion part we only have to handle parameter expansion ($) and quotes
// removal and we will implement these expansions in the same order of the
// bash cad variable expansion , word splitting => after word splitting we shall
//  remove
//  unquoted  empty strings that did result from a variable expension ,
//  then quotes removal
t_token *expand_tokens(t_token *tokens) {
    t_token *token;
    t_token *head;

    token = tokens;
    while (token) {
        if (token->type == WORD) {
            token->token = parameter_expansion(token->token);
            token = word_spliting(token);
        }
        token = token->next;
    }
    token = tokens;
    tokens = remove_empty_tokens(token, token, NULL);
    head = tokens;
    while (tokens) {
        if (tokens->type == WORD)
            tokens->token = quotes_removal(tokens->token);
        tokens = tokens->next;
    }
    return (head);
}

// this function returns the len of the variable name , exp : $abc => 4 , $ => 1
// [A-Z]{1,}[A-Z0-9_]
int get_name_len(char *token, int i) {
    int name_len;

    if (token[i + 1] == '?')
        return (2);
    if (!(is_upper(token[i + 1]) || is_lower(token[i + 1]) || token[i + 1] == '_'))
        return (0);
    name_len = 1;
    while (is_env_name(token[i + 1])) {
        i++;
        name_len++;
    }
    return (name_len);
}

// this function returns the first occurrence of a variable name
// if we have an occurrence of a $ alone we just mark it with -1
// to not interump with further calls to this function
// we will change the -1 latter
char *get_name(char *token) {
    int i;
    int name_len;
    int dbl_quote;

    i = 0;
    name_len = -1;
    dbl_quote = -1;
    while (token[i]) {
        name_len = -1;
        if (token[i] == '"')
            dbl_quote *= -1;
        if (token[i] == '\'' && dbl_quote == -1)
            i = next_quote(i + 1, token[i], token);
        if (token[i] == '$')
            name_len = get_name_len(token, i);
        if (name_len == 0)
            token[i] = -1;
        if (name_len > 0)
            break;
        i++;
    }
    if (name_len == 0 || name_len == -1)
        return (NULL);
    return (ft_substr(token, i, i + name_len));
}

// we need to check if the variable is not inside double quotes to
// not expand it , but check if we don't have double quotes before
int replace_before_name(char *new_token, char *token) {
    int i;
    int dbl_quotes;
    int sgl_quotes;

    dbl_quotes = 0;
    sgl_quotes = 0;
    i = 0;
    while (token[i] != '$' || sgl_quotes) {
        if (token[i] == '\'' && !dbl_quotes)
            sgl_quotes = !sgl_quotes;
        if (token[i] == '"')
            dbl_quotes = !dbl_quotes;
        new_token[i] = token[i];
        i++;
    }
    return (i);
}

// this function remplace the string value in the string
// name and returns an allocated string
// we add one the len of name bc it doesn't contain the '$' char
// (token[i] != '$' || j == 1) , it means while we are before an unquoted $
char *replace_name_value(char *token, char *name, char *value) {
    char *new_token;
    int i;
    int j;
    int k;

    i = 0;
    j = 0;
    new_token = ft_calloc(ft_strlen(token) + (ft_strlen(value) - ft_strlen(name)) + 1);
    i = replace_before_name(new_token, token);
    k = i;
    j = 0;
    while (value[j]) {
        if (value[j] == '$')
            value[j] = -1;
        new_token[i++] = value[j++];
    }
    k += ft_strlen(name);
    while (token[k])
        new_token[i++] = token[k++];
    free(token);
    return (new_token);
}

// this function search for variable name and replace it by its value recursively
char *parameter_expansion(char *token) {
    char *name;
    char *value;
    char *new_token;

    name = get_name(token);
    if (!name) {
        expands_dollars_dollars(token);
        return (token);
    }
    value = get_env_value(name + 1);
    new_token = replace_name_value(token, name, value);
    if (*(name + 1) == '?')
        free(value);
    free(name);
    return (parameter_expansion(new_token));
}

void free_tokens(t_token *tokens) {
    t_token *prev;

    while (tokens) {
        prev = tokens;
        if (prev->type != WORD && prev->type != LIMITER)
            free(tokens->token);
        tokens = tokens->next;
        free(prev);
    }
}

void free_all(t_token *tokens) {
    t_token *prev;

    while (tokens) {
        prev = tokens;
        free(tokens->token);
        tokens = tokens->next;
        free(prev);
    }
}

int is_expand(char **limiter) {
    if (contains(*limiter, '"') || contains(*limiter, '\'')) {
        *limiter = quotes_removal(*limiter);
        return (0);
    }
    return (1);
}

void change_flag(int s) {
    (void)s;
    g_global.heredoc_flag = dup(0);
    close(0);
}

int handle_heredoc_suite(t_cmd *cmd, char *limiter, char *file, int fd) {
    handle_signals();
    close(fd);
    free(limiter);
    fd = open(file, O_RDONLY, 0);
    if (fd == -1)
        return (0);
    cmd->infile = fd;
    free(file);
    return (1);
}

void check_heredoc(void) {
    if (g_global.heredoc_flag) {
        ft_dup2(g_global.heredoc_flag, 0);
        ft_close(g_global.heredoc_flag);
        g_global.exit_status = 1;
    }
}

int handle_heredoc(t_cmd *cmd, char *limiter, char *file) {
    int expand_mode;
    char *line;
    char *joined_line;
    int fd;

    expand_mode = is_expand(&limiter);
    sigint_heredoc();
    fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1)
        return (0);

    line = readline("> ");
    while (line && ft_strcmp(line, limiter)) {
        if (expand_mode && *line)
            line = heredoc_expansion(line);
        joined_line = ft_strjoin(line, "\n");
        write(fd, joined_line, ft_strlen(joined_line));
        free(line);
        free(joined_line);
        line = readline("> ");
        // line = readline_next(rl);
    }
    free(line);
    check_heredoc();
    return (handle_heredoc_suite(cmd, limiter, file, fd));
}

char *get_name_here(char *token) {
    int i;
    int name_len;

    i = 0;
    name_len = -1;
    while (token[i]) {
        name_len = -1;
        if (token[i] == '$')
            name_len = get_name_len(token, i);
        if (name_len == 0)
            token[i] = -1;
        if (name_len > 0)
            break;
        i++;
    }
    if (name_len == 0 || name_len == -1)
        return (NULL);
    return (ft_substr(token, i, i + name_len));
}

int replace_before_name_here(char *new_token, char *token) {
    int i;

    i = 0;
    while (token[i] != '$') {
        new_token[i] = token[i];
        i++;
    }
    return (i);
}

char *replace_name_value_here(char *token, char *name, char *value) {
    char *new_token;
    int i;
    int j;
    int k;

    i = 0;
    j = 0;
    new_token = ft_calloc(ft_strlen(token) + (ft_strlen(value) - ft_strlen(name)) + 1);
    i = replace_before_name_here(new_token, token);
    k = i;
    j = 0;
    while (value[j]) {
        if (value[j] == '$')
            value[j] = -1;
        new_token[i++] = value[j++];
    }
    k += ft_strlen(name);
    while (token[k])
        new_token[i++] = token[k++];
    free(token);
    return (new_token);
}

char *heredoc_expansion(char *line) {
    char *name;
    char *value;
    char *new_line;

    name = get_name_here(line);
    if (!name) {
        expands_dollars_dollars(line);
        return (line);
    }
    value = get_env_value(name + 1);
    new_line = replace_name_value_here(line, name, value);
    if (*(name + 1) == '?')
        free(value);
    free(name);
    return (heredoc_expansion(new_line));
}

// a function that return the position of the next quote char
int next_quote(int i, char quote, char *line) {
    while (line[i]) {
        if (line[i] == quote)
            return (i);
        i++;
    }
    return (-1);
}

// a function that returns the number of words splited by blank
// or -1 if the quotes are umatching
int word_count(char *line) {
    int count;
    int i;
    int flag;

    flag = 1;
    i = 0;
    count = 0;
    while (line[i]) {
        if (line[i] != ' ' && line[i] != '\t') {
            if (line[i] == '"' || line[i] == '\'')
                i = next_quote(i + 1, line[i], line);
            if (i == -1)
                return (-1);
            if (flag != 0)
                count++;
            flag = 0;
        } else
            flag = 1;
        i++;
    }
    return (count);
}

static char *get_next_word(char *s, int *index) {
    int i;
    int j;
    int k;
    char *word;

    k = 0;
    i = *index;
    while ((s[i] == ' ' || s[i] == '\t') && s[i])
        i++;
    j = i;
    while (!(s[i] == ' ' || s[i] == '\t') && s[i]) {
        if (s[i] == '"' || s[i] == '\'')
            i = next_quote(i + 1, s[i], s);
        i++;
    }
    word = ft_calloc(i - j + 1);
    while (j < i) {
        word[k] = s[j];
        k++;
        j++;
    }
    *index = i;
    return (word);
}

// a function that split a string by blank and returns the result in a 2d array
// or -1 if the quotes are umatching
char **split_by_blank(char *line) {
    char **res;
    int wc;
    int i;
    int j;

    wc = word_count(line);
    if (wc == -1)
        return (NULL);
    res = ft_calloc((wc + 1) * sizeof(char *));
    i = 0;
    j = 0;
    while (i < wc) {
        res[i] = get_next_word(line, &j);
        i++;
    }
    return (res);
}

// in first we split the words by blank characters and remove them , then we
// separate the words from the operators
// and create token ,then we parse the semantics of the tokens created
t_cmd *parse_line(char *line) {
    char **words;
    t_token *tokens;
    t_cmd *cmds;

    words = split_by_blank(line);
    if (words == NULL) {
        g_global.exit_status = 258;
        ft_putstr_fd("$> : syntax error\n", 2);
        return (NULL);
    }
    tokens = split_by_operator(words);
    free_split(words);
    if (!parse(tokens)) {
        g_global.exit_status = 258;
        free_all(tokens);
        ft_putstr_fd("$> : syntax error\n", 2);
        return (NULL);
    }
    tokenise_heredoc(tokens);
    tokens = expand_tokens(tokens);
    cmds = convert_to_cmds(tokens);
    free_tokens(tokens);
    return (cmds);
}
// pipe && rederiction

int is_operator(char c) {
    if (c == '&' || c == ';' || c == '(')
        return (1);
    if (c == ')' || c == '>' || c == '<')
        return (1);
    if (c == '|')
        return (1);
    return (0);
}

int position_of_operator(char *s) {
    int i;

    i = 0;
    while (s[i]) {
        if (s[i] == '"' || s[i] == '\'')
            i = next_quote(i + 1, s[i], s);
        if (is_operator(s[i]))
            return (i);
        i++;
    }
    return (-1);
}

// this function get the next word either string or sequence of operator
// we have three case :
// 1 - "simple word" => a word without operator we just duplicate it
// 2 - |||"simple world" => operators in the beguining so we group them
//     in a single word
// 3 - "simple world" | "s iadhf" => operator in the middle so we substr
//     until the operator
char *get_word(char *s, size_t *index) {
    size_t i;
    int pos;
    size_t len;

    i = *index;
    pos = position_of_operator(&s[i]);
    len = 0;
    if (pos == -1) {
        len = ft_strlen(&s[i]);
        *index += len;
        return (ft_substr(&s[i], 0, len));
    }
    if (pos == 0) {
        while (is_operator(s[i + len]))
            len++;
        *index += len;
        return (ft_substr(&s[i], 0, len));
    }
    *index += pos;
    return (ft_substr(&s[i], 0, pos));
}

// this function split the words by operator and create a linked
// list with the tokens
t_token *split_by_operator(char **words) {
    t_token *tokens;
    size_t i;
    size_t j;
    char *token;

    i = 0;
    tokens = NULL;
    while (words[i]) {
        j = 0;
        while (words[i][j]) {
            token = get_word(words[i], &j);
            add_back(&tokens, token);
        }
        i++;
    }
    return (tokens);
}

// this function checks if we have another type of operator except the ones
// handled by our minibash
int check_invalid_operator(t_token *tokens) {
    while (tokens) {
        if (tokens->type == OPERATOR)
            return (0);
        tokens = tokens->next;
    }
    return (1);
}

// this one checks if every redirection token is followed by a word
int check_redirections(t_token *tokens) {
    while (tokens) {
        if (tokens->type == REDIRECTION || tokens->type == HEREDOC) {
            if (tokens->next) {
                if ((tokens->next)->type != WORD)
                    return (0);
            } else
                return (0);
        }
        tokens = tokens->next;
    }
    return (1);
}

// this functions checks if there is a word after a pipe before
// the appearance of another pipe
// word | word => valid
// word | | word => invalid
int word_after(t_token *tokens) {
    tokens = tokens->next;
    while (tokens && tokens->type != PIPE) {
        if (tokens->type == WORD)
            return (1);
        tokens = tokens->next;
    }
    return (0);
}

// this function checks if every pipe is preceded by a word AND
// followed by a word
int check_pipes(t_token *tokens) {
    int word_before;

    word_before = 0;
    while (tokens) {
        if (tokens->type == WORD)
            word_before = 1;
        if (tokens->type == PIPE) {
            if (!word_before || !word_after(tokens))
                return (0);
            word_before = 0;
        }
        tokens = tokens->next;
    }
    return (1);
}

// this function returns 1 if the parsing is correct and -1 if incorrct
int parse(t_token *tokens) {
    if (!check_invalid_operator(tokens))
        return (0);
    if (!check_redirections(tokens))
        return (0);
    if (!check_pipes(tokens))
        return (0);
    return (1);
}

void open_pipes(t_token *tokens) {
    while (tokens) {
        if (tokens->type == PIPE)
            ft_pipe(tokens->pipe);
        tokens = tokens->next;
    }
}

t_token *next_pipe(t_token *tokens) {
    if (!tokens)
        return (NULL);
    if (tokens->type == PIPE)
        tokens = tokens->next;
    while (tokens && tokens->type != PIPE)
        tokens = tokens->next;
    return (tokens);
}

// in this function we remplace the -1 occurrences with $
// i need dollars dollars dollars is what i need
void expands_dollars_dollars(char *token) {
    while (*token) {
        if (*token == -1)
            *token = '$';
        token++;
    }
}

void free_token_word(t_token *token, char *word) {
    free(word);
    free(token);
}

// if pipe is already open
void check_and_redirect(int *inf_out, int fd) {
    if (*inf_out != 0)
        close(*inf_out);
    *inf_out = fd;
}

// in this function we chose the type of passed string
// the possible strings are sequences of operators or  normal chars
// if the sequence of operators is not to be handle by our minibash we name
// it by the token OPERATOR which should syntax error
// else the token are allowed
int choose_token_type(char *s) {
    size_t i;

    i = 0;
    if (!ft_strcmp(s, "|"))
        return (PIPE);
    if (!ft_strcmp(s, "<<"))
        return (HEREDOC);
    if (!ft_strcmp(s, ">>"))
        return (REDIRECTION);
    if (!ft_strcmp(s, "<") || !ft_strcmp(s, ">"))
        return (REDIRECTION);
    if (is_operator(s[i]))
        return (OPERATOR);
    else
        return (WORD);
}

t_token *create_node(char *s) {
    t_token *p;

    p = ft_calloc(sizeof(t_token));
    p->token = s;
    p->type = choose_token_type(s);
    return (p);
}

void add_back(t_token **head, char *s) {
    t_token *p;
    t_token *i;

    i = *head;
    p = create_node(s);
    if (i == NULL) {
        *head = p;
        return;
    }
    while (i->next)
        i = i->next;
    i->next = p;
}

void add_middle(t_token *token, char *word) {
    t_token *p;

    p = create_node(word);
    p->type = WORD;
    p->next = token->next;
    token->next = p;
}

// this function checks for heredoc and set the word next to it to a limiter
void tokenise_heredoc(t_token *token) {
    while (token) {
        if (token->type == HEREDOC)
            (token->next)->type = LIMITER;
        token = token->next;
    }
}

void init_values(int *flag, int *i, int *count, int *q) {
    *flag = 1;
    *i = 0;
    *count = 0;
    *q = -1;
}

int word_split_count(char *line) {
    int count;
    int i;
    int q;
    int flag;

    init_values(&flag, &i, &count, &flag);
    while (line[i]) {
        q = -1;
        if (line[i] != ' ' && line[i] != '\t') {
            if (line[i] == '"' || line[i] == '\'')
                q = next_quote(i + 1, line[i], line);
            if (q != -1)
                i = q;
            if (flag != 0)
                count++;
            flag = 0;
        } else
            flag = 1;
        i++;
    }
    return (count);
}

static char *get_next_word2(char *s, int *index) {
    int i;
    int j;
    int k;
    char *word;

    k = 0;
    i = *index;
    while ((s[i] == ' ' || s[i] == '\t') && s[i])
        i++;
    j = i;
    while (!(s[i] == ' ' || s[i] == '\t') && s[i])
        i++;
    word = ft_calloc(i - j + 1);
    while (j < i) {
        word[k] = s[j];
        k++;
        j++;
    }
    *index = i;
    return (word);
}

char **my_split(char *line, int wc) {
    char **res;
    int i;
    int j;

    res = ft_calloc((wc + 1) * sizeof(char *));
    i = 0;
    j = 0;
    while (i < wc) {
        res[i] = get_next_word2(line, &j);
        i++;
    }
    return (res);
}

// if wc == 0 that means that we need to remove this token ,
//  we will do it in a separate function
// if wc == 1 means that no splitting is required
// if wc > 1 means that the variable expansion caused a blank separator
// in the middle of the token
// so we need to split it into two words
t_token *word_spliting(t_token *token) {
    char **words;
    int wc;
    int i;

    wc = word_split_count(token->token);
    if (wc == 1 || wc == 0)
        return (token);
    words = my_split(token->token, wc);
    free(token->token);
    token->token = words[0];
    i = 1;
    while (words[i]) {
        add_middle(token, words[i]);
        token = token->next;
        i++;
    }
    free(words);
    return (token);
}

t_global g_global;
void init_global(char **envp);

int main(int argc, char **argv, char *envp[]) {
    (void)argc;
    (void)argv;
    // rl_catch_signals = 0;
    init_global(envp);
    shell_loop();
    return (0);
}

void init_global(char **envp) {
    char *tmp_shlvl;
    char *tmp_path;

    g_global.heredoc_flag = 0;
    g_global.exit_status = 0;
    g_global.envs = envs_init(envp);
    tmp_path = ft_getenv("PATH");
    if (!tmp_path)
        add_env(&g_global.envs, "PATH", "/bin/");
    tmp_shlvl = ft_getenv("SHLVL");
    if (!tmp_shlvl)
        add_env(&g_global.envs, "SHLVL", "1");
    else
        ft_setenv("SHLVL", ft_itoa(ft_atoi(tmp_shlvl) + 1));
}
