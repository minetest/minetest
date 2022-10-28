/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>

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

#include "nodedef.h"
#include "tool.h"
#include "inventory.h"
#ifndef SERVER
#include "client/mapblock_mesh.h"
#include "client/mesh.h"
#include "client/wieldmesh.h"
#include "client/tile.h"
#include "client/client.h"
#endif
#include "log.h"
#include "settings.h"
#include "util/serialize.h"
#include "util/container.h"
#include "util/thread.h"
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
	short_description = def.short_description;
	inventory_image = def.inventory_image;
	inventory_overlay = def.inventory_overlay;
	wield_image = def.wield_image;
	wield_overlay = def.wield_overlay;
	wield_scale = def.wield_scale;
	stack_max = def.stack_max;
	usable = def.usable;
	liquids_pointable = def.liquids_pointable;
	if (def.tool_capabilities)
		tool_capabilities = new ToolCapabilities(*def.tool_capabilities);
	groups = def.groups;
	node_placement_prediction = def.node_placement_prediction;
	place_param2 = def.place_param2;
	sound_place = def.sound_place;
	sound_place_failed = def.sound_place_failed;
	sound_use = def.sound_use;
	sound_use_air = def.sound_use_air;
	range = def.range;
	palette_image = def.palette_image;
	color = def.color;
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
	reset();
}

void ItemDefinition::reset()
{
	type = ITEM_NONE;
	name.clear();
	description.clear();
	short_description.clear();
	inventory_image.clear();
	inventory_overlay.clear();
	wield_image.clear();
	wield_overlay.clear();
	palette_image.clear();
	color = video::SColor(0xFFFFFFFF);
	wield_scale = v3f(1.0, 1.0, 1.0);
	stack_max = 99;
	usable = false;
	liquids_pointable = false;
	delete tool_capabilities;
	tool_capabilities = NULL;
	groups.clear();
	sound_place = SimpleSoundSpec();
	sound_place_failed = SimpleSoundSpec();
	sound_use = SimpleSoundSpec();
	sound_use_air = SimpleSoundSpec();
	range = -1;
	node_placement_prediction.clear();
	place_param2 = 0;
}

void ItemDefinition::serialize(std::ostream &os, u16 protocol_version) const
{
	// protocol_version >= 37
	u8 version = 6;
	writeU8(os, version);
	writeU8(os, type);
	os << serializeString16(name);
	os << serializeString16(description);
	os << serializeString16(inventory_image);
	os << serializeString16(wield_image);
	writeV3F32(os, wield_scale);
	writeS16(os, stack_max);
	writeU8(os, usable);
	writeU8(os, liquids_pointable);

	std::string tool_capabilities_s;
	if (tool_capabilities) {
		std::ostringstream tmp_os(std::ios::binary);
		tool_capabilities->serialize(tmp_os, protocol_version);
		tool_capabilities_s = tmp_os.str();
	}
	os << serializeString16(tool_capabilities_s);

	writeU16(os, groups.size());
	for (const auto &group : groups) {
		os << serializeString16(group.first);
		writeS16(os, group.second);
	}

	os << serializeString16(node_placement_prediction);

	// Version from ContentFeatures::serialize to keep in sync
	sound_place.serialize(os, protocol_version);
	sound_place_failed.serialize(os, protocol_version);

	writeF32(os, range);
	os << serializeString16(palette_image);
	writeARGB8(os, color);
	os << serializeString16(inventory_overlay);
	os << serializeString16(wield_overlay);

	os << serializeString16(short_description);

	os << place_param2;

	sound_use.serialize(os, protocol_version);
	sound_use_air.serialize(os, protocol_version);
}

