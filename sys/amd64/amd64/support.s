/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <machine/asmacros.h>
#include <machine/cputypes.h>
#include <machine/pmap.h>
#include <machine/specialreg.h>

#include "assym.s"


	.text

/* fillw(pat, base, cnt) */  
/*       %rdi,%rsi, %rdx */
ENTRY(fillw)
	movq	%rdi,%rax   
	movq	%rsi,%rdi
	movq	%rdx,%rcx
	cld
	rep
	stosw
	ret

/*****************************************************************************/
/* copyout and fubyte family                                                 */
/*****************************************************************************/
/*
 * Access user memory from inside the kernel. These routines and possibly
 * the math- and DOS emulators should be the only places that do this.
 *
 * We have to access the memory with user's permissions, so use a segment
 * selector with RPL 3. For writes to user space we have to additionally
 * check the PTE for write permission, because the 386 does not check
 * write permissions when we are executing with EPL 0. The 486 does check
 * this if the WP bit is set in CR0, so we can use a simpler version here.
 *
 * These routines set curpcb->onfault for the time they execute. When a
 * protection violation occurs inside the functions, the trap handler
 * returns to *curpcb->onfault instead of the function.
 */

/*
 * copyout(from_kernel, to_user, len)  - MP SAFE
 *         %rdi,        %rsi,    %rdx
 */
ENTRY(copyout)
	movq	PCPU(CURPCB),%rax
	movq	$copyout_fault,PCB_ONFAULT(%rax)
	testq	%rdx,%rdx			/* anything to do? */
	jz	done_copyout

	/*
	 * Check explicitly for non-user addresses.  If 486 write protection
	 * is being used, this check is essential because we are in kernel
	 * mode so the h/w does not provide any protection against writing
	 * kernel addresses.
	 */

	/*
	 * First, prevent address wrapping.
	 */
	movq	%rsi,%rax
	addq	%rdx,%rax
	jc	copyout_fault
/*
 * XXX STOP USING VM_MAXUSER_ADDRESS.
 * It is an end address, not a max, so every time it is used correctly it
 * looks like there is an off by one error, and of course it caused an off
 * by one error in several places.
 */
	movq	$VM_MAXUSER_ADDRESS,%rcx
	cmpq	%rcx,%rax
	ja	copyout_fault

	xchgq	%rdi, %rsi
	/* bcopy(%rsi, %rdi, %rdx) */
	movq	%rdx,%rcx

	shrq	$3,%rcx
	cld
	rep
	movsq
	movb	%dl,%cl
	andb	$7,%cl
	rep
	movsb

done_copyout:
	xorq	%rax,%rax
	movq	PCPU(CURPCB),%rdx
	movq	%rax,PCB_ONFAULT(%rdx)
	ret

	ALIGN_TEXT
copyout_fault:
	movq	PCPU(CURPCB),%rdx
	movq	$0,PCB_ONFAULT(%rdx)
	movq	$EFAULT,%rax
	ret

/*
 * copyin(from_user, to_kernel, len) - MP SAFE
 *        %rdi,      %rsi,      %rdx
 */
ENTRY(copyin)
	movq	PCPU(CURPCB),%rax
	movq	$copyin_fault,PCB_ONFAULT(%rax)
	testq	%rdx,%rdx			/* anything to do? */
	jz	done_copyin

	/*
	 * make sure address is valid
	 */
	movq	%rdi,%rax
	addq	%rdx,%rax
	jc	copyin_fault
	movq	$VM_MAXUSER_ADDRESS,%rcx
	cmpq	%rcx,%rax
	ja	copyin_fault

	xchgq	%rdi, %rsi
	movq	%rdx, %rcx
	movb	%cl,%al
	shrq	$3,%rcx				/* copy longword-wise */
	cld
	rep
	movsq
	movb	%al,%cl
	andb	$7,%cl				/* copy remaining bytes */
	rep
	movsb

done_copyin:
	xorq	%rax,%rax
	movq	PCPU(CURPCB),%rdx
	movq	%rax,PCB_ONFAULT(%rdx)
	ret

	ALIGN_TEXT
copyin_fault:
	movq	PCPU(CURPCB),%rdx
	movq	$0,PCB_ONFAULT(%rdx)
	movq	$EFAULT,%rax
	ret

/*
 * casuptr.  Compare and set user pointer.  Returns -1 or the current value.
 *        dst = %rdi, old = %rsi, new = %rdx
 */
ENTRY(casuptr)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-4,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

	movq	%rsi, %rax			/* old */
	cmpxchgq %rdx, (%rdi)			/* new = %rdx */

	/*
	 * The old value is in %eax.  If the store succeeded it will be the
	 * value we expected (old) from before the store, otherwise it will
	 * be the current value.
	 */

	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	$0,PCB_ONFAULT(%rcx)
	ret

/*
 * fu{byte,sword,word} - MP SAFE
 *
 *	Fetch a byte (sword, word) from user memory
 *	%rdi
 */
ENTRY(fuword64)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-8,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

	movq	(%rdi),%rax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

ENTRY(fuword32)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-4,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

# XXX use the 64 extend
	xorq	%rax, %rax
	movl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

ENTRY(fuword)
	jmp	fuword32

/*
 * These two routines are called from the profiling code, potentially
 * at interrupt time. If they fail, that's okay, good things will
 * happen later. Fail all the time for now - until the trap code is
 * able to deal with this.
 */
