/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "util/cpp11_container.h"
#include "itemgroup.h"

struct ToolGroupCap
{
	UNORDERED_MAP<int, float> times;
	int maxlevel;
	int uses;

	ToolGroupCap():
		maxlevel(1),
		uses(20)
	{}

	bool getTime(int rating, float *time) const
	{
		UNORDERED_MAP<int, float>::const_iterator i = times.find(rating);
		if (i == times.end()) {
			*time = 0;
			return false;
		}
		*time = i->second;
		return true;
	}
};


typedef UNORDERED_MAP<std::string, struct ToolGroupCap> ToolGCMap;
typedef UNORDERED_MAP<std::string, s16> DamageGroup;

struct ToolCapabilities
{
	float full_punch_interval;
	int max_drop_level;
	ToolGCMap groupcaps;
	DamageGroup damageGroups;

	ToolCapabilities(
			float full_punch_interval_=1.4,
			int max_drop_level_=1,
			const ToolGCMap &groupcaps_ = ToolGCMap(),
			const DamageGroup &damageGroups_ = DamageGroup()
	):
		full_punch_interval(full_punch_interval_),
		max_drop_level(max_drop_level_),
		groupcaps(groupcaps_),
		damageGroups(damageGroups_)
	{}

	void serialize(std::ostream &os, u16 version) const;
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

	DigParams(bool a_diggable = false, float a_time = 0.0f, u16 a_wear = 0,
			const std::string &a_main_group = ""):
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

	HitParams(s16 hp_=0, s16 wear_=0):
		hp(hp_),
		wear(wear_)
	{}
};

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp, float time_from_last_punch);

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp);

struct PunchDamageResult
{
	bool did_punch;
	int damage;
	int wear;

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

