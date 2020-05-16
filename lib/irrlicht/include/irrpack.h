// Copyright (C) 2007-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// include this file right before the data structures to be 1-aligned
// and add to each structure the PACK_STRUCT define just like this:
// struct mystruct
// {
//	...
// } PACK_STRUCT;
// Always include the irrunpack.h file right after the last type declared
// like this, and do not put any other types with different alignment
// in between!

// byte-align structures
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma warning(disable: 4103)
#	pragma pack( push, packing )
#	pragma pack( 1 )
#	define PACK_STRUCT
#elif defined( __DMC__ )
#	pragma pack( push, 1 )
#	define PACK_STRUCT
#elif defined( __GNUC__ )
	// Using pragma pack might work with earlier gcc versions already, but
	// it started to be necessary with gcc 4.7 on mingw unless compiled with -mno-ms-bitfields.
	// And I found some hints on the web that older gcc versions on the other hand had sometimes
	// trouble with pragma pack while they worked with __attribute__((packed)).
#	if (__GNUC__ > 4 ) || ((__GNUC__ == 4 ) && (__GNUC_MINOR__ >= 7))
#		pragma pack( push, packing )
#		pragma pack( 1 )
#		define PACK_STRUCT
#	else
#		define PACK_STRUCT	__attribute__((packed))
	#endif
#else
#	error compiler not supported
#endif

