/*
Copied from: https://github.com/waltje/MiniMake/tree/main
Modified by Jonathan M. Wilbur to appear in a single file and compile with
nolibc.
License at the time of copying:

BSD 3-Clause License

Copyright 2023 Fred N. van Kempen.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	archive.c - archive support			Author: Kees J. Bot
 *								13 Nov 1993
 */
#ifndef NOLIBC
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef unix
# include <sys/wait.h>
# include <unistd.h>
# include <utime.h>
#endif
#ifdef _WIN32
# include <sys/utime.h>
# ifndef PATH_MAX
#  define PATH_MAX 1024
# endif
#endif

#else

struct utimbuf {
    time_t actime; /* Access time.  */
    time_t modtime; /* Modification time.  */
};

int utime(const char *filename, const struct utimbuf *times) {
    return my_syscall2(__NR_utime, filename, times);
}

FILE *fopen (const char *restrict pathname, const char *restrict mode) {
    int flags = 0;
    int fd = open(pathname, flags);
    // You don't need to handle an error. nolibc's implementation handles it.
    return fdopen(fd, mode); // Flags are ignored, but pass them in anyway.
}

char *strcat(char *dest, char *src)
{
    char *ret = dest;
    while (*dest)
        dest++;
    while (*dest++ = *src++)
        ;
    return ret;
}

int tolower (int c) {
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    else
        return c;
}

// I just didn't implement this entirely because I think it is actually
// unnecessary for this.
// int putenv (char *value) {
//     int i = 0;
//     while (environ[i++] != NULL)
//         ;
//     environ[i] = char;
//     environ[i+1] = (char* NULL);
//     return 404;
// }

#ifndef NO_SHELL
#define SHELL_PATH "/bin/sh"
#define SHELL_CMD "sh"
#define SHELL_ARG "-c"
#define ERR_SHELL_FAILED 127

int system (char *command) {
    int status;

    if (command == NULL) {
        // TODO: Return 0 if shell does not exist
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) { // We could not spawn the child process.
        errno = ECHILD;
        return -1;
    }
    if (pid == 0) { // If this process is the new child process...
        const char const* argv[4] = {
            SHELL_CMD,
            SHELL_ARG,
            command,
            (char *) NULL,
        };
        // Replace it with the command to execute.
        // This will never actually return 
        execve(SHELL_PATH, (char* const*)&argv, environ);
        return ERR_SHELL_FAILED; // This is a standard
    } else { // ... if this process is the parent process...
        waitpid(pid, &status, 0);
        return status;        
    }
}
#else
/* This is a shitty implementation of system() for systems that do not have
/bin/sh. */
int system (char *command) {
    int status;
    char *args[100];

    if (command == NULL)
        return -1;

    pid_t pid = fork();
    if (pid == -1) { // We could not spawn the child process.
        errno = ECHILD;
        return -1;
    }
    if (pid == 0) { // If this process is the new child process...

        // This is my very ugly way of splitting a string by whitespace.
        char **argpos = &args[0];
        int argstart = 0;
        for (int c = 0; c < strlen(command); c++) {
            if (c == ' ' || c == '\t' || c == '\n') {
                *argpos = strndup(&command[argstart], c - argstart);
                argpos += sizeof(char *);
                argstart = c + 1;
            }
        }
        *argpos = (char*) NULL;
        // Replace it with the command to execute.
        // This will never actually return 
        execve(command, &args, environ);
        return ERR_SHELL_FAILED; // This is a standard
    } else { // ... if this process is the parent process...
        waitpid(pid, &status, 0);
        return status;        
    }
}
#endif

#endif

#ifdef __GLIBC__

static char itoa_buffer[21];

// Intentionally non-reentrant, like nolibc's implementation, specifically so
// we do not have to free memory each time we stringify an integer.
char *itoa(long in)
{
    snprintf(itoa_buffer, sizeof(itoa_buffer), "%ld", in);
    return itoa_buffer;
}

#endif

#ifndef MAKE_H_H
# define MAKE_H_H

#define MNOENT ENOENT

#ifndef TRUE
# define TRUE   (1)
# define FALSE  (0)
#endif

#ifndef max
# define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef _WIN32
# define DEFN1   "Makefile"
# define DEFN2   "Makefile.MSVC"
#endif
#ifdef unix
# define DEFN1   "makefile"
# define DEFN2   "Makefile"
#endif

#define TABCHAR '\t'

#define LZ1	(2048)		/*  Initial input/expand string size  */
#define LZ2	(256)		/*  Initial input/expand string size  */

/* A name. This represents a file, either to be made, or existant. */
struct name {
    struct name	*n_next;		/* Next in the list of names */
    char	*n_name;		/* Called */
    struct line	*n_line;		/* Dependencies */
    time_t	n_time;			/* Modify time of this name */
    uint8_t	n_flag;			/* Info about the name */
};
#define N_MARK    0x01			/* For cycle check */
#define N_DONE    0x02			/* Name looked at */
#define N_TARG    0x04			/* Name is a target */
#define N_PREC    0x08			/* Target is precious */
#define N_DOUBLE  0x10			/* Double colon target */
#define N_EXISTS  0x20			/* File exists */
#define N_ERROR   0x40			/* Error occured */
#define N_EXEC    0x80			/* Commands executed */

/* Definition of a target line. */
struct line {
    struct line		*l_next;	/* Next line (for ::) */
    struct depend	*l_dep;		/* Dependents for this line */
    struct cmd		*l_cmd;		/* Commands for this line */
};

/* List of dependents for a line. */
struct depend {
    struct depend	*d_next;	/* Next dependent */
    struct name		*d_name;	/* Name of dependent */
};

/* Commands for a line. */
struct cmd {
    struct cmd		*c_next;	/* Next command line */
    char		*c_cmd;		/* Command line */
};

/* Macro storage. */
struct macro {
    struct macro	*m_next;	/* Next variable */
    char		*m_name;	/* Called ... */
    char		*m_val;		/* Its value */
    uint8_t		m_flag;		/* Infinite loop check */
};
#define M_MARK		0x01		/* for infinite loop check */
#define M_OVERRIDE	0x02		/* command-line override */
#define M_MAKE		0x04		/* for MAKE macro */

/* String. */
struct str {
    char		**ptr;		/* ptr to real ptr. to string */
    int			len;		/* length of string */
    int			pos;		/* position */
};

struct stat;

/* Declaration, definition & initialization of variables */
#ifndef EXTERN
# define EXTERN
#endif
#ifndef INIT
# define INIT(x)
#endif
EXTERN char *myname;
EXTERN int8_t  domake   INIT(TRUE);  /*  Go through the motions option  */
EXTERN int8_t  ignore   INIT(FALSE); /*  Ignore exit status option      */
EXTERN int8_t  conterr  INIT(FALSE); /*  continue on errors  */
EXTERN int8_t  silent   INIT(FALSE); /*  Silent option  */
EXTERN int8_t  print    INIT(FALSE); /*  Print debuging information  */
EXTERN int8_t  rules    INIT(TRUE);  /*  Use inbuilt rules  */
EXTERN int8_t  dotouch  INIT(FALSE); /*  Touch files instead of making  */
EXTERN int8_t  quest    INIT(FALSE); /*  Question up-to-dateness of file  */
EXTERN int8_t  useenv   INIT(FALSE); /*  Env. macro def. overwrite makefile def.*/
EXTERN int8_t  dbginfo  INIT(FALSE); /*  Print lot of debugging information */
EXTERN int8_t  ambigmac INIT(TRUE);  /*  guess undef. ambiguous macros (*,<) */
EXTERN struct name  *firstname;
EXTERN char         *str1;
EXTERN char         *str2;
EXTERN struct str    str1s;
EXTERN struct str    str2s;
EXTERN struct name **suffparray; /* ptr. to array of ptrs. to name chains */
EXTERN int           sizesuffarray INIT(20); /* size of suffarray */
EXTERN int           maxsuffarray INIT(0);   /* last used entry in suffarray */
EXTERN struct macro *macrohead;
EXTERN int8_t          expmake; /* TRUE if $(MAKE) has been expanded */
EXTERN char	    *makefile;     /*  The make file  */
EXTERN int           lineno;

