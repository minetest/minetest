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

ContentFeatures::~ContentFeatures()
{
	if(translate_to)
		delete translate_to;
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
}

struct ContentFeatures g_content_features[256];

ContentFeatures & content_features(u8 i)
{
	return g_content_features[i];
}

void init_mapnode(IIrrlichtWrapper *irrlicht)
{
	// Read some settings
	bool new_style_water = g_settings.getBool("new_style_water");
	bool new_style_leaves = g_settings.getBool("new_style_leaves");

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
		for(u16 j=0; j<6; j++)
			f->tiles[j].material_type = initial_material_type;
	}
	
	u8 i;
	ContentFeatures *f = NULL;

	i = CONTENT_STONE;
	f = &g_content_features[i];
	f->setAllTextures("stone.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_GRASS;
	f = &g_content_features[i];
	f->setAllTextures("mud.png^grass_side.png");
	f->setTexture(0, "grass.png");
	f->setTexture(1, "mud.png");
	//f->setInventoryTexture(irrlicht->getTextureId("grass.png"));
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_GRASS_FOOTSTEPS;
	f = &g_content_features[i];
	//f->setInventoryTexture(irrlicht->getTextureId("grass_footsteps.png"));
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_MUD;
	f = &g_content_features[i];
	f->setAllTextures("mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_SAND;
	f = &g_content_features[i];
	f->setAllTextures("sand.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_TREE;
	f = &g_content_features[i];
	f->setAllTextures("tree.png");
	f->setTexture(0, "tree_top.png");
	f->setTexture(1, "tree_top.png");
	//f->setInventoryTexture(irrlicht->getTextureId("tree_top.png"));
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	
	i = CONTENT_LEAVES;
	f = &g_content_features[i];
	f->light_propagates = true;
	//f->param_type = CPT_MINERAL;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	if(new_style_leaves)
	{
		f->solidness = 0; // drawn separately, makes no faces
	}
	else
	{
		f->setAllTextures("[noalpha:leaves.png");
	}
	
	i = CONTENT_COALSTONE;
	f = &g_content_features[i];
	//f->translate_to = new MapNode(CONTENT_STONE, MINERAL_COAL);
	/*f->setAllTextures(TextureSpec(irrlicht->getTextureId("coal.png"),
			irrlicht->getTextureId("mineral_coal.png")));*/
	f->setAllTextures("stone.png^mineral_coal.png");
	f->is_ground_content = true;
	
	i = CONTENT_WOOD;
	f = &g_content_features[i];
	//f->setAllTextures(irrlicht->getTextureId("wood.png"));
	f->setAllTextures("wood.png");
	f->is_ground_content = true;
	
	i = CONTENT_MESE;
	f = &g_content_features[i];
	//f->setAllTextures(irrlicht->getTextureId("mese.png"));
	f->setAllTextures("mese.png");
	f->is_ground_content = true;
	
	i = CONTENT_CLOUD;
	f = &g_content_features[i];
	//f->setAllTextures(irrlicht->getTextureId("cloud.png"));
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
	//f->setInventoryTexture(irrlicht->getTextureId("water.png"));
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
	//f->setInventoryTexture(irrlicht->getTextureId("water.png"));
	if(new_style_water)
	{
		f->solidness = 0; // drawn separately, makes no faces
	}
	else // old style
	{
		f->solidness = 1;

		TileSpec t;
		if(g_texturesource)
			t.texture = g_texturesource->getTexture("water.png");
		
		t.alpha = WATER_ALPHA;
		t.material_type = MATERIAL_ALPHA_VERTEX;
		t.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
		f->setAllTiles(t);
	}
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_SOURCE;
	
	i = CONTENT_TORCH;
	f = &g_content_features[i];
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

// Pointers to c_str()s g_content_features[i].inventory_image_path
//const char * g_content_inventory_texture_paths[USEFUL_CONTENT_COUNT] = {0};

void init_content_inventory_texture_paths()
{
	dstream<<"DEPRECATED "<<__FUNCTION_NAME<<std::endl;
	/*for(u16 i=0; i<USEFUL_CONTENT_COUNT; i++)
	{
		g_content_inventory_texture_paths[i] =
				g_content_features[i].inventory_image_path.c_str();
	}*/
}

