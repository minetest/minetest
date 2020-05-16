// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_TYPES_H_INCLUDED__
#define __IRR_TYPES_H_INCLUDED__

#include "IrrCompileConfig.h"

namespace irr
{

//! 8 bit unsigned variable.
/** This is a typedef for unsigned char, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef unsigned __int8		u8;
#else
typedef unsigned char		u8;
#endif

//! 8 bit signed variable.
/** This is a typedef for signed char, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef __int8			s8;
#else
typedef signed char		s8;
#endif

//! 8 bit character variable.
/** This is a typedef for char, it ensures portability of the engine. */
typedef char			c8;



//! 16 bit unsigned variable.
/** This is a typedef for unsigned short, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef unsigned __int16	u16;
#else
typedef unsigned short		u16;
#endif

//! 16 bit signed variable.
/** This is a typedef for signed short, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef __int16			s16;
#else
typedef signed short		s16;
#endif



//! 32 bit unsigned variable.
/** This is a typedef for unsigned int, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef unsigned __int32	u32;
#else
typedef unsigned int		u32;
#endif

//! 32 bit signed variable.
/** This is a typedef for signed int, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef __int32			s32;
#else
typedef signed int		s32;
#endif


#ifdef __IRR_HAS_S64
//! 64 bit unsigned variable.
/** This is a typedef for 64bit uint, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef unsigned __int64			u64;
#elif __GNUC__
#if __WORDSIZE == 64
typedef unsigned long int 			u64;
#else
__extension__ typedef unsigned long long	u64;
#endif
#else
typedef unsigned long long			u64;
#endif

//! 64 bit signed variable.
/** This is a typedef for 64bit int, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef __int64					s64;
#elif __GNUC__
#if __WORDSIZE == 64
typedef long int 				s64;
#else
__extension__ typedef long long			s64;
#endif
#else
typedef long long				s64;
#endif
#endif	// __IRR_HAS_S64



//! 32 bit floating point variable.
/** This is a typedef for float, it ensures portability of the engine. */
typedef float				f32;

//! 64 bit floating point variable.
/** This is a typedef for double, it ensures portability of the engine. */
typedef double				f64;


} // end namespace irr


#include <wchar.h>
#ifdef _IRR_WINDOWS_API_
//! Defines for s{w,n}printf because these methods do not match the ISO C
//! standard on Windows platforms, but it does on all others.
//! These should be int snprintf(char *str, size_t size, const char *format, ...);
//! and int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
#if defined(_MSC_VER) && _MSC_VER > 1310 && !defined (_WIN32_WCE)
#define swprintf swprintf_s
#define snprintf sprintf_s
#elif !defined(__CYGWIN__)
#define swprintf _snwprintf
#define snprintf _snprintf
#endif

// define the wchar_t type if not already built in.
#ifdef _MSC_VER
#ifndef _WCHAR_T_DEFINED
//! A 16 bit wide character type.
/**
	Defines the wchar_t-type.
	In VS6, its not possible to tell
	the standard compiler to treat wchar_t as a built-in type, and
	sometimes we just don't want to include the huge stdlib.h or wchar.h,
	so we'll use this.
*/
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif // wchar is not defined
#endif // microsoft compiler
#endif // _IRR_WINDOWS_API_

namespace irr
{

//! Type name for character type used by the file system.
/** Should the wide character version of the FileSystem be used it is a
16 bit character variable. Used for unicode Filesystem and unicode strings.
Else it is a 8 bit character variable. Used for ansi Filesystem and non-unicode
strings
*/
#if defined(_IRR_WCHAR_FILESYSTEM)
	typedef wchar_t fschar_t;
	#define _IRR_TEXT(X) L##X
#else
	typedef char fschar_t;
	#define _IRR_TEXT(X) X
#endif

} // end namespace irr

