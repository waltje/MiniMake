/*************************************************************************
 *
 *  m a k e :   h . h
 *
 *  include file for make
 *========================================================================
 * Edition history
 *
 *  #    Date                         Comments                       By
 * --- -------- ---------------------------------------------------- ---
 *   1    ??                                                         ??
 *   2 23.08.89 LZ increased,N_EXISTS added,suffix as macro added    RAL
 *   3 30.08.89 macro flags added, indention changed                 PSH,RAL
 *   4 03.09.89 fixed LZ eliminated, struct str added,...            RAL
 *   5 06.09.89 TABCHAR,M_MAKE added                                 RAL
 *   6 09.09.89 tos support added, EXTERN,INIT,PARMS added           PHH,RAL
 *   7 17.09.89 __STDC__ added, make1 decl. fixed , N_EXEC added     RAL
 * ------------ Version 2.0 released ------------------------------- RAL
 *
 *************************************************************************/
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
# define EXTERN extern
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
extern int8_t getline(struct str *strs, FILE *fd);
extern char *gettok(char **ptr);

/* rules.c */
extern int8_t dyndep(struct name *np, char **pbasename, char **pinputname);
extern void makerules(void);

/* archive.c */
extern int is_archive_ref(const char *name);
extern int archive_stat(const char *name, struct stat *stp);


#endif	/*MAKE_H_H*/
