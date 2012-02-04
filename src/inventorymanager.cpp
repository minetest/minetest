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
#include "craftdef.h"

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
	else if(type == "Craft")
	{
		a = new ICraftAction(is);
	}

	return a;
}

/*
	IMoveAction
*/

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

void IMoveAction::apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef)
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
	/*
		This performs the actual movement

		If something is wrong (source item is empty, destination is the
		same as source), nothing happens
	*/
	list_from->moveItem(from_i, list_to, to_i, count);

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

void IMoveAction::clientApply(InventoryManager *mgr, IGameDef *gamedef)
{
	// Optional InventoryAction operation that is run on the client
	// to make lag less apparent.

	Inventory *inv_from = mgr->getInventory(from_inv);
	Inventory *inv_to = mgr->getInventory(to_inv);
	if(!inv_from || !inv_to)
		return;

	InventoryLocation current_player;
	current_player.setCurrentPlayer();
	Inventory *inv_player = mgr->getInventory(current_player);
	if(inv_from != inv_player || inv_to != inv_player)
		return;

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);
	if(!list_from || !list_to)
		return;

	list_from->moveItem(from_i, list_to, to_i, count);

	mgr->setInventoryModified(from_inv);
	if(inv_from != inv_to)
		mgr->setInventoryModified(to_inv);
}

/*
	IDropAction
*/

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

void IDropAction::apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef)
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

	// Take item from source list
	ItemStack item1;
	if(count == 0)
		item1 = list_from->changeItem(from_i, ItemStack());
	else
		item1 = list_from->takeItem(from_i, count);

	// Drop the item and apply the returned ItemStack
	ItemStack item2 = item1;
	if(scriptapi_item_on_drop(player->getEnv()->getLua(), item2, player,
				player->getBasePosition() + v3f(0,1,0)))
	{
		if(g_settings->getBool("creative_mode") == true
				&& from_inv.type == InventoryLocation::PLAYER)
			item2 = item1;  // creative mode

		list_from->addItem(from_i, item2);

		// Unless we have put the same amount back as we took in the first place,
		// set inventory modified flag
		if(item2.count != item1.count)
			mgr->setInventoryModified(from_inv);
	}

	infostream<<"IDropAction::apply(): dropped "
			<<" from inv=\""<<from_inv.dump()<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<std::endl;
}

void IDropAction::clientApply(InventoryManager *mgr, IGameDef *gamedef)
{
	// Optional InventoryAction operation that is run on the client
	// to make lag less apparent.

	Inventory *inv_from = mgr->getInventory(from_inv);
	if(!inv_from)
		return;

	InventoryLocation current_player;
	current_player.setCurrentPlayer();
	Inventory *inv_player = mgr->getInventory(current_player);
	if(inv_from != inv_player)
		return;

	InventoryList *list_from = inv_from->getList(from_list);
	if(!list_from)
		return;

	if(count == 0)
		list_from->changeItem(from_i, ItemStack());
	else
		list_from->takeItem(from_i, count);

	mgr->setInventoryModified(from_inv);
}

/*
	ICraftAction
*/

ICraftAction::ICraftAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, ts, ' ');
	craft_inv.deSerialize(ts);
}

void ICraftAction::apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef)
{
	Inventory *inv_craft = mgr->getInventory(craft_inv);
	
	if(!inv_craft){
		infostream<<"ICraftAction::apply(): FAIL: inventory not found: "
				<<"craft_inv=\""<<craft_inv.dump()<<"\""<<std::endl;
		return;
	}

	InventoryList *list_craft = inv_craft->getList("craft");
	InventoryList *list_craftresult = inv_craft->getList("craftresult");

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_craft){
		infostream<<"ICraftAction::apply(): FAIL: craft list not found: "
				<<"craft_inv=\""<<craft_inv.dump()<<"\""<<std::endl;
		return;
	}
	if(!list_craftresult){
		infostream<<"ICraftAction::apply(): FAIL: craftresult list not found: "
				<<"craft_inv=\""<<craft_inv.dump()<<"\""<<std::endl;
		return;
	}
	if(list_craftresult->getSize() < 1){
		infostream<<"ICraftAction::apply(): FAIL: craftresult list too short: "
				<<"craft_inv=\""<<craft_inv.dump()<<"\""<<std::endl;
		return;
	}

	ItemStack crafted;
	int count_remaining = count;
	bool found = getCraftingResult(inv_craft, crafted, false, gamedef);

	while(found && list_craftresult->itemFits(0, crafted))
	{
		// Decrement input and add crafting output
		getCraftingResult(inv_craft, crafted, true, gamedef);
		list_craftresult->addItem(0, crafted);
		mgr->setInventoryModified(craft_inv);

		actionstream<<player->getDescription()
				<<" crafts "
				<<crafted.getItemString()
				<<std::endl;

		// Decrement counter
		if(count_remaining == 1)
			break;
		else if(count_remaining > 1)
			count_remaining--;

		// Get next crafting result
		found = getCraftingResult(inv_craft, crafted, false, gamedef);
	}

	infostream<<"ICraftAction::apply(): crafted "
			<<" craft_inv=\""<<craft_inv.dump()<<"\""
			<<std::endl;
}

void ICraftAction::clientApply(InventoryManager *mgr, IGameDef *gamedef)
{
	// Optional InventoryAction operation that is run on the client
	// to make lag less apparent.
}


// Crafting helper
bool getCraftingResult(Inventory *inv, ItemStack& result,
		bool decrementInput, IGameDef *gamedef)
{
	DSTACK(__FUNCTION_NAME);
	
	result.clear();

	// TODO: Allow different sizes of crafting grids

	// Get the InventoryList in which we will operate
	InventoryList *clist = inv->getList("craft");
	if(!clist || clist->getSize() != 9)
		return false;

	// Mangle crafting grid to an another format
	CraftInput ci;
	ci.method = CRAFT_METHOD_NORMAL;
	ci.width = 3;
	for(u16 i=0; i<9; i++)
		ci.items.push_back(clist->getItem(i));

	// Find out what is crafted and add it to result item slot
	CraftOutput co;
	bool found = gamedef->getCraftDefManager()->getCraftResult(
			ci, co, decrementInput, gamedef);
	if(found)
		result.deSerialize(co.item, gamedef->getItemDefManager());

	if(found && decrementInput)
	{
		// CraftInput has been changed, apply changes in clist
		for(u16 i=0; i<9; i++)
		{
			clist->changeItem(i, ci.items[i]);
		}
	}

	return found;
}

