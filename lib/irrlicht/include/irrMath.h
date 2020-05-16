// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_MATH_H_INCLUDED__
#define __IRR_MATH_H_INCLUDED__

#include "IrrCompileConfig.h"
#include "irrTypes.h"
#include <math.h>
#include <float.h>
#include <stdlib.h> // for abs() etc.
#include <limits.h> // For INT_MAX / UINT_MAX

#if defined(_IRR_SOLARIS_PLATFORM_) || defined(__BORLANDC__) || defined (__BCPLUSPLUS__) || defined (_WIN32_WCE)
	#define sqrtf(X) (irr::f32)sqrt((irr::f64)(X))
	#define sinf(X) (irr::f32)sin((irr::f64)(X))
	#define cosf(X) (irr::f32)cos((irr::f64)(X))
	#define asinf(X) (irr::f32)asin((irr::f64)(X))
	#define acosf(X) (irr::f32)acos((irr::f64)(X))
	#define atan2f(X,Y) (irr::f32)atan2((irr::f64)(X),(irr::f64)(Y))
	#define ceilf(X) (irr::f32)ceil((irr::f64)(X))
	#define floorf(X) (irr::f32)floor((irr::f64)(X))
	#define powf(X,Y) (irr::f32)pow((irr::f64)(X),(irr::f64)(Y))
	#define fmodf(X,Y) (irr::f32)fmod((irr::f64)(X),(irr::f64)(Y))
	#define fabsf(X) (irr::f32)fabs((irr::f64)(X))
	#define logf(X) (irr::f32)log((irr::f64)(X))
#endif

#ifndef FLT_MAX
#define FLT_MAX 3.402823466E+38F
#endif

#ifndef FLT_MIN
#define FLT_MIN 1.17549435e-38F
#endif

namespace irr
{
namespace core
{

	//! Rounding error constant often used when comparing f32 values.

	const s32 ROUNDING_ERROR_S32 = 0;
#ifdef __IRR_HAS_S64
	const s64 ROUNDING_ERROR_S64 = 0;
#endif
	const f32 ROUNDING_ERROR_f32 = 0.000001f;
	const f64 ROUNDING_ERROR_f64 = 0.00000001;

#ifdef PI // make sure we don't collide with a define
#undef PI
#endif
	//! Constant for PI.
	const f32 PI		= 3.14159265359f;

	//! Constant for reciprocal of PI.
	const f32 RECIPROCAL_PI	= 1.0f/PI;

	//! Constant for half of PI.
	const f32 HALF_PI	= PI/2.0f;

#ifdef PI64 // make sure we don't collide with a define
#undef PI64
#endif
	//! Constant for 64bit PI.
	const f64 PI64		= 3.1415926535897932384626433832795028841971693993751;

	//! Constant for 64bit reciprocal of PI.
	const f64 RECIPROCAL_PI64 = 1.0/PI64;

	//! 32bit Constant for converting from degrees to radians
	const f32 DEGTORAD = PI / 180.0f;

	//! 32bit constant for converting from radians to degrees (formally known as GRAD_PI)
	const f32 RADTODEG   = 180.0f / PI;

	//! 64bit constant for converting from degrees to radians (formally known as GRAD_PI2)
	const f64 DEGTORAD64 = PI64 / 180.0;

	//! 64bit constant for converting from radians to degrees
	const f64 RADTODEG64 = 180.0 / PI64;

	//! Utility function to convert a radian value to degrees
	/** Provided as it can be clearer to write radToDeg(X) than RADTODEG * X
	\param radians	The radians value to convert to degrees.
	*/
	inline f32 radToDeg(f32 radians)
	{
		return RADTODEG * radians;
	}

	//! Utility function to convert a radian value to degrees
	/** Provided as it can be clearer to write radToDeg(X) than RADTODEG * X
	\param radians	The radians value to convert to degrees.
	*/
	inline f64 radToDeg(f64 radians)
	{
		return RADTODEG64 * radians;
	}

