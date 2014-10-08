/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "nodedef.h"

#include "main.h" // For g_settings
#include "itemdef.h"
#ifndef SERVER
#include "tile.h"
#include "mesh.h"
#include <IMeshManipulator.h>
#endif
#include "log.h"
#include "settings.h"
#include "nameidmapping.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "exceptions.h"
#include "debug.h"
#include "gamedef.h"

/*
	NodeBox
*/

void NodeBox::reset()
{
	type = NODEBOX_REGULAR;
	// default is empty
	fixed.clear();
	// default is sign/ladder-like
	wall_top = aabb3f(-BS/2, BS/2-BS/16., -BS/2, BS/2, BS/2, BS/2);
	wall_bottom = aabb3f(-BS/2, -BS/2, -BS/2, BS/2, -BS/2+BS/16., BS/2);
	wall_side = aabb3f(-BS/2, -BS/2, -BS/2, -BS/2+BS/16., BS/2, BS/2);
}

void NodeBox::serialize(std::ostream &os, u16 protocol_version) const
{
	int version = protocol_version >= 21 ? 2 : 1;
	writeU8(os, version);

	if (version == 1 && type == NODEBOX_LEVELED)
		writeU8(os, NODEBOX_FIXED);
	else
		writeU8(os, type);

	if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	{
		writeU16(os, fixed.size());
		for(std::vector<aabb3f>::const_iterator
				i = fixed.begin();
				i != fixed.end(); i++)
		{
			writeV3F1000(os, i->MinEdge);
			writeV3F1000(os, i->MaxEdge);
		}
	}
	else if(type == NODEBOX_WALLMOUNTED)
	{
		writeV3F1000(os, wall_top.MinEdge);
		writeV3F1000(os, wall_top.MaxEdge);
		writeV3F1000(os, wall_bottom.MinEdge);
		writeV3F1000(os, wall_bottom.MaxEdge);
		writeV3F1000(os, wall_side.MinEdge);
		writeV3F1000(os, wall_side.MaxEdge);
	}
}

void NodeBox::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version < 1 || version > 2)
		throw SerializationError("unsupported NodeBox version");

	reset();

	type = (enum NodeBoxType)readU8(is);

	if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	{
		u16 fixed_count = readU16(is);
		while(fixed_count--)
		{
			aabb3f box;
			box.MinEdge = readV3F1000(is);
			box.MaxEdge = readV3F1000(is);
			fixed.push_back(box);
		}
	}
	else if(type == NODEBOX_WALLMOUNTED)
	{
		wall_top.MinEdge = readV3F1000(is);
		wall_top.MaxEdge = readV3F1000(is);
		wall_bottom.MinEdge = readV3F1000(is);
		wall_bottom.MaxEdge = readV3F1000(is);
		wall_side.MinEdge = readV3F1000(is);
		wall_side.MaxEdge = readV3F1000(is);
	}
}

/*
	TileDef
*/

void TileDef::serialize(std::ostream &os, u16 protocol_version) const
{
	if(protocol_version >= 17)
		writeU8(os, 1);
	else
		writeU8(os, 0);
	os<<serializeString(name);
	writeU8(os, animation.type);
	writeU16(os, animation.aspect_w);
	writeU16(os, animation.aspect_h);
	writeF1000(os, animation.length);
	if(protocol_version >= 17)
		writeU8(os, backface_culling);
}

void TileDef::deSerialize(std::istream &is)
{
	int version = readU8(is);
	name = deSerializeString(is);
	animation.type = (TileAnimationType)readU8(is);
	animation.aspect_w = readU16(is);
	animation.aspect_h = readU16(is);
	animation.length = readF1000(is);
	if(version >= 1)
		backface_culling = readU8(is);
}

/*
	SimpleSoundSpec serialization
*/

static void serializeSimpleSoundSpec(const SimpleSoundSpec &ss,
		std::ostream &os)
{
	os<<serializeString(ss.name);
	writeF1000(os, ss.gain);
}
static void deSerializeSimpleSoundSpec(SimpleSoundSpec &ss, std::istream &is)
{
	ss.name = deSerializeString(is);
	ss.gain = readF1000(is);
}

/*
	ContentFeatures
*/

ContentFeatures::ContentFeatures()
{
	reset();
}

ContentFeatures::~ContentFeatures()
{
}

void ContentFeatures::reset()
{
	/*
		Cached stuff
	*/
#ifndef SERVER
	solidness = 2;
	visual_solidness = 0;
	backface_culling = true;
#endif
	has_on_construct = false;
	has_on_destruct = false;
	has_after_destruct = false;
	/*
		Actual data

		NOTE: Most of this is always overridden by the default values given
		      in builtin.lua
	*/
	name = "";
	groups.clear();
	// Unknown nodes can be dug
	groups["dig_immediate"] = 2;
	drawtype = NDT_NORMAL;
	mesh = "";
#ifndef SERVER
	for(u32 i = 0; i < 24; i++)
		mesh_ptr[i] = NULL;
#endif
	visual_scale = 1.0;
	for(u32 i = 0; i < 6; i++)
		tiledef[i] = TileDef();
	for(u16 j = 0; j < CF_SPECIAL_COUNT; j++)
		tiledef_special[j] = TileDef();
	alpha = 255;
	post_effect_color = video::SColor(0, 0, 0, 0);
	param_type = CPT_NONE;
	param_type_2 = CPT2_NONE;
	is_ground_content = false;
	light_propagates = false;
	sunlight_propagates = false;
	walkable = true;
	pointable = true;
	diggable = true;
	climbable = false;
	buildable_to = false;
	rightclickable = true;
	leveled = 0;
	liquid_type = LIQUID_NONE;
	liquid_alternative_flowing = "";
	liquid_alternative_source = "";
	liquid_viscosity = 0;
	liquid_renewable = true;
	freezemelt = "";
	liquid_range = LIQUID_LEVEL_MAX+1;
	drowning = 0;
	light_source = 0;
	damage_per_second = 0;
	node_box = NodeBox();
	selection_box = NodeBox();
	collision_box = NodeBox();
	waving = 0;
	legacy_facedir_simple = false;
	legacy_wallmounted = false;
	sound_footstep = SimpleSoundSpec();
	sound_dig = SimpleSoundSpec("__group");
	sound_dug = SimpleSoundSpec();
}

