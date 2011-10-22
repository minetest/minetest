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
#include "content_mapnode.h"
#include "log.h"

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
		infostream<<"WARNING: NodeMetadata: No factory for typeId="
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
		infostream<<"WARNING: NodeMetadata: ignoring SerializationError"<<std::endl;
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
	NodeMetadataList
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
		infostream<<__FUNCTION_NAME<<": version "<<version<<" not supported"
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
			infostream<<"WARNING: NodeMetadataList::deSerialize(): "
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
	}
	return something_changed;
}

