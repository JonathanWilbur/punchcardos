/* Copied from: https://github.com/idunham/m4
License below.
This is not going to work with nolibc, because it uses regular expressions. */

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ozan Yigit at York University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <regex.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t len;

    for (len = 0; len < size; len++) {
        dst[len] = src[len];
        if (!dst[len])
            return len;
    }
    if (size)
        dst[size - 1] = '\0';

    while (src[len])
        len++;

    return len;
}

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define iswhite(c) ((c) == ' ' || (c) == '\t')

/*
 * STREQ is an optimised strcmp(a,b)==0
 * STREQN is an optimised strncmp(a,b,n)==0; assumes n > 0
 */
#define STREQ(a, b) ((a)[0] == (b)[0] && strcmp(a, b) == 0)
#define STREQN(a, b, n) ((a)[0] == (b)[0] && strncmp(a, b, n) == 0)

#define YES 1
#define NO 0

#define MACRTYPE 1
#define DEFITYPE 2
#define EXPRTYPE 3
#define SUBSTYPE 4
#define IFELTYPE 5
#define LENGTYPE 6
#define CHNQTYPE 7
#define SYSCTYPE 8
#define UNDFTYPE 9
#define INCLTYPE 10
#define SINCTYPE 11
#define PASTTYPE 12
#define SPASTYPE 13
#define INCRTYPE 14
#define IFDFTYPE 15
#define PUSDTYPE 16
#define POPDTYPE 17
#define SHIFTYPE 18
#define DECRTYPE 19
#define DIVRTYPE 20
#define UNDVTYPE 21
#define DIVNTYPE 22
#define MKTMTYPE 23
#define ERRPTYPE 24
#define M4WRTYPE 25
#define TRNLTYPE 26
#define DNLNTYPE 27
#define DUMPTYPE 28
#define CHNCTYPE 29
#define INDXTYPE 30
#define SYSVTYPE 31
#define EXITTYPE 32
#define DEFNTYPE 33
#define SELFTYPE 34
#define INDIRTYPE 35
#define BUILTINTYPE 36
#define PATSTYPE 37
#define FILENAMETYPE 38
#define LINETYPE 39
#define REGEXPTYPE 40
#define ESYSCMDTYPE 41
#define TRACEONTYPE 42
#define TRACEOFFTYPE 43

#define TYPEMASK 63   /* Keep bits really corresponding to a type. */
#define RECDEF 256    /* Pure recursive def, don't expand it */
#define NOARGS 512    /* builtin needs no args */
#define NEEDARGS 1024 /* mark builtin that need args with this */

/*
 * m4 special characters
 */

#define ARGFLAG '$'
#define LPAREN '('
#define RPAREN ')'
#define LQUOTE '`'
#define RQUOTE '\''
#define COMMA ','
#define SCOMMT '#'
#define ECOMMT '\n'

#ifdef msdos
#define system(str) (-1)
#endif

/*
 * other important constants
 */

#define EOS '\0'
#define MAXINP 10         /* maximum include files   	    */
#define MAXOUT 10         /* maximum # of diversions 	    */
#define BUFSIZE 4096      /* starting size of pushback buffer */
#define INITSTACKMAX 4096 /* starting size of call stack      */
#define STRSPMAX 4096     /* starting size of string space    */
#define MAXTOK 512        /* maximum chars in a tokn 	    */
#define HASHSIZE 199      /* maximum size of hashtab 	    */
#define MAXCCHARS 5       /* max size of comment/quote delim  */

#define ALL 1
#define TOP 0

#define TRUE 1
#define FALSE 0
#define cycle for (;;)

/*
 * m4 data structures
 */

typedef struct ndblock *ndptr;

struct ndblock {       /* hastable structure         */
    char *name;        /* entry name..               */
    char *defn;        /* definition..               */
    unsigned int type; /* type of the entry..        */
    unsigned int hv;   /* hash function value..      */
    ndptr nxtptr;      /* link to next entry..       */
};

#define nil ((ndptr)0)

struct keyblk {
    const char *knam; /* keyword name */
    int ktyp;         /* keyword type */
};

typedef union { /* stack structure */
    int sfra;   /* frame entry  */
    char *sstr; /* string entry */
} stae;

struct input_file {
    FILE *file;
    char *name;
    unsigned long lineno;
    int c;
};

#define CURRENT_NAME (infile[ilevel].name)
#define CURRENT_LINE (infile[ilevel].lineno)
/*
 * macros for readibility and/or speed
 *
 *      gpbc()  - get a possibly pushed-back character
 *      pushf() - push a call frame entry onto stack
 *      pushs() - push a string pointer onto stack
 */
#define gpbc() (bp > bufbase) ? *--bp : obtain_char(infile + ilevel)
#define pushf(x)               \
    do {                       \
        if (++sp == STACKMAX)  \
            enlarge_stack();   \
        mstack[sp].sfra = (x); \
        sstack[sp] = 0;        \
    } while (0)

#define pushs(x)               \
    do {                       \
        if (++sp == STACKMAX)  \
            enlarge_stack();   \
        mstack[sp].sstr = (x); \
        sstack[sp] = 1;        \
    } while (0)

#define pushs1(x)              \
    do {                       \
        if (++sp == STACKMAX)  \
            enlarge_stack();   \
        mstack[sp].sstr = (x); \
        sstack[sp] = 0;        \
    } while (0)

/*
 *	    .				   .
 *	|   .	|  <-- sp		|  .  |
 *	+-------+			+-----+
 *	| arg 3 ----------------------->| str |
 *	+-------+			|  .  |
 *	| arg 2 ---PREVEP-----+ 	   .
 *	+-------+	      |
 *	    .		      |		|     |
 *	+-------+	      | 	+-----+
 *	| plev	|  PARLEV     +-------->| str |
 *	+-------+			|  .  |
 *	| type	|  CALTYP		   .
 *	+-------+
 *	| prcf	---PREVFP--+
 *	+-------+  	   |
 *	|   .	|  PREVSP  |
 *	    .	   	   |
 *	+-------+	   |
 *	|	<----------+
 *	+-------+
 *
 */
#define PARLEV (mstack[fp].sfra)
#define CALTYP (mstack[fp - 1].sfra)
#define PREVEP (mstack[fp + 3].sstr)
#define PREVSP (fp - 3)
#define PREVFP (mstack[fp - 2].sfra)

/* eval.c */
extern void eval(const char *[], int, int);
extern void dodefine(const char *, const char *);
extern unsigned long expansion_id;

/* expr.c */
extern int expr(const char *);

/* gnum4.c */
extern void addtoincludepath(const char *);
extern struct input_file *fopen_trypath(struct input_file *, const char *);
extern void doindir(const char *[], int);
extern void dobuiltin(const char *[], int);
extern void dopatsubst(const char *[], int);
extern void doregexp(const char *[], int);
extern void doprintlineno(struct input_file *);
extern void doprintfilename(struct input_file *);
extern void doesyscmd(const char *);

/* look.c */
extern ndptr addent(const char *);
extern unsigned hash(const char *);
extern ndptr lookup(const char *);
extern void remhash(const char *, int);

/* main.c */
extern void outputstr(const char *);
extern int builtin_type(const char *);
extern const char *builtin_realname(int);

/* misc.c */
extern void chrsave(int);
extern char *compute_prevep(void);
extern void getdiv(int);
extern ptrdiff_t indx(const char *, const char *);
extern void initspaces(void);
extern void killdiv(void);
extern void onintr(int);
extern void pbnum(int);
extern void pbunsigned(unsigned long);
extern void pbstr(const char *);
extern void putback(int);
extern void *xalloc(size_t);
extern char *xstrdup(const char *);
extern void usage(const char *);
extern void resizedivs(int);
extern size_t buffer_mark(void);
extern void dump_buffer(FILE *, size_t);

extern int obtain_char(struct input_file *);
extern void set_input(struct input_file *, FILE *, const char *);
extern void release_input(struct input_file *);

/* speeded-up versions of chrsave/putback */
#define PUTBACK(c)              \
    do {                        \
        if (bp >= endpbb)       \
            enlarge_bufspace(); \
        *bp++ = (c);            \
    } while (0)

#define CHRSAVE(c)              \
    do {                        \
        if (ep >= endest)       \
            enlarge_strspace(); \
        *ep++ = (c);            \
    } while (0)

/* and corresponding exposure for local symbols */
extern void enlarge_bufspace(void);
extern void enlarge_strspace(void);
extern char *endpbb;
extern char *endest;

/* trace.c */
extern void mark_traced(const char *, int);
extern int is_traced(const char *);
extern void trace_file(const char *);
extern ssize_t trace(const char **, int, struct input_file *);
extern void finish_trace(size_t);
extern void set_trace_flags(const char *);
extern int traced_macros;
extern FILE *traceout;

extern ndptr hashtab[];            /* hash table for macros etc. */
extern stae *mstack;               /* stack of m4 machine */
extern char *sstack;               /* shadow stack, for string space extension */
extern FILE *active;               /* active output file pointer */
extern struct input_file infile[]; /* input file stack (0=stdin) */
extern FILE **outfile;             /* diversion array(0=bitbucket) */
extern int maxout;                 /* maximum number of diversions */
extern int fp;                     /* m4 call frame pointer */
extern int ilevel;                 /* input file stack pointer */
extern int oindex;                 /* diversion index. */
extern int sp;                     /* current m4 stack pointer */
extern char *bp;                   /* first available character */
extern char *buf;                  /* push-back buffer */
extern char *bufbase;              /* buffer base for this ilevel */
extern char *bbase[];              /* buffer base per ilevel */
extern char ecommt[MAXCCHARS + 1]; /* end character for comment */
extern char *ep;                   /* first free char in strspace */
extern char lquote[MAXCCHARS + 1]; /* left quote character (`) */
extern char *m4wraps;              /* m4wrap string default. */
extern char *null;                 /* as it says.. just a null. */
extern char rquote[MAXCCHARS + 1]; /* right quote character (') */
extern char scommt[MAXCCHARS + 1]; /* start character for comment */
extern int mimic_gnu;              /* behaves like gnu-m4 */

/*
 * Definitions of diversion files.  If the name of the file is changed,
 * adjust UNIQUE to point to the wildcard (*) character in the filename.
 */

#ifdef msdos
#define _PATH_DIVNAME "\\M4*XXXXXX" /* msdos diversion files */
#define UNIQUE 3                    /* unique char location */
#endif

#if defined(unix) || defined(__NetBSD__) || defined(__OpenBSD__)
#define _PATH_DIVNAME "/tmp/m4.0XXXXXXXXXX" /* unix diversion files */
#define UNIQUE 8                            /* unique char location */
#endif

#ifdef vms
#define _PATH_DIVNAME "sys$login:m4*XXXXXX" /* vms diversion files */
#define UNIQUE 12                           /* unique char location */
#endif

#define BUILTIN_MARKER "__builtin_"

