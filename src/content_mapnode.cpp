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

// For g_settings
#include "main.h"

#include "content_mapnode.h"
#include "mapnode.h"
#include "content_nodemeta.h"

// TODO: Get rid of these and set up some attributes like toughness,
//       fluffyness, and a funciton to calculate time and durability loss
//       (and sound? and whatever else) from them
void setStoneLikeDiggingProperties(DiggingPropertiesList &list, float toughness);
void setDirtLikeDiggingProperties(DiggingPropertiesList &list, float toughness);
void setWoodLikeDiggingProperties(DiggingPropertiesList &list, float toughness);

content_t trans_table_19[21][2] = {
	{CONTENT_GRASS, 1},
	{CONTENT_TREE, 4},
	{CONTENT_LEAVES, 5},
	{CONTENT_GRASS_FOOTSTEPS, 6},
	{CONTENT_MESE, 7},
	{CONTENT_MUD, 8},
	{CONTENT_CLOUD, 10},
	{CONTENT_COALSTONE, 11},
	{CONTENT_WOOD, 12},
	{CONTENT_SAND, 13},
	{CONTENT_COBBLE, 18},
	{CONTENT_STEEL, 19},
	{CONTENT_GLASS, 20},
	{CONTENT_MOSSYCOBBLE, 22},
	{CONTENT_GRAVEL, 23},
	{CONTENT_SANDSTONE, 24},
	{CONTENT_CACTUS, 25},
	{CONTENT_BRICK, 26},
	{CONTENT_CLAY, 27},
	{CONTENT_PAPYRUS, 28},
	{CONTENT_BOOKSHELF, 29},
};

MapNode mapnode_translate_from_internal(MapNode n_from, u8 version)
{
	MapNode result = n_from;
	if(version <= 19)
	{
		content_t c_from = n_from.getContent();
		for(u32 i=0; i<sizeof(trans_table_19)/sizeof(trans_table_19[0]); i++)
		{
			if(trans_table_19[i][0] == c_from)
			{
				result.setContent(trans_table_19[i][1]);
				break;
			}
		}
	}
	return result;
}
MapNode mapnode_translate_to_internal(MapNode n_from, u8 version)
{
	MapNode result = n_from;
	if(version <= 19)
	{
		content_t c_from = n_from.getContent();
		for(u32 i=0; i<sizeof(trans_table_19)/sizeof(trans_table_19[0]); i++)
		{
			if(trans_table_19[i][1] == c_from)
			{
				result.setContent(trans_table_19[i][0]);
				break;
			}
		}
	}
	return result;
}