#define  suffix(name)   strrchr(name,(int)'.')


/* check.c */
extern void prt(void);
extern void check(struct name *np);
extern void circh(void);
extern void precious(void);

/* input.c */
extern void init(void);
extern void strrealloc(struct str *strs);
extern struct name *newname(const char *name);
extern struct name *testname(const char *name);
extern struct depend *newdep(struct name *np, struct depend *dp);
extern struct cmd *newcmd(const char *str, struct cmd *cp);
extern void newline(struct name *np, struct depend *dp, struct cmd *cp, int flag);
extern void input(FILE *fd);

/* macro.c */
extern char *getmacro(const char *name);
extern struct macro *setmacro(const char *name, const char *val);
extern void setDFmacro(const char *name, const char *val);
extern void expand(struct str *strs);

/* main.c */
extern void fatal(const char *msg, const char *a1, int a2);

/* make.c */
extern void modtime(struct name *np);
extern int make(struct name *np, int level);

/* reader.c */
extern void error(const char *msg, const char *a1);
extern int8_t getaline(struct str *strs, FILE *fd);
extern char *gettok(char **ptr);

/* rules.c */
extern int8_t dyndep(struct name *np, char **pbasename, char **pinputname);
extern void makerules(void);

/* archive.c */
extern int is_archive_ref(const char *name);
extern int archive_stat(const char *name, struct stat *stp);


#endif	/*MAKE_H_H*/


#define arraysize(a)	(sizeof(a) / sizeof((a)[0]))
#define arraylimit(a)	((a) + arraysize(a))

/* ASCII ar header. */
#define ASCII_ARMAG	"!<arch>\n"
#define ASCII_SARMAG	8
#define ASCII_ARFMAG	"`\n"

struct ascii_ar_hdr {
    char	ar_name[16];
    char	ar_date[12];
    char	ar_uid[6];
    char	ar_gid[6];
    char	ar_mode[8];
    char	ar_size[10];
    char	ar_fmag[2];
};

typedef struct archname {
    struct archname	*next;		/* Next on the hash chain. */
    char		name[16];	/* One archive entry. */
    time_t		date;		/* The timestamp. */
    /* (no need for other attibutes) */
} archname_t;


static size_t namelen;			/* Max name length, 14 or 16. */
static char *lpar, *rpar;		/* Leave these at '(' and ')'. */

#define HASHSIZE	(64 << sizeof(int))
static archname_t *nametab[HASHSIZE];


/* Compute a hash value out of a name. */
static int
hash(const char *name)
{
    const uint8_t *p = (const uint8_t *)name;
    unsigned h = 0;
    int n = namelen;

    while (*p != 0) {
	h = h * 0x1111 + *p++;
	if (--n == 0)
		break;
    }

    return h % arraysize(nametab);
}


/* Enter a name to the table, or return the date of one already there. */
static int
searchtab(const char *name, time_t *date, int scan)
{
    archname_t **pnp, *np;
    int cmp = 1;

    pnp = &nametab[hash(name)];

    while ((np = *pnp) != NULL
		&& (cmp = strncmp(name, np->name, namelen)) > 0) {
	pnp= &np->next;
    }

    if (cmp != 0) {
	if (scan) {
		errno = ENOENT;
		return -1;
	}
	if ((np = (archname_t *)malloc(sizeof(*np))) == NULL)
		fatal("No memory for archive name cache",NULL,0);
	strncpy(np->name, name, namelen);
	np->date = *date;
	np->next = *pnp;
	*pnp = np;
    }

    if (scan)
	*date = np->date;

    return 0;
}


/* Delete the name cache, a different library is to be read. */
static void
deltab(void)
{
    archname_t **pnp, *np, *junk;

    for (pnp = nametab; pnp < arraylimit(nametab); pnp++) {
	for (np = *pnp; np != NULL; ) {
		junk = np;
		np = np->next;
		free(junk);
	}

	*pnp = NULL;
    }
}


/* Transform a string into a number.  Ignore the space padding. */
static long
ar_atol(const char *s, size_t n)
{
    long l = 0;

    while (n > 0) {
	if (*s != ' ')
		l= l * 10 + (*s - '0');
	s++;
	n--;
    }

    return l;
}


/* Read a modern ASCII type archive. */
static int
read_ascii_archive(int afd)
{
    struct ascii_ar_hdr hdr;
    off_t pos = 8;
    char *p;
    time_t date;

    namelen = 16;

    for (;;) {
	if (lseek(afd, pos, SEEK_SET) == -1)
		return -1;

	switch (read(afd, &hdr, sizeof(hdr))) {
		case sizeof(hdr):
			break;

		case -1:
			return -1;

		default:
			return 0;
	}

	if (strncmp(hdr.ar_fmag, ASCII_ARFMAG, sizeof(hdr.ar_fmag)) != 0) {
		errno = EINVAL;
		return -1;
	}

	/* Strings are space padded! */
	for (p = hdr.ar_name; p < hdr.ar_name + sizeof(hdr.ar_name); p++) {
		if (*p == ' ') {
			*p = 0;
			break;
		}
	}

	/* Add a file to the cache. */
	date = ar_atol(hdr.ar_date, sizeof(hdr.ar_date));
	searchtab(hdr.ar_name, &date, 0);

	pos += sizeof(hdr) + ar_atol(hdr.ar_size, sizeof(hdr.ar_size));
	pos = (pos + 1) & (~ (off_t) 1);
    }
}


/* True if name is of the form "archive(file)". */
int
is_archive_ref(const char *name)
{
    const char *p = name;

    while (*p != 0 && *p != '(' && *p != ')')
	p++;
    lpar = (char *)p;
    if (*p++ != '(')
	return 0;

    while (*p != 0 && *p != '(' && *p != ')')
	p++;
    rpar = (char *)p;
    if (*p++ != ')')
	return 0;

    return *p == 0;
}


/* Search an archive for a file and return that file's stat info. */
int
archive_stat(const char *name, struct stat *stp)
{
    static dev_t ardev;
    static ino_t arino = 0;
    static time_t armtime;
    char magic[8];
    char *file;
    int afd;
    int r = -1;

    if (! is_archive_ref(name)) {
	errno = EINVAL;
	return -1;
    }
    *lpar = 0;
    *rpar = 0;
    file = lpar + 1;

    if (stat(name, stp) < 0)
	goto bail_out;

    if (stp->st_ino != arino || stp->st_dev != ardev) {
	/*
	 * Either the first (and probably only) library, or a different
	 * library.
	 */
	arino = stp->st_ino;
	ardev = stp->st_dev;
	armtime = stp->st_mtime;
	deltab();

	if ((afd = open(name, O_RDONLY)) < 0)
		goto bail_out;

	switch (read(afd, magic, sizeof(magic))) {
		case 8:
			if (strncmp(magic, ASCII_ARMAG, 8) == 0) {
				r = read_ascii_archive(afd);
				break;
			}
			/*FALLTHROUGH*/

		default:
			errno = EINVAL;
			/*FALLTHROUGH*/

		case -1:
			/* r= -1 */;
	}
	{ int e = errno; close(afd); errno = e; }
    } else {
	/* Library is cached. */
	r = 0;
    }

    if (r == 0) {
	/* Search the cache. */
	r = searchtab(file, &stp->st_mtime, 1);
	if (stp->st_mtime > armtime)
		stp->st_mtime = armtime;
    }

bail_out:
    /* Repair the name(file) thing. */
    *lpar = '(';
    *rpar = ')';
    return r;
}

/*
 * Prints out the structures as defined in memory.  Good for check
 * that you make file does what you want (and for debugging make).
 */