ALTENTRY(suswintr)
ENTRY(fuswintr)
	movq	$-1,%rax
	ret

/*
 * fuword16 - MP SAFE
 */
ENTRY(fuword16)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-2,%rax
	cmpq	%rax,%rdi
	ja	fusufault

# XXX use the 64 extend
	xorq	%rax, %rax
	movzwl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

/*
 * fubyte - MP SAFE
 */
ENTRY(fubyte)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-1,%rax
	cmpq	%rax,%rdi
	ja	fusufault

# XXX use the 64 extend
	xorq	%rax, %rax
	movzbl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

	ALIGN_TEXT
fusufault:
	movq	PCPU(CURPCB),%rcx
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%rcx)
	decq	%rax
	ret

/*
 * su{byte,sword,word} - MP SAFE
 *
 *	Write a byte (word, longword) to user memory
 * addr = %rdi,  value = %rsi
 */
ENTRY(suword64)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-8,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movq	%rsi,(%rdi)
	xorq	%rax,%rax
	movq	PCPU(CURPCB),%rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

ENTRY(suword32)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-4,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movl	%esi,(%rdi)
	xorq	%rax,%rax
	movq	PCPU(CURPCB),%rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

ENTRY(suword)
	jmp	suword32

/*
 * suword16 - MP SAFE
 */
ENTRY(suword16)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-2,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movw	%si,(%rdi)
	xorq	%rax,%rax
	movq	PCPU(CURPCB),%rcx		/* restore trashed register */
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

/*
 * subyte - MP SAFE
 */
ENTRY(subyte)
	movq	PCPU(CURPCB),%rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS-1,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movl	%esi, %eax
	movb	%al,(%rdi)
	xorq	%rax,%rax
	movq	PCPU(CURPCB),%rcx		/* restore trashed register */
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

/*
 * copyinstr(from, to, maxlen, int *lencopied) - MP SAFE
 *           %rdi, %rsi, %rdx, %rcx
 *
 *	copy a string from from to to, stop when a 0 character is reached.
 *	return ENAMETOOLONG if string is longer than maxlen, and
 *	EFAULT on protection violations. If lencopied is non-zero,
 *	return the actual length in *lencopied.
 */
ENTRY(copyinstr)
	movq	%rdx, %r8			/* %r8 = maxlen */
	movq	%rcx, %r9			/* %r9 = *len */
	xchgq	%rdi, %rsi			/* %rdi = from, %rsi = to */
	movq	PCPU(CURPCB),%rcx
	movq	$cpystrflt,PCB_ONFAULT(%rcx)

	movq	$VM_MAXUSER_ADDRESS,%rax

	/* make sure 'from' is within bounds */
	subq	%rsi,%rax
	jbe	cpystrflt

	/* restrict maxlen to <= VM_MAXUSER_ADDRESS-from */
	cmpq	%rdx,%rax
	jae	1f
	movq	%rax,%rdx
	movq	%rax,%r8
1:
	incq	%rdx
	cld

2:
	decq	%rdx
	jz	3f

	lodsb
	stosb
	orb	%al,%al
	jnz	2b

	/* Success -- 0 byte reached */
	decq	%rdx
	xorq	%rax,%rax
	jmp	cpystrflt_x
3:
	/* rdx is zero - return ENAMETOOLONG or EFAULT */
	movq	$VM_MAXUSER_ADDRESS,%rax
	cmpq	%rax,%rsi
	jae	cpystrflt
4:
	movq	$ENAMETOOLONG,%rax
	jmp	cpystrflt_x

cpystrflt:
	movq	$EFAULT,%rax

cpystrflt_x:
	/* set *lencopied and return %eax */
	movq	PCPU(CURPCB),%rcx
	movq	$0,PCB_ONFAULT(%rcx)

	testq	%r9,%r9
	jz	1f
	subq	%rdx,%r8
	movq	%r8,(%r9)
1:
	ret


/*
 * copystr(from, to, maxlen, int *lencopied) - MP SAFE
 *         %rdi, %rsi, %rdx, %rcx
 */
ENTRY(copystr)
	movq	%rdx, %r8			/* %r8 = maxlen */

	xchgq	%rdi, %rsi
	incq	%rdx
	cld
1:
	decq	%rdx
	jz	4f
	lodsb
	stosb
	orb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached */
	decq	%rdx
	xorq	%rax,%rax
	jmp	6f
4:
	/* rdx is zero -- return ENAMETOOLONG */
	movq	$ENAMETOOLONG,%rax

6:

	testq	%rcx, %rcx
	jz	7f
	/* set *lencopied and return %rax */
	subq	%rdx, %r8
	movq	%r8, (%rcx)
7:
	ret

/*
 * Handling of special 386 registers and descriptor tables etc
 * %rdi
 */
/* void lgdt(struct region_descriptor *rdp); */
ENTRY(lgdt)
	/* reload the descriptor table */
	lgdt	(%rdi)

	/* flush the prefetch q */
	jmp	1f
	nop
1:
	movl	$KDSEL, %eax
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%fs		/* Beware, use wrmsr to set 64 bit base */
	mov	%ax,%gs
	mov	%ax,%ss

	/* reload code selector by turning return into intersegmental return */
	popq	%rax
	pushq	$KCSEL
	pushq	%rax
	lretq
