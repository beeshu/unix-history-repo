/*-
 * Copyright (c) 1994-1995 S�ren Schmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer 
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  $Id: linux_dummy.c,v 1.9 1998/12/19 02:55:33 julian Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <i386/linux/linux.h>
#include <i386/linux/linux_proto.h>

#define DUMMY(s) 							\
int									\
linux_ ## s(struct proc *p, struct linux_ ## s ## _args *args)		\
{									\
	return (unsupported_msg(p, #s));				\
}									\
struct __hack

static int
unsupported_msg(struct proc *p, const char *fname)
{
	printf("Linux-emul(%ld): %s() not supported\n", (long)p->p_pid, fname);
	return (ENOSYS);
}

DUMMY(setup);
DUMMY(break);
DUMMY(stat);
DUMMY(mount);
DUMMY(umount);
DUMMY(stime);
DUMMY(ptrace);
DUMMY(fstat);
DUMMY(stty);
DUMMY(gtty);
DUMMY(ftime);
DUMMY(prof);
DUMMY(phys);
DUMMY(lock);
DUMMY(mpx);
DUMMY(ulimit);
DUMMY(olduname);
DUMMY(ustat);
DUMMY(ioperm);
DUMMY(ksyslog);
DUMMY(uname);
DUMMY(vhangup);
DUMMY(idle);
DUMMY(vm86);
DUMMY(swapoff);
DUMMY(sysinfo);
DUMMY(modify_ldt);
DUMMY(adjtimex);
DUMMY(create_module);
DUMMY(init_module);
DUMMY(delete_module);
DUMMY(get_kernel_syms);
DUMMY(quotactl);
DUMMY(bdflush);
