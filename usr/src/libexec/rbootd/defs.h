/*
 * Copyright (c) 1992 Regents of the University of California.
 * Copyright (c) 1988, 1992 The University of Utah and the Center
 *	for Software Science (CSS).
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Center for Software Science of the University of Utah Computer
 * Science Department.  CSS requests users of this software to return
 * to css-dist@cs.utah.edu any improvements that they make and grant
 * CSS redistribution rights.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)defs.h	5.1 (Berkeley) %G%
 *
 * Utah $Hdr: defs.h 3.1 92/07/06$
 * Author: Jeff Forys, University of Utah CSS
 */

#include <stdio.h>
#include <signal.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "rmp.h"
#include "rmp_var.h"

/*
**  Common #define's and external variables.  All other files should
**  include this.
*/

/*
 *  This may be defined in <sys/param.h>, if not, it's defined here.
 */
#ifndef	MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN 64
#endif

/*
 *  SIGUSR1 and SIGUSR2 are defined in <signal.h> for 4.3BSD systems.
 */
#ifndef SIGUSR1
#define	SIGUSR1 SIGEMT
#endif
#ifndef SIGUSR2
#define	SIGUSR2 SIGFPE
#endif

/*
 *  These can be faster & more efficient than strcmp()/strncmp()...
 */
#define	STREQN(s1,s2)		((*s1 == *s2) && (strcmp(s1,s2) == 0))
#define	STRNEQN(s1,s2,n)	((*s1 == *s2) && (strncmp(s1,s2,n) == 0))

/*
 *  Configuration file limitations.
 */
#define	C_MAXFILE	10		/* max number of boot-able files */
#define	C_LINELEN	1024		/* max length of line */

/*
 *  Direction of packet (used as argument to DispPkt).
 */
#define	DIR_RCVD	0
#define	DIR_SENT	1
#define	DIR_NONE	2

/*
 *  These need not be functions, so...
 */
#define	FreeStr(str)	free((char *) str)
#define	FreeClient(cli)	free((char *) cli)
#define	GenSessID()	(++SessionID ? SessionID: ++SessionID)

/*
 *  Converting an Ethernet address to a string is done in many routines.
 *  Using `rmp.hp_hdr.saddr' works because this field is *never* changed;
 *  it will *always* contain the source address of the packet.
 */
extern	char *GetEtherAddr();
#define	EnetStr(rptr)	GetEtherAddr(&(rptr)->rmp.hp_hdr.saddr[0])

/*
 *  Every machine we can boot will have one of these allocated for it
 *  (unless there are no restrictions on who we can boot).
 */
typedef struct client_s {
	u_char			addr[RMP_ADDRLEN];	/* addr of machine */
	char			*files[C_MAXFILE];	/* boot-able files */
	struct client_s		*next;			/* ptr to next */
} CLIENT;

/*
 *  Every active connection has one of these allocated for it.
 */
typedef struct rmpconn_s {
	struct rmp_packet	rmp;			/* RMP packet */
	int			rmplen;			/* length of packet */
	struct timeval		tstamp;			/* last time active */
	int			bootfd;			/* open boot file */
	struct rmpconn_s	*next;			/* ptr to next */
} RMPCONN;

/*
 *  All these variables are defined in "conf.c".
 */
extern	char	*ProgName;		/* path-stripped argv[0] */
extern	char	MyHost[];		/* this hosts' name */
extern	int	MyPid;			/* this processes' ID */
extern	int	DebugFlg;		/* set true if debugging */
extern	int	BootAny;		/* set true if we can boot anyone */

extern	char	*ConfigFile;		/* configuration file */
extern	char	*DfltConfig;		/* default configuration file */
extern	char	*DbgFile;		/* debug output file */
extern	char	*PidFile;		/* file containing pid of server */
extern	char	*BootDir;		/* directory w/boot files */

extern	FILE	*DbgFp;			/* debug file pointer */
extern	char	*IntfName;		/* interface we are attached to */

extern	u_short	SessionID;		/* generated session ID */

extern	char	*BootFiles[];		/* list of boot files */

extern	CLIENT	*Clients;		/* list of addrs we'll accept */
extern	RMPCONN	*RmpConns;		/* list of active connections */

extern	char	RmpMcastAddr[];		/* RMP multicast address */

/*
 * Some common routines that generally cause lint to complain.
 */
#if defined(lint) && !defined(BSD4_4)
extern	char	*malloc();
#endif
