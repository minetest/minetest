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

#ifndef STATICOBJECT_HEADER
#define STATICOBJECT_HEADER

#include "irrlichttypes_bloated.h"
#include <string>
#include <sstream>
#include "util/serialize.h"

struct StaticObject
{
	u8 type;
	v3f pos;
	std::string data;

	StaticObject():
		type(0),
		pos(0,0,0)
	{
	}
	StaticObject(u8 type_, v3f pos_, const std::string &data_):
		type(type_),
		pos(pos_),
		data(data_)
	{
	}

	void serialize(std::ostream &os)
	{
		char buf[12];
		// type
		buf[0] = type;
		os.write(buf, 1);
		// pos
		writeV3S32((u8*)buf, v3s32(pos.X*1000,pos.Y*1000,pos.Z*1000));
		os.write(buf, 12);
		// data
		os<<serializeString(data);
	}
	void deSerialize(std::istream &is, u8 version)
	{
		char buf[12];
		// type
		is.read(buf, 1);
		type = buf[0];
		// pos
		is.read(buf, 12);
		v3s32 intp = readV3S32((u8*)buf);
		pos.X = (f32)intp.X/1000;
		pos.Y = (f32)intp.Y/1000;
		pos.Z = (f32)intp.Z/1000;
		// data
		data = deSerializeString(is);
	}
};

class StaticObjectList
{
public:
	/*
		Inserts an object to the container.
		Id must be unique or 0.
	*/
	void insert(u16 id, StaticObject obj)
	{
		if(id == 0)
		{
			m_stored.push_back(obj);
		}
		else
		{
			if(m_active.find(id) != NULL)
			{
				dstream<<"ERROR: StaticObjectList::insert(): "
						<<"id already exists"<<std::endl;
				assert(0);
				return;
			}
			m_active.insert(id, obj);
		}
	}

	void remove(u16 id)
	{
		assert(id != 0);
		if(m_active.find(id) == NULL)
		{
			dstream<<"WARNING: StaticObjectList::remove(): id="<<id
					<<" not found"<<std::endl;
			return;
		}
		m_active.remove(id);
	}

	void serialize(std::ostream &os)
	{
		char buf[12];
		// version
		buf[0] = 0;
		os.write(buf, 1);
		// count
		u16 count = m_stored.size() + m_active.size();
		writeU16((u8*)buf, count);
		os.write(buf, 2);
		for(core::list<StaticObject>::Iterator
				i = m_stored.begin();
				i != m_stored.end(); i++)
		{
			StaticObject &s_obj = *i;
			s_obj.serialize(os);
		}
		for(core::map<u16, StaticObject>::Iterator
				i = m_active.getIterator();
				i.atEnd()==false; i++)
		{
			StaticObject s_obj = i.getNode()->getValue();
			s_obj.serialize(os);
		}
	}
	void deSerialize(std::istream &is)
	{
		char buf[12];
		// version
		is.read(buf, 1);
		u8 version = buf[0];
		// count
		is.read(buf, 2);
		u16 count = readU16((u8*)buf);
		for(u16 i=0; i<count; i++)
		{
			StaticObject s_obj;
			s_obj.deSerialize(is, version);
			m_stored.push_back(s_obj);
		}
	}
	
	/*
		NOTE: When an object is transformed to active, it is removed
		from m_stored and inserted to m_active.
		The caller directly manipulates these containers.
	*/
	core::list<StaticObject> m_stored;
	core::map<u16, StaticObject> m_active;

private:
};

#endif

