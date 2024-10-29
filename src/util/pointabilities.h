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

#pragma once
#include <string>
#include <unordered_map>
#include "itemgroup.h"
#include <optional>
#include "irrlichttypes.h"

enum class PointabilityType : u8
{
	// Can be pointed through.
	// Note: This MUST be the 0-th item in the enum for backwards compat.
	// Older Minetest versions send "pointable=false" as "0".
	POINTABLE_NOT,
	// Is pointable.
	// Note: This MUST be the 1-th item in the enum for backwards compat:
	// Older Minetest versions send "pointable=true" as "1".
	POINTABLE,
	// Note: Since (u8) 2 is truthy,
	// older clients will understand this as "pointable=true",
	// which is a reasonable fallback.
	POINTABLE_BLOCKING,
};

// An object to store overridden pointable properties
struct Pointabilities
{
	// Nodes
	std::unordered_map<std::string, PointabilityType> nodes;
	std::unordered_map<std::string, PointabilityType> node_groups;

	// Objects
	std::unordered_map<std::string, PointabilityType> objects;
	std::unordered_map<std::string, PointabilityType> object_groups; // armor_groups

	// Match functions return fitting pointability,
	// otherwise the default pointability should be used.

	std::optional<PointabilityType> matchNode(const std::string &name,
		const ItemGroupList &groups) const;
	std::optional<PointabilityType> matchObject(const std::string &name,
		const ItemGroupList &groups) const;
	// For players only armor groups will work
	std::optional<PointabilityType> matchPlayer(const ItemGroupList &groups) const;

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);

	// For a save enum conversion.
	static PointabilityType deSerializePointabilityType(std::istream &is);
	static void serializePointabilityType(std::ostream &os, PointabilityType pointable_type);
	static std::string toStringPointabilityType(PointabilityType pointable_type);

private:
	static std::optional<PointabilityType> matchGroups(const ItemGroupList &groups,
		const std::unordered_map<std::string, PointabilityType> &pointable_groups);
	static void serializeTypeMap(std::ostream &os,
		const std::unordered_map<std::string, PointabilityType> &map);
	static void deSerializeTypeMap(std::istream &is,
		std::unordered_map<std::string, PointabilityType> &map);
};
