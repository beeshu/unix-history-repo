/*
 * Copyright (c) 1981 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)scanw.c	5.2 (Berkeley) %G%";
#endif /* not lint */

/*
 * scanw and friends
 *
 */

# include	"curses.ext"

/*
 *	This routine implements a scanf on the standard screen.
 */
scanw(fmt, args)
char	*fmt;
int	args; {

	return _sscans(stdscr, fmt, &args);
}
/*
 *	This routine implements a scanf on the given window.
 */
wscanw(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	args; {

	return _sscans(win, fmt, &args);
}
/*
 *	This routine actually executes the scanf from the window.
 *
 *	This is really a modified version of "sscanf".  As such,
 * it assumes that sscanf interfaces with the other scanf functions
 * in a certain way.  If this is not how your system works, you
 * will have to modify this routine to use the interface that your
 * "sscanf" uses.
 */
_sscans(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	*args; {

	char	buf[100];
	FILE	junk;

	junk._flag = _IOREAD|_IOSTRG;
	junk._base = junk._ptr = buf;
	if (wgetstr(win, buf) == ERR)
		return ERR;
	junk._cnt = strlen(buf);
	return _doscan(&junk, fmt, args);
}
