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

#ifndef NODEMETADATA_HEADER
#define NODEMETADATA_HEADER

#include "irr_v3d.h"
#include <iostream>
#include <vector>
#include "util/string.h"
#include "inventory.h" // For InventoryChangeReceiver class

/*
	NodeMetadata stores arbitary amounts of data for special blocks.
	Used for furnaces, chests and signs.

	There are two interaction methods: inventory menu and text input.
	Only one can be used for a single metadata, thus only inventory OR
	text input should exist in a metadata.
*/

class Inventory;
class IItemDefManager;

class NodeMetadata: public InventoryChangeReceiver
{
public:
	NodeMetadata(IItemDefManager *item_def_mgr, v3s16 node_pos);
	~NodeMetadata();

	void on_remove_item(GameScripting *script_interface, const InventoryList *inventory_list, const ItemStack &deleted_item);
	void on_change_item(GameScripting *script_interface, const InventoryList *inventory_list, u32 query_slot, const ItemStack &old_item,const ItemStack &new_item);
	void on_add_item(GameScripting *script_interface, const InventoryList *inventory_list, u32 query_slot, const ItemStack &added_item);

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);

	void clear();

	// Generic key/value store
	std::string getString(const std::string &name, unsigned short recursion = 0) const;
	void setString(const std::string &name, const std::string &var);
	// Support variable names in values
	std::string resolveString(const std::string &str, unsigned short recursion = 0) const;
	StringMap getStrings() const
	{
		return m_stringvars;
	}

	// The inventory
	Inventory *getInventory()
	{
		return m_inventory;
	}

private:
	StringMap m_stringvars;
	Inventory *m_inventory;
	v3s16 m_node_pos;
};


/*
	List of metadata of all the nodes of a block
*/

class NodeMetadataList
{
public:
	~NodeMetadataList();

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is, IItemDefManager *item_def_mgr, v3s16 relative_map_block_pos);

	// Add all keys in this list to the vector keys
	std::vector<v3s16> getAllKeys();
	// Get pointer to data
	NodeMetadata *get(v3s16 p);
	// Deletes data
	void remove(v3s16 p);
	// Deletes old data and sets a new one
	void set(v3s16 p, NodeMetadata *d);
	// Deletes all
	void clear();

private:
	std::map<v3s16, NodeMetadata *> m_data;
};

#endif

