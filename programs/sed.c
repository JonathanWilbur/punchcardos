// sed uses in Linux kernel build:
// sed -n -r -e 's/^([0-9a-fA-F]+) [ABCDGRSTVW] (.+)$/pa_ = ;/p'
// sed -n 1p
// sed -n 's/^[[:alnum:]:_]*\.file=//p'
// sed -e 's:^:kernel/:' -e 's/$/.ko/'
// sed -n -e 's/^\([0-9a-fA-F]*\) [ABCDGRSTVW] \(_text\|__start_rodata\|__bss_start\|_end\)$/#define VO_ _AC(0x,UL)/p'
// sed -n -e 's/^\([0-9a-fA-F]*\) [a-zA-Z] \(startup_32\|efi.._stub_entry\|efi\(32\)\?_pe_entry\|input_data\|kernel_info\|_end\|_ehead\|_text\|_e\?data\|z_.*\)$/#define ZO_ 0x/p'
// sed 's/ .*//'
// sed '$d' include/linux/atomic/atomic-arch-fallback.h
// sed -n '$s:// ::p' include/linux/atomic/atomic-arch-fallback.h
// sed -ne 	's:^[[:space:]]*\.ascii[[:space:]]*"\(.*\)".*:\1:; /^->/{s:->#\(.*\):/* \1 */:; s:^->\([^ ]*\) [\$#]*\([^ ]*\) \(.*\):#define \1 \2 /* \3 */:; s:->::; p;}'
/* Taken from here: https://github.com/tar-mirror/minised/tree/master

Released under a BSD 3-Clause "New" or "Revised" License at the time of copying.

Copyright (C) 1995-2003 Eric S. Raymond
Copyright (C) 2004-2005 Rene Rebe
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 - Neither the name of ExactCODE nor the names of its contributors may
   be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef NOLIBC
#include <stdlib.h>	/* exit */
#include <stdio.h>	/* {f}puts, {f}printf, getc/putc, f{re}open, fclose, fopen */
#include <ctype.h>	/* for isprint(), isdigit(), toascii() macros */
#include <string.h>	/* for memcmp(3), strcmp */
#include <fcntl.h>
#include <unistd.h>
#else

FILE *fopen (const char *restrict pathname, const char *restrict mode) {
    int flags = 0;
    int fd = open(pathname, flags);
    // You don't need to handle an error. nolibc's implementation handles it.
    return fdopen(fd, mode); // Flags are ignored, but pass them in anyway.
}

static int ungot = -1;

int ungetc(int c, int stream) {
    return lseek(stream, -1, SEEK_CUR) != -1 ? c : EOF;
}

#endif

/* sed.h -- types and constants for the stream editor
   Copyright (C) 1995-2003 Eric S. Raymond
   Copyright (C) 2004-2005 Rene Rebe
*/

#define TRUE            1
#define FALSE           0

/* data area sizes used by both modules */
#define MAXBUF		4000	/* current line buffer size */
#define MAXAPPENDS	20	/* maximum number of appends */
#define MAXTAGS		9	/* tagged patterns are \1 to \9 */
#define MAXCMDS		200	/* maximum number of compiled commands */
#define MAXLINES	256	/* max # numeric addresses to compile */ 

/* constants for compiled-command representation */
#define EQCMD	0x01	/* = -- print current line number		*/
#define ACMD	0x02	/* a -- append text after current line 	*/
#define BCMD	0x03	/* b -- branch to label				*/
#define CCMD	0x04	/* c -- change current line 		*/
#define DCMD	0x05	/* d -- delete all of pattern space		*/
#define CDCMD	0x06	/* D -- delete first line of pattern space	*/
#define GCMD	0x07	/* g -- copy hold space to pattern space	*/
#define CGCMD	0x08	/* G -- append hold space to pattern space	*/
#define HCMD	0x09	/* h -- copy pattern space to hold space	*/
#define CHCMD	0x0A	/* H -- append hold space to pattern space	*/
#define ICMD	0x0B	/* i -- insert text before current line 	*/
#define LCMD	0x0C	/* l -- print pattern space in escaped form	*/
#define CLCMD   0x20	/* L -- hexdump					*/
#define NCMD	0x0D	/* n -- get next line into pattern space	*/
#define CNCMD	0x0E	/* N -- append next line to pattern space	*/
#define PCMD	0x0F	/* p -- print pattern space to output		*/
#define CPCMD	0x10	/* P -- print first line of pattern space	*/
#define QCMD	0x11	/* q -- exit the stream editor			*/
#define RCMD	0x12	/* r -- read in a file after current line */
#define SCMD	0x13	/* s -- regular-expression substitute		*/
#define TCMD	0x14	/* t -- branch on last substitute successful	*/
#define CTCMD	0x15	/* T -- branch on last substitute failed	*/
#define WCMD	0x16	/* w -- write pattern space to file		*/
#define CWCMD	0x17	/* W -- write first line of pattern space	*/
#define XCMD	0x18	/* x -- exhange pattern and hold spaces		*/
#define YCMD	0x19	/* y -- transliterate text			*/

typedef struct	cmd_t			/* compiled-command representation */
{
	char	*addr1;			/* first address for command */
	char	*addr2;			/* second address for command */
	union
	{
		char		*lhs;	/* s command lhs */
		struct cmd_t	*link;	/* label link */
	} u;
	char	command;		/* command code */
	char	*rhs;			/* s command replacement string */
	FILE	*fout;	 		/* associated output file descriptor */
	struct
	{
		unsigned allbut  : 1;	/* was negation specified? */
		unsigned global  : 1;	/* was p postfix specified? */
		unsigned print   : 2;	/* was g postfix specified? */
		unsigned inrange : 1;	/* in an address range? */
	} flags;
	unsigned nth;			/* sed nth occurance */
}
sedcmd;		/* use this name for declarations */

#define BAD	((char *) -1)		/* guaranteed not a string ptr */

/* address and regular expression compiled-form markers */
#define STAR	1	/* marker for Kleene star */
#define CCHR	2	/* non-newline character to be matched follows */
#define CDOT	4	/* dot wild-card marker */
#define CCL	6	/* character class follows */
#define CNL	8	/* match line start */
#define CDOL	10	/* match line end */
#define CBRA	12	/* tagged pattern start marker */
#define CKET	14	/* tagged pattern end marker */
#define CBACK	16	/* backslash-digit pair marker */
#define CLNUM	18	/* numeric-address index follows */
#define CEND	20	/* symbol for end-of-source */
#define CEOF	22	/* end-of-field mark */

#define bits(b) (1 << (b))

/* sed.h ends here */

/* sedcomp.c -- stream editor main and compilation phase
   Copyright (C) 1995-2003 Eric S. Raymond
   Copyright (C) 2004-2014 Rene Rebe

   The stream editor compiles its command input  (from files or -e options)
into an internal form using compile() then executes the compiled form using
execute(). Main() just initializes data structures, interprets command line
options, and calls compile() and execute() in appropriate sequence.
   The data structure produced by compile() is an array of compiled-command
structures (type sedcmd).  These contain several pointers into pool[], the
regular-expression and text-data pool, plus a command code and g & p flags.
In the special case that the command is a label the struct  will hold a ptr
into the labels array labels[] during most of the compile,  until resolve()
resolves references at the end.
   The operation of execute() is described in its source module. 
*/

/***** public stuff ******/

#define MAXCMDS		200	/* maximum number of compiled commands */
#define MAXLINES	256	/* max # numeric addresses to compile */ 

/* main data areas */
char	linebuf[MAXBUF+1];	/* current-line buffer */
sedcmd	cmds[MAXCMDS+1];	/* hold compiled commands */
long	linenum[MAXLINES];	/* numeric-addresses table */

/* miscellaneous shared variables */ 
int	nflag;			/* -n option flag */
int	eargc;			/* scratch copy of argument count */
sedcmd	*pending	= NULL;	/* next command to be executed */

