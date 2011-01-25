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

#ifndef TILE_HEADER
#define TILE_HEADER

#include "common_irrlicht.h"
//#include "utility.h"
#include <string>

struct TileSpec
{
	TileSpec():
		alpha(255)
	{
	}

	TileSpec(const std::string &a_name):
		name(a_name),
		alpha(255)
	{
	}

	TileSpec(const char *a_name):
		name(a_name),
		alpha(255)
	{
	}

	bool operator==(TileSpec &other)
	{
		return (name == other.name && alpha == other.alpha);
	}
	
	// path + mods
	std::string name;
	u8 alpha;
};

#endif