	//! Utility function to convert a degrees value to radians
	/** Provided as it can be clearer to write degToRad(X) than DEGTORAD * X
	\param degrees	The degrees value to convert to radians.
	*/
	inline f32 degToRad(f32 degrees)
	{
		return DEGTORAD * degrees;
	}

	//! Utility function to convert a degrees value to radians
	/** Provided as it can be clearer to write degToRad(X) than DEGTORAD * X
	\param degrees	The degrees value to convert to radians.
	*/
	inline f64 degToRad(f64 degrees)
	{
		return DEGTORAD64 * degrees;
	}

	//! returns minimum of two values. Own implementation to get rid of the STL (VS6 problems)
	template<class T>
	inline const T& min_(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	//! returns minimum of three values. Own implementation to get rid of the STL (VS6 problems)
	template<class T>
	inline const T& min_(const T& a, const T& b, const T& c)
	{
		return a < b ? min_(a, c) : min_(b, c);
	}

	//! returns maximum of two values. Own implementation to get rid of the STL (VS6 problems)
	template<class T>
	inline const T& max_(const T& a, const T& b)
	{
		return a < b ? b : a;
	}

	//! returns maximum of three values. Own implementation to get rid of the STL (VS6 problems)
	template<class T>
	inline const T& max_(const T& a, const T& b, const T& c)
	{
		return a < b ? max_(b, c) : max_(a, c);
	}

	//! returns abs of two values. Own implementation to get rid of STL (VS6 problems)
	template<class T>
	inline T abs_(const T& a)
	{
		return a < (T)0 ? -a : a;
	}

	//! returns linear interpolation of a and b with ratio t
	//! \return: a if t==0, b if t==1, and the linear interpolation else
	template<class T>
	inline T lerp(const T& a, const T& b, const f32 t)
	{
		return (T)(a*(1.f-t)) + (b*t);
	}

	//! clamps a value between low and high
	template <class T>
	inline const T clamp (const T& value, const T& low, const T& high)
	{
		return min_ (max_(value,low), high);
	}

	//! swaps the content of the passed parameters
	// Note: We use the same trick as boost and use two template arguments to
	// avoid ambiguity when swapping objects of an Irrlicht type that has not
	// it's own swap overload. Otherwise we get conflicts with some compilers
	// in combination with stl.
	template <class T1, class T2>
	inline void swap(T1& a, T2& b)
	{
		T1 c(a);
		a = b;
		b = c;
	}

	//! returns if a equals b, taking possible rounding errors into account
	inline bool equals(const f64 a, const f64 b, const f64 tolerance = ROUNDING_ERROR_f64)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	//! returns if a equals b, taking possible rounding errors into account
	inline bool equals(const f32 a, const f32 b, const f32 tolerance = ROUNDING_ERROR_f32)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	union FloatIntUnion32
	{
		FloatIntUnion32(float f1 = 0.0f) : f(f1) {}
		// Portable sign-extraction
		bool sign() const { return (i >> 31) != 0; }

		irr::s32 i;
		irr::f32 f;
	};

	//! We compare the difference in ULP's (spacing between floating-point numbers, aka ULP=1 means there exists no float between).
	//\result true when numbers have a ULP <= maxUlpDiff AND have the same sign.
	inline bool equalsByUlp(f32 a, f32 b, int maxUlpDiff)
	{
		// Based on the ideas and code from Bruce Dawson on
		// http://www.altdevblogaday.com/2012/02/22/comparing-floating-point-numbers-2012-edition/
		// When floats are interpreted as integers the two nearest possible float numbers differ just
		// by one integer number. Also works the other way round, an integer of 1 interpreted as float
		// is for example the smallest possible float number.

		FloatIntUnion32 fa(a);
		FloatIntUnion32 fb(b);

		// Different signs, we could maybe get difference to 0, but so close to 0 using epsilons is better.
		if ( fa.sign() != fb.sign() )
		{
			// Check for equality to make sure +0==-0
			if (fa.i == fb.i)
				return true;
			return false;
		}

		// Find the difference in ULPs.
		int ulpsDiff = abs_(fa.i- fb.i);
		if (ulpsDiff <= maxUlpDiff)
			return true;

		return false;
	}

#if 0
	//! returns if a equals b, not using any rounding tolerance
	inline bool equals(const s32 a, const s32 b)
	{
		return (a == b);
	}

	//! returns if a equals b, not using any rounding tolerance
	inline bool equals(const u32 a, const u32 b)
	{
		return (a == b);
	}
#endif
	//! returns if a equals b, taking an explicit rounding tolerance into account
	inline bool equals(const s32 a, const s32 b, const s32 tolerance = ROUNDING_ERROR_S32)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	//! returns if a equals b, taking an explicit rounding tolerance into account
	inline bool equals(const u32 a, const u32 b, const s32 tolerance = ROUNDING_ERROR_S32)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

#ifdef __IRR_HAS_S64
	//! returns if a equals b, taking an explicit rounding tolerance into account
	inline bool equals(const s64 a, const s64 b, const s64 tolerance = ROUNDING_ERROR_S64)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}
#endif

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const f64 a, const f64 tolerance = ROUNDING_ERROR_f64)
	{
		return fabs(a) <= tolerance;
	}

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const f32 a, const f32 tolerance = ROUNDING_ERROR_f32)
	{
		return fabsf(a) <= tolerance;
	}

	//! returns if a equals not zero, taking rounding errors into account
	inline bool isnotzero(const f32 a, const f32 tolerance = ROUNDING_ERROR_f32)
	{
		return fabsf(a) > tolerance;
	}

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const s32 a, const s32 tolerance = 0)
	{
		return ( a & 0x7ffffff ) <= tolerance;
	}

	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const u32 a, const u32 tolerance = 0)
	{
		return a <= tolerance;
	}

