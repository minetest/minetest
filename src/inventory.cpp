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

#include "inventory.h"
#include "serialization.h"
#include "utility.h"
#include "debug.h"
#include <sstream>
#include "log.h"
#include "itemdef.h"
#include "strfnd.h"
#include "content_mapnode.h" // For loading legacy MaterialItems
#include "nameidmapping.h" // For loading legacy MaterialItems

/*
	ItemStack
*/

static content_t content_translate_from_19_to_internal(content_t c_from)
{
	for(u32 i=0; i<sizeof(trans_table_19)/sizeof(trans_table_19[0]); i++)
	{
		if(trans_table_19[i][1] == c_from)
		{
			return trans_table_19[i][0];
		}
	}
	return c_from;
}

// If the string contains spaces, quotes or control characters, encodes as JSON.
// Else returns the string unmodified.
static std::string serializeJsonStringIfNeeded(const std::string &s)
{
	for(size_t i = 0; i < s.size(); ++i)
	{
		if(s[i] <= 0x1f || s[i] >= 0x7f || s[i] == ' ' || s[i] == '\"')
			return serializeJsonString(s);
	}
	return s;
}

// Parses a string serialized by serializeJsonStringIfNeeded.
static std::string deSerializeJsonStringIfNeeded(std::istream &is)
{
	std::ostringstream tmp_os;
	bool expect_initial_quote = true;
	bool is_json = false;
	bool was_backslash = false;
	for(;;)
	{
		char c = is.get();
		if(is.eof())
			break;
		if(expect_initial_quote && c == '"')
		{
			tmp_os << c;
			is_json = true;
		}
		else if(is_json)
		{
			tmp_os << c;
			if(was_backslash)
				was_backslash = false;
			else if(c == '\\')
				was_backslash = true;
			else if(c == '"')
				break; // Found end of string
		}
		else
		{
			if(c == ' ')
			{
				// Found end of word
				is.unget();
				break;
			}
			else
			{
				tmp_os << c;
			}
		}
		expect_initial_quote = false;
	}
	if(is_json)
	{
		std::istringstream tmp_is(tmp_os.str(), std::ios::binary);
		return deSerializeJsonString(tmp_is);
	}
	else
		return tmp_os.str();
}


ItemStack::ItemStack(std::string name_, u16 count_,
		u16 wear_, std::string metadata_,
		IItemDefManager *itemdef)
{
	name = itemdef->getAlias(name_);
	count = count_;
	wear = wear_;
	metadata = metadata_;

	if(name.empty() || count == 0)
		clear();
	else if(itemdef->get(name).type == ITEM_TOOL)
		count = 1;
}

void ItemStack::serialize(std::ostream &os) const
{
	DSTACK(__FUNCTION_NAME);

	if(empty())
		return;

	// Check how many parts of the itemstring are needed
	int parts = 1;
	if(count != 1)
		parts = 2;
	if(wear != 0)
		parts = 3;
	if(metadata != "")
		parts = 4;

	os<<serializeJsonStringIfNeeded(name);
	if(parts >= 2)
		os<<" "<<count;
	if(parts >= 3)
		os<<" "<<wear;
	if(parts >= 4)
		os<<" "<<serializeJsonStringIfNeeded(metadata);
}

