/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "pointedthing.h"

#include "serialize.h"
#include <sstream>

PointedThing::PointedThing():
	type(POINTEDTHING_NOTHING),
	node_undersurface(0,0,0),
	node_abovesurface(0,0,0),
	object_id(-1)
{}

std::string PointedThing::dump() const
{
	std::ostringstream os(std::ios::binary);
	if(type == POINTEDTHING_NOTHING)
	{
		os<<"[nothing]";
	}
	else if(type == POINTEDTHING_NODE)
	{
		const v3s16 &u = node_undersurface;
		const v3s16 &a = node_abovesurface;
		os<<"[node under="<<u.X<<","<<u.Y<<","<<u.Z
			<< " above="<<a.X<<","<<a.Y<<","<<a.Z<<"]";
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		os<<"[object "<<object_id<<"]";
	}
	else
	{
		os<<"[unknown PointedThing]";
	}
	return os.str();
}

void PointedThing::serialize(std::ostream &os) const
{
	writeU8(os, 0); // version
	writeU8(os, (u8)type);
	if(type == POINTEDTHING_NOTHING)
	{
		// nothing
	}
	else if(type == POINTEDTHING_NODE)
	{
		writeV3S16(os, node_undersurface);
		writeV3S16(os, node_abovesurface);
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		writeS16(os, object_id);
	}
}

void PointedThing::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0) throw SerializationError(
			"unsupported PointedThing version");
	type = (PointedThingType) readU8(is);
	if(type == POINTEDTHING_NOTHING)
	{
		// nothing
	}
	else if(type == POINTEDTHING_NODE)
	{
		node_undersurface = readV3S16(is);
		node_abovesurface = readV3S16(is);
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		object_id = readS16(is);
	}
	else
	{
		throw SerializationError(
			"unsupported PointedThingType");
	}
}

bool PointedThing::operator==(const PointedThing &pt2) const
{
	if(type != pt2.type)
		return false;
	if(type == POINTEDTHING_NODE)
	{
		if(node_undersurface != pt2.node_undersurface)
			return false;
		if(node_abovesurface != pt2.node_abovesurface)
			return false;
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		if(object_id != pt2.object_id)
			return false;
	}
	return true;
}

bool PointedThing::operator!=(const PointedThing &pt2) const
{
	return !(*this == pt2);
}

