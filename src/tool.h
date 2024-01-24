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

#pragma once

#include "irrlichttypes.h"
#include <string>
#include <iostream>
#include "itemgroup.h"
#include "json-forwards.h"
#include "common/c_types.h"
#include <json/json.h>
#include <SColor.h>

struct ItemDefinition;

struct ToolGroupCap
{
	std::unordered_map<int, float> times;
	int maxlevel = 1;
	int uses = 20;

	ToolGroupCap() = default;

	bool getTime(int rating, float *time) const
	{
		std::unordered_map<int, float>::const_iterator i = times.find(rating);
		if (i == times.end()) {
			*time = 0;
			return false;
		}
		*time = i->second;
		return true;
	}

	void toJson(Json::Value &object) const;
	void fromJson(const Json::Value &json);
};


typedef std::unordered_map<std::string, struct ToolGroupCap> ToolGCMap;
typedef std::unordered_map<std::string, s16> DamageGroup;

struct ToolCapabilities
{
	float full_punch_interval;
	int max_drop_level;
	ToolGCMap groupcaps;
	DamageGroup damageGroups;
	int punch_attack_uses;

	ToolCapabilities(
			float full_punch_interval_ = 1.4f,
			int max_drop_level_ = 1,
			const ToolGCMap &groupcaps_ = ToolGCMap(),
			const DamageGroup &damageGroups_ = DamageGroup(),
			int punch_attack_uses_ = 0
	):
		full_punch_interval(full_punch_interval_),
		max_drop_level(max_drop_level_),
		groupcaps(groupcaps_),
		damageGroups(damageGroups_),
		punch_attack_uses(punch_attack_uses_)
	{}

	void serialize(std::ostream &os, u16 version) const;
	void deSerialize(std::istream &is);
	void serializeJson(std::ostream &os) const;
	void deserializeJson(std::istream &is);
};

struct WearBarParam
{
	video::SColor color;
	float minPercent; // minimum percentage *durability* to apply color for (inclusive)
	float maxPercent; // maximum percentage *durability* to apply color for (exclusive)
	WearBarParam(
			const video::SColor &color_ = video::SColor(255, 255, 255, 255),
			float minPercent_ = 0.0f,
			float maxPercent_ = 0.0f
			):
			color(color_),
			minPercent(minPercent_),
			maxPercent(maxPercent_)
	{}

	bool matches(float percent) const
	{
		return minPercent <= percent && percent < maxPercent;
	}
};

enum BlendMode: u8 {
    BLEND_MODE_CONSTANT,
    BLEND_MODE_LINEAR,
    BlendMode_END // Dummy for validity check
};

extern const EnumString es_BlendMode[];

struct WearBarParams
{
	std::map<float, video::SColor> colorStops;
	BlendMode blend;

	WearBarParams(
			const std::map<float, video::SColor> &color_stops_ = std::map<float, video::SColor>(),
			const BlendMode blend_ = BLEND_MODE_CONSTANT
			):
			colorStops(color_stops_),
			blend(blend_)
	{}

	void serialize(std::ostream &os, u16 version) const;
	void deSerialize(std::istream &is);
	void serializeJson(std::ostream &os) const;
	void deserializeJson(std::istream &is);
	video::SColor getWearBarColor(float durabilityPercent);
};

struct DigParams
{
	bool diggable;
	// Digging time in seconds
	float time;
	// Caused wear
	u32 wear; // u32 because wear could be 65536 (single-use tool)
	std::string main_group;

	DigParams(bool a_diggable = false, float a_time = 0.0f, u32 a_wear = 0,
			const std::string &a_main_group = ""):
		diggable(a_diggable),
		time(a_time),
		wear(a_wear),
		main_group(a_main_group)
	{}
};

DigParams getDigParams(const ItemGroupList &groups,
		const ToolCapabilities *tp,
		const u16 initial_wear = 0);

struct HitParams
{
	s32 hp;
	// Caused wear
	u32 wear; // u32 because wear could be 65536 (single-use weapon)

	HitParams(s32 hp_ = 0, u32 wear_ = 0):
		hp(hp_),
		wear(wear_)
	{}
};

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp, float time_from_last_punch,
		u16 initial_wear = 0);

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp);

struct PunchDamageResult
{
	bool did_punch = false;
	int damage = 0;
	int wear = 0;

	PunchDamageResult() = default;
};

struct ItemStack;

PunchDamageResult getPunchDamage(
		const ItemGroupList &armor_groups,
		const ToolCapabilities *toolcap,
		const ItemStack *punchitem,
		float time_from_last_punch,
		u16 initial_wear = 0
);

u32 calculateResultWear(const u32 uses, const u16 initial_wear);
f32 getToolRange(const ItemDefinition &def_selected, const ItemDefinition &def_hand);
