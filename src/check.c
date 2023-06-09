/*************************************************************************
 *
 *  m a k e :   c h e c k . c
 *
 *  debugging stuff: Check structures for make.
 *========================================================================
 * Edition history
 *
 *  #    Date                         Comments                       By
 * --- -------- ---------------------------------------------------- ---
 *   1    ??                                                         ??
 *   2 23.08.89 adapted to new name tree structure                   RAL
 *   3 30.08.89 indention changed                                    PSH,RAL
 *   4 06.09.89 prt output redirected to stdout                      RAL
 * ------------ Version 2.0 released ------------------------------- RAL
 *
 *************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "h.h"


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
