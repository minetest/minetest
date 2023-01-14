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
#include "server/serveractiveobject.h"
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

static void recordRollbackAction(const MoveAction &ma, bool put,
	const ItemStack &item, IGameDef *gamedef)
{
	if (IRollbackManager *rollback = gamedef->rollback()) {
		RollbackAction action;
		std::string loc;
		{
			std::ostringstream os(std::ios::binary);
			(put ? ma.to_inv : ma.from_inv).serialize(os);
			loc = os.str();
		}
		action.setModifyInventoryStack(loc, put ? ma.to_list : ma.from_list, put ? ma.to_i : ma.from_i, put, item);
		rollback->reportAction(action);
	}
}

/*
	IMoveAction
*/

IMoveAction::IMoveAction(std::istream &is, bool somewhere) :
		move_somewhere(somewhere)
{
	std::string ts;

	std::getline(is, ts, ' ');
	action_count = stoi(ts);

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

void IMoveAction::onPutAndOnTake(const ItemStack &item, ServerActiveObject *player) const
{
	ServerScripting *sa = PLAYER_TO_SA(player);
	if (to_inv.type == InventoryLocation::DETACHED)
		sa->detached_inventory_OnPut(*this, item, player);
	else if (to_inv.type == InventoryLocation::NODEMETA)
		sa->nodemeta_inventory_OnPut(*this, item, player);
	else if (to_inv.type == InventoryLocation::PLAYER)
		sa->player_inventory_OnPut(*this, item, player);
	else
		assert(false);

	if (from_inv.type == InventoryLocation::DETACHED)
		sa->detached_inventory_OnTake(*this, item, player);
	else if (from_inv.type == InventoryLocation::NODEMETA)
		sa->nodemeta_inventory_OnTake(*this, item, player);
	else if (from_inv.type == InventoryLocation::PLAYER)
		sa->player_inventory_OnTake(*this, item, player);
	else
		assert(false);
}

void IMoveAction::onMove(int count, ServerActiveObject *player) const
{
	ServerScripting *sa = PLAYER_TO_SA(player);
	if (from_inv.type == InventoryLocation::DETACHED)
		sa->detached_inventory_OnMove(*this, count, player);
	else if (from_inv.type == InventoryLocation::NODEMETA)
		sa->nodemeta_inventory_OnMove(*this, count, player);
	else if (from_inv.type == InventoryLocation::PLAYER)
		sa->player_inventory_OnMove(*this, count, player);
	else
		assert(false);
}

int IMoveAction::allowPut(const ItemStack &item, ServerActiveObject *player) const
{
	ServerScripting *sa = PLAYER_TO_SA(player);
	int can_put_count = 0xffff;
	if (to_inv.type == InventoryLocation::DETACHED)
		can_put_count = sa->detached_inventory_AllowPut(*this, item, player);
	else if (to_inv.type == InventoryLocation::NODEMETA)
		can_put_count = sa->nodemeta_inventory_AllowPut(*this, item, player);
	else if (to_inv.type == InventoryLocation::PLAYER)
		can_put_count = sa->player_inventory_AllowPut(*this, item, player);
	else
		assert(false);
	return can_put_count;
}

int IMoveAction::allowTake(const ItemStack &item, ServerActiveObject *player) const
{
	ServerScripting *sa = PLAYER_TO_SA(player);
	int can_take_count = 0xffff;
	if (from_inv.type == InventoryLocation::DETACHED)
		can_take_count = sa->detached_inventory_AllowTake(*this, item, player);
	else if (from_inv.type == InventoryLocation::NODEMETA)
		can_take_count = sa->nodemeta_inventory_AllowTake(*this, item, player);
	else if (from_inv.type == InventoryLocation::PLAYER)
		can_take_count = sa->player_inventory_AllowTake(*this, item, player);
	else
		assert(false);
	return can_take_count;
}

int IMoveAction::allowMove(int try_take_count, ServerActiveObject *player) const
{
	ServerScripting *sa = PLAYER_TO_SA(player);
	int can_take_count = 0xffff;
	if (from_inv.type == InventoryLocation::DETACHED)
		can_take_count = sa->detached_inventory_AllowMove(*this, try_take_count, player);
	else if (from_inv.type == InventoryLocation::NODEMETA)
		can_take_count = sa->nodemeta_inventory_AllowMove(*this, try_take_count, player);
	else if (from_inv.type == InventoryLocation::PLAYER)
		can_take_count = sa->player_inventory_AllowMove(*this, try_take_count, player);
	else
		assert(false);
	return can_take_count;
}

// Returns {moved_count, can_swap}
std::pair<s16, bool> IMoveAction::move(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef,
	InventoryList *list_from, InventoryList *list_to, u16 count, bool ignore_rollback) const
{
	ItemStack src_item = list_from->getItem(from_i);

	if (src_item.empty())
		return {0, false};

	if (count == 0 || src_item.count < count)
		count = src_item.count;

	bool inf_src = false;
	bool inf_dst = false;
	u16 action_count = count;

	// Collect information of endpoints
	if (from_inv == to_inv) {
		// Move action within the same inventory
		int can_take_count = allowMove(count, player);
		if (can_take_count >= 0 && can_take_count < count)
			count = can_take_count;
	} else {
		src_item.count = count;
		int can_take_count = allowTake(src_item, player);
		if (can_take_count == -1)
			inf_src = true;
		else if (can_take_count >= 0 && can_take_count < count)
			count = can_take_count;

		if (count > 0) {
			src_item.count = count;
			int can_put_count = allowPut(src_item, player);
			if (can_put_count == -1)
				inf_dst = true;
			else if (can_put_count >= 0 && can_put_count < count)
				count = can_put_count;
		}
	}

	ItemStack orig_src_item = list_from->getItem(from_i);
	ItemStack orig_dst_item = list_to->getItem(to_i);

	u16 moved_count = 0;
	if (count > 0)
		moved_count = list_from->moveItem(from_i, list_to, to_i, count);

	infostream << "IMoveAction::apply(): moved " << moved_count << " of " << count << " items:"
		<< " action_count=" << action_count
		<< " from inv=\"" << from_inv.dump() << "\""
		<< " list=\"" << from_list << "\""
		<< " i=" << from_i
		<< " inf_src=" << inf_src
		<< " to inv=\"" << to_inv.dump() << "\""
		<< " list=\"" << to_list << "\""
		<< " i=" << to_i
		<< " inf_dst=" << inf_dst
		<< std::endl;

	if (moved_count == 0)
		return {0, count == action_count}; // Can swap only full stack

	// If source or dest are meant to be infinite, undo the change
	if (inf_src)
		list_from->changeItem(from_i, orig_src_item);
	if (inf_dst)
		list_to->changeItem(to_i, orig_dst_item);

	ItemStack item_to_record = list_to->getItem(to_i);
	item_to_record.count = moved_count;

	// Record rollback information
	if (!ignore_rollback && !inf_src)
		recordRollbackAction(*this, false, item_to_record, gamedef);

	if (!ignore_rollback && !inf_dst)
		recordRollbackAction(*this, true, item_to_record, gamedef);

	// Report move to endpoints
	if (from_inv == to_inv) {
		onMove(moved_count, player);
		mgr->setInventoryModified(from_inv);
	} else {
		onPutAndOnTake(item_to_record, player);
		mgr->setInventoryModified(from_inv);
		mgr->setInventoryModified(to_inv);
	}

	return {moved_count, false};
}

bool IMoveAction::swap(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef,
	InventoryList *list_from, InventoryList *list_to, bool ignore_rollback) const
{
	ItemStack to_item = list_to->getItem(to_i);
	if (to_item.empty())
		return false;

	IMoveAction swap_action;
	swap_action.from_inv = to_inv;
	swap_action.from_list = to_list;
	swap_action.from_i = to_i;
	swap_action.to_inv = from_inv;
	swap_action.to_list = from_list;
	swap_action.to_i = from_i;

	// From item was checked in move
	if (from_inv == to_inv) {
		int dst_can_take_count = swap_action.allowMove(to_item.count, player);
		if (dst_can_take_count >= 0 && dst_can_take_count < to_item.count)
			return false; // Can swap only full stack
	} else {
		int dst_can_take_count = swap_action.allowTake(to_item, player);
		if (dst_can_take_count >= 0 && dst_can_take_count < to_item.count)
			return false;
		int dst_can_put_count = swap_action.allowPut(to_item, player);
		if (dst_can_put_count >= 0 && dst_can_put_count < to_item.count)
			return false;
	}

	bool swapped = list_from->swapItem(from_i, list_to, to_i);
	infostream << "IMoveAction::apply(): " << (swapped ? "swapped" : "did not swap") << " items:"
		<< " from inv=\"" << from_inv.dump() << "\""
		<< " list=\"" <<  from_list << "\""
		<< " i=" <<  from_i
		<< " to inv=\"" <<  to_inv.dump() << "\""
		<< " list=\"" <<  to_list << "\""
		<< " i=" <<  to_i
		<< std::endl;

	if (!swapped)
		return false;

	// Callbacks could change the inventory, better to report what was actually swapped
	ItemStack from_item = list_to->getItem(to_i);
	to_item = list_from->getItem(from_i);

	// Record rollback information
	if (!ignore_rollback) {
		recordRollbackAction(*this, false, from_item, gamedef);
		recordRollbackAction(*this, true, from_item, gamedef);
		recordRollbackAction(swap_action, false, to_item, gamedef);
		recordRollbackAction(swap_action, true, to_item, gamedef);
	}

	// Report swap to endpoints
	if (from_inv == to_inv) {
		onMove(from_item.count, player);
		swap_action.onMove(to_item.count, player);
		mgr->setInventoryModified(from_inv);
	} else {
		onPutAndOnTake(from_item, player);
		swap_action.onPutAndOnTake(to_item, player);
		mgr->setInventoryModified(from_inv);
		mgr->setInventoryModified(to_inv);
	}

	return true;
}

void IMoveAction::apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef) const
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

	// If a list doesn't exist or the source item doesn't exist
	if (!list_from) {
		infostream << "IMoveAction::apply(): FAIL: source list not found: "
			<< "from_inv=\"" << from_inv.dump() << "\""
			<< ", from_list=\"" << from_list << "\"" << std::endl;
		return;
	}
	if (!list_to) {
		infostream << "IMoveAction::apply(): FAIL: destination list not found: "
			<< "to_inv=\"" << to_inv.dump() << "\""
			<< ", to_list=\"" << to_list << "\"" << std::endl;
		return;
	}

	if (from_i < 0 || list_from->getSize() <= (u32) from_i) {
		infostream << "IMoveAction::apply(): FAIL: source index out of bounds: "
			<< "size of from_list=\"" << list_from->getSize() << "\""
			<< ", from_index=\"" << from_i << "\"" << std::endl;
		return;
	}

	// Do not handle rollback if both inventories are that of the same player
	bool ignore_rollback = from_inv.type == InventoryLocation::PLAYER && from_inv == to_inv;

	if (move_somewhere) {
		infostream << "IMoveAction::apply(): moving item somewhere"
			<< " count=" << action_count
			<< " from inv=\"" << from_inv.dump() << "\""
			<< " list=\"" << from_list << "\""
			<< " i=" << from_i
			<< " to inv=\"" << to_inv.dump() << "\""
			<< " list=\"" << to_list << "\""
			<< std::endl;

		if (list_from == list_to)
			return;

		u16 count = action_count;
		if (count == 0) {
			ItemStack src_item = list_from->getItem(from_i);

			if (src_item.empty())
				return;

			count = src_item.count;
		}

		IMoveAction ma(*this);

		// Try to add the item to destination list
		s16 dest_size = list_to->getSize();

		// First try all the non-empty slots
		for (s16 dest_i = 0; dest_i < dest_size && count > 0; dest_i++) {
			if (!list_to->getItem(dest_i).empty()) {
				ma.to_i = dest_i;
				u16 moved_count;
				std::tie(moved_count, std::ignore) = ma.move(mgr, player, gamedef, list_from, list_to, count, ignore_rollback);
				count -= moved_count;
			}
		}

		// Then try all the empty ones
		for (s16 dest_i = 0; dest_i < dest_size && count > 0; dest_i++) {
			if (list_to->getItem(dest_i).empty()) {
				ma.to_i = dest_i;
				u16 moved_count;
				std::tie(moved_count, std::ignore) = ma.move(mgr, player, gamedef, list_from, list_to, count, ignore_rollback);
				count -= moved_count;
			}
		}
	} else {
		if (to_i < 0 || list_to->getSize() <= (u32) to_i) {
			infostream << "IMoveAction::apply(): FAIL: destination index out of bounds: "
				<< "size of to_list=\"" << list_to->getSize() << "\""
				<< ", to_index=\"" << to_i << "\"" << std::endl;
			return;
		}

		infostream << "IMoveAction::apply(): moving item"
			<< " count=" << action_count
			<< " from inv=\"" << from_inv.dump() << "\""
			<< " list=\"" << from_list << "\""
			<< " i=" << from_i
			<< " to inv=\"" << to_inv.dump() << "\""
			<< " list=\"" << to_list << "\""
			<< " i=" << to_i
			<< std::endl;

		if (list_from == list_to && from_i == to_i)
			return;

		bool allow_swap;
		std::tie(std::ignore, allow_swap) = move(mgr, player, gamedef, list_from, list_to, action_count, ignore_rollback);
		if (allow_swap) // Adding was not possible, try to swap
			swap(mgr, player, gamedef, list_from, list_to, ignore_rollback);
	}
}

void IMoveAction::clientApply(InventoryManager *mgr, IGameDef *gamedef) const
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
		list_from->moveOrSwapItem(from_i, list_to, to_i, action_count);
	else
		list_from->moveItemSomewhere(from_i, list_to, action_count);

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

void IDropAction::apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef) const
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
	if (!ignore_src_rollback && src_can_take_count != -1) {
		recordRollbackAction(*this, false, src_item, gamedef);
	}
}

void IDropAction::clientApply(InventoryManager *mgr, IGameDef *gamedef) const
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
	ServerActiveObject *player, IGameDef *gamedef) const
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

void ICraftAction::clientApply(InventoryManager *mgr, IGameDef *gamedef) const
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