void
prt(void)
{
    struct name *np;
    struct depend *dp;
    struct line *lp;
    struct cmd *cp;
    struct macro *mp;
    int i;

    for (mp = macrohead; mp; mp = mp->m_next)
	printf("%s = %s\n", mp->m_name, mp->m_val);

    putchar('\n');

    for (i = 0; i <= maxsuffarray ; i++)
	for (np = suffparray[i]->n_next; np; np = np->n_next) {
		if (np->n_flag & N_DOUBLE)
			printf("%s::\n", np->n_name);
		else
			printf("%s:\n", np->n_name);
		if (np == firstname)
			printf("(MAIN NAME)\n");
		for (lp = np->n_line; lp; lp = lp->l_next) {
			putchar(':');
			for (dp = lp->l_dep; dp; dp = dp->d_next)
				printf(" %s", dp->d_name->n_name);
			putchar('\n');

			for (cp = lp->l_cmd; cp; cp = cp->c_next)
				printf("-\t%s\n", cp->c_cmd);
			putchar('\n');
		}
		putchar('\n');
	}
}


/* Recursive routine that does the actual checking. */
void
check(struct name *np)
{
    struct depend *dp;
    struct line *lp;

    if (np->n_flag & N_MARK)
	fatal("Circular dependency from %s", np->n_name,0);

    np->n_flag |= N_MARK;

    for (lp = np->n_line; lp; lp = lp->l_next)
	for (dp = lp->l_dep; dp; dp = dp->d_next)
		check(dp->d_name);

    np->n_flag &= ~N_MARK;
}


/*
 * Look for circular dependancies.
 * ie.
 *	a: b
 *	b: a
 *  is a circular dep
 */
void
circh(void)
{
    struct name *np;
    int i;

    for (i = 0; i <= maxsuffarray ; i++)
	for (np = suffparray[i]->n_next; np; np = np->n_next)
		check(np);
}



/* Check the target .PRECIOUS, and mark its dependents as precious. */
void
precious(void)
{
    struct depend *dp;
    struct line *lp;
    struct name *np;

    if (! ((np = newname(".PRECIOUS"))->n_flag & N_TARG))
	return;

    for (lp = np->n_line; lp; lp = lp->l_next)
	for (dp = lp->l_dep; dp; dp = dp->d_next)
		dp->d_name->n_flag |= N_PREC;
}



static struct name *lastrrp;
static struct name *freerp = (struct name *)NULL;


void
init(void)
{
    if ((suffparray = (struct name **)malloc(sizesuffarray *
           sizeof(struct name *)))  == (struct name **)NULL)
	fatal("No memory for suffarray", NULL, 0);

    if ((*suffparray = (struct name *)malloc(sizeof(struct name))) == NULL)
	fatal("No memory for name", NULL, 0);

    (*suffparray)->n_next = NULL;

    if ((str1 = (char *)malloc(LZ1)) == NULL)
	fatal("No memory for str1", NULL, 0);

    str1s.ptr = &str1;
    str1s.len = LZ1;
    if ((str2 = (char *)malloc(LZ2)) == NULL)
	fatal("No memory for str2", NULL, 0);
    str2s.ptr = &str2;
    str2s.len = LZ2;
}


void
strrealloc(struct str *strs)
{
    strs->len *= 2;
    *strs->ptr = (char *)realloc(*strs->ptr, strs->len + 16);

    if (*strs->ptr == NULL)
	fatal("No memory for string reallocation", NULL, 0);
}


/* Intern a name.  Return a pointer to the name struct. */
struct name *
newname(const char *name)
{
    struct name *rp;
    struct name *rrp;
    struct name **sp;		/* ptr. to ptr. to chain of names */
    char *cp;
    char *suff;			/* ptr. to suffix in current name */
    int i;

    if ((suff = suffix(name)) != NULL) {
	for (i = 1, sp = suffparray, sp++;
	     i <= maxsuffarray && strcmp(suff, (*sp)->n_name) != 0;
	     sp++, i++);
	if (i > maxsuffarray) {
		if ( i >= sizesuffarray) { /* must realloc suffarray */
			sizesuffarray *= 2;
			if ((suffparray = (struct name **)realloc((char *)suffparray,
				sizesuffarray * sizeof(struct name *))) == NULL)
				fatal("No memory for suffarray", NULL, 0);
		}
        	maxsuffarray++;
        	sp = &suffparray[i];
        	if ((*sp = (struct name *)malloc(sizeof (struct name))) == NULL)
			fatal("No memory for name", NULL, 0);
        	(*sp)->n_next = (struct name *)0;
        	if ((cp = (char *) malloc(strlen(suff)+1)) == NULL)
			fatal("No memory for name", NULL, 0);
		strcpy(cp, suff);
		(*sp)->n_name = cp;
	}
    } else
	sp = suffparray;

    for (rp = (*sp)->n_next, rrp = *sp; rp; rp = rp->n_next, rrp = rrp->n_next)
	if (strcmp(name, rp->n_name) == 0)
		return rp;

    if (freerp == (struct name *)NULL) {
	if ((rp = (struct name *)malloc(sizeof (struct name))) == NULL)
		fatal("No memory for name", NULL, 0);
    } else {
	rp = freerp;
	freerp = NULL;
    }

    rrp->n_next = rp;
    rp->n_next = NULL;
    if ((cp = (char *)malloc(strlen(name)+1)) == NULL)
	fatal("No memory for name", NULL, 0);
    strcpy(cp, name);
    rp->n_name = cp;
    rp->n_line = NULL;
    rp->n_time = (time_t)0;
    rp->n_flag = 0;
    lastrrp = rrp;

    return rp;
}


/*
 * Test a name.
 * If the name already exists return the ptr. to its name structure.
 * Else if the file exists 'intern' the name and return the ptr.
 * Otherwise don't waste memory and return a NULL pointer
 */
struct name *
testname(const char *name)
{
    struct name *rp;

    lastrrp = NULL;
    rp = newname(name);
    if (rp->n_line || rp->n_flag & N_EXISTS)
	return(rp);
    modtime(rp);
    if (rp->n_flag & N_EXISTS)
	return(rp);
    if (lastrrp != NULL) {
	free(rp->n_name);
	lastrrp->n_next = NULL;
	freerp = rp;
    }

    return(NULL);
}


/*
 * Add a dependant to the end of the supplied list of dependants.
 * Return the new head pointer for that list.
 */
struct depend *
newdep(struct name *np, struct depend *dp)
{
    struct depend *rp;
    struct depend *rrp;

    if ((rp = (struct depend *)malloc(sizeof (struct depend))) == NULL)
	fatal("No memory for dependant", NULL, 0);
    rp->d_next = NULL;
    rp->d_name = np;

    if (dp == NULL)
	return rp;

    for (rrp = dp; rrp->d_next; rrp = rrp->d_next)
			;
    rrp->d_next = rp;

    return dp;
}


/*
 * Add a command to the end of the supplied list of commands.
 * Return the new head pointer for that list.
 */
struct cmd *
newcmd(const char *str, struct cmd *cp)
{
    struct cmd *rp;
    struct cmd *rrp;
    char *rcp;

    if (rcp = strrchr(str, '\n'))
	*rcp = '\0';	/* lose newline */

    while (isspace(*str))
	str++;

    if (*str == '\0')
	return cp;		/*  If nothing left, the exit */

    if ((rp = (struct cmd *)malloc(sizeof(struct cmd))) == NULL)
	fatal("No memory for command", NULL, 0);
    rp->c_next = (struct cmd *)0;
    if ((rcp = (char *)malloc(strlen(str)+1)) == NULL)
	fatal("No memory for command", NULL, 0);
    strcpy(rcp, str);
    rp->c_cmd = rcp;

    if (cp == NULL)
	return rp;

    for (rrp = cp; rrp->c_next; rrp = rrp->c_next)
			    ;
    rrp->c_next = rp;

    return cp;
}


/*
 * Add a new 'line' of stuff to a target.  This check to see
 * if commands already exist for the target.  If flag is set,
 * the line is a double colon target.
 *
 * Kludges:
 *  i)  If the new name begins with a '.', and there are no dependents,
 *      then the target must cease to be a target.  This is for .SUFFIXES.
 * ii)  If the new name begins with a '.', with no dependents and has
 *      commands, then replace the current commands.  This is for
 *      redefining commands for a default rule.
 *
 * Neither of these free the space used by dependents or commands,
 * since they could be used by another target.
 */
