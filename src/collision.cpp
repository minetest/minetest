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

#include "collision.h"
#include "mapblock.h"
#include "map.h"

collisionMoveResult collisionMoveSimple(Map *map, f32 pos_max_d,
		const core::aabbox3d<f32> &box_0,
		f32 dtime, v3f &pos_f, v3f &speed_f)
{
	collisionMoveResult result;

	v3f oldpos_f = pos_f;
	v3s16 oldpos_i = floatToInt(oldpos_f, BS);

	/*
		Calculate new position
	*/
	pos_f += speed_f * dtime;

	/*
		Collision detection
	*/
	
	// position in nodes
	v3s16 pos_i = floatToInt(pos_f, BS);
	
	/*
		Collision uncertainty radius
		Make it a bit larger than the maximum distance of movement
	*/
	f32 d = pos_max_d * 1.1;
	// A fairly large value in here makes moving smoother
	//f32 d = 0.15*BS;

	// This should always apply, otherwise there are glitches
	assert(d > pos_max_d);
	
	/*
		Calculate collision box
	*/
	core::aabbox3d<f32> box = box_0;
	box.MaxEdge += pos_f;
	box.MinEdge += pos_f;
	core::aabbox3d<f32> oldbox = box_0;
	oldbox.MaxEdge += oldpos_f;
	oldbox.MinEdge += oldpos_f;

	/*
		If the object lies on a walkable node, this is set to true.
	*/
	result.touching_ground = false;
	
	/*
		Go through every node around the object
		TODO: Calculate the range of nodes that need to be checked
	*/
	for(s16 y = oldpos_i.Y - 1; y <= oldpos_i.Y + 2; y++)
	for(s16 z = oldpos_i.Z - 1; z <= oldpos_i.Z + 1; z++)
	for(s16 x = oldpos_i.X - 1; x <= oldpos_i.X + 1; x++)
	{
		try{
			// Object collides into walkable nodes
			MapNode n = map->getNode(v3s16(x,y,z));
			if(content_features(n).walkable == false)
				continue;
		}
		catch(InvalidPositionException &e)
		{
			// Doing nothing here will block the object from
			// walking over map borders
		}

		core::aabbox3d<f32> nodebox = getNodeBox(v3s16(x,y,z), BS);
		
		/*
			See if the object is touching ground.

			Object touches ground if object's minimum Y is near node's
			maximum Y and object's X-Z-area overlaps with the node's
			X-Z-area.

			Use 0.15*BS so that it is easier to get on a node.
		*/
		if(
				//fabs(nodebox.MaxEdge.Y-box.MinEdge.Y) < d
				fabs(nodebox.MaxEdge.Y-box.MinEdge.Y) < 0.15*BS
				&& nodebox.MaxEdge.X-d > box.MinEdge.X
				&& nodebox.MinEdge.X+d < box.MaxEdge.X
				&& nodebox.MaxEdge.Z-d > box.MinEdge.Z
				&& nodebox.MinEdge.Z+d < box.MaxEdge.Z
		){
			result.touching_ground = true;
		}
		
		// If object doesn't intersect with node, ignore node.
		if(box.intersectsWithBox(nodebox) == false)
			continue;
		
		/*
			Go through every axis
		*/
		v3f dirs[3] = {
			v3f(0,0,1), // back-front
			v3f(0,1,0), // top-bottom
			v3f(1,0,0), // right-left
		};
		for(u16 i=0; i<3; i++)
		{
			/*
				Calculate values along the axis
			*/
			f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[i]);
			f32 nodemin = nodebox.MinEdge.dotProduct(dirs[i]);
			f32 objectmax = box.MaxEdge.dotProduct(dirs[i]);
			f32 objectmin = box.MinEdge.dotProduct(dirs[i]);
			f32 objectmax_old = oldbox.MaxEdge.dotProduct(dirs[i]);
			f32 objectmin_old = oldbox.MinEdge.dotProduct(dirs[i]);
			
			/*
				Check collision for the axis.
				Collision happens when object is going through a surface.
			*/
			bool negative_axis_collides =
				(nodemax > objectmin && nodemax <= objectmin_old + d
					&& speed_f.dotProduct(dirs[i]) < 0);
			bool positive_axis_collides =
				(nodemin < objectmax && nodemin >= objectmax_old - d
					&& speed_f.dotProduct(dirs[i]) > 0);
			bool main_axis_collides =
					negative_axis_collides || positive_axis_collides;
			
			/*
				Check overlap of object and node in other axes
			*/
			bool other_axes_overlap = true;
			for(u16 j=0; j<3; j++)
			{
				if(j == i)
					continue;
				f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[j]);
				f32 nodemin = nodebox.MinEdge.dotProduct(dirs[j]);
				f32 objectmax = box.MaxEdge.dotProduct(dirs[j]);
				f32 objectmin = box.MinEdge.dotProduct(dirs[j]);
				if(!(nodemax - d > objectmin && nodemin + d < objectmax))
				{
					other_axes_overlap = false;
					break;
				}
			}
			
			/*
				If this is a collision, revert the pos_f in the main
				direction.
			*/
			if(other_axes_overlap && main_axis_collides)
			{
				speed_f -= speed_f.dotProduct(dirs[i]) * dirs[i];
				pos_f -= pos_f.dotProduct(dirs[i]) * dirs[i];
				pos_f += oldpos_f.dotProduct(dirs[i]) * dirs[i];
			}
		
		}
	} // xyz
	
	return result;
}

collisionMoveResult collisionMovePrecise(Map *map, f32 pos_max_d,
		const core::aabbox3d<f32> &box_0,
		f32 dtime, v3f &pos_f, v3f &speed_f)
{
	collisionMoveResult final_result;

	// Maximum time increment (for collision detection etc)
	// time = distance / speed
	f32 dtime_max_increment = pos_max_d / speed_f.getLength();
	
	// Maximum time increment is 10ms or lower
	if(dtime_max_increment > 0.01)
		dtime_max_increment = 0.01;
	
	// Don't allow overly huge dtime
	if(dtime > 2.0)
		dtime = 2.0;
	
	f32 dtime_downcount = dtime;

	u32 loopcount = 0;
	do
	{
		loopcount++;

		f32 dtime_part;
		if(dtime_downcount > dtime_max_increment)
		{
			dtime_part = dtime_max_increment;
			dtime_downcount -= dtime_part;
		}
		else
		{
			dtime_part = dtime_downcount;
			/*
				Setting this to 0 (no -=dtime_part) disables an infinite loop
				when dtime_part is so small that dtime_downcount -= dtime_part
				does nothing
			*/
			dtime_downcount = 0;
		}

		collisionMoveResult result = collisionMoveSimple(map, pos_max_d,
				box_0, dtime_part, pos_f, speed_f);

		if(result.touching_ground)
			final_result.touching_ground = true;
	}
	while(dtime_downcount > 0.001);
		

	return final_result;
}


