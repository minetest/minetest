/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "rollback_interface.h"
#include <sstream>
#include "util/serialize.h"
#include "util/string.h"
#include "util/numeric.h"
#include "map.h"
#include "gamedef.h"
#include "nodedef.h"
#include "nodemetadata.h"
#include "exceptions.h"
#include "log.h"
#include "inventorymanager.h"
#include "inventory.h"
#include "mapblock.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

RollbackNode::RollbackNode(Map *map, v3s16 p, IGameDef *gamedef)
{
	INodeDefManager *ndef = gamedef->ndef();
	MapNode n = map->getNodeNoEx(p);
	name = ndef->get(n).name;
	param1 = n.param1;
	param2 = n.param2;
	NodeMetadata *metap = map->getNodeMetadata(p);
	if(metap){
		std::ostringstream os(std::ios::binary);
		metap->serialize(os);
		meta = os.str();
	}
}

std::string RollbackAction::toString() const
{
	switch(type){
	case TYPE_SET_NODE: {
		std::ostringstream os(std::ios::binary);
		os<<"[set_node";
		os<<" ";
		os<<"("<<itos(p.X)<<","<<itos(p.Y)<<","<<itos(p.Z)<<")";
		os<<" ";
		os<<serializeJsonString(n_old.name);
		os<<" ";
		os<<itos(n_old.param1);
		os<<" ";
		os<<itos(n_old.param2);
		os<<" ";
		os<<serializeJsonString(n_old.meta);
		os<<" ";
		os<<serializeJsonString(n_new.name);
		os<<" ";
		os<<itos(n_new.param1);
		os<<" ";
		os<<itos(n_new.param2);
		os<<" ";
		os<<serializeJsonString(n_new.meta);
		os<<"]";
		return os.str(); }
	case TYPE_MODIFY_INVENTORY_STACK: {
		std::ostringstream os(std::ios::binary);
		os<<"[modify_inventory_stack";
		os<<" ";
		os<<serializeJsonString(inventory_location);
		os<<" ";
		os<<serializeJsonString(inventory_list);
		os<<" ";
		os<<inventory_index;
		os<<" ";
		os<<(inventory_add?"add":"remove");
		os<<" ";
		os<<serializeJsonString(inventory_stack);
		os<<"]";
		return os.str(); }
	default:
		return "none";
	}
}

