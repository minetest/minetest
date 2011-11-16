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

#include "content_mapnode.h"

#include "irrlichttypes.h"
#include "mapnode.h"
#include "content_nodemeta.h"
#include "nodedef.h"
#include "utility.h"
#include "nameidmapping.h"
#include <map>

/*
	Legacy node definitions
*/

#define WATER_ALPHA 160

#define WATER_VISC 1
#define LAVA_VISC 7

void setConstantMaterialProperties(MaterialProperties &mprop, float time)
{
	mprop.diggability = DIGGABLE_CONSTANT;
	mprop.constant_time = time;
}

void setStoneLikeMaterialProperties(MaterialProperties &mprop, float toughness)
{
	mprop.diggability = DIGGABLE_NORMAL;
	mprop.weight = 5.0 * toughness;
	mprop.crackiness = 1.0;
	mprop.crumbliness = -0.1;
	mprop.cuttability = -0.2;
}

void setDirtLikeMaterialProperties(MaterialProperties &mprop, float toughness)
{
	mprop.diggability = DIGGABLE_NORMAL;
	mprop.weight = toughness * 1.2;
	mprop.crackiness = 0;
	mprop.crumbliness = 1.2;
	mprop.cuttability = -0.4;
}

void setGravelLikeMaterialProperties(MaterialProperties &mprop, float toughness)
{
	mprop.diggability = DIGGABLE_NORMAL;
	mprop.weight = toughness * 2.0;
	mprop.crackiness = 0.2;
	mprop.crumbliness = 1.5;
	mprop.cuttability = -1.0;
}

void setWoodLikeMaterialProperties(MaterialProperties &mprop, float toughness)
{
	mprop.diggability = DIGGABLE_NORMAL;
	mprop.weight = toughness * 1.0;
	mprop.crackiness = 0.75;
	mprop.crumbliness = -1.0;
	mprop.cuttability = 1.5;
}

void setLeavesLikeMaterialProperties(MaterialProperties &mprop, float toughness)
{
	mprop.diggability = DIGGABLE_NORMAL;
	mprop.weight = -0.5 * toughness;
	mprop.crackiness = 0;
	mprop.crumbliness = 0;
	mprop.cuttability = 2.0;
}

void setGlassLikeMaterialProperties(MaterialProperties &mprop, float toughness)
{
	mprop.diggability = DIGGABLE_NORMAL;
	mprop.weight = 0.1 * toughness;
	mprop.crackiness = 2.0;
	mprop.crumbliness = -1.0;
	mprop.cuttability = -1.0;
}

/*
	Legacy node content type IDs
	Ranges:
	0x000...0x07f (0...127): param2 is fully usable
	126 and 127 are reserved (CONTENT_AIR and CONTENT_IGNORE).
	0x800...0xfff (2048...4095): higher 4 bytes of param2 are not usable
*/
#define CONTENT_STONE 0
#define CONTENT_WATER 2
#define CONTENT_TORCH 3
#define CONTENT_WATERSOURCE 9
#define CONTENT_SIGN_WALL 14
#define CONTENT_CHEST 15
#define CONTENT_FURNACE 16
#define CONTENT_LOCKABLE_CHEST 17
#define CONTENT_FENCE 21
#define CONTENT_RAIL 30
#define CONTENT_LADDER 31
#define CONTENT_LAVA 32
#define CONTENT_LAVASOURCE 33
#define CONTENT_GRASS 0x800 //1
#define CONTENT_TREE 0x801 //4
#define CONTENT_LEAVES 0x802 //5
#define CONTENT_GRASS_FOOTSTEPS 0x803 //6
#define CONTENT_MESE 0x804 //7
#define CONTENT_MUD 0x805 //8
#define CONTENT_CLOUD 0x806 //10
#define CONTENT_COALSTONE 0x807 //11
#define CONTENT_WOOD 0x808 //12
#define CONTENT_SAND 0x809 //13
#define CONTENT_COBBLE 0x80a //18
#define CONTENT_STEEL 0x80b //19
#define CONTENT_GLASS 0x80c //20
#define CONTENT_MOSSYCOBBLE 0x80d //22
#define CONTENT_GRAVEL 0x80e //23
#define CONTENT_SANDSTONE 0x80f //24
#define CONTENT_CACTUS 0x810 //25
#define CONTENT_BRICK 0x811 //26
#define CONTENT_CLAY 0x812 //27
#define CONTENT_PAPYRUS 0x813 //28
#define CONTENT_BOOKSHELF 0x814 //29
#define CONTENT_JUNGLETREE 0x815
#define CONTENT_JUNGLEGRASS 0x816
#define CONTENT_NC 0x817
#define CONTENT_NC_RB 0x818
#define CONTENT_APPLE 0x819
#define CONTENT_SAPLING 0x820

