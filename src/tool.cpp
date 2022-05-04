/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "tool.h"
#include "itemdef.h"
#include "itemgroup.h"
#include "log.h"
#include "inventory.h"
#include "exceptions.h"
#include "convert_json.h"
#include "util/serialize.h"
#include "util/numeric.h"

void ToolGroupCap::toJson(Json::Value &object) const
{
	object["maxlevel"] = maxlevel;
	object["uses"] = uses;

	Json::Value times_object;
	for (auto time : times)
		times_object[time.first] = time.second;
	object["times"] = times_object;
}

void ToolGroupCap::fromJson(const Json::Value &json)
{
	if (json.isObject()) {
		if (json["maxlevel"].isInt())
			maxlevel = json["maxlevel"].asInt();
		if (json["uses"].isInt())
			uses = json["uses"].asInt();
		const Json::Value &times_object = json["times"];
		if (times_object.isArray()) {
			Json::ArrayIndex size = times_object.size();
			for (Json::ArrayIndex i = 0; i < size; ++i)
				if (times_object[i].isDouble())
					times[i] = times_object[i].asFloat();
		}
	}
}

void ToolCapabilities::serialize(std::ostream &os, u16 protocol_version) const
{
	if (protocol_version >= 38)
		writeU8(os, 5);
	else
		writeU8(os, 4); // proto == 37
	writeF32(os, full_punch_interval);
	writeS16(os, max_drop_level);
	writeU32(os, groupcaps.size());
	for (const auto &groupcap : groupcaps) {
		const std::string *name = &groupcap.first;
		const ToolGroupCap *cap = &groupcap.second;
		os << serializeString16(*name);
		writeS16(os, cap->uses);
		writeS16(os, cap->maxlevel);
		writeU32(os, cap->times.size());
		for (const auto &time : cap->times) {
			writeS16(os, time.first);
			writeF32(os, time.second);
		}
	}

	writeU32(os, damageGroups.size());

	for (const auto &damageGroup : damageGroups) {
		os << serializeString16(damageGroup.first);
		writeS16(os, damageGroup.second);
	}

	if (protocol_version >= 38)
		writeU16(os, rangelim(punch_attack_uses, 0, U16_MAX));
}

void ToolCapabilities::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version < 4)
		throw SerializationError("unsupported ToolCapabilities version");

	full_punch_interval = readF32(is);
	max_drop_level = readS16(is);
	groupcaps.clear();
	u32 groupcaps_size = readU32(is);
	for (u32 i = 0; i < groupcaps_size; i++) {
		std::string name = deSerializeString16(is);
		ToolGroupCap cap;
		cap.uses = readS16(is);
		cap.maxlevel = readS16(is);
		u32 times_size = readU32(is);
		for(u32 i = 0; i < times_size; i++) {
			int level = readS16(is);
			float time = readF32(is);
			cap.times[level] = time;
		}
		groupcaps[name] = cap;
	}

	u32 damage_groups_size = readU32(is);
	for (u32 i = 0; i < damage_groups_size; i++) {
		std::string name = deSerializeString16(is);
		s16 rating = readS16(is);
		damageGroups[name] = rating;
	}

	if (version >= 5)
		punch_attack_uses = readU16(is);
}

void ToolCapabilities::serializeJson(std::ostream &os) const
{
	Json::Value root;
	root["full_punch_interval"] = full_punch_interval;
	root["max_drop_level"] = max_drop_level;
	root["punch_attack_uses"] = punch_attack_uses;

	Json::Value groupcaps_object;
	for (const auto &groupcap : groupcaps) {
		groupcap.second.toJson(groupcaps_object[groupcap.first]);
	}
	root["groupcaps"] = groupcaps_object;

	Json::Value damage_groups_object;
	DamageGroup::const_iterator dgiter;
	for (dgiter = damageGroups.begin(); dgiter != damageGroups.end(); ++dgiter) {
		damage_groups_object[dgiter->first] = dgiter->second;
	}
	root["damage_groups"] = damage_groups_object;

	fastWriteJson(root, os);
}

void ToolCapabilities::deserializeJson(std::istream &is)
{
	Json::Value root;
	is >> root;
	if (root.isObject()) {
		if (root["full_punch_interval"].isDouble())
			full_punch_interval = root["full_punch_interval"].asFloat();
		if (root["max_drop_level"].isInt())
			max_drop_level = root["max_drop_level"].asInt();
		if (root["punch_attack_uses"].isInt())
			punch_attack_uses = root["punch_attack_uses"].asInt();

		Json::Value &groupcaps_object = root["groupcaps"];
		if (groupcaps_object.isObject()) {
			Json::ValueIterator gciter;
			for (gciter = groupcaps_object.begin();
					gciter != groupcaps_object.end(); ++gciter) {
				ToolGroupCap groupcap;
				groupcap.fromJson(*gciter);
				groupcaps[gciter.key().asString()] = groupcap;
			}
		}

		Json::Value &damage_groups_object = root["damage_groups"];
		if (damage_groups_object.isObject()) {
			Json::ValueIterator dgiter;
			for (dgiter = damage_groups_object.begin();
					dgiter != damage_groups_object.end(); ++dgiter) {
				Json::Value &value = *dgiter;
				if (value.isInt())
					damageGroups[dgiter.key().asString()] =
						value.asInt();
			}
		}
	}
}