#ifdef __IRR_HAS_S64
	//! returns if a equals zero, taking rounding errors into account
	inline bool iszero(const s64 a, const s64 tolerance = 0)
	{
		return abs_(a) <= tolerance;
	}
#endif

	inline s32 s32_min(s32 a, s32 b)
	{
		const s32 mask = (a - b) >> 31;
		return (a & mask) | (b & ~mask);
	}

	inline s32 s32_max(s32 a, s32 b)
	{
		const s32 mask = (a - b) >> 31;
		return (b & mask) | (a & ~mask);
	}

	inline s32 s32_clamp (s32 value, s32 low, s32 high)
	{
		return s32_min(s32_max(value,low), high);
	}

	/*
		float IEEE-754 bit represenation

		0      0x00000000
		1.0    0x3f800000
		0.5    0x3f000000
		3      0x40400000
		+inf   0x7f800000
		-inf   0xff800000
		+NaN   0x7fc00000 or 0x7ff00000
		in general: number = (sign ? -1:1) * 2^(exponent) * 1.(mantissa bits)
	*/

	typedef union { u32 u; s32 s; f32 f; } inttofloat;

	#define F32_AS_S32(f)		(*((s32 *) &(f)))
	#define F32_AS_U32(f)		(*((u32 *) &(f)))
	#define F32_AS_U32_POINTER(f)	( ((u32 *) &(f)))

	#define F32_VALUE_0		0x00000000
	#define F32_VALUE_1		0x3f800000
	#define F32_SIGN_BIT		0x80000000U
	#define F32_EXPON_MANTISSA	0x7FFFFFFFU

	//! code is taken from IceFPU
	//! Integer representation of a floating-point value.
#ifdef IRRLICHT_FAST_MATH
	#define IR(x)                           ((u32&)(x))
#else
	inline u32 IR(f32 x) {inttofloat tmp; tmp.f=x; return tmp.u;}
#endif

	//! Absolute integer representation of a floating-point value
	#define AIR(x)				(IR(x)&0x7fffffff)

	//! Floating-point representation of an integer value.
#ifdef IRRLICHT_FAST_MATH
	#define FR(x)                           ((f32&)(x))
