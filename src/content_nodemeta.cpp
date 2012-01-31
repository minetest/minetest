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

#include "content_nodemeta.h"

#include <map>
#include "inventory.h"
#include "log.h"
#include "utility.h"
#include "craftdef.h"
#include "gamedef.h"

class Inventory;

#define NODEMETA_GENERIC 1
#define NODEMETA_SIGN 14
#define NODEMETA_CHEST 15
#define NODEMETA_FURNACE 16
#define NODEMETA_LOCKABLE_CHEST 17

core::map<u16, NodeMetadata::Factory> NodeMetadata::m_types;
core::map<std::string, NodeMetadata::Factory2> NodeMetadata::m_names;

class SignNodeMetadata : public NodeMetadata
{
public:
	SignNodeMetadata(IGameDef *gamedef, std::string text);
	//~SignNodeMetadata();
	
	virtual u16 typeId() const;
	virtual const char* typeName() const
	{ return "sign"; }
	static NodeMetadata* create(std::istream &is, IGameDef *gamedef);
	static NodeMetadata* create(IGameDef *gamedef);
	virtual NodeMetadata* clone(IGameDef *gamedef);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();

	virtual bool allowsTextInput(){ return true; }
	virtual std::string getText(){ return m_text; }
	virtual void setText(const std::string &t){ m_text = t; }

private:
	std::string m_text;
};

class ChestNodeMetadata : public NodeMetadata
{
public:
	ChestNodeMetadata(IGameDef *gamedef);
	~ChestNodeMetadata();
	
	virtual u16 typeId() const;
	virtual const char* typeName() const
	{ return "chest"; }
	static NodeMetadata* create(std::istream &is, IGameDef *gamedef);
	static NodeMetadata* create(IGameDef *gamedef);
	virtual NodeMetadata* clone(IGameDef *gamedef);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual std::string getInventoryDrawSpecString();
	
private:
	Inventory *m_inventory;
};

class LockingChestNodeMetadata : public NodeMetadata
{
public:
	LockingChestNodeMetadata(IGameDef *gamedef);
	~LockingChestNodeMetadata();

	virtual u16 typeId() const;
	virtual const char* typeName() const
	{ return "locked_chest"; }
	static NodeMetadata* create(std::istream &is, IGameDef *gamedef);
	static NodeMetadata* create(IGameDef *gamedef);
	virtual NodeMetadata* clone(IGameDef *gamedef);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual std::string getInventoryDrawSpecString();

	virtual std::string getOwner(){ return m_text; }
	virtual void setOwner(std::string t){ m_text = t; }

private:
	Inventory *m_inventory;
	std::string m_text;
};

class FurnaceNodeMetadata : public NodeMetadata
{
public:
	FurnaceNodeMetadata(IGameDef *gamedef);
	~FurnaceNodeMetadata();
	
	virtual u16 typeId() const;
	virtual const char* typeName() const
	{ return "furnace"; }
	virtual NodeMetadata* clone(IGameDef *gamedef);
	static NodeMetadata* create(std::istream &is, IGameDef *gamedef);
	static NodeMetadata* create(IGameDef *gamedef);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual void inventoryModified();
	virtual bool step(float dtime);
	virtual bool nodeRemovalDisabled();
	virtual std::string getInventoryDrawSpecString();
	
protected:
	bool getCookResult(bool remove, std::string &cookresult, float &cooktime);
	bool getBurnResult(bool remove, float &burntime);

private:
	Inventory *m_inventory;
	std::string m_infotext;
	float m_step_accumulator;
	float m_fuel_totaltime;
	float m_fuel_time;
	float m_src_totaltime;
	float m_src_time;
};

/*
	SignNodeMetadata
*/

// Prototype
SignNodeMetadata proto_SignNodeMetadata(NULL, "");

SignNodeMetadata::SignNodeMetadata(IGameDef *gamedef, std::string text):
	NodeMetadata(gamedef),
	m_text(text)
{
	NodeMetadata::registerType(typeId(), typeName(), create, create);
}
u16 SignNodeMetadata::typeId() const
{
	return NODEMETA_SIGN;
}
NodeMetadata* SignNodeMetadata::create(std::istream &is, IGameDef *gamedef)
{
	std::string text = deSerializeString(is);
	return new SignNodeMetadata(gamedef, text);
}
NodeMetadata* SignNodeMetadata::create(IGameDef *gamedef)
{
	return new SignNodeMetadata(gamedef, "");
}
NodeMetadata* SignNodeMetadata::clone(IGameDef *gamedef)
{
	return new SignNodeMetadata(gamedef, m_text);
}
void SignNodeMetadata::serializeBody(std::ostream &os)
{
	os<<serializeString(m_text);
}
std::string SignNodeMetadata::infoText()
{
	return std::string("\"")+m_text+"\"";
}