void
newline(struct name *np, struct depend *dp, struct cmd *cp, int flag)
{
    int8_t hascmds = FALSE;  /*  Target has commands  */
    struct line *rp, *rrp;

    /* Handle the .SUFFIXES case */
    if (np->n_name[0] == '.' && !dp && !cp) {
	for (rp = np->n_line; rp; rp = rrp) {
		rrp = rp->l_next;
		free(rp);
	}
	np->n_line = NULL;
	np->n_flag &= ~N_TARG;

	return;
    }

    /* This loop must happen since rrp is used later. */
    for (rp = np->n_line, rrp = NULL; rp; rrp = rp, rp = rp->l_next)
	if (rp->l_cmd)
		hascmds = TRUE;

    if (hascmds && cp && !(np->n_flag & N_DOUBLE))
	/* Handle the implicit rules redefinition case */
	if (np->n_name[0] == '.' && dp == NULL) {
		np->n_line->l_cmd = cp;
		return;
	} else
		error("Commands defined twice for target %s", np->n_name);

    if (np->n_flag & N_TARG)
	if (! (np->n_flag & N_DOUBLE) != !flag)		/* like xor */
		error("Inconsistent rules for target %s", np->n_name);

    if ((rp = (struct line *)malloc(sizeof(struct line))) == NULL)
	fatal("No memory for line", NULL, 0);
    rp->l_next = NULL;
    rp->l_dep = dp;
    rp->l_cmd = cp;

    if (rrp)
         rrp->l_next = rp;
    else
         np->n_line = rp;

    np->n_flag |= N_TARG;

    if (flag)
	np->n_flag |= N_DOUBLE;
}


/* Parse input from the makefile, and construct a tree structure of it. */
void
input(FILE *fd)
{
    struct depend *dp;
    struct name *np;
    struct cmd *cp;
    char *p, *q, *a;
    int8_t dbl;

    if (getaline(&str1s, fd))
	return;		/* Read the first line */

    for (;;) {
	if (*str1 == TABCHAR)	/* Rules without targets */
		error("Rules not allowed here", NULL);

	p = str1;

	while (isspace(*p))
		p++;	/*  Find first target  */

	while (((q = strchr(p, '=')) != (char *)0) && (p != q) && (q[-1] == '\\')) {	/*  Find value */
		a = q - 1;	/*  Del \ chr; move rest back  */
		p = q;
		while (*a++ = *q++)
			;
	}

	if (q != (char *)0) {
		*q++ = '\0';		/*  Separate name and val  */
		while (isspace(*q))
			q++;
		if (p = strrchr(q, '\n'))
			*p = '\0';

		p = str1;
		if ((a = gettok(&p)) == NULL)
			error("No macro name", NULL);

		setmacro(a, q);

		if (getaline(&str1s, fd))
			return;

		continue;
	}

	/* include? */
	p = str1;
	while (isspace(*p))
		p++;

	//FIXME: this is a hack..
	if (strncmp(p, "include", 7) == 0 && isspace(p[7])) {
		char *old_makefile = makefile;
		int old_lineno = lineno;
		FILE *ifd;

		p += 8;
		memmove(str1, p, strlen(p)+1);
		expand(&str1s);
		p = str1;
		while (isspace(*p))
			p++;

		if ((q = malloc(strlen(p)+1)) == NULL)
			fatal("No memory for include", NULL, 0);

		strcpy(q, p);
		p = q;
		while ((makefile = gettok(&q)) != NULL) {
			if ((ifd = fopen(makefile, "r")) == NULL)
				fatal("Can't open %s: %s", makefile, errno);
			lineno = 0;
			input(ifd);
			fclose(ifd);
		}
		free(p);
		makefile = old_makefile;
		lineno = old_lineno;

		if (getaline(&str1s, fd))
			return;

		continue;
	}

	/* Search for commands on target line --- do not expand them ! */
	q = str1;
	cp = (struct cmd *)0;
	if ((a = strchr(q, ';')) != NULL) {
		*a++ = '\0';	/*  Separate dependents and commands */
		if (a)
			cp = newcmd(a, cp);
	}

	expand(&str1s);
	p = str1;

	while (isspace(*p))
		p++;

	while (((q = strchr(p, ':')) != (char *)0) && (p != q) && (q[-1] == '\\')) {	/*  Find dependents  */
		a = q - 1;	/*  Del \ chr; move rest back  */
		p = q;
		while (*a++ = *q++)
			;
	}

	if (q == NULL)
		error("No targets provided", NULL);

	*q++ = '\0';	/*  Separate targets and dependents  */

	if (*q == ':') {		/* Double colon */
		dbl = 1;
		q++;
	} else
		dbl = 0;

	for (dp = NULL; (p = gettok(&q)) != NULL; ) {
		/*  get list of dep's */
		np = newname(p);		/*  Intern name  */
		dp = newdep(np, dp);		/*  Add to dep list */
	}

	*((q = str1) + strlen(str1) + 1) = '\0';
	/*  Need two nulls for gettok (Remember separation)  */

	if (getaline(&str2s, fd) == FALSE) {		/*  Get commands  */
		while (*str2 == TABCHAR) {
			cp = newcmd(&str2[0], cp);
			if (getaline(&str2s, fd))
				break;
		}
	}

	while ((p = gettok(&q)) != (char *)0) {	/* Get list of targ's */
		np = newname(p);		/*  Intern name  */
		newline(np, dp, cp, dbl);
		if (!firstname && p[0] != '.')
			firstname = np;
	}

    char eof_buf[1];
	if (read(fileno(fd), &eof_buf, 1) == EOF)
		return;

	while (strlen(str2) >= str1s.len)
		strrealloc(&str1s);
	strcpy(str1, str2);
    }
}



static char   buf[256];


static struct macro *
getmp(const char *name)
{
    struct macro *rp;

    for (rp = macrohead; rp; rp = rp->m_next)
		if (strcmp(name, rp->m_name) == 0)
			return rp;

    return NULL;
}


char *
getmacro(const char *name)
{
    struct macro *mp;

    if (mp = getmp(name))
	return mp->m_val;

    return "";
}


struct macro *
setmacro(const char *name, const char *val)
{
    struct macro *rp;
    char *cp;

    /* Replace macro definition if it exists. */
    for (rp = macrohead; rp; rp = rp->m_next)
	if (strcmp(name, rp->m_name) == 0) {
		if (rp->m_flag & M_OVERRIDE)
			return rp;	/* mustn't change */

		free(rp->m_val);	/*  Free space from old  */
		break;
	}

    if (! rp) {		/*  If not defined, allocate space for new  */
	if ((rp = (struct macro *)malloc(sizeof (struct macro))) == NULL)
		fatal("No memory for macro", NULL, 0);

	rp->m_next = macrohead;
	macrohead = rp;
	rp->m_flag = FALSE;

	if ((cp = (char *)malloc(strlen(name)+1)) == NULL)
		fatal("No memory for macro", NULL, 0);
	strcpy(cp, name);
	rp->m_name = cp;
    }

    if ((cp = (char *)malloc(strlen(val)+1)) == NULL)
	fatal("No memory for macro", NULL, 0);
    strcpy(cp, val);		/*  Copy in new value  */
    rp->m_val = cp;

    return rp;
}


void
setDFmacro(const char *name, const char *val)
{
    static char filename[] = "@F";
    static char dirname[] = "@D";
    char *c, *tmp;
    int len;

    setmacro(name, val);
    *filename = *name;
    *dirname = *name;

    /* Null string -- not defined macro */
    if (! (*val)) {
	setmacro(filename, "");
	setmacro(dirname, "");
	return;
    }
    if (! (c = strrchr(val, (int)'/'))) {
	setmacro(filename, val);
	setmacro(dirname, "./");
	return;
    }
    setmacro(filename,c+1);

    len = c - val + 1;
    if ((tmp = (char *)malloc(len + 1)) == NULL)
	fatal("No memory for tmp", NULL, 0);
    strncpy(tmp, val, len);
    tmp[len] = '\0';
    setmacro(dirname, tmp);

    free(tmp);

    return;
}


