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

#ifndef NODEDEF_HEADER
#define NODEDEF_HEADER

#include "common_irrlicht.h"
#include <string>
#include <set>
#include "mapnode.h"
#ifndef SERVER
#include "tile.h"
#endif
#include "materials.h" // MaterialProperties
class ITextureSource;

/*
	TODO: Rename to nodedef.h
*/

#if 0

/*
	Content feature list
	
	Used for determining properties of MapNodes by content type without
	storing such properties in the nodes itself.
*/

/*
	Initialize content feature table.

	Must be called before accessing the table.
*/
void init_contentfeatures(ITextureSource *tsrc);

#endif

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

enum NodeBoxType
{
	NODEBOX_REGULAR, // Regular block; allows buildable_to
	NODEBOX_FIXED, // Static separately defined box
	NODEBOX_WALLMOUNTED, // Box for wall_mounted nodes; (top, bottom, side)
};

struct NodeBox
{
	enum NodeBoxType type;
	// NODEBOX_REGULAR (no parameters)
	// NODEBOX_FIXED
	core::aabbox3d<f32> fixed;
	// NODEBOX_WALLMOUNTED
	core::aabbox3d<f32> wall_top;
	core::aabbox3d<f32> wall_bottom;
	core::aabbox3d<f32> wall_side; // being at the -X side

	NodeBox():
		type(NODEBOX_REGULAR),
		// default is rail-like
		fixed(-BS/2, -BS/2, -BS/2, BS/2, -BS/2+BS/16., BS/2),
		// default is sign/ladder-like
		wall_top(-BS/2, BS/2-BS/16., -BS/2, BS/2, BS/2, BS/2),
		wall_bottom(-BS/2, -BS/2, -BS/2, BS/2, -BS/2+BS/16., BS/2),
		wall_side(-BS/2, -BS/2, -BS/2, -BS/2+BS/16., BS/2, BS/2)
	{}
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
	// Currently used for fetching liquid texture coordinates
	// - This is also updated to the above two (if they are non-NULL)
	//   when textures are updated
	AtlasPointer *special_atlas;
#endif

	// List of all block textures that have been used (value is dummy)
	// Used for texture atlas making.
	// Exists on server too for cleaner code in content_mapnode.cpp.
	std::set<std::string> used_texturenames;
	
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
	
	// Whether the node is non-liquid, source liquid or flowing liquid
	enum LiquidType liquid_type;
	// If the content is liquid, this is the flowing version of the liquid.
	content_t liquid_alternative_flowing;
	// If the content is liquid, this is the source version of the liquid.
	content_t liquid_alternative_source;
	// Viscosity for fluid flow, ranging from 1 to 7, with
	// 1 giving almost instantaneous propagation and 7 being
	// the slowest possible
	u8 liquid_viscosity;
	
	// Amount of light the node emits
	u8 light_source;
	
	u32 damage_per_second;

	NodeBox selection_box;

	MaterialProperties material;
	
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
		used_texturenames.clear();
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
		wall_mounted = false;
		air_equivalent = false;
		often_contains_mineral = false;
		dug_item = "";
		initial_metadata = NULL;
		liquid_type = LIQUID_NONE;
		liquid_alternative_flowing = CONTENT_IGNORE;
		liquid_alternative_source = CONTENT_IGNORE;
		liquid_viscosity = 0;
		light_source = 0;
		damage_per_second = 0;
		selection_box = NodeBox();
		material = MaterialProperties();
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
	void setTexture(ITextureSource *tsrc, u16 i, std::string name,
			u8 alpha=255)
	{}
	void setAllTextures(ITextureSource *tsrc, std::string name, u8 alpha=255)
	{}
#else
	void setTexture(ITextureSource *tsrc,
			u16 i, std::string name, u8 alpha=255);

	void setAllTextures(ITextureSource *tsrc,
			std::string name, u8 alpha=255)
	{
		for(u16 i=0; i<6; i++)
		{
			setTexture(tsrc, i, name, alpha);
		}
		// Force inventory texture too
		setInventoryTexture(name, tsrc);
	}
#endif

#ifndef SERVER
	void setTile(u16 i, const TileSpec &tile)
	{ tiles[i] = tile; }
	void setAllTiles(const TileSpec &tile)
	{ for(u16 i=0; i<6; i++) setTile(i, tile); }
#endif

#ifdef SERVER
	void setInventoryTexture(std::string imgname,
			ITextureSource *tsrc)
	{}
	void setInventoryTextureCube(std::string top,
			std::string left, std::string right, ITextureSource *tsrc)
	{}
#else
	void setInventoryTexture(std::string imgname, ITextureSource *tsrc);
	
	void setInventoryTextureCube(std::string top,
			std::string left, std::string right, ITextureSource *tsrc);
#endif

	/*
		Some handy methods
	*/
	bool isLiquid() const{
		return (liquid_type != LIQUID_NONE);
	}
	bool sameLiquid(const ContentFeatures &f) const{
		if(!isLiquid() || !f.isLiquid()) return false;
		return (liquid_alternative_flowing == f.liquid_alternative_flowing);
	}
};

class INodeDefManager
{
public:
	INodeDefManager(){}
	virtual ~INodeDefManager(){}
	// Get node definition
	virtual const ContentFeatures& get(content_t c) const=0;
	virtual const ContentFeatures& get(const MapNode &n) const=0;
};

class IWritableNodeDefManager : public INodeDefManager
{
public:
	IWritableNodeDefManager(){}
	virtual ~IWritableNodeDefManager(){}
	virtual IWritableNodeDefManager* clone()=0;
	// Get node definition
	virtual const ContentFeatures& get(content_t c) const=0;
	virtual const ContentFeatures& get(const MapNode &n) const=0;
		
	// Register node definition
	virtual void set(content_t c, const ContentFeatures &def)=0;
	virtual ContentFeatures* getModifiable(content_t c)=0;

	/*
		Update tile textures to latest return values of TextueSource.
		Call after updating the texture atlas of a TextureSource.
	*/
	virtual void updateTextures(ITextureSource *tsrc)=0;
};

// If textures not actually available (server), tsrc can be NULL
IWritableNodeDefManager* createNodeDefManager(ITextureSource *tsrc);


#endif

