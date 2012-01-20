/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <iostream>

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
	float flammability;

	MaterialProperties():
		diggability(DIGGABLE_NOT),
		constant_time(0.5),
		weight(0),
		crackiness(0),
		crumbliness(0),
		cuttability(0),
		flammability(0)
	{}

	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
};

struct ToolDiggingProperties
{
	// time = basetime + sum(feature here * feature in MaterialProperties)
	float full_punch_interval;
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

	ToolDiggingProperties(float full_punch_interval_=2.0,
			float a=0.75, float b=0, float c=0, float d=0, float e=0,
			float f=50, float g=0, float h=0, float i=0, float j=0);

	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
};

struct DiggingProperties
{
	bool diggable;
	// Digging time in seconds
	float time;
	// Caused wear
	u16 wear;

	DiggingProperties(bool a_diggable=false, float a_time=0, u16 a_wear=0):
		diggable(a_diggable),
		time(a_time),
		wear(a_wear)
	{}
};

DiggingProperties getDiggingProperties(const MaterialProperties *mp,
		const ToolDiggingProperties *tp, float time_from_last_punch);

DiggingProperties getDiggingProperties(const MaterialProperties *mp,
		const ToolDiggingProperties *tp);

struct HittingProperties
{
	s16 hp;
	s16 wear;

	HittingProperties(s16 hp_=0, s16 wear_=0):
		hp(hp_),
		wear(wear_)
	{}
};

HittingProperties getHittingProperties(const MaterialProperties *mp,
		const ToolDiggingProperties *tp, float time_from_last_punch);

HittingProperties getHittingProperties(const MaterialProperties *mp,
		const ToolDiggingProperties *tp);

#endif

