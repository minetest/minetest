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
#include "itemgroup.h"
#include "log.h"
#include "inventory.h"
#include "exceptions.h"
#include "util/serialize.h"
#include "util/numeric.h"

void ToolCapabilities::serialize(std::ostream &os, u16 protocol_version) const
{
	if(protocol_version <= 17)
		writeU8(os, 1); // version
	else
		writeU8(os, 2); // version
	writeF1000(os, full_punch_interval);
	writeS16(os, max_drop_level);
	writeU32(os, groupcaps.size());
	for(std::map<std::string, ToolGroupCap>::const_iterator
			i = groupcaps.begin(); i != groupcaps.end(); i++){
		const std::string *name = &i->first;
		const ToolGroupCap *cap = &i->second;
		os<<serializeString(*name);
		writeS16(os, cap->uses);
		writeS16(os, cap->maxlevel);
		writeU32(os, cap->times.size());
		for(std::map<int, float>::const_iterator
				i = cap->times.begin(); i != cap->times.end(); i++){
			writeS16(os, i->first);
			writeF1000(os, i->second);
		}
	}
	if(protocol_version > 17){
		writeU32(os, damageGroups.size());
		for(std::map<std::string, s16>::const_iterator
				i = damageGroups.begin(); i != damageGroups.end(); i++){
			os<<serializeString(i->first);
			writeS16(os, i->second);
		}
	}
}

void ToolCapabilities::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 1 && version != 2) throw SerializationError(
			"unsupported ToolCapabilities version");
	full_punch_interval = readF1000(is);
	max_drop_level = readS16(is);
	groupcaps.clear();
	u32 groupcaps_size = readU32(is);
	for(u32 i=0; i<groupcaps_size; i++){
		std::string name = deSerializeString(is);
		ToolGroupCap cap;
		cap.uses = readS16(is);
		cap.maxlevel = readS16(is);
		u32 times_size = readU32(is);
		for(u32 i=0; i<times_size; i++){
			int level = readS16(is);
			float time = readF1000(is);
			cap.times[level] = time;
		}
		groupcaps[name] = cap;
	}
	if(version == 2)
	{
		u32 damage_groups_size = readU32(is);
		for(u32 i=0; i<damage_groups_size; i++){
			std::string name = deSerializeString(is);
			s16 rating = readS16(is);
			damageGroups[name] = rating;
		}
	}
}

DigParams getDigParams(const ItemGroupList &groups,
		const ToolCapabilities *tp, float time_from_last_punch)
{
	//infostream<<"getDigParams"<<std::endl;
	/* Check group dig_immediate */
	switch(itemgroup_get(groups, "dig_immediate")){
	case 2:
		//infostream<<"dig_immediate=2"<<std::endl;
		return DigParams(true, 0.5, 0, "dig_immediate");
	case 3:
		//infostream<<"dig_immediate=3"<<std::endl;
		return DigParams(true, 0.0, 0, "dig_immediate");
	default:
		break;
	}
	
	// Values to be returned (with a bit of conversion)
	bool result_diggable = false;
	float result_time = 0.0;
	float result_wear = 0.0;
	std::string result_main_group = "";

	int level = itemgroup_get(groups, "level");
	//infostream<<"level="<<level<<std::endl;
	for(std::map<std::string, ToolGroupCap>::const_iterator
			i = tp->groupcaps.begin(); i != tp->groupcaps.end(); i++){
		const std::string &name = i->first;
		//infostream<<"group="<<name<<std::endl;
		const ToolGroupCap &cap = i->second;
		int rating = itemgroup_get(groups, name);
		float time = 0;
		bool time_exists = cap.getTime(rating, &time);
		if(!result_diggable || time < result_time){
			if(cap.maxlevel >= level && time_exists){
				result_diggable = true;
				int leveldiff = cap.maxlevel - level;
				result_time = time / MYMAX(1, leveldiff);
				if(cap.uses != 0)
					result_wear = 1.0 / cap.uses / pow(3.0, (double)leveldiff);
				else
					result_wear = 0;
				result_main_group = name;
			}
		}
	}
	//infostream<<"result_diggable="<<result_diggable<<std::endl;
	//infostream<<"result_time="<<result_time<<std::endl;
	//infostream<<"result_wear="<<result_wear<<std::endl;

	if(time_from_last_punch < tp->full_punch_interval){
		float f = time_from_last_punch / tp->full_punch_interval;
		//infostream<<"f="<<f<<std::endl;
		result_time /= f;
		result_wear /= f;
	}

	u16 wear_i = 65535.*result_wear;
	return DigParams(result_diggable, result_time, wear_i, result_main_group);
}

DigParams getDigParams(const ItemGroupList &groups,
		const ToolCapabilities *tp)
{
	return getDigParams(groups, tp, 1000000);
}

HitParams getHitParams(const ItemGroupList &armor_groups,
		const ToolCapabilities *tp, float time_from_last_punch)
{
	s16 damage = 0;
	float full_punch_interval = tp->full_punch_interval;

	for(std::map<std::string, s16>::const_iterator
			i = tp->damageGroups.begin(); i != tp->damageGroups.end(); i++){
		s16 armor = itemgroup_get(armor_groups, i->first);
		damage += i->second * rangelim(time_from_last_punch / full_punch_interval, 0.0, 1.0)
				* armor / 100.0;
	}

	return HitParams(damage, 0);
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
		float time_from_last_punch
){
	bool do_hit = true;
	{
		if(do_hit && punchitem){
			if(itemgroup_get(armor_groups, "punch_operable") &&
					(toolcap == NULL || punchitem->name == ""))
				do_hit = false;
		}
		if(do_hit){
			if(itemgroup_get(armor_groups, "immortal"))
				do_hit = false;
		}
	}
	
	PunchDamageResult result;
	if(do_hit)
	{
		HitParams hitparams = getHitParams(armor_groups, toolcap,
				time_from_last_punch);
		result.did_punch = true;
		result.wear = hitparams.wear;
		result.damage = hitparams.hp;
	}

	return result;
}