static void dodefn(const char *);
static void dopushdef(const char *, const char *);
static void dodump(const char *[], int);
static void dotrace(const char *[], int, int);
static void doifelse(const char *[], int);
static int doincl(const char *);
static int dopaste(const char *);
static void gnu_dochq(const char *[], int);
static void dochq(const char *[], int);
static void gnu_dochc(const char *[], int);
static void dochc(const char *[], int);
static void dodiv(int);
static void doundiv(const char *[], int);
static void dosub(const char *[], int);
static void map(char *, const char *, const char *, const char *);
static const char *handledash(char *, char *, const char *);
static void expand_builtin(const char *[], int, int);
static void expand_macro(const char *[], int);
static void dump_one_def(ndptr);

unsigned long expansion_id;

/*
 * eval - eval all macros and builtins calls
 *	  argc - number of elements in argv.
 *	  argv - element vector :
 *			argv[0] = definition of a user
 *				  macro or nil if built-in.
 *			argv[1] = name of the macro or
 *				  built-in.
 *			argv[2] = parameters to user-defined
 *			   .	  macro or built-in.
 *			   .
 *
 * A call in the form of macro-or-builtin() will result in:
 *			argv[0] = nullstr
 *			argv[1] = macro-or-builtin
 *			argv[2] = nullstr
 *
 * argc is 3 for macro-or-builtin() and 2 for macro-or-builtin
 */
void
    eval(argv, argc, td)
        const char *argv[];
int argc;
int td;
{
    ssize_t mark = -1;

    expansion_id++;
    if (td & RECDEF)
        errx(1, "%s at line %lu: expanding recursive definition for %s",
             CURRENT_NAME, CURRENT_LINE, argv[1]);
    if (traced_macros && is_traced(argv[1]))
        mark = trace(argv, argc, infile + ilevel);
    if (td == MACRTYPE)
        expand_macro(argv, argc);
    else
        expand_builtin(argv, argc, td);
    if (mark != -1)
        finish_trace(mark);
}

/*
 * expand_builtin - evaluate built-in macros.
 */
void
    expand_builtin(argv, argc, td)
        const char *argv[];
int argc;
int td;
{
    int c, n;
    int ac;
    static int sysval = 0;

#ifdef DEBUG
    printf("argc = %d\n", argc);
    for (n = 0; n < argc; n++)
        printf("argv[%d] = %s\n", n, argv[n]);
#endif

    /*
     * if argc == 3 and argv[2] is null, then we
     * have macro-or-builtin() type call. We adjust
     * argc to avoid further checking..
     */
    ac = argc;

    if (argc == 3 && !*(argv[2]))
        argc--;

    switch (td & TYPEMASK) {
        case DEFITYPE:
            if (argc > 2)
                dodefine(argv[2], (argc > 3) ? argv[3] : null);
            break;

        case PUSDTYPE:
            if (argc > 2)
                dopushdef(argv[2], (argc > 3) ? argv[3] : null);
            break;

        case DUMPTYPE:
            dodump(argv, argc);
            break;

        case TRACEONTYPE:
            dotrace(argv, argc, 1);
            break;

        case TRACEOFFTYPE:
            dotrace(argv, argc, 0);
            break;

        case EXPRTYPE:
            /*
             * doexpr - evaluate arithmetic
             * expression
             */
            if (argc > 2)
                pbnum(expr(argv[2]));
            break;

        case IFELTYPE:
            if (argc > 4)
                doifelse(argv, argc);
            break;

        case IFDFTYPE:
            /*
             * doifdef - select one of two
             * alternatives based on the existence of
             * another definition
             */
            if (argc > 3) {
                if (lookup(argv[2]) != nil)
                    pbstr(argv[3]);
                else if (argc > 4)
                    pbstr(argv[4]);
            }
            break;

        case LENGTYPE:
            /*
             * dolen - find the length of the
             * argument
             */
            pbnum((argc > 2) ? strlen(argv[2]) : 0);
            break;

        case INCRTYPE:
            /*
             * doincr - increment the value of the
             * argument
             */
            if (argc > 2)
                pbnum(atoi(argv[2]) + 1);
            break;

        case DECRTYPE:
            /*
             * dodecr - decrement the value of the
             * argument
             */
            if (argc > 2)
                pbnum(atoi(argv[2]) - 1);
            break;

        case SYSCTYPE:
            /*
             * dosys - execute system command
             */
            if (argc > 2)
                sysval = system(argv[2]);
            break;

        case SYSVTYPE:
            /*
             * dosysval - return value of the last
             * system call.
             *
             */
            pbnum(sysval);
            break;

        case ESYSCMDTYPE:
            if (argc > 2)
                doesyscmd(argv[2]);
            break;
        case INCLTYPE:
            if (argc > 2)
                if (!doincl(argv[2]))
                    err(1, "%s at line %lu: include(%s)",
                        CURRENT_NAME, CURRENT_LINE, argv[2]);
            break;

        case SINCTYPE:
            if (argc > 2)
                (void)doincl(argv[2]);
            break;
#ifdef EXTENDED
        case PASTTYPE:
            if (argc > 2)
                if (!dopaste(argv[2]))
                    err(1, "%s at line %lu: paste(%s)",
                        CURRENT_NAME, CURRENT_LINE, argv[2]);
            break;

        case SPASTYPE:
            if (argc > 2)
                (void)dopaste(argv[2]);
            break;
#endif
        case CHNQTYPE:
            if (mimic_gnu)
                gnu_dochq(argv, ac);
            else
                dochq(argv, argc);
            break;

        case CHNCTYPE:
            if (mimic_gnu)
                gnu_dochc(argv, ac);
            else
                dochc(argv, argc);
            break;

        case SUBSTYPE:
            /*
             * dosub - select substring
             *
             */
            if (argc > 3)
                dosub(argv, argc);
            break;

        case SHIFTYPE:
            /*
             * doshift - push back all arguments
             * except the first one (i.e. skip
             * argv[2])
             */
            if (argc > 3) {
                for (n = argc - 1; n > 3; n--) {
                    pbstr(rquote);
                    pbstr(argv[n]);
                    pbstr(lquote);
                    putback(COMMA);
                }
                pbstr(rquote);
                pbstr(argv[3]);
                pbstr(lquote);
            }
            break;

        case DIVRTYPE:
            if (argc > 2 && (n = atoi(argv[2])) != 0)
                dodiv(n);
            else {
                active = stdout;
                oindex = 0;
            }
            break;

        case UNDVTYPE:
            doundiv(argv, argc);
            break;

        case DIVNTYPE:
            /*
             * dodivnum - return the number of
             * current output diversion
             */
            pbnum(oindex);
            break;

        case UNDFTYPE:
            /*
             * doundefine - undefine a previously
             * defined macro(s) or m4 keyword(s).
             */
            if (argc > 2)
                for (n = 2; n < argc; n++)
                    remhash(argv[n], ALL);
            break;

        case POPDTYPE:
            /*
             * dopopdef - remove the topmost
             * definitions of macro(s) or m4
             * keyword(s).
             */
            if (argc > 2)
                for (n = 2; n < argc; n++)
                    remhash(argv[n], TOP);
            break;

        case MKTMTYPE:
            /*
             * dotemp - create a temporary file
             */
            if (argc > 2) {
                int fd;
                char *temp;

                temp = xstrdup(argv[2]);

                fd = mkstemp(temp);
                if (fd == -1)
                    err(1,
                        "%s at line %lu: couldn't make temp file %s",
                        CURRENT_NAME, CURRENT_LINE, argv[2]);
                close(fd);
                pbstr(temp);
                free(temp);
            }
            break;

        case TRNLTYPE:
            /*
             * dotranslit - replace all characters in
             * the source string that appears in the
             * "from" string with the corresponding
             * characters in the "to" string.
             */
            if (argc > 3) {
                char temp[STRSPMAX + 1];
                if (argc > 4)
                    map(temp, argv[2], argv[3], argv[4]);
                else
                    map(temp, argv[2], argv[3], null);
                pbstr(temp);
            } else if (argc > 2)
                pbstr(argv[2]);
            break;

        case INDXTYPE:
            /*
             * doindex - find the index of the second
             * argument string in the first argument
             * string. -1 if not present.
             */
            pbnum((argc > 3) ? indx(argv[2], argv[3]) : -1);
            break;

        case ERRPTYPE:
            /*
             * doerrp - print the arguments to stderr
             * file
             */
            if (argc > 2) {
                for (n = 2; n < argc; n++)
                    fprintf(stderr, "%s ", argv[n]);
                fprintf(stderr, "\n");
            }
            break;

        case DNLNTYPE:
            /*
             * dodnl - eat-up-to and including
             * newline
             */
            while ((c = gpbc()) != '\n' && c != EOF);
            break;

        case M4WRTYPE:
            /*
             * dom4wrap - set up for
             * wrap-up/wind-down activity
             */
            m4wraps = (argc > 2) ? xstrdup(argv[2]) : null;
            break;

        case EXITTYPE:
            /*
             * doexit - immediate exit from m4.
             */
            killdiv();
            exit((argc > 2) ? atoi(argv[2]) : 0);
            break;

        case DEFNTYPE:
            if (argc > 2)
                for (n = 2; n < argc; n++)
                    dodefn(argv[n]);
            break;

        case INDIRTYPE: /* Indirect call */
            if (argc > 2)
                doindir(argv, argc);
            break;

        case BUILTINTYPE: /* Builtins only */
            if (argc > 2)
                dobuiltin(argv, argc);
            break;

        case PATSTYPE:
            if (argc > 2)
                dopatsubst(argv, argc);
            break;
        case REGEXPTYPE:
            if (argc > 2)
                doregexp(argv, argc);
            break;
        case LINETYPE:
            doprintlineno(infile + ilevel);
            break;
        case FILENAMETYPE:
            doprintfilename(infile + ilevel);
            break;
        case SELFTYPE:
            pbstr(rquote);
            pbstr(argv[1]);
            pbstr(lquote);
            break;
        default:
            errx(1, "%s at line %lu: eval: major botch.",
                 CURRENT_NAME, CURRENT_LINE);
            break;
    }
}

/*
 * expand_macro - user-defined macro expansion
 */
void
    expand_macro(argv, argc)
        const char *argv[];
int argc;
{
    const char *t;
    const char *p;
    int n;
    int argno;

    t = argv[0]; /* defn string as a whole */
    p = t;
    while (*p)
        p++;
    p--; /* last character of defn */
    while (p > t) {
        if (*(p - 1) != ARGFLAG)
            PUTBACK(*p);
        else {
            switch (*p) {
                case '#':
                    pbnum(argc - 2);
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if ((argno = *p - '0') < argc - 1)
                        pbstr(argv[argno + 1]);
                    break;
                case '*':
                    if (argc > 2) {
                        for (n = argc - 1; n > 2; n--) {
                            pbstr(argv[n]);
                            putback(COMMA);
                        }
                        pbstr(argv[2]);
                    }
                    break;
                case '@':
                    if (argc > 2) {
                        for (n = argc - 1; n > 2; n--) {
                            pbstr(rquote);
                            pbstr(argv[n]);
                            pbstr(lquote);
                            putback(COMMA);
                        }
                        pbstr(rquote);
                        pbstr(argv[2]);
                        pbstr(lquote);
                    }
                    break;
                default:
                    PUTBACK(*p);
                    PUTBACK('$');
                    break;
            }
            p--;
        }
        p--;
    }
    if (p == t) /* do last character */
        PUTBACK(*p);
}

