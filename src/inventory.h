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

#ifndef INVENTORY_HEADER
#define INVENTORY_HEADER

#include <iostream>
#include <sstream>
#include <string>
#include "common_irrlicht.h"
#include "debug.h"
#include "mapnode.h" // For content_t

#define QUANTITY_ITEM_MAX_COUNT 99

class ServerActiveObject;
class ServerEnvironment;
struct PointedThing;
class ITextureSource;
class IGameDef;

class InventoryItem
{
public:
	InventoryItem(IGameDef *gamedef, u16 count);
	virtual ~InventoryItem();
	
	static InventoryItem* deSerialize(std::istream &is, IGameDef *gamedef);
	static InventoryItem* deSerialize(const std::string &str,
			IGameDef *gamedef);
	
	virtual const char* getName() const = 0;
	// Shall write the name and the parameters
	virtual void serialize(std::ostream &os) const = 0;
	// Shall make an exact clone of the item
	virtual InventoryItem* clone() = 0;
	// Return the name of the image for this item
	virtual std::string getImageBasename() const { return ""; }
#ifndef SERVER
	// Shall return an image of the item (or NULL)
	virtual video::ITexture * getImage() const
		{ return NULL; }
	// Shall return an image of the item without embellishments (or NULL)
	virtual video::ITexture * getImageRaw() const
		{ return getImage(); }
#endif
	// Shall return a text to show in the GUI
	virtual std::string getText() { return ""; }
	// Returns the string used for inventory
	virtual std::string getItemString();
	
	// Shall return false if item is not known and cannot be used
	virtual bool isKnown() const { return true; }

	/*
		Quantity methods
	*/

	// Return true if the item can be add()ed to the other
	virtual bool addableTo(const InventoryItem *other) const
	{ return false; }
	// Return true if the other item contains this item
	virtual bool isSubsetOf(const InventoryItem *other) const
	{ return false; }
	// Remove the other item from this one if possible and return true
	// Return false if not possible
	virtual bool removeOther(const InventoryItem *other)
	{ return false; }
	
	u16 getCount() const
	{ return m_count; }
	void setCount(u16 count)
	{ m_count = count; }

	u16 freeSpace() const
	{
		u16 max = getStackMax();
		if(m_count > max)
			return 0;
		return max - m_count;
	}

	void add(u16 count)
	{
		m_count += count;
	}
	void remove(u16 count)
	{
		assert(m_count >= count);
		m_count -= count;
	}

	/*
		Other properties
	*/

	// Maximum size of a stack
	virtual u16 getStackMax() const {return 1;}
	// Whether it can be used
	virtual bool isUsable() const {return false;}
	// Whether it can be cooked
	virtual bool isCookable() const {return false;}
	// Result of cooking (can randomize)
	virtual InventoryItem *createCookResult() const {return NULL;}
	// Time of cooking
	virtual float getCookTime() const {return 3.0;}
	// Whether it can be burned (<0 = cannot be burned)
	virtual float getBurnTime() const {return -1;}
	// Gets amount of items that dropping one ItemSAO will decrement
	// -1 means as many as possible
	virtual s16 getDropCount() const { return -1; }
	// Whether this item can point to liquids
	virtual bool areLiquidsPointable() const { return false; }

	// Creates an object from the item and places it in the world.
	// If return value is true, item should be removed.
	virtual bool dropOrPlace(ServerEnvironment *env,
			ServerActiveObject *dropper,
			v3f pos, bool place, s16 count);

	// Eat, press, activate, whatever.
	// Called when item is left-clicked while in hand.
	// If returns true, item shall be deleted.
	virtual bool use(ServerEnvironment *env,
			ServerActiveObject *user,
			const PointedThing& pointed){return false;}

protected:
	IGameDef *m_gamedef;
	u16 m_count;
};

class MaterialItem : public InventoryItem
{
public:
	MaterialItem(IGameDef *gamedef, std::string nodename, u16 count);
	// Legacy constructor
	MaterialItem(IGameDef *gamedef, content_t content, u16 count);
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "MaterialItem";
	}
	virtual void serialize(std::ostream &os) const
	{
		os<<"node";
		os<<" \"";
		os<<m_nodename;
		os<<"\" ";
		os<<m_count;
	}
	virtual InventoryItem* clone()
	{
		return new MaterialItem(m_gamedef, m_nodename, m_count);
	}
