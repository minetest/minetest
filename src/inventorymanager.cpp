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
#include "log.h"
#include "environment.h"
#include "scriptapi.h"
#include "serverobject.h"
#include "main.h"  // for g_settings
#include "settings.h"
#include "utility.h"

/*
	InventoryLocation
*/

std::string InventoryLocation::dump() const
{
	std::ostringstream os(std::ios::binary);
	serialize(os);
	return os.str();
}

void InventoryLocation::serialize(std::ostream &os) const
{
        switch(type){
        case InventoryLocation::UNDEFINED:
        {
		os<<"undefined";
	}
        break;
        case InventoryLocation::CURRENT_PLAYER:
        {
		os<<"current_player";
        }
        break;
        case InventoryLocation::PLAYER:
        {
		os<<"player:"<<name;
        }
        break;
        case InventoryLocation::NODEMETA:
        {
		os<<"nodemeta:"<<p.X<<","<<p.Y<<","<<p.Z;
        }
        break;
        default:
                assert(0);
        }
}

void InventoryLocation::deSerialize(std::istream &is)
{
	std::string tname;
	std::getline(is, tname, ':');
	if(tname == "undefined")
	{
		type = InventoryLocation::UNDEFINED;
	}
	else if(tname == "current_player")
	{
		type = InventoryLocation::CURRENT_PLAYER;
	}
	else if(tname == "player")
	{
		type = InventoryLocation::PLAYER;
		std::getline(is, name, '\n');
	}
	else if(tname == "nodemeta")
	{
		type = InventoryLocation::NODEMETA;
		std::string pos;
		std::getline(is, pos, '\n');
		Strfnd fn(pos);
		p.X = stoi(fn.next(","));
		p.Y = stoi(fn.next(","));
		p.Z = stoi(fn.next(","));
	}
	else
	{
		infostream<<"Unknown InventoryLocation type=\""<<tname<<"\""<<std::endl;
		throw SerializationError("Unknown InventoryLocation type");
	}
}

void InventoryLocation::deSerialize(std::string s)
{
	std::istringstream is(s, std::ios::binary);
	deSerialize(is);
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

IMoveAction::IMoveAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, ts, ' ');
	from_inv.deSerialize(ts);

	std::getline(is, from_list, ' ');

	std::getline(is, ts, ' ');
	from_i = stoi(ts);

	std::getline(is, ts, ' ');
	to_inv.deSerialize(ts);

	std::getline(is, to_list, ' ');

	std::getline(is, ts, ' ');
	to_i = stoi(ts);
}

void IMoveAction::apply(InventoryManager *mgr, ServerActiveObject *player)
{
	Inventory *inv_from = mgr->getInventory(from_inv);
	Inventory *inv_to = mgr->getInventory(to_inv);
	
	if(!inv_from){
		infostream<<"IMoveAction::apply(): FAIL: source inventory not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""
				<<", to_inv=\""<<to_inv.dump()<<"\""<<std::endl;
		return;
	}
	if(!inv_to){
		infostream<<"IMoveAction::apply(): FAIL: destination inventory not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""
				<<", to_inv=\""<<to_inv.dump()<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_from){
		infostream<<"IMoveAction::apply(): FAIL: source list not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""
				<<", from_list=\""<<from_list<<"\""<<std::endl;
		return;
	}
	if(!list_to){
		infostream<<"IMoveAction::apply(): FAIL: destination list not found: "
				<<"to_inv=\""<<to_inv.dump()<<"\""
				<<", to_list=\""<<to_list<<"\""<<std::endl;
		return;
	}
	if(list_from->getItem(from_i).empty())
	{
		infostream<<"IMoveAction::apply(): FAIL: source item not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""
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
				<<"are the same: inv=\""<<from_inv.dump()
				<<"\" list=\""<<from_list
				<<"\" i="<<from_i<<std::endl;
		return;
	}
	
	// Take item from source list
	ItemStack item1;
	if(count == 0)
		item1 = list_from->changeItem(from_i, ItemStack());
	else
		item1 = list_from->takeItem(from_i, count);

	// Try to add the item to destination list
	int oldcount = item1.count;
	item1 = list_to->addItem(to_i, item1);

	// If something is returned, the item was not fully added
	if(!item1.empty())
	{
		// If olditem is returned, nothing was added.
		bool nothing_added = (item1.count == oldcount);
		
		// If something else is returned, part of the item was left unadded.
		// Add the other part back to the source item
		list_from->addItem(from_i, item1);

		// If olditem is returned, nothing was added.
		// Swap the items
		if(nothing_added)
		{
			// Take item from source list
			item1 = list_from->changeItem(from_i, ItemStack());
			// Adding was not possible, swap the items.
			ItemStack item2 = list_to->changeItem(to_i, item1);
			// Put item from destination list to the source list
			list_from->changeItem(from_i, item2);
		}
	}

	mgr->setInventoryModified(from_inv);
	if(inv_from != inv_to)
		mgr->setInventoryModified(to_inv);
	
	infostream<<"IMoveAction::apply(): moved at "
			<<" count="<<count<<"\""
			<<" from inv=\""<<from_inv.dump()<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<" to inv=\""<<to_inv.dump()<<"\""
			<<" list=\""<<to_list<<"\""
			<<" i="<<to_i
			<<std::endl;
}

IDropAction::IDropAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, ts, ' ');
	from_inv.deSerialize(ts);

	std::getline(is, from_list, ' ');

	std::getline(is, ts, ' ');
	from_i = stoi(ts);
}

void IDropAction::apply(InventoryManager *mgr, ServerActiveObject *player)
{
	Inventory *inv_from = mgr->getInventory(from_inv);
	
	if(!inv_from){
		infostream<<"IDropAction::apply(): FAIL: source inventory not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_from){
		infostream<<"IDropAction::apply(): FAIL: source list not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""<<std::endl;
		return;
	}
	if(list_from->getItem(from_i).empty())
	{
		infostream<<"IDropAction::apply(): FAIL: source item not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""
				<<", from_list=\""<<from_list<<"\""
				<<" from_i="<<from_i<<std::endl;
		return;
	}

	/*
		Drop the item
	*/
	ItemStack item = list_from->getItem(from_i);
	if(scriptapi_item_on_drop(player->getEnv()->getLua(), item, player,
				player->getBasePosition() + v3f(0,1,0)))
	{
		// Apply returned ItemStack
		if(g_settings->getBool("creative_mode") == false
				|| from_inv.type != InventoryLocation::PLAYER)
			list_from->changeItem(from_i, item);
		mgr->setInventoryModified(from_inv);
	}

	infostream<<"IDropAction::apply(): dropped "
			<<" from inv=\""<<from_inv.dump()<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<std::endl;
}
