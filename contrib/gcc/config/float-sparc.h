/* float.h for target with IEEE 32, 64 and 128 bit SPARC floating point formats
   (on sparc-linux long double is 64 bit, while on sparc64-linux 128 bit) */
#ifndef _FLOAT_H_
#define _FLOAT_H_
/* Produced by enquire version 4.3, CWI, Amsterdam */

   /* Radix of exponent representation */
#undef FLT_RADIX
#define FLT_RADIX 2
   /* Number of base-FLT_RADIX digits in the significand of a float */
#undef FLT_MANT_DIG
#define FLT_MANT_DIG 24
   /* Number of decimal digits of precision in a float */
#undef FLT_DIG
#define FLT_DIG 6
   /* Addition rounds to 0: zero, 1: nearest, 2: +inf, 3: -inf, -1: unknown */
#undef FLT_ROUNDS
#define FLT_ROUNDS 1
   /* Difference between 1.0 and the minimum float greater than 1.0 */
#undef FLT_EPSILON
#define FLT_EPSILON 1.19209290e-07F
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised float */
#undef FLT_MIN_EXP
#define FLT_MIN_EXP (-125)
   /* Minimum normalised float */
#undef FLT_MIN
#define FLT_MIN 1.17549435e-38F
   /* Minimum int x such that 10**x is a normalised float */
#undef FLT_MIN_10_EXP
#define FLT_MIN_10_EXP (-37)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable float */
#undef FLT_MAX_EXP
#define FLT_MAX_EXP 128
   /* Maximum float */
#undef FLT_MAX
#define FLT_MAX 3.40282347e+38F
   /* Maximum int x such that 10**x is a representable float */
#undef FLT_MAX_10_EXP
#define FLT_MAX_10_EXP 38

   /* Number of base-FLT_RADIX digits in the significand of a double */
#undef DBL_MANT_DIG
#define DBL_MANT_DIG 53
   /* Number of decimal digits of precision in a double */
#undef DBL_DIG
#define DBL_DIG 15
   /* Difference between 1.0 and the minimum double greater than 1.0 */
#undef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised double */
#undef DBL_MIN_EXP
#define DBL_MIN_EXP (-1021)
   /* Minimum normalised double */
#undef DBL_MIN
#define DBL_MIN 2.2250738585072014e-308
   /* Minimum int x such that 10**x is a normalised double */
#undef DBL_MIN_10_EXP
#define DBL_MIN_10_EXP (-307)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable double */
#undef DBL_MAX_EXP
#define DBL_MAX_EXP 1024
   /* Maximum double */
#undef DBL_MAX
#define DBL_MAX 1.7976931348623157e+308
   /* Maximum int x such that 10**x is a representable double */
#undef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP 308

#if defined(__sparcv9) || defined(__arch64__) || defined(__LONG_DOUBLE_128__)

   /* Number of base-FLT_RADIX digits in the significand of a long double */
#undef LDBL_MANT_DIG
#define LDBL_MANT_DIG 113
   /* Number of decimal digits of precision in a long double */
#undef LDBL_DIG
#define LDBL_DIG 33
   /* Difference between 1.0 and the minimum long double greater than 1.0 */
#undef LDBL_EPSILON
#define LDBL_EPSILON 1.925929944387235853055977942584927319E-34L
   /* Minimum int x such that FLT_RADIX**(x-1) is a normalised long double */
#undef LDBL_MIN_EXP
#define LDBL_MIN_EXP (-16381)
   /* Minimum normalised long double */
#undef LDBL_MIN
#define LDBL_MIN 3.362103143112093506262677817321752603E-4932L
   /* Minimum int x such that 10**x is a normalised long double */
#undef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP (-4931)
   /* Maximum int x such that FLT_RADIX**(x-1) is a representable long double */
#undef LDBL_MAX_EXP
#define LDBL_MAX_EXP 16384
   /* Maximum long double */
#undef LDBL_MAX
#define LDBL_MAX 1.189731495357231765085759326628007016E+4932L
   /* Maximum int x such that 10**x is a representable long double */
#undef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP 4932

#else /* sparc32 */

#undef LDBL_MANT_DIG
#define LDBL_MANT_DIG DBL_MANT_DIG
#undef LDBL_DIG
#define LDBL_DIG DBL_DIG
#undef LDBL_EPSILON
#define LDBL_EPSILON DBL_EPSILON
#undef LDBL_MIN_EXP
#define LDBL_MIN_EXP DBL_MIN_EXP
#undef LDBL_MIN
#define LDBL_MIN DBL_MIN
#undef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP
#undef LDBL_MAX_EXP
#define LDBL_MAX_EXP DBL_MAX_EXP
#undef LDBL_MAX
#define LDBL_MAX DBL_MAX
#undef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP

#endif /* sparc32 */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
   /* The floating-point expression evaluation method.
        -1  indeterminate
         0  evaluate all operations and constants just to the range and
            precision of the type
         1  evaluate operations and constants of type float and double
            to the range and precision of the double type, evaluate
            long double operations and constants to the range and
            precision of the long double type
         2  evaluate all operations and constants to the range and
            precision of the long double type
   */
# undef FLT_EVAL_METHOD
# define FLT_EVAL_METHOD	0

   /* Number of decimal digits to enable rounding to the given number of
      decimal digits without loss of precision.
         if FLT_RADIX == 10^n:  #mantissa * log10 (FLT_RADIX)
         else                :  ceil (1 + #mantissa * log10 (FLT_RADIX))
      where #mantissa is the number of bits in the mantissa of the widest
      supported floating-point type.
   */
# undef DECIMAL_DIG
# if LDBL_MANT_DIG == 53
#  define DECIMAL_DIG	17
# else
#  define DECIMAL_DIG	36
# endif

#endif	/* C99 */

#endif /*  _FLOAT_H_ */