/*
 * dodefine - install definition in the table
 */
void
    dodefine(name, defn)
        const char *name;
const char *defn;
{
    ndptr p;
    int n;

    if (!*name)
        errx(1, "%s at line %lu: null definition.", CURRENT_NAME,
             CURRENT_LINE);
    if ((p = lookup(name)) == nil)
        p = addent(name);
    else if (p->defn != null)
        free((char *)p->defn);
    if (strncmp(defn, BUILTIN_MARKER, sizeof(BUILTIN_MARKER) - 1) == 0) {
        n = builtin_type(defn + sizeof(BUILTIN_MARKER) - 1);
        if (n != -1) {
            p->type = n & TYPEMASK;
            if ((n & NOARGS) == 0)
                p->type |= NEEDARGS;
            p->defn = null;
            return;
        }
    }
    if (!*defn)
        p->defn = null;
    else
        p->defn = xstrdup(defn);
    p->type = MACRTYPE;
    if (STREQ(name, defn))
        p->type |= RECDEF;
}

/*
 * dodefn - push back a quoted definition of
 *      the given name.
 */
static void
    dodefn(name)
        const char *name;
{
    ndptr p;
    const char *real;

    if ((p = lookup(name)) != nil) {
        if (p->defn != null) {
            pbstr(rquote);
            pbstr(p->defn);
            pbstr(lquote);
        } else if ((real = builtin_realname(p->type)) != NULL) {
            pbstr(real);
            pbstr(BUILTIN_MARKER);
        }
    }
}

/*
 * dopushdef - install a definition in the hash table
 *      without removing a previous definition. Since
 *      each new entry is entered in *front* of the
 *      hash bucket, it hides a previous definition from
 *      lookup.
 */
static void
    dopushdef(name, defn)
        const char *name;
const char *defn;
{
    ndptr p;

    if (!*name)
        errx(1, "%s at line %lu: null definition", CURRENT_NAME,
             CURRENT_LINE);
    p = addent(name);
    if (!*defn)
        p->defn = null;
    else
        p->defn = xstrdup(defn);
    p->type = MACRTYPE;
    if (STREQ(name, defn))
        p->type |= RECDEF;
}

/*
 * dump_one_def - dump the specified definition.
 */
static void
    dump_one_def(p)
        ndptr p;
{
    FILE *out = traceout ? traceout : stderr;
    const char *real;

    if (mimic_gnu) {
        if ((p->type & TYPEMASK) == MACRTYPE)
            fprintf(out, "%s:\t%s\n", p->name, p->defn);
        else {
            real = builtin_realname(p->type);
            if (real == NULL)
                real = null;
            fprintf(out, "%s:\t<%s>\n", p->name, real);
        }
    } else
        fprintf(out, "`%s'\t`%s'\n", p->name, p->defn);
}

/*
 * dodumpdef - dump the specified definitions in the hash
 *      table to stderr. If nothing is specified, the entire
 *      hash table is dumped.
 */
static void
    dodump(argv, argc)
        const char *argv[];
int argc;
{
    int n;
    ndptr p;

    if (argc > 2) {
        for (n = 2; n < argc; n++)
            if ((p = lookup(argv[n])) != nil)
                dump_one_def(p);
    } else {
        for (n = 0; n < HASHSIZE; n++)
            for (p = hashtab[n]; p != nil; p = p->nxtptr)
                dump_one_def(p);
    }
}

/*
 * dotrace - mark some macros as traced/untraced depending upon on.
 */
static void
    dotrace(argv, argc, on)
        const char *argv[];
int argc;
int on;
{
    int n;

    if (argc > 2) {
        for (n = 2; n < argc; n++)
            mark_traced(argv[n], on);
    } else
        mark_traced(NULL, on);
}

/*
 * doifelse - select one of two alternatives - loop.
 */
static void
    doifelse(argv, argc)
        const char *argv[];
int argc;
{
    cycle {
        if (STREQ(argv[2], argv[3]))
            pbstr(argv[4]);
        else if (argc == 6)
            pbstr(argv[5]);
        else if (argc > 6) {
            argv += 3;
            argc -= 3;
            continue;
        }
        break;
    }
}

/*
 * doinclude - include a given file.
 */
static int
    doincl(ifile)
        const char *ifile;
{
    if (ilevel + 1 == MAXINP)
        errx(1, "%s at line %lu: too many include files.",
             CURRENT_NAME, CURRENT_LINE);
    if (fopen_trypath(infile + ilevel + 1, ifile) != NULL) {
        ilevel++;
        bbase[ilevel] = bufbase = bp;
        return (1);
    } else
        return (0);
}

#ifdef EXTENDED
/*
 * dopaste - include a given file without any
 *           macro processing.
 */
static int
    dopaste(pfile)
        const char *pfile;
{
    FILE *pf;
    int c;

    if ((pf = fopen(pfile, "r")) != NULL) {
        while ((c = getc(pf)) != EOF)
            putc(c, active);
        (void)fclose(pf);
        return (1);
    } else
        return (0);
}
#endif

static void
    gnu_dochq(argv, ac)
        const char *argv[];
int ac;
{
    /* In gnu-m4 mode, the only way to restore quotes is to have no
     * arguments at all. */
    if (ac == 2) {
        lquote[0] = LQUOTE, lquote[1] = EOS;
        rquote[0] = RQUOTE, rquote[1] = EOS;
    } else {
        strlcpy(lquote, argv[2], sizeof(lquote));
        if (ac > 3)
            strlcpy(rquote, argv[3], sizeof(rquote));
        else
            rquote[0] = EOS;
    }
}

/*
 * dochq - change quote characters
 */
static void
    dochq(argv, argc)
        const char *argv[];
int argc;
{
    if (argc > 2) {
        if (*argv[2])
            strlcpy(lquote, argv[2], sizeof(lquote));
        else {
            lquote[0] = LQUOTE;
            lquote[1] = EOS;
        }
        if (argc > 3) {
            if (*argv[3])
                strlcpy(rquote, argv[3], sizeof(rquote));
        } else
            strcpy(rquote, lquote);
    } else {
        lquote[0] = LQUOTE, lquote[1] = EOS;
        rquote[0] = RQUOTE, rquote[1] = EOS;
    }
}

static void
    gnu_dochc(argv, ac)
        const char *argv[];
int ac;
{
    /* In gnu-m4 mode, no arguments mean no comment
     * arguments at all. */
    if (ac == 2) {
        scommt[0] = EOS;
        ecommt[0] = EOS;
    } else {
        if (*argv[2])
            strlcpy(scommt, argv[2], sizeof(scommt));
        else
            scommt[0] = SCOMMT, scommt[1] = EOS;
        if (ac > 3 && *argv[3])
            strlcpy(ecommt, argv[3], sizeof(ecommt));
        else
            ecommt[0] = ECOMMT, ecommt[1] = EOS;
    }
}
/*
 * dochc - change comment characters
 */
static void
    dochc(argv, argc)
        const char *argv[];
int argc;
{
    if (argc > 2) {
        if (*argv[2])
            strlcpy(scommt, argv[2], sizeof(scommt));
        if (argc > 3) {
            if (*argv[3])
                strlcpy(ecommt, argv[3], sizeof(ecommt));
        } else
            ecommt[0] = ECOMMT, ecommt[1] = EOS;
    } else {
        scommt[0] = SCOMMT, scommt[1] = EOS;
        ecommt[0] = ECOMMT, ecommt[1] = EOS;
    }
}

/*
 * dodivert - divert the output to a temporary file
 */
static void
    dodiv(n) int n;
{
    int fd;

    oindex = n;
    if (n >= maxout) {
        if (mimic_gnu)
            resizedivs(n + 10);
        else
            n = 0; /* bitbucket */
    }

    if (n < 0)
        n = 0; /* bitbucket */
    if (outfile[n] == NULL) {
        char fname[] = _PATH_DIVNAME;

        if ((fd = mkstemp(fname)) < 0 ||
            (outfile[n] = fdopen(fd, "w+")) == NULL)
            err(1, "%s: cannot divert", fname);
        if (unlink(fname) == -1)
            err(1, "%s: cannot unlink", fname);
    }
    active = outfile[n];
}

/*
 * doundivert - undivert a specified output, or all
 *              other outputs, in numerical order.
 */
static void
    doundiv(argv, argc)
        const char *argv[];
int argc;
{
    int ind;
    int n;

    if (argc > 2) {
        for (ind = 2; ind < argc; ind++) {
            n = atoi(argv[ind]);
            if (n > 0 && n < maxout && outfile[n] != NULL)
                getdiv(n);
        }
    } else
        for (n = 1; n < maxout; n++)
            if (outfile[n] != NULL)
                getdiv(n);
}

/*
 * dosub - select substring
 */
static void
    dosub(argv, argc)
        const char *argv[];
int argc;
{
    const char *ap, *fc, *k;
    int nc;

    ap = argv[2]; /* target string */
#ifdef EXPR
    fc = ap + expr(argv[3]); /* first char */
#else
    fc = ap + atoi(argv[3]); /* first char */
#endif
    nc = strlen(fc);
    if (argc >= 5)
#ifdef EXPR
        nc = min(nc, expr(argv[4]));
#else
        nc = min(nc, atoi(argv[4]));
#endif
    if (fc >= ap && fc < ap + strlen(ap))
        for (k = fc + nc - 1; k >= fc; k--)
            putback(*k);
}

/*
 * map:
 * map every character of s1 that is specified in from
 * into s3 and replace in s. (source s1 remains untouched)
 *
 * This is a standard implementation of map(s,from,to) function of ICON
 * language. Within mapvec, we replace every character of "from" with
 * the corresponding character in "to". If "to" is shorter than "from",
 * than the corresponding entries are null, which means that those
 * characters dissapear altogether. Furthermore, imagine
 * map(dest, "sourcestring", "srtin", "rn..*") type call. In this case,
 * `s' maps to `r', `r' maps to `n' and `n' maps to `*'. Thus, `s'
 * ultimately maps to `*'. In order to achieve this effect in an efficient
 * manner (i.e. without multiple passes over the destination string), we
 * loop over mapvec, starting with the initial source character. if the
 * character value (dch) in this location is different than the source
 * character (sch), sch becomes dch, once again to index into mapvec, until
 * the character value stabilizes (i.e. sch = dch, in other words
 * mapvec[n] == n). Even if the entry in the mapvec is null for an ordinary
 * character, it will stabilize, since mapvec[0] == 0 at all times. At the
 * end, we restore mapvec* back to normal where mapvec[n] == n for
 * 0 <= n <= 127. This strategy, along with the restoration of mapvec, is
 * about 5 times faster than any algorithm that makes multiple passes over
 * destination string.
 */
