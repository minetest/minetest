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

#include "nodemetadata.h"
#include "exceptions.h"
#include "gamedef.h"
#include "inventory.h"
#include "log.h"
#include "util/serialize.h"
#include "constants.h" // MAP_BLOCKSIZE
#include <sstream>
#include "script/scripting_game.h"

/*
	NodeMetadata
*/

NodeMetadata::NodeMetadata(IItemDefManager *item_def_mgr, v3s16 node_pos):
	m_inventory(new Inventory(item_def_mgr, this)),
	m_node_pos(node_pos)
{}

NodeMetadata::~NodeMetadata()
{
	delete m_inventory;
}

void NodeMetadata::on_remove_item(
	GameScripting *script_interface, 
	const InventoryList *inventory_list, 
	const ItemStack &deleted_item)
{
	if (script_interface) {
		script_interface->on_nodemeta_inventory_remove_item(
			m_node_pos, inventory_list->getName(), deleted_item);
	}
}

void NodeMetadata::on_change_item(
	GameScripting *script_interface, 
	const InventoryList *inventory_list, 
	u32 query_slot, 
	const ItemStack &old_item,
	const ItemStack &new_item)
{
	if (script_interface) {
		script_interface->on_nodemeta_inventory_change_item(
			m_node_pos, inventory_list->getName(), query_slot, old_item, new_item);
	}
}

void NodeMetadata::on_add_item(
	GameScripting *script_interface, 
	const InventoryList *inventory_list, 
	u32 query_slot, 
	const ItemStack &added_item)
{
	if (script_interface) {
		script_interface->on_nodemeta_inventory_add_item(
			m_node_pos, inventory_list->getName(), query_slot, added_item);
	}
}

void NodeMetadata::serialize(std::ostream &os) const
{
	int num_vars = m_stringvars.size();
	writeU32(os, num_vars);
	for (StringMap::const_iterator
			it = m_stringvars.begin();
			it != m_stringvars.end(); ++it) {
		os << serializeString(it->first);
		os << serializeLongString(it->second);
	}

	m_inventory->serialize(os);
}

void NodeMetadata::deSerialize(std::istream &is)
{
	m_stringvars.clear();
	int num_vars = readU32(is);
	for(int i=0; i<num_vars; i++){
		std::string name = deSerializeString(is);
		std::string var = deSerializeLongString(is);
		m_stringvars[name] = var;
	}

	m_inventory->deSerialize(is);
}

void NodeMetadata::clear()
{
	Metadata::clear();
	m_inventory->clear();
}

bool NodeMetadata::empty() const
{
	return Metadata::empty() && m_inventory->getLists().size() == 0;
}

/*
	NodeMetadataList
*/

void NodeMetadataList::serialize(std::ostream &os) const
{
	/*
		Version 0 is a placeholder for "nothing to see here; go away."
	*/

	u16 count = countNonEmpty();
	if (count == 0) {
		writeU8(os, 0); // version
		return;
	}

	writeU8(os, 1); // version
	writeU16(os, count);

	for(std::map<v3s16, NodeMetadata*>::const_iterator
			i = m_data.begin();
			i != m_data.end(); ++i)
	{
		v3s16 p = i->first;
		NodeMetadata *data = i->second;
		if (data->empty())
			continue;

		u16 p16 = p.Z * MAP_BLOCKSIZE * MAP_BLOCKSIZE + p.Y * MAP_BLOCKSIZE + p.X;
		writeU16(os, p16);

		data->serialize(os);
	}
}

void NodeMetadataList::deSerialize(std::istream &is, IItemDefManager *item_def_mgr, v3s16 relative_map_block_pos)
{
	clear();

	u8 version = readU8(is);

	if (version == 0) {
		// Nothing
		return;
	}

	if (version != 1) {
		std::string err_str = std::string(FUNCTION_NAME)
			+ ": version " + itos(version) + " not supported";
		infostream << err_str << std::endl;
		throw SerializationError(err_str);
	}

	u16 count = readU16(is);

	for (u16 i=0; i < count; i++) {
		u16 p16 = readU16(is);

		v3s16 p;
		p.Z = p16 / MAP_BLOCKSIZE / MAP_BLOCKSIZE;
		p16 &= MAP_BLOCKSIZE * MAP_BLOCKSIZE - 1;
		p.Y = p16 / MAP_BLOCKSIZE;
		p16 &= MAP_BLOCKSIZE - 1;
		p.X = p16;

		if (m_data.find(p) != m_data.end()) {
			warningstream<<"NodeMetadataList::deSerialize(): "
					<<"already set data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			continue;
		}

		NodeMetadata *data = new NodeMetadata(item_def_mgr, relative_map_block_pos+p);
		data->deSerialize(is);
		m_data[p] = data;
	}
}

NodeMetadataList::~NodeMetadataList()
{
	clear();
}

std::vector<v3s16> NodeMetadataList::getAllKeys()
{
	std::vector<v3s16> keys;

	std::map<v3s16, NodeMetadata *>::const_iterator it;
	for (it = m_data.begin(); it != m_data.end(); ++it)
		keys.push_back(it->first);

	return keys;
}

NodeMetadata *NodeMetadataList::get(v3s16 p)
{
	std::map<v3s16, NodeMetadata *>::const_iterator n = m_data.find(p);
	if (n == m_data.end())
		return NULL;
	return n->second;
}

void NodeMetadataList::remove(v3s16 p)
{
	NodeMetadata *olddata = get(p);
	if (olddata) {
		delete olddata;
		m_data.erase(p);
	}
}

void NodeMetadataList::set(v3s16 p, NodeMetadata *d)
{
	remove(p);
	m_data.insert(std::make_pair(p, d));
}

void NodeMetadataList::clear()
{
	std::map<v3s16, NodeMetadata*>::iterator it;
	for (it = m_data.begin(); it != m_data.end(); ++it) {
		delete it->second;
	}
	m_data.clear();
}

int NodeMetadataList::countNonEmpty() const
{
	int n = 0;
	std::map<v3s16, NodeMetadata*>::const_iterator it;
	for (it = m_data.begin(); it != m_data.end(); ++it) {
		if (!it->second->empty())
			n++;
	}
	return n;
}