int	last_line_used = 0;	/* last line address ($) was used */

void die (const char* msg) {
	fprintf(stderr, "sed: ");
	fprintf(stderr, msg, linebuf);
	fprintf(stderr, "\n");
	exit(2);
}

/***** module common stuff *****/

#define POOLSIZE	10000	/* size of string-pool space */
#define WFILES		10	/* max # w output files that can be compiled */
#define	RELIMIT		256	/* max chars in compiled RE */
#define	MAXDEPTH	20	/* maximum {}-nesting level */
#define	MAXLABS		50	/* max # of labels that can be handled */

#define SKIPWS(pc)	while ((*pc==' ') || (*pc=='\t')) pc++
#define IFEQ(x, v)	if (*x == v) x++ , /* do expression */

/* error messages */
static const char	AGMSG[]	= "garbled address %s";
static const char	CGMSG[]	= "garbled command %s";
static const char	TMTXT[]	= "too much text: %s";
static const char	AD1NG[]	= "no addresses allowed for %s";
static const char	AD2NG[]	= "only one address allowed for %s";
static const char	TMCDS[]	= "too many commands, last was %s";
static const char	COCFI[]	= "cannot open command-file %s";
static const char	UFLAG[]	= "unknown flag %c";
static const char	CCOFI[]	= "cannot create %s";
static const char	ULABL[]	= "undefined label %s";
static const char	TMLBR[]	= "too many {'s";
static const char	FRENL[]	= "first RE must be non-null";
static const char	NSCAX[]	= "no such command as %s";
static const char	TMRBR[]	= "too many }'s";
static const char	DLABL[]	= "duplicate label %s";
static const char	TMLAB[]	= "too many labels: %s";
static const char	TMWFI[]	= "too many w files";
static const char	REITL[]	= "RE too long: %s";
static const char	TMLNR[]	= "too many line numbers";
static const char	TRAIL[]	= "command \"%s\" has trailing garbage";
static const char	RETER[] = "RE not terminated: %s";
static const char	CCERR[] = "unknown character class: %s";

/* cclass to c function mapping ,-) */
static const char* cclasses[] = {
	"alnum", "a-zA-Z0-9",
	"lower", "a-z",
	"space", " \f\n\r\t\v",
	"alpha", "a-zA-Z",
	"digit", "0-9",
	"upper", "A-Z",
	"blank", " \t",
	"xdigit", "0-9A-Fa-f",
	"cntrl", "\x01-\x1f\x7e",
	"print", " -\x7e",
	"graph", "!-\x7e",
	"punct", "!-/:-@[-`{-\x7e",
	NULL, NULL};
 
typedef struct			/* represent a command label */
{
	char		*name;		/* the label name */
	sedcmd		*last;		/* it's on the label search list */  
	sedcmd		*address;	/* pointer to the cmd it labels */
} label;

/* label handling */
static label	labels[MAXLABS];	/* here's the label table */
static label	*lab	= labels + 1;	/* pointer to current label */
static label	*lablst = labels;	/* header for search list */

/* string pool for regular expressions, append text, etc. etc. */
static char	pool[POOLSIZE];			/* the pool */
static char	*fp	= pool;			/* current pool pointer */
static char	*poolend = pool + POOLSIZE;	/* pointer past pool end */

/* compilation state */
static FILE	*cmdf	= NULL;		/* current command source */
static char	*cp	= linebuf;	/* compile pointer */
static sedcmd	*cmdp	= cmds;		/* current compiled-cmd ptr */
static char	*lastre	= NULL;		/* old RE pointer */
static int	bdepth	= 0;		/* current {}-nesting level */
static int	bcount	= 0;		/* # tagged patterns in current RE */
static char	**eargv;		/* scratch copy of argument list */

/* compilation flags */
static int	eflag;			/* -e option flag */
static int	gflag;			/* -g option flag */

/* prototypes */
static char *address(char *expbuf);
static char *gettext(char* txp);
static char *recomp(char *expbuf, char redelim);
static char *rhscomp(char* rhsp, char delim);
static char *ycomp(char *ep, char delim);
static int cmdcomp(char cchar);
static int cmdline(char	*cbuf);
static label *search(label *ptr);
static void compile(void);
static void resolve(void);

/* sedexec.c protypes */
void execute(char* file);

/* main sequence of the stream editor */
int main(int argc, char *argv[])
{
	eargc	= argc;		/* set local copy of argument count */
	eargv	= argv;		/* set local copy of argument list */
	cmdp->addr1 = pool;	/* 1st addr expand will be at pool start */
	if (eargc == 1)
		exit(0);	/* exit immediately if no arguments */

	/* scan through the arguments, interpreting each one */
	while ((--eargc > 0) && (**++eargv == '-'))
		switch (eargv[0][1])
		{
		case 'e':
			eflag++; compile();	/* compile with e flag on */
			eflag = 0;
			continue;		/* get another argument */
		case 'f':
			if (eargc-- <= 0)	/* barf if no -f file */
				exit(2);
			if ((cmdf = fopen(*++eargv, "r")) == NULL)
			{
				fprintf(stderr, COCFI, *eargv);
				exit(2);
			}
			compile();	/* file is O.K., compile it */
			fclose(cmdf);
			continue;	/* go back for another argument */
		case 'g':
			gflag++;	/* set global flag on all s cmds */
			continue;
		case 'n':
			nflag++;	/* no print except on p flag or w */
			continue;
		default:
			fprintf(stdout, UFLAG, eargv[0][1]);
			continue;
		}

	if (cmdp == cmds)	/* no commands have been compiled */
	{
		eargv--; eargc++;
		eflag++; compile(); eflag = 0;
		eargv++; eargc--;
	}

	if (bdepth)	/* we have unbalanced squigglies */
		die(TMLBR);

	lablst->address = cmdp;	/* set up header of label linked list */
	resolve();		/* resolve label table indirections */
	if (eargc <= 0)		/* if there were no -e commands */
		execute(NULL);	/*   execute commands from stdin only */
	else while(--eargc>=0)	/* else execute only -e commands */
		execute(*eargv++);
	exit(0);		/* everything was O.K. if we got here */
}

#define	H	0x80	/* 128 bit, on if there's really code for command */
#define LOWCMD	56	/* = '8', lowest char indexed in cmdmask */ 

/* indirect through this to get command internal code, if it exists */
static char	cmdmask[] =
{
	0,	0,	H,	0,	0,	H+EQCMD,0,	0,
	0,	0,	0,	0,	H+CDCMD,0,	0,	CGCMD,
	CHCMD,	0,	0,	0,	H+CLCMD,0,	CNCMD,	0,
	CPCMD,	0,	0,	0,	H+CTCMD,0,	0,	H+CWCMD,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	H+ACMD,	H+BCMD,	H+CCMD,	DCMD,	0,	0,	GCMD,
	HCMD,	H+ICMD,	0,	0,	H+LCMD,	0,	NCMD,	0,
	PCMD,	H+QCMD,	H+RCMD,	H+SCMD,	H+TCMD,	0,	0,	H+WCMD,
	XCMD,	H+YCMD,	0,	H+BCMD,	0,	H,	0,	0,
};

