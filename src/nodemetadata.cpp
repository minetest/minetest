/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "nodemetadata.h"
#include "exceptions.h"
#include "gamedef.h"
#include "inventory.h"
#include "log.h"
#include "util/serialize.h"
#include "constants.h" // MAP_BLOCKSIZE
#include <sstream>

/*
	NodeMetadata
*/

NodeMetadata::NodeMetadata(IGameDef *gamedef):
	m_stringvars(),
	m_inventory(new Inventory(gamedef->idef()))
{
}

NodeMetadata::~NodeMetadata()
{
	delete m_inventory;
}

void NodeMetadata::serialize(std::ostream &os) const
{
	int num_vars = m_stringvars.size();
	writeU32(os, num_vars);
	for(std::map<std::string, std::string>::const_iterator
			i = m_stringvars.begin(); i != m_stringvars.end(); i++){
		os<<serializeString(i->first);
		os<<serializeLongString(i->second);
	}

	m_inventory->serialize(os);
}

void NodeMetadata::deSerialize(std::istream &is)
{
	m_stringvars.clear();
	int num_vars = readU32(is);
	for(int i=0; i<num_vars; i++){
		std::string name = deSerializeString(is);
		std::string var = deSerializeLongString(is);
		m_stringvars[name] = var;
	}

	m_inventory->deSerialize(is);
}

void NodeMetadata::clear()
{
	m_stringvars.clear();
	m_inventory->clear();
}

/*
	NodeMetadataList
*/

void NodeMetadataList::serialize(std::ostream &os) const
{
	/*
		Version 0 is a placeholder for "nothing to see here; go away."
	*/

	if(m_data.size() == 0){
		writeU8(os, 0); // version
		return;
	}

	writeU8(os, 1); // version

	u16 count = m_data.size();
	writeU16(os, count);

	for(std::map<v3s16, NodeMetadata*>::const_iterator
			i = m_data.begin();
			i != m_data.end(); i++)
	{
		v3s16 p = i->first;
		NodeMetadata *data = i->second;

		u16 p16 = p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X;
		writeU16(os, p16);

		data->serialize(os);
	}
}

void NodeMetadataList::deSerialize(std::istream &is, IGameDef *gamedef)
{
	m_data.clear();

	u8 version = readU8(is);
	
	if(version == 0){
		// Nothing
		return;
	}

	if(version != 1){
		infostream<<__FUNCTION_NAME<<": version "<<version<<" not supported"
				<<std::endl;
		throw SerializationError("NodeMetadataList::deSerialize");
	}

	u16 count = readU16(is);

	for(u16 i=0; i<count; i++)
	{
		u16 p16 = readU16(is);

		v3s16 p(0,0,0);
		p.Z += p16 / MAP_BLOCKSIZE / MAP_BLOCKSIZE;
		p16 -= p.Z * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
		p.Y += p16 / MAP_BLOCKSIZE;
		p16 -= p.Y * MAP_BLOCKSIZE;
		p.X += p16;

		if(m_data.find(p) != m_data.end())
		{
			infostream<<"WARNING: NodeMetadataList::deSerialize(): "
					<<"already set data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			continue;
		}

		NodeMetadata *data = new NodeMetadata(gamedef);
		data->deSerialize(is);
		m_data[p] = data;
	}
}

NodeMetadataList::~NodeMetadataList()
{
	clear();
}

NodeMetadata* NodeMetadataList::get(v3s16 p)
{
	std::map<v3s16, NodeMetadata*>::const_iterator n = m_data.find(p);
	if(n == m_data.end())
		return NULL;
	return n->second;
}

void NodeMetadataList::remove(v3s16 p)
{
	NodeMetadata *olddata = get(p);
	if(olddata)
	{
		delete olddata;
		m_data.erase(p);
	}
}

void NodeMetadataList::set(v3s16 p, NodeMetadata *d)
{
	remove(p);
	m_data.insert(std::make_pair(p, d));
}

void NodeMetadataList::clear()
{
	for(std::map<v3s16, NodeMetadata*>::iterator
			i = m_data.begin();
			i != m_data.end(); i++)
	{
		delete i->second;
	}
	m_data.clear();
}
