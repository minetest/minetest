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
#include "serverobject.h"
#include "content_mapnode.h"
#include "content_inventory.h"

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

ServerActiveObject* InventoryItem::createSAO(ServerEnvironment *env, u16 id, v3f pos)
{
	/*
		Create an ItemSAO
	*/
	// Get item string
	std::ostringstream os(std::ios_base::binary);
	serialize(os);
	// Create object
	ServerActiveObject *obj = new ItemSAO(env, 0, pos, os.str());
	return obj;
}

/*
	MaterialItem
*/

bool MaterialItem::isCookable()
{
	return item_material_is_cookable(m_content);
}

InventoryItem *MaterialItem::createCookResult()
{
	return item_material_create_cook_result(m_content);
}

/*
	CraftItem
*/

#ifndef SERVER
video::ITexture * CraftItem::getImage()
{
	if(g_texturesource == NULL)
		return NULL;
	
	std::string name = item_craft_get_image_name(m_subname);

	// Get such a texture
	return g_texturesource->getTextureRaw(name);
}
#endif

ServerActiveObject* CraftItem::createSAO(ServerEnvironment *env, u16 id, v3f pos)
{
	// Special cases
	ServerActiveObject *obj = item_craft_create_object(m_subname, env, id, pos);
	if(obj)
		return obj;
	// Default
	return InventoryItem::createSAO(env, id, pos);
}

u16 CraftItem::getDropCount()
{
	// Special cases
	s16 dc = item_craft_get_drop_count(m_subname);
	if(dc != -1)
		return dc;
	// Default
	return InventoryItem::getDropCount();
}

bool CraftItem::isCookable()
{
	return item_craft_is_cookable(m_subname);
}

InventoryItem *CraftItem::createCookResult()
{
	return item_craft_create_cook_result(m_subname);
}

