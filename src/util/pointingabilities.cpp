/*
Minetest
Copyright (C) 2023 cx384

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

#include "pointingabilities.h"

#include "serialize.h"
#include "exceptions.h"
#include <sstream>

std::optional<PointabilityType> PointingAbilities::matchNode(const std::string &name,
	const ItemGroupList &groups) const
{
	auto i = nodes.find(name);
	return i == nodes.end() ? matchGroups(groups, node_groups) : i->second;
}

std::optional<PointabilityType> PointingAbilities::matchObject(const std::string &name,
	const ItemGroupList &groups) const
{
	auto i = objects.find(name);
	return i == objects.end() ? matchGroups(groups, object_groups) : i->second;
}

std::optional<PointabilityType> PointingAbilities::matchGroups(const ItemGroupList &groups,
	const std::unordered_map<std::string, PointabilityType> &pointable_groups)
{
	// prefers POINTABLE over POINTABLE_NOT over POINTABLE_BLOCKING
	bool blocking = false;
	bool not_pontable = false;
	for (auto const &ability : pointable_groups) {
		if (itemgroup_get(groups, ability.first) > 0) {
			switch(ability.second) {
				case POINTABLE:
					return POINTABLE;
					break;
				case POINTABLE_NOT:
					not_pontable = true;
					break;
				default:
					blocking = true;
					break;
			}
		}
	}
	if (not_pontable)
		return POINTABLE_NOT;
	if (blocking)
		return POINTABLE_BLOCKING;
	return {};
}

void PointingAbilities::serializeTypeMap(std::ostream &os,
	const std::unordered_map<std::string, PointabilityType> &map)
{
	writeU32(os, map.size());
	for (const auto &entry : map) {
		os << serializeString16(entry.first);
		writeU8(os, (u8)entry.second);
	}
}

void PointingAbilities::deSerializeTypeMap(std::istream &is,
	std::unordered_map<std::string, PointabilityType> &map)
{
	map.clear();
	u32 size = readU32(is);
	for (u32 i = 0; i < size; i++) {
		std::string name = deSerializeString16(is);
		PointabilityType type = (PointabilityType)readU8(is);
		map[name] = type;
	}
}

void PointingAbilities::serialize(std::ostream &os) const
{
	writeU8(os, 0); // version
	serializeTypeMap(os, nodes);
	serializeTypeMap(os, node_groups);
	serializeTypeMap(os, objects);
	serializeTypeMap(os, object_groups);
}

void PointingAbilities::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version != 0)
		throw SerializationError("unsupported PointingAbilities version");
		
	deSerializeTypeMap(is, nodes);
	deSerializeTypeMap(is, node_groups);
	deSerializeTypeMap(is, objects);
	deSerializeTypeMap(is, object_groups);
}