/*
	A conversion table for backwards compatibility.
	Maps <=v19 content types to current ones.
	Should never be touched.
*/
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

void content_mapnode_get_name_id_mapping(NameIdMapping *nimap)
{
	nimap->set(0, "stone");
	nimap->set(2, "water_flowing");
	nimap->set(3, "torch");
	nimap->set(9, "water_source");
	nimap->set(14, "sign_wall");
	nimap->set(15, "chest");
	nimap->set(16, "furnace");
	nimap->set(17, "locked_chest");
	nimap->set(21, "wooden_fence");
	nimap->set(30, "rail");
	nimap->set(31, "ladder");
	nimap->set(32, "lava_flowing");
	nimap->set(33, "lava_source");
	nimap->set(0x800, "dirt_with_grass");
	nimap->set(0x801, "tree");
	nimap->set(0x802, "leaves");
	nimap->set(0x803, "dirt_with_grass_footsteps");
	nimap->set(0x804, "mese");
	nimap->set(0x805, "dirt");
	nimap->set(0x806, "cloud");
	nimap->set(0x807, "coalstone");
	nimap->set(0x808, "wood");
	nimap->set(0x809, "sand");
	nimap->set(0x80a, "cobble");
	nimap->set(0x80b, "steel");
	nimap->set(0x80c, "glass");
	nimap->set(0x80d, "mossycobble");
	nimap->set(0x80e, "gravel");
	nimap->set(0x80f, "sandstone");
	nimap->set(0x810, "cactus");
	nimap->set(0x811, "brick");
	nimap->set(0x812, "clay");
	nimap->set(0x813, "papyrus");
	nimap->set(0x814, "bookshelf");
	nimap->set(0x815, "jungletree");
	nimap->set(0x816, "junglegrass");
	nimap->set(0x817, "nyancat");
	nimap->set(0x818, "nyancat_rainbow");
	nimap->set(0x819, "apple");
	nimap->set(0x820, "sapling");
	// Static types
	nimap->set(CONTENT_IGNORE, "ignore");
	nimap->set(CONTENT_AIR, "air");
}

