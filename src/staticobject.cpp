/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "staticobject.h"
#include "util/serialize.h"

void StaticObject::serialize(std::ostream &os)
{
	// type
	writeU8(os, type);
	// pos
	writeV3F1000(os, pos);
	// data
	os<<serializeString(data);
}
void StaticObject::deSerialize(std::istream &is, u8 version)
{
	// type
	type = readU8(is);
	// pos
	pos = readV3F1000(is);
	// data
	data = deSerializeString(is);
}

void StaticObjectList::serialize(std::ostream &os)
{
	// version
	u8 version = 0;
	writeU8(os, version);
	// count
	u16 count = m_stored.size() + m_active.size();
	writeU16(os, count);
	for(std::list<StaticObject>::iterator
			i = m_stored.begin();
			i != m_stored.end(); ++i)
	{
		StaticObject &s_obj = *i;
		s_obj.serialize(os);
	}
	for(std::map<u16, StaticObject>::iterator
			i = m_active.begin();
			i != m_active.end(); ++i)
	{
		StaticObject s_obj = i->second;
		s_obj.serialize(os);
	}
}
void StaticObjectList::deSerialize(std::istream &is)
{
	// version
	u8 version = readU8(is);
	// count
	u16 count = readU16(is);
	for(u16 i=0; i<count; i++)
	{
		StaticObject s_obj;
		s_obj.deSerialize(is, version);
		m_stored.push_back(s_obj);
	}
}

