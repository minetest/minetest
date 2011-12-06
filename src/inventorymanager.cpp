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

#include "inventorymanager.h"
#include "serverremoteplayer.h"
#include "log.h"
#include "mapblock.h" // getNodeBlockPos

/*
	InventoryManager
*/

// Wrapper for old code
Inventory* InventoryManager::getInventory(InventoryContext *c, std::string id)
{
	if(id == "current_player")
	{
		assert(c->current_player);
		InventoryLocation loc;
		loc.setPlayer(c->current_player->getName());
		return getInventory(loc);
	}
	
	Strfnd fn(id);
	std::string id0 = fn.next(":");

	if(id0 == "nodemeta")
	{
		v3s16 p;
		p.X = stoi(fn.next(","));
		p.Y = stoi(fn.next(","));
		p.Z = stoi(fn.next(","));

		InventoryLocation loc;
		loc.setNodeMeta(p);
		return getInventory(loc);
	}

	errorstream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
	return NULL;
}
// Wrapper for old code
void InventoryManager::inventoryModified(InventoryContext *c, std::string id)
{
	if(id == "current_player")
	{
		assert(c->current_player);
		InventoryLocation loc;
		loc.setPlayer(c->current_player->getName());
		setInventoryModified(loc);
		return;
	}
	
	Strfnd fn(id);
	std::string id0 = fn.next(":");

	if(id0 == "nodemeta")
	{
		v3s16 p;
		p.X = stoi(fn.next(","));
		p.Y = stoi(fn.next(","));
		p.Z = stoi(fn.next(","));
		v3s16 blockpos = getNodeBlockPos(p);

		InventoryLocation loc;
		loc.setNodeMeta(p);
		setInventoryModified(loc);
		return;
	}

	errorstream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
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
	else if(type == "Drop")
	{
		a = new IDropAction(is);
	}

	return a;
}

static std::string describeC(const struct InventoryContext *c)
{
	if(c->current_player == NULL)
		return "current_player=NULL";
	else
		return std::string("current_player=") + c->current_player->getName();
}

IMoveAction::IMoveAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, from_inv, ' ');

	std::getline(is, from_list, ' ');

	std::getline(is, ts, ' ');
	from_i = stoi(ts);

	std::getline(is, to_inv, ' ');

	std::getline(is, to_list, ' ');

	std::getline(is, ts, ' ');
	to_i = stoi(ts);
}

void IMoveAction::apply(InventoryContext *c, InventoryManager *mgr,
		ServerEnvironment *env)
{
	Inventory *inv_from = mgr->getInventory(c, from_inv);
	Inventory *inv_to = mgr->getInventory(c, to_inv);
	
	if(!inv_from){
		infostream<<"IMoveAction::apply(): FAIL: source inventory not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", to_inv=\""<<to_inv<<"\""<<std::endl;
		return;
	}
	if(!inv_to){
		infostream<<"IMoveAction::apply(): FAIL: destination inventory not found: "
				"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", to_inv=\""<<to_inv<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_from){
		infostream<<"IMoveAction::apply(): FAIL: source list not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""<<std::endl;
		return;
	}
	if(!list_to){
		infostream<<"IMoveAction::apply(): FAIL: destination list not found: "
				<<"context=["<<describeC(c)<<"], to_inv=\""<<to_inv<<"\""
				<<", to_list=\""<<to_list<<"\""<<std::endl;
		return;
	}
	if(list_from->getItem(from_i) == NULL)
	{
		infostream<<"IMoveAction::apply(): FAIL: source item not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""
				<<" from_i="<<from_i<<std::endl;
		return;
	}
	/*
		If the source and the destination slots are the same
	*/
	if(inv_from == inv_to && list_from == list_to && from_i == to_i)
	{
		infostream<<"IMoveAction::apply(): FAIL: source and destination slots "
				<<"are the same: inv=\""<<from_inv<<"\" list=\""<<from_list
				<<"\" i="<<from_i<<std::endl;
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
	
	infostream<<"IMoveAction::apply(): moved at "
			<<"["<<describeC(c)<<"]"
			<<" from inv=\""<<from_inv<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<" to inv=\""<<to_inv<<"\""
			<<" list=\""<<to_list<<"\""
			<<" i="<<to_i
			<<std::endl;
}

IDropAction::IDropAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, from_inv, ' ');

	std::getline(is, from_list, ' ');

	std::getline(is, ts, ' ');
	from_i = stoi(ts);
}

void IDropAction::apply(InventoryContext *c, InventoryManager *mgr,
		ServerEnvironment *env)
{
	if(c->current_player == NULL){
		infostream<<"IDropAction::apply(): FAIL: current_player is NULL"<<std::endl;
		return;
	}

	// Do NOT cast directly to ServerActiveObject*, it breaks
	// because of multiple inheritance.
	ServerActiveObject *dropper =
		static_cast<ServerActiveObject*>(
		static_cast<ServerRemotePlayer*>(
			c->current_player
		));

	Inventory *inv_from = mgr->getInventory(c, from_inv);
	
	if(!inv_from){
		infostream<<"IDropAction::apply(): FAIL: source inventory not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_from){
		infostream<<"IDropAction::apply(): FAIL: source list not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""<<std::endl;
		return;
	}
	InventoryItem *item = list_from->getItem(from_i);
	if(item == NULL)
	{
		infostream<<"IDropAction::apply(): FAIL: source item not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""
				<<" from_i="<<from_i<<std::endl;
		return;
	}

	v3f pos = dropper->getBasePosition();
	pos.Y += 0.5*BS;

	s16 count2 = count;
	if(count2 == 0)
		count2 = -1;

	/*
		Drop the item
	*/
	bool remove = item->dropOrPlace(env, dropper, pos, false, count2);
	if(remove)
		list_from->deleteItem(from_i);

	mgr->inventoryModified(c, from_inv);

	infostream<<"IDropAction::apply(): dropped "
			<<"["<<describeC(c)<<"]"
			<<" from inv=\""<<from_inv<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<std::endl;
}

/*
	Craft checking system
*/

bool ItemSpec::checkItem(const InventoryItem *item) const
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
		if(num != 65535){
			if(mitem->getMaterial() != num)
				return false;
		} else {
			if(mitem->getNodeName() != name)
				return false;
		}
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

bool checkItemCombination(InventoryItem const * const *items, const ItemSpec *specs)
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
		const InventoryItem *item = items[items_y * 3 + items_x];
		const ItemSpec &spec = specs[specs_y * 3 + specs_x];

		if(spec.checkItem(item) == false)
			return false;
	}

	return true;
}

bool checkItemCombination(const InventoryItem * const * items,
		const InventoryItem * const * specs)
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
		if(specs[y*3 + x] == NULL)
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
		const InventoryItem *item = items[items_y * 3 + items_x];
		const InventoryItem *spec = specs[specs_y * 3 + specs_x];
		
		if(item == NULL && spec == NULL)
			continue;
		if(item == NULL && spec != NULL)
			return false;
		if(item != NULL && spec == NULL)
			return false;
		if(!spec->isSubsetOf(item))
			return false;
	}

	return true;
}