/* precompile sed commands out of a file */
static void compile(void)
{
	char	ccode;

	for(;;)					/* main compilation loop */
	{
		SKIPWS(cp);
		if (*cp == ';') {
			cp++;
			SKIPWS(cp);
		}

		if (*cp == '\0' || *cp == '#')	/* get a new command line */
			if (cmdline(cp = linebuf) < 0)
				break;
		SKIPWS(cp);

		if (*cp == '\0' || *cp == '#')	/* a comment */
			continue;

		/* compile first address */
		if (fp > poolend)
			die(TMTXT);
		else if ((fp = address(cmdp->addr1 = fp)) == BAD)
			die(AGMSG);

		if (fp == cmdp->addr1)		/* if empty RE was found */
		{
			if (lastre)		/* if there was previous RE */
				cmdp->addr1 = lastre;	/* use it */
			else
				die(FRENL);
		}
		else if (fp == NULL)		/* if fp was NULL */
		{
			fp = cmdp->addr1;	/* use current pool location */
			cmdp->addr1 = NULL;
		}
		else
		{
			lastre = cmdp->addr1;
			if (*cp == ',' || *cp == ';')	/* there's 2nd addr */
			{
				cp++;
				if (fp > poolend) die(TMTXT);
				fp = address(cmdp->addr2 = fp);
				if (fp == BAD || fp == NULL) die(AGMSG);
				if (fp == cmdp->addr2)
					cmdp->addr2 = lastre;
				else
					lastre = cmdp->addr2;
			}
			else
				cmdp->addr2 = NULL;	/* no 2nd address */
		}
		if (fp > poolend) die(TMTXT);

		SKIPWS(cp);		/* discard whitespace after address */

		if (*cp == '!') {
			cmdp->flags.allbut = 1;
			cp++; SKIPWS(cp);
		}

		/* get cmd char, range-check it */
		if ((*cp < LOWCMD) || (*cp > '~')
			|| ((ccode = cmdmask[*cp - LOWCMD]) == 0))
				die(NSCAX);

		cmdp->command = ccode & ~H;	/* fill in command value */
		if ((ccode & H) == 0)		/* if no compile-time code */
			cp++;			/* discard command char */
		else if (cmdcomp(*cp++))	/* execute it; if ret = 1 */
			continue;		/* skip next line read */

		if (++cmdp >= cmds + MAXCMDS) die(TMCDS);

		SKIPWS(cp);			/* look for trailing stuff */
		if (*cp != '\0')
		{
			if (*cp == ';')
			{
				continue;
			}
			else if (*cp != '#' && *cp != '}')
				die(TRAIL);
		}
	}
}

/* compile a single command */
static int cmdcomp(char cchar)
{
	static sedcmd	**cmpstk[MAXDEPTH];	/* current cmd stack for {} */
	static const char *fname[WFILES];	/* w file name pointers */
	static FILE	*fout[WFILES];		/* w file file ptrs */
	static int	nwfiles	= 2;		/* count of open w files */
	int		i;			/* indexing dummy used in w */
	sedcmd		*sp1, *sp2;		/* temps for label searches */
	label		*lpt;			/* ditto, and the searcher */
	char		redelim;		/* current RE delimiter */

	fout[0] = stdout;
	fout[1] = stderr;
	
	fname[0] = "/dev/stdout";
	fname[1] = "/dev/stderr";

	switch(cchar)
	{
	case '{':	/* start command group */
		cmdp->flags.allbut = !cmdp->flags.allbut;
		cmpstk[bdepth++] = &(cmdp->u.link);
		if (++cmdp >= cmds + MAXCMDS) die(TMCDS);
		if (*cp == '\0') *cp++ = ';', *cp = '\0';	/* get next cmd w/o lineread */
		return 1;

	case '}':	/* end command group */
		if (cmdp->addr1) die(AD1NG);	/* no addresses allowed */
		if (--bdepth < 0) die(TMRBR);	/* too many right braces */
		*cmpstk[bdepth] = cmdp;		/* set the jump address */
		return 1;

	case '=':			/* print current source line number */
	case 'q':			/* exit the stream editor */
		if (cmdp->addr2) die(AD2NG);
		break;

	case ':':	/* label declaration */
		if (cmdp->addr1) die(AD1NG);	/* no addresses allowed */
		fp = gettext(lab->name = fp);	/* get the label name */
		if ((lpt = search(lab)))	/* does it have a double? */
		{
			if (lpt->address) die(DLABL);	/* yes, abort */
		}
		else	/* check that it doesn't overflow label table */
		{
			lab->last = NULL;
			lpt = lab;
			if (++lab >= labels + MAXLABS) die(TMLAB);
		}
		lpt->address = cmdp;
		return 1;

	case 'b':	/* branch command */
	case 't':	/* branch-on-succeed command */
	case 'T':	/* branch-on-fail command */
		SKIPWS(cp);
		if (*cp == '\0')	/* if branch is to start of cmds... */
		{
			/* add current command to end of label last */
			if ((sp1 = lablst->last)) 
			{
				while((sp2 = sp1->u.link))
					sp1 = sp2;
				sp1->u.link = cmdp;
			}
			else	/* lablst->last == NULL */
				lablst->last = cmdp;
			break;
		}
		fp = gettext(lab->name = fp);	/* else get label into pool */
		if ((lpt = search(lab)))	/* enter branch to it */
		{
			if (lpt->address)
				cmdp->u.link = lpt->address;
			else
			{
				sp1 = lpt->last;
				while((sp2 = sp1->u.link))
					sp1 = sp2;
				sp1->u.link = cmdp;
			}
		}
		else		/* matching named label not found */
		{
			lab->last = cmdp;	/* add the new label */
			lab->address = NULL;	/* it's forward of here */
			if (++lab >= labels + MAXLABS)	/* overflow if last */
				die(TMLAB);
		}
		break;

	case 'a':	/* append text */
	case 'i':	/* insert text */
	case 'r':	/* read file into stream */
		if (cmdp->addr2) die(AD2NG);
	case 'c':	/* change text */
		if ((*cp == '\\') && (*++cp == '\n')) cp++;
		fp = gettext(cmdp->u.lhs = fp);
		break;

	case 'D':	/* delete current line in hold space */
		cmdp->u.link = cmds;
		break;

	case 's':	/* substitute regular expression */
		if (*cp == 0) /* get delimiter from 1st ch */
			die(RETER);
		else
			redelim = *cp++;
		
		if ((fp = recomp(cmdp->u.lhs = fp, redelim)) == BAD)
			die(CGMSG);
		if (fp == cmdp->u.lhs) {	/* if compiled RE zero len */ 
			if (lastre) {
				cmdp->u.lhs = lastre;	/* use the previous one */
				cp++;                   /*   skip delim */
			}
			else
				die(FRENL);
		}
		else				/* otherwise */
			lastre = cmdp->u.lhs;	/*   save the one just found */
		
		if ((cmdp->rhs = fp) > poolend) die(TMTXT);
		if ((fp = rhscomp(cmdp->rhs, redelim)) == BAD) die(CGMSG);
		if (gflag) cmdp->flags.global++;
		while (*cp == 'g' || *cp == 'p' || *cp == 'P' || isdigit(*cp))
		{
			IFEQ(cp, 'g') cmdp->flags.global++;
			IFEQ(cp, 'p') cmdp->flags.print = 1;
			IFEQ(cp, 'P') cmdp->flags.print = 2;
			if (isdigit(*cp))
			{
				if (cmdp->nth)
					break; /* no multiple n args */
				
				cmdp->nth = atoi(cp); /* check 0? */
				while (isdigit(*cp)) cp++;
			}
		}

	case 'l':	/* list pattern space */
	case 'L':	/* dump pattern space */
		if (*cp == 'w')
			cp++;		/* and execute a w command! */
		else
			break;		/* s or L or l is done */

	case 'w':	/* write-pattern-space command */
	case 'W':	/* write-first-line command */
		if (nwfiles >= WFILES) die(TMWFI);
		fname[nwfiles] = fp;
		fp = gettext((fname[nwfiles] = fp, fp));	/* filename will be in pool */
		for(i = nwfiles-1; i >= 0; i--)	/* match it in table */
			if (strcmp(fname[nwfiles], fname[i]) == 0)
			{
				cmdp->fout = fout[i];
				return 0;
			}
		/* if didn't find one, open new out file */
		if ((cmdp->fout = fopen(fname[nwfiles], "w")) == NULL)
		{
			fprintf(stderr, CCOFI, fname[nwfiles]);
			exit(2);
		}
		fout[nwfiles++] = cmdp->fout;
		break;

	case 'y':	/* transliterate text */
		fp = ycomp(cmdp->u.lhs = fp, *cp++);	/* compile translit */
		if (fp == BAD) die(CGMSG);		/* fail on bad form */
		if (fp > poolend) die(TMTXT);		/* fail on overflow */
		break;
	}
	return 0;	/* succeeded in interpreting one command */
}

