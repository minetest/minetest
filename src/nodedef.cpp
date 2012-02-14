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

#include "nodedef.h"

#include "main.h" // For g_settings
#include "itemdef.h"
#ifndef SERVER
#include "tile.h"
#endif
#include "log.h"
#include "settings.h"
#include "nameidmapping.h"

/*
	NodeBox
*/

void NodeBox::serialize(std::ostream &os) const
{
	writeU8(os, 0); // version
	writeU8(os, type);
	writeV3F1000(os, fixed.MinEdge);
	writeV3F1000(os, fixed.MaxEdge);
	writeV3F1000(os, wall_top.MinEdge);
	writeV3F1000(os, wall_top.MaxEdge);
	writeV3F1000(os, wall_bottom.MinEdge);
	writeV3F1000(os, wall_bottom.MaxEdge);
	writeV3F1000(os, wall_side.MinEdge);
	writeV3F1000(os, wall_side.MaxEdge);
}

void NodeBox::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0)
		throw SerializationError("unsupported NodeBox version");
	type = (enum NodeBoxType)readU8(is);
	fixed.MinEdge = readV3F1000(is);
	fixed.MaxEdge = readV3F1000(is);
	wall_top.MinEdge = readV3F1000(is);
	wall_top.MaxEdge = readV3F1000(is);
	wall_bottom.MinEdge = readV3F1000(is);
	wall_bottom.MaxEdge = readV3F1000(is);
	wall_side.MinEdge = readV3F1000(is);
	wall_side.MaxEdge = readV3F1000(is);
}

/*
	MaterialSpec
*/

void MaterialSpec::serialize(std::ostream &os) const
{
	os<<serializeString(tname);
	writeU8(os, backface_culling);
}

void MaterialSpec::deSerialize(std::istream &is)
{
	tname = deSerializeString(is);
	backface_culling = readU8(is);
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
#ifndef SERVER
	for(u16 j=0; j<CF_SPECIAL_COUNT; j++){
		delete special_materials[j];
		delete special_aps[j];
	}
#endif
}

void ContentFeatures::reset()
{
	/*
		Cached stuff
	*/
#ifndef SERVER
	for(u16 j=0; j<CF_SPECIAL_COUNT; j++){
		special_materials[j] = NULL;
		special_aps[j] = NULL;
	}
	solidness = 2;
	visual_solidness = 0;
	backface_culling = true;
#endif
	/*
		Actual data
		
		NOTE: Most of this is always overridden by the default values given
		      in builtin.lua
	*/
	name = "";
	drawtype = NDT_NORMAL;
	visual_scale = 1.0;
	for(u32 i=0; i<6; i++)
		tname_tiles[i] = "";
	for(u16 j=0; j<CF_SPECIAL_COUNT; j++)
		mspec_special[j] = MaterialSpec();
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
	metadata_name = "";
	liquid_type = LIQUID_NONE;
	liquid_alternative_flowing = "";
	liquid_alternative_source = "";
	liquid_viscosity = 0;
	light_source = 0;
	damage_per_second = 0;
	selection_box = NodeBox();
	material = MaterialProperties();
	// Make unknown blocks diggable
	material.diggability = DIGGABLE_CONSTANT;
	material.constant_time = 0.5;
	legacy_facedir_simple = false;
	legacy_wallmounted = false;
}

