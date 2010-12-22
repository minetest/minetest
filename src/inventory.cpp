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

InventoryItem::InventoryItem()
{
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
		//return g_device->getVideoDriver()->getTexture("../data/rat.png");
		return g_irrlicht->getTexture("../data/rat.png");
	
	if(m_inventorystring.substr(0,4) == "Sign")
		//return g_device->getVideoDriver()->getTexture("../data/sign.png");
		return g_irrlicht->getTexture("../data/sign.png");

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

	os<<"end\n";
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

		if(name == "end")
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

bool InventoryList::addItem(InventoryItem *newitem)
{
	// If it is a MaterialItem, try to find an already existing one
	// and just increment the counter
	if(std::string("MaterialItem") == newitem->getName())
	{
		u8 material = ((MaterialItem*)newitem)->getMaterial();
		u8 count = ((MaterialItem*)newitem)->getCount();
		for(u32 i=0; i<m_items.size(); i++)
		{
			InventoryItem *item2 = m_items[i];
			if(item2 == NULL)
				continue;
			if(std::string("MaterialItem") != item2->getName())
				continue;
			// Found one. Check if it is of the right material and has
			// free space
			MaterialItem *mitem2 = (MaterialItem*)item2;
			if(mitem2->getMaterial() != material)
				continue;
			//TODO: Add all that can be added and add remaining part
			// to another place
			if(mitem2->freeSpace() < count)
				continue;
			// Add to the counter
			mitem2->add(count);
			// Dump the parameter
			delete newitem;
			return true;
		}
	}
	// Else find an empty position
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
			continue;
		m_items[i] = newitem;
		return true;
	}
	// Failed
	return false;
}

bool InventoryList::addItem(u32 i, InventoryItem *newitem)
{
	// If it is an empty position, it's an easy job.
	InventoryItem *item = m_items[i];
	if(item == NULL)
	{
		m_items[i] = newitem;
		return true;
	}

	// If it is a material item, try to 
	if(std::string("MaterialItem") == newitem->getName())
	{
		u8 material = ((MaterialItem*)newitem)->getMaterial();
		u8 count = ((MaterialItem*)newitem)->getCount();
		InventoryItem *item2 = m_items[i];

		if(item2 != NULL
			&& std::string("MaterialItem") == item2->getName())
		{
			// Check if it is of the right material and has free space
			MaterialItem *mitem2 = (MaterialItem*)item2;
			if(mitem2->getMaterial() == material
					&& mitem2->freeSpace() >= count)
			{
				// Add to the counter
				mitem2->add(count);
				// Dump the parameter
				delete newitem;
				// Done
				return true;
			}
		}
	}
	
	return false;
}

void InventoryList::decrementMaterials(u16 count)
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item == NULL)
			continue;
		if(std::string("MaterialItem") == item->getName())
		{
			MaterialItem *mitem = (MaterialItem*)item;
			if(mitem->getCount() < count)
			{
				dstream<<__FUNCTION_NAME<<": decrementMaterials():"
						<<" too small material count"<<std::endl;
			}
			else if(mitem->getCount() == count)
			{
				deleteItem(i);
			}
			else
			{
				mitem->remove(1);
			}
		}
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

	os<<"end\n";
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

		if(name == "end")
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
	
	if(!list_from || !list_to || list_from->getItem(from_i) == NULL
			|| (list_from == list_to && from_i == to_i))
	{
		dstream<<__FUNCTION_NAME<<": Operation not allowed"<<std::endl;
		return;
	}
	
	// Take item from source list
	InventoryItem *item1 = list_from->changeItem(from_i, NULL);
	// Try to add the item to destination list
	if(list_to->addItem(to_i, item1))
	{
		// Done.
		return;
	}
	// Adding was not possible, switch it.
	// Switch it to the destination list
	InventoryItem *item2 = list_to->changeItem(to_i, item1);
	// Put item from destination list to the source list
	list_from->changeItem(from_i, item2);
}
	
//END
