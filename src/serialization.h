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
*/
// This represents an uninitialized or invalid format
#define SER_FMT_VER_INVALID 255
// Highest supported serialization version
#define SER_FMT_VER_HIGHEST 9
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