void ContentFeatures::serialize(std::ostream &os)
{
	writeU8(os, 1); // version
	os<<serializeString(name);
	writeU8(os, drawtype);
	writeF1000(os, visual_scale);
	writeU8(os, 6);
	for(u32 i=0; i<6; i++)
		os<<serializeString(tname_tiles[i]);
	writeU8(os, CF_SPECIAL_COUNT);
	for(u32 i=0; i<CF_SPECIAL_COUNT; i++){
		mspec_special[i].serialize(os);
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
	os<<serializeString(metadata_name);
	writeU8(os, liquid_type);
	os<<serializeString(liquid_alternative_flowing);
	os<<serializeString(liquid_alternative_source);
	writeU8(os, liquid_viscosity);
	writeU8(os, light_source);
	writeU32(os, damage_per_second);
	selection_box.serialize(os);
	material.serialize(os);
	writeU8(os, legacy_facedir_simple);
	writeU8(os, legacy_wallmounted);
}

void ContentFeatures::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 1)
		throw SerializationError("unsupported ContentFeatures version");
	name = deSerializeString(is);
	drawtype = (enum NodeDrawType)readU8(is);
	visual_scale = readF1000(is);
	if(readU8(is) != 6)
		throw SerializationError("unsupported tile count");
	for(u32 i=0; i<6; i++)
		tname_tiles[i] = deSerializeString(is);
	if(readU8(is) != CF_SPECIAL_COUNT)
		throw SerializationError("unsupported CF_SPECIAL_COUNT");
	for(u32 i=0; i<CF_SPECIAL_COUNT; i++){
		mspec_special[i].deSerialize(is);
	}
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
	metadata_name = deSerializeString(is);
	liquid_type = (enum LiquidType)readU8(is);
	liquid_alternative_flowing = deSerializeString(is);
	liquid_alternative_source = deSerializeString(is);
	liquid_viscosity = readU8(is);
	light_source = readU8(is);
	damage_per_second = readU32(is);
	selection_box.deSerialize(is);
	material.deSerialize(is);
	legacy_facedir_simple = readU8(is);
	legacy_wallmounted = readU8(is);
}

/*
	CNodeDefManager
*/

class CNodeDefManager: public IWritableNodeDefManager
{
public:
	void clear()
	{
		m_name_id_mapping.clear();
		m_name_id_mapping_with_aliases.clear();

		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			ContentFeatures &f = m_content_features[i];
			f.reset(); // Reset to defaults
		}
		