/*
	ChestNodeMetadata
*/

// Prototype
ChestNodeMetadata proto_ChestNodeMetadata(NULL);

ChestNodeMetadata::ChestNodeMetadata(IGameDef *gamedef):
	NodeMetadata(gamedef)
{
	NodeMetadata::registerType(typeId(), typeName(), create, create);
	m_inventory = NULL;
}
ChestNodeMetadata::~ChestNodeMetadata()
{
	delete m_inventory;
}
u16 ChestNodeMetadata::typeId() const
{
	return NODEMETA_CHEST;
}
NodeMetadata* ChestNodeMetadata::create(std::istream &is, IGameDef *gamedef)
{
	ChestNodeMetadata *d = new ChestNodeMetadata(gamedef);
	d->m_inventory = new Inventory(gamedef->idef());
	d->m_inventory->deSerialize(is);
	return d;
}
NodeMetadata* ChestNodeMetadata::create(IGameDef *gamedef)
{
	ChestNodeMetadata *d = new ChestNodeMetadata(gamedef);
	d->m_inventory = new Inventory(gamedef->idef());
	d->m_inventory->addList("0", 8*4);
	return d;
}
NodeMetadata* ChestNodeMetadata::clone(IGameDef *gamedef)
{
	ChestNodeMetadata *d = new ChestNodeMetadata(gamedef);
	d->m_inventory = new Inventory(*m_inventory);
	return d;
}
void ChestNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
}
std::string ChestNodeMetadata::infoText()
{
	return "Chest";
}
bool ChestNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if chest contains something
	*/
	InventoryList *list = m_inventory->getList("0");
	if(list == NULL)
		return false;
	if(list->getUsedSlots() == 0)
		return false;
	return true;
}
std::string ChestNodeMetadata::getInventoryDrawSpecString()
{
	return
		"invsize[8,9;]"
		"list[current_name;0;0,0;8,4;]"
		"list[current_player;main;0,5;8,4;]";
}

/*
	LockingChestNodeMetadata
*/

// Prototype
LockingChestNodeMetadata proto_LockingChestNodeMetadata(NULL);

LockingChestNodeMetadata::LockingChestNodeMetadata(IGameDef *gamedef):
	NodeMetadata(gamedef)
{
	NodeMetadata::registerType(typeId(), typeName(), create, create);
	m_inventory = NULL;
}
LockingChestNodeMetadata::~LockingChestNodeMetadata()
{
	delete m_inventory;
}
u16 LockingChestNodeMetadata::typeId() const
{
	return NODEMETA_LOCKABLE_CHEST;
}
NodeMetadata* LockingChestNodeMetadata::create(std::istream &is, IGameDef *gamedef)
{
	LockingChestNodeMetadata *d = new LockingChestNodeMetadata(gamedef);
	d->setOwner(deSerializeString(is));
	d->m_inventory = new Inventory(gamedef->idef());
	d->m_inventory->deSerialize(is);
	return d;
}
NodeMetadata* LockingChestNodeMetadata::create(IGameDef *gamedef)
{
	LockingChestNodeMetadata *d = new LockingChestNodeMetadata(gamedef);
	d->m_inventory = new Inventory(gamedef->idef());
	d->m_inventory->addList("0", 8*4);
	return d;
}
NodeMetadata* LockingChestNodeMetadata::clone(IGameDef *gamedef)
{
	LockingChestNodeMetadata *d = new LockingChestNodeMetadata(gamedef);
	d->m_inventory = new Inventory(*m_inventory);
	return d;
}
void LockingChestNodeMetadata::serializeBody(std::ostream &os)
{
	os<<serializeString(m_text);
	m_inventory->serialize(os);
}
std::string LockingChestNodeMetadata::infoText()
{
	return "Locking Chest";
}
bool LockingChestNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if chest contains something
	*/
	InventoryList *list = m_inventory->getList("0");
	if(list == NULL)
		return false;
	if(list->getUsedSlots() == 0)
		return false;
	return true;
}
std::string LockingChestNodeMetadata::getInventoryDrawSpecString()
{
	return
		"invsize[8,9;]"
		"list[current_name;0;0,0;8,4;]"
		"list[current_player;main;0,5;8,4;]";
}

/*
	FurnaceNodeMetadata
*/

// Prototype
FurnaceNodeMetadata proto_FurnaceNodeMetadata(NULL);

