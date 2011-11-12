/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "luaentity_common.h"

#include "utility.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

LuaEntityProperties::LuaEntityProperties():
	physical(true),
	weight(5),
	collisionbox(-0.5,-0.5,-0.5, 0.5,0.5,0.5),
	visual("single_sprite")
{
	textures.push_back("unknown_block.png");
}

std::string LuaEntityProperties::dump()
{
	std::ostringstream os(std::ios::binary);
	os<<"physical="<<physical;
	os<<", weight="<<weight;
	os<<", collisionbox="<<PP(collisionbox.MinEdge)<<","<<PP(collisionbox.MaxEdge);
	os<<", visual="<<visual;
	os<<", textures=[";
	for(core::list<std::string>::Iterator i = textures.begin();
			i != textures.end(); i++){
		os<<"\""<<(*i)<<"\" ";
	}
	os<<"]";
	return os.str();
}

void LuaEntityProperties::serialize(std::ostream &os)
{
	writeU8(os, 0); // version
	writeU8(os, physical);
	writeF1000(os, weight);
	writeV3F1000(os, collisionbox.MinEdge);
	writeV3F1000(os, collisionbox.MaxEdge);
	os<<serializeString(visual);
	writeU16(os, textures.size());
	for(core::list<std::string>::Iterator i = textures.begin();
			i != textures.end(); i++){
		os<<serializeString(*i);
	}
}

void LuaEntityProperties::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0) throw SerializationError(
			"unsupported LuaEntityProperties version");
	physical = readU8(is);
	weight = readF1000(is);
	collisionbox.MinEdge = readV3F1000(is);
	collisionbox.MaxEdge = readV3F1000(is);
	visual = deSerializeString(is);
	textures.clear();
	int texture_count = readU16(is);
	for(int i=0; i<texture_count; i++){
		textures.push_back(deSerializeString(is));
	}
}


