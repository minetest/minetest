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

#include "inventorymanager.h"
#include "debug.h"
#include "log.h"
#include "serverenvironment.h"
#include "scripting_server.h"
#include "serverobject.h"
#include "settings.h"
#include "craftdef.h"
#include "rollback_interface.h"
#include "util/strfnd.h"
#include "util/basic_macros.h"

#define PLAYER_TO_SA(p)   p->getEnv()->getScriptIface()

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
	switch (type) {
	case InventoryLocation::UNDEFINED:
		os<<"undefined";
		break;
	case InventoryLocation::CURRENT_PLAYER:
		os<<"current_player";
		break;
	case InventoryLocation::PLAYER:
		os<<"player:"<<name;
		break;
	case InventoryLocation::NODEMETA:
		os<<"nodemeta:"<<p.X<<","<<p.Y<<","<<p.Z;
		break;
	case InventoryLocation::DETACHED:
		os<<"detached:"<<name;
		break;
	default:
		FATAL_ERROR("Unhandled inventory location type");
	}
}

void InventoryLocation::deSerialize(std::istream &is)
{
	std::string tname;
	std::getline(is, tname, ':');
	if (tname == "undefined") {
		type = InventoryLocation::UNDEFINED;
	} else if (tname == "current_player") {
		type = InventoryLocation::CURRENT_PLAYER;
	} else if (tname == "player") {
		type = InventoryLocation::PLAYER;
		std::getline(is, name, '\n');
	} else if (tname == "nodemeta") {
		type = InventoryLocation::NODEMETA;
		std::string pos;
		std::getline(is, pos, '\n');
		Strfnd fn(pos);
		p.X = stoi(fn.next(","));
		p.Y = stoi(fn.next(","));
		p.Z = stoi(fn.next(","));
	} else if (tname == "detached") {
		type = InventoryLocation::DETACHED;
		std::getline(is, name, '\n');
	} else {
		infostream<<"Unknown InventoryLocation type=\""<<tname<<"\""<<std::endl;
		throw SerializationError("Unknown InventoryLocation type");
	}
}

void InventoryLocation::deSerialize(const std::string &s)
{
	std::istringstream is(s, std::ios::binary);
	deSerialize(is);
}

/*
	InventoryAction
*/

InventoryAction *InventoryAction::deSerialize(std::istream &is)
{
	std::string type;
	std::getline(is, type, ' ');

	InventoryAction *a = nullptr;

	if (type == "Move") {
		a = new IMoveAction(is, false);
	} else if (type == "MoveSomewhere") {
		a = new IMoveAction(is, true);
	} else if (type == "Drop") {
		a = new IDropAction(is);
	} else if (type == "Craft") {
		a = new ICraftAction(is);
	}

	return a;
}

/*
	IMoveAction
*/

IMoveAction::IMoveAction(std::istream &is, bool somewhere) :
		move_somewhere(somewhere)
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

	if (!somewhere) {
		std::getline(is, ts, ' ');
		to_i = stoi(ts);
	}
}