		// Set CONTENT_AIR
		{
			ContentFeatures f;
			f.name = "air";
			f.drawtype = NDT_AIRLIKE;
			f.param_type = CPT_LIGHT;
			f.light_propagates = true;
			f.sunlight_propagates = true;
			f.walkable = false;
			f.pointable = false;
			f.diggable = false;
			f.buildable_to = true;
			// Insert directly into containers
			content_t c = CONTENT_AIR;
			m_content_features[c] = f;
			addNameIdMapping(c, f.name);
		}
		// Set CONTENT_IGNORE
		{
			ContentFeatures f;
			f.name = "ignore";
			f.drawtype = NDT_AIRLIKE;
			f.param_type = CPT_NONE;
			f.light_propagates = false;
			f.sunlight_propagates = false;
			f.walkable = false;
			f.pointable = false;
			f.diggable = false;
			// A way to remove accidental CONTENT_IGNOREs
			f.buildable_to = true;
			// Insert directly into containers
			content_t c = CONTENT_IGNORE;
			m_content_features[c] = f;
			addNameIdMapping(c, f.name);
		}
	}
	// CONTENT_IGNORE = not found
	content_t getFreeId(bool require_full_param2)
	{
		// If allowed, first search in the large 4-bit-param2 pool
		if(!require_full_param2){
			for(u16 i=0x800; i<=0xfff; i++){
				const ContentFeatures &f = m_content_features[i];
				if(f.name == "")
					return i;
			}
		}
		// Then search from the small 8-bit-param2 pool
		for(u16 i=0; i<=125; i++){
			const ContentFeatures &f = m_content_features[i];
			if(f.name == "")
				return i;
		}
		return CONTENT_IGNORE;
	}
	CNodeDefManager()
	{
		clear();
	}
	virtual ~CNodeDefManager()
	{
	}
	virtual IWritableNodeDefManager* clone()
	{
		CNodeDefManager *mgr = new CNodeDefManager();
		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			mgr->set(i, get(i));
		}
		return mgr;
	}
	virtual const ContentFeatures& get(content_t c) const
	{
		assert(c <= MAX_CONTENT);
		return m_content_features[c];
	}
	virtual const ContentFeatures& get(const MapNode &n) const
	{
		return get(n.getContent());
	}
	virtual bool getId(const std::string &name, content_t &result) const
	{
		std::map<std::string, content_t>::const_iterator
			i = m_name_id_mapping_with_aliases.find(name);
		if(i == m_name_id_mapping_with_aliases.end())
			return false;
		result = i->second;
		return true;
	}
	virtual content_t getId(const std::string &name) const
	{
		content_t id = CONTENT_IGNORE;
		getId(name, id);
		return id;
	}
	virtual const ContentFeatures& get(const std::string &name) const
	{
		content_t id = CONTENT_IGNORE;
		getId(name, id);
		return get(id);
	}
	// IWritableNodeDefManager
	virtual void set(content_t c, const ContentFeatures &def)
	{
		infostream<<"registerNode: registering content id \""<<c
				<<"\": name=\""<<def.name<<"\""<<std::endl;
		assert(c <= MAX_CONTENT);
		// Don't allow redefining CONTENT_IGNORE (but allow air)
		if(def.name == "ignore" || c == CONTENT_IGNORE){
			infostream<<"registerNode: WARNING: Ignoring "
					<<"CONTENT_IGNORE redefinition"<<std::endl;
			return;
		}
		// Check that the special contents are not redefined as different id
		// because it would mess up everything
		if((def.name == "ignore" && c != CONTENT_IGNORE) ||
			(def.name == "air" && c != CONTENT_AIR)){
			errorstream<<"registerNode: IGNORING ERROR: "
					<<"trying to register built-in type \""
					<<def.name<<"\" as different id"<<std::endl;
			return;
		}
		m_content_features[c] = def;
		if(def.name != "")
			addNameIdMapping(c, def.name);
	}
	virtual content_t set(const std::string &name,
			const ContentFeatures &def)
	{
		assert(name == def.name);
		u16 id = CONTENT_IGNORE;
		bool found = m_name_id_mapping.getId(name, id);  // ignore aliases
		if(!found){
			// Determine if full param2 is required
			bool require_full_param2 = (
				def.param_type_2 == CPT2_FULL
				||
				def.param_type_2 == CPT2_FLOWINGLIQUID
				||
				def.legacy_wallmounted
			);
			// Get some id
			id = getFreeId(require_full_param2);
			if(id == CONTENT_IGNORE)
				return CONTENT_IGNORE;
			if(name != "")
				addNameIdMapping(id, name);
		}
		set(id, def);
		return id;
	}
	virtual content_t allocateDummy(const std::string &name)
	{
		assert(name != "");
		ContentFeatures f;
		f.name = name;
		// Make unknown blocks diggable
		f.material.diggability = DIGGABLE_CONSTANT;
		f.material.constant_time = 0.5;
		return set(name, f);
	}
	virtual void updateAliases(IItemDefManager *idef)
	{
		std::set<std::string> all = idef->getAll();
		m_name_id_mapping_with_aliases.clear();
		for(std::set<std::string>::iterator
				i = all.begin(); i != all.end(); i++)
		{
			std::string name = *i;
			std::string convert_to = idef->getAlias(name);
			content_t id;
			if(m_name_id_mapping.getId(convert_to, id))
			{
				m_name_id_mapping_with_aliases.insert(
						std::make_pair(name, id));
			}
		}
	}
	virtual void updateTextures(ITextureSource *tsrc)
	{
#ifndef SERVER
		infostream<<"CNodeDefManager::updateTextures(): Updating "
				<<"textures in node definitions"<<std::endl;

		bool new_style_water = g_settings->getBool("new_style_water");
		bool new_style_leaves = g_settings->getBool("new_style_leaves");
		bool opaque_water = g_settings->getBool("opaque_water");
		
		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			ContentFeatures *f = &m_content_features[i];

			std::string tname_tiles[6];
			for(u32 j=0; j<6; j++)
			{
				tname_tiles[j] = f->tname_tiles[j];
				if(tname_tiles[j] == "")
					tname_tiles[j] = "unknown_block.png";
                        }

			switch(f->drawtype){
			default:
			case NDT_NORMAL:
				f->solidness = 2;
				break;
			case NDT_AIRLIKE:
				f->solidness = 0;
				break;
			case NDT_LIQUID:
				assert(f->liquid_type == LIQUID_SOURCE);
				if(opaque_water)
					f->alpha = 255;
				if(new_style_water){
					f->solidness = 0;
				} else {
					f->solidness = 1;
					if(f->alpha == 255)
						f->solidness = 2;
					f->backface_culling = false;
				}
				break;
			case NDT_FLOWINGLIQUID:
				assert(f->liquid_type == LIQUID_FLOWING);
				f->solidness = 0;
				if(opaque_water)
					f->alpha = 255;
				break;
			case NDT_GLASSLIKE:
				f->solidness = 0;
				f->visual_solidness = 1;
				break;
			case NDT_ALLFACES:
				f->solidness = 0;
				f->visual_solidness = 1;
				break;
			case NDT_ALLFACES_OPTIONAL:
				if(new_style_leaves){
					f->drawtype = NDT_ALLFACES;
					f->solidness = 0;
					f->visual_solidness = 1;
				} else {
					f->drawtype = NDT_NORMAL;
					f->solidness = 2;
					for(u32 i=0; i<6; i++){
						tname_tiles[i] += std::string("^[noalpha");
					}
				}
				break;
			case NDT_TORCHLIKE:
			case NDT_SIGNLIKE:
			case NDT_PLANTLIKE:
			case NDT_FENCELIKE:
			case NDT_RAILLIKE:
				f->solidness = 0;
				break;
			}

			// Tile textures
			for(u16 j=0; j<6; j++){
				f->tiles[j].texture = tsrc->getTexture(tname_tiles[j]);
				f->tiles[j].alpha = f->alpha;
				if(f->alpha == 255)
					f->tiles[j].material_type = MATERIAL_ALPHA_SIMPLE;
				else
					f->tiles[j].material_type = MATERIAL_ALPHA_VERTEX;
				if(f->backface_culling)
					f->tiles[j].material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
				else
					f->tiles[j].material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
			}
			// Special textures
			for(u16 j=0; j<CF_SPECIAL_COUNT; j++){
				// Remove all stuff
				if(f->special_aps[j]){
					delete f->special_aps[j];
					f->special_aps[j] = NULL;
				}
				if(f->special_materials[j]){
					delete f->special_materials[j];
					f->special_materials[j] = NULL;
				}
				// Skip if should not exist
				if(f->mspec_special[j].tname == "")
					continue;
				// Create all stuff
				f->special_aps[j] = new AtlasPointer(
						tsrc->getTexture(f->mspec_special[j].tname));
				f->special_materials[j] = new video::SMaterial;
				f->special_materials[j]->setFlag(video::EMF_LIGHTING, false);
				f->special_materials[j]->setFlag(video::EMF_BACK_FACE_CULLING,
						f->mspec_special[j].backface_culling);
				f->special_materials[j]->setFlag(video::EMF_BILINEAR_FILTER, false);
				f->special_materials[j]->setFlag(video::EMF_FOG_ENABLE, true);
				f->special_materials[j]->setTexture(0, f->special_aps[j]->atlas);
				if(f->alpha != 255)
					f->special_materials[j]->MaterialType =
							video::EMT_TRANSPARENT_VERTEX_ALPHA;
			}
		}