void ContentFeatures::serialize(std::ostream &os, u16 protocol_version)
{
	if(protocol_version < 24){
		serializeOld(os, protocol_version);
		return;
	}

	writeU8(os, 7); // version
	os<<serializeString(name);
	writeU16(os, groups.size());
	for(ItemGroupList::const_iterator
			i = groups.begin(); i != groups.end(); i++){
		os<<serializeString(i->first);
		writeS16(os, i->second);
	}
	writeU8(os, drawtype);
	writeF1000(os, visual_scale);
	writeU8(os, 6);
	for(u32 i = 0; i < 6; i++)
		tiledef[i].serialize(os, protocol_version);
	writeU8(os, CF_SPECIAL_COUNT);
	for(u32 i = 0; i < CF_SPECIAL_COUNT; i++){
		tiledef_special[i].serialize(os, protocol_version);
	}
	writeU8(os, alpha);
	writeU8(os, post_effect_color.getAlpha());
	writeU8(os, post_effect_color.getRed());
	writeU8(os, post_effect_color.getGreen());
	writeU8(os, post_effect_color.getBlue());
	writeU8(os, param_type);
	writeU8(os, param_type_2);
	writeU8(os, is_ground_content);
	writeU8(os, light_propagates);
	writeU8(os, sunlight_propagates);
	writeU8(os, walkable);
	writeU8(os, pointable);
	writeU8(os, diggable);
	writeU8(os, climbable);
	writeU8(os, buildable_to);
	os<<serializeString(""); // legacy: used to be metadata_name
	writeU8(os, liquid_type);
	os<<serializeString(liquid_alternative_flowing);
	os<<serializeString(liquid_alternative_source);
	writeU8(os, liquid_viscosity);
	writeU8(os, liquid_renewable);
	writeU8(os, light_source);
	writeU32(os, damage_per_second);
	node_box.serialize(os, protocol_version);
	selection_box.serialize(os, protocol_version);
	writeU8(os, legacy_facedir_simple);
	writeU8(os, legacy_wallmounted);
	serializeSimpleSoundSpec(sound_footstep, os);
	serializeSimpleSoundSpec(sound_dig, os);
	serializeSimpleSoundSpec(sound_dug, os);
	writeU8(os, rightclickable);
	writeU8(os, drowning);
	writeU8(os, leveled);
	writeU8(os, liquid_range);
	writeU8(os, waving);
	// Stuff below should be moved to correct place in a version that otherwise changes
	// the protocol version
	os<<serializeString(mesh);
	collision_box.serialize(os, protocol_version);
}

void ContentFeatures::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 7){
		deSerializeOld(is, version);
		return;
	}

	name = deSerializeString(is);
	groups.clear();
	u32 groups_size = readU16(is);
	for(u32 i = 0; i < groups_size; i++){
		std::string name = deSerializeString(is);
		int value = readS16(is);
		groups[name] = value;
	}
	drawtype = (enum NodeDrawType)readU8(is);
	visual_scale = readF1000(is);
	if(readU8(is) != 6)
		throw SerializationError("unsupported tile count");
	for(u32 i = 0; i < 6; i++)
		tiledef[i].deSerialize(is);
	if(readU8(is) != CF_SPECIAL_COUNT)
		throw SerializationError("unsupported CF_SPECIAL_COUNT");
	for(u32 i = 0; i < CF_SPECIAL_COUNT; i++)
		tiledef_special[i].deSerialize(is);
	alpha = readU8(is);
	post_effect_color.setAlpha(readU8(is));
	post_effect_color.setRed(readU8(is));
	post_effect_color.setGreen(readU8(is));
	post_effect_color.setBlue(readU8(is));
	param_type = (enum ContentParamType)readU8(is);
	param_type_2 = (enum ContentParamType2)readU8(is);
	is_ground_content = readU8(is);
	light_propagates = readU8(is);
	sunlight_propagates = readU8(is);
	walkable = readU8(is);
	pointable = readU8(is);
	diggable = readU8(is);
	climbable = readU8(is);
	buildable_to = readU8(is);
	deSerializeString(is); // legacy: used to be metadata_name
	liquid_type = (enum LiquidType)readU8(is);
	liquid_alternative_flowing = deSerializeString(is);
	liquid_alternative_source = deSerializeString(is);
	liquid_viscosity = readU8(is);
	liquid_renewable = readU8(is);
	light_source = readU8(is);
	damage_per_second = readU32(is);
	node_box.deSerialize(is);
	selection_box.deSerialize(is);
	legacy_facedir_simple = readU8(is);
	legacy_wallmounted = readU8(is);
	deSerializeSimpleSoundSpec(sound_footstep, is);
	deSerializeSimpleSoundSpec(sound_dig, is);
	deSerializeSimpleSoundSpec(sound_dug, is);
	rightclickable = readU8(is);
	drowning = readU8(is);
	leveled = readU8(is);
	liquid_range = readU8(is);
	waving = readU8(is);
	// If you add anything here, insert it primarily inside the try-catch
	// block to not need to increase the version.
	try{
		// Stuff below should be moved to correct place in a version that
		// otherwise changes the protocol version
	mesh = deSerializeString(is);
	collision_box.deSerialize(is);
	}catch(SerializationError &e) {};
}

