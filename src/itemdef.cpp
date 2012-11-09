/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2011 Kahrl <kahrl@gmx.net>

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

#include "itemdef.h"

#include "gamedef.h"
#include "nodedef.h"
#include "tool.h"
#include "inventory.h"
#ifndef SERVER
#include "mapblock_mesh.h"
#include "mesh.h"
#include "tile.h"
#endif
#include "log.h"
#include "util/serialize.h"
#include <map>
#include <set>

/*
	ItemDefinition
*/
ItemDefinition::ItemDefinition()
{
	resetInitial();
}

ItemDefinition::ItemDefinition(const ItemDefinition &def)
{
	resetInitial();
	*this = def;
}

ItemDefinition& ItemDefinition::operator=(const ItemDefinition &def)
{
	if(this == &def)
		return *this;

	reset();

	type = def.type;
	name = def.name;
	description = def.description;
	inventory_image = def.inventory_image;
	wield_image = def.wield_image;
	wield_scale = def.wield_scale;
	stack_max = def.stack_max;
	usable = def.usable;
	liquids_pointable = def.liquids_pointable;
	if(def.tool_capabilities)
	{
		tool_capabilities = new ToolCapabilities(
				*def.tool_capabilities);
	}
	groups = def.groups;
	node_placement_prediction = def.node_placement_prediction;
#ifndef SERVER
	inventory_texture = def.inventory_texture;
	if(def.wield_mesh)
	{
		wield_mesh = def.wield_mesh;
		wield_mesh->grab();
	}
#endif
	return *this;
}

ItemDefinition::~ItemDefinition()
{
	reset();
}

void ItemDefinition::resetInitial()
{
	// Initialize pointers to NULL so reset() does not delete undefined pointers
	tool_capabilities = NULL;
#ifndef SERVER
	inventory_texture = NULL;
	wield_mesh = NULL;
#endif
	reset();
}

void ItemDefinition::reset()
{
	type = ITEM_NONE;
	name = "";
	description = "";
	inventory_image = "";
	wield_image = "";
	wield_scale = v3f(1.0, 1.0, 1.0);
	stack_max = 99;
	usable = false;
	liquids_pointable = false;
	if(tool_capabilities)
	{
		delete tool_capabilities;
		tool_capabilities = NULL;
	}
	groups.clear();

	node_placement_prediction = "";
	
#ifndef SERVER
	inventory_texture = NULL;
	if(wield_mesh)
	{
		wield_mesh->drop();
		wield_mesh = NULL;
	}
#endif
}

void ItemDefinition::serialize(std::ostream &os) const
{
	writeU8(os, 1); // version
	writeU8(os, type);
	os<<serializeString(name);
	os<<serializeString(description);
	os<<serializeString(inventory_image);
	os<<serializeString(wield_image);
	writeV3F1000(os, wield_scale);
	writeS16(os, stack_max);
	writeU8(os, usable);
	writeU8(os, liquids_pointable);
	std::string tool_capabilities_s = "";
	if(tool_capabilities){
		std::ostringstream tmp_os(std::ios::binary);
		tool_capabilities->serialize(tmp_os);
		tool_capabilities_s = tmp_os.str();
	}
	os<<serializeString(tool_capabilities_s);
	writeU16(os, groups.size());
	for(std::map<std::string, int>::const_iterator
			i = groups.begin(); i != groups.end(); i++){
		os<<serializeString(i->first);
		writeS16(os, i->second);
	}
	os<<serializeString(node_placement_prediction);
}

void ItemDefinition::deSerialize(std::istream &is)
{
	// Reset everything
	reset();

	// Deserialize
	int version = readU8(is);
	if(version != 1)
		throw SerializationError("unsupported ItemDefinition version");
	type = (enum ItemType)readU8(is);
	name = deSerializeString(is);
	description = deSerializeString(is);
	inventory_image = deSerializeString(is);
	wield_image = deSerializeString(is);
	wield_scale = readV3F1000(is);
	stack_max = readS16(is);
	usable = readU8(is);
	liquids_pointable = readU8(is);
	std::string tool_capabilities_s = deSerializeString(is);
	if(!tool_capabilities_s.empty())
	{
		std::istringstream tmp_is(tool_capabilities_s, std::ios::binary);
		tool_capabilities = new ToolCapabilities;
		tool_capabilities->deSerialize(tmp_is);
	}
	groups.clear();
	u32 groups_size = readU16(is);
	for(u32 i=0; i<groups_size; i++){
		std::string name = deSerializeString(is);
		int value = readS16(is);
		groups[name] = value;
	}
	// If you add anything here, insert it primarily inside the try-catch
	// block to not need to increase the version.
	try{
		node_placement_prediction = deSerializeString(is);
	}catch(SerializationError &e) {};
}

/*
	CItemDefManager
*/

// SUGG: Support chains of aliases?

