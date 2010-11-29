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

video::ITexture * MapBlockObjectItem::getImage()
{
	if(m_inventorystring.substr(0,3) == "Rat")
		return g_device->getVideoDriver()->getTexture("../data/rat.png");
	
	if(m_inventorystring.substr(0,4) == "Sign")
		return g_device->getVideoDriver()->getTexture("../data/sign.png");

	return NULL;
}
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

Inventory::Inventory(u32 size)
{
	m_size = size;
	clearItems();
}

Inventory::~Inventory()
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		delete m_items[i];
	}
}

void Inventory::clearItems()
{
	m_items.clear();
	for(u32 i=0; i<m_size; i++)
	{
		m_items.push_back(NULL);
	}
}

void Inventory::serialize(std::ostream &os)
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

void Inventory::deSerialize(std::istream &is)
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

Inventory & Inventory::operator = (Inventory &other)
{
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

u32 Inventory::getSize()
{
	return m_items.size();
}

u32 Inventory::getUsedSlots()
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

InventoryItem * Inventory::getItem(u32 i)
{
	if(i > m_items.size() - 1)
		return NULL;
	return m_items[i];
}

InventoryItem * Inventory::changeItem(u32 i, InventoryItem *newitem)
{
	assert(i < m_items.size());

	InventoryItem *olditem = m_items[i];
	m_items[i] = newitem;
	return olditem;
}

void Inventory::deleteItem(u32 i)
{
	assert(i < m_items.size());
	InventoryItem *item = changeItem(i, NULL);
	if(item)
		delete item;
}

bool Inventory::addItem(InventoryItem *newitem)
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

void Inventory::print(std::ostream &o)
{
	o<<"Player inventory:"<<std::endl;
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
	
//END