/*
	CNodeDefManager
*/

class CNodeDefManager: public IWritableNodeDefManager {
public:
	CNodeDefManager();
	virtual ~CNodeDefManager();
	void clear();
	virtual IWritableNodeDefManager *clone();
	virtual const ContentFeatures& get(content_t c) const;
	virtual const ContentFeatures& get(const MapNode &n) const;
	virtual bool getId(const std::string &name, content_t &result) const;
	virtual content_t getId(const std::string &name) const;
	virtual void getIds(const std::string &name, std::set<content_t> &result) const;
	virtual const ContentFeatures& get(const std::string &name) const;
	content_t allocateId();
	virtual content_t set(const std::string &name, const ContentFeatures &def);
	virtual content_t allocateDummy(const std::string &name);
	virtual void updateAliases(IItemDefManager *idef);
	virtual void updateTextures(IGameDef *gamedef);
	void serialize(std::ostream &os, u16 protocol_version);
	void deSerialize(std::istream &is);
	virtual NodeResolver *getResolver();

private:
	void addNameIdMapping(content_t i, std::string name);
#ifndef SERVER
	void fillTileAttribs(ITextureSource *tsrc, TileSpec *tile, TileDef *tiledef,
		u32 shader_id, bool use_normal_texture, bool backface_culling,
		u8 alpha, u8 material_type);
#endif

	// Features indexed by id
	std::vector<ContentFeatures> m_content_features;

	// A mapping for fast converting back and forth between names and ids
	NameIdMapping m_name_id_mapping;

	// Like m_name_id_mapping, but only from names to ids, and includes
	// item aliases too. Updated by updateAliases()
	// Note: Not serialized.

	std::map<std::string, content_t> m_name_id_mapping_with_aliases;

	// A mapping from groups to a list of content_ts (and their levels)
	// that belong to it.  Necessary for a direct lookup in getIds().
	// Note: Not serialized.
	std::map<std::string, GroupItems> m_group_to_items;

	// Next possibly free id
	content_t m_next_id;

	// NodeResolver to queue pending node resolutions
	NodeResolver m_resolver;
};


CNodeDefManager::CNodeDefManager() :
	m_resolver(this)
{
	clear();
}


CNodeDefManager::~CNodeDefManager()
{
#ifndef SERVER
	for (u32 i = 0; i < m_content_features.size(); i++) {
		ContentFeatures *f = &m_content_features[i];
		for (u32 j = 0; j < 24; j++) {
			if (f->mesh_ptr[j])
				f->mesh_ptr[j]->drop();
		}
	}
#endif
}


void CNodeDefManager::clear()
{
	m_content_features.clear();
	m_name_id_mapping.clear();
	m_name_id_mapping_with_aliases.clear();
	m_group_to_items.clear();
	m_next_id = 0;

	u32 initial_length = 0;
	initial_length = MYMAX(initial_length, CONTENT_UNKNOWN + 1);
	initial_length = MYMAX(initial_length, CONTENT_AIR + 1);
	initial_length = MYMAX(initial_length, CONTENT_IGNORE + 1);
	m_content_features.resize(initial_length);

	// Set CONTENT_UNKNOWN
	{
		ContentFeatures f;
		f.name = "unknown";
		// Insert directly into containers
		content_t c = CONTENT_UNKNOWN;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}

	// Set CONTENT_AIR
	{
		ContentFeatures f;
		f.name                = "air";
		f.drawtype            = NDT_AIRLIKE;
		f.param_type          = CPT_LIGHT;
		f.light_propagates    = true;
		f.sunlight_propagates = true;
		f.walkable            = false;
		f.pointable           = false;
		f.diggable            = false;
		f.buildable_to        = true;
		f.is_ground_content   = true;
		// Insert directly into containers
		content_t c = CONTENT_AIR;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}

	// Set CONTENT_IGNORE
	{
		ContentFeatures f;
		f.name                = "ignore";
		f.drawtype            = NDT_AIRLIKE;
		f.param_type          = CPT_NONE;
		f.light_propagates    = false;
		f.sunlight_propagates = false;
		f.walkable            = false;
		f.pointable           = false;
		f.diggable            = false;
		f.buildable_to        = true; // A way to remove accidental CONTENT_IGNOREs
		f.is_ground_content   = true;
		// Insert directly into containers
		content_t c = CONTENT_IGNORE;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}
}


IWritableNodeDefManager *CNodeDefManager::clone()
{
	CNodeDefManager *mgr = new CNodeDefManager();
	*mgr = *this;
	return mgr;
}


const ContentFeatures& CNodeDefManager::get(content_t c) const
{
	if (c < m_content_features.size())
		return m_content_features[c];
	else
		return m_content_features[CONTENT_UNKNOWN];
}


const ContentFeatures& CNodeDefManager::get(const MapNode &n) const
{
	return get(n.getContent());
}


bool CNodeDefManager::getId(const std::string &name, content_t &result) const
{
	std::map<std::string, content_t>::const_iterator
		i = m_name_id_mapping_with_aliases.find(name);
	if(i == m_name_id_mapping_with_aliases.end())
		return false;
	result = i->second;
	return true;
}


content_t CNodeDefManager::getId(const std::string &name) const
{
	content_t id = CONTENT_IGNORE;
	getId(name, id);
	return id;
}


void CNodeDefManager::getIds(const std::string &name,
		std::set<content_t> &result) const
{
	//TimeTaker t("getIds", NULL, PRECISION_MICRO);
	if (name.substr(0,6) != "group:") {
		content_t id = CONTENT_IGNORE;
		if(getId(name, id))
			result.insert(id);
		return;
	}
	std::string group = name.substr(6);

	std::map<std::string, GroupItems>::const_iterator
		i = m_group_to_items.find(group);
	if (i == m_group_to_items.end())
		return;

	const GroupItems &items = i->second;
	for (GroupItems::const_iterator j = items.begin();
		j != items.end(); ++j) {
		if ((*j).second != 0)
			result.insert((*j).first);
	}
	//printf("getIds: %dus\n", t.stop());
}


