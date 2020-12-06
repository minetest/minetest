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

#include "nameidmapping.h"
#include "exceptions.h"
#include "util/serialize.h"

void NameIdMapping::serialize(std::ostream &os) const
{
	writeU8(os, 0); // version
	writeU16(os, m_id_to_name.size());
	for (const auto &i : m_id_to_name) {
		writeU16(os, i.first);
		os << serializeString16(i.second);
	}
}

void NameIdMapping::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version != 0)
		throw SerializationError("unsupported NameIdMapping version");
	u32 count = readU16(is);
	m_id_to_name.clear();
	m_name_to_id.clear();
	for (u32 i = 0; i < count; i++) {
		u16 id = readU16(is);
		std::string name = deSerializeString16(is);
		m_id_to_name[id] = name;
		m_name_to_id[name] = id;
	}
}
