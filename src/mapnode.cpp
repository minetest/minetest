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

#include "common_irrlicht.h"
#include "mapnode.h"
#include "porting.h"
#include <string>
#include "mineral.h"
#include "main.h" // For g_settings
#include "mapnode_contentfeatures.h"
#include "content_mapnode.h" // For mapnode_translate_*_internal

/*
	Nodes make a face if contents differ and solidness differs.
	Return value:
		0: No face
		1: Face uses m1's content
		2: Face uses m2's content
	equivalent: Whether the blocks share the same face (eg. water and glass)
*/
u8 face_contents(content_t m1, content_t m2, bool *equivalent)
{
	*equivalent = false;

	if(m1 == CONTENT_IGNORE || m2 == CONTENT_IGNORE)
		return 0;
	
	bool contents_differ = (m1 != m2);
	
	// Contents don't differ for different forms of same liquid
	if(content_liquid(m1) && content_liquid(m2)
			&& make_liquid_flowing(m1) == make_liquid_flowing(m2))
		contents_differ = false;
	
	u8 c1 = content_solidness(m1);
	u8 c2 = content_solidness(m2);

	bool solidness_differs = (c1 != c2);
	bool makes_face = contents_differ && solidness_differs;

	if(makes_face == false)
		return 0;
	
	if(c1 == 0)
		c1 = content_features(m1).visual_solidness;
	if(c2 == 0)
		c2 = content_features(m2).visual_solidness;
	
	if(c1 == c2){
		*equivalent = true;
		// If same solidness, liquid takes precense
		if(content_features(m1).liquid_type != LIQUID_NONE)
			return 1;
		if(content_features(m2).liquid_type != LIQUID_NONE)
			return 2;
	}
	
	if(c1 > c2)
		return 1;
	else
		return 2;
}

v3s16 facedir_rotate(u8 facedir, v3s16 dir)
{
	/*
		Face 2 (normally Z-) direction:
		facedir=0: Z-
		facedir=1: X-
		facedir=2: Z+
		facedir=3: X+
	*/
	v3s16 newdir;
	if(facedir==0) // Same
		newdir = v3s16(dir.X, dir.Y, dir.Z);
	else if(facedir == 1) // Face is taken from rotXZccv(-90)
		newdir = v3s16(-dir.Z, dir.Y, dir.X);
	else if(facedir == 2) // Face is taken from rotXZccv(180)
		newdir = v3s16(-dir.X, dir.Y, -dir.Z);
	else if(facedir == 3) // Face is taken from rotXZccv(90)
		newdir = v3s16(dir.Z, dir.Y, -dir.X);
	else
		newdir = dir;
	return newdir;
}

u8 packDir(v3s16 dir)
{
	u8 b = 0;

	if(dir.X > 0)
		b |= (1<<0);
	else if(dir.X < 0)
		b |= (1<<1);

	if(dir.Y > 0)
		b |= (1<<2);
	else if(dir.Y < 0)
		b |= (1<<3);

	if(dir.Z > 0)
		b |= (1<<4);
	else if(dir.Z < 0)
		b |= (1<<5);
	
	return b;
}
v3s16 unpackDir(u8 b)
{
	v3s16 d(0,0,0);

	if(b & (1<<0))
		d.X = 1;
	else if(b & (1<<1))
		d.X = -1;

	if(b & (1<<2))
		d.Y = 1;
	else if(b & (1<<3))
		d.Y = -1;

	if(b & (1<<4))
		d.Z = 1;
	else if(b & (1<<5))
		d.Z = -1;
	
	return d;
}

/*
	MapNode
*/

// These four are DEPRECATED.
bool MapNode::light_propagates()
{
	return light_propagates_content(getContent());
}
bool MapNode::sunlight_propagates()
{
	return sunlight_propagates_content(getContent());
}
u8 MapNode::solidness()
{
	return content_solidness(getContent());
}
u8 MapNode::light_source()
{
	return content_features(*this).light_source;
}

void MapNode::setLight(enum LightBank bank, u8 a_light)
{
	// If node doesn't contain light data, ignore this
	if(content_features(*this).param_type != CPT_LIGHT)
		return;
	if(bank == LIGHTBANK_DAY)
	{
		param1 &= 0xf0;
		param1 |= a_light & 0x0f;
	}
	else if(bank == LIGHTBANK_NIGHT)
	{
		param1 &= 0x0f;
		param1 |= (a_light & 0x0f)<<4;
	}
	else
		assert(0);
}

u8 MapNode::getLight(enum LightBank bank)
{
	// Select the brightest of [light source, propagated light]
	u8 light = 0;
	if(content_features(*this).param_type == CPT_LIGHT)
	{
		if(bank == LIGHTBANK_DAY)
			light = param1 & 0x0f;
		else if(bank == LIGHTBANK_NIGHT)
			light = (param1>>4)&0x0f;
		else
			assert(0);
	}
	if(light_source() > light)
		light = light_source();
	return light;
}

u8 MapNode::getLightBanksWithSource()
{
	// Select the brightest of [light source, propagated light]
	u8 lightday = 0;
	u8 lightnight = 0;
	if(content_features(*this).param_type == CPT_LIGHT)
	{
		lightday = param1 & 0x0f;
		lightnight = (param1>>4)&0x0f;
	}
	if(light_source() > lightday)
		lightday = light_source();
	if(light_source() > lightnight)
		lightnight = light_source();
	return (lightday&0x0f) | ((lightnight<<4)&0xf0);
}

