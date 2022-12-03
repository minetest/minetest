/*
Minetest
Copyright (C) 2023 Andrey2470T, AndreyT <andrey012203@gmail.com>

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

#include "irrlichttypes_bloated.h"
#include <utility>
#include <string>
#include <sstream>
#include <vector>
#include "tileanimation.h"
#include "script/common/c_content.h"
#include <SMaterialLayer.h>

enum class LineType {
	FLAT,
	DIHEDRAL,
	CYLINDRIC,
	NONE
};

struct LineParams {
	enum LineProperty {
		LP_TYPE = 0,
		LP_START_POS,
		LP_END_POS,
		LP_START_COLOR,
		LP_END_COLOR,
		LP_START_ATTACH_TO,
		LP_END_ATTACH_TO,
		LP_WIDTH,
		LP_TEXTURE,
		LP_PLAYERNAME,
		LP_ANIMATION,
		LP_BACKFACE_CULLING,
		LP_LIGHT_LEVEL,
		LP_AXIS_ANGLE,
		LP_COUNT
	};

	LineParams()
	{
		reset();
	}

	LineType type;

	// Positions of start and env vertices.
	std::pair<v3f, v3f> pos;
	// Colors of start and end vertices.
	std::pair<video::SColor, video::SColor> color;
	// IDs of objects which start and end vertices are attached to.
	std::pair<u16, u16> attached_ids;

	// Thickness of the line.
	f32 width;

	std::string texture;

	// If 'playername' is not empty string, the line will be sent only to the client with that name.
	std::string playername;

	bool backface_culling;

	TileAnimationParams animation;

	// Can be from 0-14 integers.
	u8 light_level;

	// Angle in radians around the line lengthwise axis
	f32 axis_angle;

	// Contains boolean value for each property.
	// If property has 'true', it means the given property was changed in 'set_properties()' method.
	// Otherwise it has 'false'.
	// As there may be only a few changed params and a whole structure is sent, without this container
	// it would be impossible to determine which params are actually changed.
	std::vector<bool> last_changed_props;

	void reset();

	void serialize(std::ostream &os, u16 protocol_version) const;
	void deserialize(std::istream &is, u16 protocol_version);
};
