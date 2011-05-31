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

#include "nodemetadata.h"
#include "utility.h"
#include "mapnode.h"
#include "exceptions.h"
#include "inventory.h"
#include <sstream>

/*
	NodeMetadata
*/

core::map<u16, NodeMetadata::Factory> NodeMetadata::m_types;

NodeMetadata::NodeMetadata()
{
}

NodeMetadata::~NodeMetadata()
{
}

NodeMetadata* NodeMetadata::deSerialize(std::istream &is)
{
	// Read id
	u8 buf[2];
	is.read((char*)buf, 2);
	s16 id = readS16(buf);
	
	// Read data
	std::string data = deSerializeString(is);
	
	// Find factory function
	core::map<u16, Factory>::Node *n;
	n = m_types.find(id);
	if(n == NULL)
	{
		// If factory is not found, just return.
		dstream<<"WARNING: NodeMetadata: No factory for typeId="
				<<id<<std::endl;
		return NULL;
	}
	
	// Try to load the metadata. If it fails, just return.
	try
	{
		std::istringstream iss(data, std::ios_base::binary);
		
		Factory f = n->getValue();
		NodeMetadata *meta = (*f)(iss);
		return meta;
	}
	catch(SerializationError &e)
	{
		dstream<<"WARNING: NodeMetadata: ignoring SerializationError"<<std::endl;
		return NULL;
	}
}

void NodeMetadata::serialize(std::ostream &os)
{
	u8 buf[2];
	writeU16(buf, typeId());
	os.write((char*)buf, 2);
	
	std::ostringstream oss(std::ios_base::binary);
	serializeBody(oss);
	os<<serializeString(oss.str());
}

void NodeMetadata::registerType(u16 id, Factory f)
{
	core::map<u16, Factory>::Node *n;
	n = m_types.find(id);
	if(n)
		return;
	m_types.insert(id, f);
}

/*
	SignNodeMetadata
*/

// Prototype
SignNodeMetadata proto_SignNodeMetadata("");

SignNodeMetadata::SignNodeMetadata(std::string text):
	m_text(text)
{
	NodeMetadata::registerType(typeId(), create);
}
u16 SignNodeMetadata::typeId() const
{
	return CONTENT_SIGN_WALL;
}
NodeMetadata* SignNodeMetadata::create(std::istream &is)
{
	std::string text = deSerializeString(is);
	return new SignNodeMetadata(text);
}
NodeMetadata* SignNodeMetadata::clone()
{
	return new SignNodeMetadata(m_text);
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
ChestNodeMetadata proto_ChestNodeMetadata;

ChestNodeMetadata::ChestNodeMetadata()
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
	return CONTENT_CHEST;
}
NodeMetadata* ChestNodeMetadata::create(std::istream &is)
{
	ChestNodeMetadata *d = new ChestNodeMetadata();
	d->m_inventory->deSerialize(is);
	return d;
}
NodeMetadata* ChestNodeMetadata::clone()
{
	ChestNodeMetadata *d = new ChestNodeMetadata();
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

/*
	FurnaceNodeMetadata
*/

// Prototype
FurnaceNodeMetadata proto_FurnaceNodeMetadata;

FurnaceNodeMetadata::FurnaceNodeMetadata()
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
	return CONTENT_FURNACE;
}
NodeMetadata* FurnaceNodeMetadata::clone()
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata();
	*d->m_inventory = *m_inventory;
	return d;
}
NodeMetadata* FurnaceNodeMetadata::create(std::istream &is)
{
	FurnaceNodeMetadata *d = new FurnaceNodeMetadata();

	d->m_inventory->deSerialize(is);

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
		InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		InventoryItem *src_item = src_list->getItem(0);

		if(src_item)
			return "Furnace is out of fuel";
		else
			return "Furnace is inactive";
	}
	else
	{
		std::string s = "Furnace is active (";
		s += itos(m_fuel_time/m_fuel_totaltime*100);
		s += "%)";
		return s;
	}
}
void FurnaceNodeMetadata::inventoryModified()
{
	dstream<<"Furnace inventory modification callback"<<std::endl;
}
bool FurnaceNodeMetadata::step(float dtime)
{
	if(dtime > 60.0)
		dstream<<"Furnace stepping a long time ("<<dtime<<")"<<std::endl;
	// Update at a fixed frequency
	const float interval = 2.0;
	m_step_accumulator += dtime;
	bool changed = false;
	while(m_step_accumulator > interval)
	{
		m_step_accumulator -= interval;
		dtime = interval;

		//dstream<<"Furnace step dtime="<<dtime<<std::endl;
		
		InventoryList *dst_list = m_inventory->getList("dst");
		assert(dst_list);

		InventoryList *src_list = m_inventory->getList("src");
		assert(src_list);
		InventoryItem *src_item = src_list->getItem(0);
		
		// Start only if there are free slots in dst, so that it can
		// accomodate any result item
		if(dst_list->getFreeSlots() > 0 && src_item && src_item->isCookable())
		{
			m_src_totaltime = 3;
		}
		else
		{
			m_src_time = 0;
			m_src_totaltime = 0;
		}

		if(m_fuel_time < m_fuel_totaltime)
		{
			//dstream<<"Furnace is active"<<std::endl;
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
			continue;
		}
		
		if(src_item == NULL || m_src_totaltime < 0.001)
		{
			continue;
		}
		
		//dstream<<"Furnace is out of fuel"<<std::endl;

		InventoryList *fuel_list = m_inventory->getList("fuel");
		assert(fuel_list);
		InventoryItem *fuel_item = fuel_list->getItem(0);

		if(ItemSpec(ITEM_MATERIAL, CONTENT_TREE).checkItem(fuel_item))
		{
			m_fuel_totaltime = 30;
			m_fuel_time = 0;
			fuel_list->decrementMaterials(1);
			changed = true;
		}
		else if(ItemSpec(ITEM_MATERIAL, CONTENT_WOOD).checkItem(fuel_item))
		{
			m_fuel_totaltime = 30/4;
			m_fuel_time = 0;
			fuel_list->decrementMaterials(1);
			changed = true;
		}
		else if(ItemSpec(ITEM_CRAFT, "Stick").checkItem(fuel_item))
		{
			m_fuel_totaltime = 30/4/4;
			m_fuel_time = 0;
			fuel_list->decrementMaterials(1);
			changed = true;
		}
		else if(ItemSpec(ITEM_CRAFT, "lump_of_coal").checkItem(fuel_item))
		{
			m_fuel_totaltime = 40;
			m_fuel_time = 0;
			fuel_list->decrementMaterials(1);
			changed = true;
		}
		else
		{
			//dstream<<"No fuel found"<<std::endl;
		}
	}
	return changed;
}

