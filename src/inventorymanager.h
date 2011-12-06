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

#ifndef INVENTORYMANAGER_HEADER
#define INVENTORYMANAGER_HEADER

#include "inventory.h"

// Should probably somehow replace InventoryContext over time
struct InventoryLocation
{
	enum Type{
		UNDEFINED,
		PLAYER,
		NODEMETA
	} type;

	std::string name; // PLAYER
	v3s16 p; // NODEMETA

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
};

class Player;

struct InventoryContext
{
	Player *current_player;
	
	InventoryContext():
		current_player(NULL)
	{}
};

struct InventoryAction;

class InventoryManager
{
public:
	InventoryManager(){}
	virtual ~InventoryManager(){}
	
	// Get an inventory or set it modified (so it will be updated over
	// network or so)
	virtual Inventory* getInventory(const InventoryLocation &loc){return NULL;}
	virtual void setInventoryModified(const InventoryLocation &loc){}

	// Used on the client to send an action to the server
	virtual void inventoryAction(InventoryAction *a){}

	// (Deprecated; these wrap to the latter ones)
	// Get a pointer to an inventory specified by id. id can be:
	// - "current_player"
	// - "nodemeta:X,Y,Z"
	Inventory* getInventory(InventoryContext *c, std::string id);
	// Used on the server by InventoryAction::apply and other stuff
	void inventoryModified(InventoryContext *c, std::string id);
};

#define IACTION_MOVE 0
#define IACTION_DROP 1

struct InventoryAction
{
	static InventoryAction * deSerialize(std::istream &is);
	
	virtual u16 getType() const = 0;
	virtual void serialize(std::ostream &os) const = 0;
	virtual void apply(InventoryContext *c, InventoryManager *mgr,
			ServerEnvironment *env) = 0;
};

struct IMoveAction : public InventoryAction
{
	// count=0 means "everything"
	u16 count;
	std::string from_inv;
	std::string from_list;
	s16 from_i;
	std::string to_inv;
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
		os<<from_inv<<" ";
		os<<from_list<<" ";
		os<<from_i<<" ";
		os<<to_inv<<" ";
		os<<to_list<<" ";
		os<<to_i;
	}

	void apply(InventoryContext *c, InventoryManager *mgr,
			ServerEnvironment *env);
};

struct IDropAction : public InventoryAction
{
	// count=0 means "everything"
	u16 count;
	std::string from_inv;
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
		os<<from_inv<<" ";
		os<<from_list<<" ";
		os<<from_i;
	}

	void apply(InventoryContext *c, InventoryManager *mgr,
			ServerEnvironment *env);
};

/*
	Craft checking system
*/

enum ItemSpecType
{
	ITEM_NONE,
	ITEM_MATERIAL,
	ITEM_CRAFT,
	ITEM_TOOL,
	ITEM_MBO
};

struct ItemSpec
{
	enum ItemSpecType type;
	// Only other one of these is used
	std::string name;
	u16 num;

	ItemSpec():
		type(ITEM_NONE)
	{
	}
	ItemSpec(enum ItemSpecType a_type, std::string a_name):
		type(a_type),
		name(a_name),
		num(65535)
	{
	}
	ItemSpec(enum ItemSpecType a_type, u16 a_num):
		type(a_type),
		name(""),
		num(a_num)
	{
	}

	bool checkItem(const InventoryItem *item) const;
};

/*
	items: a pointer to an array of 9 pointers to items
	specs: a pointer to an array of 9 ItemSpecs
*/
bool checkItemCombination(const InventoryItem * const*items, const ItemSpec *specs);

/*
	items: a pointer to an array of 9 pointers to items
	specs: a pointer to an array of 9 pointers to items
*/
bool checkItemCombination(const InventoryItem * const * items,
		const InventoryItem * const * specs);


#endif

