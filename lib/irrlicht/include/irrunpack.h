// Copyright (C) 2007-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// include this file to switch back to default alignment
// file belongs to irrpack.h, see there for more info

// Default alignment
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop, packing )
#elif defined (__DMC__)
#	pragma pack( pop )
#elif defined( __GNUC__ )
#   if (__GNUC__ > 4 ) || ((__GNUC__ == 4 ) && (__GNUC_MINOR__ >= 7))
#	    pragma pack( pop, packing )
#   endif
#endif

#undef PACK_STRUCT