/*
	NodeMetadatalist
*/

void NodeMetadataList::serialize(std::ostream &os)
{
	u8 buf[6];
	
	u16 version = 1;
	writeU16(buf, version);
	os.write((char*)buf, 2);

	u16 count = m_data.size();
	writeU16(buf, count);
	os.write((char*)buf, 2);

	for(core::map<v3s16, NodeMetadata*>::Iterator
			i = m_data.getIterator();
			i.atEnd()==false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		NodeMetadata *data = i.getNode()->getValue();
		
		u16 p16 = p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X;
		writeU16(buf, p16);
		os.write((char*)buf, 2);

		data->serialize(os);
	}
	
}
void NodeMetadataList::deSerialize(std::istream &is)
{
	m_data.clear();

	u8 buf[6];
	
	is.read((char*)buf, 2);
	u16 version = readU16(buf);

	if(version > 1)
	{
		dstream<<__FUNCTION_NAME<<": version "<<version<<" not supported"
				<<std::endl;
		throw SerializationError("NodeMetadataList::deSerialize");
	}
	
	is.read((char*)buf, 2);
	u16 count = readU16(buf);
	
	for(u16 i=0; i<count; i++)
	{
		is.read((char*)buf, 2);
		u16 p16 = readU16(buf);

		v3s16 p(0,0,0);
		p.Z += p16 / MAP_BLOCKSIZE / MAP_BLOCKSIZE;
		p16 -= p.Z * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
		p.Y += p16 / MAP_BLOCKSIZE;
		p16 -= p.Y * MAP_BLOCKSIZE;
		p.X += p16;
		
		NodeMetadata *data = NodeMetadata::deSerialize(is);

		if(data == NULL)
			continue;
		
		if(m_data.find(p))
		{
			dstream<<"WARNING: NodeMetadataList::deSerialize(): "
					<<"already set data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			delete data;
			continue;
		}

		m_data.insert(p, data);
	}
}
	
NodeMetadataList::~NodeMetadataList()
{
	for(core::map<v3s16, NodeMetadata*>::Iterator
			i = m_data.getIterator();
			i.atEnd()==false; i++)
	{
		delete i.getNode()->getValue();
	}
}

NodeMetadata* NodeMetadataList::get(v3s16 p)
{
	core::map<v3s16, NodeMetadata*>::Node *n;
	n = m_data.find(p);
	if(n == NULL)
		return NULL;
	return n->getValue();
}

void NodeMetadataList::remove(v3s16 p)
{
	NodeMetadata *olddata = get(p);
	if(olddata)
	{
		delete olddata;
		m_data.remove(p);
	}
}

void NodeMetadataList::set(v3s16 p, NodeMetadata *d)
{
	remove(p);
	m_data.insert(p, d);
}

bool NodeMetadataList::step(float dtime)
{
	bool something_changed = false;
	for(core::map<v3s16, NodeMetadata*>::Iterator
			i = m_data.getIterator();
			i.atEnd()==false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		NodeMetadata *meta = i.getNode()->getValue();
		bool changed = meta->step(dtime);
		if(changed)
			something_changed = true;
		/*if(res.inventory_changed)
		{
			std::string inv_id;
			inv_id += "nodemeta:";
			inv_id += itos(p.X);
			inv_id += ",";
			inv_id += itos(p.Y);
			inv_id += ",";
			inv_id += itos(p.Z);
			InventoryContext c;
			c.current_player = NULL;
			inv_mgr->inventoryModified(&c, inv_id);
		}*/
	}
	return something_changed;
}

