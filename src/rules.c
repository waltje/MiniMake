/*************************************************************************
 *
 *  m a k e :   r u l e s . c
 *
 *  Control of the implicit suffix rules
 *========================================================================
 * Edition history
 *
 *  #    Date                         Comments                       By
 * --- -------- ---------------------------------------------------- ---
 *   1    ??                                                         ??
 *   2 01.07.89 $<,$* bugs fixed, impl. r. ending in expl. r. added  RAL
 *   3 23.08.89 suffix as macro, testname intr., algorithem to find
 *              source dep. made more intelligent (see Readme3)      RAL
 *   4 30.08.89 indention changed                                    PSH,RAL
 *   5 03.09.89 fixed LZ eliminated                                  RAL
 *   6 07.09.89 rules of type '.c', .DEFAULT added, dep. search impr.RAL
 * ------------ Version 2.0 released ------------------------------- RAL
 *
 *************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "h.h"


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
