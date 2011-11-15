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

#include "materials.h"
#include "mapnode.h"
#include "nodedef.h"
#include "tooldef.h"
#include "utility.h"

void MaterialProperties::serialize(std::ostream &os)
{
	writeU8(os, 0); // version
	writeU8(os, diggability);
	writeF1000(os, weight);
	writeF1000(os, crackiness);
	writeF1000(os, crumbliness);
	writeF1000(os, cuttability);
}

void MaterialProperties::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0)
		throw SerializationError("unsupported MaterialProperties version");
	diggability = (enum Diggability)readU8(is);
	weight = readF1000(is);
	crackiness = readF1000(is);
	crumbliness = readF1000(is);
	cuttability = readF1000(is);
}

DiggingProperties getDiggingProperties(u16 content, ToolDiggingProperties *tp,
		INodeDefManager *nodemgr)
{
	assert(tp);
	const MaterialProperties &mp = nodemgr->get(content).material;
	if(mp.diggability == DIGGABLE_NOT)
		return DiggingProperties(false, 0, 0);
	if(mp.diggability == DIGGABLE_CONSTANT)
		return DiggingProperties(true, mp.constant_time, 0);

	float time = tp->basetime;
	time += tp->dt_weight * mp.weight;
	time += tp->dt_crackiness * mp.crackiness;
	time += tp->dt_crumbliness * mp.crumbliness;
	time += tp->dt_cuttability * mp.cuttability;
	if(time < 0.2)
		time = 0.2;

	float durability = tp->basedurability;
	durability += tp->dd_weight * mp.weight;
	durability += tp->dd_crackiness * mp.crackiness;
	durability += tp->dd_crumbliness * mp.crumbliness;
	durability += tp->dd_cuttability * mp.cuttability;
	if(durability < 1)
		durability = 1;

	float wear = 1.0 / durability;
	u16 wear_i = wear/65535.;
	return DiggingProperties(true, time, wear_i);
}

