/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from BSDI kern.c,v 1.2 1998/11/25 22:38:27 don Exp
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "nlm_prot.h"
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfsclient/nfs_lock.h>

#include "lockd.h"
#include "lockd_lock.h"
#include <nfsclient/nfs.h>

#define nfslockdans(_v, _ansp)	\
	((_ansp)->la_vers = _v, \
	nfsclnt(NFSCLNT_LOCKDANS, _ansp))

/* Lock request owner. */
typedef struct __owner {
	pid_t	 pid;				/* Process ID. */
	time_t	 tod;				/* Time-of-day. */
} OWNER;
static OWNER owner;

static char hostname[MAXHOSTNAMELEN + 1];	/* Hostname. */

int	lock_request(LOCKD_MSG *);
int	test_request(LOCKD_MSG *);
void	show(LOCKD_MSG *);
int	unlock_request(LOCKD_MSG *);

/*
 * will break because fifo needs to be repopened when EOF'd
 */
#ifdef SETUID_DAEMON
#define lockd_seteuid(uid)	seteuid(uid)
#else
#define lockd_seteuid(uid)	(1)
#endif

#define d_calls (debug_level > 1)
#define d_args (debug_level > 2)

#define from_addr(sockaddr) \
	(inet_ntoa((sockaddr)->sin_addr))

void
client_cleanup(sig, code)
	int sig;
	int code;
{
	(void)lockd_seteuid(0);
	(void)unlink(_PATH_LCKFIFO);
	exit(-1);
}

/*
 * client_request --
 *	Loop around messages from the kernel, forwarding them off to
 *	NLM servers.
 */
pid_t
client_request(void)
{
	LOCKD_MSG msg;
	fd_set rdset;
	int fd, nr, ret;
	pid_t child;

	/* Recreate the NLM fifo. */
	(void)unlink(_PATH_LCKFIFO);
	(void)umask(S_IXGRP|S_IXOTH);
	if (mkfifo(_PATH_LCKFIFO, S_IWUSR | S_IRUSR)) {
		syslog(LOG_ERR, "mkfifo: %s: %m", _PATH_LCKFIFO);
		exit (1);
	}

	/*
	 * Create a separate process, the client code is really a separate
	 * daemon that shares a lot of code.
	 */
	switch (child = fork()) {
	case -1:
		err(1, "fork");
	case 0:
		break;
	default:
		return (child);
	}

	signal(SIGHUP, client_cleanup);
	signal(SIGTERM, client_cleanup);

	/* Setup. */
	(void)time(&owner.tod);
	owner.pid = getpid();
	(void)gethostname(hostname, sizeof(hostname) - 1);

reopen:
	/* Open the fifo for reading. */
	if ((fd = open(_PATH_LCKFIFO, O_RDONLY /* | O_NONBLOCK */)) < 0)
		syslog(LOG_ERR, "open: %s: %m", _PATH_LCKFIFO);

	/* drop our root priviledges */
	(void)lockd_seteuid(daemon_uid);

	/* Set up the select. */
	FD_ZERO(&rdset);

	for (;;) {
		/* Wait for contact... fifo's return EAGAIN when read with 
		 * no data
		 */
		FD_SET(fd, &rdset);
		(void)select(fd + 1, &rdset, NULL, NULL, NULL);

		/* Read the fixed length message. */
		if ((nr = read(fd, &msg, sizeof(msg))) == sizeof(msg)) {
			if (d_args)
				show(&msg);

			if (msg.lm_version != LOCKD_MSG_VERSION) {
				syslog(LOG_ERR,
				    "unknown msg type: %d", msg.lm_version);
			}
			/*
			 * Send it to the NLM server and don't grant the lock
			 * if we fail for any reason.
			 */
			switch (msg.lm_fl.l_type) {
			case F_RDLCK:
			case F_WRLCK:
				if (msg.lm_getlk)
					ret = test_request(&msg);
				else
					ret = lock_request(&msg);
				break;
			case F_UNLCK:
				ret = unlock_request(&msg);
				break;
			default:
				ret = 1;
				syslog(LOG_ERR,
				    "unknown lock type: %d", msg.lm_fl.l_type);
				break;
			}
			if (ret) {
				struct lockd_ans ans;

				ans.la_msg_ident = msg.lm_msg_ident;
				ans.la_errno = EHOSTUNREACH;

				if (nfslockdans(LOCKD_ANS_VERSION, &ans)) {
					syslog((errno == EPIPE ? LOG_INFO : 
						LOG_ERR), "process %lu: %m",
						(u_long)msg.lm_msg_ident.pid);
				}
			}
		} else if (nr == -1) {
			if (errno != EAGAIN) {
				syslog(LOG_ERR, "read: %s: %m", _PATH_LCKFIFO);
				goto err;
			}
		} else if (nr != 0) {
			syslog(LOG_ERR,
			    "%s: discard %d bytes", _PATH_LCKFIFO, nr);
		} if (nr == 0) {
			close (fd);
			goto reopen;
		}
	}

	/* Reached only on error. */
err:
	(void)lockd_seteuid(0);
	(void)unlink(_PATH_LCKFIFO);
	_exit (1);
}

