/*	archive.c - archive support			Author: Kees J. Bot
 *								13 Nov 1993
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#ifdef unix
# include <unistd.h>
#endif
#include "h.h"


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