void IMoveAction::apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef)
{
	Inventory *inv_from = mgr->getInventory(from_inv);
	Inventory *inv_to = mgr->getInventory(to_inv);

	if (!inv_from) {
		infostream << "IMoveAction::apply(): FAIL: source inventory not found: "
			<< "from_inv=\""<<from_inv.dump() << "\""
			<< ", to_inv=\"" << to_inv.dump() << "\"" << std::endl;
		return;
	}
	if (!inv_to) {
		infostream << "IMoveAction::apply(): FAIL: destination inventory not found: "
			<< "from_inv=\"" << from_inv.dump() << "\""
			<< ", to_inv=\"" << to_inv.dump() << "\"" << std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if (!list_from) {
		infostream << "IMoveAction::apply(): FAIL: source list not found: "
			<< "from_inv=\"" << from_inv.dump() << "\""
			<< ", from_list=\"" << from_list << "\"" << std::endl;
		return;
	}
	if (!list_to) {
		infostream << "IMoveAction::apply(): FAIL: destination list not found: "
			<< "to_inv=\""<<to_inv.dump() << "\""
			<< ", to_list=\"" << to_list << "\"" << std::endl;
		return;
	}

	if (move_somewhere) {
		s16 old_to_i = to_i;
		u16 old_count = count;
		caused_by_move_somewhere = true;
		move_somewhere = false;

		infostream << "IMoveAction::apply(): moving item somewhere"
			<< " msom=" << move_somewhere
			<< " count=" << count
			<< " from inv=\"" << from_inv.dump() << "\""
			<< " list=\"" << from_list << "\""
			<< " i=" << from_i
			<< " to inv=\"" << to_inv.dump() << "\""
			<< " list=\"" << to_list << "\""
			<< std::endl;

		// Try to add the item to destination list
		s16 dest_size = list_to->getSize();
		// First try all the non-empty slots
		for (s16 dest_i = 0; dest_i < dest_size && count > 0; dest_i++) {
			if (!list_to->getItem(dest_i).empty()) {
				to_i = dest_i;
				apply(mgr, player, gamedef);
				count -= move_count;
			}
		}

		// Then try all the empty ones
		for (s16 dest_i = 0; dest_i < dest_size && count > 0; dest_i++) {
			if (list_to->getItem(dest_i).empty()) {
				to_i = dest_i;
				apply(mgr, player, gamedef);
				count -= move_count;
			}
		}

		to_i = old_to_i;
		count = old_count;
		caused_by_move_somewhere = false;
		move_somewhere = true;
		return;
	}

	if ((u16)to_i > list_to->getSize()) {
		infostream << "IMoveAction::apply(): FAIL: destination index out of bounds: "
			<< "to_i=" << to_i
			<< ", size=" << list_to->getSize() << std::endl;
		return;
	}
	/*
		Do not handle rollback if both inventories are that of the same player
	*/
	bool ignore_rollback = (
		from_inv.type == InventoryLocation::PLAYER &&
		from_inv == to_inv);

	/*
		Collect information of endpoints
	*/

	int try_take_count = count;
	if (try_take_count == 0)
		try_take_count = list_from->getItem(from_i).count;

	int src_can_take_count = 0xffff;
	int dst_can_put_count = 0xffff;

	/* Query detached inventories */

	// Move occurs in the same detached inventory
	if (from_inv.type == InventoryLocation::DETACHED &&
			from_inv == to_inv) {
		src_can_take_count = PLAYER_TO_SA(player)->detached_inventory_AllowMove(
			*this, try_take_count, player);
		dst_can_put_count = src_can_take_count;
	} else {
		// Destination is detached
		if (to_inv.type == InventoryLocation::DETACHED) {
			ItemStack src_item = list_from->getItem(from_i);
			src_item.count = try_take_count;
			dst_can_put_count = PLAYER_TO_SA(player)->detached_inventory_AllowPut(
				*this, src_item, player);
		}
		// Source is detached
		if (from_inv.type == InventoryLocation::DETACHED) {
			ItemStack src_item = list_from->getItem(from_i);
			src_item.count = try_take_count;
			src_can_take_count = PLAYER_TO_SA(player)->detached_inventory_AllowTake(
				*this, src_item, player);
		}
	}

	/* Query node metadata inventories */

	// Both endpoints are nodemeta
	// Move occurs in the same nodemeta inventory
	if (from_inv.type == InventoryLocation::NODEMETA &&
			from_inv == to_inv) {
		src_can_take_count = PLAYER_TO_SA(player)->nodemeta_inventory_AllowMove(
			*this, try_take_count, player);
		dst_can_put_count = src_can_take_count;
	} else {
		// Destination is nodemeta
		if (to_inv.type == InventoryLocation::NODEMETA) {
			ItemStack src_item = list_from->getItem(from_i);
			src_item.count = try_take_count;
			dst_can_put_count = PLAYER_TO_SA(player)->nodemeta_inventory_AllowPut(
				*this, src_item, player);
		}
		// Source is nodemeta
		if (from_inv.type == InventoryLocation::NODEMETA) {
			ItemStack src_item = list_from->getItem(from_i);
			src_item.count = try_take_count;
			src_can_take_count = PLAYER_TO_SA(player)->nodemeta_inventory_AllowTake(
				*this, src_item, player);
		}
	}

	// Query player inventories

	// Move occurs in the same player inventory
	if (from_inv.type == InventoryLocation::PLAYER &&
			from_inv == to_inv) {
		src_can_take_count = PLAYER_TO_SA(player)->player_inventory_AllowMove(
			*this, try_take_count, player);
		dst_can_put_count = src_can_take_count;
	} else {
		// Destination is a player
		if (to_inv.type == InventoryLocation::PLAYER) {
			ItemStack src_item = list_from->getItem(from_i);
			src_item.count = try_take_count;
			dst_can_put_count = PLAYER_TO_SA(player)->player_inventory_AllowPut(
				*this, src_item, player);
		}
		// Source is a player
		if (from_inv.type == InventoryLocation::PLAYER) {
			ItemStack src_item = list_from->getItem(from_i);
			src_item.count = try_take_count;
			src_can_take_count = PLAYER_TO_SA(player)->player_inventory_AllowTake(
				*this, src_item, player);
		}
	}

	int old_count = count;

	/* Modify count according to collected data */
	count = try_take_count;
	if (src_can_take_count != -1 && count > src_can_take_count)
		count = src_can_take_count;
	if (dst_can_put_count != -1 && count > dst_can_put_count)
		count = dst_can_put_count;
	/* Limit according to source item count */
	if (count > list_from->getItem(from_i).count)
		count = list_from->getItem(from_i).count;

	/* If no items will be moved, don't go further */
	if (count == 0) {
		// Undo client prediction. See 'clientApply'
		if (from_inv.type == InventoryLocation::PLAYER)
			list_from->setModified();

		if (to_inv.type == InventoryLocation::PLAYER)
			list_to->setModified();

		infostream<<"IMoveAction::apply(): move was completely disallowed:"
				<<" count="<<old_count
				<<" from inv=\""<<from_inv.dump()<<"\""
				<<" list=\""<<from_list<<"\""
				<<" i="<<from_i
				<<" to inv=\""<<to_inv.dump()<<"\""
				<<" list=\""<<to_list<<"\""
				<<" i="<<to_i
				<<std::endl;
		return;
	}

	ItemStack src_item = list_from->getItem(from_i);
	src_item.count = count;
	ItemStack from_stack_was = list_from->getItem(from_i);
	ItemStack to_stack_was = list_to->getItem(to_i);

	/*
		Perform actual move

		If something is wrong (source item is empty, destination is the
		same as source), nothing happens
	*/
	bool did_swap = false;
	move_count = list_from->moveItem(from_i,
		list_to, to_i, count, !caused_by_move_somewhere, &did_swap);

	// If source is infinite, reset it's stack
	if (src_can_take_count == -1) {
		// For the caused_by_move_somewhere == true case we didn't force-put the item,
		// which guarantees there is no leftover, and code below would duplicate the
		// (not replaced) to_stack_was item.
		if (!caused_by_move_somewhere) {
			// If destination stack is of different type and there are leftover
			// items, attempt to put the leftover items to a different place in the
			// destination inventory.
			// The client-side GUI will try to guess if this happens.
			if (from_stack_was.name != to_stack_was.name) {
				for (u32 i = 0; i < list_to->getSize(); i++) {
					if (list_to->getItem(i).empty()) {
						list_to->changeItem(i, to_stack_was);
						break;
					}
				}
			}
		}
		if (move_count > 0 || did_swap) {
			list_from->deleteItem(from_i);
			list_from->addItem(from_i, from_stack_was);
		}
	}
	// If destination is infinite, reset it's stack and take count from source
	if (dst_can_put_count == -1) {
		list_to->deleteItem(to_i);
		list_to->addItem(to_i, to_stack_was);
		list_from->deleteItem(from_i);
		list_from->addItem(from_i, from_stack_was);
		list_from->takeItem(from_i, count);
	}

	infostream << "IMoveAction::apply(): moved"
			<< " msom=" << move_somewhere
			<< " caused=" << caused_by_move_somewhere
			<< " count=" << count
			<< " from inv=\"" << from_inv.dump() << "\""
			<< " list=\"" << from_list << "\""
			<< " i=" << from_i
			<< " to inv=\"" << to_inv.dump() << "\""
			<< " list=\"" << to_list << "\""
			<< " i=" << to_i
			<< std::endl;

	// If we are inside the move somewhere loop, we don't need to report
	// anything if nothing happened (perhaps we don't need to report
	// anything for caused_by_move_somewhere == true, but this way its safer)
	if (caused_by_move_somewhere && move_count == 0)
		return;

	/*
		Record rollback information
	*/
	if (!ignore_rollback && gamedef->rollback()) {
		IRollbackManager *rollback = gamedef->rollback();

		// If source is not infinite, record item take
		if (src_can_take_count != -1) {
			RollbackAction action;
			std::string loc;
			{
				std::ostringstream os(std::ios::binary);
				from_inv.serialize(os);
				loc = os.str();
			}
			action.setModifyInventoryStack(loc, from_list, from_i, false,
					src_item);
			rollback->reportAction(action);
		}
		// If destination is not infinite, record item put
		if (dst_can_put_count != -1) {
			RollbackAction action;
			std::string loc;
			{
				std::ostringstream os(std::ios::binary);
				to_inv.serialize(os);
				loc = os.str();
			}
			action.setModifyInventoryStack(loc, to_list, to_i, true,
					src_item);
			rollback->reportAction(action);
		}
	}

	/*
		Report move to endpoints
	*/

	/* Detached inventories */

	// Both endpoints are same detached
	if (from_inv.type == InventoryLocation::DETACHED &&
			from_inv == to_inv) {
		PLAYER_TO_SA(player)->detached_inventory_OnMove(
				*this, count, player);
	} else {
		// Destination is detached
		if (to_inv.type == InventoryLocation::DETACHED) {
			PLAYER_TO_SA(player)->detached_inventory_OnPut(
				*this, src_item, player);
		}
		// Source is detached
		if (from_inv.type == InventoryLocation::DETACHED) {
			PLAYER_TO_SA(player)->detached_inventory_OnTake(
				*this, src_item, player);
		}
	}

	/* Node metadata inventories */

	// Both endpoints are same nodemeta
	if (from_inv.type == InventoryLocation::NODEMETA &&
			from_inv == to_inv) {
		PLAYER_TO_SA(player)->nodemeta_inventory_OnMove(
			*this, count, player);
	} else {
		// Destination is nodemeta
		if (to_inv.type == InventoryLocation::NODEMETA) {
			PLAYER_TO_SA(player)->nodemeta_inventory_OnPut(
				*this, src_item, player);
		}
		// Source is nodemeta
		if (from_inv.type == InventoryLocation::NODEMETA) {
			PLAYER_TO_SA(player)->nodemeta_inventory_OnTake(
				*this, src_item, player);
		}
	}

	// Player inventories

	// Both endpoints are same player inventory
	if (from_inv.type == InventoryLocation::PLAYER &&
			from_inv == to_inv) {
		PLAYER_TO_SA(player)->player_inventory_OnMove(
			*this, count, player);
	} else {
		// Destination is player inventory
		if (to_inv.type == InventoryLocation::PLAYER) {
			PLAYER_TO_SA(player)->player_inventory_OnPut(
				*this, src_item, player);
		}
		// Source is player inventory
		if (from_inv.type == InventoryLocation::PLAYER) {
			PLAYER_TO_SA(player)->player_inventory_OnTake(
				*this, src_item, player);
		}
	}

	mgr->setInventoryModified(from_inv);
	if (inv_from != inv_to)
		mgr->setInventoryModified(to_inv);
}

void IMoveAction::clientApply(InventoryManager *mgr, IGameDef *gamedef)
{
	// Optional InventoryAction operation that is run on the client
	// to make lag less apparent.

	Inventory *inv_from = mgr->getInventory(from_inv);
	Inventory *inv_to = mgr->getInventory(to_inv);
	if (!inv_from || !inv_to)
		return;

	InventoryLocation current_player;
	current_player.setCurrentPlayer();
	Inventory *inv_player = mgr->getInventory(current_player);
	if (inv_from != inv_player || inv_to != inv_player)
		return;

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);
	if (!list_from || !list_to)
		return;

	if (!move_somewhere)
		list_from->moveItem(from_i, list_to, to_i, count);
	else
		list_from->moveItemSomewhere(from_i, list_to, count);

	mgr->setInventoryModified(from_inv);
	if (inv_from != inv_to)
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

	if (!inv_from) {
		infostream<<"IDropAction::apply(): FAIL: source inventory not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if (!list_from) {
		infostream<<"IDropAction::apply(): FAIL: source list not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""<<std::endl;
		return;
	}
	if (list_from->getItem(from_i).empty()) {
		infostream<<"IDropAction::apply(): FAIL: source item not found: "
				<<"from_inv=\""<<from_inv.dump()<<"\""
				<<", from_list=\""<<from_list<<"\""
				<<" from_i="<<from_i<<std::endl;
		return;
	}

	/*
		Do not handle rollback if inventory is player's
	*/
	bool ignore_src_rollback = (from_inv.type == InventoryLocation::PLAYER);

	/*
		Collect information of endpoints
	*/

	int take_count = list_from->getItem(from_i).count;
	if (count != 0 && count < take_count)
		take_count = count;
	int src_can_take_count = take_count;

	ItemStack src_item = list_from->getItem(from_i);
	src_item.count = take_count;

	// Run callbacks depending on source inventory
	switch (from_inv.type) {
	case InventoryLocation::DETACHED:
		src_can_take_count = PLAYER_TO_SA(player)->detached_inventory_AllowTake(
			*this, src_item, player);
		break;
	case InventoryLocation::NODEMETA:
		src_can_take_count = PLAYER_TO_SA(player)->nodemeta_inventory_AllowTake(
			*this, src_item, player);
		break;
	case InventoryLocation::PLAYER:
		src_can_take_count = PLAYER_TO_SA(player)->player_inventory_AllowTake(
			*this, src_item, player);
		break;
	default:
		break;
	}

	if (src_can_take_count != -1 && src_can_take_count < take_count)
		take_count = src_can_take_count;

	// Update item due executed callbacks
	src_item = list_from->getItem(from_i);

	// Drop the item
	ItemStack item1 = list_from->getItem(from_i);
	item1.count = take_count;
	if(PLAYER_TO_SA(player)->item_OnDrop(item1, player,
				player->getBasePosition())) {
		int actually_dropped_count = take_count - item1.count;

		if (actually_dropped_count == 0) {
			infostream<<"Actually dropped no items"<<std::endl;

			// Revert client prediction. See 'clientApply'
			if (from_inv.type == InventoryLocation::PLAYER)
				list_from->setModified();
			return;
		}

		// If source isn't infinite
		if (src_can_take_count != -1) {
			// Take item from source list
			ItemStack item2 = list_from->takeItem(from_i, actually_dropped_count);

			if (item2.count != actually_dropped_count)
				errorstream<<"Could not take dropped count of items"<<std::endl;
		}

		src_item.count = actually_dropped_count;
		mgr->setInventoryModified(from_inv);
	}

	infostream<<"IDropAction::apply(): dropped "
			<<" from inv=\""<<from_inv.dump()<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<std::endl;


	/*
		Report drop to endpoints
	*/

	switch (from_inv.type) {
	case InventoryLocation::DETACHED:
		PLAYER_TO_SA(player)->detached_inventory_OnTake(
			*this, src_item, player);
		break;
	case InventoryLocation::NODEMETA:
		PLAYER_TO_SA(player)->nodemeta_inventory_OnTake(
			*this, src_item, player);
		break;
	case InventoryLocation::PLAYER:
		PLAYER_TO_SA(player)->player_inventory_OnTake(
			*this, src_item, player);
		break;
	default:
		break;
	}

	/*
		Record rollback information
	*/
	if (!ignore_src_rollback && gamedef->rollback()) {
		IRollbackManager *rollback = gamedef->rollback();

		// If source is not infinite, record item take
		if (src_can_take_count != -1) {
			RollbackAction action;
			std::string loc;
			{
				std::ostringstream os(std::ios::binary);
				from_inv.serialize(os);
				loc = os.str();
			}
			action.setModifyInventoryStack(loc, from_list, from_i,
					false, src_item);
			rollback->reportAction(action);
		}
	}
}

