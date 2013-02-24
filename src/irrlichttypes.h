/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef IRRLICHTTYPES_HEADER
#define IRRLICHTTYPES_HEADER

#include <irrTypes.h>

using namespace irr;

// Irrlicht 1.8+ defines 64bit unsigned symbol in irrTypes.h
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
#ifdef _MSC_VER
	// Windows
	typedef long long s64;
	typedef unsigned long long u64;
#else
	// Posix
	#include <stdint.h>
	typedef int64_t s64;
	typedef uint64_t u64;
#endif
#endif

#endif

