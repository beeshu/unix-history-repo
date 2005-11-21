/* s_cosf.c -- float version of s_cos.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef lint
static char rcsid[] = "$FreeBSD$";
#endif

#include "math.h"
#define	INLINE_KERNEL_COSF
#define	INLINE_KERNEL_SINF
#include "math_private.h"
#include "k_cosf.c"
#include "k_sinf.c"

/* Small multiples of pi/2 rounded to double precision. */
static const double
c1pio2 = 1*M_PI_2,			/* 0x3FF921FB, 0x54442D18 */
c2pio2 = 2*M_PI_2,			/* 0x400921FB, 0x54442D18 */
c3pio2 = 3*M_PI_2,			/* 0x4012D97C, 0x7F3321D2 */
c4pio2 = 4*M_PI_2;			/* 0x401921FB, 0x54442D18 */

static inline float
__kernel_cosdf(double x)
{
	return __kernel_cosf((float)x, x - (float)x);
}

static inline float
__kernel_sindf(double x)
{
	return __kernel_sinf((float)x, x - (float)x, 1);
}

float
cosf(float x)
{
	float y[2];
	int32_t n,ix;

	x = fabsf(x);
	GET_FLOAT_WORD(ix,x);

	if(ix <= 0x3f490fda) {		/* |x| ~<= pi/4 */
	    if(ix<0x39800000)		/* |x| < 2**-12 */
		if(((int)x)==0) return 1.0;	/* 1 with inexact if x != 0 */
	    return __kernel_cosf(x,0.0);
	}
	if(ix<=0x407b53d1) {		/* |x| <= ~5*pi/4 */
	    if(ix<=0x4016cbe3)		/* |x| <= ~3pi/4 */
		return -__kernel_sindf(x - c1pio2);
	    else
		return -__kernel_cosdf(x - c2pio2);
	}
	if(ix<=0x40e231d5) {		/* |x| <= ~9*pi/4 */
	    if(ix<=0x40afeddf)		/* |x| <= ~7*pi/4 */
		return __kernel_sindf(x - c3pio2);
	    else
		return __kernel_cosdf(x - c4pio2);
	}

    /* cos(Inf or NaN) is NaN */
	else if (ix>=0x7f800000) return x-x;

    /* general argument reduction needed */
	else {
	    n = __ieee754_rem_pio2f(x,y);
	    switch(n&3) {
		case 0: return  __kernel_cosf(y[0],y[1]);
		case 1: return -__kernel_sinf(y[0],y[1],1);
		case 2: return -__kernel_cosf(y[0],y[1]);
		default:
		        return  __kernel_sinf(y[0],y[1],1);
	    }
	}
}