const ContentFeatures& CNodeDefManager::get(const std::string &name) const
{
	content_t id = CONTENT_UNKNOWN;
	getId(name, id);
	return get(id);
}


// returns CONTENT_IGNORE if no free ID found
content_t CNodeDefManager::allocateId()
{
	for (content_t id = m_next_id;
			id >= m_next_id; // overflow?
			++id) {
		while (id >= m_content_features.size()) {
			m_content_features.push_back(ContentFeatures());
		}
		const ContentFeatures &f = m_content_features[id];
		if (f.name == "") {
			m_next_id = id + 1;
			return id;
		}
	}
	// If we arrive here, an overflow occurred in id.
	// That means no ID was found
	return CONTENT_IGNORE;
}


// IWritableNodeDefManager
content_t CNodeDefManager::set(const std::string &name, const ContentFeatures &def)
{
	assert(name != "");
	assert(name == def.name);

	// Don't allow redefining ignore (but allow air and unknown)
	if (name == "ignore") {
		infostream << "NodeDefManager: WARNING: Ignoring "
			"CONTENT_IGNORE redefinition"<<std::endl;
		return CONTENT_IGNORE;
	}

	content_t id = CONTENT_IGNORE;
	if (!m_name_id_mapping.getId(name, id)) { // ignore aliases
		// Get new id
		id = allocateId();
		if (id == CONTENT_IGNORE) {
			infostream << "NodeDefManager: WARNING: Absolute "
				"limit reached" << std::endl;
			return CONTENT_IGNORE;
		}
		assert(id != CONTENT_IGNORE);
		addNameIdMapping(id, name);
	}
	m_content_features[id] = def;
	verbosestream << "NodeDefManager: registering content id \"" << id
		<< "\": name=\"" << def.name << "\""<<std::endl;

	// Add this content to the list of all groups it belongs to
	// FIXME: This should remove a node from groups it no longer
	// belongs to when a node is re-registered
	for (ItemGroupList::const_iterator i = def.groups.begin();
		i != def.groups.end(); ++i) {
		std::string group_name = i->first;

		std::map<std::string, GroupItems>::iterator
			j = m_group_to_items.find(group_name);
		if (j == m_group_to_items.end()) {
			m_group_to_items[group_name].push_back(
					std::make_pair(id, i->second));
		} else {
			GroupItems &items = j->second;
			items.push_back(std::make_pair(id, i->second));
		}
	}
	return id;
}


content_t CNodeDefManager::allocateDummy(const std::string &name)
{
	assert(name != "");
	ContentFeatures f;
	f.name = name;
	return set(name, f);
}


void CNodeDefManager::updateAliases(IItemDefManager *idef)
{
	std::set<std::string> all = idef->getAll();
	m_name_id_mapping_with_aliases.clear();
	for (std::set<std::string>::iterator
			i = all.begin(); i != all.end(); i++) {
		std::string name = *i;
		std::string convert_to = idef->getAlias(name);
		content_t id;
		if (m_name_id_mapping.getId(convert_to, id)) {
			m_name_id_mapping_with_aliases.insert(
					std::make_pair(name, id));
		}
	}
}


