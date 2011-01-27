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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "inventory.h"
#include "serialization.h"
#include "utility.h"
#include "debug.h"
#include <sstream>
#include "main.h"

/*
	InventoryItem
*/

InventoryItem::InventoryItem(u16 count)
{
	m_count = count;
}

InventoryItem::~InventoryItem()
{
}

InventoryItem* InventoryItem::deSerialize(std::istream &is)
{
	DSTACK(__FUNCTION_NAME);

	//is.imbue(std::locale("C"));
	// Read name
	std::string name;
	std::getline(is, name, ' ');
	
	if(name == "MaterialItem")
	{
		// u16 reads directly as a number (u8 doesn't)
		u16 material;
		is>>material;
		u16 count;
		is>>count;
		if(material > 255)
			throw SerializationError("Too large material number");
		return new MaterialItem(material, count);
	}
	else if(name == "MBOItem")
	{
		std::string inventorystring;
		std::getline(is, inventorystring, '|');
		return new MapBlockObjectItem(inventorystring);
	}
	else if(name == "CraftItem")
	{
		std::string subname;
		std::getline(is, subname, ' ');
		u16 count;
		is>>count;
		return new CraftItem(subname, count);
	}
	else if(name == "ToolItem")
	{
		std::string toolname;
		std::getline(is, toolname, ' ');
		u16 wear;
		is>>wear;
		return new ToolItem(toolname, wear);
	}
	else
	{
		dstream<<"Unknown InventoryItem name=\""<<name<<"\""<<std::endl;
		throw SerializationError("Unknown InventoryItem name");
	}
}

/*
	MapBlockObjectItem
*/
#ifndef SERVER
video::ITexture * MapBlockObjectItem::getImage()
{
	if(m_inventorystring.substr(0,3) == "Rat")
		//return g_device->getVideoDriver()->getTexture(porting::getDataPath("rat.png").c_str());
		return g_irrlicht->getTexture(porting::getDataPath("rat.png").c_str());
	
	if(m_inventorystring.substr(0,4) == "Sign")
		//return g_device->getVideoDriver()->getTexture(porting::getDataPath("sign.png").c_str());
		return g_irrlicht->getTexture(porting::getDataPath("sign.png").c_str());

	return NULL;
}
#endif
std::string MapBlockObjectItem::getText()
{
	if(m_inventorystring.substr(0,3) == "Rat")
		return "";
	
	if(m_inventorystring.substr(0,4) == "Sign")
		return "";

	return "obj";
}

MapBlockObject * MapBlockObjectItem::createObject
		(v3f pos, f32 player_yaw, f32 player_pitch)
{
	std::istringstream is(m_inventorystring);
	std::string name;
	std::getline(is, name, ' ');
	
	if(name == "None")
	{
		return NULL;
	}
	else if(name == "Sign")
	{
		std::string text;
		std::getline(is, text, '|');
		SignObject *obj = new SignObject(NULL, -1, pos);
		obj->setText(text);
		obj->setYaw(-player_yaw);
		return obj;
	}
	else if(name == "Rat")
	{
		RatObject *obj = new RatObject(NULL, -1, pos);
		return obj;
	}
	else if(name == "ItemObj")
	{
		/*
			Now we are an inventory item containing the serialization
			string of an object that contains the serialization
			string of an inventory item. Fuck this.
		*/
		//assert(0);
		dstream<<__FUNCTION_NAME<<": WARNING: Ignoring ItemObj "
				<<"because an item-object should never be inside "
				<<"an object-item."<<std::endl;
		return NULL;
	}
	else
	{
		return NULL;
	}
}

/*
	Inventory
*/

InventoryList::InventoryList(std::string name, u32 size)
{
	m_name = name;
	m_size = size;
	clearItems();
}

InventoryList::~InventoryList()
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		if(m_items[i])
			delete m_items[i];
	}
}

void InventoryList::clearItems()
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		if(m_items[i])
			delete m_items[i];
	}

	m_items.clear();

	for(u32 i=0; i<m_size; i++)
	{
		m_items.push_back(NULL);
	}
}