/* generate replacement string for substitute command right hand side
   rhsp:	place to compile expression to
   delim:	regular-expression end-mark to look for */
static char *rhscomp(char* rhsp, char delim)	/* uses bcount */
{
	register char	*p = cp;

	for(;;)
		/* copy for the likely case it is not s.th. special */
		if ((*rhsp = *p++) == '\\') /* back reference or escape  */
		{
			if (*p >= '0' && *p <= '9') /* back reference */
			{
			dobackref:
				*rhsp = *p++;
				/* check validity of pattern tag */
				if (*rhsp > bcount + '0')
					return BAD;
				*rhsp++ |= 0x80; /* mark the good ones */
			}
			else /* escape */
			{
				switch (*p) {
					case 'n': *rhsp = '\n'; break;
					case 'r': *rhsp = '\r'; break;
					case 't': *rhsp = '\t'; break;
					default: *rhsp = *p;
				}
				rhsp++; p++;
			}
		}
		else if (*rhsp == delim)	/* found RE end, hooray... */
		{
			*rhsp++ = '\0';		/* cap the expression string */
			cp = p;
			return rhsp;		/* pt at 1 past the RE */
		}
		else if (*rhsp == '&')		/* special case, convert to backref \0 */
		{
			*--p = '0';
			goto dobackref;
		}
		else if (*rhsp++ == '\0')	/* last ch not RE end, help! */
			return BAD;
}

/* compile a regular expression to internal form
   expbuf:	place to compile it to
   redelim:	RE end-marker to look for */
static char *recomp(char *expbuf, char redelim)	/* uses cp, bcount */
{
	register char	*ep = expbuf;	/* current-compiled-char pointer */
	register char	*sp = cp;	/* source-character ptr */
	register int	c;		/* current-character pointer */
	char		negclass;	/* all-but flag */
	char		*lastep;	/* ptr to last expr compiled */
	char		*lastep2;	/* dito, but from the last loop */
	char		*svclass;	/* start of current char class */
	char		brnest[MAXTAGS];	/* bracket-nesting array */
	char		*brnestp;	/* ptr to current bracket-nest */
	char		*pp;		/* scratch pointer */
	int 		classct;	/* class element count */
	int		tags;		/* # of closed tags */

	if (*cp == redelim) {		/* if first char is RE endmarker */
	    return ep;
	}

	lastep = lastep2 = NULL;	/* there's no previous RE */
	brnestp = brnest;		/* initialize ptr to brnest array */
	tags = bcount = 0;		/* initialize counters */

	if ((*ep++ = (*sp == '^')))	/* check for start-of-line syntax */
		sp++;

	for (;;)
	{
		if (*sp == 0) /* no termination */
			die (RETER);
		if (ep >= expbuf + RELIMIT)	/* match is too large */
			return cp = sp, BAD;
		if ((c = *sp++) == redelim)	/* found the end of the RE */
		{
			cp = sp;
			if (brnestp != brnest)	/* \(, \) unbalanced */
				return BAD;
			*ep++ = CEOF;		/* write end-of-pattern mark */
			return ep;		/* return ptr to compiled RE */
		}

		lastep = lastep2;
		lastep2 = ep;

		switch (c)
		{
		case '\\':
			if ((c = *sp++) == '(')	/* start tagged section */
			{
				if (bcount >= MAXTAGS)
					return cp = sp, BAD;
				*brnestp++ = bcount;	/* update tag stack */
				*ep++ = CBRA;		/* enter tag-start */
				*ep++ = bcount++;	/* bump tag count */
				lastep2 = NULL;
				continue;
			}
			else if (c == ')')	/* end tagged section */
			{
				if (brnestp <= brnest)	/* extra \) */
					return cp = sp, BAD;
				*ep++ = CKET;		/* enter end-of-tag */
				*ep++ = *--brnestp;	/* pop tag stack */
				tags++;			/* count closed tags */
				for (lastep2 = ep-1; *lastep2 != CBRA; )
					--lastep2; /* FIXME: lastep becomes start */
				continue;
			}
			else if (c >= '1' && c <= '9' && c != redelim)	/* tag use, if !delim */
			{
				if ((c -= '1') >= tags)	/* too few */
					return BAD;
				*ep++ = CBACK;		/* enter tag mark */
				*ep++ = c;		/* and the number */
				continue;
			}
			else if (c == '\n')	/* escaped newline no good */
				return cp = sp, BAD;
			else if (c == 'n')		/* match a newline */
				c = '\n';
			else if (c == 't')		/* match a tab */
				c = '\t';
			else if (c == 'r')		/* match a return */
				c = '\r';
			else if (c == '+') /* 1..n repeat of previous pattern */
			{
			  if (lastep == NULL)	/* if + not first on line */
				goto defchar;	/*   match a literal + */
			  pp = ep;		/* else save old ep */
			  *ep++ = *lastep++ | STAR;	/* flag the copy */
			  while (lastep < pp)	/* so we can blt the pattern */
				*ep++ = *lastep++;
			  lastep2 = lastep;       /* no new expression */
			  continue;
			}
			goto defchar;		/* else match \c */

		case '\0':	/* ignore nuls */
			continue;

		case '\n':	/* trailing pattern delimiter is missing */
			return cp = sp, BAD;

		case '.':	/* match any char except newline */
			*ep++ = CDOT;
			continue;

		case '*':	/* 0..n repeat of previous pattern */
			if (lastep == NULL)	/* if * isn't first on line */
				goto defchar;	/*   match a literal * */
			*lastep |= STAR;	/* flag previous pattern */
			lastep2 = lastep;	/* no new expression */
			continue;

		case '$':	/* match only end-of-line */
			if (*sp != redelim)	/* if we're not at end of RE */
				goto defchar;	/*   match a literal $ */
			*ep++ = CDOL;		/* insert end-symbol mark */
			continue;

		case '[':	/* begin character set pattern */
			if (ep + 17 >= expbuf + RELIMIT)
				die(REITL);
			*ep++ = CCL;		/* insert class mark */
			if ((negclass = ((c = *sp++) == '^')))
				c = *sp++;
			svclass = sp;		/* save ptr to class start */
			do {
				if (c == '\0') die(CGMSG);
				/* handle predefined character classes */
				if (c == '[' && *sp == ':')
				{
				  /* look for the matching ":]]" */
				  char *p;
				  const char *p2;
				  for (p = sp+3; *p; p++)
				    if  (*p == ']' &&
				         *(p-1) == ']' &&
					 *(p-2) == ':')
					{
					  char cc[8];
					  const char **it;
					  p2 = sp+1;
					  for (p2 = sp+1;
					       p2 < p-2 && p2-sp-1 < sizeof(cc);
					       p2++)
					    cc[p2-sp-1] = *p2;
					  cc[p2-sp-1] = 0; /* termination */

					  it = cclasses;
					  while (*it && strcmp(*it, cc))
						it +=2;
					  if (!*it++)
					    die(CCERR);

					  /* generate mask */
					  p2 = *it;
					  while (*p2) {
					    if (p2[1] == '-' && p2[2]) {
						for (c = *p2; c <= p2[2]; c++)
                                		  ep[c >> 3] |= bits(c & 7);
						p2 += 3;
					    }
					    else {
						c = *p2++;
					  	ep[c >> 3] |= bits(c & 7);
					    }
					  }
					  sp = p; c = 0; break;
					}
				}

				/* handle character ranges */
				if (c == '-' && sp > svclass && *sp != ']')
					for (c = sp[-2]; c < *sp; c++)
						ep[c >> 3] |= bits(c & 7);

				/* handle escape sequences in sets */
				if (c == '\\')
				{
					if ((c = *sp++) == 'n')
						c = '\n';
					else if (c == 't')
						c = '\t';
					else if (c == 'r')
						c = '\r';
				}

				/* enter (possibly translated) char in set */
				if (c)
					ep[c >> 3] |= bits(c & 7);
			} while
				((c = *sp++) != ']');

			/* invert the bitmask if all-but was specified */
			if (negclass)
				for(classct = 0; classct < 16; classct++)
					ep[classct] ^= 0xFF;
			ep[0] &= 0xFE;		/* never match ASCII 0 */ 
			ep += 16;		/* advance ep past set mask */
			continue;

		defchar:	/* match literal character */
		default:	/* which is what we'd do by default */
			*ep++ = CCHR;		/* insert character mark */
			*ep++ = c;
		}
	}
}

