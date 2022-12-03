/*
Minetest
Copyright (C) 2023 Andrey2470T, AndreyT <andreyt2203@gmail.com>

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

#include "line_params.h"
#include "log.h"
#include "constants.h"

void LineParams::reset()
{
	type = LineType::FLAT;

	pos.first = v3f(0.0f);
	pos.second = v3f(0.0f);

	color.first = video::SColor(0, 0, 0, 255);
	color.second = video::SColor(0, 0, 0, 255);

	attached_ids.first = 0;
	attached_ids.second = 0;

	width = 0.05f * BS;

	texture = "";

	playername = "";

	backface_culling = false;

	light_level = 0;

	axis_angle = 0.0f;

	last_changed_props.resize(LP_COUNT);
}

void LineParams::serialize(std::ostream &os, u16 protocol_version) const
{
	writeU8(os, (u16)type);

	writeV3F32(os, pos.first);
	writeV3F32(os, pos.second);

	writeARGB8(os, color.first);
	writeARGB8(os, color.second);

	writeU16(os, attached_ids.first);
	writeU16(os, attached_ids.second);

	writeF32(os, width);

	os << serializeString32(texture);
	os << serializeString32(playername);

	writeU8(os, (u8)backface_culling);

	animation.serialize(os, protocol_version);

	writeU16(os, light_level);

	writeF32(os, axis_angle);

	for (auto b : last_changed_props)
		writeU8(os, (u8)b);
}

void LineParams::deserialize(std::istream &is, u16 protocol_version)
{
	type = (LineType)readU8(is);

	pos.first = readV3F32(is);
	pos.second = readV3F32(is);

	color.first = readARGB8(is);
	color.second = readARGB8(is);

	attached_ids.first = readU16(is);
	attached_ids.second = readU16(is);

	width = readF32(is);

	texture = deSerializeString32(is);
	playername = deSerializeString32(is);

	backface_culling = (bool)readU8(is);

	animation.deSerialize(is, protocol_version);

	light_level = readU16(is);

	axis_angle = readF32(is);

	for (u32 i = 0; i < last_changed_props.size(); i++)
		last_changed_props[i] = (bool)readU8(is);
}