class NewNameGetter
{
public:
	NewNameGetter()
	{
		old_to_new["CONTENT_STONE"] = "stone";
		old_to_new["CONTENT_WATER"] = "water_flowing";
		old_to_new["CONTENT_TORCH"] = "torch";
		old_to_new["CONTENT_WATERSOURCE"] = "water_source";
		old_to_new["CONTENT_SIGN_WALL"] = "sign_wall";
		old_to_new["CONTENT_CHEST"] = "chest";
		old_to_new["CONTENT_FURNACE"] = "furnace";
		old_to_new["CONTENT_LOCKABLE_CHEST"] = "locked_chest";
		old_to_new["CONTENT_FENCE"] = "wooden_fence";
		old_to_new["CONTENT_RAIL"] = "rail";
		old_to_new["CONTENT_LADDER"] = "ladder";
		old_to_new["CONTENT_LAVA"] = "lava_flowing";
		old_to_new["CONTENT_LAVASOURCE"] = "lava_source";
		old_to_new["CONTENT_GRASS"] = "dirt_with_grass";
		old_to_new["CONTENT_TREE"] = "tree";
		old_to_new["CONTENT_LEAVES"] = "leaves";
		old_to_new["CONTENT_GRASS_FOOTSTEPS"] = "dirt_with_grass_footsteps";
		old_to_new["CONTENT_MESE"] = "mese";
		old_to_new["CONTENT_MUD"] = "dirt";
		old_to_new["CONTENT_CLOUD"] = "cloud";
		old_to_new["CONTENT_COALSTONE"] = "coalstone";
		old_to_new["CONTENT_WOOD"] = "wood";
		old_to_new["CONTENT_SAND"] = "sand";
		old_to_new["CONTENT_COBBLE"] = "cobble";
		old_to_new["CONTENT_STEEL"] = "steel";
		old_to_new["CONTENT_GLASS"] = "glass";
		old_to_new["CONTENT_MOSSYCOBBLE"] = "mossycobble";
		old_to_new["CONTENT_GRAVEL"] = "gravel";
		old_to_new["CONTENT_SANDSTONE"] = "sandstone";
		old_to_new["CONTENT_CACTUS"] = "cactus";
		old_to_new["CONTENT_BRICK"] = "brick";
		old_to_new["CONTENT_CLAY"] = "clay";
		old_to_new["CONTENT_PAPYRUS"] = "papyrus";
		old_to_new["CONTENT_BOOKSHELF"] = "bookshelf";
		old_to_new["CONTENT_JUNGLETREE"] = "jungletree";
		old_to_new["CONTENT_JUNGLEGRASS"] = "junglegrass";
		old_to_new["CONTENT_NC"] = "nyancat";
		old_to_new["CONTENT_NC_RB"] = "nyancat_rainbow";
		old_to_new["CONTENT_APPLE"] = "apple";
		old_to_new["CONTENT_SAPLING"] = "sapling";
		// Just in case
		old_to_new["CONTENT_IGNORE"] = "ignore";
		old_to_new["CONTENT_AIR"] = "air";
	}
	std::string get(const std::string &old)
	{
		std::map<std::string, std::string>::const_iterator i;
		i = old_to_new.find(old);
		if(i == old_to_new.end())
			return "";
		return i->second;
	}
private:
	std::map<std::string, std::string> old_to_new;
};

NewNameGetter newnamegetter;

std::string content_mapnode_get_new_name(const std::string &oldname)
{
	return newnamegetter.get(oldname);
}

content_t legacy_get_id(const std::string &oldname, INodeDefManager *ndef)
{
	std::string newname = content_mapnode_get_new_name(oldname);
	if(newname == "")
		return CONTENT_IGNORE;
	content_t id;
	bool found = ndef->getId(newname, id);
	if(!found)
		return CONTENT_IGNORE;
	return id;
}

