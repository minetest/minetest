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
#include "util/numeric.h"
#include "mapnode.h"
#ifndef SERVER
#include "client/tile.h"
#include "shader.h"
class Client;
#endif
#include "itemgroup.h"
#include "sound.h" // SimpleSoundSpec
#include "constants.h" // BS
#include "tileanimation.h"

class INodeDefManager;
class IItemDefManager;
class ITextureSource;
class IShaderSource;
class IGameDef;
class NodeResolver;

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
	// 2D rotation for things like plants
	CPT2_DEGROTATE,
	// Mesh options for plants
	CPT2_MESHOPTIONS,
	// Index for palette
	CPT2_COLOR,
	// 3 bits of palette index, then facedir
	CPT2_COLORED_FACEDIR,
	// 5 bits of palette index, then wallmounted
	CPT2_COLORED_WALLMOUNTED,
	// Glasslike framed drawtype internal liquid level, param2 values 0 to 63
	CPT2_GLASSLIKE_LIQUID_LEVEL,
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
	NODEBOX_CONNECTED, // optionally draws nodeboxes if a neighbor node attaches
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
	// NODEBOX_CONNECTED
	std::vector<aabb3f> connect_top;
	std::vector<aabb3f> connect_bottom;
	std::vector<aabb3f> connect_front;
	std::vector<aabb3f> connect_left;
	std::vector<aabb3f> connect_back;
	std::vector<aabb3f> connect_right;

	NodeBox()
	{ reset(); }

	void reset();
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is);
};

struct MapNode;
class NodeMetadata;

enum LeavesStyle {
	LEAVES_FANCY,
	LEAVES_SIMPLE,
	LEAVES_OPAQUE,
};

class TextureSettings {
public:
	LeavesStyle leaves_style;
	bool opaque_water;
	bool connected_glass;
	bool use_normal_texture;
	bool enable_mesh_cache;
	bool enable_minimap;

	TextureSettings() {}

	void readSettings();
};

enum NodeDrawType
{
	// A basic solid block
	NDT_NORMAL,
	// Nothing is drawn
	NDT_AIRLIKE,
	// Do not draw face towards same kind of flowing/source liquid
	NDT_LIQUID,
	// A very special kind of thing
	NDT_FLOWINGLIQUID,
	// Glass-like, don't draw faces towards other glass
	NDT_GLASSLIKE,
	// Leaves-like, draw all faces no matter what
	NDT_ALLFACES,
	// Enabled -> ndt_allfaces, disabled -> ndt_normal
	NDT_ALLFACES_OPTIONAL,
	// Single plane perpendicular to a surface
	NDT_TORCHLIKE,
	// Single plane parallel to a surface
	NDT_SIGNLIKE,
	// 2 vertical planes in a 'X' shape diagonal to XZ axes.
	// paramtype2 = "meshoptions" allows various forms, sizes and
	// vertical and horizontal random offsets.
	NDT_PLANTLIKE,
	// Fenceposts that connect to neighbouring fenceposts with horizontal bars
	NDT_FENCELIKE,
	// Selects appropriate junction texture to connect like rails to
	// neighbouring raillikes.
	NDT_RAILLIKE,
	// Custom Lua-definable structure of multiple cuboids
	NDT_NODEBOX,
	// Glass-like, draw connected frames and all visible faces.
	// param2 > 0 defines 64 levels of internal liquid
	// Uses 3 textures, one for frames, second for faces,
	// optional third is a 'special tile' for the liquid.
	NDT_GLASSLIKE_FRAMED,
	// Draw faces slightly rotated and only on neighbouring nodes
	NDT_FIRELIKE,
	// Enabled -> ndt_glasslike_framed, disabled -> ndt_glasslike
	NDT_GLASSLIKE_FRAMED_OPTIONAL,
	// Uses static meshes
	NDT_MESH,
};

// Mesh options for NDT_PLANTLIKE with CPT2_MESHOPTIONS
static const u8 MO_MASK_STYLE          = 0x07;
static const u8 MO_BIT_RANDOM_OFFSET   = 0x08;
static const u8 MO_BIT_SCALE_SQRT2     = 0x10;
static const u8 MO_BIT_RANDOM_OFFSET_Y = 0x20;
enum PlantlikeStyle {
	PLANT_STYLE_CROSS,
	PLANT_STYLE_CROSS2,
	PLANT_STYLE_STAR,
	PLANT_STYLE_HASH,
	PLANT_STYLE_HASH2,
};