void IDropAction::clientApply(InventoryManager *mgr, IGameDef *gamedef)
{
	// Optional InventoryAction operation that is run on the client
	// to make lag less apparent.

	Inventory *inv_from = mgr->getInventory(from_inv);
	if (!inv_from)
		return;

	InventoryLocation current_player;
	current_player.setCurrentPlayer();
	Inventory *inv_player = mgr->getInventory(current_player);
	if (inv_from != inv_player)
		return;

	InventoryList *list_from = inv_from->getList(from_list);
	if (!list_from)
		return;

	if (count == 0)
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

void ICraftAction::apply(InventoryManager *mgr,
	ServerActiveObject *player, IGameDef *gamedef)
{
	Inventory *inv_craft = mgr->getInventory(craft_inv);

	if (!inv_craft) {
		infostream << "ICraftAction::apply(): FAIL: inventory not found: "
				<< "craft_inv=\"" << craft_inv.dump() << "\"" << std::endl;
		return;
	}

	InventoryList *list_craft = inv_craft->getList("craft");
	InventoryList *list_craftresult = inv_craft->getList("craftresult");
	InventoryList *list_main = inv_craft->getList("main");

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if (!list_craft) {
		infostream << "ICraftAction::apply(): FAIL: craft list not found: "
				<< "craft_inv=\"" << craft_inv.dump() << "\"" << std::endl;
		return;
	}
	if (!list_craftresult) {
		infostream << "ICraftAction::apply(): FAIL: craftresult list not found: "
				<< "craft_inv=\"" << craft_inv.dump() << "\"" << std::endl;
		return;
	}
	if (list_craftresult->getSize() < 1) {
		infostream << "ICraftAction::apply(): FAIL: craftresult list too short: "
				<< "craft_inv=\"" << craft_inv.dump() << "\"" << std::endl;
		return;
	}

	ItemStack crafted;
	ItemStack craftresultitem;
	int count_remaining = count;
	std::vector<ItemStack> output_replacements;
	getCraftingResult(inv_craft, crafted, output_replacements, false, gamedef);
	PLAYER_TO_SA(player)->item_CraftPredict(crafted, player, list_craft, craft_inv);
	bool found = !crafted.empty();

	while (found && list_craftresult->itemFits(0, crafted)) {
		InventoryList saved_craft_list = *list_craft;

		std::vector<ItemStack> temp;
		// Decrement input and add crafting output
		getCraftingResult(inv_craft, crafted, temp, true, gamedef);
		PLAYER_TO_SA(player)->item_OnCraft(crafted, player, &saved_craft_list, craft_inv);
		list_craftresult->addItem(0, crafted);
		mgr->setInventoryModified(craft_inv);

		// Add the new replacements to the list
		IItemDefManager *itemdef = gamedef->getItemDefManager();
		for (auto &itemstack : temp) {
			for (auto &output_replacement : output_replacements) {
				if (itemstack.name == output_replacement.name) {
					itemstack = output_replacement.addItem(itemstack, itemdef);
					if (itemstack.empty())
						continue;
				}
			}
			output_replacements.push_back(itemstack);
		}

		actionstream << player->getDescription()
				<< " crafts "
				<< crafted.getItemString()
				<< std::endl;

		// Decrement counter
		if (count_remaining == 1)
			break;

		if (count_remaining > 1)
			count_remaining--;

		// Get next crafting result
		getCraftingResult(inv_craft, crafted, temp, false, gamedef);
		PLAYER_TO_SA(player)->item_CraftPredict(crafted, player, list_craft, craft_inv);
		found = !crafted.empty();
	}

	// Put the replacements in the inventory or drop them on the floor, if
	// the inventory is full
	for (auto &output_replacement : output_replacements) {
		if (list_main)
			output_replacement = list_main->addItem(output_replacement);
		if (output_replacement.empty())
			continue;
		u16 count = output_replacement.count;
		do {
			PLAYER_TO_SA(player)->item_OnDrop(output_replacement, player,
				player->getBasePosition());
			if (count >= output_replacement.count) {
				errorstream << "Couldn't drop replacement stack " <<
					output_replacement.getItemString() << " because drop loop didn't "
					"decrease count." << std::endl;

				break;
			}
		} while (!output_replacement.empty());
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
bool getCraftingResult(Inventory *inv, ItemStack &result,
		std::vector<ItemStack> &output_replacements,
		bool decrementInput, IGameDef *gamedef)
{
	result.clear();

	// Get the InventoryList in which we will operate
	InventoryList *clist = inv->getList("craft");
	if (!clist)
		return false;

	// Mangle crafting grid to an another format
	CraftInput ci;
	ci.method = CRAFT_METHOD_NORMAL;
	ci.width = clist->getWidth() ? clist->getWidth() : 3;
	for (u16 i=0; i < clist->getSize(); i++)
		ci.items.push_back(clist->getItem(i));

	// Find out what is crafted and add it to result item slot
	CraftOutput co;
	bool found = gamedef->getCraftDefManager()->getCraftResult(
			ci, co, output_replacements, decrementInput, gamedef);
	if (found)
		result.deSerialize(co.item, gamedef->getItemDefManager());

	if (found && decrementInput) {
		// CraftInput has been changed, apply changes in clist
		for (u16 i=0; i < clist->getSize(); i++) {
			clist->changeItem(i, ci.items[i]);
		}
	}

	return found;
}