void CNodeDefManager::updateTextures(IGameDef *gamedef)
{
#ifndef SERVER
	infostream << "CNodeDefManager::updateTextures(): Updating "
		"textures in node definitions" << std::endl;
	
	ITextureSource *tsrc = gamedef->tsrc();
	IShaderSource *shdsrc = gamedef->getShaderSource();
	scene::ISceneManager* smgr = gamedef->getSceneManager();
	scene::IMeshManipulator* meshmanip = smgr->getMeshManipulator();

	bool new_style_water           = g_settings->getBool("new_style_water");
	bool new_style_leaves          = g_settings->getBool("new_style_leaves");
	bool connected_glass           = g_settings->getBool("connected_glass");
	bool opaque_water              = g_settings->getBool("opaque_water");
	bool enable_shaders            = g_settings->getBool("enable_shaders");
	bool enable_bumpmapping        = g_settings->getBool("enable_bumpmapping");
	bool enable_parallax_occlusion = g_settings->getBool("enable_parallax_occlusion");

	bool use_normal_texture = enable_shaders &&
		(enable_bumpmapping || enable_parallax_occlusion);

	for (u32 i = 0; i < m_content_features.size(); i++) {
		ContentFeatures *f = &m_content_features[i];

		// Figure out the actual tiles to use
		TileDef tiledef[6];
		for (u32 j = 0; j < 6; j++) {
			tiledef[j] = f->tiledef[j];
			if (tiledef[j].name == "")
				tiledef[j].name = "unknown_node.png";
		}

		bool is_liquid = false;
		bool is_water_surface = false;

		u8 material_type = (f->alpha == 255) ?
			TILE_MATERIAL_BASIC : TILE_MATERIAL_ALPHA;

		switch (f->drawtype) {
		default:
		case NDT_NORMAL:
			f->solidness = 2;
			break;
		case NDT_AIRLIKE:
			f->solidness = 0;
			break;
		case NDT_LIQUID:
			assert(f->liquid_type == LIQUID_SOURCE);
			if (opaque_water)
				f->alpha = 255;
			if (new_style_water){
				f->solidness = 0;
			} else {
				f->solidness = 1;
				f->backface_culling = false;
			}
			is_liquid = true;
			break;
		case NDT_FLOWINGLIQUID:
			assert(f->liquid_type == LIQUID_FLOWING);
			f->solidness = 0;
			if (opaque_water)
				f->alpha = 255;
			is_liquid = true;
			break;
		case NDT_GLASSLIKE:
			f->solidness = 0;
			f->visual_solidness = 1;
			break;
		case NDT_GLASSLIKE_FRAMED:
			f->solidness = 0;
			f->visual_solidness = 1;
			break;
		case NDT_GLASSLIKE_FRAMED_OPTIONAL:
			f->solidness = 0;
			f->visual_solidness = 1;
			f->drawtype = connected_glass ? NDT_GLASSLIKE_FRAMED : NDT_GLASSLIKE;
			break;
		case NDT_ALLFACES:
			f->solidness = 0;
			f->visual_solidness = 1;
			break;
		case NDT_ALLFACES_OPTIONAL:
			if (new_style_leaves) {
				f->drawtype = NDT_ALLFACES;
				f->solidness = 0;
				f->visual_solidness = 1;
			} else {
				f->drawtype = NDT_NORMAL;
				f->solidness = 2;
				for (u32 i = 0; i < 6; i++)
					tiledef[i].name += std::string("^[noalpha");
			}
			if (f->waving == 1)
				material_type = TILE_MATERIAL_WAVING_LEAVES;
			break;
		case NDT_PLANTLIKE:
			f->solidness = 0;
			f->backface_culling = false;
			if (f->waving == 1)
				material_type = TILE_MATERIAL_WAVING_PLANTS;
			break;
		case NDT_FIRELIKE:
			f->backface_culling = false;
			f->solidness = 0;
			break;
		case NDT_MESH:
			f->solidness = 0;
			f->backface_culling = false;
			break;
		case NDT_TORCHLIKE:
		case NDT_SIGNLIKE:
		case NDT_FENCELIKE:
		case NDT_RAILLIKE:
		case NDT_NODEBOX:
			f->solidness = 0;
			break;
		}

		if (is_liquid) {
			material_type = (f->alpha == 255) ?
				TILE_MATERIAL_LIQUID_OPAQUE : TILE_MATERIAL_LIQUID_TRANSPARENT;
			if (f->name == "default:water_source")
				is_water_surface = true;
		}

		u32 tile_shader[6];
		for (u16 j = 0; j < 6; j++) {
			tile_shader[j] = shdsrc->getShader("nodes_shader",
				material_type, f->drawtype);
		}

		if (is_water_surface) {
			tile_shader[0] = shdsrc->getShader("water_surface_shader",
				material_type, f->drawtype);
		}

		// Tiles (fill in f->tiles[])
		for (u16 j = 0; j < 6; j++) {
			fillTileAttribs(tsrc, &f->tiles[j], &tiledef[j], tile_shader[j],
				use_normal_texture, f->backface_culling, f->alpha, material_type);
		}

		// Special tiles (fill in f->special_tiles[])
		for (u16 j = 0; j < CF_SPECIAL_COUNT; j++) {
			fillTileAttribs(tsrc, &f->special_tiles[j], &f->tiledef_special[j],
				tile_shader[j], use_normal_texture,
				f->tiledef_special[j].backface_culling, f->alpha, material_type);
		}

		// Meshnode drawtype
		// Read the mesh and apply scale
		if ((f->drawtype == NDT_MESH) && (f->mesh != "")) {
			f->mesh_ptr[0] = gamedef->getMesh(f->mesh);
			if (f->mesh_ptr[0]){
				v3f scale = v3f(1.0, 1.0, 1.0) * BS * f->visual_scale;
				scaleMesh(f->mesh_ptr[0], scale);
				recalculateBoundingBox(f->mesh_ptr[0]);
			}
		}

		//Convert regular nodebox nodes to meshnodes
		//Change the drawtype and apply scale
		else if ((f->drawtype == NDT_NODEBOX) && 
				((f->node_box.type == NODEBOX_REGULAR) ||
				(f->node_box.type == NODEBOX_FIXED)) &&
				(!f->node_box.fixed.empty())) {
			f->drawtype = NDT_MESH;
			f->mesh_ptr[0] = convertNodeboxNodeToMesh(f);
			v3f scale = v3f(1.0, 1.0, 1.0) * f->visual_scale;
			scaleMesh(f->mesh_ptr[0], scale);
			recalculateBoundingBox(f->mesh_ptr[0]);
		}

		//Cache 6dfacedir rotated clones of meshes
		if (f->mesh_ptr[0] && (f->param_type_2 == CPT2_FACEDIR)) {
			for (u16 j = 1; j < 24; j++) {
				f->mesh_ptr[j] = cloneMesh(f->mesh_ptr[0]);
				rotateMeshBy6dFacedir(f->mesh_ptr[j], j);
				recalculateBoundingBox(f->mesh_ptr[j]);
				meshmanip->recalculateNormals(f->mesh_ptr[j], false, false);
			}
		}
	}
#endif
}


