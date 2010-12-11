/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef SERIALIZATION_HEADER
#define SERIALIZATION_HEADER

#include "common_irrlicht.h"
#include "exceptions.h"
#include <iostream>
#include "utility.h"

/*
	NOTE: The goal is to increment this so that saved maps will be
	      loadable by any version. Other compatibility is not
		  maintained.
	Serialization format versions:
	== Unsupported ==
	0: original networked test with 1-byte nodes
	1: update with 2-byte nodes
	== Supported ==
	2: lighting is transmitted in param
	3: optional fetching of far blocks
	4: block compression
	5: sector objects NOTE: block compression was left accidentally out
	6: failed attempt at switching block compression on again
	7: block compression switched on again
	8: (dev) server-initiated block transfers and all kinds of stuff
	9: (dev) block objects
	10: (dev) water pressure
*/
// This represents an uninitialized or invalid format
#define SER_FMT_VER_INVALID 255
// Highest supported serialization version
#define SER_FMT_VER_HIGHEST 10
// Lowest supported serialization version
#define SER_FMT_VER_LOWEST 2

#define ser_ver_supported(v) (v >= SER_FMT_VER_LOWEST && v <= SER_FMT_VER_HIGHEST)

void compress(SharedBuffer<u8> data, std::ostream &os, u8 version);
void decompress(std::istream &is, std::ostream &os, u8 version);

/*class Serializable
{
public:
	void serialize(std::ostream &os, u8 version) = 0;
	void deSerialize(std::istream &istr);
};*/

#endif