/* read next command from -e argument or command file */
static int cmdline(char	*cbuf)		/* uses eflag, eargc, cmdf */
{
	register int	inc;	/* not char because must hold EOF */

	cbuf--;			/* so pre-increment points us at cbuf */

	/* e command flag is on */
	if (eflag)
	{
		register char	*p;	/* ptr to current -e argument */
		static char	*savep;	/* saves previous value of p */

		if (eflag > 0)	/* there are pending -e arguments */
		{
			eflag = -1;
			if (eargc-- <= 0)
				exit(2);	/* if no arguments, barf */

			/* else transcribe next e argument into cbuf */
			p = *++eargv;
			while((*++cbuf = *p++))
				if (*cbuf == '\\')
				{
					if ((*++cbuf = *p++) == '\0')
						return savep = NULL, -1;
					else
						continue;
				}
				else if (*cbuf == '\n')	/* end of 1 cmd line */
				{ 
					*cbuf = '\0';
					return savep = p, 1;
					/* we'll be back for the rest... */
				}

			/* found end-of-string; can advance to next argument */
			return savep = NULL, 1;
		}

		if ((p = savep) == NULL)
			return -1;

		while((*++cbuf = *p++))
			if (*cbuf == '\\')
			{
				if ((*++cbuf = *p++) == '0')
					return savep = NULL, -1;
				else
					continue;
			}
			else if (*cbuf == '\n')
			{
				*cbuf = '\0';
				return savep = p, 1;
			}

		return savep = NULL, 1;
	}

	/* if no -e flag read from command file descriptor */
	while((inc = getc(cmdf)) != EOF)		/* get next char */
		if ((*++cbuf = inc) == '\\')		/* if it's escape */ 
			*++cbuf = inc = getc(cmdf);	/* get next char */
		else if (*cbuf == '\n')			/* end on newline */
			return *cbuf = '\0', 1;	/* cap the string */

	return *++cbuf = '\0', -1;	/* end-of-file, no more chars */
}

/* expand an address at *cp... into expbuf, return ptr at following char */
static char *address(char *expbuf)		/* uses cp, linenum */
{
	static int	numl = 0;	/* current ind in addr-number table */
	register char	*rcp;		/* temp compile ptr for forwd look */
	long		lno;		/* computed value of numeric address */

	if (*cp == '$')			/* end-of-source address */
	{
		*expbuf++ = CEND;	/* write symbolic end address */
		*expbuf++ = CEOF;	/* and the end-of-address mark (!) */
		cp++;			/* go to next source character */
		last_line_used = TRUE;
		return expbuf;		/* we're done */
	}
	if (*cp == '/')			/* start of regular-expression match */
		return recomp(expbuf, *cp++);	/* compile the RE */

	rcp = cp; lno = 0;		/* now handle a numeric address */
	while(*rcp >= '0' && *rcp <= '9')	/* collect digits */
		lno = lno*10 + *rcp++ - '0';	/*  compute their value */

	if (rcp > cp)			/* if we caught a number... */
	{
		*expbuf++ = CLNUM;	/* put a numeric-address marker */
		*expbuf++ = numl;	/* and the address table index */
		linenum[numl++] = lno;	/* and set the table entry */
		if (numl >= MAXLINES)	/* oh-oh, address table overflow */
			die(TMLNR);	/*   abort with error message */
		*expbuf++ = CEOF;	/* write the end-of-address marker */
		cp = rcp;		/* point compile past the address */ 
		return expbuf;		/* we're done */
	}

	return NULL;		/* no legal address was found */
}

/* accept multiline input from *cp..., discarding leading whitespace
   txp: where to put the text */
static char *gettext(char* txp)		/* uses global cp */
{
	register char	*p = cp;

	SKIPWS(p);			/* discard whitespace */
	do {
		if ((*txp = *p++) == '\\')	/* handle escapes */
			*txp = *p++;
		if (*txp == '\0')		/* we're at end of input */
			return cp = --p, ++txp;
		else if (*txp == '\n')		/* also SKIPWS after newline */
			SKIPWS(p);
	} while (txp++);		/* keep going till we find that nul */
	return txp;
}

/* find the label matching *ptr, return NULL if none */
static label *search(label *ptr)		/* uses global lablst */
{
	register label	*rp;
	for(rp = lablst; rp < ptr; rp++)
		if ((rp->name != NULL) && (strcmp(rp->name, ptr->name) == 0))
			return rp;
	return NULL;
}

/* write label links into the compiled-command space */
static void resolve(void)			/* uses global lablst */
{
	register label		*lptr;
	register sedcmd		*rptr, *trptr;

	/* loop through the label table */
	for(lptr = lablst; lptr < lab; lptr++)
		if (lptr->address == NULL)	/* barf if not defined */
		{
			fprintf(stderr, ULABL, lptr->name);
			exit(2);
		}
		else if (lptr->last)		/* if last is non-null */
		{
			rptr = lptr->last;		/* chase it */
			while((trptr = rptr->u.link))	/* resolve refs */
			{
				rptr->u.link = lptr->address;
				rptr = trptr;
			}
			rptr->u.link = lptr->address;
		}
}

/* compile a y (transliterate) command
   ep:		where to compile to
   delim:	end delimiter to look for */
static char *ycomp(char *ep, char delim)
{
	char *tp, *sp;
	int c;

	/* scan the 'from' section for invalid chars */
	for(sp = tp = cp; *tp != delim; tp++)
	{
		if (*tp == '\\')
			tp++;
		if ((*tp == '\n') || (*tp == '\0'))
			return BAD;
	}
	tp++;		/* tp now points at first char of 'to' section */

	/* now rescan the 'from' section */
	while((c = *sp++ & 0x7F) != delim)
	{
		if (c == '\\' && *sp == 'n')
		{
			sp++;
			c = '\n';
		}
		if ((ep[c] = *tp++) == '\\' && *tp == 'n')
		{
			ep[c] = '\n';
			tp++;
		}
		if ((ep[c] == delim) || (ep[c] == '\0'))
			return BAD;
	}

	if (*tp != delim)	/* 'to', 'from' parts have unequal lengths */
		return BAD;

	cp = ++tp;			/* point compile ptr past translit */

	for(c = 0; c < 128; c++)	/* fill in self-map entries in table */
		if (ep[c] == 0)
			ep[c] = c;

	return ep + 0x80;	/* return first free location past table end */
}

/* sedcomp.c ends here */

/* sedexec.c -- axecute compiled form of stream editor commands
   Copyright (C) 1995-2003 Eric S. Raymond
   Copyright (C) 2004-2014 Rene Rebe

   The single entry point of this module is the function execute(). It
may take a string argument (the name of a file to be used as text)  or
the argument NULL which tells it to filter standard input. It executes
the compiled commands in cmds[] on each line in turn.
   The function command() does most of the work.  match() and advance()
are used for matching text against precompiled regular expressions and
dosub() does right-hand-side substitution.  Getline() does text input;
readout() and memcmp() are output and string-comparison utilities.  
*/

