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

#ifndef UTIL_POINTEDTHING_HEADER
#define UTIL_POINTEDTHING_HEADER

#include "../irrlichttypes.h"
#include "../irr_v3d.h"
#include <iostream>
#include <string>

enum PointedThingType
{
	POINTEDTHING_NOTHING,
	POINTEDTHING_NODE,
	POINTEDTHING_OBJECT
};

struct PointedThing
{
	PointedThingType type;
	v3s16 node_undersurface;
	v3s16 node_abovesurface;
	s16 object_id;

	PointedThing();
	std::string dump() const;
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
	bool operator==(const PointedThing &pt2) const;
	bool operator!=(const PointedThing &pt2) const;
};

#endif

