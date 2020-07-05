/*
Minetest
Copyright (C) 2020 Hugues Ross <hugues.ross@gmail.com>

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

#include "irrlichttypes.h"
#include <string>
#include <vector>

typedef u16 override_t;

//! Bitmask enum specifying what a texture override should apply to
enum class OverrideTarget : override_t
{
	INVALID = 0,
	TOP = 1 << 0,
	BOTTOM = 1 << 1,
	LEFT = 1 << 2,
	RIGHT = 1 << 3,
	FRONT = 1 << 4,
	BACK = 1 << 5,
	INVENTORY = 1 << 6,
	WIELD = 1 << 7,
	SPECIAL_1 = 1 << 8,
	SPECIAL_2 = 1 << 9,
	SPECIAL_3 = 1 << 10,
	SPECIAL_4 = 1 << 11,
	SPECIAL_5 = 1 << 12,
	SPECIAL_6 = 1 << 13,

	// clang-format off
	SIDES = LEFT | RIGHT | FRONT | BACK,
	ALL_FACES = TOP | BOTTOM | SIDES,
	ALL_SPECIAL = SPECIAL_1 | SPECIAL_2 | SPECIAL_3 | SPECIAL_4 | SPECIAL_5 | SPECIAL_6,
	NODE_TARGETS = ALL_FACES | ALL_SPECIAL,
	ITEM_TARGETS = INVENTORY | WIELD,
	// clang-format on
};

struct TextureOverride
{
	std::string id;
	std::string texture;
	override_t target;

	// Helper function for checking if an OverrideTarget is found in
	// a TextureOverride without casting
	inline bool hasTarget(OverrideTarget overrideTarget) const
	{
		return (target & static_cast<override_t>(overrideTarget)) != 0;
	}
};

//! Class that provides texture override information from a texture pack
class TextureOverrideSource
{
public:
	TextureOverrideSource(std::string filepath);

	//! Get all overrides that apply to item definitions
	std::vector<TextureOverride> getItemTextureOverrides();

	//! Get all overrides that apply to node definitions
	std::vector<TextureOverride> getNodeTileOverrides();

private:
	std::vector<TextureOverride> m_overrides;
};