/***** shared variables imported from the main ******/

/* main data areas */
extern char	linebuf[];	/* current-line buffer */
extern sedcmd	cmds[];		/* hold compiled commands */
extern long	linenum[];	/* numeric-addresses table */

/* miscellaneous shared variables */
extern int	nflag;		/* -n option flag */
extern int	eargc;		/* scratch copy of argument count */
extern sedcmd	*pending;	/* ptr to command waiting to be executed */

extern int	last_line_used; /* last line address ($) used */

/***** end of imported stuff *****/

#define MAXHOLD		MAXBUF	/* size of the hold space */
#define GENSIZ		MAXBUF	/* maximum genbuf size */

static const char LTLMSG[]	= "sed: line too long\n";

static char	*spend;		/* current end-of-line-buffer pointer */
static long	lnum = 0L;	/* current source line number */

/* append buffer maintenance */
static sedcmd	*appends[MAXAPPENDS];	/* array of ptrs to a,i,c commands */
static sedcmd	**aptr = appends;	/* ptr to current append */

/* genbuf and its pointers */
static char	genbuf[GENSIZ];
static char	*loc1;
static char	*loc2;
static char	*locs;

/* command-logic flags */
static int	lastline;		/* do-line flag */
static int	line_with_newline;	/* line had newline */
static int	jump;			/* jump to cmd's link address if set */
static int	delete;			/* delete command flag */

/* tagged-pattern tracking */
static char	*bracend[MAXTAGS];	/* tagged pattern start pointers */
static char	*brastart[MAXTAGS];	/* tagged pattern end pointers */

/* prototypes */
static char *sed_getline(char *buf, int max);
static char *place(char* asp, char* al1, char* al2);
static int advance(char* lp, char* ep, char** eob);
static int match(char *expbuf, int gf);
static int selected(sedcmd *ipc);
static int substitute(sedcmd *ipc);
static void command(sedcmd *ipc);
static void dosub(char *rhsbuf);
static void dumpto(char *p1, FILE *fp);
static void listto(char *p1, FILE *fp);
static void readout(void);
static void truncated(int h);

/* execute the compiled commands in cmds[] on a file
   file:  name of text source file to be filtered */
void execute(char* file)
{
	register sedcmd		*ipc;		/* ptr to current command */
	char			*execp;		/* ptr to source */

	if (file != NULL)	/* filter text from a named file */ 
    {
        int fd = open(file, O_RDONLY, 0644);
        if (fd == -1) {
            fprintf(stderr, "sed: can't open %s\n", file);
            exit(39);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            exit(41);
        }
    }
		// if (freopen(file, "r", stdin) == NULL)
		// 	fprintf(stderr, "sed: can't open %s\n", file);

	if (pending)		/* there's a command waiting */
	{
		ipc = pending;		/* it will be first executed */
		pending = FALSE;	/*   turn off the waiting flag */
		goto doit;		/*   go to execute it immediately */
	}

	/* here's the main command-execution loop */
	for(;;)
	{
		/* get next line to filter */
		if ((execp = sed_getline(linebuf, MAXBUF+1)) == BAD)
			return;
		spend = execp;

		/* loop through compiled commands, executing them */
		for(ipc = cmds; ipc->command; )
		{
			/* address command to select? - If not address
			   but allbut then invert, that is skip, the commmand */
			if (ipc->addr1 || ipc->flags.allbut) {
				if (!ipc->addr1 || !selected(ipc)) {
					ipc++;	/* not selected, next cmd */
					continue;
				}
			}
	doit:
			command(ipc);	/* execute the command pointed at */

			if (delete)	/* if delete flag is set */
				break;	/* don't exec rest of compiled cmds */

			if (jump)	/* if jump set, follow cmd's link */
			{
				jump = FALSE;
				if ((ipc = ipc->u.link) == 0)
				{
					ipc = cmds;
					break;
				}
			}
			else		/* normal goto next command */
				ipc++;
		}
		/* we've now done all modification commands on the line */

		/* here's where the transformed line is output */
		if (!nflag && !delete)
		{
			fwrite(linebuf, spend - linebuf, 1, stdout);
			if (line_with_newline)
				putc('\n', stdout);
		}

		/* if we've been set up for append, emit the text from it */
		if (aptr > appends)
			readout();

		delete = FALSE;	/* clear delete flag; about to get next cmd */
	}
}

/* is current command selected */
static int selected(sedcmd *ipc)
{
	register char	*p1 = ipc->addr1;	/* point p1 at first address */
	register char	*p2 = ipc->addr2;	/*   and p2 at second */
	unsigned char	c;
	int selected = FALSE;

	if (ipc->flags.inrange)
	{
		selected = TRUE;
		if (*p2 == CEND)
			;
		else if (*p2 == CLNUM)
		{
			c = p2[1];
			if (lnum >= linenum[c])
				ipc->flags.inrange = FALSE;
		}
		else if (match(p2, 0))
			ipc->flags.inrange = FALSE;
	}
	else if (*p1 == CEND)
	{
		if (lastline)
			selected = TRUE;
	}
	else if (*p1 == CLNUM)
	{
		c = p1[1];
		if (lnum == linenum[c]) {
			selected = TRUE;
			if (p2)
				ipc->flags.inrange = TRUE;
		}
	}
	else if (match(p1, 0))
	{
		selected = TRUE;
		if (p2)
			ipc->flags.inrange = TRUE;
	}
	return ipc->flags.allbut ? !selected : selected;
}

/* match RE at expbuf against linebuf; if gf set, copy linebuf from genbuf */
static int _match(char *expbuf, int gf)	/* uses genbuf */
{
	char *p1, *p2, c;

	if (!gf)
	{
		p1 = linebuf;
		locs = NULL;
	}
	else
	{
		if (*expbuf)
			return FALSE;
		/* if the last match was zero length, continue to next */
		if (loc2 - loc1 == 0) {
			loc2++;
		}
		locs = p1 = loc2;
	}
	
	p2 = expbuf;
	if (*p2++)
	{
		loc1 = p1;
		if (*p2 == CCHR && p2[1] != *p1)	/* 1st char is wrong */
			return FALSE;		/*   so fail */
		return advance(p1, p2, NULL);	/* else try to match rest */
	}

	/* quick check for 1st character if it's literal */
	if (*p2 == CCHR)
	{
		c = p2[1];		/* pull out character to search for */
		do {
			if (*p1 != c)
				continue;	/* scan the source string */
			if (advance(p1, p2, NULL)) /* found it, match the rest */
				return loc1 = p1, TRUE;
		} while (*p1++);
		return FALSE;		/* didn't find that first char */
	}

	/* else try for unanchored match of the pattern */
	do {
		if (advance(p1, p2, NULL))
			return loc1 = p1, TRUE;
	} while (*p1++);

	/* if got here, didn't match either way */
	return FALSE;
}

static int match(char *expbuf, int gf)	/* uses genbuf */
{
	const char *loc2i = loc2;
	const int ret = _match(expbuf, gf);
	
	/* if last match was zero length, do not allow a follpwing zero match */
	if (loc2i && loc1 == loc2i && loc2 - loc1 == 0) {
		loc2++;
		return _match(expbuf, gf);
	}
	return ret;
}

/* attempt to advance match pointer by one pattern element
   lp:	source (linebuf) ptr
   ep:	regular expression element ptr */
