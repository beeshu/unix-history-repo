/*-
 * Copyright (c) 2009-2010 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by Pawel Jakub Dawidek under sponsorship from
 * the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "hast.h"
#include "hastd.h"
#include "hast_proto.h"
#include "hooks.h"
#include "nv.h"
#include "pjdlog.h"
#include "proto.h"
#include "subr.h"

#include "control.h"

static void
control_set_role_common(struct hastd_config *cfg, struct nv *nvout,
    uint8_t role, struct hast_resource *res, const char *name, unsigned int no)
{
	int oldrole;

	/* Name is always needed. */
	if (name != NULL)
		nv_add_string(nvout, name, "resource%u", no);

	if (res == NULL) {
		assert(cfg != NULL);
		assert(name != NULL);

		TAILQ_FOREACH(res, &cfg->hc_resources, hr_next) {
			if (strcmp(res->hr_name, name) == 0)
				break;
		}
		if (res == NULL) {
			nv_add_int16(nvout, EHAST_NOENTRY, "error%u", no);
			return;
		}
	}
	assert(res != NULL);

	/* Send previous role back. */
	nv_add_string(nvout, role2str(res->hr_role), "role%u", no);

	/* Nothing changed, return here. */
	if (role == res->hr_role)
		return;

	pjdlog_prefix_set("[%s] (%s) ", res->hr_name, role2str(res->hr_role));
	pjdlog_info("Role changed to %s.", role2str(role));

	/* Change role to the new one. */
	oldrole = res->hr_role;
	res->hr_role = role;
	pjdlog_prefix_set("[%s] (%s) ", res->hr_name, role2str(res->hr_role));

	/*
	 * If previous role was primary or secondary we have to kill process
	 * doing that work.
	 */
	if (res->hr_workerpid != 0) {
		if (kill(res->hr_workerpid, SIGTERM) < 0) {
			pjdlog_errno(LOG_WARNING,
			    "Unable to kill worker process %u",
			    (unsigned int)res->hr_workerpid);
		} else if (waitpid(res->hr_workerpid, NULL, 0) !=
		    res->hr_workerpid) {
			pjdlog_errno(LOG_WARNING,
			    "Error while waiting for worker process %u",
			    (unsigned int)res->hr_workerpid);
		} else {
			pjdlog_debug(1, "Worker process %u stopped.",
			    (unsigned int)res->hr_workerpid);
		}
		res->hr_workerpid = 0;
	}

	/* Start worker process if we are changing to primary. */
	if (role == HAST_ROLE_PRIMARY)
		hastd_primary(res);
	pjdlog_prefix_set("%s", "");
	hook_exec(res->hr_exec, "role", res->hr_name, role2str(oldrole),
	    role2str(res->hr_role), NULL);
}

void
control_set_role(struct hast_resource *res, uint8_t role)
{

	control_set_role_common(NULL, NULL, role, res, NULL, 0);
}

static void
control_status_worker(struct hast_resource *res, struct nv *nvout,
    unsigned int no)
{
	struct nv *cnvin, *cnvout;
	const char *str;
	int error;

	cnvin = cnvout = NULL;
	error = 0;

	/*
	 * Prepare and send command to worker process.
	 */
	cnvout = nv_alloc();
	nv_add_uint8(cnvout, HASTCTL_STATUS, "cmd");
	error = nv_error(cnvout);
	if (error != 0) {
		/* LOG */
		goto end;
	}
	if (hast_proto_send(res, res->hr_ctrl, cnvout, NULL, 0) < 0) {
		error = errno;
		/* LOG */
		goto end;
	}

	/*
	 * Receive response.
	 */
	if (hast_proto_recv_hdr(res->hr_ctrl, &cnvin) < 0) {
		error = errno;
		/* LOG */
		goto end;
	}

	error = nv_get_int64(cnvin, "error");
	if (error != 0)
		goto end;

	if ((str = nv_get_string(cnvin, "status")) == NULL) {
		error = ENOENT;
		/* LOG */
		goto end;
	}
	nv_add_string(nvout, str, "status%u", no);
	nv_add_uint64(nvout, nv_get_uint64(cnvin, "dirty"), "dirty%u", no);
	nv_add_uint32(nvout, nv_get_uint32(cnvin, "extentsize"),
	    "extentsize%u", no);
	nv_add_uint32(nvout, nv_get_uint32(cnvin, "keepdirty"),
	    "keepdirty%u", no);
end:
	if (cnvin != NULL)
		nv_free(cnvin);
	if (cnvout != NULL)
		nv_free(cnvout);
	if (error != 0)
		nv_add_int16(nvout, error, "error");
}