static void
    map(dest, src, from, to) char *dest;
const char *src;
const char *from;
const char *to;
{
    const char *tmp;
    unsigned char sch, dch;
    static char frombis[257];
    static char tobis[257];
    static unsigned char mapvec[256] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
        53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
        87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
        103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
        116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
        129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
        142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154,
        155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
        168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
        181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193,
        194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
        207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
        220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
        233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245,
        246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

    if (*src) {
        if (mimic_gnu) {
            /*
             * expand character ranges on the fly
             */
            from = handledash(frombis, frombis + 256, from);
            to = handledash(tobis, tobis + 256, to);
        }
        tmp = from;
        /*
         * create a mapping between "from" and
         * "to"
         */
        while (*from)
            mapvec[(unsigned char)(*from++)] = (*to) ? (unsigned char)(*to++) : 0;

        while (*src) {
            sch = (unsigned char)(*src++);
            dch = mapvec[sch];
            while (dch != sch) {
                sch = dch;
                dch = mapvec[sch];
            }
            if ((*dest = (char)dch))
                dest++;
        }
        /*
         * restore all the changed characters
         */
        while (*tmp) {
            mapvec[(unsigned char)(*tmp)] = (unsigned char)(*tmp);
            tmp++;
        }
    }
    *dest = '\0';
}

/*
 * handledash:
 *  use buffer to copy the src string, expanding character ranges
 * on the way.
 */
static const char *
handledash(buffer, end, src)
char *buffer;
char *end;
const char *src;
{
    char *p;

    p = buffer;
    while (*src) {
        if (src[1] == '-' && src[2]) {
            unsigned char i;
            for (i = (unsigned char)src[0];
                 i <= (unsigned char)src[2]; i++) {
                *p++ = i;
                if (p == end) {
                    *p = '\0';
                    return buffer;
                }
            }
            src += 3;
        } else
            *p++ = *src++;
        if (p == end)
            break;
    }
    *p = '\0';
    return buffer;
}

/*
 *      expression evaluator: performs a standard recursive
 *      descent parse to evaluate any expression permissible
 *      within the following grammar:
 *
 *	expr	:	query EOS
 *	query	:	lor
 *		|	lor "?" query ":" query
 *	lor	:	land { "||" land }
 *	land	:	bor { "&&" bor }
 *	bor	:	xor { "|" xor }
 *	xor	:	band { "^" eqrel }
 *	band	:	eqrel { "&" eqrel }
 *	eqrel	:	nerel { ("==" | "!=") nerel }
 *	nerel	:	shift { ("<" | ">" | "<=" | ">=") shift }
 *	shift	:	primary { ("<<" | ">>") primary }
 *	primary	:	term { ("+" | "-") term }
 *	term	:	exp { ("*" | "/" | "%") exp }
 *	exp	:	unary { "**" unary }
 *	unary	:	factor
 *		|	("+" | "-" | "~" | "!") unary
 *	factor	:	constant
 *		|	"(" query ")"
 *	constant:	num
 *		|	"'" CHAR "'"
 *	num	:	DIGIT
 *		|	DIGIT num
 *
 *
 *      This expression evaluator is lifted from a public-domain
 *      C Pre-Processor included with the DECUS C Compiler distribution.
 *      It is hacked somewhat to be suitable for m4.
 *
 *      Originally by:  Mike Lutz
 *                      Bob Harper
 */

#define EQL 0
#define NEQ 1
#define LSS 2
#define LEQ 3
#define GTR 4
#define GEQ 5
#define OCTAL 8
#define DECIMAL 10
#define HEX 16

static const char *nxtch; /* Parser scan pointer */
static const char *where;

static int query(int);
static int lor(int);
static int land(int);
static int bor(int);
static int xor (int);
static int band(int);
static int eqrel(int);
static int nerel(int);
static int shift(int);
static int primary(int);
static int term(int);
static int m4_exp(int);
static int unary(int);
static int factor(int);
static int constant(int);
static int num(int);
static int skipws(void);
static void experr(const char *);

/*
 * For longjmp
 */
#include <setjmp.h>
static jmp_buf expjump;

/*
 * macros:
 *      ungetch - Put back the last character examined.
 *      getch   - return the next character from expr string.
 */
#define ungetch() nxtch--
#define getch() *nxtch++

int
    expr(expbuf)
        const char *expbuf;
{
    int rval;

    nxtch = expbuf;
    where = expbuf;
    if (setjmp(expjump) != 0)
        return FALSE;

    rval = query(1);
    if (skipws() == EOS)
        return rval;

    printf("m4: ill-formed expression.\n");
    return FALSE;
}

/*
 * query : lor | lor '?' query ':' query
 */
static int
query(int mayeval) {
    int bool, true_val, false_val;

    bool = lor(mayeval);
    if (skipws() != '?') {
        ungetch();
        return bool;
    }

    true_val = query(bool);
    if (skipws() != ':')
        experr("bad query: missing \":\"");

    false_val = query(!bool);
    return bool ? true_val : false_val;
}

/*
 * lor : land { '||' land }
 */
static int
lor(int mayeval) {
    int c, vl, vr;

    vl = land(mayeval);
    while ((c = skipws()) == '|') {
        if (getch() != '|') {
            ungetch();
            break;
        }
        if (vl != 0)
            mayeval = 0;
        vr = land(mayeval);
        vl = vl || vr;
    }

    ungetch();
    return vl;
}

/*
 * land : not { '&&' not }
 */
static int
land(int mayeval) {
    int c, vl, vr;

    vl = bor(mayeval);
    while ((c = skipws()) == '&') {
        if (getch() != '&') {
            ungetch();
            break;
        }
        if (vl == 0)
            mayeval = 0;
        vr = bor(mayeval);
        vl = vl && vr;
    }

    ungetch();
    return vl;
}

/*
 * bor : xor { "|" xor }
 */
static int
bor(int mayeval) {
    int vl, vr, c, cr;

    vl = xor(mayeval);
    while ((c = skipws()) == '|') {
        cr = getch();
        ungetch();
        if (cr == '|')
            break;
        vr = xor(mayeval);
        vl |= vr;
    }
    ungetch();
    return (vl);
}

/*
 * xor : band { "^" band }
 */
static int xor (int mayeval) {
    int vl, vr, c;

    vl = band(mayeval);
    while ((c = skipws()) == '^') {
        vr = band(mayeval);
        vl ^= vr;
    }
    ungetch();
    return (vl);
}

    /*
     * band : eqrel { "&" eqrel }
     */
    static int band(int mayeval) {
    int c, cr, vl, vr;

    vl = eqrel(mayeval);
    while ((c = skipws()) == '&') {
        cr = getch();
        ungetch();
        if (cr == '&')
            break;
        vr = eqrel(mayeval);
        vl &= vr;
    }
    ungetch();
    return vl;
}

/*
 * eqrel : nerel { ("==" | "!=" ) nerel }
 */
static int
eqrel(int mayeval) {
    int vl, vr, c, cr;

    vl = nerel(mayeval);
    while ((c = skipws()) == '!' || c == '=') {
        if ((cr = getch()) != '=') {
            ungetch();
            break;
        }
        vr = nerel(mayeval);
        switch (c) {
            case '=':
                vl = (vl == vr);
                break;
            case '!':
                vl = (vl != vr);
                break;
        }
    }
    ungetch();
    return vl;
}

/*
 * nerel : shift { ("<=" | ">=" | "<" | ">") shift }
 */
static int
nerel(int mayeval) {
    int vl, vr, c, cr;

    vl = shift(mayeval);
    while ((c = skipws()) == '<' || c == '>') {
        if ((cr = getch()) != '=') {
            ungetch();
            cr = '\0';
        }
        vr = shift(mayeval);
        switch (c) {
            case '<':
                vl = (cr == '\0') ? (vl < vr) : (vl <= vr);
                break;
            case '>':
                vl = (cr == '\0') ? (vl > vr) : (vl >= vr);
                break;
        }
    }
    ungetch();
    return vl;
}

/*
 * shift : primary { ("<<" | ">>") primary }
 */
static int
shift(int mayeval) {
    int vl, vr, c;

    vl = primary(mayeval);
    while (((c = skipws()) == '<' || c == '>') && getch() == c) {
        vr = primary(mayeval);

        if (c == '<')
            vl <<= vr;
        else
            vl >>= vr;
    }

    if (c == '<' || c == '>')
        ungetch();
    ungetch();
    return vl;
}

/*
 * primary : term { ("+" | "-") term }
 */
static int
primary(int mayeval) {
    int c, vl, vr;

    vl = term(mayeval);
    while ((c = skipws()) == '+' || c == '-') {
        vr = term(mayeval);

        if (c == '+')
            vl += vr;
        else
            vl -= vr;
    }

    ungetch();
    return vl;
}

/*
 * term : exp { ("*" | "/" | "%") exp }
 */
static int
term(int mayeval) {
    int c, vl, vr;

    vl = m4_exp(mayeval);
    while ((c = skipws()) == '*' || c == '/' || c == '%') {
        vr = m4_exp(mayeval);

        switch (c) {
            case '*':
                vl *= vr;
                break;
            case '/':
                if (!mayeval)
                    /* short-circuit */;
                else if (vr == 0)
                    errx(1, "division by zero in eval.");
                else
                    vl /= vr;
                break;
            case '%':
                if (!mayeval)
                    /* short-circuit */;
                else if (vr == 0)
                    errx(1, "modulo zero in eval.");
                else
                    vl %= vr;
                break;
        }
    }
    ungetch();
    return vl;
}

/*
 * exp : unary { "**" exp }
 */
static int
m4_exp(int mayeval) {
    int c, vl, vr, n;

    vl = unary(mayeval);
    while ((c = skipws()) == '*') {
        if (getch() != '*') {
            ungetch();
            break;
        }
        vr = unary(mayeval);
        n = 1;
        while (vr-- > 0)
            n *= vl;
        return n;
    }

    ungetch();
    return vl;
}

/*
 * unary : factor | ("+" | "-" | "~" | "!") unary
 */
static int
unary(int mayeval) {
    int val, c;

    if ((c = skipws()) == '+' || c == '-' || c == '~' || c == '!') {
        val = unary(mayeval);

        switch (c) {
            case '+':
                return val;
            case '-':
                return -val;
            case '~':
                return ~val;
            case '!':
                return !val;
        }
    }

    ungetch();
    return factor(mayeval);
}

/*
 * factor : constant | '(' query ')'
 */
static int
factor(int mayeval) {
    int val;

    if (skipws() == '(') {
        val = query(mayeval);
        if (skipws() != ')')
            experr("bad factor: missing \")\"");
        return val;
    }

    ungetch();
    return constant(mayeval);
}

/*
 * constant: num | 'char'
 * Note: constant() handles multi-byte constants
 */