void ItemStack::deSerialize(std::istream &is, IItemDefManager *itemdef)
{
	DSTACK(__FUNCTION_NAME);

	clear();

	// Read name
	name = deSerializeJsonStringIfNeeded(is);

	// Skip space
	std::string tmp;
	std::getline(is, tmp, ' ');
	if(!tmp.empty())
		throw SerializationError("Unexpected text after item name");
	
	if(name == "MaterialItem")
	{
		// Obsoleted on 2011-07-30

		u16 material;
		is>>material;
		u16 materialcount;
		is>>materialcount;
		// Convert old materials
		if(material <= 0xff)
			material = content_translate_from_19_to_internal(material);
		if(material > MAX_CONTENT)
			throw SerializationError("Too large material number");
		// Convert old id to name
		NameIdMapping legacy_nimap;
		content_mapnode_get_name_id_mapping(&legacy_nimap);
		legacy_nimap.getName(material, name);
		if(name == "")
			name = "unknown_block";
		name = itemdef->getAlias(name);
		count = materialcount;
	}
	else if(name == "MaterialItem2")
	{
		// Obsoleted on 2011-11-16

		u16 material;
		is>>material;
		u16 materialcount;
		is>>materialcount;
		if(material > MAX_CONTENT)
			throw SerializationError("Too large material number");
		// Convert old id to name
		NameIdMapping legacy_nimap;
		content_mapnode_get_name_id_mapping(&legacy_nimap);
		legacy_nimap.getName(material, name);
		if(name == "")
			name = "unknown_block";
		name = itemdef->getAlias(name);
		count = materialcount;
	}
	else if(name == "node" || name == "NodeItem" || name == "MaterialItem3"
			|| name == "craft" || name == "CraftItem")
	{
		// Obsoleted on 2012-01-07

		std::string all;
		std::getline(is, all, '\n');
		// First attempt to read inside ""
		Strfnd fnd(all);
		fnd.next("\"");
		// If didn't skip to end, we have ""s
		if(!fnd.atend()){
			name = fnd.next("\"");
		} else { // No luck, just read a word then
			fnd.start(all);
			name = fnd.next(" ");
		}
		fnd.skip_over(" ");
		name = itemdef->getAlias(name);
		count = stoi(trim(fnd.next("")));
		if(count == 0)
			count = 1;
	}
	else if(name == "MBOItem")
	{
		// Obsoleted on 2011-10-14
		throw SerializationError("MBOItem not supported anymore");
	}
	else if(name == "tool" || name == "ToolItem")
	{
		// Obsoleted on 2012-01-07

		std::string all;
		std::getline(is, all, '\n');
		// First attempt to read inside ""
		Strfnd fnd(all);
		fnd.next("\"");
		// If didn't skip to end, we have ""s
		if(!fnd.atend()){
			name = fnd.next("\"");
		} else { // No luck, just read a word then
			fnd.start(all);
			name = fnd.next(" ");
		}
		count = 1;
		// Then read wear
		fnd.skip_over(" ");
		name = itemdef->getAlias(name);
		wear = stoi(trim(fnd.next("")));
	}
	else
	{
		do  // This loop is just to allow "break;"
		{
			// The real thing

			// Apply item aliases
			name = itemdef->getAlias(name);

			// Read the count
			std::string count_str;
			std::getline(is, count_str, ' ');
			if(count_str.empty())
			{
				count = 1;
				break;
			}
			else
				count = stoi(count_str);

			// Read the wear
			std::string wear_str;
			std::getline(is, wear_str, ' ');
			if(wear_str.empty())
				break;
			else
				wear = stoi(wear_str);

			// Read metadata
			metadata = deSerializeJsonStringIfNeeded(is);

			// In case fields are added after metadata, skip space here:
			//std::getline(is, tmp, ' ');
			//if(!tmp.empty())
			//	throw SerializationError("Unexpected text after metadata");

		} while(false);
	}

	if(name.empty() || count == 0)
		clear();
	else if(itemdef->get(name).type == ITEM_TOOL)
		count = 1;
}

void ItemStack::deSerialize(const std::string &str, IItemDefManager *itemdef)
{
	std::istringstream is(str, std::ios::binary);
	deSerialize(is, itemdef);
}

std::string ItemStack::getItemString() const
{
	// Get item string
	std::ostringstream os(std::ios::binary);
	serialize(os);
	return os.str();
}

ItemStack ItemStack::addItem(const ItemStack &newitem_,
		IItemDefManager *itemdef)
{
	ItemStack newitem = newitem_;

	// If the item is empty or the position invalid, bail out
	if(newitem.empty())
	{
		// nothing can be added trivially
	}
	// If this is an empty item, it's an easy job.
	else if(empty())
	{
		*this = newitem;
		newitem.clear();
	}
	// If item name differs, bail out
	else if(name != newitem.name)
	{
		// cannot be added
	}
	// If the item fits fully, add counter and delete it
	else if(newitem.count <= freeSpace(itemdef))
	{
		add(newitem.count);
		newitem.clear();
	}
	// Else the item does not fit fully. Add all that fits and return
	// the rest.
	else
	{
		u16 freespace = freeSpace(itemdef);
		add(freespace);
		newitem.remove(freespace);
	}

	return newitem;
}

