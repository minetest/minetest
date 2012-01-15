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

#include "content_sao.h"
#include "collision.h"
#include "environment.h"
#include "settings.h"
#include "main.h" // For g_profiler
#include "profiler.h"
#include "serialization.h" // For compressZlib
#include "materials.h" // For MaterialProperties
#include "tooldef.h" // ToolDiggingProperties

/* Some helper functions */

// Y is copied, X and Z change is limited
void accelerate_xz(v3f &speed, v3f target_speed, f32 max_increase)
{
	v3f d_wanted = target_speed - speed;
	d_wanted.Y = 0;
	f32 dl_wanted = d_wanted.getLength();
	f32 dl = dl_wanted;
	if(dl > max_increase)
		dl = max_increase;
	
	v3f d = d_wanted.normalize() * dl;

	speed.X += d.X;
	speed.Z += d.Z;
	speed.Y = target_speed.Y;
}

bool checkFreePosition(Map *map, v3s16 p0, v3s16 size)
{
	for(int dx=0; dx<size.X; dx++)
	for(int dy=0; dy<size.Y; dy++)
	for(int dz=0; dz<size.Z; dz++){
		v3s16 dp(dx, dy, dz);
		v3s16 p = p0 + dp;
		MapNode n = map->getNodeNoEx(p);
		if(n.getContent() != CONTENT_AIR)
			return false;
	}
	return true;
}

bool checkWalkablePosition(Map *map, v3s16 p0)
{
	v3s16 p = p0 + v3s16(0,-1,0);
	MapNode n = map->getNodeNoEx(p);
	if(n.getContent() != CONTENT_AIR)
		return true;
	return false;
}

bool checkFreeAndWalkablePosition(Map *map, v3s16 p0, v3s16 size)
{
	if(!checkFreePosition(map, p0, size))
		return false;
	if(!checkWalkablePosition(map, p0))
		return false;
	return true;
}

void get_random_u32_array(u32 a[], u32 len)
{
	u32 i, n;
	for(i=0; i<len; i++)
		a[i] = i;
	n = len;
	while(n > 1){
		u32 k = myrand() % n;
		n--;
		u32 temp = a[n];
		a[n] = a[k];
		a[k] = temp;
	}
}

void explodeSquare(Map *map, v3s16 p0, v3s16 size)
{
	core::map<v3s16, MapBlock*> modified_blocks;

	for(int dx=0; dx<size.X; dx++)
	for(int dy=0; dy<size.Y; dy++)
	for(int dz=0; dz<size.Z; dz++){
		v3s16 dp(dx - size.X/2, dy - size.Y/2, dz - size.Z/2);
		v3s16 p = p0 + dp;
		MapNode n = map->getNodeNoEx(p);
		if(n.getContent() == CONTENT_IGNORE)
			continue;
		//map->removeNodeWithEvent(p);
		map->removeNodeAndUpdate(p, modified_blocks);
	}

	// Send a MEET_OTHER event
	MapEditEvent event;
	event.type = MEET_OTHER;
	for(core::map<v3s16, MapBlock*>::Iterator
		  i = modified_blocks.getIterator();
		  i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		event.modified_blocks.insert(p, true);
	}
	map->dispatchEvent(&event);
}