#else
	inline f32 FR(u32 x) {inttofloat tmp; tmp.u=x; return tmp.f;}
	inline f32 FR(s32 x) {inttofloat tmp; tmp.s=x; return tmp.f;}
#endif

	//! integer representation of 1.0
	#define IEEE_1_0			0x3f800000
	//! integer representation of 255.0
	#define IEEE_255_0			0x437f0000

#ifdef IRRLICHT_FAST_MATH
	#define	F32_LOWER_0(f)		(F32_AS_U32(f) >  F32_SIGN_BIT)
	#define	F32_LOWER_EQUAL_0(f)	(F32_AS_S32(f) <= F32_VALUE_0)
	#define	F32_GREATER_0(f)	(F32_AS_S32(f) >  F32_VALUE_0)
	#define	F32_GREATER_EQUAL_0(f)	(F32_AS_U32(f) <= F32_SIGN_BIT)
	#define	F32_EQUAL_1(f)		(F32_AS_U32(f) == F32_VALUE_1)
	#define	F32_EQUAL_0(f)		( (F32_AS_U32(f) & F32_EXPON_MANTISSA ) == F32_VALUE_0)

	// only same sign
	#define	F32_A_GREATER_B(a,b)	(F32_AS_S32((a)) > F32_AS_S32((b)))

#else

	#define	F32_LOWER_0(n)		((n) <  0.0f)
	#define	F32_LOWER_EQUAL_0(n)	((n) <= 0.0f)
	#define	F32_GREATER_0(n)	((n) >  0.0f)
	#define	F32_GREATER_EQUAL_0(n)	((n) >= 0.0f)
	#define	F32_EQUAL_1(n)		((n) == 1.0f)
	#define	F32_EQUAL_0(n)		((n) == 0.0f)
	#define	F32_A_GREATER_B(a,b)	((a) > (b))
#endif


#ifndef REALINLINE
	#ifdef _MSC_VER
		#define REALINLINE __forceinline
	#else
		#define REALINLINE inline
	#endif
#endif

#if defined(__BORLANDC__) || defined (__BCPLUSPLUS__)

	// 8-bit bools in borland builder

	//! conditional set based on mask and arithmetic shift
	REALINLINE u32 if_c_a_else_b ( const c8 condition, const u32 a, const u32 b )
	{
		return ( ( -condition >> 7 ) & ( a ^ b ) ) ^ b;
	}

	//! conditional set based on mask and arithmetic shift
	REALINLINE u32 if_c_a_else_0 ( const c8 condition, const u32 a )
	{
		return ( -condition >> 31 ) & a;
	}
#else

	//! conditional set based on mask and arithmetic shift
	REALINLINE u32 if_c_a_else_b ( const s32 condition, const u32 a, const u32 b )
	{
		return ( ( -condition >> 31 ) & ( a ^ b ) ) ^ b;
	}

	//! conditional set based on mask and arithmetic shift
	REALINLINE u16 if_c_a_else_b ( const s16 condition, const u16 a, const u16 b )
	{
		return ( ( -condition >> 15 ) & ( a ^ b ) ) ^ b;
	}

	//! conditional set based on mask and arithmetic shift
	REALINLINE u32 if_c_a_else_0 ( const s32 condition, const u32 a )
	{
		return ( -condition >> 31 ) & a;
	}
#endif

	/*
		if (condition) state |= m; else state &= ~m;
	*/
	REALINLINE void setbit_cond ( u32 &state, s32 condition, u32 mask )
	{
		// 0, or any postive to mask
		//s32 conmask = -condition >> 31;
		state ^= ( ( -condition >> 31 ) ^ state ) & mask;
	}

	inline f32 round_( f32 x )
	{
		return floorf( x + 0.5f );
	}

	REALINLINE void clearFPUException ()
	{
#ifdef IRRLICHT_FAST_MATH
		return;
#ifdef feclearexcept
		feclearexcept(FE_ALL_EXCEPT);
#elif defined(_MSC_VER)
		__asm fnclex;
#elif defined(__GNUC__) && defined(__x86__)
		__asm__ __volatile__ ("fclex \n\t");
#else
#  warn clearFPUException not supported.
#endif
#endif
	}

	// calculate: sqrt ( x )
	REALINLINE f32 squareroot(const f32 f)
	{
		return sqrtf(f);
	}

	// calculate: sqrt ( x )
	REALINLINE f64 squareroot(const f64 f)
	{
		return sqrt(f);
	}

	// calculate: sqrt ( x )
	REALINLINE s32 squareroot(const s32 f)
	{
		return static_cast<s32>(squareroot(static_cast<f32>(f)));
	}