// Initialize default (legacy) node definitions
void content_mapnode_init(IWritableNodeDefManager *nodemgr)
{
	content_t i;
	ContentFeatures f;

	i = CONTENT_STONE;
	f = ContentFeatures();
	f.name = "stone";
	f.setAllTextures("stone.png");
	f.setInventoryTextureCube("stone.png", "stone.png", "stone.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.often_contains_mineral = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(CONTENT_COBBLE)+" 1";
	setStoneLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_GRASS;
	f = ContentFeatures();
	f.name = "dirt_with_grass";
	f.setAllTextures("mud.png^grass_side.png");
	f.setTexture(0, "grass.png");
	f.setTexture(1, "mud.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(CONTENT_MUD)+" 1";
	setDirtLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_GRASS_FOOTSTEPS;
	f = ContentFeatures();
	f.name = "dirt_with_grass_footsteps";
	f.setAllTextures("mud.png^grass_side.png");
	f.setTexture(0, "grass_footsteps.png");
	f.setTexture(1, "mud.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(CONTENT_MUD)+" 1";
	setDirtLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_MUD;
	f = ContentFeatures();
	f.name = "dirt";
	f.setAllTextures("mud.png");
	f.setInventoryTextureCube("mud.png", "mud.png", "mud.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setDirtLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_SAND;
	f = ContentFeatures();
	f.name = "sand";
	f.setAllTextures("sand.png");
	f.setInventoryTextureCube("sand.png", "sand.png", "sand.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.cookresult_item = std::string("MaterialItem2 ")+itos(CONTENT_GLASS)+" 1";
	setDirtLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_GRAVEL;
	f = ContentFeatures();
	f.name = "gravel";
	f.setAllTextures("gravel.png");
	f.setInventoryTextureCube("gravel.png", "gravel.png", "gravel.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setGravelLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_SANDSTONE;
	f = ContentFeatures();
	f.name = "sandstone";
	f.setAllTextures("sandstone.png");
	f.setInventoryTextureCube("sandstone.png", "sandstone.png", "sandstone.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(CONTENT_SAND)+" 1";
	setDirtLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_CLAY;
	f = ContentFeatures();
	f.name = "clay";
	f.setAllTextures("clay.png");
	f.setInventoryTextureCube("clay.png", "clay.png", "clay.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("CraftItem lump_of_clay 4");
	setDirtLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_BRICK;
	f = ContentFeatures();
	f.name = "brick";
	f.setAllTextures("brick.png");
	f.setInventoryTextureCube("brick.png", "brick.png", "brick.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("CraftItem clay_brick 4");
	setStoneLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_TREE;
	f = ContentFeatures();
	f.name = "tree";
	f.setAllTextures("tree.png");
	f.setTexture(0, "tree_top.png");
	f.setTexture(1, "tree_top.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.cookresult_item = "CraftItem lump_of_coal 1";
	f.furnace_burntime = 30;
	setWoodLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_JUNGLETREE;
	f = ContentFeatures();
	f.name = "jungletree";
	f.setAllTextures("jungletree.png");
	f.setTexture(0, "jungletree_top.png");
	f.setTexture(1, "jungletree_top.png");
	f.param_type = CPT_MINERAL;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.furnace_burntime = 30;
	setWoodLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_JUNGLEGRASS;
	f = ContentFeatures();
	f.name = "junglegrass";
	f.drawtype = NDT_PLANTLIKE;
	f.visual_scale = 1.3;
	f.setAllTextures("junglegrass.png");
	f.setInventoryTexture("junglegrass.png");
	f.light_propagates = true;
	f.param_type = CPT_LIGHT;
	f.air_equivalent = false; // grass grows underneath
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.walkable = false;
	setLeavesLikeMaterialProperties(f.material, 1.0);
	f.furnace_burntime = 2;
	nodemgr->set(i, f);

	i = CONTENT_LEAVES;
	f = ContentFeatures();
	f.name = "leaves";
	f.drawtype = NDT_ALLFACES_OPTIONAL;
	f.setAllTextures("leaves.png");
	//f.setAllTextures("[noalpha:leaves.png");
	f.light_propagates = true;
	f.param_type = CPT_LIGHT;
	f.extra_dug_item = std::string("MaterialItem2 ")+itos(CONTENT_SAPLING)+" 1";
	f.extra_dug_item_rarity = 20;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setLeavesLikeMaterialProperties(f.material, 1.0);
	f.furnace_burntime = 1.0;
	nodemgr->set(i, f);

	i = CONTENT_CACTUS;
	f = ContentFeatures();
	f.name = "cactus";
	f.setAllTextures("cactus_side.png");
	f.setTexture(0, "cactus_top.png");
	f.setTexture(1, "cactus_top.png");
	f.setInventoryTextureCube("cactus_top.png", "cactus_side.png", "cactus_side.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeMaterialProperties(f.material, 0.75);
	f.furnace_burntime = 15;
	nodemgr->set(i, f);

	i = CONTENT_PAPYRUS;
	f = ContentFeatures();
	f.name = "papyrus";
	f.drawtype = NDT_PLANTLIKE;
	f.setAllTextures("papyrus.png");
	f.setInventoryTexture("papyrus.png");
	f.light_propagates = true;
	f.param_type = CPT_LIGHT;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.walkable = false;
	setLeavesLikeMaterialProperties(f.material, 0.5);
	f.furnace_burntime = 1;
	nodemgr->set(i, f);

	i = CONTENT_BOOKSHELF;
	f = ContentFeatures();
	f.name = "bookshelf";
	f.setAllTextures("bookshelf.png");
	f.setTexture(0, "wood.png");
	f.setTexture(1, "wood.png");
	// FIXME: setInventoryTextureCube() only cares for the first texture
	f.setInventoryTextureCube("bookshelf.png", "bookshelf.png", "bookshelf.png");
	//f.setInventoryTextureCube("wood.png", "bookshelf.png", "bookshelf.png");
	f.param_type = CPT_MINERAL;
	f.is_ground_content = true;
	setWoodLikeMaterialProperties(f.material, 0.75);
	f.furnace_burntime = 30;
	nodemgr->set(i, f);

	i = CONTENT_GLASS;
	f = ContentFeatures();
	f.name = "glass";
	f.drawtype = NDT_GLASSLIKE;
	f.setAllTextures("glass.png");
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.param_type = CPT_LIGHT;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.setInventoryTextureCube("glass.png", "glass.png", "glass.png");
	setGlassLikeMaterialProperties(f.material, 1.0);
	nodemgr->set(i, f);

	i = CONTENT_FENCE;
	f = ContentFeatures();
	f.name = "wooden_fence";
	f.drawtype = NDT_FENCELIKE;
	f.setInventoryTexture("fence.png");
	f.setTexture(0, "wood.png");
	f.light_propagates = true;
	f.param_type = CPT_LIGHT;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.air_equivalent = true; // grass grows underneath
	f.selection_box.type = NODEBOX_FIXED;
	f.selection_box.fixed = core::aabbox3d<f32>(
			-BS/7, -BS/2, -BS/7, BS/7, BS/2, BS/7);
	f.furnace_burntime = 30/2;
	setWoodLikeMaterialProperties(f.material, 0.75);
	nodemgr->set(i, f);

	i = CONTENT_RAIL;
	f = ContentFeatures();
	f.name = "rail";
	f.drawtype = NDT_RAILLIKE;
	f.setInventoryTexture("rail.png");
	f.setTexture(0, "rail.png");
	f.setTexture(1, "rail_curved.png");
	f.setTexture(2, "rail_t_junction.png");
	f.setTexture(3, "rail_crossing.png");
	f.light_propagates = true;
	f.param_type = CPT_LIGHT;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.air_equivalent = true; // grass grows underneath
	f.walkable = false;
	f.selection_box.type = NODEBOX_FIXED;
	f.furnace_burntime = 5;
	setDirtLikeMaterialProperties(f.material, 0.75);
	nodemgr->set(i, f);

	i = CONTENT_LADDER;
	f = ContentFeatures();
	f.name = "ladder";
	f.drawtype = NDT_SIGNLIKE;
	f.setAllTextures("ladder.png");
	f.setInventoryTexture("ladder.png");
	f.light_propagates = true;
	f.param_type = CPT_LIGHT;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem ")+itos(i)+" 1";
	f.wall_mounted = true;
	f.air_equivalent = true;
	f.walkable = false;
	f.climbable = true;
	f.selection_box.type = NODEBOX_WALLMOUNTED;
	f.furnace_burntime = 5;
	setWoodLikeMaterialProperties(f.material, 0.5);

	nodemgr->set(i, f);

	i = CONTENT_COALSTONE;
	f = ContentFeatures();
	f.name = "coalstone";
	f.setAllTextures("stone.png^mineral_coal.png");
	f.is_ground_content = true;
	setStoneLikeMaterialProperties(f.material, 1.5);
	nodemgr->set(i, f);

	i = CONTENT_WOOD;
	f = ContentFeatures();
	f.name = "wood";
	f.setAllTextures("wood.png");
	f.setInventoryTextureCube("wood.png", "wood.png", "wood.png");
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.furnace_burntime = 30/4;
	setWoodLikeMaterialProperties(f.material, 0.75);
	nodemgr->set(i, f);

	i = CONTENT_MESE;
	f = ContentFeatures();
	f.name = "mese";
	f.setAllTextures("mese.png");
	f.setInventoryTextureCube("mese.png", "mese.png", "mese.png");
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.furnace_burntime = 30;
	setStoneLikeMaterialProperties(f.material, 0.5);
	nodemgr->set(i, f);

	i = CONTENT_CLOUD;
	f = ContentFeatures();
	f.name = "cloud";
	f.setAllTextures("cloud.png");
	f.setInventoryTextureCube("cloud.png", "cloud.png", "cloud.png");
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	nodemgr->set(i, f);

	i = CONTENT_AIR;
	f = ContentFeatures();
	f.name = "air";
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.walkable = false;
	f.pointable = false;
	f.diggable = false;
	f.buildable_to = true;
	f.air_equivalent = true;
	nodemgr->set(i, f);

	i = CONTENT_WATER;
	f = ContentFeatures();
	f.name = "water_flowing";
	f.drawtype = NDT_FLOWINGLIQUID;
	f.setAllTextures("water.png");
	f.alpha = WATER_ALPHA;
	f.setInventoryTextureCube("water.png", "water.png", "water.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.walkable = false;
	f.pointable = false;
	f.diggable = false;
	f.buildable_to = true;
	f.liquid_type = LIQUID_FLOWING;
	f.liquid_alternative_flowing = CONTENT_WATER;
	f.liquid_alternative_source = CONTENT_WATERSOURCE;
	f.liquid_viscosity = WATER_VISC;
	f.post_effect_color = video::SColor(64, 100, 100, 200);
	f.setSpecialMaterial(0, MaterialSpec("water.png", false));
	f.setSpecialMaterial(1, MaterialSpec("water.png", true));
	nodemgr->set(i, f);

	i = CONTENT_WATERSOURCE;
	f = ContentFeatures();
	f.name = "water_source";
	f.drawtype = NDT_LIQUID;
	f.setAllTextures("water.png");
	f.alpha = WATER_ALPHA;
	f.setInventoryTextureCube("water.png", "water.png", "water.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.walkable = false;
	f.pointable = false;
	f.diggable = false;
	f.buildable_to = true;
	f.liquid_type = LIQUID_SOURCE;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.liquid_alternative_flowing = CONTENT_WATER;
	f.liquid_alternative_source = CONTENT_WATERSOURCE;
	f.liquid_viscosity = WATER_VISC;
	f.post_effect_color = video::SColor(64, 100, 100, 200);
	// New-style water source material (mostly unused)
	f.setSpecialMaterial(0, MaterialSpec("water.png", false));
	nodemgr->set(i, f);

	i = CONTENT_LAVA;
	f = ContentFeatures();
	f.name = "lava_flowing";
	f.drawtype = NDT_FLOWINGLIQUID;
	f.setAllTextures("lava.png");
	f.setInventoryTextureCube("lava.png", "lava.png", "lava.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = false;
	f.light_source = LIGHT_MAX-1;
	f.walkable = false;
	f.pointable = false;
	f.diggable = false;
	f.buildable_to = true;
	f.liquid_type = LIQUID_FLOWING;
	f.liquid_alternative_flowing = CONTENT_LAVA;
	f.liquid_alternative_source = CONTENT_LAVASOURCE;
	f.liquid_viscosity = LAVA_VISC;
	f.damage_per_second = 4*2;
	f.post_effect_color = video::SColor(192, 255, 64, 0);
	f.setSpecialMaterial(0, MaterialSpec("lava.png", false));
	f.setSpecialMaterial(1, MaterialSpec("lava.png", true));
	nodemgr->set(i, f);

	i = CONTENT_LAVASOURCE;
	f = ContentFeatures();
	f.name = "lava_source";
	f.drawtype = NDT_LIQUID;
	f.setAllTextures("lava.png");
	f.setInventoryTextureCube("lava.png", "lava.png", "lava.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = false;
	f.light_source = LIGHT_MAX-1;
	f.walkable = false;
	f.pointable = false;
	f.diggable = false;
	f.buildable_to = true;
	f.liquid_type = LIQUID_SOURCE;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.liquid_alternative_flowing = CONTENT_LAVA;
	f.liquid_alternative_source = CONTENT_LAVASOURCE;
	f.liquid_viscosity = LAVA_VISC;
	f.damage_per_second = 4*2;
	f.post_effect_color = video::SColor(192, 255, 64, 0);
	// New-style lava source material (mostly unused)
	f.setSpecialMaterial(0, MaterialSpec("lava.png", false));
	f.furnace_burntime = 60;
	nodemgr->set(i, f);

	i = CONTENT_TORCH;
	f = ContentFeatures();
	f.name = "torch";
	f.drawtype = NDT_TORCHLIKE;
	f.setTexture(0, "torch_on_floor.png");
	f.setTexture(1, "torch_on_ceiling.png");
	f.setTexture(2, "torch.png");
	f.setInventoryTexture("torch_on_floor.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.walkable = false;
	f.wall_mounted = true;
	f.air_equivalent = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.light_source = LIGHT_MAX-1;
	f.selection_box.type = NODEBOX_WALLMOUNTED;
	f.selection_box.wall_top = core::aabbox3d<f32>(
			-BS/10, BS/2-BS/3.333*2, -BS/10, BS/10, BS/2, BS/10);
	f.selection_box.wall_bottom = core::aabbox3d<f32>(
			-BS/10, -BS/2, -BS/10, BS/10, -BS/2+BS/3.333*2, BS/10);
	f.selection_box.wall_side = core::aabbox3d<f32>(
			-BS/2, -BS/3.333, -BS/10, -BS/2+BS/3.333, BS/3.333, BS/10);
	setConstantMaterialProperties(f.material, 0.0);
	f.furnace_burntime = 4;
	nodemgr->set(i, f);

	i = CONTENT_SIGN_WALL;
	f = ContentFeatures();
	f.name = "sign_wall";
	f.drawtype = NDT_SIGNLIKE;
	f.setAllTextures("sign_wall.png");
	f.setInventoryTexture("sign_wall.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.walkable = false;
	f.wall_mounted = true;
	f.air_equivalent = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f.initial_metadata == NULL)
		f.initial_metadata = new SignNodeMetadata(NULL, "Some sign");
	setConstantMaterialProperties(f.material, 0.5);
	f.selection_box.type = NODEBOX_WALLMOUNTED;
	f.furnace_burntime = 10;
	nodemgr->set(i, f);

	i = CONTENT_CHEST;
	f = ContentFeatures();
	f.name = "chest";
	f.param_type = CPT_FACEDIR_SIMPLE;
	f.setAllTextures("chest_side.png");
	f.setTexture(0, "chest_top.png");
	f.setTexture(1, "chest_top.png");
	f.setTexture(5, "chest_front.png"); // Z-
	f.setInventoryTexture("chest_top.png");
	//f.setInventoryTextureCube("chest_top.png", "chest_side.png", "chest_side.png");
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f.initial_metadata == NULL)
		f.initial_metadata = new ChestNodeMetadata(NULL);
	setWoodLikeMaterialProperties(f.material, 1.0);
	f.furnace_burntime = 30;
	nodemgr->set(i, f);

	i = CONTENT_LOCKABLE_CHEST;
	f = ContentFeatures();
	f.name = "locked_chest";
	f.param_type = CPT_FACEDIR_SIMPLE;
	f.setAllTextures("chest_side.png");
	f.setTexture(0, "chest_top.png");
	f.setTexture(1, "chest_top.png");
	f.setTexture(5, "chest_lock.png"); // Z-
	f.setInventoryTexture("chest_lock.png");
	//f.setInventoryTextureCube("chest_top.png", "chest_side.png", "chest_side.png");
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f.initial_metadata == NULL)
		f.initial_metadata = new LockingChestNodeMetadata(NULL);
	setWoodLikeMaterialProperties(f.material, 1.0);
	f.furnace_burntime = 30;
	nodemgr->set(i, f);

	i = CONTENT_FURNACE;
	f = ContentFeatures();
	f.name = "furnace";
	f.param_type = CPT_FACEDIR_SIMPLE;
	f.setAllTextures("furnace_side.png");
	f.setTexture(5, "furnace_front.png"); // Z-
	f.setInventoryTexture("furnace_front.png");
	//f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.dug_item = std::string("MaterialItem2 ")+itos(CONTENT_COBBLE)+" 6";
	if(f.initial_metadata == NULL)
		f.initial_metadata = new FurnaceNodeMetadata(NULL);
	setStoneLikeMaterialProperties(f.material, 3.0);
	nodemgr->set(i, f);

	i = CONTENT_COBBLE;
	f = ContentFeatures();
	f.name = "cobble";
	f.setAllTextures("cobble.png");
	f.setInventoryTextureCube("cobble.png", "cobble.png", "cobble.png");
	f.param_type = CPT_NONE;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.cookresult_item = std::string("MaterialItem2 ")+itos(CONTENT_STONE)+" 1";
	setStoneLikeMaterialProperties(f.material, 0.9);
	nodemgr->set(i, f);

	i = CONTENT_MOSSYCOBBLE;
	f = ContentFeatures();
	f.name = "mossycobble";
	f.setAllTextures("mossycobble.png");
	f.setInventoryTextureCube("mossycobble.png", "mossycobble.png", "mossycobble.png");
	f.param_type = CPT_NONE;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f.material, 0.8);
	nodemgr->set(i, f);

	i = CONTENT_STEEL;
	f = ContentFeatures();
	f.name = "steelblock";
	f.setAllTextures("steel_block.png");
	f.setInventoryTextureCube("steel_block.png", "steel_block.png",
			"steel_block.png");
	f.param_type = CPT_NONE;
	f.is_ground_content = true;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f.material, 5.0);
	nodemgr->set(i, f);

	i = CONTENT_NC;
	f = ContentFeatures();
	f.name = "nyancat";
	f.param_type = CPT_FACEDIR_SIMPLE;
	f.setAllTextures("nc_side.png");
	f.setTexture(5, "nc_front.png"); // Z-
	f.setTexture(4, "nc_back.png"); // Z+
	f.setInventoryTexture("nc_front.png");
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f.material, 3.0);
	f.furnace_burntime = 1;
	nodemgr->set(i, f);

	i = CONTENT_NC_RB;
	f = ContentFeatures();
	f.name = "nyancat_rainbow";
	f.setAllTextures("nc_rb.png");
	f.setInventoryTexture("nc_rb.png");
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f.material, 3.0);
	f.furnace_burntime = 1;
	nodemgr->set(i, f);

	i = CONTENT_SAPLING;
	f = ContentFeatures();
	f.name = "sapling";
	f.drawtype = NDT_PLANTLIKE;
	f.visual_scale = 1.0;
	f.setAllTextures("sapling.png");
	f.setInventoryTexture("sapling.png");
	f.param_type = CPT_LIGHT;
	f.dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f.light_propagates = true;
	f.air_equivalent = false;
	f.walkable = false;
	setConstantMaterialProperties(f.material, 0.0);
	f.furnace_burntime = 10;
	nodemgr->set(i, f);

	i = CONTENT_APPLE;
	f = ContentFeatures();
	f.name = "apple";
	f.drawtype = NDT_PLANTLIKE;
	f.visual_scale = 1.0;
	f.setAllTextures("apple.png");
	f.setInventoryTexture("apple.png");
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.walkable = false;
	f.air_equivalent = true;
	f.dug_item = std::string("CraftItem apple 1");
	setConstantMaterialProperties(f.material, 0.0);
	f.furnace_burntime = 3;
	nodemgr->set(i, f);
}


