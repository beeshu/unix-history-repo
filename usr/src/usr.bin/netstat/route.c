/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)route.c	5.5 85/09/18";
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <netns/ns.h>

#include <netdb.h>

extern	int kmem;
extern	int nflag;
extern	char *routename(), *netname(), *ns_print();

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	short	b_mask;
	char	b_val;
} bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_DYNAMIC,	'D' },
	{ 0 }
};

/*
 * Print routing tables.
 */
routepr(hostaddr, netaddr, hashsizeaddr)
	off_t hostaddr, netaddr, hashsizeaddr;
{
	struct mbuf mb;
	register struct rtentry *rt;
	register struct mbuf *m;
	register struct bits *p;
	char name[16], *flags;
	struct mbuf **routehash;
	struct ifnet ifnet;
	int hashsize;
	int i, doinghost = 1;

	if (hostaddr == 0) {
		printf("rthost: symbol not in namelist\n");
		return;
	}
	if (netaddr == 0) {
		printf("rtnet: symbol not in namelist\n");
		return;
	}
	if (hashsizeaddr == 0) {
		printf("rthashsize: symbol not in namelist\n");
		return;
	}
	klseek(kmem, hashsizeaddr, 0);
	read(kmem, &hashsize, sizeof (hashsize));
	routehash = (struct mbuf **)malloc( hashsize*sizeof (struct mbuf *) );
	klseek(kmem, hostaddr, 0);
	read(kmem, routehash, hashsize*sizeof (struct mbuf *));
	printf("Routing tables\n");
	printf("%-15.15s %-15.15s %-8.8s %-6.6s %-10.10s %s\n",
		"Destination", "Gateway",
		"Flags", "Refcnt", "Use", "Interface");
again:
	for (i = 0; i < hashsize; i++) {
		if (routehash[i] == 0)
			continue;
		m = routehash[i];
		while (m) {
			struct sockaddr_in *sin;
			struct sockaddr_ns *sns;
			long *l = (long *)&rt->rt_dst;

			klseek(kmem, m, 0);
			read(kmem, &mb, sizeof (mb));
			rt = mtod(&mb, struct rtentry *);
			switch(rt->rt_dst.sa_family) {
		case AF_INET:
			sin = (struct sockaddr_in *)&rt->rt_dst;
			printf("%-15.15s ",
			    (sin->sin_addr.s_addr == 0) ? "default" :
			    (rt->rt_flags & RTF_HOST) ?
			    routename(sin->sin_addr) : netname(sin->sin_addr, 0));
			sin = (struct sockaddr_in *)&rt->rt_gateway;
			printf("%-15.15s ", routename(sin->sin_addr));
			break;
		case AF_NS:
			printf("%-15s ",
			    ns_print((struct sockaddr_ns *)&rt->rt_dst));
			printf("%-15s ",
			    ns_print((struct sockaddr_ns *)&rt->rt_gateway));
			break;
		default:
			printf("%8.8x %8.8x %8.8x %8.8x",*l, l[1], l[2], l[3]);
			l = (long *)&rt->rt_gateway;
			printf("%8.8x %8.8x %8.8x %8.8x",*l, l[1], l[2], l[3]);
			}
			for (flags = name, p = bits; p->b_mask; p++)
				if (p->b_mask & rt->rt_flags)
					*flags++ = p->b_val;
			*flags = '\0';
			printf("%-8.8s %-6d %-10d ", name,
				rt->rt_refcnt, rt->rt_use);
			if (rt->rt_ifp == 0) {
				putchar('\n');
				m = mb.m_next;
				continue;
			}
			klseek(kmem, rt->rt_ifp, 0);
			read(kmem, &ifnet, sizeof (ifnet));
			klseek(kmem, (int)ifnet.if_name, 0);
			read(kmem, name, 16);
			printf("%s%d\n", name, ifnet.if_unit);
			m = mb.m_next;
		}
	}
	if (doinghost) {
		klseek(kmem, netaddr, 0);
		read(kmem, routehash, hashsize*sizeof (struct mbuf *));
		doinghost = 0;
		goto again;
	}
	free(routehash);
}

