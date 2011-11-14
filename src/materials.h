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

enum Diggability
{
	DIGGABLE_NOT,
	DIGGABLE_NORMAL,
	DIGGABLE_CONSTANT
};

struct MaterialProperties
{
	// Values can be anything. 0 is normal.
	
	enum Diggability diggability;

	// Constant time for DIGGABLE_CONSTANT
	float constant_time;

	// Weight; the amount of stuff in the block. Not realistic.
	float weight;
	// Rock; wood a bit.
	// A Pickaxe manages high crackiness well.
	float crackiness;
	// Sand is extremely crumble; dirt is quite crumble.
	// A shovel is good for crumbly stuff. Pickaxe is horrible.
	float crumbliness;
	// An axe is best for cuttable heavy stuff.
	// Sword is best for cuttable light stuff.
	float cuttability;
	// If high, ignites easily
	//float flammability;

	MaterialProperties():
		diggability(DIGGABLE_NOT),
		constant_time(0.5),
		weight(1),
		crackiness(1),
		crumbliness(1),
		cuttability(1)
	{}
};

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

class ToolDiggingProperties;
class INodeDefManager;

DiggingProperties getDiggingProperties(u16 content, ToolDiggingProperties *tp,
		INodeDefManager *nodemgr);

#endif

