#!/usr/bin/env perl

# ====================================================================
# Written by Andy Polyakov <appro@fy.chalmers.se> for the OpenSSL
# project. The module is, however, dual licensed under OpenSSL and
# CRYPTOGAMS licenses depending on where you obtain it. For further
# details see http://www.openssl.org/~appro/cryptogams/.
# ====================================================================

# SHA256/512 block procedures for s390x.

# April 2007.
#
# sha256_block_data_order is reportedly >3 times faster than gcc 3.3
# generated code (must be a bug in compiler, as improvement is
# "pathologically" high, in particular in comparison to other SHA
# modules). But the real twist is that it detects if hardware support
# for SHA256 is available and in such case utilizes it. Then the
# performance can reach >6.5x of assembler one for larger chunks.
#
# sha512_block_data_order is ~70% faster than gcc 3.3 generated code.

# January 2009.
#
# Add support for hardware SHA512 and reschedule instructions to
# favour dual-issue z10 pipeline. Hardware SHA256/512 is ~4.7x faster
# than software.

# November 2010.
#
# Adapt for -m31 build. If kernel supports what's called "highgprs"
# feature on Linux [see /proc/cpuinfo], it's possible to use 64-bit
# instructions and achieve "64-bit" performance even in 31-bit legacy
# application context. The feature is not specific to any particular
# processor, as long as it's "z-CPU". Latter implies that the code
# remains z/Architecture specific. On z900 SHA256 was measured to
# perform 2.4x and SHA512 - 13x better than code generated by gcc 4.3.

$flavour = shift;

if ($flavour =~ /3[12]/) {
	$SIZE_T=4;
	$g="";
} else {
	$SIZE_T=8;
	$g="g";
}

$t0="%r0";
$t1="%r1";
$ctx="%r2";	$t2="%r2";
$inp="%r3";
$len="%r4";	# used as index in inner loop

$A="%r5";
$B="%r6";
$C="%r7";
$D="%r8";
$E="%r9";
$F="%r10";
$G="%r11";
$H="%r12";	@V=($A,$B,$C,$D,$E,$F,$G,$H);
$tbl="%r13";
$T1="%r14";
$sp="%r15";

while (($output=shift) && ($output!~/^\w[\w\-]*\.\w+$/)) {}
open STDOUT,">$output";

if ($output =~ /512/) {
	$label="512";
	$SZ=8;
	$LD="lg";	# load from memory
	$ST="stg";	# store to memory
	$ADD="alg";	# add with memory operand
	$ROT="rllg";	# rotate left
	$SHR="srlg";	# logical right shift [see even at the end]
	@Sigma0=(25,30,36);
	@Sigma1=(23,46,50);
	@sigma0=(56,63, 7);
	@sigma1=( 3,45, 6);
	$rounds=80;
	$kimdfunc=3;	# 0 means unknown/unsupported/unimplemented/disabled
} else {
	$label="256";
	$SZ=4;
	$LD="llgf";	# load from memory
	$ST="st";	# store to memory
	$ADD="al";	# add with memory operand
	$ROT="rll";	# rotate left
	$SHR="srl";	# logical right shift
	@Sigma0=(10,19,30);
	@Sigma1=( 7,21,26);
	@sigma0=(14,25, 3);
	@sigma1=(13,15,10);
	$rounds=64;
	$kimdfunc=2;	# magic function code for kimd instruction
}
$Func="sha${label}_block_data_order";
$Table="K${label}";
$stdframe=16*$SIZE_T+4*8;
$frame=$stdframe+16*$SZ;

sub BODY_00_15 {
my ($i,$a,$b,$c,$d,$e,$f,$g,$h) = @_;

$code.=<<___ if ($i<16);
	$LD	$T1,`$i*$SZ`($inp)	### $i
___
$code.=<<___;
	$ROT	$t0,$e,$Sigma1[0]
	$ROT	$t1,$e,$Sigma1[1]
	 lgr	$t2,$f
	xgr	$t0,$t1
	$ROT	$t1,$t1,`$Sigma1[2]-$Sigma1[1]`
	 xgr	$t2,$g
	$ST	$T1,`$stdframe+$SZ*($i%16)`($sp)
	xgr	$t0,$t1			# Sigma1(e)
	algr	$T1,$h			# T1+=h
	 ngr	$t2,$e
	 lgr	$t1,$a
	algr	$T1,$t0			# T1+=Sigma1(e)
	$ROT	$h,$a,$Sigma0[0]
	 xgr	$t2,$g			# Ch(e,f,g)
	$ADD	$T1,`$i*$SZ`($len,$tbl)	# T1+=K[i]
	$ROT	$t0,$a,$Sigma0[1]
	algr	$T1,$t2			# T1+=Ch(e,f,g)
	 ogr	$t1,$b
	xgr	$h,$t0
	 lgr	$t2,$a
	 ngr	$t1,$c
	$ROT	$t0,$t0,`$Sigma0[2]-$Sigma0[1]`
	xgr	$h,$t0			# h=Sigma0(a)
	 ngr	$t2,$b
	algr	$h,$T1			# h+=T1
	 ogr	$t2,$t1			# Maj(a,b,c)
	algr	$d,$T1			# d+=T1
	algr	$h,$t2			# h+=Maj(a,b,c)
___
}