class CItemDefManager: public IWritableItemDefManager
{
public:
	CItemDefManager()
	{
		clear();
	}
	virtual ~CItemDefManager()
	{
	}
	virtual const ItemDefinition& get(const std::string &name_) const
	{
		// Convert name according to possible alias
		std::string name = getAlias(name_);
		// Get the definition
		std::map<std::string, ItemDefinition*>::const_iterator i;
		i = m_item_definitions.find(name);
		if(i == m_item_definitions.end())
			i = m_item_definitions.find("unknown");
		assert(i != m_item_definitions.end());
		return *(i->second);
	}
	virtual std::string getAlias(const std::string &name) const
	{
		std::map<std::string, std::string>::const_iterator i;
		i = m_aliases.find(name);
		if(i != m_aliases.end())
			return i->second;
		return name;
	}
	virtual std::set<std::string> getAll() const
	{
		std::set<std::string> result;
		for(std::map<std::string, ItemDefinition*>::const_iterator
				i = m_item_definitions.begin();
				i != m_item_definitions.end(); i++)
		{
			result.insert(i->first);
		}
		for(std::map<std::string, std::string>::const_iterator
				i = m_aliases.begin();
				i != m_aliases.end(); i++)
		{
			result.insert(i->first);
		}
		return result;
	}
	virtual bool isKnown(const std::string &name_) const
	{
		// Convert name according to possible alias
		std::string name = getAlias(name_);
		// Get the definition
		std::map<std::string, ItemDefinition*>::const_iterator i;
		return m_item_definitions.find(name) != m_item_definitions.end();
	}
	void clear()
	{
		for(std::map<std::string, ItemDefinition*>::const_iterator
				i = m_item_definitions.begin();
				i != m_item_definitions.end(); i++)
		{
			delete i->second;
		}
		m_item_definitions.clear();
		m_aliases.clear();

		// Add the four builtin items:
		//   "" is the hand
		//   "unknown" is returned whenever an undefined item is accessed
		//   "air" is the air node
		//   "ignore" is the ignore node

		ItemDefinition* hand_def = new ItemDefinition;
		hand_def->name = "";
		hand_def->wield_image = "wieldhand.png";
		hand_def->tool_capabilities = new ToolCapabilities;
		m_item_definitions.insert(std::make_pair("", hand_def));

		ItemDefinition* unknown_def = new ItemDefinition;
		unknown_def->name = "unknown";
		m_item_definitions.insert(std::make_pair("unknown", unknown_def));

		ItemDefinition* air_def = new ItemDefinition;
		air_def->type = ITEM_NODE;
		air_def->name = "air";
		m_item_definitions.insert(std::make_pair("air", air_def));

		ItemDefinition* ignore_def = new ItemDefinition;
		ignore_def->type = ITEM_NODE;
		ignore_def->name = "ignore";
		m_item_definitions.insert(std::make_pair("ignore", ignore_def));
	}
	virtual void registerItem(const ItemDefinition &def)
	{
		verbosestream<<"ItemDefManager: registering \""<<def.name<<"\""<<std::endl;
		// Ensure that the "" item (the hand) always has ToolCapabilities
		if(def.name == "")
			assert(def.tool_capabilities != NULL);
		
		if(m_item_definitions.count(def.name) == 0)
			m_item_definitions[def.name] = new ItemDefinition(def);
		else
			*(m_item_definitions[def.name]) = def;

		// Remove conflicting alias if it exists
		bool alias_removed = (m_aliases.erase(def.name) != 0);
		if(alias_removed)
			infostream<<"ItemDefManager: erased alias "<<def.name
					<<" because item was defined"<<std::endl;
	}
	virtual void registerAlias(const std::string &name,
			const std::string &convert_to)
	{
		if(m_item_definitions.find(name) == m_item_definitions.end())
		{
			verbosestream<<"ItemDefManager: setting alias "<<name
				<<" -> "<<convert_to<<std::endl;
			m_aliases[name] = convert_to;
		}
	}