/* Do the dirty work for expand. */
static void
doexp(struct str *to, const char *from)
{
    struct macro *mp;
    const char *rp;
    char *p, *q;

    rp = from;
    p  = &(*to->ptr)[to->pos];
    while (*rp) {
	if (*rp != '$') {
		*p++ = *rp++;
		to->pos++;
	} else {
		q = buf;
		if (*++rp == '{')
			while (*++rp && *rp != '}')
				*q++ = *rp;
		else if (*rp == '(')
			while (*++rp && *rp != ')')
				*q++ = *rp;
		else if (!*rp) {
			*p++ = '$';
			to->pos++;
			goto bail;
		} else
			*q++ = *rp;

		*q = '\0';
		if (*rp)
			rp++;
		if (!(mp = getmp(buf)))
			mp = setmacro(buf, "");
		if (mp->m_flag & M_MARK)
			fatal("Infinitely recursive macro %s", mp->m_name,0);
		mp->m_flag |= M_MARK;

		if (mp->m_flag & M_MAKE)
			expmake = TRUE;
		doexp(to, mp->m_val);
		p = &(*to->ptr)[to->pos];

		mp->m_flag &= ~M_MARK;
	}

bail:
	if (to->pos >= to->len) {
		strrealloc(to);
		p = &(*to->ptr)[to->pos];
	}
    }

    *p = '\0';
}


/* Expand any macros in str. */
void
expand(struct str *strs)
{
    char *a;

    if ((a = (char *)malloc(strlen(*strs->ptr)+1)) == NULL)
	fatal("No memory for temporary string", NULL, 0);
    strcpy(a, *strs->ptr);
    strs->pos = 0;
    doexp(strs, a);

    free(a);
}



static int8_t  execflag;


static void
dbgprint(int level, struct name *np, const char *comment)
{
    char *timep;

    if (np) {
#ifdef __GLIBC__
        timep = ctime(&np->n_time);
        timep[24] = '\0';
        fputs(&timep[4], stdout);
#else
        // Keeping padding the same: the vast majority of epoch timestamps
        // will be ten-digits in length.
        // By the way, nolibc does not have the padding operator.
        printf("%.24ld", time(NULL));
#endif
    } else
        fputs("                    ", stdout);

    fputs("   ", stdout);

    while (level--)
	    fputs("  ", stdout);

    if (np) {
        fputs(np->n_name, stdout);
        if (np->n_flag & N_DOUBLE)
            fputs("  :: ", stdout);
        else
            fputs("  : ", stdout);
    }

    fputs(comment, stdout);
    putchar((int)'\n');
    fflush(stdout);
}


static void
tellstatus(FILE *out, const char *name, int status)
{
//FIXME: create Windows code here
#if 0
  char cwd[PATH_MAX];

  fprintf(out, "%s in %s: ",
	name, getcwd(cwd, sizeof(cwd)) == NULL ? "?" : cwd);

  if (WIFEXITED(status)) {
	fprintf(out, "Exit code %d", WEXITSTATUS(status));
  } else {
	fprintf(out, "Signal %d%s",
		WTERMSIG(status), status & 0x80 ? " - core dumped" : "");
  }
#endif
}


/* Exec a shell that returns exit status correctly (/bin/esh). */
int
dosh(char *string, const char *shell)
{
//FIXME: should we not use the 'shell' parameter here?
    return system(string);
}


#ifdef unix
/*
 * Make a file look very outdated after an error trying to make it.
 * Don't remove, this keeps hard links intact.  (kjb)
 */
int makeold (const char *name) {
    struct utimbuf a;
    a.actime = a.modtime = 0;	/* The epoch */
    return utime(name, &a);
}
#endif


/* Do commands to make a target. */
static void
docmds1(struct name *np, struct line *lp)
{
    struct cmd *cp;
    char *shell;
    char *p, *q;
    int8_t ssilent;
    int8_t signore;
    int estat;

    if (*(shell = getmacro("SHELL")) == '\0')
#ifdef _WIN32
	shell = "cmd.exe /c";
#endif
#ifdef unix
	shell = "/bin/sh";
#endif

    for (cp = lp->l_cmd; cp; cp = cp->c_next) {
	execflag = TRUE;
	strcpy(str1, cp->c_cmd);
	expmake = FALSE;
	expand(&str1s);
	q = str1;
	ssilent = silent;
	signore = ignore;
	while ((*q == '@') || (*q == '-')) {
		if (*q == '@')	   /*  Specific silent  */
			ssilent = TRUE;
		else		   /*  Specific ignore  */
			signore = TRUE;
		if (! domake)
			putchar(*q);  /* Show all characters. */
		q++;		   /*  Not part of the command  */
	}

	for (p = q; *p; p++) {
		if (*p == '\n' && p[1] != '\0') {
			*p = ' ';
			if (!ssilent || !domake)
				fputs("\\\n", stdout);
		} else if (!ssilent || !domake)
			putchar(*p);
	}
	if (!ssilent || !domake)
		putchar('\n');

	if (domake || expmake) {	/*  Get the shell to execute it  */
		fflush(stdout);
		if ((estat = dosh(q, shell)) != 0) {
			if (estat == -1)
				fatal("Couldn't execute %s", shell, 0);
			else if (signore) {
				tellstatus(stdout, myname, estat);
				printf(" (Ignored)\n");
			} else {
				tellstatus(stderr, myname, estat);
				fprintf(stderr, "\n");
				if (! (np->n_flag & N_PREC))
#ifdef unix
					if (makeold(np->n_name) == 0)
						fprintf(stderr, "%s: made '%s' look old.\n", myname, np->n_name);
#else
			    		if (unlink(np->n_name) == 0)
						fprintf(stderr, "%s: '%s' removed.\n", myname, np->n_name);
#endif
				if (! conterr)
					exit(estat != 0);
				np->n_flag |= N_ERROR;
				return;
			}
		}
	}
    }
}


void
docmds(struct name *np)
{
    struct line *lp;

    for (lp = np->n_line; lp; lp = lp->l_next)
	docmds1(np, lp);
}


/*
 *	Get the modification time of a file.  If the first
 *	doesn't exist, it's modtime is set to 0.
 */
void
modtime(struct name *np)
{
    struct stat info;
    int r;

    if (is_archive_ref(np->n_name))
	r = archive_stat(np->n_name, &info);
    else
	r = stat(np->n_name, &info);

    if (r < 0) {
	if (errno != ENOENT)
		fatal("Can't open %s: %s", np->n_name, errno);

	np->n_time = (time_t)0;
	np->n_flag &= ~N_EXISTS;
    } else {
	np->n_time = info.st_mtime;
	np->n_flag |= N_EXISTS;
    }
}


/* Update the mod time of a file to now. */
void
touch(struct name *np)
{
    struct utimbuf a;
    char c;
    int fd;

    if (!domake || !silent)
	printf("touch(%s)\n", np->n_name);

    if (domake) {
	a.actime = a.modtime = time(NULL);
	if (utime(np->n_name, &a) < 0)
		printf("%s: '%s' not touched - non-existant\n",
					myname, np->n_name);
    }
}


static void
implmacros(struct name *np, struct line *lp, char **pbasename, char **pinputname)
{
    struct line *llp;
    char *p, *q;
    char *suff;				/*  Old suffix  */
    struct depend *dp;
    int baselen;
    int8_t dpflag = FALSE;

    /* get basename out of target name */
    p = str2;
    q = np->n_name;
    suff = suffix(q);
    while (*q && (q < suff || !suff))
	*p++ = *q++;
    *p = '\0';
    if ((*pbasename = (char *)malloc(strlen(str2)+1)) == NULL)
	fatal("No memory for basename", NULL, 0);
    strcpy(*pbasename, str2);
    baselen = strlen(str2);

    if ( lp)
	llp = lp;
    else
	llp = np->n_line;

    while (llp) {
	for (dp = llp->l_dep; dp; dp = dp->d_next) {
		if (strncmp(*pbasename, dp->d_name->n_name, baselen) == 0) {
			*pinputname = dp->d_name->n_name;
			return;
		}

		if (! dpflag) {
			*pinputname = dp->d_name->n_name;
			dpflag = TRUE;
		}
	}

	if (lp)
		break;
	llp = llp->l_next;
    }

#if NO_WE_DO_WANT_THIS_BASENAME
    free(*pbasename);  /* basename ambiguous or no dependency file */
    *pbasename = NULL;
#endif
}