void RollbackAction::fromStream(std::istream &is) throw(SerializationError)
{
	int c = is.get();
	if(c != '['){
		is.putback(c);
		throw SerializationError("RollbackAction: starting [ not found");
	}
	
	std::string id;
	std::getline(is, id, ' ');
	
	if(id == "set_node")
	{
		c = is.get();
		if(c != '('){
			is.putback(c);
			throw SerializationError("RollbackAction: starting ( not found");
		}
		// Position
		std::string px_raw;
		std::string py_raw;
		std::string pz_raw;
		std::getline(is, px_raw, ',');
		std::getline(is, py_raw, ',');
		std::getline(is, pz_raw, ')');
		c = is.get();
		if(c != ' '){
			is.putback(c);
			throw SerializationError("RollbackAction: after-p ' ' not found");
		}
		v3s16 loaded_p(stoi(px_raw), stoi(py_raw), stoi(pz_raw));
		// Old node
		std::string old_name;
		try{
			old_name = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"old_name: "<<e.what()<<std::endl;
			throw e;
		}
		c = is.get();
		if(c != ' '){
			is.putback(c);
			throw SerializationError("RollbackAction: after-old_name ' ' not found");
		}
		std::string old_p1_raw;
		std::string old_p2_raw;
		std::getline(is, old_p1_raw, ' ');
		std::getline(is, old_p2_raw, ' ');
		int old_p1 = stoi(old_p1_raw);
		int old_p2 = stoi(old_p2_raw);
		std::string old_meta;
		try{
			old_meta = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"old_meta: "<<e.what()<<std::endl;
			throw e;
		}
		c = is.get();
		if(c != ' '){
			is.putback(c);
			throw SerializationError("RollbackAction: after-old_meta ' ' not found");
		}
		// New node
		std::string new_name;
		try{
			new_name = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"new_name: "<<e.what()<<std::endl;
			throw e;
		}
		c = is.get();
		if(c != ' '){
			is.putback(c);
			throw SerializationError("RollbackAction: after-new_name ' ' not found");
		}
		std::string new_p1_raw;
		std::string new_p2_raw;
		std::getline(is, new_p1_raw, ' ');
		std::getline(is, new_p2_raw, ' ');
		int new_p1 = stoi(new_p1_raw);
		int new_p2 = stoi(new_p2_raw);
		std::string new_meta;
		try{
			new_meta = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"new_meta: "<<e.what()<<std::endl;
			throw e;
		}
		c = is.get();
		if(c != ']'){
			is.putback(c);
			throw SerializationError("RollbackAction: after-new_meta ] not found");
		}
		// Set values
		type = TYPE_SET_NODE;
		p = loaded_p;
		n_old.name = old_name;
		n_old.param1 = old_p1;
		n_old.param2 = old_p2;
		n_old.meta = old_meta;
		n_new.name = new_name;
		n_new.param1 = new_p1;
		n_new.param2 = new_p2;
		n_new.meta = new_meta;
	}
	else if(id == "modify_inventory_stack")
	{
		// Location
		std::string location;
		try{
			location = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"location: "<<e.what()<<std::endl;
			throw e;
		}
		c = is.get();
		if(c != ' '){
			is.putback(c);
			throw SerializationError("RollbackAction: after-loc ' ' not found");
		}
		// List
		std::string listname;
		try{
			listname = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"listname: "<<e.what()<<std::endl;
			throw e;
		}
		c = is.get();
		if(c != ' '){
			is.putback(c);
			throw SerializationError("RollbackAction: after-list ' ' not found");
		}
		// Index
		std::string index_raw;
		std::getline(is, index_raw, ' ');
		// add/remove
		std::string addremove;
		std::getline(is, addremove, ' ');
		if(addremove != "add" && addremove != "remove"){
			throw SerializationError("RollbackAction: addremove is not add or remove");
		}
		// Itemstring
		std::string stack;
		try{
			stack = deSerializeJsonString(is);
		}catch(SerializationError &e){
			errorstream<<"Serialization error in RollbackAction::fromStream(): "
					<<"stack: "<<e.what()<<std::endl;
			throw e;
		}
		// Set values
		type = TYPE_MODIFY_INVENTORY_STACK;
		inventory_location = location;
		inventory_list = listname;
		inventory_index = stoi(index_raw);
		inventory_add = (addremove == "add");
		inventory_stack = stack;
	}
	else
	{
		throw SerializationError("RollbackAction: Unknown id");
	}
}

bool RollbackAction::isImportant(IGameDef *gamedef) const
{
	switch(type){
	case TYPE_SET_NODE: {
		// If names differ, action is always important
		if(n_old.name != n_new.name)
			return true;
		// If metadata differs, action is always important
		if(n_old.meta != n_new.meta)
			return true;
		INodeDefManager *ndef = gamedef->ndef();
		// Both are of the same name, so a single definition is needed
		const ContentFeatures &def = ndef->get(n_old.name);
		// If the type is flowing liquid, action is not important
		if(def.liquid_type == LIQUID_FLOWING)
			return false;
		// Otherwise action is important
		return true; }
	default:
		return true;
	}
}

bool RollbackAction::getPosition(v3s16 *dst) const
{
	switch(type){
	case RollbackAction::TYPE_SET_NODE:
		if(dst) *dst = p;
		return true;
	case RollbackAction::TYPE_MODIFY_INVENTORY_STACK: {
		InventoryLocation loc;
		loc.deSerialize(inventory_location);
		if(loc.type != InventoryLocation::NODEMETA)
			return false;
		if(dst) *dst = loc.p;
		return true; }
	default:
		return false;
	}
}