FurnaceNodeMetadata::FurnaceNodeMetadata(IGameDef *gamedef):
	NodeMetadata(gamedef)
{
	NodeMetadata::registerType(typeId(), typeName(), create, create);
	
	m_inventory = NULL;

	m_infotext = "Furnace is inactive";

	m_step_accumulator = 0;
	m_fuel_totaltime = 0;
	m_fuel_time = 0;
	m_src_totaltime = 0;
	m_src_time = 0;
}
FurnaceNodeMetadata::~FurnaceNodeMetadata()
{
	delete m_inventory;
}
u16 FurnaceNodeMetadata::typeId() const
{
	return NODEMETA_FURNACE;
}
NodeMetadata* FurnaceNodeMetadata::clone(IGameDef *gamedef)
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata(m_gamedef);
	d->m_inventory = new Inventory(*m_inventory);
	return d;
}
NodeMetadata* FurnaceNodeMetadata::create(std::istream &is, IGameDef *gamedef)
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata(gamedef);

	d->m_inventory = new Inventory(gamedef->idef());
	d->m_inventory->deSerialize(is);

	int temp = 0;
	is>>temp;
	d->m_fuel_totaltime = (float)temp/10;
	temp = 0;
	is>>temp;
	d->m_fuel_time = (float)temp/10;
	temp = 0;
	is>>temp;
	d->m_src_totaltime = (float)temp/10;
	temp = 0;
	is>>temp;
	d->m_src_time = (float)temp/10;

	if(is.eof())
	{
		// Old furnaces didn't serialize src_totaltime and src_time
		d->m_src_totaltime = 0;
		d->m_src_time = 0;
		d->m_infotext = "";
	}
	else
	{
		// New furnaces also serialize the infotext (so that the
		// client doesn't need to have the list of cooking recipes).
		d->m_infotext = deSerializeJsonString(is);
	}

	return d;
}
NodeMetadata* FurnaceNodeMetadata::create(IGameDef *gamedef)
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata(gamedef);
	d->m_inventory = new Inventory(gamedef->idef());
	d->m_inventory->addList("fuel", 1);
	d->m_inventory->addList("src", 1);
	d->m_inventory->addList("dst", 4);
	return d;
}
void FurnaceNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
	os<<itos(m_fuel_totaltime*10)<<" ";
	os<<itos(m_fuel_time*10)<<" ";
	os<<itos(m_src_totaltime*10)<<" ";
	os<<itos(m_src_time*10)<<" ";
	os<<serializeJsonString(m_infotext);
}
std::string FurnaceNodeMetadata::infoText()
{
	return m_infotext;
}
bool FurnaceNodeMetadata::nodeRemovalDisabled()
{
	/*
		Disable removal if furnace is not empty
	*/
	InventoryList *list[3] = {m_inventory->getList("src"),
	m_inventory->getList("dst"), m_inventory->getList("fuel")};
	
	for(int i = 0; i < 3; i++) {
		if(list[i] == NULL)
			continue;
		if(list[i]->getUsedSlots() == 0)
			continue;
		return true;
	}
	return false;
	
}
void FurnaceNodeMetadata::inventoryModified()
{
	infostream<<"Furnace inventory modification callback"<<std::endl;
}
bool FurnaceNodeMetadata::step(float dtime)
{
	if(dtime > 60.0)
		infostream<<"Furnace stepping a long time ("<<dtime<<")"<<std::endl;

	InventoryList *dst_list = m_inventory->getList("dst");
	assert(dst_list);

	// Update at a fixed frequency
	const float interval = 2.0;
	m_step_accumulator += dtime;
	bool changed = false;
	while(m_step_accumulator > interval)
	{
		m_step_accumulator -= interval;
		dtime = interval;

		//infostream<<"Furnace step dtime="<<dtime<<std::endl;

		bool changed_this_loop = false;

		// Check
		// 1. if the source item is cookable
		// 2. if there is room for the cooked item
		std::string cookresult;
		float cooktime;
		bool cookable = getCookResult(false, cookresult, cooktime);
		ItemStack cookresult_item;
		bool room_available = false;
		if(cookable)
		{
			cookresult_item.deSerialize(cookresult, m_gamedef->idef());
			room_available = dst_list->roomForItem(cookresult_item);
		}

		// Step fuel time
		bool burning = (m_fuel_time < m_fuel_totaltime);
		if(burning)
		{
			changed_this_loop = true;
			m_fuel_time += dtime;
		}

		std::string infotext;
		if(room_available)
		{
			float burntime;
			if(burning)
			{
				changed_this_loop = true;
				m_src_time += dtime;
				m_src_totaltime = cooktime;
				infotext = "Furnace is cooking";
			}
			else if(getBurnResult(true, burntime))
			{
				// Fuel inserted
				changed_this_loop = true;
				m_fuel_time = 0;
				m_fuel_totaltime = burntime;
				//m_src_time += dtime;
				//m_src_totaltime = cooktime;
				infotext = "Furnace is cooking";
			}
			else
			{
				m_src_time = 0;
				m_src_totaltime = 0;
				infotext = "Furnace is out of fuel";
			}
			if(m_src_totaltime > 0.001 && m_src_time >= m_src_totaltime)
			{
				// One item fully cooked
				changed_this_loop = true;
				dst_list->addItem(cookresult_item);
				getCookResult(true, cookresult, cooktime); // decrement source
				m_src_totaltime = 0;
				m_src_time = 0;
			}
		}
		else
		{
			// Not cookable or no room available
			m_src_totaltime = 0;
			m_src_time = 0;
			if(cookable)
				infotext = "Furnace is overloaded";
			else if(burning)
				infotext = "Furnace is active";
			else
			{
				infotext = "Furnace is inactive";
				m_fuel_totaltime = 0;
				m_fuel_time = 0;
			}
		}

		// Do this so it doesn't always show (0%) for weak fuel
		if(m_fuel_totaltime > 3) {
			infotext += " (";
			infotext += itos(m_fuel_time/m_fuel_totaltime*100);
			infotext += "%)";
		}

		if(infotext != m_infotext)
		{
			m_infotext = infotext;
			changed_this_loop = true;
		}

		if(burning && m_fuel_time >= m_fuel_totaltime)
		{
			m_fuel_time = 0;
			m_fuel_totaltime = 0;
		}

		if(changed_this_loop)
		{
			changed = true;
		}
		else
		{
			m_step_accumulator = 0;
			break;
		}
	}
	return changed;
}
std::string FurnaceNodeMetadata::getInventoryDrawSpecString()
{
	return
		"invsize[8,9;]"
		"list[current_name;fuel;2,3;1,1;]"
		"list[current_name;src;2,1;1,1;]"
		"list[current_name;dst;5,1;2,2;]"
		"list[current_player;main;0,5;8,4;]";
}
bool FurnaceNodeMetadata::getCookResult(bool remove,
		std::string &cookresult, float &cooktime)
{
	std::vector<ItemStack> items;
	InventoryList *src_list = m_inventory->getList("src");
	assert(src_list);
	items.push_back(src_list->getItem(0));

	CraftInput ci(CRAFT_METHOD_COOKING, 1, items);
	CraftOutput co;
	bool found = m_gamedef->getCraftDefManager()->getCraftResult(
			ci, co, remove, m_gamedef);
	if(remove)
		src_list->changeItem(0, ci.items[0]);

	cookresult = co.item;
	cooktime = co.time;
	return found;
}
bool FurnaceNodeMetadata::getBurnResult(bool remove, float &burntime)
{
	std::vector<ItemStack> items;
	InventoryList *fuel_list = m_inventory->getList("fuel");
	assert(fuel_list);
	items.push_back(fuel_list->getItem(0));

	CraftInput ci(CRAFT_METHOD_FUEL, 1, items);
	CraftOutput co;
	bool found = m_gamedef->getCraftDefManager()->getCraftResult(
			ci, co, remove, m_gamedef);
	if(remove)
		fuel_list->changeItem(0, ci.items[0]);

	burntime = co.time;
	return found;
}


