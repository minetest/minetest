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

#ifndef INVENTORY_HEADER
#define INVENTORY_HEADER

#include <iostream>
#include <sstream>
#include <string>
#include "common_irrlicht.h"
#include "debug.h"
#include "main.h" // For g_materials
#include "mapnode.h" // For content_t

#define QUANTITY_ITEM_MAX_COUNT 99

class ServerActiveObject;
class ServerEnvironment;
class Player;

class InventoryItem
{
public:
	InventoryItem(u16 count);
	virtual ~InventoryItem();
	
	static InventoryItem* deSerialize(std::istream &is);
	
	virtual const char* getName() const = 0;
	// Shall write the name and the parameters
	virtual void serialize(std::ostream &os) const = 0;
	// Shall make an exact clone of the item
	virtual InventoryItem* clone() = 0;
#ifndef SERVER
	// Return the name of the image for this item
	virtual std::string getBasename() const { return ""; }
	// Shall return an image of the item (or NULL)
	virtual video::ITexture * getImage() const { return NULL; }
	// Shall return an image of the item without embellishments (or NULL)
	virtual video::ITexture * getImageRaw() const { return getImage(); }
#endif
	// Shall return a text to show in the GUI
	virtual std::string getText() { return ""; }
	// Returns the string used for inventory
	virtual std::string getItemString();
	// Creates an object from the item, to be placed in the world.
	virtual ServerActiveObject* createSAO(ServerEnvironment *env, u16 id, v3f pos);
	// Gets amount of items that dropping one SAO will decrement
	virtual u16 getDropCount() const { return getCount(); }

	/*
		Quantity methods
	*/

	// Shall return true if the item can be add()ed to the other
	virtual bool addableTo(const InventoryItem *other) const
	{
		return false;
	}
	
	u16 getCount() const
	{
		return m_count;
	}
	void setCount(u16 count)
	{
		m_count = count;
	}
	// This should return something else for stackable items
	virtual u16 freeSpace() const
	{
		return 0;
	}
	void add(u16 count)
	{
		assert(m_count + count <= QUANTITY_ITEM_MAX_COUNT);
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

	// Whether it can be cooked
	virtual bool isCookable() const {return false;}
	// Time of cooking
	virtual float getCookTime(){return 3.0;}
	// Result of cooking (can randomize)
	virtual InventoryItem *createCookResult() const {return NULL;}
	
	// Eat, press, activate, whatever.
	// Called when item is right-clicked when lying on ground.
	// If returns true, item shall be deleted.
	virtual bool use(ServerEnvironment *env,
			Player *player){return false;}

protected:
	u16 m_count;
};

class MaterialItem : public InventoryItem
{
public:
	MaterialItem(content_t content, u16 count):
		InventoryItem(count)
	{
		m_content = content;
	}
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "MaterialItem";
	}
	virtual void serialize(std::ostream &os) const
	{
		//os.imbue(std::locale("C"));
		os<<"MaterialItem2";
		os<<" ";
		os<<(unsigned int)m_content;
		os<<" ";
		os<<m_count;
	}
	virtual InventoryItem* clone()
	{
		return new MaterialItem(m_content, m_count);
	}
#ifndef SERVER
	video::ITexture * getImage() const
	{
		return content_features(m_content).inventory_texture;
	}
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
		if(m->getMaterial() != m_content)
			return false;
		return true;
	}
	u16 freeSpace() const
	{
		if(m_count > QUANTITY_ITEM_MAX_COUNT)
			return 0;
		return QUANTITY_ITEM_MAX_COUNT - m_count;
	}
	/*
		Other properties
	*/
	bool isCookable() const;
	InventoryItem *createCookResult() const;
	/*
		Special methods
	*/
	content_t getMaterial()
	{
		return m_content;
	}
private:
	content_t m_content;
};