sub BODY_16_XX {
my ($i,$a,$b,$c,$d,$e,$f,$g,$h) = @_;

$code.=<<___;
	$LD	$T1,`$stdframe+$SZ*(($i+1)%16)`($sp)	### $i
	$LD	$t1,`$stdframe+$SZ*(($i+14)%16)`($sp)
	$ROT	$t0,$T1,$sigma0[0]
	$SHR	$T1,$sigma0[2]
	$ROT	$t2,$t0,`$sigma0[1]-$sigma0[0]`
	xgr	$T1,$t0
	$ROT	$t0,$t1,$sigma1[0]
	xgr	$T1,$t2					# sigma0(X[i+1])
	$SHR	$t1,$sigma1[2]
	$ADD	$T1,`$stdframe+$SZ*($i%16)`($sp)	# +=X[i]
	xgr	$t1,$t0
	$ROT	$t0,$t0,`$sigma1[1]-$sigma1[0]`
	$ADD	$T1,`$stdframe+$SZ*(($i+9)%16)`($sp)	# +=X[i+9]
	xgr	$t1,$t0				# sigma1(X[i+14])
	algr	$T1,$t1				# +=sigma1(X[i+14])
___
	&BODY_00_15(@_);
}

$code.=<<___;
.text
.align	64
.type	$Table,\@object
$Table:
___
$code.=<<___ if ($SZ==4);
	.long	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
	.long	0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
	.long	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
	.long	0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
	.long	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
	.long	0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
	.long	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
	.long	0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
	.long	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
	.long	0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
	.long	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
	.long	0xd192e819,0xd6990624,0xf40e3585,0x106aa070
	.long	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
	.long	0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
	.long	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
	.long	0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
___
$code.=<<___ if ($SZ==8);
	.quad	0x428a2f98d728ae22,0x7137449123ef65cd
	.quad	0xb5c0fbcfec4d3b2f,0xe9b5dba58189dbbc
	.quad	0x3956c25bf348b538,0x59f111f1b605d019
	.quad	0x923f82a4af194f9b,0xab1c5ed5da6d8118
	.quad	0xd807aa98a3030242,0x12835b0145706fbe
	.quad	0x243185be4ee4b28c,0x550c7dc3d5ffb4e2
	.quad	0x72be5d74f27b896f,0x80deb1fe3b1696b1
	.quad	0x9bdc06a725c71235,0xc19bf174cf692694
	.quad	0xe49b69c19ef14ad2,0xefbe4786384f25e3
	.quad	0x0fc19dc68b8cd5b5,0x240ca1cc77ac9c65
	.quad	0x2de92c6f592b0275,0x4a7484aa6ea6e483
	.quad	0x5cb0a9dcbd41fbd4,0x76f988da831153b5
	.quad	0x983e5152ee66dfab,0xa831c66d2db43210
	.quad	0xb00327c898fb213f,0xbf597fc7beef0ee4
	.quad	0xc6e00bf33da88fc2,0xd5a79147930aa725
	.quad	0x06ca6351e003826f,0x142929670a0e6e70
	.quad	0x27b70a8546d22ffc,0x2e1b21385c26c926
	.quad	0x4d2c6dfc5ac42aed,0x53380d139d95b3df
	.quad	0x650a73548baf63de,0x766a0abb3c77b2a8
	.quad	0x81c2c92e47edaee6,0x92722c851482353b
	.quad	0xa2bfe8a14cf10364,0xa81a664bbc423001
	.quad	0xc24b8b70d0f89791,0xc76c51a30654be30
	.quad	0xd192e819d6ef5218,0xd69906245565a910
	.quad	0xf40e35855771202a,0x106aa07032bbd1b8
	.quad	0x19a4c116b8d2d0c8,0x1e376c085141ab53
	.quad	0x2748774cdf8eeb99,0x34b0bcb5e19b48a8
	.quad	0x391c0cb3c5c95a63,0x4ed8aa4ae3418acb
	.quad	0x5b9cca4f7763e373,0x682e6ff3d6b2b8a3
	.quad	0x748f82ee5defb2fc,0x78a5636f43172f60
	.quad	0x84c87814a1f0ab72,0x8cc702081a6439ec
	.quad	0x90befffa23631e28,0xa4506cebde82bde9
	.quad	0xbef9a3f7b2c67915,0xc67178f2e372532b
	.quad	0xca273eceea26619c,0xd186b8c721c0c207
	.quad	0xeada7dd6cde0eb1e,0xf57d4f7fee6ed178
	.quad	0x06f067aa72176fba,0x0a637dc5a2c898a6
	.quad	0x113f9804bef90dae,0x1b710b35131c471b
	.quad	0x28db77f523047d84,0x32caab7b40c72493
	.quad	0x3c9ebe0a15c9bebc,0x431d67c49c100d4c
	.quad	0x4cc5d4becb3e42b6,0x597f299cfc657e2a
	.quad	0x5fcb6fab3ad6faec,0x6c44198c4a475817