void
set_auth(cl, ucred)
	CLIENT *cl;
	struct ucred *ucred;
{
        if (cl->cl_auth != NULL)
                cl->cl_auth->ah_ops->ah_destroy(cl->cl_auth);
        cl->cl_auth = authunix_create(hostname,
                        ucred->cr_uid,
                        ucred->cr_groups[0],
                        ucred->cr_ngroups-1,
                        &ucred->cr_groups[1]);
}


/*
 * test_request --
 *	Convert a lock LOCKD_MSG into an NLM request, and send it off.
 */
int
test_request(LOCKD_MSG *msg)
{
	CLIENT *cli;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	char dummy;

	if (d_calls)
		syslog(LOG_DEBUG, "test request: %s: %s to %s",
		    msg->lm_nfsv3 ? "V4" : "V1/3",
		    msg->lm_fl.l_type == F_WRLCK ? "write" : "read",
		    from_addr((struct sockaddr_in *)&msg->lm_addr));

	if (msg->lm_nfsv3) {
		struct nlm4_testargs arg4;

		arg4.cookie.n_bytes = (char *)&msg->lm_msg_ident;
		arg4.cookie.n_len = sizeof(msg->lm_msg_ident);
		arg4.exclusive = msg->lm_fl.l_type == F_WRLCK ? 1 : 0;
		arg4.alock.caller_name = hostname;
		arg4.alock.fh.n_bytes = (char *)&msg->lm_fh;
		arg4.alock.fh.n_len = msg->lm_fh_len;
		arg4.alock.oh.n_bytes = (char *)&owner;
		arg4.alock.oh.n_len = sizeof(owner);
		arg4.alock.svid = msg->lm_msg_ident.pid;
		arg4.alock.l_offset = msg->lm_fl.l_start;
		arg4.alock.l_len = msg->lm_fl.l_len;

		if ((cli = get_client(
		    (struct sockaddr *)&msg->lm_addr,
		    NLM_VERS4)) == NULL)
			return (1);

		set_auth(cli, &msg->lm_cred);
		(void)clnt_call(cli, NLM_TEST_MSG,
		    xdr_nlm4_testargs, &arg4, xdr_void, &dummy, timeout);
	} else {
		struct nlm_testargs arg;

		arg.cookie.n_bytes = (char *)&msg->lm_msg_ident;
		arg.cookie.n_len = sizeof(msg->lm_msg_ident);
		arg.exclusive = msg->lm_fl.l_type == F_WRLCK ? 1 : 0;
		arg.alock.caller_name = hostname;
		arg.alock.fh.n_bytes = (char *)&msg->lm_fh;
		arg.alock.fh.n_len = msg->lm_fh_len;
		arg.alock.oh.n_bytes = (char *)&owner;
		arg.alock.oh.n_len = sizeof(owner);
		arg.alock.svid = msg->lm_msg_ident.pid;
		arg.alock.l_offset = msg->lm_fl.l_start;
		arg.alock.l_len = msg->lm_fl.l_len;

		if ((cli = get_client(
		    (struct sockaddr *)&msg->lm_addr,
		    NLM_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->lm_cred);
		(void)clnt_call(cli, NLM_TEST_MSG,
		    xdr_nlm_testargs, &arg, xdr_void, &dummy, timeout);
	}
	return (0);
}

/*
 * lock_request --
 *	Convert a lock LOCKD_MSG into an NLM request, and send it off.
 */
int
lock_request(LOCKD_MSG *msg)
{
	CLIENT *cli;
	struct nlm4_lockargs arg4;
	struct nlm_lockargs arg;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	char dummy;

	if (d_calls)
		syslog(LOG_DEBUG, "lock request: %s: %s to %s",
		    msg->lm_nfsv3 ? "V4" : "V1/3",
		    msg->lm_fl.l_type == F_WRLCK ? "write" : "read",
		    from_addr((struct sockaddr_in *)&msg->lm_addr));

	if (msg->lm_nfsv3) {
		arg4.cookie.n_bytes = (char *)&msg->lm_msg_ident;
		arg4.cookie.n_len = sizeof(msg->lm_msg_ident);
		arg4.block = msg->lm_wait ? 1 : 0;
		arg4.exclusive = msg->lm_fl.l_type == F_WRLCK ? 1 : 0;
		arg4.alock.caller_name = hostname;
		arg4.alock.fh.n_bytes = (char *)&msg->lm_fh;
		arg4.alock.fh.n_len = msg->lm_fh_len;
		arg4.alock.oh.n_bytes = (char *)&owner;
		arg4.alock.oh.n_len = sizeof(owner);
		arg4.alock.svid = msg->lm_msg_ident.pid;
		arg4.alock.l_offset = msg->lm_fl.l_start;
		arg4.alock.l_len = msg->lm_fl.l_len;
		arg4.reclaim = 0;
		arg4.state = nsm_state;

		if ((cli = get_client(
		    (struct sockaddr *)&msg->lm_addr,
		    NLM_VERS4)) == NULL)
			return (1);

		set_auth(cli, &msg->lm_cred);
		(void)clnt_call(cli, NLM_LOCK_MSG,
		    xdr_nlm4_lockargs, &arg4, xdr_void, &dummy, timeout);
	} else {
		arg.cookie.n_bytes = (char *)&msg->lm_msg_ident;
		arg.cookie.n_len = sizeof(msg->lm_msg_ident);
		arg.block = msg->lm_wait ? 1 : 0;
		arg.exclusive = msg->lm_fl.l_type == F_WRLCK ? 1 : 0;
		arg.alock.caller_name = hostname;
		arg.alock.fh.n_bytes = (char *)&msg->lm_fh;
		arg.alock.fh.n_len = msg->lm_fh_len;
		arg.alock.oh.n_bytes = (char *)&owner;
		arg.alock.oh.n_len = sizeof(owner);
		arg.alock.svid = msg->lm_msg_ident.pid;
		arg.alock.l_offset = msg->lm_fl.l_start;
		arg.alock.l_len = msg->lm_fl.l_len;
		arg.reclaim = 0;
		arg.state = nsm_state;

		if ((cli = get_client(
		    (struct sockaddr *)&msg->lm_addr,
		    NLM_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->lm_cred);
		(void)clnt_call(cli, NLM_LOCK_MSG,
		    xdr_nlm_lockargs, &arg, xdr_void, &dummy, timeout);
	}
	return (0);
}

/*
 * unlock_request --
 *	Convert an unlock LOCKD_MSG into an NLM request, and send it off.
 */
int
unlock_request(LOCKD_MSG *msg)
{
	CLIENT *cli;
	struct nlm4_unlockargs arg4;
	struct nlm_unlockargs arg;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	char dummy;

	if (d_calls)
		syslog(LOG_DEBUG, "unlock request: %s: to %s",
		    msg->lm_nfsv3 ? "V4" : "V1/3",
		    from_addr((struct sockaddr_in *)&msg->lm_addr));

	if (msg->lm_nfsv3) {
		arg4.cookie.n_bytes = (char *)&msg->lm_msg_ident;
		arg4.cookie.n_len = sizeof(msg->lm_msg_ident);
		arg4.alock.caller_name = hostname;
		arg4.alock.fh.n_bytes = (char *)&msg->lm_fh;
		arg4.alock.fh.n_len = msg->lm_fh_len;
		arg4.alock.oh.n_bytes = (char *)&owner;
		arg4.alock.oh.n_len = sizeof(owner);
		arg4.alock.svid = msg->lm_msg_ident.pid;
		arg4.alock.l_offset = msg->lm_fl.l_start;
		arg4.alock.l_len = msg->lm_fl.l_len;

		if ((cli = get_client(
		    (struct sockaddr *)&msg->lm_addr,
		    NLM_VERS4)) == NULL)
			return (1);

		set_auth(cli, &msg->lm_cred);
		(void)clnt_call(cli, NLM_UNLOCK_MSG,
		    xdr_nlm4_unlockargs, &arg4, xdr_void, &dummy, timeout);
	} else {
		arg.cookie.n_bytes = (char *)&msg->lm_msg_ident;
		arg.cookie.n_len = sizeof(msg->lm_msg_ident);
		arg.alock.caller_name = hostname;
		arg.alock.fh.n_bytes = (char *)&msg->lm_fh;
		arg.alock.fh.n_len = msg->lm_fh_len;
		arg.alock.oh.n_bytes = (char *)&owner;
		arg.alock.oh.n_len = sizeof(owner);
		arg.alock.svid = msg->lm_msg_ident.pid;
		arg.alock.l_offset = msg->lm_fl.l_start;
		arg.alock.l_len = msg->lm_fl.l_len;

		if ((cli = get_client(
		    (struct sockaddr *)&msg->lm_addr,
		    NLM_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->lm_cred);
		(void)clnt_call(cli, NLM_UNLOCK_MSG,
		    xdr_nlm_unlockargs, &arg, xdr_void, &dummy, timeout);
	}

	return (0);
}

int
lock_answer(int pid, netobj *netcookie, int result, int *pid_p, int version)
{
	struct lockd_ans ans;

	if (netcookie->n_len != sizeof(ans.la_msg_ident)) {
		if (pid == -1) {	/* we're screwed */
			syslog(LOG_ERR, "inedible nlm cookie");
			return -1;
		}
		ans.la_msg_ident.pid = pid;
		ans.la_msg_ident.msg_seq = -1;
	} else {
		memcpy(&ans.la_msg_ident, netcookie->n_bytes,
		    sizeof(ans.la_msg_ident));
	}

	if (d_calls)
		syslog(LOG_DEBUG, "lock answer: pid %lu: %s %d",
		    ans.la_msg_ident.pid,
		    version == NLM_VERS4 ? "nlmv4" : "nlmv3",
		    result);

	ans.la_set_getlk_pid = 0;
	if (version == NLM_VERS4)
		switch (result) {
		case nlm4_granted:
			ans.la_errno = 0;
			break;
		default:
			ans.la_errno = EACCES;
			break;
		case nlm4_denied:
			if (pid_p == NULL)
				ans.la_errno = EACCES;
			else {
				/* this is an answer to a nlm_test msg */
				ans.la_set_getlk_pid = 1;
				ans.la_getlk_pid = *pid_p;
				ans.la_errno = 0;
			}
			break;
		case nlm4_denied_nolocks:
			ans.la_errno = EAGAIN;
			break;
		case nlm4_blocked:
			return -1;
			/* NOTREACHED */
		case nlm4_denied_grace_period:
			ans.la_errno = EAGAIN;
			break;
		case nlm4_deadlck:
			ans.la_errno = EDEADLK;
			break;
		case nlm4_rofs:
			ans.la_errno = EROFS;
			break;
		case nlm4_stale_fh:
			ans.la_errno = ESTALE;
			break;
		case nlm4_fbig:
			ans.la_errno = EFBIG;
			break;
		case nlm4_failed:
			ans.la_errno = EACCES;
			break;
		}
	else
		switch (result) {
		case nlm_granted:
			ans.la_errno = 0;
			break;
		default:
			ans.la_errno = EACCES;
			break;
		case nlm_denied:
			if (pid_p == NULL)
				ans.la_errno = EACCES;
			else {
				/* this is an answer to a nlm_test msg */
				ans.la_set_getlk_pid = 1;
				ans.la_getlk_pid = *pid_p;
				ans.la_errno = 0;
			}
			break;
		case nlm_denied_nolocks:
			ans.la_errno = EAGAIN;
			break;
		case nlm_blocked:
			return -1;
			/* NOTREACHED */
		case nlm_denied_grace_period:
			ans.la_errno = EAGAIN;
			break;
		case nlm_deadlck:
			ans.la_errno = EDEADLK;
			break;
		}

	if (nfslockdans(LOCKD_ANS_VERSION, &ans)) {
		syslog(((errno == EPIPE || errno == ESRCH) ? 
			LOG_INFO : LOG_ERR), 
			"process %lu: %m", (u_long)ans.la_msg_ident.pid);
		return -1;
	}
	return 0;
}

/*
 * show --
 *	Display the contents of a kernel LOCKD_MSG structure.
 */
void
show(LOCKD_MSG *mp)
{
	static char hex[] = "0123456789abcdef";
	struct fid *fidp;
	fsid_t *fsidp;
	size_t len;
	u_int8_t *p, *t, buf[NFS_SMALLFH*3+1];

	printf("process ID: %lu\n", (long)mp->lm_msg_ident.pid);

	fsidp = (fsid_t *)&mp->lm_fh;
	fidp = (struct fid *)((u_int8_t *)&mp->lm_fh + sizeof(fsid_t));

	for (t = buf, p = (u_int8_t *)mp->lm_fh,
	    len = mp->lm_fh_len;
	    len > 0; ++p, --len) {
		*t++ = '\\';
		*t++ = hex[(*p & 0xf0) >> 4];
		*t++ = hex[*p & 0x0f];
	}
	*t = '\0';

	printf("fh_len %d, fh %s\n", mp->lm_fh_len, buf);

	/* Show flock structure. */
	printf("start %qu; len %qu; pid %lu; type %d; whence %d\n",
	    mp->lm_fl.l_start, mp->lm_fl.l_len, (u_long)mp->lm_fl.l_pid,
	    mp->lm_fl.l_type, mp->lm_fl.l_whence);

	/* Show wait flag. */
	printf("wait was %s\n", mp->lm_wait ? "set" : "not set");
}
