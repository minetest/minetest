/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "voxelalgorithms.h"
#include "nodedef.h"

namespace voxalgo
{

void setLight(VoxelManipulator &v, VoxelArea a, u8 light,
		INodeDefManager *ndef)
{
	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
	{
		v3s16 p(x,y,z);
		MapNode &n = v.getNodeRefUnsafe(p);
		n.setLight(LIGHTBANK_DAY, light, ndef);
		n.setLight(LIGHTBANK_NIGHT, light, ndef);
	}
}

void clearLightAndCollectSources(VoxelManipulator &v, VoxelArea a,
		enum LightBank bank, INodeDefManager *ndef,
		core::map<v3s16, bool> & light_sources,
		core::map<v3s16, u8> & unlight_from)
{
	// The full area we shall touch
	VoxelArea required_a = a;
	required_a.pad(v3s16(0,0,0));
	// Make sure we have access to it
	v.emerge(a);

	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
	{
		v3s16 p(x,y,z);
		MapNode &n = v.getNodeRefUnsafe(p);
		u8 oldlight = n.getLight(bank, ndef);
		n.setLight(bank, 0, ndef);

		// If node sources light, add to list
		u8 source = ndef->get(n).light_source;
		if(source != 0)
			light_sources[p] = true;

		// Collect borders for unlighting
		if((x==a.MinEdge.X || x == a.MaxEdge.X
		|| y==a.MinEdge.Y || y == a.MaxEdge.Y
		|| z==a.MinEdge.Z || z == a.MaxEdge.Z)
		&& oldlight != 0)
		{
			unlight_from.insert(p, oldlight);
		}
	}
}

SunlightPropagateResult propagateSunlight(VoxelManipulator &v, VoxelArea a,
		bool inexistent_top_provides_sunlight,
		core::map<v3s16, bool> & light_sources,
		INodeDefManager *ndef)
{
	// Return values
	bool bottom_sunlight_valid = true;

	// The full area we shall touch extends one extra at top and bottom
	VoxelArea required_a = a;
	required_a.pad(v3s16(0,1,0));
	// Make sure we have access to it
	v.emerge(a);
	
	s16 max_y = a.MaxEdge.Y;
	s16 min_y = a.MinEdge.Y;
	
	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	{
		v3s16 p_overtop(x, max_y+1, z);
		bool overtop_has_sunlight = false;
		// If overtop node does not exist, trust heuristics
		if(!v.exists(p_overtop))
			overtop_has_sunlight = inexistent_top_provides_sunlight;
		else if(v.getNodeRefUnsafe(p_overtop).getContent() == CONTENT_IGNORE)
			overtop_has_sunlight = inexistent_top_provides_sunlight;
		// Otherwise refer to it's light value
		else
			overtop_has_sunlight = (v.getNodeRefUnsafe(p_overtop).getLight(
					LIGHTBANK_DAY, ndef) == LIGHT_SUN);

		// Copy overtop's sunlight all over the place
		u8 incoming_light = overtop_has_sunlight ? LIGHT_SUN : 0;
		for(s32 y=max_y; y>=min_y; y--)
		{
			v3s16 p(x,y,z);
			MapNode &n = v.getNodeRefUnsafe(p);
			if(incoming_light == 0){
				// Do nothing
			} else if(incoming_light == LIGHT_SUN &&
					ndef->get(n).sunlight_propagates){
				// Do nothing
			} else if(ndef->get(n).sunlight_propagates == false){
				incoming_light = 0;
			} else {
				incoming_light = diminish_light(incoming_light);
			}
			u8 old_light = n.getLight(LIGHTBANK_DAY, ndef);

			if(incoming_light > old_light)
				n.setLight(LIGHTBANK_DAY, incoming_light, ndef);
			
			if(diminish_light(incoming_light) != 0)
				light_sources.insert(p, true);
		}
		
		// Check validity of sunlight at top of block below if it
		// hasn't already been proven invalid
		if(bottom_sunlight_valid)
		{
			bool sunlight_should_continue_down = (incoming_light == LIGHT_SUN);
			v3s16 p_overbottom(x, min_y-1, z);
			if(!v.exists(p_overbottom) ||
					v.getNodeRefUnsafe(p_overbottom
							).getContent() == CONTENT_IGNORE){
				// Is not known, cannot compare
			} else {
				bool overbottom_has_sunlight = (v.getNodeRefUnsafe(p_overbottom
						).getLight(LIGHTBANK_DAY, ndef) == LIGHT_SUN);
				if(sunlight_should_continue_down != overbottom_has_sunlight){
					bottom_sunlight_valid = false;
				}
			}
		}
	}

	return SunlightPropagateResult(bottom_sunlight_valid);
}

} // namespace voxalgo