/*
	MapBlockObjectItem DEPRECATED
	TODO: Remove
*/
#ifndef SERVER
video::ITexture * MapBlockObjectItem::getImage()
{
	if(m_inventorystring.substr(0,3) == "Rat")
		return g_texturesource->getTextureRaw("rat.png");
	
	if(m_inventorystring.substr(0,4) == "Sign")
		return g_texturesource->getTextureRaw("sign.png");

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
	//m_dirty = false;
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

	//setDirty(true);
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
		// This is a temporary backwards compatibility fix
		else if(name == "end")
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
	//setDirty(true);

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

u32 InventoryList::getFreeSlots()
{
	return getSize() - getUsedSlots();
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
	//setDirty(true);
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
	if(newitem == NULL)
		return NULL;
	
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
	if(newitem == NULL)
		return NULL;
	
	//setDirty(true);
	
	// If it is an empty position, it's an easy job.
	InventoryItem *to_item = getItem(i);
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

bool InventoryList::itemFits(u32 i, InventoryItem *newitem)
{
	// If it is an empty position, it's an easy job.
	InventoryItem *to_item = getItem(i);
	if(to_item == NULL)
	{
		return true;
	}
	
	// If not addable, return the item
	if(newitem->addableTo(to_item) == false)
		return false;
	
	// If the item fits fully in the slot, add counter and delete it
	if(newitem->getCount() <= to_item->freeSpace())
	{
		return true;
	}

	return false;
}

InventoryItem * InventoryList::takeItem(u32 i, u32 count)
{
	if(count == 0)
		return NULL;
	
	//setDirty(true);

	InventoryItem *item = getItem(i);
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

void IMoveAction::apply(InventoryContext *c, InventoryManager *mgr)
{
#if 1

	/*dstream<<"from_inv="<<from_inv<<" to_inv="<<to_inv<<std::endl;
	dstream<<"from_list="<<from_list<<" to_list="<<to_list<<std::endl;
	dstream<<"from_i="<<from_i<<" to_i="<<to_i<<std::endl;*/

	Inventory *inv_from = mgr->getInventory(c, from_inv);
	Inventory *inv_to = mgr->getInventory(c, to_inv);

	if(!inv_from || !inv_to)
	{
		dstream<<__FUNCTION_NAME<<": Operation not allowed "
				<<"(inventories not found)"<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);

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
	*/
	if(!list_from || !list_to)
	{
		dstream<<__FUNCTION_NAME<<": Operation not allowed "
				<<"(a list doesn't exist)"
				<<std::endl;
		return;
	}
	if(list_from->getItem(from_i) == NULL)
	{
		dstream<<__FUNCTION_NAME<<": Operation not allowed "
				<<"(the source item doesn't exist)"
				<<std::endl;
		return;
	}
	/*
		If the source and the destination slots are the same
	*/
	if(inv_from == inv_to && list_from == list_to && from_i == to_i)
	{
		dstream<<__FUNCTION_NAME<<": Operation not allowed "
				<<"(source and the destination slots are the same)"<<std::endl;
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

	// If something is returned, the item was not fully added
	if(item1 != NULL)
	{
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
		}
	}

	mgr->inventoryModified(c, from_inv);
	if(from_inv != to_inv)
		mgr->inventoryModified(c, to_inv);
#endif
}

/*
	Craft checking system
*/

bool ItemSpec::checkItem(InventoryItem *item)
{
	if(type == ITEM_NONE)
	{
		// Has to be no item
		if(item != NULL)
			return false;
		return true;
	}
	
	// There should be an item
	if(item == NULL)
		return false;

	std::string itemname = item->getName();

	if(type == ITEM_MATERIAL)
	{
		if(itemname != "MaterialItem")
			return false;
		MaterialItem *mitem = (MaterialItem*)item;
		if(mitem->getMaterial() != num)
			return false;
	}
	else if(type == ITEM_CRAFT)
	{
		if(itemname != "CraftItem")
			return false;
		CraftItem *mitem = (CraftItem*)item;
		if(mitem->getSubName() != name)
			return false;
	}
	else if(type == ITEM_TOOL)
	{
		// Not supported yet
		assert(0);
	}
	else if(type == ITEM_MBO)
	{
		// Not supported yet
		assert(0);
	}
	else
	{
		// Not supported yet
		assert(0);
	}
	return true;
}

bool checkItemCombination(InventoryItem **items, ItemSpec *specs)
{
	u16 items_min_x = 100;
	u16 items_max_x = 100;
	u16 items_min_y = 100;
	u16 items_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(items[y*3 + x] == NULL)
			continue;
		if(items_min_x == 100 || x < items_min_x)
			items_min_x = x;
		if(items_min_y == 100 || y < items_min_y)
			items_min_y = y;
		if(items_max_x == 100 || x > items_max_x)
			items_max_x = x;
		if(items_max_y == 100 || y > items_max_y)
			items_max_y = y;
	}
	// No items at all, just return false
	if(items_min_x == 100)
		return false;
	
	u16 items_w = items_max_x - items_min_x + 1;
	u16 items_h = items_max_y - items_min_y + 1;

	u16 specs_min_x = 100;
	u16 specs_max_x = 100;
	u16 specs_min_y = 100;
	u16 specs_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(specs[y*3 + x].type == ITEM_NONE)
			continue;
		if(specs_min_x == 100 || x < specs_min_x)
			specs_min_x = x;
		if(specs_min_y == 100 || y < specs_min_y)
			specs_min_y = y;
		if(specs_max_x == 100 || x > specs_max_x)
			specs_max_x = x;
		if(specs_max_y == 100 || y > specs_max_y)
			specs_max_y = y;
	}
	// No specs at all, just return false
	if(specs_min_x == 100)
		return false;

	u16 specs_w = specs_max_x - specs_min_x + 1;
	u16 specs_h = specs_max_y - specs_min_y + 1;

	// Different sizes
	if(items_w != specs_w || items_h != specs_h)
		return false;

	for(u16 y=0; y<specs_h; y++)
	for(u16 x=0; x<specs_w; x++)
	{
		u16 items_x = items_min_x + x;
		u16 items_y = items_min_y + y;
		u16 specs_x = specs_min_x + x;
		u16 specs_y = specs_min_y + y;
		InventoryItem *item = items[items_y * 3 + items_x];
		ItemSpec &spec = specs[specs_y * 3 + specs_x];

		if(spec.checkItem(item) == false)
			return false;
	}

	return true;
}
	
//END