bool RollbackAction::applyRevert(Map *map, InventoryManager *imgr, IGameDef *gamedef) const
{
	try{
		switch(type){
		case TYPE_NOTHING:
			return true;
		case TYPE_SET_NODE: {
			INodeDefManager *ndef = gamedef->ndef();
			// Make sure position is loaded from disk
			map->emergeBlock(getContainerPos(p, MAP_BLOCKSIZE), false);
			// Check current node
			MapNode current_node = map->getNodeNoEx(p);
			std::string current_name = ndef->get(current_node).name;
			// If current node not the new node, it's bad
			if(current_name != n_new.name)
				return false;
			/*// If current node not the new node and not ignore, it's bad
			if(current_name != n_new.name && current_name != "ignore")
				return false;*/
			// Create rollback node
			MapNode n(ndef, n_old.name, n_old.param1, n_old.param2);
			// Set rollback node
			try{
				if(!map->addNodeWithEvent(p, n)){
					infostream<<"RollbackAction::applyRevert(): "
							<<"AddNodeWithEvent failed at "
							<<PP(p)<<" for "<<n_old.name<<std::endl;
					return false;
				}
				NodeMetadata *meta = map->getNodeMetadata(p);
				if(n_old.meta != ""){
					if(!meta){
						meta = new NodeMetadata(gamedef);
						map->setNodeMetadata(p, meta);
					}
					std::istringstream is(n_old.meta, std::ios::binary);
					meta->deSerialize(is);
				} else {
					map->removeNodeMetadata(p);
				}
				// NOTE: This same code is in scriptapi.cpp
				// Inform other things that the metadata has changed
				v3s16 blockpos = getContainerPos(p, MAP_BLOCKSIZE);
				MapEditEvent event;
				event.type = MEET_BLOCK_NODE_METADATA_CHANGED;
				event.p = blockpos;
				map->dispatchEvent(&event);
				// Set the block to be saved
				MapBlock *block = map->getBlockNoCreateNoEx(blockpos);
				if(block)
					block->raiseModified(MOD_STATE_WRITE_NEEDED,
							"NodeMetaRef::reportMetadataChange");
			}catch(InvalidPositionException &e){
				infostream<<"RollbackAction::applyRevert(): "
						<<"InvalidPositionException: "<<e.what()<<std::endl;
				return false;
			}
			// Success
			return true; }
		case TYPE_MODIFY_INVENTORY_STACK: {
			InventoryLocation loc;
			loc.deSerialize(inventory_location);
			ItemStack stack;
			stack.deSerialize(inventory_stack, gamedef->idef());
			Inventory *inv = imgr->getInventory(loc);
			if(!inv){
				infostream<<"RollbackAction::applyRevert(): Could not get "
						"inventory at "<<inventory_location<<std::endl;
				return false;
			}
			InventoryList *list = inv->getList(inventory_list);
			if(!list){
				infostream<<"RollbackAction::applyRevert(): Could not get "
						"inventory list \""<<inventory_list<<"\" in "
						<<inventory_location<<std::endl;
				return false;
			}
			if(list->getSize() <= inventory_index){
				infostream<<"RollbackAction::applyRevert(): List index "
						<<inventory_index<<" too large in "
						<<"inventory list \""<<inventory_list<<"\" in "
						<<inventory_location<<std::endl;
			}
			// If item was added, take away item, otherwise add removed item
			if(inventory_add){
				// Silently ignore different current item
				if(list->getItem(inventory_index).name != stack.name)
					return false;
				list->takeItem(inventory_index, stack.count);
			} else {
				list->addItem(inventory_index, stack);
			}
			// Inventory was modified; send to clients
			imgr->setInventoryModified(loc);
			return true; }
		default:
			errorstream<<"RollbackAction::applyRevert(): type not handled"
					<<std::endl;
			return false;
		}
	}catch(SerializationError &e){
		errorstream<<"RollbackAction::applyRevert(): n_old.name="<<n_old.name
				<<", SerializationError: "<<e.what()<<std::endl;
	}
	return false;
}