void content_mapnode_init()
{
	// Read some settings
	bool new_style_water = g_settings.getBool("new_style_water");
	bool new_style_leaves = g_settings.getBool("new_style_leaves");
	bool invisible_stone = g_settings.getBool("invisible_stone");

	content_t i;
	ContentFeatures *f = NULL;

	i = CONTENT_STONE;
	f = &content_features(i);
	f->setAllTextures("stone.png");
	f->setInventoryTextureCube("stone.png", "stone.png", "stone.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_COBBLE)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 1.0);
	if(invisible_stone)
		f->solidness = 0; // For debugging, hides regular stone
	
	i = CONTENT_GRASS;
	f = &content_features(i);
	f->setAllTextures("mud.png^grass_side.png");
	f->setTexture(0, "grass.png");
	f->setTexture(1, "mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_MUD)+" 1";
	setDirtLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_GRASS_FOOTSTEPS;
	f = &content_features(i);
	f->setAllTextures("mud.png^grass_side.png");
	f->setTexture(0, "grass_footsteps.png");
	f->setTexture(1, "mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_MUD)+" 1";
	setDirtLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_MUD;
	f = &content_features(i);
	f->setAllTextures("mud.png");
	f->setInventoryTextureCube("mud.png", "mud.png", "mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setDirtLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_SAND;
	f = &content_features(i);
	f->setAllTextures("sand.png");
	f->setInventoryTextureCube("sand.png", "sand.png", "sand.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setDirtLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_GRAVEL;
	f = &content_features(i);
	f->setAllTextures("gravel.png");
	f->setInventoryTextureCube("gravel.png", "gravel.png", "gravel.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setDirtLikeDiggingProperties(f->digging_properties, 1.75);
	
	i = CONTENT_SANDSTONE;
	f = &content_features(i);
	f->setAllTextures("sandstone.png");
	f->setInventoryTextureCube("sandstone.png", "sandstone.png", "sandstone.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_SAND)+" 1";
	setDirtLikeDiggingProperties(f->digging_properties, 1.0);

	i = CONTENT_CLAY;
	f = &content_features(i);
	f->setAllTextures("clay.png");
	f->setInventoryTextureCube("clay.png", "clay.png", "clay.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("CraftItem lump_of_clay 4");
	setDirtLikeDiggingProperties(f->digging_properties, 1.0);

	i = CONTENT_BRICK;
	f = &content_features(i);
	f->setAllTextures("brick.png");
	f->setInventoryTextureCube("brick.png", "brick.png", "brick.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("CraftItem clay_brick 4");
	setStoneLikeDiggingProperties(f->digging_properties, 1.0);

	i = CONTENT_TREE;
	f = &content_features(i);
	f->setAllTextures("tree.png");
	f->setTexture(0, "tree_top.png");
	f->setTexture(1, "tree_top.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_JUNGLETREE;
	f = &content_features(i);
	f->setAllTextures("jungletree.png");
	f->setTexture(0, "jungletree_top.png");
	f->setTexture(1, "jungletree_top.png");
	f->param_type = CPT_MINERAL;
	//f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_JUNGLEGRASS;
	f = &content_features(i);
	f->setInventoryTexture("junglegrass.png");
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	//f->is_ground_content = true;
	f->air_equivalent = false; // grass grows underneath
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	setWoodLikeDiggingProperties(f->digging_properties, 0.10);

	i = CONTENT_LEAVES;
	f = &content_features(i);
	f->light_propagates = true;
	//f->param_type = CPT_MINERAL;
	f->param_type = CPT_LIGHT;
	//f->is_ground_content = true;
	if(new_style_leaves)
	{
		f->solidness = 0; // drawn separately, makes no faces
		f->setInventoryTextureCube("leaves.png", "leaves.png", "leaves.png");
	}
	else
	{
		f->setAllTextures("[noalpha:leaves.png");
	}
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeDiggingProperties(f->digging_properties, 0.15);

	i = CONTENT_CACTUS;
	f = &content_features(i);
	f->setAllTextures("cactus_side.png");
	f->setTexture(0, "cactus_top.png");
	f->setTexture(1, "cactus_top.png");
	f->setInventoryTextureCube("cactus_top.png", "cactus_side.png", "cactus_side.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeDiggingProperties(f->digging_properties, 0.75);

	i = CONTENT_PAPYRUS;
	f = &content_features(i);
	f->setInventoryTexture("papyrus.png");
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	setWoodLikeDiggingProperties(f->digging_properties, 0.25);

	i = CONTENT_BOOKSHELF;
	f = &content_features(i);
	f->setAllTextures("bookshelf.png");
	f->setTexture(0, "wood.png");
	f->setTexture(1, "wood.png");
	// FIXME: setInventoryTextureCube() only cares for the first texture
	f->setInventoryTextureCube("bookshelf.png", "bookshelf.png", "bookshelf.png");
	//f->setInventoryTextureCube("wood.png", "bookshelf.png", "bookshelf.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	setWoodLikeDiggingProperties(f->digging_properties, 0.75);

	i = CONTENT_GLASS;
	f = &content_features(i);
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->setInventoryTextureCube("glass.png", "glass.png", "glass.png");
	setWoodLikeDiggingProperties(f->digging_properties, 0.15);

	i = CONTENT_FENCE;
	f = &content_features(i);
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->air_equivalent = true; // grass grows underneath
	f->setInventoryTexture("item_fence.png");
	setWoodLikeDiggingProperties(f->digging_properties, 0.75);

	i = CONTENT_RAIL;
	f = &content_features(i);
	f->setInventoryTexture("rail.png");
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->air_equivalent = true; // grass grows underneath
	f->walkable = false;
	setDirtLikeDiggingProperties(f->digging_properties, 0.75);

	i = CONTENT_LADDER;
	f = &content_features(i);
	f->setInventoryTexture("item_ladder.png");
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem ")+itos(i)+" 1";
	f->wall_mounted = true;
	f->solidness = 0;
	f->air_equivalent = true;
	f->walkable = false;
	setWoodLikeDiggingProperties(f->digging_properties, 0.5);

	// Deprecated
	i = CONTENT_COALSTONE;
	f = &content_features(i);
	f->setAllTextures("stone.png^mineral_coal.png");
	f->is_ground_content = true;
	setStoneLikeDiggingProperties(f->digging_properties, 1.5);
	
	i = CONTENT_WOOD;
	f = &content_features(i);
	f->setAllTextures("wood.png");
	f->setInventoryTextureCube("wood.png", "wood.png", "wood.png");
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeDiggingProperties(f->digging_properties, 0.75);
	
	i = CONTENT_MESE;
	f = &content_features(i);
	f->setAllTextures("mese.png");
	f->setInventoryTextureCube("mese.png", "mese.png", "mese.png");
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 0.5);
	
	i = CONTENT_CLOUD;
	f = &content_features(i);
	f->setAllTextures("cloud.png");
	f->setInventoryTextureCube("cloud.png", "cloud.png", "cloud.png");
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	
	i = CONTENT_AIR;
	f = &content_features(i);
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0;
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->air_equivalent = true;
	
	i = CONTENT_WATER;
	f = &content_features(i);
	f->setInventoryTextureCube("water.png", "water.png", "water.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->solidness = 0; // Drawn separately, makes no faces
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_FLOWING;
	f->liquid_alternative_flowing = CONTENT_WATER;
	f->liquid_alternative_source = CONTENT_WATERSOURCE;
	
	i = CONTENT_WATERSOURCE;
	f = &content_features(i);
	//f->setInventoryTexture("water.png");
	f->setInventoryTextureCube("water.png", "water.png", "water.png");
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
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->liquid_alternative_flowing = CONTENT_WATER;
	f->liquid_alternative_source = CONTENT_WATERSOURCE;
	
	i = CONTENT_TORCH;
	f = &content_features(i);
	f->setInventoryTexture("torch_on_floor.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	f->wall_mounted = true;
	f->air_equivalent = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->light_source = LIGHT_MAX-1;
	f->digging_properties.set("", DiggingProperties(true, 0.0, 0));
	
	i = CONTENT_SIGN_WALL;
	f = &content_features(i);
	f->setInventoryTexture("sign_wall.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	f->wall_mounted = true;
	f->air_equivalent = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new SignNodeMetadata("Some sign");
	f->digging_properties.set("", DiggingProperties(true, 0.5, 0));
	
	i = CONTENT_CHEST;
	f = &content_features(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("chest_side.png");
	f->setTexture(0, "chest_top.png");
	f->setTexture(1, "chest_top.png");
	f->setTexture(5, "chest_front.png"); // Z-
	f->setInventoryTexture("chest_top.png");
	//f->setInventoryTextureCube("chest_top.png", "chest_side.png", "chest_side.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new ChestNodeMetadata();
	setWoodLikeDiggingProperties(f->digging_properties, 1.0);
	
	i = CONTENT_FURNACE;
	f = &content_features(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("furnace_side.png");
	f->setTexture(5, "furnace_front.png"); // Z-
	f->setInventoryTexture("furnace_front.png");
	//f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_COBBLE)+" 6";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new FurnaceNodeMetadata();
	setStoneLikeDiggingProperties(f->digging_properties, 3.0);
	
	i = CONTENT_COBBLE;
	f = &content_features(i);
	f->setAllTextures("cobble.png");
	f->setInventoryTextureCube("cobble.png", "cobble.png", "cobble.png");
	f->param_type = CPT_NONE;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 0.9);

	i = CONTENT_MOSSYCOBBLE;
	f = &content_features(i);
	f->setAllTextures("mossycobble.png");
	f->setInventoryTextureCube("mossycobble.png", "mossycobble.png", "mossycobble.png");
	f->param_type = CPT_NONE;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 0.8);
	
	i = CONTENT_STEEL;
	f = &content_features(i);
	f->setAllTextures("steel_block.png");
	f->setInventoryTextureCube("steel_block.png", "steel_block.png",
			"steel_block.png");
	f->param_type = CPT_NONE;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 5.0);
	
	i = CONTENT_NC;
	f = &content_features(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("nc_side.png");
	f->setTexture(5, "nc_front.png"); // Z-
	f->setTexture(4, "nc_back.png"); // Z+
	f->setInventoryTexture("nc_front.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 3.0);
	
	i = CONTENT_NC_RB;
	f = &content_features(i);
	f->setAllTextures("nc_rb.png");
	f->setInventoryTexture("nc_rb.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeDiggingProperties(f->digging_properties, 3.0);
	
	// NOTE: Remember to add frequently used stuff to the texture atlas in tile.cpp
	

	/*
		Add MesePick to everything
	*/
	for(u16 i=0; i<=MAX_CONTENT; i++)
	{
		content_features(i).digging_properties.set("MesePick",
				DiggingProperties(true, 0.0, 65535./1337));
	}

}

void setStoneLikeDiggingProperties(DiggingPropertiesList &list, float toughness)
{
	list.set("",
			DiggingProperties(true, 15.0*toughness, 0));
	
	list.set("WPick",
			DiggingProperties(true, 1.3*toughness, 65535./30.*toughness));
	list.set("STPick",
			DiggingProperties(true, 0.75*toughness, 65535./100.*toughness));
	list.set("SteelPick",
			DiggingProperties(true, 0.50*toughness, 65535./333.*toughness));

	/*list.set("MesePick",
			DiggingProperties(true, 0.0*toughness, 65535./20.*toughness));*/
}

void setDirtLikeDiggingProperties(DiggingPropertiesList &list, float toughness)
{
	list.set("",
			DiggingProperties(true, 0.75*toughness, 0));
	
	list.set("WShovel",
			DiggingProperties(true, 0.4*toughness, 65535./50.*toughness));
	list.set("STShovel",
			DiggingProperties(true, 0.2*toughness, 65535./150.*toughness));
	list.set("SteelShovel",
			DiggingProperties(true, 0.15*toughness, 65535./400.*toughness));
}

void setWoodLikeDiggingProperties(DiggingPropertiesList &list, float toughness)
{
	list.set("",
			DiggingProperties(true, 3.0*toughness, 0));
	
	list.set("WAxe",
			DiggingProperties(true, 1.5*toughness, 65535./30.*toughness));
	list.set("STAxe",
			DiggingProperties(true, 0.75*toughness, 65535./100.*toughness));
	list.set("SteelAxe",
			DiggingProperties(true, 0.5*toughness, 65535./333.*toughness));
}