void ItemDefinition::deSerialize(std::istream &is, u16 protocol_version)
{
	// Reset everything
	reset();

	// Deserialize
	int version = readU8(is);
	if (version < 6)
		throw SerializationError("unsupported ItemDefinition version");

	type = (enum ItemType)readU8(is);
	name = deSerializeString16(is);
	description = deSerializeString16(is);
	inventory_image = deSerializeString16(is);
	wield_image = deSerializeString16(is);
	wield_scale = readV3F32(is);
	stack_max = readS16(is);
	usable = readU8(is);
	liquids_pointable = readU8(is);

	std::string tool_capabilities_s = deSerializeString16(is);
	if (!tool_capabilities_s.empty()) {
		std::istringstream tmp_is(tool_capabilities_s, std::ios::binary);
		tool_capabilities = new ToolCapabilities;
		tool_capabilities->deSerialize(tmp_is);
	}

	groups.clear();
	u32 groups_size = readU16(is);
	for(u32 i=0; i<groups_size; i++){
		std::string name = deSerializeString16(is);
		int value = readS16(is);
		groups[name] = value;
	}

	node_placement_prediction = deSerializeString16(is);

	sound_place.deSerialize(is, protocol_version);
	sound_place_failed.deSerialize(is, protocol_version);

	range = readF32(is);
	palette_image = deSerializeString16(is);
	color = readARGB8(is);
	inventory_overlay = deSerializeString16(is);
	wield_overlay = deSerializeString16(is);

	// If you add anything here, insert it inside the try-catch
	// block to not need to increase the version.
	try {
		short_description = deSerializeString16(is);

		place_param2 = readU8(is); // 0 if missing

		sound_use.deSerialize(is, protocol_version);
		sound_use_air.deSerialize(is, protocol_version);
	} catch(SerializationError &e) {};
}


/*
	CItemDefManager
*/

// SUGG: Support chains of aliases?

class CItemDefManager: public IWritableItemDefManager
{
#ifndef SERVER
	struct ClientCached
	{
		video::ITexture *inventory_texture;
		ItemMesh wield_mesh;
		Palette *palette;

		ClientCached():
			inventory_texture(NULL),
			palette(NULL)
		{}
	};
#endif

public:
	CItemDefManager()
	{

#ifndef SERVER
		m_main_thread = std::this_thread::get_id();
#endif
		clear();
	}
	virtual ~CItemDefManager()
	{
#ifndef SERVER
		const std::vector<ClientCached*> &values = m_clientcached.getValues();
		for (ClientCached *cc : values) {
			if (cc->wield_mesh.mesh)
				cc->wield_mesh.mesh->drop();
			delete cc;
		}

#endif
		for (auto &item_definition : m_item_definitions) {
			delete item_definition.second;
		}
		m_item_definitions.clear();
	}
	virtual const ItemDefinition& get(const std::string &name_) const
	{
		// Convert name according to possible alias
		std::string name = getAlias(name_);
		// Get the definition
		auto i = m_item_definitions.find(name);
		if (i == m_item_definitions.cend())
			i = m_item_definitions.find("unknown");
		assert(i != m_item_definitions.cend());
		return *(i->second);
	}
	virtual const std::string &getAlias(const std::string &name) const
	{
		auto it = m_aliases.find(name);
		if (it != m_aliases.cend())
			return it->second;
		return name;
	}
	virtual void getAll(std::set<std::string> &result) const
	{
		result.clear();
		for (const auto &item_definition : m_item_definitions) {
			result.insert(item_definition.first);
		}

		for (const auto &alias : m_aliases) {
			result.insert(alias.first);
		}
	}
	virtual bool isKnown(const std::string &name_) const
	{
		// Convert name according to possible alias
		std::string name = getAlias(name_);
		// Get the definition
		return m_item_definitions.find(name) != m_item_definitions.cend();
	}
#ifndef SERVER
public:
	ClientCached* createClientCachedDirect(const std::string &name,
			Client *client) const
	{
		infostream<<"Lazily creating item texture and mesh for \""
				<<name<<"\""<<std::endl;

		// This is not thread-safe
		sanity_check(std::this_thread::get_id() == m_main_thread);

		// Skip if already in cache
		ClientCached *cc = NULL;
		m_clientcached.get(name, &cc);
		if(cc)
			return cc;

		ITextureSource *tsrc = client->getTextureSource();
		const ItemDefinition &def = get(name);

		// Create new ClientCached
		cc = new ClientCached();

		// Create an inventory texture
		cc->inventory_texture = NULL;
		if (!def.inventory_image.empty())
			cc->inventory_texture = tsrc->getTexture(def.inventory_image);

		ItemStack item = ItemStack();
		item.name = def.name;

		getItemMesh(client, item, &(cc->wield_mesh));

		cc->palette = tsrc->getPalette(def.palette_image);

		// Put in cache
		m_clientcached.set(name, cc);

		return cc;
	}
	ClientCached* getClientCached(const std::string &name,
			Client *client) const
	{
		ClientCached *cc = NULL;
		m_clientcached.get(name, &cc);
		if (cc)
			return cc;

		if (std::this_thread::get_id() == m_main_thread) {
			return createClientCachedDirect(name, client);
		}

		// We're gonna ask the result to be put into here
		static ResultQueue<std::string, ClientCached*, u8, u8> result_queue;

		// Throw a request in
		m_get_clientcached_queue.add(name, 0, 0, &result_queue);
		try {
			while(true) {
				// Wait result for a second
				GetResult<std::string, ClientCached*, u8, u8>
					result = result_queue.pop_front(1000);

				if (result.key == name) {
					return result.item;
				}
			}
		} catch(ItemNotFoundException &e) {
			errorstream << "Waiting for clientcached " << name
				<< " timed out." << std::endl;
			return &m_dummy_clientcached;
		}
	}
	// Get item inventory texture
	virtual video::ITexture* getInventoryTexture(const std::string &name,
			Client *client) const
	{
		ClientCached *cc = getClientCached(name, client);
		if(!cc)
			return NULL;
		return cc->inventory_texture;
	}
	// Get item wield mesh
	virtual ItemMesh* getWieldMesh(const std::string &name,
			Client *client) const
	{
		ClientCached *cc = getClientCached(name, client);
		if(!cc)
			return NULL;
		return &(cc->wield_mesh);
	}

