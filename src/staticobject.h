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

#ifndef STATICOBJECT_HEADER
#define STATICOBJECT_HEADER

#include "irrlichttypes_bloated.h"
#include <string>
#include <sstream>
#include <list>
#include <map>
#include "debug.h"

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

	void serialize(std::ostream &os);
	void deSerialize(std::istream &is, u8 version);
};

class StaticObjectList
{
public:
	/*
		Inserts an object to the container.
		Id must be unique (active) or 0 (stored).
	*/
	void insert(u16 id, StaticObject obj)
	{
		if(id == 0)
		{
			m_stored.push_back(obj);
		}
		else
		{
			if(m_active.find(id) != m_active.end())
			{
				dstream<<"ERROR: StaticObjectList::insert(): "
						<<"id already exists"<<std::endl;
				assert(0);
				return;
			}
			m_active[id] = obj;
		}
	}

	void remove(u16 id)
	{
		assert(id != 0);
		if(m_active.find(id) == m_active.end())
		{
			dstream<<"WARNING: StaticObjectList::remove(): id="<<id
					<<" not found"<<std::endl;
			return;
		}
		m_active.erase(id);
	}

	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
	
	/*
		NOTE: When an object is transformed to active, it is removed
		from m_stored and inserted to m_active.
		The caller directly manipulates these containers.
	*/
	std::list<StaticObject> m_stored;
	std::map<u16, StaticObject> m_active;

private:
};

#endif