bool ItemStack::itemFits(const ItemStack &newitem_,
		ItemStack *restitem,
		IItemDefManager *itemdef) const
{
	ItemStack newitem = newitem_;

	// If the item is empty or the position invalid, bail out
	if(newitem.empty())
	{
		// nothing can be added trivially
	}
	// If this is an empty item, it's an easy job.
	else if(empty())
	{
		newitem.clear();
	}
	// If item name differs, bail out
	else if(name != newitem.name)
	{
		// cannot be added
	}
	// If the item fits fully, delete it
	else if(newitem.count <= freeSpace(itemdef))
	{
		newitem.clear();
	}
	// Else the item does not fit fully. Return the rest.
	// the rest.
	else
	{
		u16 freespace = freeSpace(itemdef);
		newitem.remove(freespace);
	}

	if(restitem)
		*restitem = newitem;
	return newitem.empty();
}

ItemStack ItemStack::takeItem(u32 takecount)
{
	if(takecount == 0 || count == 0)
		return ItemStack();

	ItemStack result = *this;
	if(takecount >= count)
	{
		// Take all
		clear();
	}
	else
	{
		// Take part
		remove(takecount);
		result.count = takecount;
	}
	return result;
}

ItemStack ItemStack::peekItem(u32 peekcount) const
{
	if(peekcount == 0 || count == 0)
		return ItemStack();

	ItemStack result = *this;
	if(peekcount < count)
		result.count = peekcount;
	return result;
}

/*
	Inventory
*/

InventoryList::InventoryList(std::string name, u32 size, IItemDefManager *itemdef)
{
	m_name = name;
	m_size = size;
	m_itemdef = itemdef;
	clearItems();
	//m_dirty = false;
}

InventoryList::~InventoryList()
{
}

void InventoryList::clearItems()
{
	m_items.clear();

	for(u32 i=0; i<m_size; i++)
	{
		m_items.push_back(ItemStack());
	}

	//setDirty(true);
}

void InventoryList::setSize(u32 newsize)
{
	if(newsize != m_items.size())
		m_items.resize(newsize);
	m_size = newsize;
}

void InventoryList::serialize(std::ostream &os) const
{
	//os.imbue(std::locale("C"));
	
	for(u32 i=0; i<m_items.size(); i++)
	{
		const ItemStack &item = m_items[i];
		if(item.empty())
		{
			os<<"Empty";
		}
		else
		{
			os<<"Item ";
			item.serialize(os);
		}
		os<<"\n";
	}

	os<<"EndInventoryList\n";
}

void InventoryList::deSerialize(std::istream &is)
{
	//is.imbue(std::locale("C"));

	clearItems();
	u32 item_i = 0;

	for(;;)
	{
		std::string line;
		std::getline(is, line, '\n');

		std::istringstream iss(line);
		//iss.imbue(std::locale("C"));

		std::string name;
		std::getline(iss, name, ' ');

		if(name == "EndInventoryList")
		{
			break;
		}
		// This is a temporary backwards compatibility fix
		else if(name == "end")
		{
			break;
		}
		else if(name == "Item")
		{
			if(item_i > getSize() - 1)
				throw SerializationError("too many items");
			ItemStack item;
			item.deSerialize(iss, m_itemdef);
			m_items[item_i++] = item;
		}
		else if(name == "Empty")
		{
			if(item_i > getSize() - 1)
				throw SerializationError("too many items");
			m_items[item_i++].clear();
		}
		else
		{
			throw SerializationError("Unknown inventory identifier");
		}
	}
}

InventoryList::InventoryList(const InventoryList &other)
{
	*this = other;
}

InventoryList & InventoryList::operator = (const InventoryList &other)
{
	m_items = other.m_items;
	m_size = other.m_size;
	m_name = other.m_name;
	m_itemdef = other.m_itemdef;
	//setDirty(true);

	return *this;
}

