#ifndef lint
static char sccsid[] = "@(#)versys.c	5.3 (Berkeley) %G%";
#endif

#include "uucp.h"
#include <stdio.h>
#include <ctype.h>

#define SNAMESIZE 7

/*
 *	verify system names n1 and n2
 *	return codes:  SUCCESS  |  FAIL
 *
 *	NOTE:
 *		the old calling sequence was versys(name) but is
 *	now versys(&name) so that we can perform aliasing!!!!
 *	See accompanying changes in uucp.c and uux.c
 *		-- Ray Essick, April 27, 1984
 */
versys(nameptr)
register char **nameptr;
{
	register FILE *fp;
	char line[BUFSIZ];
	char *name;

	DEBUG (11, "Before Alias: %s\n", *nameptr);
	uualias(nameptr);			/* alias expansion */
	DEBUG (11, "After Alias: %s\n", *nameptr);
	name = *nameptr;			/* dereference */

	if (strncmp(name, Myname, 7) == 0)
		return SUCCESS;

	fp = fopen(SYSFILE, "r");
	ASSERT(fp != NULL, CANTOPEN, SYSFILE, 0);
	while (cfgets(line, sizeof(line), fp) != NULL) {
		char *targs[100];

		getargs(line, targs, 100);
		if (strncmp(name, targs[0], 7) == SAME) {
			fclose(fp);
			return SUCCESS;
		}
	}
	fclose(fp);
	return FAIL;
}

/*
 *	Works (sort of) like rhost(3) on 4.1[abc] Bsd systems.
 *
 *	Looks for the host in the L.aliases file and returns the
 *	"standard" name by modifying the pointer. The returned
 *	value is saved with malloc(3) so it isn't zapped by
 *	subsequent calls.
 *
 *	Returns:
 *		FAIL		No L.aliases file
 *		SUCCESS		Anything else
 */

uualias(hostptr)
char  **hostptr;			  /* we change it */
{
	FILE * Aliases;			  /* list of aliases */
	char buf[BUFSIZ];
	int atend;
	char *p, *q;
	char *koshername;		 /* "official" name */

	if ((Aliases = fopen(ALIASFILE, "r")) == NULL) {
		DEBUG(11, "No %s file\n", ALIASFILE);
		return FAIL;			  /* no alias file */
	}

	DEBUG (11, "Alias expansion for %s\n", *hostptr);
	while (fgets(buf, sizeof (buf), Aliases)) {
		if (buf[0] == '#')		  /* comment line */
			continue;
		p = &buf[0];
		atend = 0;
		DEBUG(11, "Alias line: %s\n", buf);

		while (!atend) {
			while (isspace(*p) && *p != '\n')
				p++;			  /* skip white space */
			q = p;
			while (!isspace(*q) && *q != '\n')
				q++;			  /* find end */
			if (*q == '\n')
				atend++;		  /* last entry */
			*q = '\0';
			DEBUG(11, "Compare against: %s\n", p);
			if (strcmp(*hostptr, p) == 0)/* match? */ {
				koshername = malloc(strlen(buf) + 1);
				strcpy(koshername, buf); /* save it */
				fclose(Aliases);
				DEBUG(4, "Alias: %s to ", *hostptr);
				DEBUG(4, "%s\n", koshername);
				*hostptr = koshername;	  /* correct one */
				return SUCCESS;		  /* all is well */
			}
			p = q + 1;			  /* try next entry */
		}

	}
	fclose(Aliases);
	DEBUG(11, "Alias doesn't match %s, remains unchanged\n", *hostptr);
	return SUCCESS;				  /* unchanged host */
}
