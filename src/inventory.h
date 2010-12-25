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
#include "mapblockobject.h"
// For g_materials
#include "main.h"

#define QUANTITY_ITEM_MAX_COUNT 99

class InventoryItem
{
public:
	InventoryItem(u16 count);
	virtual ~InventoryItem();
	
	static InventoryItem* deSerialize(std::istream &is);
	
	virtual const char* getName() const = 0;
	// Shall write the name and the parameters
	virtual void serialize(std::ostream &os) = 0;
	// Shall make an exact clone of the item
	virtual InventoryItem* clone() = 0;
#ifndef SERVER
	// Shall return an image to show in the GUI (or NULL)
	virtual video::ITexture * getImage() { return NULL; }
#endif
	// Shall return a text to show in the GUI
	virtual std::string getText() { return ""; }

	// Shall return true if the item can be add()ed to the other
	virtual bool addableTo(InventoryItem *other)
	{
		return false;
	}
	
	/*
		Quantity methods
	*/
	u16 getCount()
	{
		return m_count;
	}
	void setCount(u16 count)
	{
		m_count = count;
	}
	virtual u16 freeSpace()
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

protected:
	u16 m_count;
};

class MaterialItem : public InventoryItem
{
public:
	MaterialItem(u8 content, u16 count):
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
	virtual void serialize(std::ostream &os)
	{
		//os.imbue(std::locale("C"));
		os<<getName();
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
	video::ITexture * getImage()
	{
		/*if(m_content == CONTENT_TORCH)
			return g_texturecache.get("torch_on_floor");

		u16 tile = content_tile(m_content, v3s16(1,0,0));
		return g_tile_contents[tile].getTexture(0);*/
		
		if(m_content >= USEFUL_CONTENT_COUNT)
			return NULL;
			
		return g_irrlicht->getTexture(g_content_inventory_textures[m_content]);
	}
#endif
	std::string getText()
	{
		std::ostringstream os;
		os<<m_count;
		return os.str();
	}

	virtual bool addableTo(InventoryItem *other)
	{
		if(std::string(other->getName()) != "MaterialItem")
			return false;
		MaterialItem *m = (MaterialItem*)other;
		if(m->getMaterial() != m_content)
			return false;
		return true;
	}
	u16 freeSpace()
	{
		if(m_count > QUANTITY_ITEM_MAX_COUNT)
			return 0;
		return QUANTITY_ITEM_MAX_COUNT - m_count;
	}
	/*
		Special methods
	*/
	u8 getMaterial()
	{
		return m_content;
	}
private:
	u8 m_content;
};

class MapBlockObjectItem : public InventoryItem
{
public:
	MapBlockObjectItem(std::string inventorystring):
		InventoryItem(1)
	{
		m_inventorystring = inventorystring;
	}
	
	/*
		Implementation interface
	*/
	virtual const char* getName() const
	{
		return "MBOItem";
	}
	virtual void serialize(std::ostream &os)
	{
		for(;;)
		{
			size_t t = m_inventorystring.find('|');
			if(t == std::string::npos)
				break;
			m_inventorystring[t] = '?';
		}
		os<<getName();
		os<<" ";
		os<<m_inventorystring;
		os<<"|";
	}
	virtual InventoryItem* clone()
	{
		return new MapBlockObjectItem(m_inventorystring);
	}

#ifndef SERVER
	video::ITexture * getImage();
#endif
	std::string getText();

	/*
		Special methods
	*/
	std::string getInventoryString()
	{
		return m_inventorystring;
	}

	MapBlockObject * createObject(v3f pos, f32 player_yaw, f32 player_pitch);

private:
	std::string m_inventorystring;
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
	virtual void serialize(std::ostream &os)
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
	video::ITexture * getImage()
	{
		std::string basename;
		if(m_subname == "Stick")
			basename = "../data/stick.png";
		// Default to cloud texture
		else
			basename = tile_texture_path_get(TILE_CLOUD);
		
		// Get such a texture
		return g_irrlicht->getTexture(basename);
		//return g_irrlicht->getTexture(TextureSpec(finalname, basename, mod));
	}
#endif
	std::string getText()
	{
		std::ostringstream os;
		os<<m_count;
		return os.str();
	}
	virtual bool addableTo(InventoryItem *other)
	{
		if(std::string(other->getName()) != "CraftItem")
			return false;
		CraftItem *m = (CraftItem*)other;
		if(m->m_subname != m_subname)
			return false;
		return true;
	}
	u16 freeSpace()
	{
		if(m_count > QUANTITY_ITEM_MAX_COUNT)
			return 0;
		return QUANTITY_ITEM_MAX_COUNT - m_count;
	}
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
	virtual void serialize(std::ostream &os)
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
	video::ITexture * getImage()
	{
		std::string basename;
		if(m_toolname == "WPick")
			basename = "../data/tool_wpick.png";
		else if(m_toolname == "STPick")
			basename = "../data/tool_stpick.png";
		else if(m_toolname == "MesePick")
			basename = "../data/tool_mesepick.png";
		// Default to cloud texture
		else
			basename = tile_texture_path_get(TILE_CLOUD);
		
		/*
			Calculate some progress value with sane amount of
			maximum states
		*/
		u32 maxprogress = 30;
		u32 toolprogress = (65535-m_wear)/(65535/maxprogress);
		
		// Make texture name for the new texture with a progress bar
		std::ostringstream os;
		os<<basename<<"-toolprogress-"<<toolprogress;
		std::string finalname = os.str();

		float value_f = (float)toolprogress / (float)maxprogress;
		
		// Get such a texture
		TextureMod *mod = new ProgressBarTextureMod(value_f);
		return g_irrlicht->getTexture(TextureSpec(finalname, basename, mod));
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
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);

	InventoryList(const InventoryList &other);
	InventoryList & operator = (const InventoryList &other);

	std::string getName();
	u32 getSize();
	// Count used slots
	u32 getUsedSlots();
	
	// Get pointer to item
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
};

class Inventory
{
public:
	~Inventory();

	void clear();

	Inventory();
	Inventory(const Inventory &other);
	Inventory & operator = (const Inventory &other);
	
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);

	InventoryList * addList(const std::string &name, u32 size);
	InventoryList * getList(const std::string &name);
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
	s32 getListIndex(const std::string &name);

	core::array<InventoryList*> m_lists;
};

#define IACTION_MOVE 0

struct InventoryAction
{
	static InventoryAction * deSerialize(std::istream &is);
	
	virtual u16 getType() const = 0;
	virtual void serialize(std::ostream &os) = 0;
	virtual void apply(Inventory *inventory) = 0;
};

struct IMoveAction : public InventoryAction
{
	// count=0 means "everything"
	u16 count;
	std::string from_name;
	s16 from_i;
	std::string to_name;
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

		std::getline(is, from_name, ' ');

		std::getline(is, ts, ' ');
		from_i = stoi(ts);

		std::getline(is, to_name, ' ');

		std::getline(is, ts, ' ');
		to_i = stoi(ts);
	}

	u16 getType() const
	{
		return IACTION_MOVE;
	}

	void serialize(std::ostream &os)
	{
		os<<"Move ";
		os<<count<<" ";
		os<<from_name<<" ";
		os<<from_i<<" ";
		os<<to_name<<" ";
		os<<to_i;
	}

	void apply(Inventory *inventory);
};

#endif