static void
control_status(struct hastd_config *cfg, struct nv *nvout,
    struct hast_resource *res, const char *name, unsigned int no)
{

	assert(cfg != NULL);
	assert(nvout != NULL);
	assert(name != NULL);

	/* Name is always needed. */
	nv_add_string(nvout, name, "resource%u", no);

	if (res == NULL) {
		TAILQ_FOREACH(res, &cfg->hc_resources, hr_next) {
			if (strcmp(res->hr_name, name) == 0)
				break;
		}
		if (res == NULL) {
			nv_add_int16(nvout, EHAST_NOENTRY, "error%u", no);
			return;
		}
	}
	assert(res != NULL);
	nv_add_string(nvout, res->hr_provname, "provname%u", no);
	nv_add_string(nvout, res->hr_localpath, "localpath%u", no);
	nv_add_string(nvout, res->hr_remoteaddr, "remoteaddr%u", no);
	switch (res->hr_replication) {
	case HAST_REPLICATION_FULLSYNC:
		nv_add_string(nvout, "fullsync", "replication%u", no);
		break;
	case HAST_REPLICATION_MEMSYNC:
		nv_add_string(nvout, "memsync", "replication%u", no);
		break;
	case HAST_REPLICATION_ASYNC:
		nv_add_string(nvout, "async", "replication%u", no);
		break;
	default:
		nv_add_string(nvout, "unknown", "replication%u", no);
		break;
	}
	nv_add_string(nvout, role2str(res->hr_role), "role%u", no);

	switch (res->hr_role) {
	case HAST_ROLE_PRIMARY:
		assert(res->hr_workerpid != 0);
		/* FALLTHROUGH */
	case HAST_ROLE_SECONDARY:
		if (res->hr_workerpid != 0)
			break;
		/* FALLTHROUGH */
	default:
		return;
	}

	/*
	 * If we are here, it means that we have a worker process, which we
	 * want to ask some questions.
	 */
	control_status_worker(res, nvout, no);
}