static int
constant(int mayeval) {
    int i;
    int value;
    char c;
    int v[sizeof(int)];

    if (skipws() != '\'') {
        ungetch();
        return num(mayeval);
    }
    for (i = 0; i < sizeof(int); i++) {
        if ((c = getch()) == '\'') {
            ungetch();
            break;
        }
        if (c == '\\') {
            switch (c = getch()) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    ungetch();
                    c = num(mayeval);
                    break;
                case 'n':
                    c = 012;
                    break;
                case 'r':
                    c = 015;
                    break;
                case 't':
                    c = 011;
                    break;
                case 'b':
                    c = 010;
                    break;
                case 'f':
                    c = 014;
                    break;
            }
        }
        v[i] = c;
    }
    if (i == 0 || getch() != '\'')
        experr("illegal character constant");
    for (value = 0; --i >= 0;) {
        value <<= 8;
        value += v[i];
    }
    return value;
}

/*
 * num : digit | num digit
 */
static int
num(int mayeval) {
    int rval, c, base;
    int ndig;

    base = ((c = skipws()) == '0') ? OCTAL : DECIMAL;
    rval = 0;
    ndig = 0;
    if (base == OCTAL) {
        c = skipws();
        if (c == 'x' || c == 'X') {
            base = HEX;
            c = skipws();
        } else
            ndig++;
    }
    while ((base == HEX && isxdigit(c)) ||
           (c >= '0' && c <= (base == OCTAL ? '7' : '9'))) {
        rval *= base;
        if (isalpha(c))
            rval += (tolower(c) - 'a' + 10);
        else
            rval += (c - '0');
        c = getch();
        ndig++;
    }
    ungetch();

    if (ndig == 0)
        experr("bad constant");

    return rval;
}

/*
 * Skip over any white space and return terminating char.
 */
static int
skipws() {
    char c;

    while ((c = getch()) <= ' ' && c > EOS);
    return c;
}

/*
 * resets environment to eval(), prints an error
 * and forces eval to return FALSE.
 */
static void
    experr(msg)
        const char *msg;
{
    printf("m4: %s in expr %s.\n", msg, where);
    longjmp(expjump, -1);
}

int mimic_gnu = 0;

/*
 * Support for include path search
 * First search in the current directory.
 * If not found, and the path is not absolute, include path kicks in.
 * First, -I options, in the order found on the command line.
 * Then M4PATH env variable
 */

struct path_entry {
    char *name;
    struct path_entry *next;
} *first, *last;

static struct path_entry *new_path_entry(const char *);
static void ensure_m4path(void);
static struct input_file *dopath(struct input_file *, const char *);

static struct path_entry *
    new_path_entry(dirname)
        const char *dirname;
{
    struct path_entry *n;

    n = malloc(sizeof(struct path_entry));
    if (!n)
        errx(1, "out of memory");
    n->name = strdup(dirname);
    if (!n->name)
        errx(1, "out of memory");
    n->next = 0;
    return n;
}

void
    addtoincludepath(dirname)
        const char *dirname;
{
    struct path_entry *n;

    n = new_path_entry(dirname);

    if (last) {
        last->next = n;
        last = n;
    } else
        last = first = n;
}

static void
ensure_m4path() {
    static int envpathdone = 0;
    char *envpath;
    char *sweep;
    char *path;

    if (envpathdone)
        return;
    envpathdone = TRUE;
    envpath = getenv("M4PATH");
    if (!envpath)
        return;
    /* for portability: getenv result is read-only */
    envpath = strdup(envpath);
    if (!envpath)
        errx(1, "out of memory");
    for (sweep = envpath;
         (path = strsep(&sweep, ":")) != NULL;)
        addtoincludepath(path);
    free(envpath);
}

static struct input_file *
dopath(i, filename)
struct input_file *i;
const char *filename;
{
    char path[MAXPATHLEN];
    struct path_entry *pe;
    FILE *f;

    for (pe = first; pe; pe = pe->next) {
        snprintf(path, sizeof(path), "%s/%s", pe->name, filename);
        if ((f = fopen(path, "r")) != 0) {
            set_input(i, f, path);
            return i;
        }
    }
    return NULL;
}

struct input_file *
fopen_trypath(i, filename)
struct input_file *i;
const char *filename;
{
    FILE *f;

    f = fopen(filename, "r");
    if (f != NULL) {
        set_input(i, f, filename);
        return i;
    }
    if (filename[0] == '/')
        return NULL;

    ensure_m4path();

    return dopath(i, filename);
}

void
    doindir(argv, argc)
        const char *argv[];
int argc;
{
    ndptr p;

    p = lookup(argv[2]);
    if (p == NULL)
        errx(1, "undefined macro %s", argv[2]);
    argv[1] = p->defn;
    eval(argv + 1, argc - 1, p->type);
}

void
    dobuiltin(argv, argc)
        const char *argv[];
int argc;
{
    int n;
    argv[1] = NULL;
    n = builtin_type(argv[2]);
    if (n != -1)
        eval(argv + 1, argc - 1, n);
    else
        errx(1, "unknown builtin %s", argv[2]);
}

/* We need some temporary buffer space, as pb pushes BACK and substitution
 * proceeds forward... */
static char *buffer;
static size_t bufsize = 0;
static size_t current = 0;

static void addchars(const char *, size_t);
static void addchar(char);
static char *twiddle(const char *);
static char *getstring(void);
static void exit_regerror(int, regex_t *);
static void do_subst(const char *, regex_t *, const char *, regmatch_t *);
static void do_regexpindex(const char *, regex_t *, regmatch_t *);
static void do_regexp(const char *, regex_t *, const char *, regmatch_t *);
static void add_sub(int, const char *, regex_t *, regmatch_t *);
static void add_replace(const char *, regex_t *, const char *, regmatch_t *);
#define addconstantstring(s) addchars((s), sizeof(s) - 1)

static void
    addchars(c, n)
        const char *c;
size_t n;
{
    if (n == 0)
        return;
    while (current + n > bufsize) {
        if (bufsize == 0)
            bufsize = 1024;
        else
            bufsize *= 2;
        buffer = realloc(buffer, bufsize);
        if (buffer == NULL)
            errx(1, "out of memory");
    }
    memcpy(buffer + current, c, n);
    current += n;
}

static void
    addchar(c) char c;
{
    if (current + 1 > bufsize) {
        if (bufsize == 0)
            bufsize = 1024;
        else
            bufsize *= 2;
        buffer = realloc(buffer, bufsize);
        if (buffer == NULL)
            errx(1, "out of memory");
    }
    buffer[current++] = c;
}

static char *
getstring() {
    addchar('\0');
    current = 0;
    return buffer;
}

static void
    exit_regerror(er, re) int er;
regex_t *re;
{
    size_t errlen;
    char *errbuf;

    errlen = regerror(er, re, NULL, 0);
    errbuf = xalloc(errlen);
    regerror(er, re, errbuf, errlen);
    errx(1, "regular expression error: %s", errbuf);
}

static void
    add_sub(n, string, re, pm) int n;
const char *string;
regex_t *re;
regmatch_t *pm;
{
    if (n > re->re_nsub)
        warnx("No subexpression %d", n);
    /* Subexpressions that did not match are
     * not an error.  */
    else if (pm[n].rm_so != -1 &&
             pm[n].rm_eo != -1) {
        addchars(string + pm[n].rm_so,
                 pm[n].rm_eo - pm[n].rm_so);
    }
}

/* Add replacement string to the output buffer, recognizing special
 * constructs and replacing them with substrings of the original string.
 */
static void
    add_replace(string, re, replace, pm)
        const char *string;
regex_t *re;
const char *replace;
regmatch_t *pm;
{
    const char *p;

    for (p = replace; *p != '\0'; p++) {
        if (*p == '&' && !mimic_gnu) {
            add_sub(0, string, re, pm);
            continue;
        }
        if (*p == '\\') {
            if (p[1] == '\\') {
                addchar(p[1]);
                p++;
                continue;
            }
            if (p[1] == '&') {
                if (mimic_gnu)
                    add_sub(0, string, re, pm);
                else
                    addchar(p[1]);
                p++;
                continue;
            }
            if (isdigit((unsigned char)p[1])) {
                add_sub(*(++p) - '0', string, re, pm);
                continue;
            }
        }
        addchar(*p);
    }
}

static void
    do_subst(string, re, replace, pm)
        const char *string;
regex_t *re;
const char *replace;
regmatch_t *pm;
{
    int error;
    int flags = 0;
    const char *last_match = NULL;

    while ((error = regexec(re, string, re->re_nsub + 1, pm, flags)) == 0) {
        if (pm[0].rm_eo != 0) {
            if (string[pm[0].rm_eo - 1] == '\n')
                flags = 0;
            else
                flags = REG_NOTBOL;
        }

        /* NULL length matches are special... We use the `vi-mode'
         * rule: don't allow a NULL-match at the last match
         * position.
         */
        if (pm[0].rm_so == pm[0].rm_eo &&
            string + pm[0].rm_so == last_match) {
            if (*string == '\0')
                return;
            addchar(*string);
            if (*string++ == '\n')
                flags = 0;
            else
                flags = REG_NOTBOL;
            continue;
        }
        last_match = string + pm[0].rm_so;
        addchars(string, pm[0].rm_so);
        add_replace(string, re, replace, pm);
        string += pm[0].rm_eo;
    }
    if (error != REG_NOMATCH)
        exit_regerror(error, re);
    pbstr(string);
}

static void
    do_regexp(string, re, replace, pm)
        const char *string;
regex_t *re;
const char *replace;
regmatch_t *pm;
{
    int error;

    switch (error = regexec(re, string, re->re_nsub + 1, pm, 0)) {
        case 0:
            add_replace(string, re, replace, pm);
            pbstr(getstring());
            break;
        case REG_NOMATCH:
            break;
        default:
            exit_regerror(error, re);
    }
}

static void
    do_regexpindex(string, re, pm)
        const char *string;
regex_t *re;
regmatch_t *pm;
{
    int error;

    switch (error = regexec(re, string, re->re_nsub + 1, pm, 0)) {
        case 0:
            pbunsigned(pm[0].rm_so);
            break;
        case REG_NOMATCH:
            pbnum(-1);
            break;
        default:
            exit_regerror(error, re);
    }
}

/* In Gnu m4 mode, parentheses for backmatch don't work like POSIX 1003.2
 * says. So we twiddle with the regexp before passing it to regcomp.
 */
static char *
    twiddle(p)
        const char *p;
{
    /* This could use strcspn for speed... */
    while (*p != '\0') {
        if (*p == '\\') {
            switch (p[1]) {
                case '(':
                case ')':
                case '|':
                    addchar(p[1]);
                    break;
                case 'w':
                    addconstantstring("[_a-zA-Z0-9]");
                    break;
                case 'W':
                    addconstantstring("[^_a-zA-Z0-9]");
                    break;
                case '<':
                    addconstantstring("[[:<:]]");
                    break;
                case '>':
                    addconstantstring("[[:>:]]");
                    break;
                default:
                    addchars(p, 2);
                    break;
            }
            p += 2;
            continue;
        }
        if (*p == '(' || *p == ')' || *p == '|')
            addchar('\\');

        addchar(*p);
        p++;
    }
    return getstring();
}