#endif
	}
	void serialize(std::ostream &os)
	{
		u16 count = 0;
		std::ostringstream tmp_os(std::ios::binary);
		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			if(i == CONTENT_IGNORE || i == CONTENT_AIR)
				continue;
			ContentFeatures *f = &m_content_features[i];
			if(f->name == "")
				continue;
			writeU16(tmp_os, i);
			f->serialize(tmp_os);
			count++;
		}
		writeU16(os, count);
		os<<serializeLongString(tmp_os.str());
	}
	void deSerialize(std::istream &is)
	{
		clear();
		u16 count = readU16(is);
		std::istringstream tmp_is(deSerializeLongString(is), std::ios::binary);
		for(u16 n=0; n<count; n++){
			u16 i = readU16(tmp_is);
			if(i > MAX_CONTENT){
				errorstream<<"ContentFeatures::deSerialize(): "
						<<"Too large content id: "<<i<<std::endl;
				continue;
			}
			/*// Do not deserialize special types
			if(i == CONTENT_IGNORE || i == CONTENT_AIR)
				continue;*/
			ContentFeatures *f = &m_content_features[i];
			f->deSerialize(tmp_is);
			if(f->name != "")
				addNameIdMapping(i, f->name);
		}
	}
private:
	void addNameIdMapping(content_t i, std::string name)
	{
		m_name_id_mapping.set(i, name);
		m_name_id_mapping_with_aliases.insert(std::make_pair(name, i));
	}
private:
	// Features indexed by id
	ContentFeatures m_content_features[MAX_CONTENT+1];
	// A mapping for fast converting back and forth between names and ids
	NameIdMapping m_name_id_mapping;
	// Like m_name_id_mapping, but only from names to ids, and includes
	// item aliases too. Updated by updateAliases()
	// Note: Not serialized.
	std::map<std::string, content_t> m_name_id_mapping_with_aliases;
};

IWritableNodeDefManager* createNodeDefManager()
{
	return new CNodeDefManager();
}

