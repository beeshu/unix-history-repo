/*-
 * Copyright (c) 2002 Tim J. Robbins
 * All rights reserved.
 *
 * Copyright (c) 2013 Ed Schouten <ed@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Test program for c16rtomb() as specified by ISO/IEC 9899:2011.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <uchar.h>

int
main(int argc, char *argv[])
{
	mbstate_t s;
	char buf[MB_LEN_MAX + 1];

	/*
	 * C/POSIX locale.
	 */

	printf("1..1\n");

	/*
	 * If the buffer argument is NULL, c16 is implicitly 0,
	 * c16rtomb() resets its internal state.
	 */
	assert(c16rtomb(NULL, L'\0', NULL) == 1);
	assert(c16rtomb(NULL, 0xdc00, NULL) == 1);

	/* Null wide character. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	assert(c16rtomb(buf, 0, &s) == 1);
	assert((unsigned char)buf[0] == 0 && (unsigned char)buf[1] == 0xcc);

	/* Latin letter A, internal state. */
	assert(c16rtomb(NULL, L'\0', NULL) == 1);
	assert(c16rtomb(NULL, L'A', NULL) == 1);

	/* Latin letter A. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	assert(c16rtomb(buf, L'A', &s) == 1);
	assert((unsigned char)buf[0] == 'A' && (unsigned char)buf[1] == 0xcc);

	/* Unicode character 'Pile of poo'. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	assert(c16rtomb(buf, 0xd83d, &s) == 0);
	assert(c16rtomb(buf, 0xdca9, &s) == (size_t)-1);
	assert(errno == EILSEQ);

	/*
	 * UTF-8.
	 */

	assert(strcmp(setlocale(LC_CTYPE, "en_US.UTF-8"), "en_US.UTF-8") == 0);

	/* Unicode character 'Pile of poo'. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	assert(c16rtomb(buf, 0xd83d, &s) == 0);
	assert(c16rtomb(buf, 0xdca9, &s) == 4);
	assert((unsigned char)buf[0] == 0xf0 && (unsigned char)buf[1] == 0x9f &&
	    (unsigned char)buf[2] == 0x92 && (unsigned char)buf[3] == 0xa9 &&
	    (unsigned char)buf[4] == 0xcc);

	/* Invalid code; 'Pile of poo' without the trail surrogate. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	assert(c16rtomb(buf, 0xd83d, &s) == 0);
	assert(c16rtomb(buf, L'A', &s) == (size_t)-1);
	assert(errno == EILSEQ);

	/* Invalid code; 'Pile of poo' without the lead surrogate. */
	memset(&s, 0, sizeof(s));
	memset(buf, 0xcc, sizeof(buf));
	assert(c16rtomb(buf, 0xdca9, &s) == (size_t)-1);
	assert(errno == EILSEQ);

	printf("ok 1 - c16rtomb()\n");
}