/*
	Stand-alone definition of a TileSpec (basically a server-side TileSpec)
*/

struct TileDef
{
	std::string name;
	bool backface_culling; // Takes effect only in special cases
	bool tileable_horizontal;
	bool tileable_vertical;
	//! If true, the tile has its own color.
	bool has_color;
	//! The color of the tile.
	video::SColor color;

	struct TileAnimationParams animation;

	TileDef() :
		name(""),
		backface_culling(true),
		tileable_horizontal(true),
		tileable_vertical(true),
		has_color(false),
		color(video::SColor(0xFFFFFFFF))
	{
		animation.type = TAT_NONE;
	}

	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is, const u8 contentfeatures_version, const NodeDrawType drawtype);
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

	// --- GENERAL PROPERTIES ---

	std::string name; // "" = undefined node
	ItemGroupList groups; // Same as in itemdef
	// Type of MapNode::param1
	ContentParamType param_type;
	// Type of MapNode::param2
	ContentParamType2 param_type_2;

	// --- VISUAL PROPERTIES ---

	enum NodeDrawType drawtype;
	std::string mesh;
#ifndef SERVER
	scene::IMesh *mesh_ptr[24];
	video::SColor minimap_color;
#endif
	float visual_scale; // Misc. scale parameter
	TileDef tiledef[6];
	// These will be drawn over the base tiles.
	TileDef tiledef_overlay[6];
	TileDef tiledef_special[CF_SPECIAL_COUNT]; // eg. flowing liquid
	// If 255, the node is opaque.
	// Otherwise it uses texture alpha.
	u8 alpha;
	// The color of the node.
	video::SColor color;
	std::string palette_name;
	std::vector<video::SColor> *palette;
	// Used for waving leaves/plants
	u8 waving;
	// for NDT_CONNECTED pairing
	u8 connect_sides;
	std::vector<std::string> connects_to;
	std::set<content_t> connects_to_ids;
	// Post effect color, drawn when the camera is inside the node.
	video::SColor post_effect_color;
	// Flowing liquid or snow, value = default level
	u8 leveled;

	// --- LIGHTING-RELATED ---

	bool light_propagates;
	bool sunlight_propagates;
	// Amount of light the node emits
	u8 light_source;

	// --- MAP GENERATION ---

	// True for all ground-like things like stone and mud, false for eg. trees
	bool is_ground_content;

	// --- INTERACTION PROPERTIES ---

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
	u32 damage_per_second;

	// --- LIQUID PROPERTIES ---

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
	// Number of flowing liquids surrounding source
	u8 liquid_range;
	u8 drowning;
	// Liquids flow into and replace node
	bool floodable;

	// --- NODEBOXES ---

	NodeBox node_box;
	NodeBox selection_box;
	NodeBox collision_box;

	// --- SOUND PROPERTIES ---

	SimpleSoundSpec sound_footstep;
	SimpleSoundSpec sound_dig;
	SimpleSoundSpec sound_dug;

	// --- LEGACY ---

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
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is);
	void serializeOld(std::ostream &os, u16 protocol_version) const;
	void deSerializeOld(std::istream &is, int version);
	/*!
	 * Since vertex alpha is no longer supported, this method
	 * adds opacity directly to the texture pixels.
	 *
	 * \param tiles array of the tile definitions.
	 * \param length length of tiles
	 */
	void correctAlpha(TileDef *tiles, int length);

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

	int getGroup(const std::string &group) const
	{
		return itemgroup_get(groups, group);
	}

#ifndef SERVER
	void fillTileAttribs(ITextureSource *tsrc, TileLayer *tile, TileDef *tiledef,
		u32 shader_id, bool use_normal_texture, bool backface_culling,
		u8 material_type);
	void updateTextures(ITextureSource *tsrc, IShaderSource *shdsrc,
		scene::IMeshManipulator *meshmanip, Client *client, const TextureSettings &tsettings);
#endif
};

