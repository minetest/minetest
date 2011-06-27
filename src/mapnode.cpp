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
#include "tile.h"
#include "porting.h"
#include <string>
#include "mineral.h"
// For g_settings
#include "main.h"
#include "content_mapnode.h"
#include "nodemetadata.h"

ContentFeatures::~ContentFeatures()
{
	if(translate_to)
		delete translate_to;
	if(initial_metadata)
		delete initial_metadata;
}

void ContentFeatures::setTexture(u16 i, std::string name, u8 alpha)
{
	if(g_texturesource)
	{
		tiles[i].texture = g_texturesource->getTexture(name);
	}
	
	if(alpha != 255)
	{
		tiles[i].alpha = alpha;
		tiles[i].material_type = MATERIAL_ALPHA_VERTEX;
	}

	if(inventory_texture == NULL)
		setInventoryTexture(name);
}

void ContentFeatures::setInventoryTexture(std::string imgname)
{
	if(g_texturesource == NULL)
		return;
	
	imgname += "^[forcesingle";
	
	inventory_texture = g_texturesource->getTextureRaw(imgname);
}

void ContentFeatures::setInventoryTextureCube(std::string top,
		std::string left, std::string right)
{
	if(g_texturesource == NULL)
		return;
	
	str_replace_char(top, '^', '&');
	str_replace_char(left, '^', '&');
	str_replace_char(right, '^', '&');

	std::string imgname_full;
	imgname_full += "[inventorycube{";
	imgname_full += top;
	imgname_full += "{";
	imgname_full += left;
	imgname_full += "{";
	imgname_full += right;
	inventory_texture = g_texturesource->getTextureRaw(imgname_full);
}

struct ContentFeatures g_content_features[256];

ContentFeatures & content_features(u8 i)
{
	return g_content_features[i];
}

/*
	See mapnode.h for description.
*/
void init_mapnode()
{
	if(g_texturesource == NULL)
	{
		dstream<<"INFO: Initial run of init_mapnode with "
				"g_texturesource=NULL. If this segfaults, "
				"there is a bug with something not checking for "
				"the NULL value."<<std::endl;
	}
	else
	{
		dstream<<"INFO: Full run of init_mapnode with "
				"g_texturesource!=NULL"<<std::endl;
	}

	/*// Read some settings
	bool new_style_water = g_settings.getBool("new_style_water");
	bool new_style_leaves = g_settings.getBool("new_style_leaves");*/

	/*
		Initialize content feature table
	*/
	
	/*
		Set initial material type to same in all tiles, so that the
		same material can be used in more stuff.
		This is set according to the leaves because they are the only
		differing material to which all materials can be changed to
		get this optimization.
	*/
	u8 initial_material_type = MATERIAL_ALPHA_SIMPLE;
	/*if(new_style_leaves)
		initial_material_type = MATERIAL_ALPHA_SIMPLE;
	else
		initial_material_type = MATERIAL_ALPHA_NONE;*/
	for(u16 i=0; i<256; i++)
	{
		ContentFeatures *f = &g_content_features[i];
		// Re-initialize
		f->reset();

		for(u16 j=0; j<6; j++)
			f->tiles[j].material_type = initial_material_type;
	}

	/*
		Initialize mapnode content
	*/
	content_mapnode_init();
	
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

TileSpec MapNode::getTile(v3s16 dir)
{
	if(content_features(d).param_type == CPT_FACEDIR_SIMPLE)
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
		spec = content_features(d).tiles[0];
	else 
		spec = content_features(d).tiles[dir_i];
	
	/*
		If it contains some mineral, change texture id
	*/
	if(content_features(d).param_type == CPT_MINERAL && g_texturesource)
	{
		u8 mineral = param & 0x1f;
		std::string mineral_texture_name = mineral_block_texture(mineral);
		if(mineral_texture_name != "")
		{
			u32 orig_id = spec.texture.id;
			std::string texture_name = g_texturesource->getTextureName(orig_id);
			//texture_name += "^blit:";
			texture_name += "^";
			texture_name += mineral_texture_name;
			u32 new_id = g_texturesource->getTextureId(texture_name);
			spec.texture = g_texturesource->getTexture(new_id);
		}
	}

	return spec;
}

u8 MapNode::getMineral()
{
	if(content_features(d).param_type == CPT_MINERAL)
	{
		return param & 0x1f;
	}

	return MINERAL_NONE;
}

/*
	Gets lighting value at face of node
	
	Parameters must consist of air and !air.
	Order doesn't matter.

	If either of the nodes doesn't exist, light is 0.
	
	parameters:
		daynight_ratio: 0...1000
		n: getNodeParent(p)
		n2: getNodeParent(p + face_dir)
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