	virtual void updateTexturesAndMeshes(IGameDef *gamedef)
	{
#ifndef SERVER
		infostream<<"ItemDefManager::updateTexturesAndMeshes(): Updating "
				<<"textures and meshes in item definitions"<<std::endl;

		ITextureSource *tsrc = gamedef->getTextureSource();
		INodeDefManager *nodedef = gamedef->getNodeDefManager();
		IrrlichtDevice *device = tsrc->getDevice();
		video::IVideoDriver *driver = device->getVideoDriver();

		for(std::map<std::string, ItemDefinition*>::iterator
				i = m_item_definitions.begin();
				i != m_item_definitions.end(); i++)
		{
			ItemDefinition *def = i->second;

			bool need_node_mesh = false;

			// Create an inventory texture
			def->inventory_texture = NULL;
			if(def->inventory_image != "")
			{
				def->inventory_texture = tsrc->getTextureRaw(def->inventory_image);
			}
			else if(def->type == ITEM_NODE)
			{
				need_node_mesh = true;
			}

			// Create a wield mesh
			if(def->wield_mesh != NULL)
			{
				def->wield_mesh->drop();
				def->wield_mesh = NULL;
			}
			if(def->type == ITEM_NODE && def->wield_image == "")
			{
				need_node_mesh = true;
			}
			else if(def->wield_image != "" || def->inventory_image != "")
			{
				// Extrude the wield image into a mesh

				std::string imagename;
				if(def->wield_image != "")
					imagename = def->wield_image;
				else
					imagename = def->inventory_image;

				def->wield_mesh = createExtrudedMesh(
						tsrc->getTextureRaw(imagename),
						driver,
						def->wield_scale * v3f(40.0, 40.0, 4.0));
				if(def->wield_mesh == NULL)
				{
					infostream<<"ItemDefManager: WARNING: "
						<<"updateTexturesAndMeshes(): "
						<<"Unable to create extruded mesh for item "
						<<def->name<<std::endl;
				}
			}

			if(need_node_mesh)
			{
				/*
					Get node properties
				*/
				content_t id = nodedef->getId(def->name);
				const ContentFeatures &f = nodedef->get(id);

				u8 param1 = 0;
				if(f.param_type == CPT_LIGHT)
					param1 = 0xee;

				/*
				 	Make a mesh from the node
				*/
				MeshMakeData mesh_make_data(gamedef);
				MapNode mesh_make_node(id, param1, 0);
				mesh_make_data.fillSingleNode(&mesh_make_node);
				MapBlockMesh mapblock_mesh(&mesh_make_data);

				scene::IMesh *node_mesh = mapblock_mesh.getMesh();
				assert(node_mesh);
				setMeshColor(node_mesh, video::SColor(255, 255, 255, 255));

				/*
					Scale and translate the mesh so it's a unit cube
					centered on the origin
				*/
				scaleMesh(node_mesh, v3f(1.0/BS, 1.0/BS, 1.0/BS));
				translateMesh(node_mesh, v3f(-1.0, -1.0, -1.0));

				/*
					Draw node mesh into a render target texture
				*/
				if(def->inventory_texture == NULL)
				{
					core::dimension2d<u32> dim(64,64);
					std::string rtt_texture_name = "INVENTORY_"
						+ def->name + "_RTT";
					v3f camera_position(0, 1.0, -1.5);
					camera_position.rotateXZBy(45);
					v3f camera_lookat(0, 0, 0);
					core::CMatrix4<f32> camera_projection_matrix;
					// Set orthogonal projection
					camera_projection_matrix.buildProjectionMatrixOrthoLH(
							1.65, 1.65, 0, 100);

					video::SColorf ambient_light(0.2,0.2,0.2);
					v3f light_position(10, 100, -50);
					video::SColorf light_color(0.5,0.5,0.5);
					f32 light_radius = 1000;

					def->inventory_texture = generateTextureFromMesh(
						node_mesh, device, dim, rtt_texture_name,
						camera_position,
						camera_lookat,
						camera_projection_matrix,
						ambient_light,
						light_position,
						light_color,
						light_radius);

					// render-to-target didn't work
					if(def->inventory_texture == NULL)
					{
						def->inventory_texture =
							tsrc->getTextureRaw(f.tiledef[0].name);
					}
				}

				/*
					Use the node mesh as the wield mesh
				*/
				if(def->wield_mesh == NULL)
				{
					// Scale to proper wield mesh proportions
					scaleMesh(node_mesh, v3f(30.0, 30.0, 30.0)
							* def->wield_scale);
					def->wield_mesh = node_mesh;
					def->wield_mesh->grab();
				}

				// falling outside of here deletes node_mesh
			}
		}
#endif
	}
	void serialize(std::ostream &os)
	{
		writeU8(os, 0); // version
		u16 count = m_item_definitions.size();
		writeU16(os, count);
		for(std::map<std::string, ItemDefinition*>::const_iterator
				i = m_item_definitions.begin();
				i != m_item_definitions.end(); i++)
		{
			ItemDefinition *def = i->second;
			// Serialize ItemDefinition and write wrapped in a string
			std::ostringstream tmp_os(std::ios::binary);
			def->serialize(tmp_os);
			os<<serializeString(tmp_os.str());
		}
		writeU16(os, m_aliases.size());
		for(std::map<std::string, std::string>::const_iterator
			i = m_aliases.begin(); i != m_aliases.end(); i++)
		{
			os<<serializeString(i->first);
			os<<serializeString(i->second);
		}
	}
	void deSerialize(std::istream &is)
	{
		// Clear everything
		clear();
		// Deserialize
		int version = readU8(is);
		if(version != 0)
			throw SerializationError("unsupported ItemDefManager version");
		u16 count = readU16(is);
		for(u16 i=0; i<count; i++)
		{
			// Deserialize a string and grab an ItemDefinition from it
			std::istringstream tmp_is(deSerializeString(is), std::ios::binary);
			ItemDefinition def;
			def.deSerialize(tmp_is);
			// Register
			registerItem(def);
		}
		u16 num_aliases = readU16(is);
		for(u16 i=0; i<num_aliases; i++)
		{
			std::string name = deSerializeString(is);
			std::string convert_to = deSerializeString(is);
			registerAlias(name, convert_to);
		}
	}
private:
	// Key is name
	std::map<std::string, ItemDefinition*> m_item_definitions;
	// Aliases
	std::map<std::string, std::string> m_aliases;
};

IWritableItemDefManager* createItemDefManager()
{
	return new CItemDefManager();
}