class INodeDefManager {
public:
	INodeDefManager(){}
	virtual ~INodeDefManager(){}
	// Get node definition
	virtual const ContentFeatures &get(content_t c) const=0;
	virtual const ContentFeatures &get(const MapNode &n) const=0;
	virtual bool getId(const std::string &name, content_t &result) const=0;
	virtual content_t getId(const std::string &name) const=0;
	// Allows "group:name" in addition to regular node names
	// returns false if node name not found, true otherwise
	virtual bool getIds(const std::string &name, std::set<content_t> &result)
			const=0;
	virtual const ContentFeatures &get(const std::string &name) const=0;

	virtual void serialize(std::ostream &os, u16 protocol_version) const=0;

	virtual void pendNodeResolve(NodeResolver *nr)=0;
	virtual bool cancelNodeResolveCallback(NodeResolver *nr)=0;
	virtual bool nodeboxConnects(const MapNode from, const MapNode to, u8 connect_face)=0;
	/*!
	 * Returns the smallest box in node coordinates that
	 * contains all nodes' selection boxes.
	 */
	virtual core::aabbox3d<s16> getSelectionBoxIntUnion() const=0;
};

class IWritableNodeDefManager : public INodeDefManager {
public:
	IWritableNodeDefManager(){}
	virtual ~IWritableNodeDefManager(){}
	virtual IWritableNodeDefManager* clone()=0;
	// Get node definition
	virtual const ContentFeatures &get(content_t c) const=0;
	virtual const ContentFeatures &get(const MapNode &n) const=0;
	virtual bool getId(const std::string &name, content_t &result) const=0;
	// If not found, returns CONTENT_IGNORE
	virtual content_t getId(const std::string &name) const=0;
	// Allows "group:name" in addition to regular node names
	virtual bool getIds(const std::string &name, std::set<content_t> &result)
		const=0;
	// If not found, returns the features of CONTENT_UNKNOWN
	virtual const ContentFeatures &get(const std::string &name) const=0;

	// Register node definition by name (allocate an id)
	// If returns CONTENT_IGNORE, could not allocate id
	virtual content_t set(const std::string &name,
			const ContentFeatures &def)=0;
	// If returns CONTENT_IGNORE, could not allocate id
	virtual content_t allocateDummy(const std::string &name)=0;
	// Remove a node
	virtual void removeNode(const std::string &name)=0;

	/*
		Update item alias mapping.
		Call after updating item definitions.
	*/
	virtual void updateAliases(IItemDefManager *idef)=0;

	/*
		Override textures from servers with ones specified in texturepack/override.txt
	*/
	virtual void applyTextureOverrides(const std::string &override_filepath)=0;

	/*
		Update tile textures to latest return values of TextueSource.
	*/
	virtual void updateTextures(IGameDef *gamedef,
		void (*progress_cbk)(void *progress_args, u32 progress, u32 max_progress),
		void *progress_cbk_args)=0;

	virtual void serialize(std::ostream &os, u16 protocol_version) const=0;
	virtual void deSerialize(std::istream &is)=0;

	virtual void setNodeRegistrationStatus(bool completed)=0;

	virtual void pendNodeResolve(NodeResolver *nr)=0;
	virtual bool cancelNodeResolveCallback(NodeResolver *nr)=0;
	virtual void runNodeResolveCallbacks()=0;
	virtual void resetNodeResolveState()=0;
	virtual void mapNodeboxConnections()=0;
	virtual core::aabbox3d<s16> getSelectionBoxIntUnion() const=0;
};

IWritableNodeDefManager *createNodeDefManager();

class NodeResolver {
public:
	NodeResolver();
	virtual ~NodeResolver();
	virtual void resolveNodeNames() = 0;

	bool getIdFromNrBacklog(content_t *result_out,
		const std::string &node_alt, content_t c_fallback);
	bool getIdsFromNrBacklog(std::vector<content_t> *result_out,
		bool all_required=false, content_t c_fallback=CONTENT_IGNORE);

	void nodeResolveInternal();

	u32 m_nodenames_idx;
	u32 m_nnlistsizes_idx;
	std::vector<std::string> m_nodenames;
	std::vector<size_t> m_nnlistsizes;
	INodeDefManager *m_ndef;
	bool m_resolve_done;
};

#endif