#ifndef SERVER
	video::ITexture * getImage() const;
#endif
	std::string getText()
	{
		std::ostringstream os;
		os<<m_count;
		return os.str();
	}

	virtual bool addableTo(const InventoryItem *other) const
	{
		if(std::string(other->getName()) != "MaterialItem")
			return false;
		MaterialItem *m = (MaterialItem*)other;
		if(m->m_nodename != m_nodename)
			return false;
		return true;
	}
	virtual bool isSubsetOf(const InventoryItem *other) const
	{
		if(std::string(other->getName()) != "MaterialItem")
			return false;
		MaterialItem *m = (MaterialItem*)other;
		if(m->m_nodename != m_nodename)
			return false;
		return m_count <= m->m_count;
	}
	virtual bool removeOther(const InventoryItem *other)
	{
		if(!other->isSubsetOf(this))
			return false;
		MaterialItem *m = (MaterialItem*)other;
		m_count += m->m_count;
		return true;
	}

	u16 getStackMax() const
	{
		return QUANTITY_ITEM_MAX_COUNT;
	}

	/*
		Other properties
	*/
	bool isCookable() const;
	InventoryItem *createCookResult() const;
	float getCookTime() const;
	float getBurnTime() const;
	/*
		Special properties (not part of virtual interface)
	*/
	std::string getNodeName() const
	{ return m_nodename; }
	content_t getMaterial() const;
private:
	std::string m_nodename;
};

/*
	An item that is used as a mid-product when crafting.
	Subnames:
	- Stick
*/
class CraftItem : public InventoryItem
{
public:
	CraftItem(IGameDef *gamedef, std::string subname, u16 count);
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "CraftItem";
	}
	virtual void serialize(std::ostream &os) const
	{
		os<<"craft";
		os<<" \"";
		os<<m_subname;
		os<<"\" ";
		os<<m_count;
	}
	virtual InventoryItem* clone()
	{
		return new CraftItem(m_gamedef, m_subname, m_count);
	}
#ifndef SERVER
	video::ITexture * getImage() const;
#endif
	std::string getText()
	{
		std::ostringstream os;
		os<<m_count;
		return os.str();
	}

	virtual bool isKnown() const;

	virtual bool addableTo(const InventoryItem *other) const
	{
		if(std::string(other->getName()) != "CraftItem")
			return false;
		CraftItem *m = (CraftItem*)other;
		if(m->m_subname != m_subname)
			return false;
		return true;
	}
	virtual bool isSubsetOf(const InventoryItem *other) const
	{
		if(std::string(other->getName()) != "CraftItem")
			return false;
		CraftItem *m = (CraftItem*)other;
		if(m->m_subname != m_subname)
			return false;
		return m_count <= m->m_count;
	}
	virtual bool removeOther(const InventoryItem *other)
	{
		if(!other->isSubsetOf(this))
			return false;
		CraftItem *m = (CraftItem*)other;
		m_count += m->m_count;
		return true;
	}

	/*
		Other properties
	*/

	u16 getStackMax() const;
	bool isUsable() const;
	bool isCookable() const;
	InventoryItem *createCookResult() const;
	float getCookTime() const;
	float getBurnTime() const;
	s16 getDropCount() const;
	bool areLiquidsPointable() const;

	bool dropOrPlace(ServerEnvironment *env,
			ServerActiveObject *dropper,
			v3f pos, bool place, s16 count);
	bool use(ServerEnvironment *env,
			ServerActiveObject *user,
			const PointedThing& pointed);

	/*
		Special methods
	*/
	std::string getSubName()
	{
		return m_subname;
	}
private:
	std::string m_subname;
};

class ToolItem : public InventoryItem
{
public:
	ToolItem(IGameDef *gamedef, std::string toolname, u16 wear);
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "ToolItem";
	}
	virtual void serialize(std::ostream &os) const
	{
		os<<"tool";
		os<<" \"";
		os<<m_toolname;
		os<<"\" ";
		os<<m_wear;
	}
	virtual InventoryItem* clone()
	{
		return new ToolItem(m_gamedef, m_toolname, m_wear);
	}

	std::string getImageBasename() const;
#ifndef SERVER
	video::ITexture * getImage() const;
	video::ITexture * getImageRaw() const;
