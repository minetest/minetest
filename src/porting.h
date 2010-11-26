/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef PORTING_HEADER
#define PORTING_HEADER

#ifdef _WIN32
	#define SWPRINTF_CHARSTRING L"%S"
#else
	#define SWPRINTF_CHARSTRING L"%s"
#endif

#endif