static u32 calculateResultWear(const u32 uses, const u16 initial_wear)
{
	if (uses == 0) {
		// Trivial case: Infinite uses
		return 0;
	}
	/* Finite uses. This is not trivial,
	as the maximum wear is not neatly evenly divisible by
	most possible uses numbers. For example, for 128
	uses, the calculation of wear is trivial, as
	65536 / 128 uses = 512 wear,
	so the tool will get 512 wear 128 times in its lifetime.
	But for a number like 130, this does not work:
	65536 / 130 uses = 504.123... wear.
	Since wear must be an integer, we will get
	504*130 = 65520, which would lead to the wrong number
	of uses.

	Instead, we partition the "wear range" into blocks:
	A block represents a single use and can be
	of two possible sizes: normal and oversized.
	A normal block is equal to floor(65536 / uses).
	An oversized block is a normal block plus 1.
	Then we determine how many oversized and normal
	blocks we need and finally, whether we add
	the normal wear or the oversized wear.

	Example for 130 uses:
	* Normal wear = 504
	* Number of normal blocks = 114
	* Oversized wear = 505
	* Number of oversized blocks = 16

	If we add everything together, we get:
	  114*504 + 16*505 = 65536
	*/
	u32 result_wear;
	u32 wear_normal = ((U16_MAX+1) / uses);
	// Will be non-zero if its not evenly divisible
	u16 blocks_oversize = (U16_MAX+1) % uses;
	// Whether to add one extra wear point in case
	// of oversized wear.
	u16 wear_extra = 0;
	if (blocks_oversize > 0) {
		u16 blocks_normal = uses - blocks_oversize;
		/* When the wear has reached this value, we
		   know that wear_normal has been applied
		   for blocks_normal times, therefore,
		   only oversized blocks remain.
		   This also implies the raw tool wear number
		   increases a bit faster after this point,
		   but this should be barely noticable by the
		   player.
		*/
		u16 wear_extra_at = blocks_normal * wear_normal;
		if (initial_wear >= wear_extra_at) {
			wear_extra = 1;
		}
	}
	result_wear = wear_normal + wear_extra;
	return result_wear;
}

DigParams getDigParams(const ItemGroupList &groups,
		const ToolCapabilities *tp,
		const u16 initial_wear)
{

	// Group dig_immediate defaults to fixed time and no wear
	if (tp->groupcaps.find("dig_immediate") == tp->groupcaps.cend()) {
		switch (itemgroup_get(groups, "dig_immediate")) {
		case 2:
			return DigParams(true, 0.5, 0, "dig_immediate");
		case 3:
			return DigParams(true, 0, 0, "dig_immediate");
		default:
			break;
		}
	}

	// Values to be returned (with a bit of conversion)
	bool result_diggable = false;
	float result_time = 0.0;
	u32 result_wear = 0;
	std::string result_main_group;

	int level = itemgroup_get(groups, "level");
	for (const auto &groupcap : tp->groupcaps) {
		const ToolGroupCap &cap = groupcap.second;

		int leveldiff = cap.maxlevel - level;
		if (leveldiff < 0)
			continue;

		const std::string &groupname = groupcap.first;
		float time = 0;
		int rating = itemgroup_get(groups, groupname);
		bool time_exists = cap.getTime(rating, &time);
		if (!time_exists)
			continue;

		if (leveldiff > 1)
			time /= leveldiff;
		if (!result_diggable || time < result_time) {
			result_time = time;
			result_diggable = true;
			// The actual number of uses increases
			// exponentially with leveldiff.
			// If the levels are equal, real_uses equals cap.uses.
			u32 real_uses = cap.uses * pow(3.0, leveldiff);
			real_uses = MYMIN(real_uses, U16_MAX);
			result_wear = calculateResultWear(real_uses, initial_wear);
			result_main_group = groupname;
		}
	}

	return DigParams(result_diggable, result_time, result_wear, result_main_group);
}

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp, float time_from_last_punch,
		u16 initial_wear)
{
	s32 damage = 0;
	float result_wear = 0.0f;
	float punch_interval_multiplier =
			rangelim(time_from_last_punch / tp->full_punch_interval, 0.0f, 1.0f);

	for (const auto &damageGroup : tp->damageGroups) {
		s16 armor = itemgroup_get(armor_groups, damageGroup.first);
		damage += damageGroup.second * punch_interval_multiplier * armor / 100.0;
	}

	if (tp->punch_attack_uses > 0) {
		result_wear = calculateResultWear(tp->punch_attack_uses, initial_wear);
		result_wear *= punch_interval_multiplier;
	}
	// Keep damage in sane bounds for simplicity
	damage = rangelim(damage, -U16_MAX, U16_MAX);

	u32 wear_i = (u32) result_wear;
	return {damage, wear_i};
}

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp)
{
	return getHitParams(armor_groups, tp, 1000000);
}

PunchDamageResult getPunchDamage(
		const ItemGroupList &armor_groups,
		const ToolCapabilities *toolcap,
		const ItemStack *punchitem,
		float time_from_last_punch,
		u16 initial_wear
){
	bool do_hit = true;
	{
		if (do_hit && punchitem) {
			if (itemgroup_get(armor_groups, "punch_operable") &&
					(toolcap == NULL || punchitem->name.empty()))
				do_hit = false;
		}

		if (do_hit) {
			if(itemgroup_get(armor_groups, "immortal"))
				do_hit = false;
		}
	}

	PunchDamageResult result;
	if(do_hit)
	{
		HitParams hitparams = getHitParams(armor_groups, toolcap,
				time_from_last_punch,
				punchitem->wear);
		result.did_punch = true;
		result.wear = hitparams.wear;
		result.damage = hitparams.hp;
	}

	return result;
}

f32 getToolRange(const ItemDefinition &def_selected, const ItemDefinition &def_hand)
{
	float max_d = def_selected.range;
	float max_d_hand = def_hand.range;

	if (max_d < 0 && max_d_hand >= 0)
		max_d = max_d_hand;
	else if (max_d < 0)
		max_d = 4.0f;

	return max_d;
}