#endif

	std::string getText()
	{
		return "";
	}
	
	virtual bool isKnown() const;

	virtual bool isSubsetOf(const InventoryItem *other) const
	{
		if(std::string(other->getName()) != "ToolItem")
			return false;
		ToolItem *m = (ToolItem*)other;
		if(m->m_toolname != m_toolname)
			return false;
		return m_wear <= m->m_wear;
	}
	virtual bool removeOther(const InventoryItem *other)
	{
		if(!other->isSubsetOf(this))
			return false;
		ToolItem *m = (ToolItem*)other;
		m_wear -= m->m_wear;
		return true;
	}

	/*
		Special methods
	*/
	std::string getToolName()
	{
		return m_toolname;
	}
	u16 getWear()
	{
		return m_wear;
	}
	// Returns true if weared out
	bool addWear(u16 add)
	{
		if(m_wear >= 65535 - add)
		{
			m_wear = 65535;
			return true;
		}
		else
		{
			m_wear += add;
			return false;
		}
	}
private:
	std::string m_toolname;
	u16 m_wear;
};

class InventoryList
{
public:
	InventoryList(std::string name, u32 size);
	~InventoryList();
	void clearItems();
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is, IGameDef *gamedef);

	InventoryList(const InventoryList &other);
	InventoryList & operator = (const InventoryList &other);

	const std::string &getName() const;
	u32 getSize();
	// Count used slots
	u32 getUsedSlots();
	u32 getFreeSlots();

	/*bool getDirty(){ return m_dirty; }
	void setDirty(bool dirty=true){ m_dirty = dirty; }*/
	
	// Get pointer to item
	const InventoryItem * getItem(u32 i) const;
	InventoryItem * getItem(u32 i);
	// Returns old item (or NULL). Parameter can be NULL.
	InventoryItem * changeItem(u32 i, InventoryItem *newitem);
	// Delete item
	void deleteItem(u32 i);

	// Adds an item to a suitable place. Returns leftover item.
	// If all went into the list, returns NULL.
	InventoryItem * addItem(InventoryItem *newitem);

	// If possible, adds item to given slot.
	// If cannot be added at all, returns the item back.
	// If can be added partly, decremented item is returned back.
	// If can be added fully, NULL is returned.
	InventoryItem * addItem(u32 i, InventoryItem *newitem);

	// Checks whether the item could be added to the given slot
	bool itemFits(const u32 i, const InventoryItem *newitem);

	// Checks whether there is room for a given item
	bool roomForItem(const InventoryItem *item);

	// Checks whether there is room for a given item aftr it has been cooked
	bool roomForCookedItem(const InventoryItem *item);

	// Takes some items from a slot.
	// If there are not enough, takes as many as it can.
	// Returns NULL if couldn't take any.
	InventoryItem * takeItem(u32 i, u32 count);

	// Decrements amount of every material item
	void decrementMaterials(u16 count);

	void print(std::ostream &o);
	
private:
	core::array<InventoryItem*> m_items;
	u32 m_size;
	std::string m_name;
	//bool m_dirty;
};

class Inventory
{
public:
	~Inventory();

	void clear();

	Inventory();
	Inventory(const Inventory &other);
	Inventory & operator = (const Inventory &other);
	
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is, IGameDef *gamedef);

	InventoryList * addList(const std::string &name, u32 size);
	InventoryList * getList(const std::string &name);
	const InventoryList * getList(const std::string &name) const;
	bool deleteList(const std::string &name);
	// A shorthand for adding items.
	// Returns NULL if the item was fully added, leftover otherwise.
	InventoryItem * addItem(const std::string &listname, InventoryItem *newitem)
	{
		InventoryList *list = getList(listname);
		if(list == NULL)
			return newitem;
		return list->addItem(newitem);
	}
	
private:
	// -1 if not found
	const s32 getListIndex(const std::string &name) const;

	core::array<InventoryList*> m_lists;
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
	
	/*
		Get a pointer to an inventory specified by id.
		id can be:
		- "current_player"
		- "nodemeta:X,Y,Z"
	*/
	virtual Inventory* getInventory(InventoryContext *c, std::string id)
		{return NULL;}
	// Used on the server by InventoryAction::apply and other stuff
	virtual void inventoryModified(InventoryContext *c, std::string id)
		{}
	// Used on the client
	virtual void inventoryAction(InventoryAction *a)
		{}
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