//! define a break macro for debugging.
#if defined(_DEBUG)
#if defined(_IRR_WINDOWS_API_) && defined(_MSC_VER) && !defined (_WIN32_WCE)
  #if defined(WIN64) || defined(_WIN64) // using portable common solution for x64 configuration
  #include <crtdbg.h>
  #define _IRR_DEBUG_BREAK_IF( _CONDITION_ ) if (_CONDITION_) {_CrtDbgBreak();}
  #else
  #define _IRR_DEBUG_BREAK_IF( _CONDITION_ ) if (_CONDITION_) {_asm int 3}
  #endif
#else
#include "assert.h"
#define _IRR_DEBUG_BREAK_IF( _CONDITION_ ) assert( !(_CONDITION_) );
#endif
#else
#define _IRR_DEBUG_BREAK_IF( _CONDITION_ )
#endif

//! Defines a deprecated macro which generates a warning at compile time
/** The usage is simple
For typedef:		typedef _IRR_DEPRECATED_ int test1;
For classes/structs:	class _IRR_DEPRECATED_ test2 { ... };
For methods:		class test3 { _IRR_DEPRECATED_ virtual void foo() {} };
For functions:		template<class T> _IRR_DEPRECATED_ void test4(void) {}
**/
#if defined(IGNORE_DEPRECATED_WARNING)
#define _IRR_DEPRECATED_
#elif _MSC_VER >= 1310 //vs 2003 or higher
#define _IRR_DEPRECATED_ __declspec(deprecated)
#elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)) // all versions above 3.0 should support this feature
#define _IRR_DEPRECATED_  __attribute__ ((deprecated))
#else
#define _IRR_DEPRECATED_
#endif

//! Defines a small statement to work around a microsoft compiler bug.
/** The microsoft compiler 7.0 - 7.1 has a bug:
When you call unmanaged code that returns a bool type value of false from managed code,
the return value may appear as true. See
http://support.microsoft.com/default.aspx?kbid=823071 for details.
Compiler version defines: VC6.0 : 1200, VC7.0 : 1300, VC7.1 : 1310, VC8.0 : 1400*/
#if defined(_IRR_WINDOWS_API_) && defined(_MSC_VER) && (_MSC_VER > 1299) && (_MSC_VER < 1400)
#define _IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX __asm mov eax,100
#else
#define _IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX
#endif // _IRR_MANAGED_MARSHALLING_BUGFIX


// memory debugging
#if defined(_DEBUG) && defined(IRRLICHT_EXPORTS) && defined(_MSC_VER) && \
	(_MSC_VER > 1299) && !defined(_IRR_DONT_DO_MEMORY_DEBUGGING_HERE) && !defined(_WIN32_WCE)

	#define CRTDBG_MAP_ALLOC
	#define _CRTDBG_MAP_ALLOC
	#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
	#include <stdlib.h>
	#include <crtdbg.h>
	#define new DEBUG_CLIENTBLOCK
#endif

// disable truncated debug information warning in visual studio 6 by default
#if defined(_MSC_VER) && (_MSC_VER < 1300 )
#pragma warning( disable: 4786)
#endif // _MSC


//! ignore VC8 warning deprecated
/** The microsoft compiler */
#if defined(_IRR_WINDOWS_API_) && defined(_MSC_VER) && (_MSC_VER >= 1400)
	//#pragma warning( disable: 4996)
	//#define _CRT_SECURE_NO_DEPRECATE 1
	//#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif


//! creates four CC codes used in Irrlicht for simple ids
/** some compilers can create those by directly writing the
code like 'code', but some generate warnings so we use this macro here */
#define MAKE_IRR_ID(c0, c1, c2, c3) \
		((irr::u32)(irr::u8)(c0) | ((irr::u32)(irr::u8)(c1) << 8) | \
		((irr::u32)(irr::u8)(c2) << 16) | ((irr::u32)(irr::u8)(c3) << 24 ))

#if defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#define _strcmpi(a,b) strcmpi(a,b)
#endif

#endif // __IRR_TYPES_H_INCLUDED__

