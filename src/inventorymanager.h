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

#ifndef INVENTORYMANAGER_HEADER
#define INVENTORYMANAGER_HEADER

#include "inventory.h"
#include <iostream>
#include <string>
class ServerActiveObject;

struct InventoryLocation
{
	enum Type{
		UNDEFINED,
		CURRENT_PLAYER,
		PLAYER,
		NODEMETA,
        DETACHED,
	} type;

	std::string name; // PLAYER, DETACHED
	v3s16 p; // NODEMETA

	InventoryLocation()
	{
		setUndefined();
	}
	void setUndefined()
	{
		type = UNDEFINED;
	}
	void setCurrentPlayer()
	{
		type = CURRENT_PLAYER;
	}
	void setPlayer(const std::string &name_)
	{
		type = PLAYER;
		name = name_;
	}
	void setNodeMeta(v3s16 p_)
	{
		type = NODEMETA;
		p = p_;
	}
	void setDetached(const std::string &name_)
	{
		type = DETACHED;
		name = name_;
	}

	bool operator==(const InventoryLocation &other) const
	{
		if(type != other.type)
			return false;
		switch(type){
		case UNDEFINED:
			return false;
		case CURRENT_PLAYER:
			return true;
		case PLAYER:
			return (name == other.name);
		case NODEMETA:
			return (p == other.p);
		case DETACHED:
			return (name == other.name);
		}
		return false;
	}
	bool operator!=(const InventoryLocation &other) const
	{
		return !(*this == other);
	}

	void applyCurrentPlayer(const std::string &name_)
	{
		if(type == CURRENT_PLAYER)
			setPlayer(name_);
	}

	std::string dump() const;
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
	void deSerialize(std::string s);
};

struct InventoryAction;

class InventoryManager
{
public:
	InventoryManager(){}
	virtual ~InventoryManager(){}
	
	// Get an inventory (server and client)
	virtual Inventory* getInventory(const InventoryLocation &loc){return NULL;}
    // Set modified (will be saved and sent over network; only on server)
	virtual void setInventoryModified(const InventoryLocation &loc){}
    // Send inventory action to server (only on client)
	virtual void inventoryAction(InventoryAction *a){}
};

#define IACTION_MOVE 0
#define IACTION_DROP 1
#define IACTION_CRAFT 2

struct InventoryAction
{
	static InventoryAction * deSerialize(std::istream &is);
	
	virtual u16 getType() const = 0;
	virtual void serialize(std::ostream &os) const = 0;
	virtual void apply(InventoryManager *mgr, ServerActiveObject *player,
			IGameDef *gamedef) = 0;
	virtual void clientApply(InventoryManager *mgr, IGameDef *gamedef) = 0;
	virtual ~InventoryAction() {};
};

struct IMoveAction : public InventoryAction
{
	// count=0 means "everything"
	u16 count;
	InventoryLocation from_inv;
	std::string from_list;
	s16 from_i;
	InventoryLocation to_inv;
	std::string to_list;
	s16 to_i;
	
	IMoveAction()
	{
		count = 0;
		from_i = -1;
		to_i = -1;
	}
	
	IMoveAction(std::istream &is);

	u16 getType() const
	{
		return IACTION_MOVE;
	}

	void serialize(std::ostream &os) const
	{
		os<<"Move ";
		os<<count<<" ";
		os<<from_inv.dump()<<" ";
		os<<from_list<<" ";
		os<<from_i<<" ";
		os<<to_inv.dump()<<" ";
		os<<to_list<<" ";
		os<<to_i;
	}

	void apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef);

	void clientApply(InventoryManager *mgr, IGameDef *gamedef);
};

struct IDropAction : public InventoryAction
{
	// count=0 means "everything"
	u16 count;
	InventoryLocation from_inv;
	std::string from_list;
	s16 from_i;
	
	IDropAction()
	{
		count = 0;
		from_i = -1;
	}
	
	IDropAction(std::istream &is);

	u16 getType() const
	{
		return IACTION_DROP;
	}

	void serialize(std::ostream &os) const
	{
		os<<"Drop ";
		os<<count<<" ";
		os<<from_inv.dump()<<" ";
		os<<from_list<<" ";
		os<<from_i;
	}

	void apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef);

	void clientApply(InventoryManager *mgr, IGameDef *gamedef);
};

struct ICraftAction : public InventoryAction
{
	// count=0 means "everything"
	u16 count;
	InventoryLocation craft_inv;
	
	ICraftAction()
	{
		count = 0;
	}
	
	ICraftAction(std::istream &is);

	u16 getType() const
	{
		return IACTION_CRAFT;
	}

	void serialize(std::ostream &os) const
	{
		os<<"Craft ";
		os<<count<<" ";
		os<<craft_inv.dump()<<" ";
	}

	void apply(InventoryManager *mgr, ServerActiveObject *player, IGameDef *gamedef);

	void clientApply(InventoryManager *mgr, IGameDef *gamedef);
};

// Crafting helper
bool getCraftingResult(Inventory *inv, ItemStack& result,
		bool decrementInput, IGameDef *gamedef);

#endif

