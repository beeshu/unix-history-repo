/* $Id: lib.h,v 1.4 1993/09/18 03:39:49 jkh Exp $ */

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * Include and define various things wanted by the library routines.
 *
 */

#ifndef _INST_LIB_LIB_H_
#define _INST_LIB_LIB_H_

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

/* Macros */
#define SUCCESS	(0)
#define	FAIL	(-1)

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#define YES		2
#define NO		1

/* Usually "rm", but often "echo" during debugging! */
#define REMOVE_CMD	"rm"

/* Usually "rm", but often "echo" during debugging! */
#define RMDIR_CMD	"rmdir"

/* Where we put logging information */
#define LOG_DIR		"/var/db/pkg"

/* The names of our "special" files */
#define CONTENTS_FNAME	"+CONTENTS"
#define COMMENT_FNAME	"+COMMENT"
#define DESC_FNAME	"+DESC"
#define INSTALL_FNAME	"+INSTALL"
#define DEINSTALL_FNAME	"+DEINSTALL"
#define REQUIRE_FNAME	"+REQUIRE"

#define CMD_CHAR	'@'	/* prefix for extended PLIST cmd */


enum _plist_t {
    PLIST_FILE, PLIST_CWD, PLIST_CMD, PLIST_CHMOD,
    PLIST_CHOWN, PLIST_CHGRP, PLIST_COMMENT,
    PLIST_IGNORE, PLIST_NAME, PLIST_UNEXEC
};
typedef enum _plist_t plist_t;

/* Types */
typedef unsigned int Boolean;

struct _plist {
    struct _plist *prev, *next;
    char *name;
    Boolean marked;
    plist_t type;
};
typedef struct _plist *PackingList;

struct _pack {
    struct _plist *head, *tail;
};
typedef struct _pack Package;

/* Prototypes */
/* Misc */
int		vsystem(const char *, ...);
void		cleanup(int);
char		*make_playpen(char *);
void		leave_playpen(void);
char		*where_playpen(void);

/* String */
char 		*get_dash_string(char **);
char		*copy_string(char *);
Boolean		suffix(char *, char *);
void		nuke_suffix(char *);
void		str_lowercase(char *);

/* File */
Boolean		fexists(char *);
Boolean		isdir(char *);
Boolean		isempty(char *);
char		*get_file_contents(char *);
void		write_file(char *, char *);
void		copy_file(char *, char *, char *);
void		copy_hierarchy(char *, char *, Boolean);
int		delete_hierarchy(char *, Boolean);
int		unpack(char *, char *);
void		format_cmd(char *, char *, char *, char *);

/* Msg */
void		upchuck(const char *);
void		barf(const char *, ...);
void		whinge(const char *, ...);
Boolean		y_or_n(Boolean, const char *, ...);

/* Packing list */
PackingList	new_plist_entry(void);
PackingList	last_plist(Package *);
Boolean		in_plist(Package *, plist_t);
void		plist_delete(Package *, Boolean, plist_t, char *);
void		free_plist(Package *);
void		mark_plist(Package *);
void		csum_plist_entry(char *, PackingList);
void		add_plist(Package *, plist_t, char *);
void		add_plist_top(Package *, plist_t, char *);
void		write_plist(Package *, FILE *);
void		read_plist(Package *, FILE *);
int		plist_cmd(char *, char **);
void		delete_package(Boolean, Package *);

/* For all */
void		usage(const char *, const char *, ...);
int		pkg_perform(char **);

/* Externs */
extern Boolean	Verbose;
extern Boolean	Fake;
extern int	AutoAnswer;

#endif /* _INST_LIB_LIB_H_ */
