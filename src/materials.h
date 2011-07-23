/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef MATERIALS_HEADER
#define MATERIALS_HEADER

/*
	Material properties
*/

#include "common_irrlicht.h"
#include <string>

struct DiggingProperties
{
	DiggingProperties():
		diggable(false),
		time(0.0),
		wear(0)
	{
	}
	DiggingProperties(bool a_diggable, float a_time, u16 a_wear):
		diggable(a_diggable),
		time(a_time),
		wear(a_wear)
	{
	}
	bool diggable;
	// Digging time in seconds
	float time;
	// Caused wear
	u16 wear;
};

/*
	This is a bad way of determining mining characteristics.
	TODO: Get rid of this and set up some attributes like toughness,
	      fluffyness, and a funciton to calculate time and durability loss
	      (and sound? and whatever else) from them
*/
class DiggingPropertiesList
{
public:
	DiggingPropertiesList()
	{
	}

	void set(const std::string toolname,
			const DiggingProperties &prop)
	{
		m_digging_properties[toolname] = prop;
	}

	DiggingProperties get(const std::string toolname)
	{
		core::map<std::string, DiggingProperties>::Node *n;
		n = m_digging_properties.find(toolname);
		if(n == NULL)
		{
			// Not diggable by this tool, try to get defaults
			n = m_digging_properties.find("");
			if(n == NULL)
			{
				// Not diggable at all
				return DiggingProperties();
			}
		}
		// Return found properties
		return n->getValue();
	}

	void clear()
	{
		m_digging_properties.clear();
	}

private:
	// toolname="": default properties (digging by hand)
	// Key is toolname
	core::map<std::string, DiggingProperties> m_digging_properties;
};

// For getting the default properties, set tool=""
DiggingProperties getDiggingProperties(u16 material, const std::string &tool);

#endif

