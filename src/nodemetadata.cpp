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
	u8 buf[2];
	is.read((char*)buf, 2);
	s16 id = readS16(buf);

	core::map<u16, Factory>::Node *n;
	n = m_types.find(id);
	if(n == NULL)
	{
		dstream<<"NodeMetadata(): No factory for typeId="<<id<<std::endl;
		throw SerializationError("Unknown metadata id");
	}
	
	Factory f = n->getValue();
	NodeMetadata *meta = (*f)(is);
	return meta;
}

void NodeMetadata::serialize(std::ostream &os)
{
	u8 buf[2];
	writeU16(buf, typeId());
	os.write((char*)buf, 2);

	serializeBody(os);
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
/*Inventory* ChestNodeMetadata::getInventory()
{
	return m_inventory;
}*/

/*
	NodeMetadatalist
*/

void NodeMetadataList::serialize(std::ostream &os)
{
	u8 buf[6];
	
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
		
		if(m_data.find(p))
		{
			dstream<<"ERROR: NodeMetadataList::deSerialize(): "
					<<"already set data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<std::endl;
			throw SerializationError("NodeMetadataList::deSerialize()");
		}

		NodeMetadata *data = NodeMetadata::deSerialize(is);
		
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