#ifndef SERVER
void CNodeDefManager::fillTileAttribs(ITextureSource *tsrc, TileSpec *tile,
		TileDef *tiledef, u32 shader_id, bool use_normal_texture,
		bool backface_culling, u8 alpha, u8 material_type)
{
	tile->shader_id     = shader_id;
	tile->texture       = tsrc->getTexture(tiledef->name, &tile->texture_id);
	tile->alpha         = alpha;
	tile->material_type = material_type;

	// Normal texture
	if (use_normal_texture)
		tile->normal_texture = tsrc->getNormalTexture(tiledef->name);

	// Material flags
	tile->material_flags = 0;
	if (backface_culling)
		tile->material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
	if (tiledef->animation.type == TAT_VERTICAL_FRAMES)
		tile->material_flags |= MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;

	// Animation parameters
	int frame_count = 1;
	if (tile->material_flags & MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES) {
		// Get texture size to determine frame count by aspect ratio
		v2u32 size = tile->texture->getOriginalSize();
		int frame_height = (float)size.X /
				(float)tiledef->animation.aspect_w *
				(float)tiledef->animation.aspect_h;
		frame_count = size.Y / frame_height;
		int frame_length_ms = 1000.0 * tiledef->animation.length / frame_count;
		tile->animation_frame_count = frame_count;
		tile->animation_frame_length_ms = frame_length_ms;
	}

	if (frame_count == 1) {
		tile->material_flags &= ~MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
	} else {
		std::ostringstream os(std::ios::binary);
		for (int i = 0; i < frame_count; i++) {
			FrameSpec frame;

			os.str("");
			os << tiledef->name << "^[verticalframe:"
				<< frame_count << ":" << i;

			frame.texture = tsrc->getTexture(os.str(), &frame.texture_id);
			if (tile->normal_texture)
				frame.normal_texture = tsrc->getNormalTexture(os.str());
			tile->frames[i] = frame;
		}
	}
}
#endif


void CNodeDefManager::serialize(std::ostream &os, u16 protocol_version)
{
	writeU8(os, 1); // version
	u16 count = 0;
	std::ostringstream os2(std::ios::binary);
	for (u32 i = 0; i < m_content_features.size(); i++) {
		if (i == CONTENT_IGNORE || i == CONTENT_AIR
				|| i == CONTENT_UNKNOWN)
			continue;
		ContentFeatures *f = &m_content_features[i];
		if (f->name == "")
			continue;
		writeU16(os2, i);
		// Wrap it in a string to allow different lengths without
		// strict version incompatibilities
		std::ostringstream wrapper_os(std::ios::binary);
		f->serialize(wrapper_os, protocol_version);
		os2<<serializeString(wrapper_os.str());

		assert(count + 1 > count); // must not overflow
		count++;
	}
	writeU16(os, count);
	os << serializeLongString(os2.str());
}


void CNodeDefManager::deSerialize(std::istream &is)
{
	clear();
	int version = readU8(is);
	if (version != 1)
		throw SerializationError("unsupported NodeDefinitionManager version");
	u16 count = readU16(is);
	std::istringstream is2(deSerializeLongString(is), std::ios::binary);
	ContentFeatures f;
	for (u16 n = 0; n < count; n++) {
		u16 i = readU16(is2);

		// Read it from the string wrapper
		std::string wrapper = deSerializeString(is2);
		std::istringstream wrapper_is(wrapper, std::ios::binary);
		f.deSerialize(wrapper_is);

		// Check error conditions
		if (i == CONTENT_IGNORE || i == CONTENT_AIR || i == CONTENT_UNKNOWN) {
			infostream << "NodeDefManager::deSerialize(): WARNING: "
				"not changing builtin node " << i << std::endl;
			continue;
		}
		if (f.name == "") {
			infostream << "NodeDefManager::deSerialize(): WARNING: "
				"received empty name" << std::endl;
			continue;
		}

		// Ignore aliases
		u16 existing_id;
		if (m_name_id_mapping.getId(f.name, existing_id) && i != existing_id) {
			infostream << "NodeDefManager::deSerialize(): WARNING: "
				"already defined with different ID: " << f.name << std::endl;
			continue;
		}

		// All is ok, add node definition with the requested ID
		if (i >= m_content_features.size())
			m_content_features.resize((u32)(i) + 1);
		m_content_features[i] = f;
		addNameIdMapping(i, f.name);
		verbosestream << "deserialized " << f.name << std::endl;
	}
}


void CNodeDefManager::addNameIdMapping(content_t i, std::string name)
{
	m_name_id_mapping.set(i, name);
	m_name_id_mapping_with_aliases.insert(std::make_pair(name, i));
}


NodeResolver *CNodeDefManager::getResolver()
{
	return &m_resolver;
}


IWritableNodeDefManager *createNodeDefManager()
{
	return new CNodeDefManager();
}