/*
	An item that is used as a mid-product when crafting.
	Subnames:
	- Stick
*/
class CraftItem : public InventoryItem
{
public:
	CraftItem(std::string subname, u16 count):
		InventoryItem(count)
	{
		m_subname = subname;
	}
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "CraftItem";
	}
	virtual void serialize(std::ostream &os) const
	{
		os<<getName();
		os<<" ";
		os<<m_subname;
		os<<" ";
		os<<m_count;
	}
	virtual InventoryItem* clone()
	{
		return new CraftItem(m_subname, m_count);
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

	ServerActiveObject* createSAO(ServerEnvironment *env, u16 id, v3f pos);
	u16 getDropCount() const;

	virtual bool addableTo(const InventoryItem *other) const
	{
		if(std::string(other->getName()) != "CraftItem")
			return false;
		CraftItem *m = (CraftItem*)other;
		if(m->m_subname != m_subname)
			return false;
		return true;
	}
	u16 freeSpace() const
	{
		if(m_count > QUANTITY_ITEM_MAX_COUNT)
			return 0;
		return QUANTITY_ITEM_MAX_COUNT - m_count;
	}

	/*
		Other properties
	*/

	bool isCookable() const;
	InventoryItem *createCookResult() const;

	bool use(ServerEnvironment *env, Player *player);
	
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
	ToolItem(std::string toolname, u16 wear):
		InventoryItem(1)
	{
		m_toolname = toolname;
		m_wear = wear;
	}
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "ToolItem";
	}
	virtual void serialize(std::ostream &os) const
	{
		os<<getName();
		os<<" ";
		os<<m_toolname;
		os<<" ";
		os<<m_wear;
	}
	virtual InventoryItem* clone()
	{
		return new ToolItem(m_toolname, m_wear);
	}
#ifndef SERVER
	std::string getBasename() const {
		if(m_toolname == "WPick")
			return "tool_woodpick.png";
		else if(m_toolname == "STPick")
			return "tool_stonepick.png";
		else if(m_toolname == "SteelPick")
			return "tool_steelpick.png";
		else if(m_toolname == "MesePick")
			return "tool_mesepick.png";
		else if(m_toolname == "WShovel")
			return "tool_woodshovel.png";
		else if(m_toolname == "STShovel")
			return "tool_stoneshovel.png";
		else if(m_toolname == "SteelShovel")
			return "tool_steelshovel.png";
		else if(m_toolname == "WAxe")
			return "tool_woodaxe.png";
		else if(m_toolname == "STAxe")
			return "tool_stoneaxe.png";
		else if(m_toolname == "SteelAxe")
			return "tool_steelaxe.png";
		else if(m_toolname == "WSword")
			return "tool_woodsword.png";
		else if(m_toolname == "STSword")
			return "tool_stonesword.png";
		else if(m_toolname == "SteelSword")
			return "tool_steelsword.png";
		else
			return "cloud.png";
}
	
	video::ITexture * getImage() const
	{
		if(g_texturesource == NULL)
			return NULL;
		
		std::string basename = getBasename();
		
		/*
			Calculate a progress value with sane amount of
			maximum states
		*/
		u32 maxprogress = 30;
		u32 toolprogress = (65535-m_wear)/(65535/maxprogress);
		
		float value_f = (float)toolprogress / (float)maxprogress;
		std::ostringstream os;
		os<<basename<<"^[progressbar"<<value_f;

		return g_texturesource->getTextureRaw(os.str());
	}

	video::ITexture * getImageRaw() const
	{
		if(g_texturesource == NULL)
			return NULL;
		
		return g_texturesource->getTextureRaw(getBasename());
	}
#endif
	std::string getText()
	{
		return "";
		
		/*std::ostringstream os;
		u16 f = 4;
		u16 d = 65535/f;
		u16 i;
		for(i=0; i<(65535-m_wear)/d; i++)
			os<<'X';
		for(; i<f; i++)
			os<<'-';
		return os.str();*/
		
		/*std::ostringstream os;
		os<<m_toolname;
		os<<" ";
		os<<(m_wear/655);
		return os.str();*/
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
	void deSerialize(std::istream &is);

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
	void deSerialize(std::istream &is);

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

struct InventoryAction
{
	static InventoryAction * deSerialize(std::istream &is);
	
	virtual u16 getType() const = 0;
	virtual void serialize(std::ostream &os) const = 0;
	virtual void apply(InventoryContext *c, InventoryManager *mgr) = 0;
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
	IMoveAction(std::istream &is)
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

	void apply(InventoryContext *c, InventoryManager *mgr);
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

#endif

