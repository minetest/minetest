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
#include "inventory.h"
#include "log.h"
#include "utility.h"

#define NODEMETA_SIGN 14
#define NODEMETA_CHEST 15
#define NODEMETA_LOCKABLE_CHEST 17
#define NODEMETA_FURNACE 16

/*
	SignNodeMetadata
*/

// Prototype
SignNodeMetadata proto_SignNodeMetadata(NULL, "");

SignNodeMetadata::SignNodeMetadata(IGameDef *gamedef, std::string text):
	NodeMetadata(gamedef),
	m_text(text)
{
	NodeMetadata::registerType(typeId(), create);
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
	NodeMetadata::registerType(typeId(), create);
	
	m_inventory = new Inventory();
	m_inventory->addList("0", 8*4);
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
	d->m_inventory->deSerialize(is, gamedef);
	return d;
}
NodeMetadata* ChestNodeMetadata::clone(IGameDef *gamedef)
{
	ChestNodeMetadata *d = new ChestNodeMetadata(gamedef);
	*d->m_inventory = *m_inventory;
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
	NodeMetadata::registerType(typeId(), create);

	m_inventory = new Inventory();
	m_inventory->addList("0", 8*4);
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
	d->m_inventory->deSerialize(is, gamedef);
	return d;
}
NodeMetadata* LockingChestNodeMetadata::clone(IGameDef *gamedef)
{
	LockingChestNodeMetadata *d = new LockingChestNodeMetadata(gamedef);
	*d->m_inventory = *m_inventory;
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
	NodeMetadata::registerType(typeId(), create);
	
	m_inventory = new Inventory();
	m_inventory->addList("fuel", 1);
	m_inventory->addList("src", 1);
	m_inventory->addList("dst", 4);

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
	*d->m_inventory = *m_inventory;
	return d;
}
NodeMetadata* FurnaceNodeMetadata::create(std::istream &is, IGameDef *gamedef)
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata(gamedef);

	d->m_inventory->deSerialize(is, gamedef);

	int temp;
	is>>temp;
	d->m_fuel_totaltime = (float)temp/10;
	is>>temp;
	d->m_fuel_time = (float)temp/10;

	return d;
}
void FurnaceNodeMetadata::serializeBody(std::ostream &os)
{
	m_inventory->serialize(os);
	os<<itos(m_fuel_totaltime*10)<<" ";
	os<<itos(m_fuel_time*10)<<" ";
}
std::string FurnaceNodeMetadata::infoText()
{
	//return "Furnace";
	if(m_fuel_time >= m_fuel_totaltime)
	{
		const InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		const InventoryItem *src_item = src_list->getItem(0);

		if(src_item && src_item->isCookable()) {
			InventoryList *dst_list = m_inventory->getList("dst");
			if(!dst_list->roomForCookedItem(src_item))
				return "Furnace is overloaded";
			return "Furnace is out of fuel";
		}
		else
			return "Furnace is inactive";
	}
	else
	{
		std::string s = "Furnace is active";
		// Do this so it doesn't always show (0%) for weak fuel
		if(m_fuel_totaltime > 3) {
			s += " (";
			s += itos(m_fuel_time/m_fuel_totaltime*100);
			s += "%)";
		}
		return s;
	}
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
	// Update at a fixed frequency
	const float interval = 2.0;
	m_step_accumulator += dtime;
	bool changed = false;
	while(m_step_accumulator > interval)
	{
		m_step_accumulator -= interval;
		dtime = interval;

		//infostream<<"Furnace step dtime="<<dtime<<std::endl;
		
		InventoryList *dst_list = m_inventory->getList("dst");
		assert(dst_list);

		InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		InventoryItem *src_item = src_list->getItem(0);
		
		bool room_available = false;
		
		if(src_item && src_item->isCookable())
			room_available = dst_list->roomForCookedItem(src_item);
		
		// Start only if there are free slots in dst, so that it can
		// accomodate any result item
		if(room_available)
		{
			m_src_totaltime = src_item->getCookTime();
		}
		else
		{
			m_src_time = 0;
			m_src_totaltime = 0;
		}
		
		/*
			If fuel is burning, increment the burn counters.
			If item finishes cooking, move it to result.
		*/
		if(m_fuel_time < m_fuel_totaltime)
		{
			//infostream<<"Furnace is active"<<std::endl;
			m_fuel_time += dtime;
			m_src_time += dtime;
			if(m_src_time >= m_src_totaltime && m_src_totaltime > 0.001
					&& src_item)
			{
				InventoryItem *cookresult = src_item->createCookResult();
				dst_list->addItem(cookresult);
				src_list->decrementMaterials(1);
				m_src_time = 0;
				m_src_totaltime = 0;
			}
			changed = true;
			
			// If the fuel was not used up this step, just keep burning it
			if(m_fuel_time < m_fuel_totaltime)
				continue;
		}
		
		/*
			Get the source again in case it has all burned
		*/
		src_item = src_list->getItem(0);
		
		/*
			If there is no source item, or the source item is not cookable,
			or the furnace is still cooking, or the furnace became overloaded, stop loop.
		*/
		if(src_item == NULL || !room_available || m_fuel_time < m_fuel_totaltime ||
			dst_list->roomForCookedItem(src_item) == false)
		{
			m_step_accumulator = 0;
			break;
		}
		
		//infostream<<"Furnace is out of fuel"<<std::endl;

		InventoryList *fuel_list = m_inventory->getList("fuel");
		assert(fuel_list);
		const InventoryItem *fuel_item = fuel_list->getItem(0);

		if(fuel_item && fuel_item->getBurnTime() >= 0){
			m_fuel_totaltime = fuel_item->getBurnTime();
			m_fuel_time = 0;
			fuel_list->decrementMaterials(1);
			changed = true;
		} else {
			//infostream<<"No fuel found"<<std::endl;
			// No fuel, stop loop.
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


