/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SERIALIZATION_HEADER
#define SERIALIZATION_HEADER

#include "irrlichttypes.h"
#include "exceptions.h"
#include <iostream>
#include "util/pointer.h"

/*
	Map format serialization version
	--------------------------------

	For map data (blocks, nodes, sectors).
	
	NOTE: The goal is to increment this so that saved maps will be
	      loadable by any version. Other compatibility is not
		  maintained.
		  
	0: original networked test with 1-byte nodes
	1: update with 2-byte nodes
	2: lighting is transmitted in param
	3: optional fetching of far blocks
	4: block compression
	5: sector objects NOTE: block compression was left accidentally out
	6: failed attempt at switching block compression on again
	7: block compression switched on again
	8: server-initiated block transfers and all kinds of stuff
	9: block objects
	10: water pressure
	11: zlib'd blocks, block flags
	12: UnlimitedHeightmap now uses interpolated areas
	13: Mapgen v2
	14: NodeMetadata
	15: StaticObjects
	16: larger maximum size of node metadata, and compression
	17: MapBlocks contain timestamp
	18: new generator (not really necessary, but it's there)
	19: new content type handling
	20: many existing content types translated to extended ones
	21: dynamic content type allocation
	22: minerals removed, facedir & wallmounted changed
	23: new node metadata format
	24: 16-bit node ids and node timers (never released as stable)
	25: Improved node timer format
	26: Never written; read the same as 25
*/
// This represents an uninitialized or invalid format
#define SER_FMT_VER_INVALID 255
// Highest supported serialization version
#define SER_FMT_VER_HIGHEST_READ 26
// Saved on disk version
#define SER_FMT_VER_HIGHEST_WRITE 25
// Lowest supported serialization version
#define SER_FMT_VER_LOWEST 0

#define ser_ver_supported(v) (v >= SER_FMT_VER_LOWEST && v <= SER_FMT_VER_HIGHEST_READ)

/*
	Misc. serialization functions
*/

void compressZlib(SharedBuffer<u8> data, std::ostream &os, int level = -1);
void compressZlib(const std::string &data, std::ostream &os, int level = -1);
void decompressZlib(std::istream &is, std::ostream &os);

// These choose between zlib and a self-made one according to version
void compress(SharedBuffer<u8> data, std::ostream &os, u8 version);
//void compress(const std::string &data, std::ostream &os, u8 version);
void decompress(std::istream &is, std::ostream &os, u8 version);

#endif

