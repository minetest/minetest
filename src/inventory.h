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

#ifndef INVENTORY_HEADER
#define INVENTORY_HEADER

#include <iostream>
#include <string>
#include <vector>
#include "irrlichttypes.h"
#include "debug.h"
#include "itemdef.h"

struct ToolCapabilities;

struct ItemStack
{
	ItemStack(): name(""), count(0), wear(0), metadata("") {}
	ItemStack(std::string name_, u16 count_,
			u16 wear, std::string metadata_,
			IItemDefManager *itemdef);
	~ItemStack() {}

	// Serialization
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is, IItemDefManager *itemdef);
	void deSerialize(const std::string &s, IItemDefManager *itemdef);

	// Returns the string used for inventory
	std::string getItemString() const;

	/*
		Quantity methods
	*/

	bool empty() const
	{
		return count == 0;
	}

	void clear()
	{
		name = "";
		count = 0;
		wear = 0;
		metadata = "";
	}

	void add(u16 n)
	{
		count += n;
	}

	void remove(u16 n)
	{
		assert(count >= n);
		count -= n;
		if(count == 0)
			clear(); // reset name, wear and metadata too
	}

	// Maximum size of a stack
	u16 getStackMax(IItemDefManager *itemdef) const
	{
		s16 max = itemdef->get(name).stack_max;
		return (max >= 0) ? max : 0;
	}

	// Number of items that can be added to this stack
	u16 freeSpace(IItemDefManager *itemdef) const
	{
		u16 max = getStackMax(itemdef);
		if(count > max)
			return 0;
		return max - count;
	}

	// Returns false if item is not known and cannot be used
	bool isKnown(IItemDefManager *itemdef) const
	{
		return itemdef->isKnown(name);
	}

	// Returns a pointer to the item definition struct,
	// or a fallback one (name="unknown") if the item is unknown.
	const ItemDefinition& getDefinition(
			IItemDefManager *itemdef) const
	{
		return itemdef->get(name);
	}

	// Get tool digging properties, or those of the hand if not a tool
	const ToolCapabilities& getToolCapabilities(
			IItemDefManager *itemdef) const
	{
		ToolCapabilities *cap;
		cap = itemdef->get(name).tool_capabilities;
		if(cap == NULL)
			cap = itemdef->get("").tool_capabilities;
		assert(cap != NULL);
		return *cap;
	}

	// Wear out (only tools)
	// Returns true if the item is (was) a tool
	bool addWear(s32 amount, IItemDefManager *itemdef)
	{
		if(getDefinition(itemdef).type == ITEM_TOOL)
		{
			if(amount > 65535 - wear)
				clear();
			else if(amount < -wear)
				wear = 0;
			else
				wear += amount;
			return true;
		}
		else
		{
			return false;
		}
	}

	// If possible, adds newitem to this item.
	// If cannot be added at all, returns the item back.
	// If can be added partly, decremented item is returned back.
	// If can be added fully, empty item is returned.
	ItemStack addItem(const ItemStack &newitem,
			IItemDefManager *itemdef);

	// Checks whether newitem could be added.
	// If restitem is non-NULL, it receives the part of newitem that
	// would be left over after adding.
	bool itemFits(const ItemStack &newitem,
			ItemStack *restitem,  // may be NULL
			IItemDefManager *itemdef) const;

	// Takes some items.
	// If there are not enough, takes as many as it can.
	// Returns empty item if couldn't take any.
	ItemStack takeItem(u32 takecount);

	// Similar to takeItem, but keeps this ItemStack intact.
	ItemStack peekItem(u32 peekcount) const;

	/*
		Properties
	*/
	std::string name;
	u16 count;
	u16 wear;
	std::string metadata;
};

class InventoryList
{
public:
	InventoryList(std::string name, u32 size, IItemDefManager *itemdef);
	~InventoryList();
	void clearItems();
	void setSize(u32 newsize);
	void setWidth(u32 newWidth);
	void setName(const std::string &name);
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);

	InventoryList(const InventoryList &other);
	InventoryList & operator = (const InventoryList &other);
	bool operator == (const InventoryList &other) const;
	bool operator != (const InventoryList &other) const
	{
		return !(*this == other);
	}

	const std::string &getName() const;
	u32 getSize() const;
	u32 getWidth() const;
	// Count used slots
	u32 getUsedSlots() const;
	u32 getFreeSlots() const;

	// Get reference to item
	const ItemStack& getItem(u32 i) const;
	ItemStack& getItem(u32 i);
	// Returns old item. Parameter can be an empty item.
	ItemStack changeItem(u32 i, const ItemStack &newitem);
	// Delete item
	void deleteItem(u32 i);

	// Adds an item to a suitable place. Returns leftover item (possibly empty).
	ItemStack addItem(const ItemStack &newitem);

	// If possible, adds item to given slot.
	// If cannot be added at all, returns the item back.
	// If can be added partly, decremented item is returned back.
	// If can be added fully, empty item is returned.
	ItemStack addItem(u32 i, const ItemStack &newitem);

	// Checks whether the item could be added to the given slot
	// If restitem is non-NULL, it receives the part of newitem that
	// would be left over after adding.
	bool itemFits(const u32 i, const ItemStack &newitem,
			ItemStack *restitem = NULL) const;

	// Checks whether there is room for a given item
	bool roomForItem(const ItemStack &item) const;

	// Checks whether the given count of the given item name
	// exists in this inventory list.
	bool containsItem(const ItemStack &item) const;

	// Removes the given count of the given item name from
	// this inventory list. Walks the list in reverse order.
	// If not as many items exist as requested, removes as
	// many as possible.
	// Returns the items that were actually removed.
	ItemStack removeItem(const ItemStack &item);

	// Takes some items from a slot.
	// If there are not enough, takes as many as it can.
	// Returns empty item if couldn't take any.
	ItemStack takeItem(u32 i, u32 takecount);

	// Similar to takeItem, but keeps the slot intact.
	ItemStack peekItem(u32 i, u32 peekcount) const;

	// Move an item to a different list (or a different stack in the same list)
	// count is the maximum number of items to move (0 for everything)
	void moveItem(u32 i, InventoryList *dest, u32 dest_i, u32 count = 0);

private:
	std::vector<ItemStack> m_items;
	u32 m_size, m_width;
	std::string m_name;
	IItemDefManager *m_itemdef;
};

class Inventory
{
public:
	~Inventory();

	void clear();
	void clearContents();

	Inventory(IItemDefManager *itemdef);
	Inventory(const Inventory &other);
	Inventory & operator = (const Inventory &other);
	bool operator == (const Inventory &other) const;
	bool operator != (const Inventory &other) const
	{
		return !(*this == other);
	}

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);

	InventoryList * addList(const std::string &name, u32 size);
	InventoryList * getList(const std::string &name);
	const InventoryList * getList(const std::string &name) const;
	std::vector<const InventoryList*> getLists();
	bool deleteList(const std::string &name);
	// A shorthand for adding items. Returns leftover item (possibly empty).
	ItemStack addItem(const std::string &listname, const ItemStack &newitem)
	{
		m_dirty = true;
		InventoryList *list = getList(listname);
		if(list == NULL)
			return newitem;
		return list->addItem(newitem);
	}

	bool checkModified() const
	{
		return m_dirty;
	}

	void setModified(const bool x)
	{
		m_dirty = x;
	}

private:
	// -1 if not found
	const s32 getListIndex(const std::string &name) const;

	std::vector<InventoryList*> m_lists;
	IItemDefManager *m_itemdef;
	bool m_dirty;
};

#endif

