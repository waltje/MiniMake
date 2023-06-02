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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#define EXTERN
#define INIT(x) = x
#define INITARRAY
#include "h.h"


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
    putenv(makeflags);

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


void
fatal(const char *msg, const char *a1, int a2)
{
    fprintf(stderr, "%s: ", myname);
    fprintf(stderr, msg, a1, strerror(a2));
    fputc('\n', stderr);

    exit(1);
}