const std::string &InventoryList::getName() const
{
	return m_name;
}

u32 InventoryList::getSize() const
{
	return m_items.size();
}

u32 InventoryList::getUsedSlots() const
{
	u32 num = 0;
	for(u32 i=0; i<m_items.size(); i++)
	{
		if(!m_items[i].empty())
			num++;
	}
	return num;
}

u32 InventoryList::getFreeSlots() const
{
	return getSize() - getUsedSlots();
}

const ItemStack& InventoryList::getItem(u32 i) const
{
	assert(i < m_size);
	return m_items[i];
}

ItemStack& InventoryList::getItem(u32 i)
{
	assert(i < m_size);
	return m_items[i];
}

ItemStack InventoryList::changeItem(u32 i, const ItemStack &newitem)
{
	if(i >= m_items.size())
		return newitem;

	ItemStack olditem = m_items[i];
	m_items[i] = newitem;
	//setDirty(true);
	return olditem;
}

void InventoryList::deleteItem(u32 i)
{
	assert(i < m_items.size());
	m_items[i].clear();
}

ItemStack InventoryList::addItem(const ItemStack &newitem_)
{
	ItemStack newitem = newitem_;

	if(newitem.empty())
		return newitem;
	
	/*
		First try to find if it could be added to some existing items
	*/
	for(u32 i=0; i<m_items.size(); i++)
	{
		// Ignore empty slots
		if(m_items[i].empty())
			continue;
		// Try adding
		newitem = addItem(i, newitem);
		if(newitem.empty())
			return newitem; // All was eaten
	}

	/*
		Then try to add it to empty slots
	*/
	for(u32 i=0; i<m_items.size(); i++)
	{
		// Ignore unempty slots
		if(!m_items[i].empty())
			continue;
		// Try adding
		newitem = addItem(i, newitem);
		if(newitem.empty())
			return newitem; // All was eaten
	}

	// Return leftover
	return newitem;
}

ItemStack InventoryList::addItem(u32 i, const ItemStack &newitem)
{
	if(i >= m_items.size())
		return newitem;

	ItemStack leftover = m_items[i].addItem(newitem, m_itemdef);
	//if(leftover != newitem)
	//	setDirty(true);
	return leftover;
}

bool InventoryList::itemFits(const u32 i, const ItemStack &newitem,
		ItemStack *restitem) const
{
	if(i >= m_items.size())
	{
		if(restitem)
			*restitem = newitem;
		return false;
	}

	return m_items[i].itemFits(newitem, restitem, m_itemdef);
}

bool InventoryList::roomForItem(const ItemStack &item_) const
{
	ItemStack item = item_;
	ItemStack leftover;
	for(u32 i=0; i<m_items.size(); i++)
	{
		if(itemFits(i, item, &leftover))
			return true;
		item = leftover;
	}
	return false;
}

bool InventoryList::containsItem(const ItemStack &item) const
{
	u32 count = item.count;
	if(count == 0)
		return true;
	for(std::vector<ItemStack>::const_reverse_iterator
			i = m_items.rbegin();
			i != m_items.rend(); i++)
	{
		if(count == 0)
			break;
		if(i->name == item.name)
		{
			if(i->count >= count)
				return true;
			else
				count -= i->count;
		}
	}
	return false;
}

ItemStack InventoryList::removeItem(const ItemStack &item)
{
	ItemStack removed;
	for(std::vector<ItemStack>::reverse_iterator
			i = m_items.rbegin();
			i != m_items.rend(); i++)
	{
		if(i->name == item.name)
		{
			u32 still_to_remove = item.count - removed.count;
			removed.addItem(i->takeItem(still_to_remove), m_itemdef);
			if(removed.count == item.count)
				break;
		}
	}
	return removed;
}

ItemStack InventoryList::takeItem(u32 i, u32 takecount)
{
	if(i >= m_items.size())
		return ItemStack();

	ItemStack taken = m_items[i].takeItem(takecount);
	//if(!taken.empty())
	//	setDirty(true);
	return taken;
}

ItemStack InventoryList::peekItem(u32 i, u32 peekcount) const
{
	if(i >= m_items.size())
		return ItemStack();

	return m_items[i].peekItem(peekcount);
}