/*
	GenericNodeMetadata
*/

class GenericNodeMetadata : public NodeMetadata
{
private:
	Inventory *m_inventory;
	std::string m_text;
	std::string m_owner;

	std::string m_infotext;
	std::string m_inventorydrawspec;
	bool m_allow_text_input;
	bool m_removal_disabled;
	bool m_enforce_owner;

	bool m_inventory_modified;
	bool m_text_modified;

	std::map<std::string, std::string> m_stringvars;

public:
	u16 typeId() const
	{
		return NODEMETA_GENERIC;
	}
	const char* typeName() const
	{
		return "generic";
	}

	GenericNodeMetadata(IGameDef *gamedef):
		NodeMetadata(gamedef),

		m_inventory(NULL),
		m_text(""),
		m_owner(""),

		m_infotext("GenericNodeMetadata"),
		m_inventorydrawspec(""),
		m_allow_text_input(false),
		m_removal_disabled(false),
		m_enforce_owner(false),

		m_inventory_modified(false),
		m_text_modified(false)
	{
		NodeMetadata::registerType(typeId(), typeName(), create, create);
	}
	virtual ~GenericNodeMetadata()
	{
		delete m_inventory;
	}
	NodeMetadata* clone(IGameDef *gamedef)
	{
		GenericNodeMetadata *d = new GenericNodeMetadata(m_gamedef);

		d->m_inventory = new Inventory(*m_inventory);
		d->m_text = m_text;
		d->m_owner = m_owner;

		d->m_infotext = m_infotext;
		d->m_inventorydrawspec = m_inventorydrawspec;
		d->m_allow_text_input = m_allow_text_input;
		d->m_removal_disabled = m_removal_disabled;
		d->m_enforce_owner = m_enforce_owner;
		d->m_inventory_modified = m_inventory_modified;
		d->m_text_modified = m_text_modified;
		return d;
	}
	static NodeMetadata* create(IGameDef *gamedef)
	{
		GenericNodeMetadata *d = new GenericNodeMetadata(gamedef);
		d->m_inventory = new Inventory(gamedef->idef());
		return d;
	}
	static NodeMetadata* create(std::istream &is, IGameDef *gamedef)
	{
		GenericNodeMetadata *d = new GenericNodeMetadata(gamedef);
		
		d->m_inventory = new Inventory(gamedef->idef());
		d->m_inventory->deSerialize(is);
		d->m_text = deSerializeLongString(is);
		d->m_owner = deSerializeString(is);
		
		d->m_infotext = deSerializeString(is);
		d->m_inventorydrawspec = deSerializeString(is);
		d->m_allow_text_input = readU8(is);
		d->m_removal_disabled = readU8(is);
		d->m_enforce_owner = readU8(is);

		int num_vars = readU32(is);
		for(int i=0; i<num_vars; i++){
			std::string name = deSerializeString(is);
			std::string var = deSerializeLongString(is);
			d->m_stringvars[name] = var;
		}

		return d;
	}
	void serializeBody(std::ostream &os)
	{
		m_inventory->serialize(os);
		os<<serializeLongString(m_text);
		os<<serializeString(m_owner);

		os<<serializeString(m_infotext);
		os<<serializeString(m_inventorydrawspec);
		writeU8(os, m_allow_text_input);
		writeU8(os, m_removal_disabled);
		writeU8(os, m_enforce_owner);

		int num_vars = m_stringvars.size();
		writeU32(os, num_vars);
		for(std::map<std::string, std::string>::iterator
				i = m_stringvars.begin(); i != m_stringvars.end(); i++){
			os<<serializeString(i->first);
			os<<serializeLongString(i->second);
		}
	}