/* patsubst(string, regexp, opt replacement) */
/* argv[2]: string
 * argv[3]: regexp
 * argv[4]: opt rep
 */
void
    dopatsubst(argv, argc)
        const char *argv[];
int argc;
{
    int error;
    regex_t re;
    regmatch_t *pmatch;

    if (argc <= 3) {
        warnx("Too few arguments to patsubst");
        return;
    }
    error = regcomp(&re, mimic_gnu ? twiddle(argv[3]) : argv[3],
                    REG_NEWLINE | REG_EXTENDED);
    if (error != 0)
        exit_regerror(error, &re);

    pmatch = xalloc(sizeof(regmatch_t) * (re.re_nsub + 1));
    do_subst(argv[2], &re,
             argc != 4 && argv[4] != NULL ? argv[4] : "", pmatch);
    pbstr(getstring());
    free(pmatch);
    regfree(&re);
}

void
    doregexp(argv, argc)
        const char *argv[];
int argc;
{
    int error;
    regex_t re;
    regmatch_t *pmatch;

    if (argc <= 3) {
        warnx("Too few arguments to regexp");
        return;
    }
    error = regcomp(&re, mimic_gnu ? twiddle(argv[3]) : argv[3],
                    REG_EXTENDED);
    if (error != 0)
        exit_regerror(error, &re);

    pmatch = xalloc(sizeof(regmatch_t) * (re.re_nsub + 1));
    if (argv[4] == NULL || argc == 4)
        do_regexpindex(argv[2], &re, pmatch);
    else
        do_regexp(argv[2], &re, argv[4], pmatch);
    free(pmatch);
    regfree(&re);
}

void
    doesyscmd(cmd)
        const char *cmd;
{
    int p[2];
    pid_t pid, cpid;
    char *argv[4];
    int cc;
    int status;

    /* Follow gnu m4 documentation: first flush buffers. */
    fflush(NULL);

    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = (char *)cmd;
    argv[3] = NULL;

    /* Just set up standard output, share stderr and stdin with m4 */
    if (pipe(p) == -1)
        err(1, "bad pipe");
    switch (cpid = fork()) {
        case -1:
            err(1, "bad fork");
            /* NOTREACHED */
        case 0:
            (void)close(p[0]);
            (void)dup2(p[1], 1);
            (void)close(p[1]);
            execv(_PATH_BSHELL, argv);
            exit(1);
        default:
            /* Read result in two stages, since m4's buffer is
             * pushback-only. */
            (void)close(p[1]);
            do {
                char result[BUFSIZE];
                cc = read(p[0], result, sizeof result);
                if (cc > 0)
                    addchars(result, cc);
            } while (cc > 0 || (cc == -1 && errno == EINTR));

            (void)close(p[0]);
            while ((pid = wait(&status)) != cpid && pid >= 0)
                continue;
            pbstr(getstring());
    }
}

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

/* #include <sys/cdefs.h> */
#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)look.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: look.c,v 1.10 2004/06/20 22:20:15 jmc Exp $");
#endif
#endif /* not lint */

/*
 * look.c
 * Facility: m4 macro processor
 * by: oz
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void freent(ndptr);

unsigned
    hash(name)
        const char *name;
{
    unsigned int h = 0;
    while (*name)
        h = (h << 5) + h + *name++;
    return (h);
}

/*
 * find name in the hash table
 */
ndptr
    lookup(name)
        const char *name;
{
    ndptr p;
    unsigned int h;

    h = hash(name);
    for (p = hashtab[h % HASHSIZE]; p != nil; p = p->nxtptr)
        if (h == p->hv && STREQ(name, p->name))
            break;
    return (p);
}

/*
 * hash and create an entry in the hash table.
 * The new entry is added in front of a hash bucket.
 */
ndptr
    addent(name)
        const char *name;
{
    unsigned int h;
    ndptr p;

    h = hash(name);
    p = (ndptr)xalloc(sizeof(struct ndblock));
    p->nxtptr = hashtab[h % HASHSIZE];
    hashtab[h % HASHSIZE] = p;
    p->name = xstrdup(name);
    p->hv = h;
    return p;
}

static void
    freent(p)
        ndptr p;
{
    free((char *)p->name);
    if (p->defn != null)
        free((char *)p->defn);
    free((char *)p);
}

/*
 * remove an entry from the hashtable
 */
void
    remhash(name, all)
        const char *name;
int all;
{
    unsigned int h;
    ndptr xp, tp, mp;

    h = hash(name);
    mp = hashtab[h % HASHSIZE];
    tp = nil;
    while (mp != nil) {
        if (mp->hv == h && STREQ(mp->name, name)) {
            mp = mp->nxtptr;
            if (tp == nil) {
                freent(hashtab[h % HASHSIZE]);
                hashtab[h % HASHSIZE] = mp;
            } else {
                xp = tp->nxtptr;
                tp->nxtptr = mp;
                freent(xp);
            }
            if (!all)
                break;
        } else {
            tp = mp;
            mp = mp->nxtptr;
        }
    }
}

char *ep;              /* first free char in strspace */
static char *strspace; /* string space for evaluation */
char *endest;          /* end of string space	       */
static size_t strsize = STRSPMAX;
static size_t bufsize2 = BUFSIZE;

char *buf;           /* push-back buffer	       */
char *bufbase;       /* the base for current ilevel */
char *bbase[MAXINP]; /* the base for each ilevel    */
char *bp;            /* first available character   */
char *endpbb;        /* end of push-back buffer     */

/*
 * find the index of second str in the first str.
 */
ptrdiff_t
    indx(s1, s2)
        const char *s1;
const char *s2;
{
    char *t;

    t = strstr(s1, s2);
    if (t == NULL)
        return (-1);
    return (t - s1);
}

/*
 *  putback - push character back onto input
 */
void
    putback(c) int c;
{
    if (c == EOF)
        return;
    if (bp >= endpbb)
        enlarge_bufspace();
    *bp++ = c;
}

/*
 *  pbstr - push string back onto input
 *          putback is replicated to improve
 *          performance.
 */
void
    pbstr(s)
        const char *s;
{
    size_t n;

    n = strlen(s);
    while (endpbb - bp <= n)
        enlarge_bufspace();
    while (n > 0)
        *bp++ = s[--n];
}

/*
 *  pbnum - convert number to string, push back on input.
 */
void
    pbnum(n) int n;
{
    int num;

    num = (n < 0) ? -n : n;
    do {
        putback(num % 10 + '0');
    } while ((num /= 10) > 0);

    if (n < 0)
        putback('-');
}

/*
 *  pbunsigned - convert unsigned long to string, push back on input.
 */
void
    pbunsigned(n) unsigned long n;
{
    do {
        putback(n % 10 + '0');
    } while ((n /= 10) > 0);
}

void initspaces() {
    int i;

    strspace = xalloc(strsize + 1);
    ep = strspace;
    endest = strspace + strsize;
    buf = (char *)xalloc(bufsize2);
    bufbase = buf;
    bp = buf;
    endpbb = buf + bufsize2;
    for (i = 0; i < MAXINP; i++)
        bbase[i] = buf;
}

void enlarge_strspace() {
    char *newstrspace;
    int i;

    strsize *= 2;
    newstrspace = malloc(strsize + 1);
    if (!newstrspace)
        errx(1, "string space overflow");
    memcpy(newstrspace, strspace, strsize / 2);
    for (i = 0; i <= sp; i++)
        if (sstack[i])
            mstack[i].sstr = (mstack[i].sstr - strspace) + newstrspace;
    ep = (ep - strspace) + newstrspace;
    free(strspace);
    strspace = newstrspace;
    endest = strspace + strsize;
}

void enlarge_bufspace() {
    char *newbuf;
    int i;

    bufsize2 *= 2;
    newbuf = realloc(buf, bufsize2);
    if (!newbuf)
        errx(1, "too many characters pushed back");
    for (i = 0; i < MAXINP; i++)
        bbase[i] = (bbase[i] - buf) + newbuf;
    bp = (bp - buf) + newbuf;
    bufbase = (bufbase - buf) + newbuf;
    buf = newbuf;
    endpbb = buf + bufsize2;
}

/*
 *  chrsave - put single char on string space
 */
void
    chrsave(c) int c;
{
    if (ep >= endest)
        enlarge_strspace();
    *ep++ = c;
}

/*
 * read in a diversion file, and dispose it.
 */
void
    getdiv(n) int n;
{
    int c;

    if (active == outfile[n])
        errx(1, "undivert: diversion still active");
    rewind(outfile[n]);
    while ((c = getc(outfile[n])) != EOF)
        putc(c, active);
    (void)fclose(outfile[n]);
    outfile[n] = NULL;
}

void
    onintr(signo) int signo;
{
#define intrmessage "m4: interrupted.\n"
    write(STDERR_FILENO, intrmessage, sizeof(intrmessage));
    _exit(1);
}

/*
 * killdiv - get rid of the diversion files
 */
void killdiv() {
    int n;

    for (n = 0; n < maxout; n++)
        if (outfile[n] != NULL) {
            (void)fclose(outfile[n]);
        }
}

/*
 * resizedivs: allocate more diversion files */
void
    resizedivs(n) int n;
{
    int i;

    outfile = (FILE **)realloc(outfile, sizeof(FILE *) * n);
    if (outfile == NULL)
        errx(1, "too many diverts %d", n);
    for (i = maxout; i < n; i++)
        outfile[i] = NULL;
    maxout = n;
}

void *
xalloc(n)
size_t n;
{
    char *p = malloc(n);

    if (p == NULL)
        err(1, "malloc");
    return p;
}

char *
    xstrdup(s)
        const char *s;
{
    char *p = strdup(s);
    if (p == NULL)
        err(1, "strdup");
    return p;
}

void
    usage(progname)
        const char *progname;
{
    fprintf(stderr, "usage: %s [-Pg]: \t\t\tGNU --prefix-builtins \n [-Dname[=val]] [-I dirname]: \tSame as cc \n [-Uname]: \t\t\t\tundefine name\n", progname);
    fprintf(stderr, " [-d flags] [-o trfile] [-t macro]\n");
    exit(1);
}

int obtain_char(f)
struct input_file *f;
{
    if (f->c == EOF)
        return EOF;
    else if (f->c == '\n')
        f->lineno++;

    f->c = fgetc(f->file);
    return f->c;
}

void
    set_input(f, real, name) struct input_file *f;
FILE *real;
const char *name;
{
    f->file = real;
    f->lineno = 1;
    f->c = 0;
    f->name = xstrdup(name);
}

void
    release_input(f) struct input_file *f;
{
    if (f->file != stdin)
        fclose(f->file);
    f->c = EOF;
    /*
     * XXX can't free filename, as there might still be
     * error information pointing to it.
     */
}

void
    doprintlineno(f) struct input_file *f;
{
    pbunsigned(f->lineno);
}

void
    doprintfilename(f) struct input_file *f;
{
    pbstr(rquote);
    pbstr(f->name);
    pbstr(lquote);
}