void InventoryList::moveItem(u32 i, InventoryList *dest, u32 dest_i, u32 count)
{
	if(this == dest && i == dest_i)
		return;

	// Take item from source list
	ItemStack item1;
	if(count == 0)
		item1 = changeItem(i, ItemStack());
	else
		item1 = takeItem(i, count);

	if(item1.empty())
		return;

	// Try to add the item to destination list
	u32 oldcount = item1.count;
	item1 = dest->addItem(dest_i, item1);

	// If something is returned, the item was not fully added
	if(!item1.empty())
	{
		// If olditem is returned, nothing was added.
		bool nothing_added = (item1.count == oldcount);

		// If something else is returned, part of the item was left unadded.
		// Add the other part back to the source item
		addItem(i, item1);

		// If olditem is returned, nothing was added.
		// Swap the items
		if(nothing_added)
		{
			// Take item from source list
			item1 = changeItem(i, ItemStack());
			// Adding was not possible, swap the items.
			ItemStack item2 = dest->changeItem(dest_i, item1);
			// Put item from destination list to the source list
			changeItem(i, item2);
		}
	}
}

/*
	Inventory
*/

Inventory::~Inventory()
{
	clear();
}

void Inventory::clear()
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		delete m_lists[i];
	}
	m_lists.clear();
}

Inventory::Inventory(IItemDefManager *itemdef)
{
	m_itemdef = itemdef;
}

Inventory::Inventory(const Inventory &other)
{
	*this = other;
}

Inventory & Inventory::operator = (const Inventory &other)
{
	// Gracefully handle self assignment
	if(this != &other)
	{
		clear();
		m_itemdef = other.m_itemdef;
		for(u32 i=0; i<other.m_lists.size(); i++)
		{
			m_lists.push_back(new InventoryList(*other.m_lists[i]));
		}
	}
	return *this;
}

void Inventory::serialize(std::ostream &os) const
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		InventoryList *list = m_lists[i];
		os<<"List "<<list->getName()<<" "<<list->getSize()<<"\n";
		list->serialize(os);
	}

	os<<"EndInventory\n";
}

void Inventory::deSerialize(std::istream &is)
{
	clear();

	for(;;)
	{
		std::string line;
		std::getline(is, line, '\n');

		std::istringstream iss(line);

		std::string name;
		std::getline(iss, name, ' ');

		if(name == "EndInventory")
		{
			break;
		}
		// This is a temporary backwards compatibility fix
		else if(name == "end")
		{
			break;
		}
		else if(name == "List")
		{
			std::string listname;
			u32 listsize;

			std::getline(iss, listname, ' ');
			iss>>listsize;

			InventoryList *list = new InventoryList(listname, listsize, m_itemdef);
			list->deSerialize(is);

			m_lists.push_back(list);
		}
		else
		{
			throw SerializationError("Unknown inventory identifier");
		}
	}
}

InventoryList * Inventory::addList(const std::string &name, u32 size)
{
	s32 i = getListIndex(name);
	if(i != -1)
	{
		if(m_lists[i]->getSize() != size)
		{
			delete m_lists[i];
			m_lists[i] = new InventoryList(name, size, m_itemdef);
		}
		return m_lists[i];
	}
	else
	{
		InventoryList *list = new InventoryList(name, size, m_itemdef);
		m_lists.push_back(list);
		return list;
	}
}

InventoryList * Inventory::getList(const std::string &name)
{
	s32 i = getListIndex(name);
	if(i == -1)
		return NULL;
	return m_lists[i];
}

bool Inventory::deleteList(const std::string &name)
{
	s32 i = getListIndex(name);
	if(i == -1)
		return false;
	delete m_lists[i];
	m_lists.erase(m_lists.begin() + i);
	return true;
}

const InventoryList * Inventory::getList(const std::string &name) const
{
	s32 i = getListIndex(name);
	if(i == -1)
		return NULL;
	return m_lists[i];
}

const s32 Inventory::getListIndex(const std::string &name) const
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		if(m_lists[i]->getName() == name)
			return i;
	}
	return -1;
}

//END
