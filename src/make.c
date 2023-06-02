/*************************************************************************
 *
 *  m a k e :   m a k e . c
 *
 *  Do the actual making for make plus system dependent stuff
 *========================================================================
 * Edition history
 *
 *  #    Date                         Comments                       By
 * --- -------- ---------------------------------------------------- ---
 *   1    ??                                                         ??
 *   2 01.07.89 $<,$* bugs fixed                                     RAL
 *   3 23.08.89 (time_t)time((time_t*)0) bug fixed, N_EXISTS added   RAL
 *   4 30.08.89 leading sp. in cmd. output eliminated, indention ch. PSH,RAL
 *   5 03.09.89 :: time fixed, error output -> stderr, N_ERROR intr.
 *              fixed LZ elimintaed                                  RAL
 *   6 07.09.89 implmacro, DF macros,debug stuff added               RAL
 *   7 09.09.89 tos support added                                    PHH,RAL
 *   8 17.09.89 make1 arg. fixed, N_EXEC introduced                  RAL
 * ------------ Version 2.0 released ------------------------------- RAL
 *     18.05.90 fixed -n bug with silent rules.  (Now echos them.)   PAN
 *
 *************************************************************************/
#ifdef _WIN32
# include <windows.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include "h.h"
#include <sys/stat.h>
#ifdef _WIN32
# include <sys/utime.h>
# ifndef PATH_MAX
#  define PATH_MAX 1024
# endif
#endif
#ifdef unix
# include <sys/wait.h>
# include <unistd.h>
# include <utime.h>
#endif


static int8_t  execflag;


static void
dbgprint(int level, struct name *np, const char *comment)
{
    char *timep;

    if (np) {
	timep = ctime(&np->n_time);
	timep[24] = '\0';
	fputs(&timep[4], stdout);
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
dosh(const char *string, const char *shell)
{
//FIXME: should we not use the 'shell' parameter here?
    return system(string);
}


#ifdef unix
/*
 * Make a file look very outdated after an error trying to make it.
 * Don't remove, this keeps hard links intact.  (kjb)
 */
int
makeold(const char *name)
{
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
