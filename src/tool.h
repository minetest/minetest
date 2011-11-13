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

#ifndef TOOL_HEADER
#define TOOL_HEADER

#include <string>

struct ToolDiggingProperties
{
	// time = basetime + sum(feature here * feature in MaterialProperties)
	float basetime;
	float dt_weight;
	float dt_crackiness;
	float dt_crumbliness;
	float dt_cuttability;
	float basedurability;
	float dd_weight;
	float dd_crackiness;
	float dd_crumbliness;
	float dd_cuttability;

	ToolDiggingProperties(float a=0.75, float b=0, float c=0, float d=0, float e=0,
			float f=50, float g=0, float h=0, float i=0, float j=0):
		basetime(a),
		dt_weight(b),
		dt_crackiness(c),
		dt_crumbliness(d),
		dt_cuttability(e),
		basedurability(f),
		dd_weight(g),
		dd_crackiness(h),
		dd_crumbliness(i),
		dd_cuttability(j)
	{}
};

std::string tool_get_imagename(const std::string &toolname);

ToolDiggingProperties tool_get_digging_properties(const std::string &toolname);

#endif