//// Serialization of old ContentFeatures formats
void ContentFeatures::serializeOld(std::ostream &os, u16 protocol_version)
{
	if (protocol_version == 13)
	{
		writeU8(os, 5); // version
		os<<serializeString(name);
		writeU16(os, groups.size());
		for (ItemGroupList::const_iterator
				i = groups.begin(); i != groups.end(); i++) {
			os<<serializeString(i->first);
			writeS16(os, i->second);
		}
		writeU8(os, drawtype);
		writeF1000(os, visual_scale);
		writeU8(os, 6);
		for (u32 i = 0; i < 6; i++)
			tiledef[i].serialize(os, protocol_version);
		//CF_SPECIAL_COUNT = 2 before cf ver. 7 and protocol ver. 24
		writeU8(os, 2);
		for (u32 i = 0; i < 2; i++)
			tiledef_special[i].serialize(os, protocol_version);
		writeU8(os, alpha);
		writeU8(os, post_effect_color.getAlpha());
		writeU8(os, post_effect_color.getRed());
		writeU8(os, post_effect_color.getGreen());
		writeU8(os, post_effect_color.getBlue());
		writeU8(os, param_type);
		writeU8(os, param_type_2);
		writeU8(os, is_ground_content);
		writeU8(os, light_propagates);
		writeU8(os, sunlight_propagates);
		writeU8(os, walkable);
		writeU8(os, pointable);
		writeU8(os, diggable);
		writeU8(os, climbable);
		writeU8(os, buildable_to);
		os<<serializeString(""); // legacy: used to be metadata_name
		writeU8(os, liquid_type);
		os<<serializeString(liquid_alternative_flowing);
		os<<serializeString(liquid_alternative_source);
		writeU8(os, liquid_viscosity);
		writeU8(os, light_source);
		writeU32(os, damage_per_second);
		node_box.serialize(os, protocol_version);
		selection_box.serialize(os, protocol_version);
		writeU8(os, legacy_facedir_simple);
		writeU8(os, legacy_wallmounted);
		serializeSimpleSoundSpec(sound_footstep, os);
		serializeSimpleSoundSpec(sound_dig, os);
		serializeSimpleSoundSpec(sound_dug, os);
	}
	else if (protocol_version > 13 && protocol_version < 24) {
		writeU8(os, 6); // version
		os<<serializeString(name);
		writeU16(os, groups.size());
		for (ItemGroupList::const_iterator
			i = groups.begin(); i != groups.end(); i++) {
				os<<serializeString(i->first);
				writeS16(os, i->second);
		}
		writeU8(os, drawtype);
		writeF1000(os, visual_scale);
		writeU8(os, 6);
		for (u32 i = 0; i < 6; i++)
			tiledef[i].serialize(os, protocol_version);
		//CF_SPECIAL_COUNT = 2 before cf ver. 7 and protocol ver. 24
		writeU8(os, 2);
		for (u32 i = 0; i < 2; i++)
			tiledef_special[i].serialize(os, protocol_version);
		writeU8(os, alpha);
		writeU8(os, post_effect_color.getAlpha());
		writeU8(os, post_effect_color.getRed());
		writeU8(os, post_effect_color.getGreen());
		writeU8(os, post_effect_color.getBlue());
		writeU8(os, param_type);
		writeU8(os, param_type_2);
		writeU8(os, is_ground_content);
		writeU8(os, light_propagates);
		writeU8(os, sunlight_propagates);
		writeU8(os, walkable);
		writeU8(os, pointable);
		writeU8(os, diggable);
		writeU8(os, climbable);
		writeU8(os, buildable_to);
		os<<serializeString(""); // legacy: used to be metadata_name
		writeU8(os, liquid_type);
		os<<serializeString(liquid_alternative_flowing);
		os<<serializeString(liquid_alternative_source);
		writeU8(os, liquid_viscosity);
		writeU8(os, liquid_renewable);
		writeU8(os, light_source);
		writeU32(os, damage_per_second);
		node_box.serialize(os, protocol_version);
		selection_box.serialize(os, protocol_version);
		writeU8(os, legacy_facedir_simple);
		writeU8(os, legacy_wallmounted);
		serializeSimpleSoundSpec(sound_footstep, os);
		serializeSimpleSoundSpec(sound_dig, os);
		serializeSimpleSoundSpec(sound_dug, os);
		writeU8(os, rightclickable);
		writeU8(os, drowning);
		writeU8(os, leveled);
		writeU8(os, liquid_range);
	} else 
		throw SerializationError("ContentFeatures::serialize(): "
			"Unsupported version requested");
}


void ContentFeatures::deSerializeOld(std::istream &is, int version)
{
	if (version == 5) // In PROTOCOL_VERSION 13
	{
		name = deSerializeString(is);
		groups.clear();
		u32 groups_size = readU16(is);
		for(u32 i=0; i<groups_size; i++){
			std::string name = deSerializeString(is);
			int value = readS16(is);
			groups[name] = value;
		}
		drawtype = (enum NodeDrawType)readU8(is);
		visual_scale = readF1000(is);
		if (readU8(is) != 6)
			throw SerializationError("unsupported tile count");
		for (u32 i = 0; i < 6; i++)
			tiledef[i].deSerialize(is);
		if (readU8(is) != CF_SPECIAL_COUNT)
			throw SerializationError("unsupported CF_SPECIAL_COUNT");
		for (u32 i = 0; i < CF_SPECIAL_COUNT; i++)
			tiledef_special[i].deSerialize(is);
		alpha = readU8(is);
		post_effect_color.setAlpha(readU8(is));
		post_effect_color.setRed(readU8(is));
		post_effect_color.setGreen(readU8(is));
		post_effect_color.setBlue(readU8(is));
		param_type = (enum ContentParamType)readU8(is);
		param_type_2 = (enum ContentParamType2)readU8(is);
		is_ground_content = readU8(is);
		light_propagates = readU8(is);
		sunlight_propagates = readU8(is);
		walkable = readU8(is);
		pointable = readU8(is);
		diggable = readU8(is);
		climbable = readU8(is);
		buildable_to = readU8(is);
		deSerializeString(is); // legacy: used to be metadata_name
		liquid_type = (enum LiquidType)readU8(is);
		liquid_alternative_flowing = deSerializeString(is);
		liquid_alternative_source = deSerializeString(is);
		liquid_viscosity = readU8(is);
		light_source = readU8(is);
		damage_per_second = readU32(is);
		node_box.deSerialize(is);
		selection_box.deSerialize(is);
		legacy_facedir_simple = readU8(is);
		legacy_wallmounted = readU8(is);
		deSerializeSimpleSoundSpec(sound_footstep, is);
		deSerializeSimpleSoundSpec(sound_dig, is);
		deSerializeSimpleSoundSpec(sound_dug, is);
	} else if (version == 6) {
		name = deSerializeString(is);
		groups.clear();
		u32 groups_size = readU16(is);
		for (u32 i = 0; i < groups_size; i++) {
			std::string name = deSerializeString(is);
			int	value = readS16(is);
			groups[name] = value;
		}
		drawtype = (enum NodeDrawType)readU8(is);
		visual_scale = readF1000(is);
		if (readU8(is) != 6)
			throw SerializationError("unsupported tile count");
		for (u32 i = 0; i < 6; i++)
			tiledef[i].deSerialize(is);
		// CF_SPECIAL_COUNT in version 6 = 2
		if (readU8(is) != 2)
			throw SerializationError("unsupported CF_SPECIAL_COUNT");
		for (u32 i = 0; i < 2; i++)
			tiledef_special[i].deSerialize(is);
		alpha = readU8(is);
		post_effect_color.setAlpha(readU8(is));
		post_effect_color.setRed(readU8(is));
		post_effect_color.setGreen(readU8(is));
		post_effect_color.setBlue(readU8(is));
		param_type = (enum ContentParamType)readU8(is);
		param_type_2 = (enum ContentParamType2)readU8(is);
		is_ground_content = readU8(is);
		light_propagates = readU8(is);
		sunlight_propagates = readU8(is);
		walkable = readU8(is);
		pointable = readU8(is);
		diggable = readU8(is);
		climbable = readU8(is);
		buildable_to = readU8(is);
		deSerializeString(is); // legacy: used to be metadata_name
		liquid_type = (enum LiquidType)readU8(is);
		liquid_alternative_flowing = deSerializeString(is);
		liquid_alternative_source = deSerializeString(is);
		liquid_viscosity = readU8(is);
		liquid_renewable = readU8(is);
		light_source = readU8(is);
		damage_per_second = readU32(is);
		node_box.deSerialize(is);
		selection_box.deSerialize(is);
		legacy_facedir_simple = readU8(is);
		legacy_wallmounted = readU8(is);
		deSerializeSimpleSoundSpec(sound_footstep, is);
		deSerializeSimpleSoundSpec(sound_dig, is);
		deSerializeSimpleSoundSpec(sound_dug, is);
		rightclickable = readU8(is);
		drowning = readU8(is);
		leveled = readU8(is);
		liquid_range = readU8(is);
	} else {
		throw SerializationError("unsupported ContentFeatures version");
	}
}