static void
make1(struct name *np, struct line *lp, struct depend *qdp, char *basename, char *inputname)
{
    struct depend *dp;

    if (dotouch)
	touch(np);
    else if (! (np->n_flag & N_ERROR)) {
	strcpy(str1, "");

	if (! inputname) {
		inputname = str1;  /* default */
		if (ambigmac)
			implmacros(np, lp, &basename, &inputname);
	}
	setDFmacro("<", inputname);

	if (! basename)
		basename = str1;
	setDFmacro("*", basename);

	for (dp = qdp; dp; dp = qdp) {
		if (strlen(str1))
			strcat(str1, " ");
		strcat(str1, dp->d_name->n_name);
		qdp = dp->d_next;
		free(dp);
	}
	setmacro("?", str1);
	setDFmacro("@", np->n_name);

	if (lp)		/* lp set if doing a :: rule */
		docmds1(np, lp);
	else
		docmds(np);
    }
}


/* Recursive routine to make a target. */
int
make(struct name *np, int level)
{
    struct depend *dp, *qdp;
    struct line *lp;
    time_t now, t, dtime = 0;
    int8_t dbgfirst = TRUE;
    char *basename = NULL;
    char *inputname = NULL;

    if (np->n_flag & N_DONE) {
	if (dbginfo)
		dbgprint(level, np, "already done");
	return 0;
    }

    modtime(np);		/*  Gets modtime of this file  */

    while (time(&now) == np->n_time) {
	/*
	 * Time of target is equal to the current time.
	 * This bothers us, because we can't tell if it needs to be
	 * updated if we update a file it depends on within a second.
	 * So wait a second.  (A per-second timer is too coarse for
	 * today's fast machines.)
	 */
#ifdef _WIN32
	Sleep(1 * 1000);
#else
	sleep(1);
#endif
    }

    if (rules) {
	for (lp = np->n_line; lp; lp = lp->l_next)
		if (lp->l_cmd)
			break;
	if (! lp)
		dyndep(np, &basename, &inputname);
    }

    if (! (np->n_flag & (N_TARG | N_EXISTS))) {
	fprintf(stderr,"%s: Don't know how to make %s\n", myname, np->n_name);
	if (conterr) {
		np->n_flag |= N_ERROR;
		if (dbginfo)
			dbgprint(level, np, "don't know how to make");
		return 0;
	} else
		exit(1);
    }

    for (qdp = NULL, lp = np->n_line; lp; lp = lp->l_next) {
	for (dp = lp->l_dep; dp; dp = dp->d_next) {
		if (dbginfo && dbgfirst) {
			dbgprint(level, np, " {");
			dbgfirst = FALSE;
		}
		make(dp->d_name, level+1);
		if (np->n_time < dp->d_name->n_time)
			qdp = newdep(dp->d_name, qdp);
		dtime = max(dtime, dp->d_name->n_time);
		if (dp->d_name->n_flag & N_ERROR)
			np->n_flag |= N_ERROR;
		if (dp->d_name->n_flag & N_EXEC)
			np->n_flag |= N_EXEC;
	}

	if (!quest && (np->n_flag & N_DOUBLE) && (np->n_time < dtime || !( np->n_flag & N_EXISTS))) {
		execflag = FALSE;
		make1(np, lp, qdp, basename, inputname); /* free()'s qdp */
		dtime = 0;
		qdp = NULL;

		if (execflag)
			np->n_flag |= N_EXEC;
	}
    }

    np->n_flag |= N_DONE;

    if (quest) {
	t = np->n_time;
	np->n_time = now;
	return (t < dtime);
    } else if ((np->n_time < dtime || !( np->n_flag & N_EXISTS)) && !(np->n_flag & N_DOUBLE)) {
	execflag = FALSE;

	make1(np, NULL, qdp, basename, inputname); /* frees qdp */
	np->n_time = now;

	if (execflag)
		np->n_flag |= N_EXEC;
    } else if (np->n_flag & N_EXEC)
	np->n_time = now;

    if (dbginfo) {
	if (dbgfirst) {
		if (np->n_flag & N_ERROR)
			dbgprint(level, np, "skipped because of error");
		else if (np->n_flag & N_EXEC)
			dbgprint(level, np, "successfully made");
		else
			dbgprint(level, np, "is up to date");
	} else {
		if (np->n_flag & N_ERROR)
			dbgprint(level, NULL, "} skipped because of error");
		else
			if (np->n_flag & N_EXEC)
				dbgprint(level, NULL, "} successfully made");
			else
				dbgprint(level, NULL, "} is up to date");
	}
    }

    if (level == 0 && !(np->n_flag & N_EXEC))
	printf("%s: '%s' is up to date\n", myname, np->n_name);

    if (basename)
	free(basename);

    return 0;
}



/* Syntax error handler.  Print message, with line number, and exits. */
void
error(const char *msg, const char *a1)
{
    fprintf(stderr, "%s: ", myname);
    fprintf(stderr, msg, a1);
    if (lineno)
	fprintf(stderr, " in %s near line %d", makefile, lineno);
    fputc('\n', stderr);

    exit(1);
}


/*
 * Read a line into the supplied string.  Remove comments,
 * ignore blank lines. Deal with quoted (\) #, and quoted
 * newlines.  If EOF return TRUE.
 *
 * The comment handling code has been changed to leave comments and
 * backslashes alone in shell commands (lines starting with a tab).
 *
 * This is not what POSIX wants, but what all makes do.  (KJB)
 */
int8_t
getaline(struct str *strs, FILE *fd)
{
    char *a, *p, *q;
    int c;

    for (;;) {
	strs->pos = 0;
	for (;;) {
		do {
			if (strs->pos >= strs->len)
				strrealloc(strs);
			if ((c = getc(fd)) == EOF)
				return TRUE;		/* EOF */
			(*strs->ptr)[strs->pos++] = c;
		} while (c != '\n');

		lineno++;

		if (strs->pos >= 2 && (*strs->ptr)[strs->pos - 2] == '\\') {
			(*strs->ptr)[strs->pos - 2] = '\n';
			strs->pos--;
		} else
			break;
	}

	if (strs->pos >= strs->len)
		strrealloc(strs);
	(*strs->ptr)[strs->pos] = '\0';

	p = q = *strs->ptr;
	while (isspace(*q))
		q++;
	if (*p != '\t' || *q == '#') {
		while (((q = strchr(p, '#')) != NULL) && (p != q) && (q[-1] == '\\')) {
			a = q - 1;	/*  Del \ chr; move rest back  */
			p = q;
			while (*a++ = *q++)
				;
		}

		if (q != NULL) {
			q[0] = '\n';
			q[1] = '\0';
		}
	}

	p = *strs->ptr;
	while (isspace(*p))	/* Checking for blank */
		p++;

	if (*p != '\0')
		return FALSE;
    }
}


/*
 * Get a word from the current line, surounded by white space.
 * return a pointer to it. String returned has no white spaces
 * in it.
 */
char *
gettok(char **ptr)
{
    char *p;

    while (isspace(**ptr))	/* Skip spaces */
	(*ptr)++;

    if (**ptr == '\0')		/* Nothing after spaces */
	return NULL;

    p = *ptr;			/* word starts here */

    while ((**ptr != '\0') && (!isspace(**ptr)))
	(*ptr)++;		/* Find end of word */

    *(*ptr)++ = '\0';		/* Terminate it */

    return p;
}