/*
 * buffer_mark/dump_buffer: allows one to save a mark in a buffer,
 * and later dump everything that was added since then to a file.
 */
size_t
buffer_mark() {
    return bp - buf;
}

void
    dump_buffer(f, m)
        FILE *f;
size_t m;
{
    char *s;

    for (s = bp; s - buf > m;)
        fputc(*--s, f);
}

FILE *traceout;

int traced_macros = 0;

#define TRACE_ARGS 1
#define TRACE_EXPANSION 2
#define TRACE_QUOTE 4
#define TRACE_FILENAME 8
#define TRACE_LINENO 16
#define TRACE_CONT 32
#define TRACE_ID 64
#define TRACE_NEWFILE 128 /* not implemented yet */
#define TRACE_INPUT 256   /* not implemented yet */
#define TRACE_ALL 512

static struct t {
    struct t *next;
    char *name;
    int on;
} *l;

static unsigned int letter_to_flag(int);
static void print_header(struct input_file *);
static struct t *find_trace_entry(const char *);
static int frame_level(void);

static unsigned int flags = TRACE_QUOTE | TRACE_EXPANSION;

static struct t *
    find_trace_entry(name)
        const char *name;
{
    struct t *n;

    for (n = l; n != NULL; n = n->next)
        if (STREQ(n->name, name))
            return n;
    return NULL;
}

void
    mark_traced(name, on)
        const char *name;
int on;
{
    struct t *n, *n2;

    traced_macros = 1;

    if (name == NULL) {
        if (on)
            flags |= TRACE_ALL;
        else {
            flags &= ~TRACE_ALL;
            traced_macros = 0;
        }
        for (n = l; n != NULL; n = n2) {
            n2 = n->next;
            free(n->name);
            free(n);
        }
        l = NULL;
    } else {
        n = find_trace_entry(name);
        if (n == NULL) {
            n = xalloc(sizeof(struct t));
            n->name = xstrdup(name);
            n->next = l;
            l = n;
        }
        n->on = on;
    }
}

int
    is_traced(name)
        const char *name;
{
    struct t *n;

    for (n = l; n != NULL; n = n->next)
        if (STREQ(n->name, name))
            return n->on;
    return (flags & TRACE_ALL) ? 1 : 0;
}

void
    trace_file(name)
        const char *name;
{
    if (traceout)
        fclose(traceout);
    traceout = fopen(name, "w");
    if (!traceout)
        err(1, "can't open %s", name);
}

static unsigned int
letter_to_flag(c)
int c;
{
    switch (c) {
        case 'a':
            return TRACE_ARGS;
        case 'e':
            return TRACE_EXPANSION;
        case 'q':
            return TRACE_QUOTE;
        case 'c':
            return TRACE_CONT;
        case 'x':
            return TRACE_ID;
        case 'f':
            return TRACE_FILENAME;
        case 'l':
            return TRACE_LINENO;
        case 'p':
            return TRACE_NEWFILE;
        case 'i':
            return TRACE_INPUT;
        case 't':
            return TRACE_ALL;
        case 'V':
            return ~0;
        default:
            return 0;
    }
}

void
    set_trace_flags(s)
        const char *s;
{
    char mode = 0;
    unsigned int f = 0;

    traced_macros = 1;

    if (*s == '+' || *s == '-')
        mode = *s++;
    while (*s)
        f |= letter_to_flag(*s++);
    switch (mode) {
        case 0:
            flags = f;
            break;
        case '+':
            flags |= f;
            break;
        case '-':
            flags &= ~f;
            break;
    }
}

static int
frame_level() {
    int level;
    int framep;

    for (framep = fp, level = 0; framep != 0;
         level++, framep = mstack[framep - 2].sfra);
    return level;
}

static void
    print_header(inp) struct input_file *inp;
{
    FILE *out = traceout ? traceout : stderr;

    fprintf(out, "m4trace:");
    if (flags & TRACE_FILENAME)
        fprintf(out, "%s:", inp->name);
    if (flags & TRACE_LINENO)
        fprintf(out, "%lu:", inp->lineno);
    fprintf(out, " -%d- ", frame_level());
    if (flags & TRACE_ID)
        fprintf(out, "id %lu: ", expansion_id);
}

ssize_t
    trace(argv, argc, inp)
        const char **argv;
int argc;
struct input_file *inp;
{
    FILE *out = traceout ? traceout : stderr;

    print_header(inp);
    if (flags & TRACE_CONT) {
        fprintf(out, "%s ...\n", argv[1]);
        print_header(inp);
    }
    fprintf(out, "%s", argv[1]);
    if ((flags & TRACE_ARGS) && argc > 2) {
        char delim[3];
        int i;

        delim[0] = LPAREN;
        delim[1] = EOS;
        for (i = 2; i < argc; i++) {
            fprintf(out, "%s%s%s%s", delim,
                    (flags & TRACE_QUOTE) ? lquote : "",
                    argv[i],
                    (flags & TRACE_QUOTE) ? rquote : "");
            delim[0] = COMMA;
            delim[1] = ' ';
            delim[2] = EOS;
        }
        fprintf(out, "%c", RPAREN);
    }
    if (flags & TRACE_CONT) {
        fprintf(out, " -> ???\n");
        print_header(inp);
        fprintf(out, argc > 2 ? "%s(...)" : "%s", argv[1]);
    }
    if (flags & TRACE_EXPANSION)
        return buffer_mark();
    else {
        fprintf(out, "\n");
        return -1;
    }
}

void
    finish_trace(mark)
        size_t mark;
{
    FILE *out = traceout ? traceout : stderr;

    fprintf(out, " -> ");
    if (flags & TRACE_QUOTE)
        fprintf(out, "%s", lquote);
    dump_buffer(out, mark);
    if (flags & TRACE_QUOTE)
        fprintf(out, "%s", rquote);
    fprintf(out, "\n");
}

ndptr hashtab[HASHSIZE];          /* hash table for macros etc.  */
stae *mstack;                     /* stack of m4 machine         */
char *sstack;                     /* shadow stack, for string space extension */
static size_t STACKMAX;           /* current maximum size of stack */
int sp;                           /* current m4  stack pointer   */
int fp;                           /* m4 call frame pointer       */
struct input_file infile[MAXINP]; /* input file stack (0=stdin)  */
FILE **outfile;                   /* diversion array(0=bitbucket)*/
int maxout;
FILE *active;                          /* active output file pointer  */
int ilevel = 0;                        /* input file stack pointer    */
int oindex = 0;                        /* diversion index..	       */
char *null = "";                       /* as it says.. just a null..  */
char *m4wraps = "";                    /* m4wrap string default..     */
int m4prefix = 0;                      /* prefix keywords with m4_    */
char lquote[MAXCCHARS + 1] = {LQUOTE}; /* left quote character  (`)   */
char rquote[MAXCCHARS + 1] = {RQUOTE}; /* right quote character (')   */
char scommt[MAXCCHARS + 1] = {SCOMMT}; /* start character for comment */
char ecommt[MAXCCHARS + 1] = {ECOMMT}; /* end character for comment   */

struct keyblk keywrds[] = {
    /* m4 keywords to be installed */
    {"include", INCLTYPE},
    {"sinclude", SINCTYPE},
    {"define", DEFITYPE},
    {"defn", DEFNTYPE},
    {"divert", DIVRTYPE | NOARGS},
    {"expr", EXPRTYPE},
    {"eval", EXPRTYPE},
    {"substr", SUBSTYPE},
    {"ifelse", IFELTYPE},
    {"ifdef", IFDFTYPE},
    {"len", LENGTYPE},
    {"incr", INCRTYPE},
    {"decr", DECRTYPE},
    {"dnl", DNLNTYPE | NOARGS},
    {"changequote", CHNQTYPE | NOARGS},
    {"changecom", CHNCTYPE | NOARGS},
    {"index", INDXTYPE},
#ifdef EXTENDED
    {"paste", PASTTYPE},
    {"spaste", SPASTYPE},
    /* Newer extensions, needed to handle gnu-m4 scripts */
    {"indir", INDIRTYPE},
    {"builtin", BUILTINTYPE},
    {"patsubst", PATSTYPE},
    {"regexp", REGEXPTYPE},
    {"esyscmd", ESYSCMDTYPE},
    {"__file__", FILENAMETYPE | NOARGS},
    {"__line__", LINETYPE | NOARGS},
#endif
    {"popdef", POPDTYPE},
    {"pushdef", PUSDTYPE},
    {"dumpdef", DUMPTYPE | NOARGS},
    {"shift", SHIFTYPE | NOARGS},
    {"translit", TRNLTYPE},
    {"undefine", UNDFTYPE},
    {"undivert", UNDVTYPE | NOARGS},
    {"divnum", DIVNTYPE | NOARGS},
    {"maketemp", MKTMTYPE},
    {"errprint", ERRPTYPE | NOARGS},
    {"m4wrap", M4WRTYPE | NOARGS},
    {"m4exit", EXITTYPE | NOARGS},
    {"syscmd", SYSCTYPE},
    {"sysval", SYSVTYPE | NOARGS},
    {"traceon", TRACEONTYPE | NOARGS},
    {"traceoff", TRACEOFFTYPE | NOARGS},

#if defined(unix) || defined(__unix__)
    {"unix", SELFTYPE | NOARGS},
#else
#ifdef vms
    {"vms", SELFTYPE | NOARGS},
#endif
#endif
};

#define MAXKEYS (sizeof(keywrds) / sizeof(struct keyblk))

extern int optind;
extern char *optarg;

#define MAXRECORD 50
static struct position {
    char *name;
    unsigned long line;
} quotes[MAXRECORD], paren[MAXRECORD];

static void record(struct position *, int);
static void dump_stack(struct position *, int);

static void macro(void);
static void initkwds(void);
static ndptr inspect(int, char *);
static int do_look_ahead(int, const char *);

static void enlarge_stack(void);

int main(int, char *[]);

