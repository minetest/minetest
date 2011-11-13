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

#ifndef MAPNODE_CONTENTFEATURES_HEADER
#define MAPNODE_CONTENTFEATURES_HEADER

#include "common_irrlicht.h"
#include <string>
#include "mapnode.h"
#ifndef SERVER
#include "tile.h"
#endif

/*
	Content feature list
	
	Used for determining properties of MapNodes by content type without
	storing such properties in the nodes itself.
*/

/*
	Initialize content feature table.

	Must be called before accessing the table.
*/
void init_contentfeatures();

enum ContentParamType
{
	CPT_NONE,
	CPT_LIGHT,
	CPT_MINERAL,
	// Direction for chests and furnaces and such
	CPT_FACEDIR_SIMPLE
};

enum LiquidType
{
	LIQUID_NONE,
	LIQUID_FLOWING,
	LIQUID_SOURCE
};

struct MapNode;
class NodeMetadata;

struct ContentFeatures
{
#ifndef SERVER
	/*
		0: up
		1: down
		2: right
		3: left
		4: back
		5: front
	*/
	TileSpec tiles[6];
	
	video::ITexture *inventory_texture;

	// Used currently for flowing liquids
	u8 vertex_alpha;
	// Post effect color, drawn when the camera is inside the node.
	video::SColor post_effect_color;
	// Special irrlicht material, used sometimes
	video::SMaterial *special_material;
	video::SMaterial *special_material2;
	AtlasPointer *special_atlas;
#endif

	// List of all block textures that have been used (value is dummy)
	// Exists on server too for cleaner code in content_mapnode.cpp
	core::map<std::string, bool> used_texturenames;
	
	// Type of MapNode::param1
	ContentParamType param_type;
	// True for all ground-like things like stone and mud, false for eg. trees
	bool is_ground_content;
	bool light_propagates;
	bool sunlight_propagates;
	u8 solidness; // Used when choosing which face is drawn
	u8 visual_solidness; // When solidness=0, this tells how it looks like
	// This is used for collision detection.
	// Also for general solidness queries.
	bool walkable;
	// Player can point to these
	bool pointable;
	// Player can dig these
	bool diggable;
	// Player can climb these
	bool climbable;
	// Player can build on these
	bool buildable_to;
	// Whether the node has no liquid, source liquid or flowing liquid
	enum LiquidType liquid_type;
	// If true, param2 is set to direction when placed. Used for torches.
	// NOTE: the direction format is quite inefficient and should be changed
	bool wall_mounted;
	// If true, node is equivalent to air. Torches are, air is. Water is not.
	// Is used for example to check whether a mud block can have grass on.
	bool air_equivalent;
	// Whether this content type often contains mineral.
	// Used for texture atlas creation.
	// Currently only enabled for CONTENT_STONE.
	bool often_contains_mineral;
	
	// Inventory item string as which the node appears in inventory when dug.
	// Mineral overrides this.
	std::string dug_item;

	// Extra dug item and its rarity
	std::string extra_dug_item;
	s32 extra_dug_item_rarity;

	// Initial metadata is cloned from this
	NodeMetadata *initial_metadata;
	
	// If the content is liquid, this is the flowing version of the liquid.
	// If content is liquid, this is the same content.
	content_t liquid_alternative_flowing;
	// If the content is liquid, this is the source version of the liquid.
	content_t liquid_alternative_source;
	// Viscosity for fluid flow, ranging from 1 to 7, with
	// 1 giving almost instantaneous propagation and 7 being
	// the slowest possible
	u8 liquid_viscosity;
	
	// Amount of light the node emits
	u8 light_source;
	
	// Digging properties for different tools
	DiggingPropertiesList digging_properties;

	u32 damage_per_second;
	
	// NOTE: Move relevant properties to here from elsewhere