	// Get item palette
	virtual Palette* getPalette(const std::string &name,
			Client *client) const
	{
		ClientCached *cc = getClientCached(name, client);
		if(!cc)
			return NULL;
		return cc->palette;
	}

	virtual video::SColor getItemstackColor(const ItemStack &stack,
		Client *client) const
	{
		// Look for direct color definition
		const std::string &colorstring = stack.metadata.getString("color", 0);
		video::SColor directcolor;
		if (!colorstring.empty() && parseColorString(colorstring, directcolor, true))
			return directcolor;
		// See if there is a palette
		Palette *palette = getPalette(stack.name, client);
		const std::string &index = stack.metadata.getString("palette_index", 0);
		if (palette && !index.empty())
			return (*palette)[mystoi(index, 0, 255)];
		// Fallback color
		return get(stack.name).color;
	}
#endif
	void applyTextureOverrides(const std::vector<TextureOverride> &overrides)
	{
		infostream << "ItemDefManager::applyTextureOverrides(): Applying "
			"overrides to textures" << std::endl;

		for (const TextureOverride& texture_override : overrides) {
			if (m_item_definitions.find(texture_override.id) == m_item_definitions.end()) {
				continue; // Ignore unknown item
			}

			ItemDefinition* itemdef = m_item_definitions[texture_override.id];

			if (texture_override.hasTarget(OverrideTarget::INVENTORY))
				itemdef->inventory_image = texture_override.texture;

			if (texture_override.hasTarget(OverrideTarget::WIELD))
				itemdef->wield_image = texture_override.texture;
		}
	}
	void clear()
	{
		for (auto &i : m_item_definitions)
		{
			delete i.second;
		}
		m_item_definitions.clear();
		m_aliases.clear();

		// Add the four builtin items:
		//   "" is the hand
		//   "unknown" is returned whenever an undefined item
		//     is accessed (is also the unknown node)
		//   "air" is the air node
		//   "ignore" is the ignore node

		ItemDefinition* hand_def = new ItemDefinition;
		hand_def->name.clear();
		hand_def->wield_image = "wieldhand.png";
		hand_def->tool_capabilities = new ToolCapabilities;
		m_item_definitions.insert(std::make_pair("", hand_def));

		ItemDefinition* unknown_def = new ItemDefinition;
		unknown_def->type = ITEM_NODE;
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
		TRACESTREAM(<< "ItemDefManager: registering " << def.name << std::endl);
		// Ensure that the "" item (the hand) always has ToolCapabilities
		if (def.name.empty())
			FATAL_ERROR_IF(!def.tool_capabilities, "Hand does not have ToolCapabilities");

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
	virtual void unregisterItem(const std::string &name)
	{
		verbosestream<<"ItemDefManager: unregistering \""<<name<<"\""<<std::endl;

		delete m_item_definitions[name];
		m_item_definitions.erase(name);
	}
	virtual void registerAlias(const std::string &name,
			const std::string &convert_to)
	{
		if (m_item_definitions.find(name) == m_item_definitions.end()) {
			TRACESTREAM(<< "ItemDefManager: setting alias " << name
				<< " -> " << convert_to << std::endl);
			m_aliases[name] = convert_to;
		}
	}
	void serialize(std::ostream &os, u16 protocol_version)
	{
		writeU8(os, 0); // version
		u16 count = m_item_definitions.size();
		writeU16(os, count);

		for (const auto &it : m_item_definitions) {
			ItemDefinition *def = it.second;
			// Serialize ItemDefinition and write wrapped in a string
			std::ostringstream tmp_os(std::ios::binary);
			def->serialize(tmp_os, protocol_version);
			os << serializeString16(tmp_os.str());
		}

		writeU16(os, m_aliases.size());

		for (const auto &it : m_aliases) {
			os << serializeString16(it.first);
			os << serializeString16(it.second);
		}
	}
	void deSerialize(std::istream &is, u16 protocol_version)
	{
		// Clear everything
		clear();

		if(readU8(is) != 0)
			throw SerializationError("unsupported ItemDefManager version");

		u16 count = readU16(is);
		for(u16 i=0; i<count; i++)
		{
			// Deserialize a string and grab an ItemDefinition from it
			std::istringstream tmp_is(deSerializeString16(is), std::ios::binary);
			ItemDefinition def;
			def.deSerialize(tmp_is, protocol_version);
			// Register
			registerItem(def);
		}
		u16 num_aliases = readU16(is);
		for(u16 i=0; i<num_aliases; i++)
		{
			std::string name = deSerializeString16(is);
			std::string convert_to = deSerializeString16(is);
			registerAlias(name, convert_to);
		}
	}
	void processQueue(IGameDef *gamedef)
	{
#ifndef SERVER
		//NOTE this is only thread safe for ONE consumer thread!
		while(!m_get_clientcached_queue.empty())
		{
			GetRequest<std::string, ClientCached*, u8, u8>
					request = m_get_clientcached_queue.pop();

			m_get_clientcached_queue.pushResult(request,
					createClientCachedDirect(request.key, (Client *)gamedef));
		}
#endif
	}
private:
	// Key is name
	std::map<std::string, ItemDefinition*> m_item_definitions;
	// Aliases
	StringMap m_aliases;
#ifndef SERVER
	// The id of the thread that is allowed to use irrlicht directly
	std::thread::id m_main_thread;
	// A reference to this can be returned when nothing is found, to avoid NULLs
	mutable ClientCached m_dummy_clientcached;
	// Cached textures and meshes
	mutable MutexedMap<std::string, ClientCached*> m_clientcached;
	// Queued clientcached fetches (to be processed by the main thread)
	mutable RequestQueue<std::string, ClientCached*, u8, u8> m_get_clientcached_queue;
#endif
};

IWritableItemDefManager* createItemDefManager()
{
	return new CItemDefManager();
}
