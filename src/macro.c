/*************************************************************************
 *
 *  m a k e :   m a c r o . c
 *
 *  Macro control for make
 *========================================================================
 * Edition history
 *
 *  #    Date                         Comments                       By
 * --- -------- ---------------------------------------------------- ---
 *   1    ??                                                         ??
 *   2 23.08.89 Error message corrected                              RAL
 *   3 30.08.89 macro flags added, indention ch.                     PSH,RAL
 *   4 03.09.89 fixed LZ eliminated, doexp(...) changed              RAL
 *   5 06.09.89 M_MAKE added, setDFmacro added                       RAL
 *   6 20.09.89 work around for Minix PC ACK bug                     BE,RAL
 * ------------ Version 2.0 released ------------------------------- RAL
 *
 *************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "h.h"


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