/*
	NodeResolver
*/

NodeResolver::NodeResolver(INodeDefManager *ndef)
{
	m_ndef = ndef;
	m_is_node_registration_complete = false;
}


NodeResolver::~NodeResolver()
{
	while (!m_pending_contents.empty()) {
		NodeResolveInfo *nri = m_pending_contents.front();
		m_pending_contents.pop_front();
		delete nri;
	}
}


int NodeResolver::addNode(std::string n_wanted, std::string n_alt,
		content_t c_fallback, content_t *content)
{
	if (m_is_node_registration_complete) {
		if (m_ndef->getId(n_wanted, *content))
			return NR_STATUS_SUCCESS;

		if (n_alt == "")
			return NR_STATUS_FAILURE;

		return m_ndef->getId(n_alt, *content) ?
			NR_STATUS_SUCCESS : NR_STATUS_FAILURE;
	} else {
		NodeResolveInfo *nfi = new NodeResolveInfo;
		nfi->n_wanted   = n_wanted;
		nfi->n_alt      = n_alt;
		nfi->c_fallback = c_fallback;
		nfi->output     = content;

		m_pending_contents.push_back(nfi);

		return NR_STATUS_PENDING;
	}
}


int NodeResolver::addNodeList(const char *nodename,
		std::vector<content_t> *content_vec)
{
	if (m_is_node_registration_complete) {
		std::set<content_t> idset;
		std::set<content_t>::iterator it;

		m_ndef->getIds(nodename, idset);
		for (it = idset.begin(); it != idset.end(); ++it)
			content_vec->push_back(*it);

		return idset.size() ? NR_STATUS_SUCCESS : NR_STATUS_FAILURE;
	} else {
		m_pending_content_vecs.push_back(
			std::make_pair(std::string(nodename), content_vec));
		return NR_STATUS_PENDING;
	}
}


bool NodeResolver::cancelNode(content_t *content)
{
	bool found = false;

	std::list<NodeResolveInfo *>::iterator it = m_pending_contents.begin();
	while (it != m_pending_contents.end()) {
		NodeResolveInfo *nfi = *it;
		if (nfi->output == content) {
			it = m_pending_contents.erase(it);
			delete nfi;
			found = true;
		}
	}

	return found;
}


int NodeResolver::cancelNodeList(std::vector<content_t> *content_vec)
{
	int num_canceled = 0;

	std::list<std::pair<std::string, std::vector<content_t> *> >::iterator it;
	it = m_pending_content_vecs.begin();
	while (it != m_pending_content_vecs.end()) {
		if (it->second == content_vec) {
			it = m_pending_content_vecs.erase(it);
			num_canceled++;
		}
	}

	return num_canceled;
}


int NodeResolver::resolveNodes()
{
	int num_failed = 0;

	//// Resolve pending single node name -> content ID mappings
	while (!m_pending_contents.empty()) {
		NodeResolveInfo *nri = m_pending_contents.front();		
		m_pending_contents.pop_front();

		bool success = true;
		if (!m_ndef->getId(nri->n_wanted, *nri->output)) {
			success = (nri->n_alt != "") ?
				m_ndef->getId(nri->n_alt, *nri->output) : false;
		}

		if (!success) {
			*nri->output = nri->c_fallback;
			num_failed++;
			errorstream << "NodeResolver::resolveNodes():  Failed to "
				"resolve '" << nri->n_wanted;
			if (nri->n_alt != "")
				errorstream << "' and '" << nri->n_alt;
			errorstream << "' to a content ID" << std::endl;
		}

		delete nri;
	}

	//// Resolve pending node names and add to content_t vector
	while (!m_pending_content_vecs.empty()) {
		std::pair<std::string, std::vector<content_t> *> item =
			m_pending_content_vecs.front();
		m_pending_content_vecs.pop_front();

		std::string &name = item.first;
		std::vector<content_t> *output = item.second;
		
		std::set<content_t> idset;
		std::set<content_t>::iterator it;

		m_ndef->getIds(name, idset);
		for (it = idset.begin(); it != idset.end(); ++it)
			output->push_back(*it);
	}

	//// Mark node registration as complete so future resolve
	//// requests are satisfied immediately
	m_is_node_registration_complete = true;

	return num_failed;
}