void InventoryList::serialize(std::ostream &os)
{
	//os.imbue(std::locale("C"));
	
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
		{
			os<<"Item ";
			item->serialize(os);
		}
		else
		{
			os<<"Empty";
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
		else if(name == "Item")
		{
			if(item_i > getSize() - 1)
				throw SerializationError("too many items");
			InventoryItem *item = InventoryItem::deSerialize(iss);
			m_items[item_i++] = item;
		}
		else if(name == "Empty")
		{
			if(item_i > getSize() - 1)
				throw SerializationError("too many items");
			m_items[item_i++] = NULL;
		}
		else
		{
			throw SerializationError("Unknown inventory identifier");
		}
	}
}

InventoryList::InventoryList(const InventoryList &other)
{
	/*
		Do this so that the items get cloned. Otherwise the pointers
		in the array will just get copied.
	*/
	*this = other;
}

InventoryList & InventoryList::operator = (const InventoryList &other)
{
	m_name = other.m_name;
	m_size = other.m_size;
	clearItems();
	for(u32 i=0; i<other.m_items.size(); i++)
	{
		InventoryItem *item = other.m_items[i];
		if(item != NULL)
		{
			m_items[i] = item->clone();
		}
	}

	return *this;
}

std::string InventoryList::getName()
{
	return m_name;
}

u32 InventoryList::getSize()
{
	return m_items.size();
}

u32 InventoryList::getUsedSlots()
{
	u32 num = 0;
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
			num++;
	}
	return num;
}

InventoryItem * InventoryList::getItem(u32 i)
{
	if(i > m_items.size() - 1)
		return NULL;
	return m_items[i];
}

InventoryItem * InventoryList::changeItem(u32 i, InventoryItem *newitem)
{
	assert(i < m_items.size());

	InventoryItem *olditem = m_items[i];
	m_items[i] = newitem;
	return olditem;
}

void InventoryList::deleteItem(u32 i)
{
	assert(i < m_items.size());
	InventoryItem *item = changeItem(i, NULL);
	if(item)
		delete item;
}

InventoryItem * InventoryList::addItem(InventoryItem *newitem)
{
	/*
		First try to find if it could be added to some existing items
	*/
	for(u32 i=0; i<m_items.size(); i++)
	{
		// Ignore empty slots
		if(m_items[i] == NULL)
			continue;
		// Try adding
		newitem = addItem(i, newitem);
		if(newitem == NULL)
			return NULL; // All was eaten
	}

	/*
		Then try to add it to empty slots
	*/
	for(u32 i=0; i<m_items.size(); i++)
	{
		// Ignore unempty slots
		if(m_items[i] != NULL)
			continue;
		// Try adding
		newitem = addItem(i, newitem);
		if(newitem == NULL)
			return NULL; // All was eaten
	}

	// Return leftover
	return newitem;
}

InventoryItem * InventoryList::addItem(u32 i, InventoryItem *newitem)
{
	// If it is an empty position, it's an easy job.
	InventoryItem *to_item = m_items[i];
	if(to_item == NULL)
	{
		m_items[i] = newitem;
		return NULL;
	}
	
	// If not addable, return the item
	if(newitem->addableTo(to_item) == false)
		return newitem;
	
	// If the item fits fully in the slot, add counter and delete it
	if(newitem->getCount() <= to_item->freeSpace())
	{
		to_item->add(newitem->getCount());
		delete newitem;
		return NULL;
	}
	// Else the item does not fit fully. Add all that fits and return
	// the rest.
	else
	{
		u16 freespace = to_item->freeSpace();
		to_item->add(freespace);
		newitem->remove(freespace);
		return newitem;
	}
}

InventoryItem * InventoryList::takeItem(u32 i, u32 count)
{
	if(count == 0)
		return NULL;

	InventoryItem *item = m_items[i];
	// If it is an empty position, return NULL
	if(item == NULL)
		return NULL;
	
	if(count >= item->getCount())
	{
		// Get the item by swapping NULL to its place
		return changeItem(i, NULL);
	}
	else
	{
		InventoryItem *item2 = item->clone();
		item->remove(count);
		item2->setCount(count);
		return item2;
	}
	
	return false;
}

