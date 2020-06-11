/*
Minetest
Copyright (C) 2020 sfan5 <sfan5@live.de>

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

#include "particles.h"
#include "util/serialize.h"

void ParticleParameters::serialize(std::ostream &os, u16 protocol_ver) const
{
	writeV3F32(os, pos);
	writeV3F32(os, vel);
	writeV3F32(os, acc);
	writeF32(os, expirationtime);
	writeF32(os, size);
	writeU8(os, collisiondetection);
	os << serializeLongString(texture);
	writeU8(os, vertical);
	writeU8(os, collision_removal);
	animation.serialize(os, 6); /* NOT the protocol ver */
	writeU8(os, glow);
	writeU8(os, object_collision);
	writeU16(os, node.param0);
	writeU8(os, node.param2);
	writeU8(os, node_tile);
}

void ParticleParameters::deSerialize(std::istream &is, u16 protocol_ver)
{
	pos = readV3F32(is);
	vel = readV3F32(is);
	acc = readV3F32(is);
	expirationtime = readF32(is);
	size = readF32(is);
	collisiondetection = readU8(is);
	texture = deSerializeLongString(is);
	vertical = readU8(is);
	collision_removal = readU8(is);
	animation.deSerialize(is, 6); /* NOT the protocol ver */
	glow = readU8(is);
	object_collision = readU8(is);
	// This is kinda awful
	u16 tmp_param0 = readU16(is);
	if (is.eof())
		return;
	node.param0 = tmp_param0;
	node.param2 = readU8(is);
	node_tile = readU8(is);
}