/*
 * Dynamic dependency.
 *
 * This routine applies the suffix rules to try and find a source
 * and a set of rules for a missing target.  If found, np is made
 * into a target with the implicit source name, and rules.
 *
 * Returns TRUE if np was made into a target.
 */
int8_t
dyndep(struct name *np, char **pbasename, char **pinputname)
{
    char *p, *q;
    char *suff;					/* Old suffix */
    struct name *op = NULL, *optmp;		/* New dependent */
    struct name *sp;				/* Suffix */
    struct line *lp, *nlp;
    struct depend *dp, *ndp;
    struct cmd *cmdp;
    char *newsuff;
    int8_t depexists = FALSE;

    p = str1;
    q = np->n_name;
    suff = suffix(q);
    while (*q && (q < suff || !suff))
	*p++ = *q++;
    *p = '\0';
    if ((*pbasename = (char *)malloc(strlen(str1)+1)) == NULL)
	fatal("No memory for basename", NULL, 0);
    strcpy(*pbasename, str1);
    if (! suff)
	suff = p - str1 + *pbasename;  /* set suffix to nullstring */

    if (! ((sp = newname(".SUFFIXES"))->n_flag & N_TARG))
	return FALSE;

    /* search all .SUFFIXES lines */
    for (lp = sp->n_line; lp; lp = lp->l_next)
	/* try all suffixes */
	for (dp = lp->l_dep; dp; dp = dp->d_next) {
		/* compose implicit rule name (.c.o)...*/
		newsuff = dp->d_name->n_name;
		while (strlen(suff)+strlen(newsuff)+1 >= str1s.len)
			strrealloc(&str1s);
		p = str1;
		q = newsuff;
		while (*p++ = *q++)
			;
		p--;
		q = suff;
		while (*p++ = *q++)
			;

		/* look if the rule exists */
		sp = newname(str1);
		if (sp->n_flag & N_TARG) {
			/* compose resulting dependency name */
			while (strlen(*pbasename) + strlen(newsuff)+1 >= str1s.len)
				strrealloc(&str1s);
			q = *pbasename;
			p = str1;
			while (*p++ = *q++)
				;
			p--;
			q = newsuff;
			while (*p++ = *q++)
				;

			/* test if dependency file or an explicit rule exists */
           		if ((optmp = testname(str1)) != NULL) {
				/* store first possible dependency as default */
				if (op == NULL) {
					op = optmp;
					cmdp = sp->n_line->l_cmd;
				}

				/* check if testname is an explicit dependency */
				for (nlp = np->n_line; nlp; nlp = nlp->l_next) {
					for (ndp = nlp->l_dep; ndp; ndp = ndp->d_next) {
						if (strcmp(ndp->d_name->n_name, str1) == 0) {
							op = optmp;
							cmdp = sp->n_line->l_cmd;
							ndp = NULL;
							goto found2;
						}
						depexists = TRUE;
					}
				}

				/* if no explicit dependencies : accept testname */
				if (! depexists)
					goto found;
			}
		}
	}

    if (op == NULL) {
	if (np->n_flag & N_TARG) {     /* DEFAULT handling */
		if (! ((sp = newname(".DEFAULT"))->n_flag & N_TARG))
			return FALSE;

		if (! (sp->n_line))
			return FALSE;

		cmdp = sp->n_line->l_cmd;
		for (nlp = np->n_line; nlp; nlp = nlp->l_next) {
			if (ndp = nlp->l_dep) {
				op = ndp->d_name;
				ndp = NULL;
				goto found2;
			}
		}
		newline(np, NULL, cmdp, 0);
		*pinputname = NULL;
		*pbasename  = NULL;
		return TRUE;
	} else
		return FALSE;
    }

found:
    ndp = newdep(op, NULL);

found2:
    newline(np, ndp, cmdp, 0);
    *pinputname = op->n_name;

    return TRUE;
}


/* Make the default rules. */
void
makerules(void)
{
    struct cmd *cp;
    struct name *np;
    struct depend *dp;

    /* Some of the Windows implicit rules. */
#ifdef _WIN32
    setmacro("RM", "DEL /Q 2>NUL");
# if defined(_MSC_VER)
    setmacro("CC", "cl");
    setmacro("CFLAGS", "/O2");
#   define F_OBJ "obj"
# elif defined(__TCC__)
    setmacro("CC", "tcc");
    setmacro("CFLAGS", "-O2");
#   define F_OBJ "o"
# else
    setmacro("CC", "cc");
    setmacro("CFLAGS", "-O");
#   define F_OBJ "o"
# endif

# if defined(_MSC_VER)
    cp = newcmd("$(CC) -c $(CFLAGS) $<", NULL);
    np = newname(".c.o");
    newline(np, NULL, cp, 0);
# endif

    cp = newcmd("$(CC) -c $(CFLAGS) $<", NULL);
    np = newname(".c." F_OBJ);
    newline(np, NULL, cp, 0);

    cp = newcmd("$(CC) -c $(CFLAGS) $<", NULL);
    np = newname(".asm." F_OBJ);
    newline(np, NULL, cp, 0);

# ifdef _MSC_VER
    cp = newcmd("$(CC) $(CFLAGS) /Fe$@ $<", NULL);
# else
    cp = newcmd("$(CC) $(CFLAGS) -o $@ $<", NULL);
# endif
    np = newname(".c");
    newline(np, NULL, cp, 0);

    np = newname("." F_OBJ);
    dp = newdep(np, NULL);
    np = newname(".asm");
    dp = newdep(np, dp);
    np = newname(".c");
    dp = newdep(np, dp);
    np = newname(".SUFFIXES");
    newline(np, dp, NULL, 0);
#endif /* _WIN32 */

    /* Some of the UNIX implicit rules. */
#ifdef unix
    setmacro("RM", "rm -f");
# if defined(__TCC__)
    setmacro("CC", "tcc");
    setmacro("CFLAGS", "-O2");
# else
    setmacro("CC", "cc");
    setmacro("CFLAGS", "-O");
# endif

    cp = newcmd("$(CC) -S $(CFLAGS) $<", NULL);
    np = newname(".c.s");
    newline(np, NULL, cp, 0);

    cp = newcmd("$(CC) -c $(CFLAGS) $<", NULL);
    np = newname(".c.o");
    newline(np, NULL, cp, 0);

    cp = newcmd("$(CC) $(CFLAGS) -o $@ $<", NULL);
    np = newname(".c");
    newline(np, NULL, cp, 0);

    cp = newcmd("$(CC) -c $(CFLAGS) $<", NULL);
    np = newname(".s.o");
    newline(np, NULL, cp, 0);

    setmacro("YACC", "yacc");
    /*setmacro("YFLAGS", "");	*/
    cp = newcmd("$(YACC) $(YFLAGS) $<", NULL);
    cp = newcmd("mv   y.tab.c $@", cp);
    np = newname(".y.c");
    newline(np, NULL, cp, 0);

    cp = newcmd("$(YACC) $(YFLAGS) $<", NULL);
    cp = newcmd("$(CC) $(CFLAGS) -c y.tab.c", cp);
    cp = newcmd("mv y.tab.o $@", cp);
    np = newname(".y.o");
    cp = newcmd("rm y.tab.c", cp);
    newline(np, NULL, cp, 0);

    setmacro("FLEX", "flex");
    cp = newcmd("$(FLEX) $(FLEX_FLAGS) $<", NULL);
    cp = newcmd("mv lex.yy.c $@", cp);
    np = newname(".l.c");
    newline(np, NULL, cp, 0);

    cp = newcmd("$(FLEX) $(FLEX_FLAGS) $<", NULL);
    cp = newcmd("$(CC) $(CFLAGS) -c lex.yy.c", cp);
    cp = newcmd("mv lex.yy.o $@", cp);
    np = newname(".l.o");
    cp = newcmd("rm lex.yy.c", cp);
    newline(np, NULL, cp, 0);

    np = newname(".o");
    dp = newdep(np, NULL);
    np = newname(".s");
    dp = newdep(np, dp);
    np = newname(".c");
    dp = newdep(np, dp);
    np = newname(".y");
    dp = newdep(np, dp);
    np = newname(".l");
    dp = newdep(np, dp);
    np = newname(".SUFFIXES");
    newline(np, dp, NULL, 0);
#endif /* unix */
}