static int advance(char* lp, char* ep, char** eob)
{
	char	*curlp;		/* save ptr for closures */ 
	char	c;		/* scratch character holder */
	char	*bbeg;
	int	ct;
	signed int	bcount = -1;

	for (;;)
		switch (*ep++)
		{
		case CCHR:		/* literal character */
			if (*ep++ == *lp++)	/* if chars are equal */
				continue;	/* matched */
			return FALSE;		/* else return false */

		case CDOT:		/* anything but newline */
			if (*lp++)		/* first NUL is at EOL */
				continue;	/* keep going if didn't find */
			return FALSE;		/* else return false */

		case CNL:		/* start-of-line */
		case CDOL:		/* end-of-line */
			if (*lp == 0)		/* found that first NUL? */
				continue;	/* yes, keep going */
			return FALSE;		/* else return false */

		case CEOF:		/* end-of-address mark */
			loc2 = lp;		/* set second loc */
			return TRUE;		/* return true */

		case CCL:		/* a closure */
			c = *lp++ & 0177;
			if (ep[c>>3] & bits(c & 07))	/* is char in set? */
			{
				ep += 16;	/* then skip rest of bitmask */
				continue;	/*   and keep going */
			}
			return FALSE;		/* else return false */

		case CBRA:		/* start of tagged pattern */
			brastart[(unsigned char)*ep++] = lp;	/* mark it */
			continue;		/* and go */

		case CKET:		/* end of tagged pattern */
			bcount = *ep;
			if (eob) {
				*eob = lp;
				return TRUE;
			}
			else
				bracend[(unsigned char)*ep++] = lp;    /* mark it */
			continue;		/* and go */

		case CBACK:		/* match back reference */
			bbeg = brastart[(unsigned char)*ep];
			ct = bracend[(unsigned char)*ep++] - bbeg;

			if (memcmp(bbeg, lp, ct) == 0)
			{
				lp += ct;
				continue;
			}
			return FALSE;

		case CBRA|STAR:		/* \(...\)* */
		{
			char *lastlp;
			curlp = lp;

			if (*ep > bcount)
				brastart[(unsigned char)*ep] = bracend[(unsigned char)*ep] = lp;

			while (advance(lastlp=lp, ep+1, &lp)) {
				if (*ep > bcount && lp != lastlp) {
					bracend[(unsigned char)*ep] = lp;    /* mark it */
					brastart[(unsigned char)*ep] = lastlp;
				}
				if (lp == lastlp) break;
			}
			ep++;

			/* FIXME: scan for the brace end */
			while (*ep != CKET)
				ep++;
			ep+=2;

			if (lp == curlp) /* 0 matches */
				continue;
			lp++; /* because the star handling decrements it */
			goto star;
		}
		case CBACK|STAR:	/* \n* */
			bbeg = brastart[(unsigned char)*ep];
			ct = bracend[(unsigned char)*ep++] - bbeg;
			curlp = lp;
			while(memcmp(bbeg, lp, ct) == 0)
				lp += ct;

			while(lp >= curlp)
			{
				if (advance(lp, ep, eob))
					return TRUE;
				lp -= ct;
			}
			return FALSE;

		case CDOT|STAR:		/* match .* */
			curlp = lp;		/* save closure start loc */
			while (*lp++);		/* match anything */ 
			goto star;		/* now look for followers */

		case CCHR|STAR:		/* match <literal char>* */
			curlp = lp;		/* save closure start loc */
			while (*lp++ == *ep);	/* match many of that char */
			ep++;			/* to start of next element */
			goto star;		/* match it and followers */

		case CCL|STAR:		/* match [...]* */
			curlp = lp;		/* save closure start loc */
			do {
				c = *lp++ & 0x7F;	/* match any in set */
			} while (ep[c>>3] & bits(c & 07));
			ep += 16;		/* skip past the set */
			goto star;		/* match followers */

		star:		/* the recursion part of a * or + match */
			if (--lp == curlp) {	/* 0 matches */
				continue;
			}
#if 0
			if (*ep == CCHR)
			{
				c = ep[1];
				do {
					if (*lp != c)
						continue;
					if (advance(lp, ep, eob))
						return TRUE;
				} while (lp-- > curlp);
				return FALSE;
			}

			if (*ep == CBACK)
			{
				c = *(brastart[ep[1]]);
				do {
					if (*lp != c)
						continue;
					if (advance(lp, ep, eob))
						return TRUE;
				} while (lp-- > curlp);
				return FALSE;
			}
#endif
			/* match followers, try shorter match, if needed */
			do {
				if (lp == locs)
					break;
				if (advance(lp, ep, eob))
					return TRUE;
			} while (lp-- > curlp);
			return FALSE;

		default:
			fprintf(stderr, "sed: internal RE error, %o\n", *--ep);
			exit (2);
		}
}

/* perform s command
   ipc:	ptr to s command struct */
static int substitute(sedcmd *ipc)
{
	unsigned int n = 0;
	/* find a match */
	while (match(ipc->u.lhs, n /* use last loc2 for n>0 */)) {
		/* nth 0 is implied 1 */
		n++;
		if (!ipc->nth || n == ipc->nth) {
			dosub(ipc->rhs);		/* perform it once */
			break;
		}
	}
	if (n == 0)
		return FALSE;			/* command fails */

	if (ipc->flags.global)			/* if global flag enabled */
		do {				/* cycle through possibles */
			if (match(ipc->u.lhs, 1)) {	/* found another */
				dosub(ipc->rhs);	/* so substitute */
			}
			else				/* otherwise, */
				break;			/* we're done */
		} while (*loc2);
	return TRUE;				/* we succeeded */
}

/* generate substituted right-hand side (of s command)
   rhsbuf:	where to put the result */
static void dosub(char *rhsbuf)		/* uses linebuf, genbuf, spend */
{
	char	*lp, *sp, *rp;
	int	c;

	/* copy linebuf to genbuf up to location 1 */
	lp = linebuf; sp = genbuf;
	while (lp < loc1) *sp++ = *lp++;

	/* substitute */
	for (rp = rhsbuf; (c = *rp++); )
	{
		if (c & 0200 && (c & 0177) == '0')
		{
			sp = place(sp, loc1, loc2);
			continue;
		}
		else if (c & 0200 && (c &= 0177) >= '1' && c < MAXTAGS+'1')
		{
			sp = place(sp, brastart[c-'1'], bracend[c-'1']);
			continue;
		}
		*sp++ = c & 0177;
		if (sp >= genbuf + MAXBUF)
			fprintf(stderr, LTLMSG);

	}

	/* adjust location pointers and copy reminder */
	lp = loc2;
	{
		long len = loc2 - loc1;
		loc2 = sp - genbuf + linebuf;
		loc1 = loc2 - len;
	}
	while ((*sp++ = *lp++))
		if (sp >= genbuf + MAXBUF)
			fprintf(stderr, LTLMSG);
	lp = linebuf; sp = genbuf;
	while ((*lp++ = *sp++));
	spend = lp-1;
}

/* place chars at *al1...*(al1 - 1) at asp... in genbuf[] */
static char *place(char* asp, char* al1, char* al2)		/* uses genbuf */
{
	while (al1 < al2)
	{
		*asp++ = *al1++;
		if (asp >= genbuf + MAXBUF)
			fprintf(stderr, LTLMSG);
	}
	return asp;
}

/* list the pattern space in visually unambiguous form *p1... to fp
   p1: the source
   fp: output stream to write to */
static void listto(char *p1, FILE *fp)
{
	for (; p1<spend; p1++)
		if (isprint(*p1))
			putc(*p1, fp);		/* pass it through */
		else
		{
			putc('\\', fp);		/* emit a backslash */
			switch(*p1)
			{
			case '\b':	putc('b', fp); break;	/* BS */
			case '\t':	putc('t', fp); break;	/* TAB */
			case '\n':	putc('n', fp); break;	/* NL */
			case '\r':	putc('r', fp); break;	/* CR */
			case '\033':	putc('e', fp); break;	/* ESC */
			default:	fprintf(fp, "%02x", *p1);
			}
		}
	putc('\n', fp);
}

/* write a hex dump expansion of *p1... to fp
   p1: source
   fp: output */
static void dumpto(char *p1, FILE *fp)
{
	for (; p1<spend; p1++)
		fprintf(fp, "%02x", *p1);
	fprintf(fp, "%02x", '\n');
	putc('\n', fp);
}