int main(argc, argv)
int argc;
char *argv[];
{
    int c;
    int n;
    char *p;

    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
        signal(SIGINT, onintr);

    /*
     * We need to know if -P is there before checking -D and -U.
     */
    while ((c = getopt(argc, argv, "D:I:PU:d:go:t:")) != -1)
        if (c == 'P')
            m4prefix = 1;
    optind = 1;

    initkwds();
    initspaces();
    STACKMAX = INITSTACKMAX;

    mstack = (stae *)xalloc(sizeof(stae) * STACKMAX);
    sstack = (char *)xalloc(STACKMAX);

    maxout = 0;
    outfile = NULL;
    resizedivs(MAXOUT);

    while ((c = getopt(argc, argv, "D:I:PU:d:go:t:")) != -1)
        switch (c) {
            case 'D': /* define something..*/
                for (p = optarg; *p; p++)
                    if (*p == '=')
                        break;
                if (*p)
                    *p++ = EOS;
                dodefine(optarg, p);
                break;
            case 'I':
                addtoincludepath(optarg);
                break;
            case 'P':
                break;
            case 'U': /* undefine...       */
                remhash(optarg, TOP);
                break;
            case 'd':
                set_trace_flags(optarg);
                break;
            case 'g':
                mimic_gnu = 1;
                break;
            case 'o':
                trace_file(optarg);
                break;
            case 't':
                mark_traced(optarg, 1);
                break;
            case '?':
            default:
                usage(argv[0]);
        }

    argc -= optind;
    argv += optind;

    active = stdout; /* default active output     */
    bbase[0] = bufbase;
    if (!argc) {
        sp = -1; /* stack pointer initialized */
        fp = 0;  /* frame pointer initialized */
        set_input(infile + 0, stdin, "stdin");
        /* default input (naturally) */
        macro();
    } else
        for (; argc--; ++argv) {
            p = *argv;
            if (p[0] == '-' && p[1] == EOS)
                set_input(infile, stdin, "stdin");
            else if (fopen_trypath(infile, p) == NULL)
                err(1, "%s", p);
            sp = -1;
            fp = 0;
            macro();
            release_input(infile);
        }

    if (*m4wraps) {         /* anything for rundown ??   */
        ilevel = 0;         /* in case m4wrap includes.. */
        bufbase = bp = buf; /* use the entire buffer   */
        pbstr(m4wraps);     /* user-defined wrapup act   */
        macro();            /* last will and testament   */
    }

    if (active != stdout)
        active = stdout;         /* reset output just in case */
    for (n = 1; n < maxout; n++) /* default wrap-up: undivert */
        if (outfile[n] != NULL)
            getdiv(n);
    /* remove bitbucket if used  */
    if (outfile[0] != NULL) {
        (void)fclose(outfile[0]);
    }

    return 0;
}

/*
 * Look ahead for `token'.
 * (on input `t == token[0]')
 * Used for comment and quoting delimiters.
 * Returns 1 if `token' present; copied to output.
 *         0 if `token' not found; all characters pushed back
 */
static int
do_look_ahead(t, token)
int t;
const char *token;
{
    int i;

    assert((unsigned char)t == (unsigned char)token[0]);

    for (i = 1; *++token; i++) {
        t = gpbc();
        if (t == EOF || (unsigned char)t != (unsigned char)*token) {
            putback(t);
            while (--i)
                putback(*--token);
            return 0;
        }
    }
    return 1;
}

#define LOOK_AHEAD(t, token) (t != EOF &&                                        \
                              (unsigned char)(t) == (unsigned char)(token)[0] && \
                              do_look_ahead(t, token))

/*
 * macro - the work horse..
 */
static void
macro() {
    char token[MAXTOK + 1];
    int t, l;
    ndptr p;
    int nlpar;

    l = 0; /* XXXGCC -Wuninitialized [sun2] */

    cycle {
        t = gpbc();
        if (t == '_' || isalpha(t)) {
            p = inspect(t, token);
            if (p != nil)
                putback(l = gpbc());
            if (p == nil || (l != LPAREN &&
                             (p->type & NEEDARGS) != 0))
                outputstr(token);
            else {
                /*
                 * real thing.. First build a call frame:
                 */
                pushf(fp);       /* previous call frm */
                pushf(p->type);  /* type of the call  */
                pushf(0);        /* parenthesis level */
                fp = sp;         /* new frame pointer */
                                 /*
                                  * now push the string arguments:
                                  */
                pushs1(p->defn); /* defn string */
                pushs1(p->name); /* macro name  */
                pushs(ep);       /* start next..*/

                if (l != LPAREN && PARLEV == 0) {
                    /* no bracks  */
                    chrsave(EOS);

                    if (sp == STACKMAX)
                        errx(1, "internal stack overflow");
                    eval((const char **)mstack + fp + 1, 2,
                         CALTYP);

                    ep = PREVEP; /* flush strspace */
                    sp = PREVSP; /* previous sp..  */
                    fp = PREVFP; /* rewind stack...*/
                }
            }
        } else if (t == EOF) {
            if (sp > -1) {
                warnx("unexpected end of input, unclosed parenthesis:");
                dump_stack(paren, PARLEV);
                exit(1);
            }
            if (ilevel <= 0)
                break; /* all done thanks.. */
            release_input(infile + ilevel--);
            bufbase = bbase[ilevel];
            continue;
        }
        /*
         * non-alpha token possibly seen..
         * [the order of else if .. stmts is important.]
         */
        else if (LOOK_AHEAD(t, lquote)) { /* strip quotes */
            nlpar = 0;
            record(quotes, nlpar++);
            /*
             * Opening quote: scan forward until matching
             * closing quote has been found.
             */
            do {
                l = gpbc();
                if (LOOK_AHEAD(l, rquote)) {
                    if (--nlpar > 0)
                        outputstr(rquote);
                } else if (LOOK_AHEAD(l, lquote)) {
                    record(quotes, nlpar++);
                    outputstr(lquote);
                } else if (l == EOF) {
                    if (nlpar == 1)
                        warnx("unclosed quote:");
                    else
                        warnx("%d unclosed quotes:", nlpar);
                    dump_stack(quotes, nlpar);
                    exit(1);
                } else {
                    if (nlpar > 0) {
                        if (sp < 0)
                            putc(l, active);
                        else
                            CHRSAVE(l);
                    }
                }
            } while (nlpar != 0);
        }

        else if (sp < 0 && LOOK_AHEAD(t, scommt)) {
            fputs(scommt, active);

            for (;;) {
                t = gpbc();
                if (LOOK_AHEAD(t, ecommt)) {
                    fputs(ecommt, active);
                    break;
                }
                if (t == EOF)
                    break;
                putc(t, active);
            }
        }

        else if (sp < 0) {   /* not in a macro at all */
            putc(t, active); /* output directly..	 */
        }

        else
            switch (t) {
                case LPAREN:
                    if (PARLEV > 0)
                        chrsave(t);
                    while (isspace(l = gpbc())); /* skip blank, tab, nl.. */
                    putback(l);
                    record(paren, PARLEV++);
                    break;

                case RPAREN:
                    if (--PARLEV > 0)
                        chrsave(t);
                    else { /* end of argument list */
                        chrsave(EOS);

                        if (sp == STACKMAX)
                            errx(1, "internal stack overflow");

                        eval((const char **)mstack + fp + 1, sp - fp,
                             CALTYP);

                        ep = PREVEP; /* flush strspace */
                        sp = PREVSP; /* previous sp..  */
                        fp = PREVFP; /* rewind stack...*/
                    }
                    break;

                case COMMA:
                    if (PARLEV == 1) {
                        chrsave(EOS); /* new argument   */
                        while (isspace(l = gpbc()));
                        putback(l);
                        pushs(ep);
                    } else
                        chrsave(t);
                    break;

                default:
                    if (LOOK_AHEAD(t, scommt)) {
                        char *p;
                        for (p = scommt; *p; p++)
                            chrsave(*p);
                        for (;;) {
                            t = gpbc();
                            if (LOOK_AHEAD(t, ecommt)) {
                                for (p = ecommt; *p; p++)
                                    chrsave(*p);
                                break;
                            }
                            if (t == EOF)
                                break;
                            CHRSAVE(t);
                        }
                    } else
                        CHRSAVE(t); /* stack the char */
                    break;
            }
    }
}

/*
 * output string directly, without pushing it for reparses.
 */
void
    outputstr(s)
        const char *s;
{
    if (sp < 0)
        while (*s)
            putc(*s++, active);
    else
        while (*s)
            CHRSAVE(*s++);
}

/*
 * build an input token..
 * consider only those starting with _ or A-Za-z. This is a
 * combo with lookup to speed things up.
 */
static ndptr
inspect(c, tp)
int c;
char *tp;
{
    char *name = tp;
    char *etp = tp + MAXTOK;
    ndptr p;
    unsigned int h;

    h = *tp++ = c;

    while ((isalnum(c = gpbc()) || c == '_') && tp < etp)
        h = (h << 5) + h + (*tp++ = c);
    if (c != EOF)
        PUTBACK(c);
    *tp = EOS;
    /* token is too long, it won't match anything, but it can still
     * be output. */
    if (tp == ep) {
        outputstr(name);
        while (isalnum(c = gpbc()) || c == '_') {
            if (sp < 0)
                putc(c, active);
            else
                CHRSAVE(c);
        }
        *name = EOS;
        return nil;
    }

    for (p = hashtab[h % HASHSIZE]; p != nil; p = p->nxtptr)
        if (h == p->hv && STREQ(name, p->name))
            break;
    return p;
}

/*
 * initkwds - initialise m4 keywords as fast as possible.
 * This very similar to install, but without certain overheads,
 * such as calling lookup. Malloc is not used for storing the
 * keyword strings, since we simply use the static pointers
 * within keywrds block.
 */
static void
initkwds() {
    size_t i;
    unsigned int h;
    ndptr p;
    char *k;

    for (i = 0; i < MAXKEYS; i++) {
        k = (char *)keywrds[i].knam;
        if (m4prefix) {
            size_t klen = strlen(k);
            char *newk = malloc(klen + 4);

            if (snprintf(newk, klen + 4, "m4_%s", k) == -1)
                err(1, "snprintf");
            keywrds[i].knam = newk;
            k = newk;
        }
        h = hash(k);
        p = (ndptr)xalloc(sizeof(struct ndblock));
        p->nxtptr = hashtab[h % HASHSIZE];
        hashtab[h % HASHSIZE] = p;
        p->name = xstrdup(keywrds[i].knam);
        p->defn = null;
        p->hv = h;
        p->type = keywrds[i].ktyp & TYPEMASK;
        if ((keywrds[i].ktyp & NOARGS) == 0)
            p->type |= NEEDARGS;
    }
}

/* Look up a builtin type, even if overridden by the user */
int
    builtin_type(key)
        const char *key;
{
    int i;

    for (i = 0; i != MAXKEYS; i++)
        if (STREQ(keywrds[i].knam, key))
            return keywrds[i].ktyp;
    return -1;
}

const char *
builtin_realname(n)
int n;
{
    int i;

    for (i = 0; i != MAXKEYS; i++)
        if (((keywrds[i].ktyp ^ n) & TYPEMASK) == 0)
            return keywrds[i].knam;
    return NULL;
}

static void
    record(t, lev) struct position *t;
int lev;
{
    if (lev < MAXRECORD) {
        t[lev].name = CURRENT_NAME;
        t[lev].line = CURRENT_LINE;
    }
}

static void
    dump_stack(t, lev) struct position *t;
int lev;
{
    int i;

    for (i = 0; i < lev; i++) {
        if (i == MAXRECORD) {
            fprintf(stderr, "   ...\n");
            break;
        }
        fprintf(stderr, "   %s at line %lu\n",
                t[i].name, t[i].line);
    }
}

static void
enlarge_stack() {
    STACKMAX *= 2;
    mstack = realloc(mstack, sizeof(stae) * STACKMAX);
    sstack = realloc(sstack, STACKMAX);
    if (mstack == NULL || sstack == NULL)
        errx(1, "Evaluation stack overflow (%lu)",
             (unsigned long)STACKMAX);
}
