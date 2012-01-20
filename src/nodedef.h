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
#include <iostream>
#include <set>
#include "mapnode.h"
#ifndef SERVER
#include "tile.h"
#endif
#include "materials.h" // MaterialProperties
class IItemDefManager;
class ITextureSource;
class IGameDef;

enum ContentParamType
{
	CPT_NONE,
	CPT_LIGHT,
};

enum ContentParamType2
{
	CPT2_NONE,
	// Need 8-bit param2
	CPT2_FULL,
	// Flowing liquid properties
	CPT2_FLOWINGLIQUID,
	// Direction for chests and furnaces and such
	CPT2_FACEDIR,
	// Direction for signs, torches and such
	CPT2_WALLMOUNTED,
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
	NODEBOX_WALLMOUNTED, // Box for wall mounted nodes; (top, bottom, side)
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

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
};

struct MapNode;
class NodeMetadata;

struct MaterialSpec
{
	std::string tname;
	bool backface_culling;
	
	MaterialSpec(const std::string &tname_="", bool backface_culling_=true):
		tname(tname_),
		backface_culling(backface_culling_)
	{}

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
};

enum NodeDrawType
{
	NDT_NORMAL, // A basic solid block
	NDT_AIRLIKE, // Nothing is drawn
	NDT_LIQUID, // Do not draw face towards same kind of flowing/source liquid
	NDT_FLOWINGLIQUID, // A very special kind of thing
	NDT_GLASSLIKE, // Glass-like, don't draw faces towards other glass
	NDT_ALLFACES, // Leaves-like, draw all faces no matter what
	NDT_ALLFACES_OPTIONAL, // Fancy -> allfaces, fast -> normal
	NDT_TORCHLIKE,
	NDT_SIGNLIKE,
	NDT_PLANTLIKE,
	NDT_FENCELIKE,
	NDT_RAILLIKE,
};

#define CF_SPECIAL_COUNT 2

struct ContentFeatures
{
	/*
		Cached stuff
	*/
#ifndef SERVER
	// 0     1     2     3     4     5
	// up    down  right left  back  front 
	TileSpec tiles[6];
	// Special material/texture
	// - Currently used for flowing liquids
	video::SMaterial *special_materials[CF_SPECIAL_COUNT];
	AtlasPointer *special_aps[CF_SPECIAL_COUNT];
	u8 solidness; // Used when choosing which face is drawn
	u8 visual_solidness; // When solidness=0, this tells how it looks like
	bool backface_culling;
#endif

	/*
		Actual data
	*/

	std::string name; // "" = undefined node

	// Visual definition
	enum NodeDrawType drawtype;
	float visual_scale; // Misc. scale parameter
	std::string tname_tiles[6];
	MaterialSpec mspec_special[CF_SPECIAL_COUNT]; // Use setter methods
	u8 alpha;

	// Post effect color, drawn when the camera is inside the node.
	video::SColor post_effect_color;
	// Type of MapNode::param1
	ContentParamType param_type;
	// Type of MapNode::param2
	ContentParamType2 param_type_2;
	// True for all ground-like things like stone and mud, false for eg. trees
	bool is_ground_content;
	bool light_propagates;
	bool sunlight_propagates;
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
	// Metadata name of node (eg. "furnace")
	std::string metadata_name;
	// Whether the node is non-liquid, source liquid or flowing liquid
	enum LiquidType liquid_type;
	// If the content is liquid, this is the flowing version of the liquid.
	std::string liquid_alternative_flowing;
	// If the content is liquid, this is the source version of the liquid.
	std::string liquid_alternative_source;
	// Viscosity for fluid flow, ranging from 1 to 7, with
	// 1 giving almost instantaneous propagation and 7 being
	// the slowest possible
	u8 liquid_viscosity;
	// Amount of light the node emits
	u8 light_source;
	u32 damage_per_second;
	NodeBox selection_box;
	MaterialProperties material;
	// Compatibility with old maps
	// Set to true if paramtype used to be 'facedir_simple'
	bool legacy_facedir_simple;
	// Set to true if wall_mounted used to be set to true
	bool legacy_wallmounted;

	/*
		Methods
	*/
	
	ContentFeatures();
	~ContentFeatures();
	void reset();
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);

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
	virtual bool getId(const std::string &name, content_t &result) const=0;
	virtual content_t getId(const std::string &name) const=0;
	virtual const ContentFeatures& get(const std::string &name) const=0;
	
	virtual void serialize(std::ostream &os)=0;
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
	virtual bool getId(const std::string &name, content_t &result) const=0;
	virtual content_t getId(const std::string &name) const=0;
	// If not found, returns the features of CONTENT_IGNORE
	virtual const ContentFeatures& get(const std::string &name) const=0;

	// Register node definition
	virtual void set(content_t c, const ContentFeatures &def)=0;
	// Register node definition by name (allocate an id)
	// If returns CONTENT_IGNORE, could not allocate id
	virtual content_t set(const std::string &name,
			const ContentFeatures &def)=0;
	// If returns CONTENT_IGNORE, could not allocate id
	virtual content_t allocateDummy(const std::string &name)=0;

	/*
		Update item alias mapping.
		Call after updating item definitions.
	*/
	virtual void updateAliases(IItemDefManager *idef)=0;

	/*
		Update tile textures to latest return values of TextueSource.
		Call after updating the texture atlas of a TextureSource.
	*/
	virtual void updateTextures(ITextureSource *tsrc)=0;

	virtual void serialize(std::ostream &os)=0;
	virtual void deSerialize(std::istream &is)=0;
};

IWritableNodeDefManager* createNodeDefManager();

#endif