char *
routename(in)
	struct in_addr in;
{
	register char *cp;
	static char line[50];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;
	char *index();

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag) {
		hp = gethostbyaddr(&in, sizeof (struct in_addr),
			AF_INET);
		if (hp) {
			if ((cp = index(hp->h_name, '.')) &&
			    !strcmp(cp + 1, domain))
				*cp = 0;
			cp = hp->h_name;
		}
	}
	if (cp)
		strcpy(line, cp);
	else {
#define C(x)	((x) & 0xff)
		in.s_addr = ntohl(in.s_addr);
		sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(in, mask)
	struct in_addr in;
	u_long mask;
{
	char *cp = 0;
	static char line[50];
	struct netent *np = 0;
	u_long net;
	register i;
	int subnetshift;

	in.s_addr = ntohl(in.s_addr);
	if (!nflag && in.s_addr) {
		if (mask == 0) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (in.s_addr &~ mask)
				mask = (long)mask >> subnetshift;
		}
		net = in.s_addr & mask;
		while ((mask & 1) == 0)
			mask >>= 1, net >>= 1;
		np = getnetbyaddr(net, AF_INET);
		if (np)
			cp = np->n_name;
	}
	if (cp)
		strcpy(line, cp);
	else if ((in.s_addr & 0xffffff) == 0)
		sprintf(line, "%u", C(in.s_addr >> 24));
	else if ((in.s_addr & 0xffff) == 0)
		sprintf(line, "%u.%u", C(in.s_addr >> 24) , C(in.s_addr >> 16));
	else if ((in.s_addr & 0xff) == 0)
		sprintf(line, "%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8));
	else
		sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	return (line);
}

/*
 * Print routing statistics
 */
rt_stats(off)
	off_t off;
{
	struct rtstat rtstat;

	if (off == 0) {
		printf("rtstat: symbol not in namelist\n");
		return;
	}
	klseek(kmem, off, 0);
	read(kmem, (char *)&rtstat, sizeof (rtstat));
	printf("routing:\n");
	printf("\t%d bad routing redirect%s\n",
		rtstat.rts_badredirect, plural(rtstat.rts_badredirect));
	printf("\t%d dynamically created route%s\n",
		rtstat.rts_dynamic, plural(rtstat.rts_dynamic));
	printf("\t%d new gateway%s due to redirects\n",
		rtstat.rts_newgateway, plural(rtstat.rts_newgateway));
	printf("\t%d destination%s found unreachable\n",
		rtstat.rts_unreach, plural(rtstat.rts_unreach));
	printf("\t%d use%s of a wildcard route\n",
		rtstat.rts_wildcard, plural(rtstat.rts_wildcard));
}
short ns_bh[] = {-1,-1,-1};

char *
ns_print(sns)
struct sockaddr_ns *sns;
{
	register struct ns_addr *dna = &sns->sns_addr;
	long net = ntohl(ns_netof(*dna));
	static char mybuf[50];
	register char *p = mybuf;
	short port = dna->x_port;

	sprintf(p,"%lx:", net);

	while(*p)p++; /* find end of string */

	if (strncmp(ns_bh,dna->x_host.c_host,6)==0)
		sprintf(p,"any");
	else
		sprintf(p,"%x.%x.%x.%x.%x.%x",
			    dna->x_host.c_host[0], dna->x_host.c_host[1],
			    dna->x_host.c_host[2], dna->x_host.c_host[3],
			    dna->x_host.c_host[4], dna->x_host.c_host[5]);
	if (port) {
	while(*p)p++; /* find end of string */
		sprintf(p,":%x",ntohs(port));
	}
	return(mybuf);
}
char *
ns_phost(sns)
struct sockaddr_ns *sns;
{
	register struct ns_addr *dna = &sns->sns_addr;
	long net = ntohl(ns_netof(*dna));
	static char mybuf[50];
	register char *p = mybuf;
	if (strncmp(ns_bh,dna->x_host.c_host,6)==0)
		sprintf(p,"any");
	else
		sprintf(p,"%x,%x,%x",
			   dna->x_host.s_host[0], dna->x_host.s_host[1],
			    dna->x_host.s_host[2]);
	return(mybuf);
}