#ifdef __IRR_HAS_S64
	// calculate: sqrt ( x )
	REALINLINE s64 squareroot(const s64 f)
	{
		return static_cast<s64>(squareroot(static_cast<f64>(f)));
	}
#endif

	// calculate: 1 / sqrt ( x )
	REALINLINE f64 reciprocal_squareroot(const f64 x)
	{
		return 1.0 / sqrt(x);
	}

	// calculate: 1 / sqrtf ( x )
	REALINLINE f32 reciprocal_squareroot(const f32 f)
	{
#if defined ( IRRLICHT_FAST_MATH )
	#if defined(_MSC_VER)
		// SSE reciprocal square root estimate, accurate to 12 significant
		// bits of the mantissa
		f32 recsqrt;
		__asm rsqrtss xmm0, f           // xmm0 = rsqrtss(f)
		__asm movss recsqrt, xmm0       // return xmm0
		return recsqrt;

/*
		// comes from Nvidia
		u32 tmp = (u32(IEEE_1_0 << 1) + IEEE_1_0 - *(u32*)&x) >> 1;
		f32 y = *(f32*)&tmp;
		return y * (1.47f - 0.47f * x * y * y);
*/
	#else
		return 1.f / sqrtf(f);
	#endif
#else // no fast math
		return 1.f / sqrtf(f);
#endif
	}

	// calculate: 1 / sqrtf( x )
	REALINLINE s32 reciprocal_squareroot(const s32 x)
	{
		return static_cast<s32>(reciprocal_squareroot(static_cast<f32>(x)));
	}

	// calculate: 1 / x
	REALINLINE f32 reciprocal( const f32 f )
	{
#if defined (IRRLICHT_FAST_MATH)

		// SSE Newton-Raphson reciprocal estimate, accurate to 23 significant
		// bi ts of the mantissa
		// One Newtown-Raphson Iteration:
		// f(i+1) = 2 * rcpss(f) - f * rcpss(f) * rcpss(f)
		f32 rec;
		__asm rcpss xmm0, f               // xmm0 = rcpss(f)
		__asm movss xmm1, f               // xmm1 = f
		__asm mulss xmm1, xmm0            // xmm1 = f * rcpss(f)
		__asm mulss xmm1, xmm0            // xmm2 = f * rcpss(f) * rcpss(f)
		__asm addss xmm0, xmm0            // xmm0 = 2 * rcpss(f)
		__asm subss xmm0, xmm1            // xmm0 = 2 * rcpss(f)
										  //        - f * rcpss(f) * rcpss(f)
		__asm movss rec, xmm0             // return xmm0
		return rec;


		//! i do not divide through 0.. (fpu expection)
		// instead set f to a high value to get a return value near zero..
		// -1000000000000.f.. is use minus to stay negative..
		// must test's here (plane.normal dot anything ) checks on <= 0.f
		//u32 x = (-(AIR(f) != 0 ) >> 31 ) & ( IR(f) ^ 0xd368d4a5 ) ^ 0xd368d4a5;
		//return 1.f / FR ( x );

#else // no fast math
		return 1.f / f;
#endif
	}

	// calculate: 1 / x
	REALINLINE f64 reciprocal ( const f64 f )
	{
		return 1.0 / f;
	}


	// calculate: 1 / x, low precision allowed
	REALINLINE f32 reciprocal_approxim ( const f32 f )
	{
#if defined( IRRLICHT_FAST_MATH)

		// SSE Newton-Raphson reciprocal estimate, accurate to 23 significant
		// bi ts of the mantissa
		// One Newtown-Raphson Iteration:
		// f(i+1) = 2 * rcpss(f) - f * rcpss(f) * rcpss(f)
		f32 rec;
		__asm rcpss xmm0, f               // xmm0 = rcpss(f)
		__asm movss xmm1, f               // xmm1 = f
		__asm mulss xmm1, xmm0            // xmm1 = f * rcpss(f)
		__asm mulss xmm1, xmm0            // xmm2 = f * rcpss(f) * rcpss(f)
		__asm addss xmm0, xmm0            // xmm0 = 2 * rcpss(f)
		__asm subss xmm0, xmm1            // xmm0 = 2 * rcpss(f)
										  //        - f * rcpss(f) * rcpss(f)
		__asm movss rec, xmm0             // return xmm0
		return rec;


/*
		// SSE reciprocal estimate, accurate to 12 significant bits of
		f32 rec;
		__asm rcpss xmm0, f             // xmm0 = rcpss(f)
		__asm movss rec , xmm0          // return xmm0
		return rec;
*/
/*
		register u32 x = 0x7F000000 - IR ( p );
		const f32 r = FR ( x );
		return r * (2.0f - p * r);
*/
#else // no fast math
		return 1.f / f;
#endif
	}


	REALINLINE s32 floor32(f32 x)
	{
#ifdef IRRLICHT_FAST_MATH
		const f32 h = 0.5f;

		s32 t;

#if defined(_MSC_VER)
		__asm
		{
			fld	x
			fsub	h
			fistp	t
		}
#elif defined(__GNUC__)
		__asm__ __volatile__ (
			"fsub %2 \n\t"
			"fistpl %0"
			: "=m" (t)
			: "t" (x), "f" (h)
			: "st"
			);
#else
#  warn IRRLICHT_FAST_MATH not supported.
		return (s32) floorf ( x );
#endif
		return t;
#else // no fast math
		return (s32) floorf ( x );
#endif
	}


	REALINLINE s32 ceil32 ( f32 x )
	{
#ifdef IRRLICHT_FAST_MATH
		const f32 h = 0.5f;

		s32 t;

#if defined(_MSC_VER)
		__asm
		{
			fld	x
			fadd	h
			fistp	t
		}
#elif defined(__GNUC__)
		__asm__ __volatile__ (
			"fadd %2 \n\t"
			"fistpl %0 \n\t"
			: "=m"(t)
			: "t"(x), "f"(h)
			: "st"
			);
#else
#  warn IRRLICHT_FAST_MATH not supported.
		return (s32) ceilf ( x );
#endif
		return t;
#else // not fast math
		return (s32) ceilf ( x );
#endif
	}



	REALINLINE s32 round32(f32 x)
	{
#if defined(IRRLICHT_FAST_MATH)
		s32 t;

#if defined(_MSC_VER)
		__asm
		{
			fld   x
			fistp t
		}
#elif defined(__GNUC__)
		__asm__ __volatile__ (
			"fistpl %0 \n\t"
			: "=m"(t)
			: "t"(x)
			: "st"
			);
#else
#  warn IRRLICHT_FAST_MATH not supported.
		return (s32) round_(x);
#endif
		return t;
#else // no fast math
		return (s32) round_(x);
#endif
	}

	inline f32 f32_max3(const f32 a, const f32 b, const f32 c)
	{
		return a > b ? (a > c ? a : c) : (b > c ? b : c);
	}

	inline f32 f32_min3(const f32 a, const f32 b, const f32 c)
	{
		return a < b ? (a < c ? a : c) : (b < c ? b : c);
	}

	inline f32 fract ( f32 x )
	{
		return x - floorf ( x );
	}

} // end namespace core
} // end namespace irr

#ifndef IRRLICHT_FAST_MATH
	using irr::core::IR;
	using irr::core::FR;
#endif

#endif