static void truncated(int h)
{
	static long last = 0L;

	if (lnum == last) return;
	last = lnum;

	fprintf(stderr, "sed: ");
	fprintf(stderr, h ? "hold space" : "line %ld", lnum);
	fprintf(stderr, " truncated to %d characters\n", MAXBUF);
}

/* execute compiled command pointed at by ipc */
static void command(sedcmd *ipc)
{
	static int	didsub;			/* true if last s succeeded */
	static char	holdsp[MAXHOLD];	/* the hold space */
	static char	*hspend = holdsp;	/* hold space end pointer */
	register char	*p1, *p2;
	char		*execp;

	switch(ipc->command)
	{
	case ACMD:		/* append */
		*aptr++ = ipc;
		if (aptr >= appends + MAXAPPENDS)
			fprintf(stderr,
				"sed: too many appends after line %ld\n",
				lnum);
		*aptr = 0;
		break;

	case CCMD:		/* change pattern space */
		delete = TRUE;
		if (!ipc->flags.inrange || lastline)
			printf("%s\n", ipc->u.lhs);		
		break;

	case DCMD:		/* delete pattern space */
		delete = TRUE;
		break;

	case CDCMD:		/* delete a line in hold space */
		p1 = p2 = linebuf;
		while(*p1 != '\n')
			if ((delete = (*p1++ == 0)))
				return;
		p1++;
		while((*p2++ = *p1++)) continue;
		spend = p2-1;
		jump++;
		break;

	case EQCMD:		/* show current line number */
		fprintf(stdout, "%ld\n", lnum);
		break;

	case GCMD:		/* copy hold space to pattern space */
		p1 = linebuf;	p2 = holdsp;	while((*p1++ = *p2++));
		spend = p1-1;
		break;

	case CGCMD:		/* append hold space to pattern space */
		*spend++ = '\n';
		p1 = spend;	p2 = holdsp;
		do {
			if (p1 > linebuf + MAXBUF) {
				truncated(FALSE);
				p1[-1] = 0;
  				break;
			}
		} while((*p1++ = *p2++));

		spend = p1-1;
		break;

	case HCMD:		/* copy pattern space to hold space */
		p1 = holdsp;	p2 = linebuf;	while((*p1++ = *p2++));
		hspend = p1-1;
		break;

	case CHCMD:		/* append pattern space to hold space */
		*hspend++ = '\n';
		p1 = hspend;	p2 = linebuf;
		do {
			if (p1 > holdsp + MAXBUF) {
				truncated(TRUE);
				p1[-1] = 0;
  				break;
			}
		} while((*p1++ = *p2++));

		hspend = p1-1;
		break;

	case ICMD:		/* insert text */
		printf("%s\n", ipc->u.lhs);
		break;

	case BCMD:		/* branch to label */
		jump = TRUE;
		break;

	case LCMD:		/* list text */
		listto(linebuf, (ipc->fout != NULL)?ipc->fout:stdout); break;

	case CLCMD:		/* dump text */
		dumpto(linebuf, (ipc->fout != NULL)?ipc->fout:stdout); break;

	case NCMD:		/* read next line into pattern space */
		if (!nflag)
			puts(linebuf);	/* flush out the current line */
		if (aptr > appends)
			readout();	/* do pending a, r commands */
		if ((execp = sed_getline(linebuf, MAXBUF+1)) == BAD)
		{
			pending = ipc;
			delete = TRUE;
			break;
		}
		spend = execp;
		break;

	case CNCMD:		/* append next line to pattern space */
		if (aptr > appends)
			readout();
		*spend++ = '\n';
		if ((execp = sed_getline(spend,
		                         linebuf + MAXBUF+1 - spend)) == BAD)
		{
			pending = ipc;
			delete = TRUE;
			break;
		}
		spend = execp;
		break;

	case PCMD:		/* print pattern space */
		puts(linebuf);
		break;

	case CPCMD:		/* print one line from pattern space */
		cpcom:		/* so s command can jump here */
		for(p1 = linebuf; *p1 != '\n' && *p1 != '\0'; )
			putc(*p1++, stdout);
		putc('\n', stdout);
		break;

	case QCMD:		/* quit the stream editor */
		if (!nflag)
			puts(linebuf);	/* flush out the current line */
		if (aptr > appends)
			readout();	/* do any pending a and r commands */
		exit(0);

	case RCMD:		/* read a file into the stream */
		*aptr++ = ipc;
		if (aptr >= appends + MAXAPPENDS)
			fprintf(stderr,
				"sed: too many reads after line %ld\n",
				lnum);
		*aptr = 0;
		break;

	case SCMD:		/* substitute RE */
		didsub = substitute(ipc);
		if (ipc->flags.print && didsub)
		{
			if (ipc->flags.print == TRUE)
				puts(linebuf);
			else
				goto cpcom;
		}
		if (didsub && ipc->fout)
			fprintf(ipc->fout, "%s\n", linebuf);
		break;

	case TCMD:		/* branch on last s successful */
	case CTCMD:		/* branch on last s failed */
		if (didsub == (ipc->command == CTCMD))
			break;		/* no branch if last s failed, else */
		didsub = FALSE;
		jump = TRUE;		/*  set up to jump to assoc'd label */
		break;

	case CWCMD:		/* write one line from pattern space */
		for(p1 = linebuf; *p1 != '\n' && *p1 != '\0'; )
			putc(*p1++, ipc->fout);
		putc('\n', ipc->fout);
		break;

	case WCMD:		/* write pattern space to file */
		fprintf(ipc->fout, "%s\n", linebuf);
		break;

	case XCMD:		/* exchange pattern and hold spaces */
		p1 = linebuf;	p2 = genbuf;	while((*p2++ = *p1++)) continue;
		p1 = holdsp;	p2 = linebuf;	while((*p2++ = *p1++)) continue;
		spend = p2 - 1;
		p1 = genbuf;	p2 = holdsp;	while((*p2++ = *p1++)) continue;
		hspend = p2 - 1;
		break;

	case YCMD:
		p1 = linebuf;	p2 = ipc->u.lhs;
		while((*p1 = p2[(unsigned char)*p1]))
			p1++;
		break;
	}
}

/* get next line of text to be filtered
   buf: where to send the input
   max: max chars to read */
static char *sed_getline(char *buf, int max)
{
	if (fgets(buf, max, stdin) != NULL)
	{
		int c;

		lnum++;			/* note that we got another line */
		/* find the end of the input and overwrite a possible '\n' */
		while (*buf != '\n' && *buf != 0)
		    buf++;
		line_with_newline = *buf == '\n';
		*buf=0;

		/* detect last line - but only if the address was used in a command */
		if  (last_line_used) {
		  if ((c = fgetc(stdin)) != EOF)
#ifndef NOLIBC
			ungetc (c, stdin);
#else
            ungetc (c, STDIN_FILENO);
#endif
		  else {
			if (eargc == 0)		/* if no more args */
				lastline = TRUE;	/* set a flag */
		  }
		}

		return buf;		/* return ptr to terminating null */ 
	}
	else
	{
		return BAD;
	}
}

/* write file indicated by r command to output */
static void readout(void)
{
	register int	t;	/* hold input char or EOF */
	FILE		*fi;	/* ptr to file to be read */

	aptr = appends - 1;	/* arrange for pre-increment to work right */
	while(*++aptr)
		if ((*aptr)->command == ACMD)		/* process "a" cmd */
			printf("%s\n", (*aptr)->u.lhs);
		else					/* process "r" cmd */
		{
			if ((fi = fopen((*aptr)->u.lhs, "r")) == NULL)
				continue;
			while((t = getc(fi)) != EOF)
				putc((char) t, stdout);
			fclose(fi);
		}
	aptr = appends;		/* reset the append ptr */
	*aptr = 0;
}

/* sedexec.c ends here */