___
$code.=<<___;
.size	$Table,.-$Table
.globl	$Func
.type	$Func,\@function
$Func:
	sllg	$len,$len,`log(16*$SZ)/log(2)`
___
$code.=<<___ if ($kimdfunc);
	larl	%r1,OPENSSL_s390xcap_P
	lg	%r0,0(%r1)
	tmhl	%r0,0x4000	# check for message-security assist
	jz	.Lsoftware
	lghi	%r0,0
	la	%r1,`2*$SIZE_T`($sp)
	.long	0xb93e0002	# kimd %r0,%r2
	lg	%r0,`2*$SIZE_T`($sp)
	tmhh	%r0,`0x8000>>$kimdfunc`
	jz	.Lsoftware
	lghi	%r0,$kimdfunc
	lgr	%r1,$ctx
	lgr	%r2,$inp
	lgr	%r3,$len
	.long	0xb93e0002	# kimd %r0,%r2
	brc	1,.-4		# pay attention to "partial completion"
	br	%r14
.align	16
.Lsoftware:
___
$code.=<<___;
	lghi	%r1,-$frame
	la	$len,0($len,$inp)
	stm${g}	$ctx,%r15,`2*$SIZE_T`($sp)
	lgr	%r0,$sp
	la	$sp,0(%r1,$sp)
	st${g}	%r0,0($sp)

	larl	$tbl,$Table
	$LD	$A,`0*$SZ`($ctx)
	$LD	$B,`1*$SZ`($ctx)
	$LD	$C,`2*$SZ`($ctx)
	$LD	$D,`3*$SZ`($ctx)
	$LD	$E,`4*$SZ`($ctx)
	$LD	$F,`5*$SZ`($ctx)
	$LD	$G,`6*$SZ`($ctx)
	$LD	$H,`7*$SZ`($ctx)

.Lloop:
	lghi	$len,0
___
for ($i=0;$i<16;$i++)	{ &BODY_00_15($i,@V); unshift(@V,pop(@V)); }
$code.=".Lrounds_16_xx:\n";
for (;$i<32;$i++)	{ &BODY_16_XX($i,@V); unshift(@V,pop(@V)); }
$code.=<<___;
	aghi	$len,`16*$SZ`
	lghi	$t0,`($rounds-16)*$SZ`
	clgr	$len,$t0
	jne	.Lrounds_16_xx

	l${g}	$ctx,`$frame+2*$SIZE_T`($sp)
	la	$inp,`16*$SZ`($inp)
	$ADD	$A,`0*$SZ`($ctx)
	$ADD	$B,`1*$SZ`($ctx)
	$ADD	$C,`2*$SZ`($ctx)
	$ADD	$D,`3*$SZ`($ctx)
	$ADD	$E,`4*$SZ`($ctx)
	$ADD	$F,`5*$SZ`($ctx)
	$ADD	$G,`6*$SZ`($ctx)
	$ADD	$H,`7*$SZ`($ctx)
	$ST	$A,`0*$SZ`($ctx)
	$ST	$B,`1*$SZ`($ctx)
	$ST	$C,`2*$SZ`($ctx)
	$ST	$D,`3*$SZ`($ctx)
	$ST	$E,`4*$SZ`($ctx)
	$ST	$F,`5*$SZ`($ctx)
	$ST	$G,`6*$SZ`($ctx)
	$ST	$H,`7*$SZ`($ctx)
	cl${g}	$inp,`$frame+4*$SIZE_T`($sp)
	jne	.Lloop

	lm${g}	%r6,%r15,`$frame+6*$SIZE_T`($sp)	
	br	%r14
.size	$Func,.-$Func
.string	"SHA${label} block transform for s390x, CRYPTOGAMS by <appro\@openssl.org>"
.comm	OPENSSL_s390xcap_P,16,8
___

$code =~ s/\`([^\`]*)\`/eval $1/gem;
# unlike 32-bit shift 64-bit one takes three arguments
$code =~ s/(srlg\s+)(%r[0-9]+),/$1$2,$2,/gm;

print $code;
close STDOUT;
