/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef NODEDEF_HEADER
#define NODEDEF_HEADER

#include "irrlichttypes_bloated.h"
#include <string>
#include <iostream>
#include <map>
#include <list>
#include "mapnode.h"
#ifndef SERVER
#include "tile.h"
#include "shader.h"
#endif
#include "itemgroup.h"
#include "sound.h" // SimpleSoundSpec
#include "constants.h" // BS

class IItemDefManager;
class ITextureSource;
class IShaderSource;
class IGameDef;

typedef std::list<std::pair<content_t, int> > GroupItems;

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
	// Block level like FLOWINGLIQUID
	CPT2_LEVELED,
};

enum LiquidType
{
	LIQUID_NONE,
	LIQUID_FLOWING,
	LIQUID_SOURCE,
};

enum NodeBoxType
{
	NODEBOX_REGULAR, // Regular block; allows buildable_to
	NODEBOX_FIXED, // Static separately defined box(es)
	NODEBOX_WALLMOUNTED, // Box for wall mounted nodes; (top, bottom, side)
	NODEBOX_LEVELED, // Same as fixed, but with dynamic height from param2. for snow, ...
};

struct NodeBox
{
	enum NodeBoxType type;
	// NODEBOX_REGULAR (no parameters)
	// NODEBOX_FIXED
	std::vector<aabb3f> fixed;
	// NODEBOX_WALLMOUNTED
	aabb3f wall_top;
	aabb3f wall_bottom;
	aabb3f wall_side; // being at the -X side

	NodeBox()
	{ reset(); }

	void reset();
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is);
};

struct MapNode;
class NodeMetadata;

/*
	Stand-alone definition of a TileSpec (basically a server-side TileSpec)
*/
enum TileAnimationType{
	TAT_NONE=0,
	TAT_VERTICAL_FRAMES=1,
};
struct TileDef
{
	std::string name;
	bool backface_culling; // Takes effect only in special cases
	struct{
		enum TileAnimationType type;
		int aspect_w; // width for aspect ratio
		int aspect_h; // height for aspect ratio
		float length; // seconds
	} animation;

	TileDef()
	{
		name = "";
		backface_culling = true;
		animation.type = TAT_NONE;
		animation.aspect_w = 1;
		animation.aspect_h = 1;
		animation.length = 1.0;
	}

	void serialize(std::ostream &os, u16 protocol_version) const;
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
	NDT_NODEBOX,
	NDT_GLASSLIKE_FRAMED, // Glass-like, draw connected frames and all all
	                      // visible faces
						  // uses 2 textures, one for frames, second for faces
	NDT_FIRELIKE, // Draw faces slightly rotated and only on connecting nodes,
	NDT_GLASSLIKE_FRAMED_OPTIONAL,	// enabled -> connected, disabled -> Glass-like
									// uses 2 textures, one for frames, second for faces
	NDT_MESH, // Uses static meshes
};

#define CF_SPECIAL_COUNT 6

struct ContentFeatures
{
	/*
		Cached stuff
	*/
#ifndef SERVER
	// 0     1     2     3     4     5
	// up    down  right left  back  front 
	TileSpec tiles[6];
	// Special tiles
	// - Currently used for flowing liquids
	TileSpec special_tiles[CF_SPECIAL_COUNT];
	u8 solidness; // Used when choosing which face is drawn
	u8 visual_solidness; // When solidness=0, this tells how it looks like
	bool backface_culling;
#endif

	// Server-side cached callback existence for fast skipping
	bool has_on_construct;
	bool has_on_destruct;
	bool has_after_destruct;

	/*
		Actual data
	*/

	std::string name; // "" = undefined node
	ItemGroupList groups; // Same as in itemdef

	// Visual definition
	enum NodeDrawType drawtype;
	std::string mesh;
#ifndef SERVER
	scene::IMesh *mesh_ptr[24];
#endif	
	float visual_scale; // Misc. scale parameter
	TileDef tiledef[6];
	TileDef tiledef_special[CF_SPECIAL_COUNT]; // eg. flowing liquid
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
	// Player cannot build to these (placement prediction disabled)
	bool rightclickable;
	// Flowing liquid or snow, value = default level
	u8 leveled;
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
	// Is liquid renewable (new liquid source will be created between 2 existing)
	bool liquid_renewable;
	// Ice for water, water for ice
	std::string freezemelt;
	// Number of flowing liquids surrounding source
	u8 liquid_range;
	u8 drowning;
	// Amount of light the node emits
	u8 light_source;
	u32 damage_per_second;
	NodeBox node_box;
	NodeBox selection_box;
	NodeBox collision_box;
	// Used for waving leaves/plants
	u8 waving;
	// Compatibility with old maps
	// Set to true if paramtype used to be 'facedir_simple'
	bool legacy_facedir_simple;
	// Set to true if wall_mounted used to be set to true
	bool legacy_wallmounted;

	// Sound properties
	SimpleSoundSpec sound_footstep;
	SimpleSoundSpec sound_dig;
	SimpleSoundSpec sound_dug;

	/*
		Methods
	*/
	
	ContentFeatures();
	~ContentFeatures();
	void reset();
	void serialize(std::ostream &os, u16 protocol_version);
	void deSerialize(std::istream &is);
	void serializeOld(std::ostream &os, u16 protocol_version);
	void deSerializeOld(std::istream &is, int version);

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

struct NodeResolveInfo {
	std::string n_wanted;
	std::string n_alt;
	content_t c_fallback;
	content_t *output;
};

#define NR_STATUS_FAILURE 0
#define NR_STATUS_PENDING 1
#define NR_STATUS_SUCCESS 2

class NodeResolver {
public:
	NodeResolver(INodeDefManager *ndef);
	~NodeResolver();

	int addNode(std::string n_wanted, std::string n_alt,
		content_t c_fallback, content_t *content);
	int addNodeList(const char *nodename, std::vector<content_t> *content_vec);

	bool cancelNode(content_t *content);
	int cancelNodeList(std::vector<content_t> *content_vec);

	int resolveNodes();

	bool isNodeRegFinished() { return m_is_node_registration_complete; }

private:
	INodeDefManager *m_ndef;
	bool m_is_node_registration_complete;
	std::list<NodeResolveInfo *> m_pending_contents;
	std::list<std::pair<std::string, std::vector<content_t> *> > m_pending_content_vecs;
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
	// Allows "group:name" in addition to regular node names
	virtual void getIds(const std::string &name, std::set<content_t> &result)
			const=0;
	virtual const ContentFeatures& get(const std::string &name) const=0;
	
	virtual void serialize(std::ostream &os, u16 protocol_version)=0;

	virtual NodeResolver *getResolver()=0;
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
	// If not found, returns CONTENT_IGNORE
	virtual content_t getId(const std::string &name) const=0;
	// Allows "group:name" in addition to regular node names
	virtual void getIds(const std::string &name, std::set<content_t> &result)
			const=0;
	// If not found, returns the features of CONTENT_UNKNOWN
	virtual const ContentFeatures& get(const std::string &name) const=0;

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
	*/
	virtual void updateTextures(IGameDef *gamedef)=0;

	virtual void serialize(std::ostream &os, u16 protocol_version)=0;
	virtual void deSerialize(std::istream &is)=0;

	virtual NodeResolver *getResolver()=0;
};

IWritableNodeDefManager *createNodeDefManager();

#endif