#ifndef SERVER
TileSpec MapNode::getTile(v3s16 dir, ITextureSource *tsrc)
{
	if(content_features(*this).param_type == CPT_FACEDIR_SIMPLE)
		dir = facedir_rotate(param1, dir);
	
	TileSpec spec;
	
	s32 dir_i = -1;
	
	if(dir == v3s16(0,0,0))
		dir_i = -1;
	else if(dir == v3s16(0,1,0))
		dir_i = 0;
	else if(dir == v3s16(0,-1,0))
		dir_i = 1;
	else if(dir == v3s16(1,0,0))
		dir_i = 2;
	else if(dir == v3s16(-1,0,0))
		dir_i = 3;
	else if(dir == v3s16(0,0,1))
		dir_i = 4;
	else if(dir == v3s16(0,0,-1))
		dir_i = 5;
	
	if(dir_i == -1)
		// Non-directional
		spec = content_features(*this).tiles[0];
	else 
		spec = content_features(*this).tiles[dir_i];
	
	/*
		If it contains some mineral, change texture id
	*/
	if(content_features(*this).param_type == CPT_MINERAL && tsrc)
	{
		u8 mineral = getMineral();
		std::string mineral_texture_name = mineral_block_texture(mineral);
		if(mineral_texture_name != "")
		{
			u32 orig_id = spec.texture.id;
			std::string texture_name = tsrc->getTextureName(orig_id);
			//texture_name += "^blit:";
			texture_name += "^";
			texture_name += mineral_texture_name;
			u32 new_id = tsrc->getTextureId(texture_name);
			spec.texture = tsrc->getTexture(new_id);
		}
	}

	return spec;
}
#endif

u8 MapNode::getMineral()
{
	if(content_features(*this).param_type == CPT_MINERAL)
	{
		return param1 & 0x0f;
	}

	return MINERAL_NONE;
}

u32 MapNode::serializedLength(u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
		
	if(version == 0)
		return 1;
	else if(version <= 9)
		return 2;
	else
		return 3;
}
void MapNode::serialize(u8 *dest, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
		
	// Translate to wanted version
	MapNode n_foreign = mapnode_translate_from_internal(*this, version);

	u8 actual_param0 = n_foreign.param0;

	// Convert special values from new version to old
	if(version <= 18)
	{
		// In these versions, CONTENT_IGNORE and CONTENT_AIR
		// are 255 and 254
		if(actual_param0 == CONTENT_IGNORE)
			actual_param0 = 255;
		else if(actual_param0 == CONTENT_AIR)
			actual_param0 = 254;
	}

	if(version == 0)
	{
		dest[0] = actual_param0;
	}
	else if(version <= 9)
	{
		dest[0] = actual_param0;
		dest[1] = n_foreign.param1;
	}
	else
	{
		dest[0] = actual_param0;
		dest[1] = n_foreign.param1;
		dest[2] = n_foreign.param2;
	}
}
void MapNode::deSerialize(u8 *source, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapNode format not supported");
		
	if(version == 0)
	{
		param0 = source[0];
	}
	else if(version == 1)
	{
		param0 = source[0];
		// This version doesn't support saved lighting
		if(light_propagates() || light_source() > 0)
			param1 = 0;
		else
			param1 = source[1];
	}
	else if(version <= 9)
	{
		param0 = source[0];
		param1 = source[1];
	}
	else
	{
		param0 = source[0];
		param1 = source[1];
		param2 = source[2];
	}
	
	// Convert special values from old version to new
	if(version <= 18)
	{
		// In these versions, CONTENT_IGNORE and CONTENT_AIR
		// are 255 and 254
		if(param0 == 255)
			param0 = CONTENT_IGNORE;
		else if(param0 == 254)
			param0 = CONTENT_AIR;
	}
	// version 19 is fucked up with sometimes the old values and sometimes not
	if(version == 19)
	{
		if(param0 == 255)
			param0 = CONTENT_IGNORE;
		else if(param0 == 254)
			param0 = CONTENT_AIR;
	}

	// Translate to our known version
	*this = mapnode_translate_to_internal(*this, version);
}

/*
	Gets lighting value at face of node
	
	Parameters must consist of air and !air.
	Order doesn't matter.

	If either of the nodes doesn't exist, light is 0.
	
	parameters:
		daynight_ratio: 0...1000
		n: getNode(p) (uses only the lighting value)
		n2: getNode(p + face_dir) (uses only the lighting value)
		face_dir: axis oriented unit vector from p to p2
	
	returns encoded light value.
*/
u8 getFaceLight(u32 daynight_ratio, MapNode n, MapNode n2,
		v3s16 face_dir)
{
	try{
		u8 light;
		u8 l1 = n.getLightBlend(daynight_ratio);
		u8 l2 = n2.getLightBlend(daynight_ratio);
		if(l1 > l2)
			light = l1;
		else
			light = l2;

		// Make some nice difference to different sides

		// This makes light come from a corner
		/*if(face_dir.X == 1 || face_dir.Z == 1 || face_dir.Y == -1)
			light = diminish_light(diminish_light(light));
		else if(face_dir.X == -1 || face_dir.Z == -1)
			light = diminish_light(light);*/
		
		// All neighboring faces have different shade (like in minecraft)
		if(face_dir.X == 1 || face_dir.X == -1 || face_dir.Y == -1)
			light = diminish_light(diminish_light(light));
		else if(face_dir.Z == 1 || face_dir.Z == -1)
			light = diminish_light(light);

		return light;
	}
	catch(InvalidPositionException &e)
	{
		return 0;
	}
}