	std::string infoText()
	{
		return m_infotext;
	}
	Inventory* getInventory()
	{
		return m_inventory;
	}
	void inventoryModified()
	{
		m_inventory_modified = true;
	}
	bool step(float dtime)
	{
		return false;
	}
	bool nodeRemovalDisabled()
	{
		return m_removal_disabled;
	}
	std::string getInventoryDrawSpecString()
	{
		return m_inventorydrawspec;
	}
	bool allowsTextInput()
	{
		return m_allow_text_input;
	}
	std::string getText()
	{
		return m_text;
	}
	void setText(const std::string &t)
	{
		m_text = t;
		m_text_modified = true;
	}
	std::string getOwner()
	{
		if(m_enforce_owner)
			return m_owner;
		else
			return "";
	}
	void setOwner(std::string t)
	{
		m_owner = t;
	}
	
	/* Interface for GenericNodeMetadata */

	void setInfoText(const std::string &text)
	{
		infostream<<"GenericNodeMetadata::setInfoText(\""
				<<text<<"\")"<<std::endl;
		m_infotext = text;
	}
	void setInventoryDrawSpec(const std::string &text)
	{
		m_inventorydrawspec = text;
	}
	void setAllowTextInput(bool b)
	{
		m_allow_text_input = b;
	}
	void setRemovalDisabled(bool b)
	{
		m_removal_disabled = b;
	}
	void setEnforceOwner(bool b)
	{
		m_enforce_owner = b;
	}
	bool isInventoryModified()
	{
		return m_inventory_modified;
	}
	void resetInventoryModified()
	{
		m_inventory_modified = false;
	}
	bool isTextModified()
	{
		return m_text_modified;
	}
	void resetTextModified()
	{
		m_text_modified = false;
	}
	void setString(const std::string &name, const std::string &var)
	{
		m_stringvars[name] = var;
	}
	std::string getString(const std::string &name)
	{
		std::map<std::string, std::string>::iterator i;
		i = m_stringvars.find(name);
		if(i == m_stringvars.end())
			return "";
		return i->second;
	}
};

// Prototype
GenericNodeMetadata proto_GenericNodeMetadata(NULL);