void InventoryList::decrementMaterials(u16 count)
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = takeItem(i, count);
		if(item)
			delete item;
	}
}

void InventoryList::print(std::ostream &o)
{
	o<<"InventoryList:"<<std::endl;
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
		{
			o<<i<<": ";
			item->serialize(o);
			o<<"\n";
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

Inventory::Inventory()
{
}

Inventory::Inventory(const Inventory &other)
{
	*this = other;
}

Inventory & Inventory::operator = (const Inventory &other)
{
	clear();
	for(u32 i=0; i<other.m_lists.size(); i++)
	{
		m_lists.push_back(new InventoryList(*other.m_lists[i]));
	}
	return *this;
}

void Inventory::serialize(std::ostream &os)
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
		else if(name == "List")
		{
			std::string listname;
			u32 listsize;

			std::getline(iss, listname, ' ');
			iss>>listsize;

			InventoryList *list = new InventoryList(listname, listsize);
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
			m_lists[i] = new InventoryList(name, size);
		}
		return m_lists[i];
	}
	else
	{
		m_lists.push_back(new InventoryList(name, size));
		return m_lists.getLast();
	}
}

InventoryList * Inventory::getList(const std::string &name)
{
	s32 i = getListIndex(name);
	if(i == -1)
		return NULL;
	return m_lists[i];
}

s32 Inventory::getListIndex(const std::string &name)
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		if(m_lists[i]->getName() == name)
			return i;
	}
	return -1;
}

/*
	InventoryAction
*/

InventoryAction * InventoryAction::deSerialize(std::istream &is)
{
	std::string type;
	std::getline(is, type, ' ');

	InventoryAction *a = NULL;

	if(type == "Move")
	{
		a = new IMoveAction(is);
	}

	return a;
}

void IMoveAction::apply(Inventory *inventory)
{
	/*dstream<<"from_name="<<from_name<<" to_name="<<to_name<<std::endl;
	dstream<<"from_i="<<from_i<<" to_i="<<to_i<<std::endl;*/
	InventoryList *list_from = inventory->getList(from_name);
	InventoryList *list_to = inventory->getList(to_name);
	/*dstream<<"list_from="<<list_from<<" list_to="<<list_to
			<<std::endl;*/
	/*if(list_from)
		dstream<<" list_from->getItem(from_i)="<<list_from->getItem(from_i)
				<<std::endl;
	if(list_to)
		dstream<<" list_to->getItem(to_i)="<<list_to->getItem(to_i)
				<<std::endl;*/
	
	/*
		If a list doesn't exist or the source item doesn't exist
		or the source and the destination slots are the same
	*/
	if(!list_from || !list_to || list_from->getItem(from_i) == NULL
			|| (list_from == list_to && from_i == to_i))
	{
		dstream<<__FUNCTION_NAME<<": Operation not allowed"<<std::endl;
		return;
	}
	
	// Take item from source list
	InventoryItem *item1 = NULL;
	if(count == 0)
		item1 = list_from->changeItem(from_i, NULL);
	else
		item1 = list_from->takeItem(from_i, count);

	// Try to add the item to destination list
	InventoryItem *olditem = item1;
	item1 = list_to->addItem(to_i, item1);

	// If nothing is returned, the item was fully added
	if(item1 == NULL)
		return;
	
	// If olditem is returned, nothing was added.
	bool nothing_added = (item1 == olditem);
	
	// If something else is returned, part of the item was left unadded.
	// Add the other part back to the source item
	list_from->addItem(from_i, item1);

	// If olditem is returned, nothing was added.
	// Swap the items
	if(nothing_added)
	{
		// Take item from source list
		item1 = list_from->changeItem(from_i, NULL);
		// Adding was not possible, swap the items.
		InventoryItem *item2 = list_to->changeItem(to_i, item1);
		// Put item from destination list to the source list
		list_from->changeItem(from_i, item2);
		return;
	}
}
	
//END
