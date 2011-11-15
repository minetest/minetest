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
#include "settings.h"
#include "nodedef.h"

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

// See header for description
void content_mapnode_init(ITextureSource *tsrc, IWritableNodeDefManager *nodemgr)
{
	if(tsrc == NULL)
		dstream<<"INFO: Initial run of content_mapnode_init with "
				"tsrc=NULL. If this segfaults, "
				"there is a bug with something not checking for "
				"the NULL value."<<std::endl;
	else
		dstream<<"INFO: Full run of content_mapnode_init with "
				"tsrc!=NULL"<<std::endl;

	// Read some settings
	bool new_style_water = g_settings->getBool("new_style_water");
	bool new_style_leaves = g_settings->getBool("new_style_leaves");
	bool invisible_stone = g_settings->getBool("invisible_stone");
	bool opaque_water = g_settings->getBool("opaque_water");

	content_t i;
	ContentFeatures *f = NULL;

	i = CONTENT_STONE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("stone.png");
	f->setInventoryTextureCube("stone.png", "stone.png", "stone.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->often_contains_mineral = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_COBBLE)+" 1";
	setStoneLikeMaterialProperties(f->material, 1.0);
	if(invisible_stone)
		f->solidness = 0; // For debugging, hides regular stone
	
	i = CONTENT_GRASS;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("mud.png^grass_side.png");
	f->setTexture(0, "grass.png");
	f->setTexture(1, "mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_MUD)+" 1";
	setDirtLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_GRASS_FOOTSTEPS;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("mud.png^grass_side.png");
	f->setTexture(0, "grass_footsteps.png");
	f->setTexture(1, "mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_MUD)+" 1";
	setDirtLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_MUD;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("mud.png");
	f->setInventoryTextureCube("mud.png", "mud.png", "mud.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setDirtLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_SAND;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("sand.png");
	f->setInventoryTextureCube("sand.png", "sand.png", "sand.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setDirtLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_GRAVEL;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("gravel.png");
	f->setInventoryTextureCube("gravel.png", "gravel.png", "gravel.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setGravelLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_SANDSTONE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("sandstone.png");
	f->setInventoryTextureCube("sandstone.png", "sandstone.png", "sandstone.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_SAND)+" 1";
	setDirtLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_CLAY;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("clay.png");
	f->setInventoryTextureCube("clay.png", "clay.png", "clay.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("CraftItem lump_of_clay 4");
	setDirtLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_BRICK;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("brick.png");
	f->setInventoryTextureCube("brick.png", "brick.png", "brick.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("CraftItem clay_brick 4");
	setStoneLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_TREE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("tree.png");
	f->setTexture(0, "tree_top.png");
	f->setTexture(1, "tree_top.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_JUNGLETREE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("jungletree.png");
	f->setTexture(0, "jungletree_top.png");
	f->setTexture(1, "jungletree_top.png");
	f->param_type = CPT_MINERAL;
	//f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_JUNGLEGRASS;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("junglegrass.png");
	f->used_texturenames.insert("junglegrass.png"); // Add to atlas
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	//f->is_ground_content = true;
	f->air_equivalent = false; // grass grows underneath
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	setLeavesLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_LEAVES;
	f = nodemgr->getModifiable(i);
	f->light_propagates = true;
	//f->param_type = CPT_MINERAL;
	f->param_type = CPT_LIGHT;
	//f->is_ground_content = true;
	if(new_style_leaves)
	{
		f->solidness = 0; // drawn separately, makes no faces
		f->visual_solidness = 1;
		f->setAllTextures("leaves.png");
		f->setInventoryTextureCube("leaves.png", "leaves.png", "leaves.png");
	}
	else
	{
		f->setAllTextures("[noalpha:leaves.png");
	}
	f->extra_dug_item = std::string("MaterialItem2 ")+itos(CONTENT_SAPLING)+" 1";
	f->extra_dug_item_rarity = 20;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setLeavesLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_CACTUS;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("cactus_side.png");
	f->setTexture(0, "cactus_top.png");
	f->setTexture(1, "cactus_top.png");
	f->setInventoryTextureCube("cactus_top.png", "cactus_side.png", "cactus_side.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeMaterialProperties(f->material, 0.75);

	i = CONTENT_PAPYRUS;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("papyrus.png");
	f->used_texturenames.insert("papyrus.png"); // Add to atlas
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	setLeavesLikeMaterialProperties(f->material, 0.5);

	i = CONTENT_BOOKSHELF;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("bookshelf.png");
	f->setTexture(0, "wood.png");
	f->setTexture(1, "wood.png");
	// FIXME: setInventoryTextureCube() only cares for the first texture
	f->setInventoryTextureCube("bookshelf.png", "bookshelf.png", "bookshelf.png");
	//f->setInventoryTextureCube("wood.png", "bookshelf.png", "bookshelf.png");
	f->param_type = CPT_MINERAL;
	f->is_ground_content = true;
	setWoodLikeMaterialProperties(f->material, 0.75);

	i = CONTENT_GLASS;
	f = nodemgr->getModifiable(i);
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->visual_solidness = 1;
	f->setAllTextures("glass.png");
	f->setInventoryTextureCube("glass.png", "glass.png", "glass.png");
	setGlassLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_FENCE;
	f = nodemgr->getModifiable(i);
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->air_equivalent = true; // grass grows underneath
	f->setInventoryTexture("fence.png");
	f->used_texturenames.insert("fence.png"); // Add to atlas
	f->selection_box.type = NODEBOX_FIXED;
	f->selection_box.fixed = core::aabbox3d<f32>(
			-BS/7, -BS/2, -BS/7, BS/7, BS/2, BS/7);
	setWoodLikeMaterialProperties(f->material, 0.75);

	i = CONTENT_RAIL;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("rail.png");
	f->used_texturenames.insert("rail.png"); // Add to atlas
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->solidness = 0; // drawn separately, makes no faces
	f->air_equivalent = true; // grass grows underneath
	f->walkable = false;
	f->selection_box.type = NODEBOX_FIXED;
	setDirtLikeMaterialProperties(f->material, 0.75);

	i = CONTENT_LADDER;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("ladder.png");
	f->used_texturenames.insert("ladder.png"); // Add to atlas
	f->light_propagates = true;
	f->param_type = CPT_LIGHT;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem ")+itos(i)+" 1";
	f->wall_mounted = true;
	f->solidness = 0;
	f->air_equivalent = true;
	f->walkable = false;
	f->climbable = true;
	f->selection_box.type = NODEBOX_WALLMOUNTED;
	setWoodLikeMaterialProperties(f->material, 0.5);

	// Deprecated
	i = CONTENT_COALSTONE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("stone.png^mineral_coal.png");
	f->is_ground_content = true;
	setStoneLikeMaterialProperties(f->material, 1.5);
	
	i = CONTENT_WOOD;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("wood.png");
	f->setInventoryTextureCube("wood.png", "wood.png", "wood.png");
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setWoodLikeMaterialProperties(f->material, 0.75);
	
	i = CONTENT_MESE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("mese.png");
	f->setInventoryTextureCube("mese.png", "mese.png", "mese.png");
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f->material, 0.5);
	
	i = CONTENT_CLOUD;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("cloud.png");
	f->setInventoryTextureCube("cloud.png", "cloud.png", "cloud.png");
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	
	i = CONTENT_AIR;
	f = nodemgr->getModifiable(i);
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
	f = nodemgr->getModifiable(i);
	f->setInventoryTextureCube("water.png", "water.png", "water.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->solidness = 0; // Drawn separately, makes no faces
	f->visual_solidness = 1;
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_FLOWING;
	f->liquid_alternative_flowing = CONTENT_WATER;
	f->liquid_alternative_source = CONTENT_WATERSOURCE;
	f->liquid_viscosity = WATER_VISC;
	if(!opaque_water)
		f->alpha = WATER_ALPHA;
	f->post_effect_color = video::SColor(64, 100, 100, 200);
	// Flowing water material
	f->mspec_special[0].tname = "water.png";
	f->mspec_special[0].backface_culling = false;
	f->mspec_special[1].tname = "water.png";
	f->mspec_special[1].backface_culling = true;

/*#ifndef SERVER
	if(f->special_material == NULL && tsrc)
	{
		f->special_material = new video::SMaterial;
		f->special_material->setFlag(video::EMF_LIGHTING, false);
		f->special_material->setFlag(video::EMF_BACK_FACE_CULLING, false);
		f->special_material->setFlag(video::EMF_BILINEAR_FILTER, false);
		f->special_material->setFlag(video::EMF_FOG_ENABLE, true);
		if(!opaque_water)
			f->special_material->MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
		AtlasPointer *pa_water1 = new AtlasPointer(tsrc->getTexture(
				tsrc->getTextureId("water.png")));
		f->special_material->setTexture(0, pa_water1->atlas);

		// Flowing water material, backface culled
		f->special_material2 = new video::SMaterial;
		*f->special_material2 = *f->special_material;
		f->special_material2->setFlag(video::EMF_BACK_FACE_CULLING, true);
		
		f->special_atlas = pa_water1;
	}
#endif*/
	
	i = CONTENT_WATERSOURCE;
	f = nodemgr->getModifiable(i);
	if(new_style_water)
	{
		f->solidness = 0; // drawn separately, makes no faces
	}
	else // old style
	{
		f->solidness = 1;
		f->setAllTextures("water.png", WATER_ALPHA);
		f->backface_culling = false;
	}
	//f->setInventoryTexture("water.png");
	f->setInventoryTextureCube("water.png", "water.png", "water.png");
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
	f->liquid_viscosity = WATER_VISC;
	if(!opaque_water)
		f->alpha = WATER_ALPHA;
	f->post_effect_color = video::SColor(64, 100, 100, 200);
	// New-style water source material (mostly unused)
	f->mspec_special[0].tname = "water.png";
	f->mspec_special[0].backface_culling = false;
/*#ifndef SERVER
	if(f->special_material == NULL && tsrc)
	{
		f->special_material = new video::SMaterial;
		f->special_material->setFlag(video::EMF_LIGHTING, false);
		f->special_material->setFlag(video::EMF_BACK_FACE_CULLING, false);
		f->special_material->setFlag(video::EMF_BILINEAR_FILTER, false);
		f->special_material->setFlag(video::EMF_FOG_ENABLE, true);
		f->special_material->MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
		AtlasPointer *pa_water1 = new AtlasPointer(tsrc->getTexture(
				tsrc->getTextureId("water.png")));
		f->special_material->setTexture(0, pa_water1->atlas);
		f->special_atlas = pa_water1;
	}
#endif*/
	
	i = CONTENT_LAVA;
	f = nodemgr->getModifiable(i);
	f->setInventoryTextureCube("lava.png", "lava.png", "lava.png");
	f->used_texturenames.insert("lava.png"); // Add to atlas
	f->param_type = CPT_LIGHT;
	f->light_propagates = false;
	f->light_source = LIGHT_MAX-1;
	f->solidness = 0; // Drawn separately, makes no faces
	f->visual_solidness = 1; // Does not completely cover block boundaries
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_FLOWING;
	f->liquid_alternative_flowing = CONTENT_LAVA;
	f->liquid_alternative_source = CONTENT_LAVASOURCE;
	f->liquid_viscosity = LAVA_VISC;
	f->damage_per_second = 4*2;
	f->post_effect_color = video::SColor(192, 255, 64, 0);
	// Flowing lava material
	f->mspec_special[0].tname = "lava.png";
	f->mspec_special[0].backface_culling = false;
	f->mspec_special[1].tname = "lava.png";
	f->mspec_special[1].backface_culling = true;
/*#ifndef SERVER
	if(f->special_material == NULL && tsrc)
	{
	}
#endif*/
	
	i = CONTENT_LAVASOURCE;
	f = nodemgr->getModifiable(i);
	f->used_texturenames.insert("ladder.png"); // Add to atlas
	if(new_style_water)
	{
		f->solidness = 0; // drawn separately, makes no faces
	}
	else // old style
	{
		f->solidness = 2;
		f->setAllTextures("lava.png");
	}
	f->setInventoryTextureCube("lava.png", "lava.png", "lava.png");
	f->param_type = CPT_LIGHT;
	f->light_propagates = false;
	f->light_source = LIGHT_MAX-1;
	f->walkable = false;
	f->pointable = false;
	f->diggable = false;
	f->buildable_to = true;
	f->liquid_type = LIQUID_SOURCE;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->liquid_alternative_flowing = CONTENT_LAVA;
	f->liquid_alternative_source = CONTENT_LAVASOURCE;
	f->liquid_viscosity = LAVA_VISC;
	f->damage_per_second = 4*2;
	f->post_effect_color = video::SColor(192, 255, 64, 0);
	// New-style lava source material (mostly unused)
	f->mspec_special[0].tname = "lava.png";
	f->mspec_special[0].backface_culling = false;
/*#ifndef SERVER
	if(f->special_material == NULL && tsrc)
	{
	}
#endif*/
	
	i = CONTENT_TORCH;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("torch_on_floor.png");
	f->used_texturenames.insert("torch_on_floor.png"); // Add to atlas
	f->used_texturenames.insert("torch_on_ceiling.png"); // Add to atlas
	f->used_texturenames.insert("torch.png"); // Add to atlas
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	f->wall_mounted = true;
	f->air_equivalent = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->light_source = LIGHT_MAX-1;
	f->selection_box.type = NODEBOX_WALLMOUNTED;
	f->selection_box.wall_top = core::aabbox3d<f32>(
			-BS/10, BS/2-BS/3.333*2, -BS/10, BS/10, BS/2, BS/10);
	f->selection_box.wall_bottom = core::aabbox3d<f32>(
			-BS/10, -BS/2, -BS/10, BS/10, -BS/2+BS/3.333*2, BS/10);
	f->selection_box.wall_side = core::aabbox3d<f32>(
			-BS/2, -BS/3.333, -BS/10, -BS/2+BS/3.333, BS/3.333, BS/10);
	setConstantMaterialProperties(f->material, 0.0);
	
	i = CONTENT_SIGN_WALL;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("sign_wall.png");
	f->used_texturenames.insert("sign_wall.png"); // Add to atlas
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	f->wall_mounted = true;
	f->air_equivalent = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new SignNodeMetadata(NULL, "Some sign");
	setConstantMaterialProperties(f->material, 0.5);
	f->selection_box.type = NODEBOX_WALLMOUNTED;
	
	i = CONTENT_CHEST;
	f = nodemgr->getModifiable(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("chest_side.png");
	f->setTexture(0, "chest_top.png");
	f->setTexture(1, "chest_top.png");
	f->setTexture(5, "chest_front.png"); // Z-
	f->setInventoryTexture("chest_top.png");
	//f->setInventoryTextureCube("chest_top.png", "chest_side.png", "chest_side.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new ChestNodeMetadata(NULL);
	setWoodLikeMaterialProperties(f->material, 1.0);
	
	i = CONTENT_LOCKABLE_CHEST;
	f = nodemgr->getModifiable(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("chest_side.png");
	f->setTexture(0, "chest_top.png");
	f->setTexture(1, "chest_top.png");
	f->setTexture(5, "chest_lock.png"); // Z-
	f->setInventoryTexture("chest_lock.png");
	//f->setInventoryTextureCube("chest_top.png", "chest_side.png", "chest_side.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new LockingChestNodeMetadata(NULL);
	setWoodLikeMaterialProperties(f->material, 1.0);

	i = CONTENT_FURNACE;
	f = nodemgr->getModifiable(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("furnace_side.png");
	f->setTexture(5, "furnace_front.png"); // Z-
	f->setInventoryTexture("furnace_front.png");
	//f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->dug_item = std::string("MaterialItem2 ")+itos(CONTENT_COBBLE)+" 6";
	if(f->initial_metadata == NULL)
		f->initial_metadata = new FurnaceNodeMetadata(NULL);
	setStoneLikeMaterialProperties(f->material, 3.0);
	
	i = CONTENT_COBBLE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("cobble.png");
	f->setInventoryTextureCube("cobble.png", "cobble.png", "cobble.png");
	f->param_type = CPT_NONE;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f->material, 0.9);

	i = CONTENT_MOSSYCOBBLE;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("mossycobble.png");
	f->setInventoryTextureCube("mossycobble.png", "mossycobble.png", "mossycobble.png");
	f->param_type = CPT_NONE;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f->material, 0.8);
	
	i = CONTENT_STEEL;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("steel_block.png");
	f->setInventoryTextureCube("steel_block.png", "steel_block.png",
			"steel_block.png");
	f->param_type = CPT_NONE;
	f->is_ground_content = true;
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f->material, 5.0);
	
	i = CONTENT_NC;
	f = nodemgr->getModifiable(i);
	f->param_type = CPT_FACEDIR_SIMPLE;
	f->setAllTextures("nc_side.png");
	f->setTexture(5, "nc_front.png"); // Z-
	f->setTexture(4, "nc_back.png"); // Z+
	f->setInventoryTexture("nc_front.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f->material, 3.0);
	
	i = CONTENT_NC_RB;
	f = nodemgr->getModifiable(i);
	f->setAllTextures("nc_rb.png");
	f->setInventoryTexture("nc_rb.png");
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	setStoneLikeMaterialProperties(f->material, 3.0);

	i = CONTENT_SAPLING;
	f = nodemgr->getModifiable(i);
	f->param_type = CPT_LIGHT;
	f->setAllTextures("sapling.png");
	f->setInventoryTexture("sapling.png");
	f->used_texturenames.insert("sapling.png"); // Add to atlas
	f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	f->light_propagates = true;
	f->air_equivalent = false;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	setConstantMaterialProperties(f->material, 0.0);
	
	i = CONTENT_APPLE;
	f = nodemgr->getModifiable(i);
	f->setInventoryTexture("apple.png");
	f->used_texturenames.insert("apple.png"); // Add to atlas
	f->param_type = CPT_LIGHT;
	f->light_propagates = true;
	f->sunlight_propagates = true;
	f->solidness = 0; // drawn separately, makes no faces
	f->walkable = false;
	f->air_equivalent = true;
	f->dug_item = std::string("CraftItem apple 1");
	setConstantMaterialProperties(f->material, 0.0);
}