void
control_handle(struct hastd_config *cfg)
{
	struct proto_conn *conn;
	struct nv *nvin, *nvout;
	unsigned int ii;
	const char *str;
	uint8_t cmd, role;
	int error;

	if (proto_accept(cfg->hc_controlconn, &conn) < 0) {
		pjdlog_errno(LOG_ERR, "Unable to accept control connection");
		return;
	}

	nvin = nvout = NULL;
	role = HAST_ROLE_UNDEF;

	if (hast_proto_recv_hdr(conn, &nvin) < 0) {
		pjdlog_errno(LOG_ERR, "Unable to receive control header");
		nvin = NULL;
		goto close;
	}

	/* Obtain command code. 0 means that nv_get_uint8() failed. */
	cmd = nv_get_uint8(nvin, "cmd");
	if (cmd == 0) {
		pjdlog_error("Control header is missing 'cmd' field.");
		error = EHAST_INVALID;
		goto close;
	}

	/* Allocate outgoing nv structure. */
	nvout = nv_alloc();
	if (nvout == NULL) {
		pjdlog_error("Unable to allocate header for control response.");
		error = EHAST_NOMEMORY;
		goto close;
	}

	error = 0;

	str = nv_get_string(nvin, "resource0");
	if (str == NULL) {
		pjdlog_error("Control header is missing 'resource0' field.");
		error = EHAST_INVALID;
		goto fail;
	}
	if (cmd == HASTCTL_SET_ROLE) {
		role = nv_get_uint8(nvin, "role");
		switch (role) {
		case HAST_ROLE_INIT:	/* Is that valid to set, hmm? */
		case HAST_ROLE_PRIMARY:
		case HAST_ROLE_SECONDARY:
			break;
		default:
			pjdlog_error("Invalid role received (%hhu).", role);
			error = EHAST_INVALID;
			goto fail;
		}
	}
	if (strcmp(str, "all") == 0) {
		struct hast_resource *res;

		/* All configured resources. */

		ii = 0;
		TAILQ_FOREACH(res, &cfg->hc_resources, hr_next) {
			switch (cmd) {
			case HASTCTL_SET_ROLE:
				control_set_role_common(cfg, nvout, role, res,
				    res->hr_name, ii++);
				break;
			case HASTCTL_STATUS:
				control_status(cfg, nvout, res, res->hr_name,
				    ii++);
				break;
			default:
				pjdlog_error("Invalid command received (%hhu).",
				    cmd);
				error = EHAST_UNIMPLEMENTED;
				goto fail;
			}
		}
	} else {
		/* Only selected resources. */

		for (ii = 0; ; ii++) {
			str = nv_get_string(nvin, "resource%u", ii);
			if (str == NULL)
				break;
			switch (cmd) {
			case HASTCTL_SET_ROLE:
				control_set_role_common(cfg, nvout, role, NULL,
				    str, ii);
				break;
			case HASTCTL_STATUS:
				control_status(cfg, nvout, NULL, str, ii);
				break;
			default:
				pjdlog_error("Invalid command received (%hhu).",
				    cmd);
				error = EHAST_UNIMPLEMENTED;
				goto fail;
			}
		}
	}
	if (nv_error(nvout) != 0)
		goto close;
fail:
	if (error != 0)
		nv_add_int16(nvout, error, "error");

	if (hast_proto_send(NULL, conn, nvout, NULL, 0) < 0)
		pjdlog_errno(LOG_ERR, "Unable to send control response");
close:
	if (nvin != NULL)
		nv_free(nvin);
	if (nvout != NULL)
		nv_free(nvout);
	proto_close(conn);
}

/*
 * Thread handles control requests from the parent.
 */
void *
ctrl_thread(void *arg)
{
	struct hast_resource *res = arg;
	struct nv *nvin, *nvout;
	uint8_t cmd;

	for (;;) {
		if (hast_proto_recv_hdr(res->hr_ctrl, &nvin) < 0) {
			if (sigexit_received)
				pthread_exit(NULL);
			pjdlog_errno(LOG_ERR,
			    "Unable to receive control message");
			kill(getpid(), SIGTERM);
			pthread_exit(NULL);
		}
		cmd = nv_get_uint8(nvin, "cmd");
		if (cmd == 0) {
			pjdlog_error("Control message is missing 'cmd' field.");
			nv_free(nvin);
			continue;
		}
		nv_free(nvin);
		nvout = nv_alloc();
		switch (cmd) {
		case HASTCTL_STATUS:
			if (res->hr_remotein != NULL &&
			    res->hr_remoteout != NULL) {
				nv_add_string(nvout, "complete", "status");
			} else {
				nv_add_string(nvout, "degraded", "status");
			}
			nv_add_uint32(nvout, (uint32_t)res->hr_extentsize,
			    "extentsize");
			if (res->hr_role == HAST_ROLE_PRIMARY) {
				nv_add_uint32(nvout,
				    (uint32_t)res->hr_keepdirty, "keepdirty");
				nv_add_uint64(nvout,
				    (uint64_t)(activemap_ndirty(res->hr_amp) *
				    res->hr_extentsize), "dirty");
			} else {
				nv_add_uint32(nvout, (uint32_t)0, "keepdirty");
				nv_add_uint64(nvout, (uint64_t)0, "dirty");
			}
			break;
		default:
			nv_add_int16(nvout, EINVAL, "error");
			break;
		}
		if (nv_error(nvout) != 0) {
			pjdlog_error("Unable to create answer on control message.");
			nv_free(nvout);
			continue;
		}
		if (hast_proto_send(NULL, res->hr_ctrl, nvout, NULL, 0) < 0) {
			pjdlog_errno(LOG_ERR,
			    "Unable to send reply to control message");
		}
		nv_free(nvout);
	}
	/* NOTREACHED */
	return (NULL);
}
