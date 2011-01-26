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

#include "mapnode.h"
#include "tile.h"
#include "porting.h"
#include <string>
#include "mineral.h"

ContentFeatures::~ContentFeatures()
{
	if(translate_to)
		delete translate_to;
}

struct ContentFeatures g_content_features[256];

void init_mapnode()
{
	u8 i;
	ContentFeatures *f = NULL;

	i = CONTENT_STONE;
	f = &g_content_features[i];
	f->setAllTextures("stone.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_GRASS;
	f = &g_content_features[i];
	//f->setAllTextures("mud.png[[mod:sidegrass");
	f->setAllTextures("mud.png[[mod:blitname:grass_side.png");
	f->setTexture(0, "grass.png");
	f->setTexture(1, "mud.png");
	f->setInventoryImage("grass.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_GRASS_FOOTSTEPS;
	f = &g_content_features[i];
	//f->setAllTextures("mud.png[[mod:sidegrass");
	f->setAllTextures("mud.png[[mod:blitname:grass_side.png");
	f->setTexture(0, "grass_footsteps.png");
	f->setTexture(1, "mud.png");
	f->setInventoryImage("grass_footsteps.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_MUD;
	f = &g_content_features[i];
	f->setAllTextures("mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_SAND;
	f = &g_content_features[i];
	f->setAllTextures("mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_TREE;
	f = &g_content_features[i];
	f->setAllTextures("tree.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_LEAVES;
	f = &g_content_features[i];
	f->setAllTextures("leaves.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_COALSTONE;
	f = &g_content_features[i];
	f->translate_to = new MapNode(CONTENT_STONE, MINERAL_COAL);
	/*f->setAllTextures("coalstone.png");
	f->is_ground_content = true;*/
	
	i = CONTENT_WOOD;
	f = &g_content_features[i];
	f->setAllTextures("wood.png");
	f->is_ground_content = true;
	
	i = CONTENT_MESE;
	f = &g_content_features[i];
	f->setAllTextures("mese.png");
	f->is_ground_content = true;
	
	i = CONTENT_CLOUD;
	f = &g_content_features[i];
	f->setAllTextures("cloud.png");
	f->is_ground_content = true;
	
	i = CONTENT_AIR;
	f = &g_content_features[i];
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0;
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	
	i = CONTENT_WATER;
	f = &g_content_features[i];
	f->setInventoryImage("water.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->solidness = 0; // Drawn separately, makes no faces
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_FLOWING;
	
	i = CONTENT_WATERSOURCE;
	f = &g_content_features[i];
	f->setTexture(0, "water.png", WATER_ALPHA);
	f->setInventoryImage("water.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->solidness = 1;
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_SOURCE;
	
	i = CONTENT_TORCH;
	f = &g_content_features[i];
	f->setInventoryImage("torch_on_floor.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	f->wall_mounted = true;
	
}

TileSpec MapNode::getTile(v3s16 dir)
{
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
	
	if(content_features(d).param_type == CPT_MINERAL)
	{
		u8 mineral = param & 0x1f;
		const char *ts = mineral_block_texture(mineral);
		if(ts[0] != 0)
		{
			spec.name += "[[mod:blitname:";
			spec.name += ts;
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

// Pointers to c_str()s g_content_features[i].inventory_image_path
const char * g_content_inventory_texture_paths[USEFUL_CONTENT_COUNT] = {0};

void init_content_inventory_texture_paths()
{
	for(u16 i=0; i<USEFUL_CONTENT_COUNT; i++)
	{
		g_content_inventory_texture_paths[i] =
				g_content_features[i].inventory_image_path.c_str();
	}
}