/*************************************************************************
 *
 *  m a k e :   m a i n . c
 *
 *========================================================================
 * Edition history
 *
 *  #    Date                         Comments                       By
 * --- -------- ---------------------------------------------------- ---
 *   1    ??                                                         ??
 *   2 01.07.89 strcmp(makefile,..) only if makefile a valid ptr.    RAL
 *   3 23.08.89 initname() added                                     RAL
 *   4 30.08.89 argument parsing impr., indention ch., macro fl. add.PSH,RAL
 *   5 03.09.89 k-option added, initname -> init changed             RAL
 *   6 06.09.89 environment, MAKEFLAGS, e,d,a options added,         RAL
 *   7 09.09.89 tos support added, fatal args added, fopen makefile  PHH,RAL
 *   8 17.09.89 setoptions fixed for __STDC__                        RAL
 * ------------ Version 2.0 released ------------------------------- RAL
 *
 *************************************************************************/

/*
 *	make:
 *
 *	-a try to guess undefined ambiguous macros (*,<)
 *	-d print debugging info
 *	-e environment macro def. overwrite makefile def.
 *	-f makefile name
 *	-i ignore exit status
 *	-k continue on errors
 *	-n pretend to make
 *	-p print all macros & targets
 *	-q question up-to-dateness of target.  Return exit status 1 if not
 *	-r don't not use inbuilt rules
 *	-s make silently
 *	-t touch files instead of making them
 *	-v show version
 */
#define EXTERN
#ifndef INIT
#define INIT(x) = x
#endif
#define INITARRAY

static char version[]= "3.0";
static FILE *ifd;           /*  Input file desciptor  */
static char *ptrmakeflags;

/* There must be enough 'space' for all possible flags ! */
static char  makeflags[] = "MAKEFLAGS=                    ";


static void
usage(void)
{
    fprintf(stderr, "Syntax: %s [{options | macro=val | target}]\n\n", myname);
    fprintf(stderr, "Options : -a : try to guess undefined ambiguous macros (*,<)\n");
    fprintf(stderr, "          -d : print debugging information\n");
    fprintf(stderr, "          -e : environment macro def. overwrite makefile def.\n");
    fprintf(stderr, "          -f filename : makefile name (default: makefile, Makefile)\n");
    fprintf(stderr, "          -i : ignore exit status of executed commands\n");
    fprintf(stderr, "          -k : continue with unrelated branches on errors\n");
    fprintf(stderr, "          -n : pretend to make\n");
    fprintf(stderr, "          -p : print all macros & targets\n");
    fprintf(stderr, "          -q : question up-to-dateness of target\n");
    fprintf(stderr, "          -r : don't use inbuilt rules\n");
    fprintf(stderr, "          -s : make silently\n");
    fprintf(stderr, "          -t : touch files instead of making them\n");
    fprintf(stderr, "          -v : show version\n");
    fprintf(stderr, "\nEnvironment: MAKEFLAGS\n");

    exit(1);
}


static void
setoption(char option)
{
    char *c;

    if (isupper(option))
	option = tolower(option);

    switch (option) {
	case 'a':
		ambigmac = TRUE;
		break;

	case 'd':
		dbginfo = TRUE;
		break;

	case 'e':
		useenv = TRUE;
		break;

	case 'i':	/* ignore fault mode */
		ignore = TRUE;
		break;

	case 'k':	/* continue on errror */
		conterr = TRUE;
		break;

	case 'n':	/* pretend mode */
		domake = FALSE;
		break;

	case 'p':
		print = TRUE;
		break;

	case 'q':
		quest = TRUE;
		break;

	case 'r':
		rules = FALSE;
		break;

	case 's':	/* silent about commands */
		silent = TRUE;
		break;

	case 't':
		dotouch = TRUE;
		break;

	case 'v':
		printf("This is MAKE version %s\n", version);
		exit(0);
		/*NOTREACHED*/

	default:	/* wrong option */
		usage();
		/*NOTREACHED*/
    }

    for (c = ptrmakeflags; !isspace((int)*c); c++)
	if (*c == option)
		return;

    *c = option;
}


int
main(int argc, char **argv, char **envp)
{
    struct name *np;
    struct macro *mp;
    char **targv;
    char **nargv;	/* for removing items from argv */
    char *p;		/* For argument processing */
    int estat = 0;	/* For question */
    int targc;		/* temporary for multiple scans */

    ptrmakeflags = &makeflags[10];
    myname = (argc-- < 1) ? "make" : *argv++;

    targc = argc;
    targv = nargv = argv;
    while (targc--) {
	if ((p = strchr(*targv, '=')) != NULL) {
		*p = '\0';
		mp = setmacro(*targv, p + 1);
		mp->m_flag |= M_OVERRIDE;
		--argc;
	} else
		*nargv++ = *targv;

	++targv;
    }

    targc = argc;
    targv = nargv = argv;
    while (targc--) {
	if (**targv == '-') {
		--argc;
		p = *targv++;
		while (*++p != '\0') {
			switch (*p) {
			case 'f':	/* alternate file name */
				if (*++p == '\0') {
					--argc;
					if (targc-- == 0)
						usage();
					p = *targv++;
				}
				makefile = p;
				goto end_of_args;

			default :
				setoption(*p);
				break;
			}
		}
end_of_args:;
	} else
		*nargv++ = *targv++;
    }

    /* evaluate and update environment MAKEFLAGS */
    if ((p = getenv("MAKEFLAGS")) != NULL)
	while (*p)
		setoption(*p++);

    for (p = ptrmakeflags; !isspace((int)*p); p++)
			;
    *p = '\0';

#ifndef NOLIBC
    putenv(makeflags);
#endif

    if (makefile && strcmp(makefile, "-") == 0)  /*   use stdin as makefile  */
	ifd = stdin;
    else if (! makefile) {    /*  If no file, then use default */
	if ((ifd = fopen(makefile = DEFN1, "r")) == NULL) {
		if (errno != MNOENT || !DEFN2)
			fatal("Can't open %s: %s", DEFN1, errno);
		else if ((ifd = fopen(makefile = DEFN2, "r")) == NULL)
			fatal("Can't open %s: %s", DEFN2, errno);
	}
    } else if ((ifd = fopen(makefile, "r")) == NULL)
	fatal("Can't open %s: %s", makefile, errno);

    init();

    makerules();

    mp = setmacro("MAKE", myname);
    mp->m_flag |= M_MAKE;
    setmacro("$", "$");

    /* set environment macros */
    while (*envp) {
	if ((p = strchr(*envp, '=')) != NULL) {
		*p = '\0';
		mp = setmacro(*envp, p + 1);
		*p = '=';
		if (useenv)
			mp->m_flag |= M_OVERRIDE;
	} else
		fatal("invalid environment: %s", *envp, 0);

	++envp;
    }

    input(ifd);		/* Input all the gunga */
    fclose(ifd);	/* Finished with makefile */
    lineno = 0;		/* Any calls to error now print no line number */

    if (print)
	prt();		/* Print out structures */

    np = newname(".SILENT");
    if (np->n_flag & N_TARG)
	silent = TRUE;

    np = newname(".IGNORE");
    if (np->n_flag & N_TARG)
	ignore = TRUE;

    precious();

    if (! firstname)
	fatal("No targets defined", NULL, 0);

    circh();	/*  Check circles in target definitions  */

    if (!argc)
	estat = make(firstname, 0);
    else
	while (argc--) {
		estat |= make(newname(*argv++), 0);
	}

    if (quest)
	return(estat);
    else

    return(0);
}

void fatal(const char *msg, const char *a1, int a2) {
    fprintf(stderr, "%s: ", myname);
    fprintf(stderr, msg, a1, itoa(a2));
    fputc('\n', stderr);

    exit(1);
}