	void reset()
	{
#ifndef SERVER
		inventory_texture = NULL;
		
		vertex_alpha = 255;
		post_effect_color = video::SColor(0, 0, 0, 0);
		special_material = NULL;
		special_material2 = NULL;
		special_atlas = NULL;
#endif
		param_type = CPT_NONE;
		is_ground_content = false;
		light_propagates = false;
		sunlight_propagates = false;
		solidness = 2;
		visual_solidness = 0;
		walkable = true;
		pointable = true;
		diggable = true;
		climbable = false;
		buildable_to = false;
		liquid_type = LIQUID_NONE;
		wall_mounted = false;
		air_equivalent = false;
		often_contains_mineral = false;
		dug_item = "";
		initial_metadata = NULL;
		liquid_alternative_flowing = CONTENT_IGNORE;
		liquid_alternative_source = CONTENT_IGNORE;
		liquid_viscosity = 0;
		light_source = 0;
		digging_properties.clear();
		damage_per_second = 0;
	}

	ContentFeatures()
	{
		reset();
	}

	~ContentFeatures();
	
	/*
		Quickhands for simple materials
	*/
	
#ifdef SERVER
	void setTexture(u16 i, std::string name, u8 alpha=255)
	{}
	void setAllTextures(std::string name, u8 alpha=255)
	{}
#else
	void setTexture(u16 i, std::string name, u8 alpha=255);

	void setAllTextures(std::string name, u8 alpha=255)
	{
		for(u16 i=0; i<6; i++)
		{
			setTexture(i, name, alpha);
		}
		// Force inventory texture too
		setInventoryTexture(name);
	}
#endif

#ifndef SERVER
	void setTile(u16 i, const TileSpec &tile)
	{
		tiles[i] = tile;
	}
	void setAllTiles(const TileSpec &tile)
	{
		for(u16 i=0; i<6; i++)
		{
			setTile(i, tile);
		}
	}
#endif

#ifdef SERVER
	void setInventoryTexture(std::string imgname)
	{}
	void setInventoryTextureCube(std::string top,
			std::string left, std::string right)
	{}
#else
	void setInventoryTexture(std::string imgname);
	
	void setInventoryTextureCube(std::string top,
			std::string left, std::string right);
#endif
};

/*
	Call this to access the ContentFeature list
*/
ContentFeatures & content_features(content_t i);
ContentFeatures & content_features(MapNode &n);

/*
	Here is a bunch of DEPRECATED functions.
*/

/*
	If true, the material allows light propagation and brightness is stored
	in param.
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline bool light_propagates_content(content_t m)
{
	return content_features(m).light_propagates;
}
/*
	If true, the material allows lossless sunlight propagation.
	NOTE: It doesn't seem to go through torches regardlessly of this
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline bool sunlight_propagates_content(content_t m)
{
	return content_features(m).sunlight_propagates;
}
/*
	On a node-node surface, the material of the node with higher solidness
	is used for drawing.
	0: Invisible
	1: Transparent
	2: Opaque
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline u8 content_solidness(content_t m)
{
	return content_features(m).solidness;
}
// Objects collide with walkable contents
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_walkable(content_t m)
{
	return content_features(m).walkable;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_liquid(content_t m)
{
	return content_features(m).liquid_type != LIQUID_NONE;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_flowing_liquid(content_t m)
{
	return content_features(m).liquid_type == LIQUID_FLOWING;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_liquid_source(content_t m)
{
	return content_features(m).liquid_type == LIQUID_SOURCE;
}
// CONTENT_WATER || CONTENT_WATERSOURCE -> CONTENT_WATER
// CONTENT_LAVA || CONTENT_LAVASOURCE -> CONTENT_LAVA
// NOTE: Don't use, use "content_features(m).whatever" instead
inline content_t make_liquid_flowing(content_t m)
{
	u8 c = content_features(m).liquid_alternative_flowing;
	assert(c != CONTENT_IGNORE);
	return c;
}
// Pointable contents can be pointed to in the map
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_pointable(content_t m)
{
	return content_features(m).pointable;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_diggable(content_t m)
{
	return content_features(m).diggable;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_buildable_to(content_t m)
{
	return content_features(m).buildable_to;
}


#endif

