/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef TOOL_HEADER
#define TOOL_HEADER

#include "irrlichttypes.h"
#include <string>
#include <iostream>
#include <map>
#include "itemgroup.h"

struct ToolGroupCap
{
	std::map<int, float> times;
	int maxlevel;
	int uses;

	ToolGroupCap():
		maxlevel(1),
		uses(20)
	{}

	bool getTime(int rating, float *time) const
	{
		std::map<int, float>::const_iterator i = times.find(rating);
		if(i == times.end()){
			*time = 0;
			return false;
		}
		*time = i->second;
		return true;
	}
};


// CLANG SUCKS DONKEY BALLS
typedef std::map<std::string, struct ToolGroupCap> ToolGCMap;

struct ToolCapabilities
{
	float full_punch_interval;
	int max_drop_level;
	// CLANG SUCKS DONKEY BALLS
	ToolGCMap groupcaps;

	ToolCapabilities(
			float full_punch_interval_=1.4,
			int max_drop_level_=1,
			// CLANG SUCKS DONKEY BALLS
			ToolGCMap groupcaps_=ToolGCMap()
	):
		full_punch_interval(full_punch_interval_),
		max_drop_level(max_drop_level_),
		groupcaps(groupcaps_)
	{}

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
};

struct DigParams
{
	bool diggable;
	// Digging time in seconds
	float time;
	// Caused wear
	u16 wear;
	std::string main_group;

	DigParams(bool a_diggable=false, float a_time=0, u16 a_wear=0,
			std::string a_main_group=""):
		diggable(a_diggable),
		time(a_time),
		wear(a_wear),
		main_group(a_main_group)
	{}
};

DigParams getDigParams(const ItemGroupList &groups,
		const ToolCapabilities *tp, float time_from_last_punch);

DigParams getDigParams(const ItemGroupList &groups,
		const ToolCapabilities *tp);

struct HitParams
{
	s16 hp;
	s16 wear;
	std::string main_group;

	HitParams(s16 hp_=0, s16 wear_=0, std::string main_group_=""):
		hp(hp_),
		wear(wear_),
		main_group(main_group_)
	{}
};

HitParams getHitParams(const ItemGroupList &groups,
		const ToolCapabilities *tp, float time_from_last_punch);

HitParams getHitParams(const ItemGroupList &groups,
		const ToolCapabilities *tp);

struct PunchDamageResult
{
	bool did_punch;
	int damage;
	int wear;
	std::string main_group;

	PunchDamageResult():
		did_punch(false),
		damage(0),
		wear(0)
	{}
};

struct ItemStack;

PunchDamageResult getPunchDamage(
		const ItemGroupList &armor_groups,
		const ToolCapabilities *toolcap,
		const ItemStack *punchitem,
		float time_from_last_punch
);

#endif

