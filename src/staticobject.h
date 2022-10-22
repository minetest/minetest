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

#pragma once

#include "irrlichttypes_bloated.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "debug.h"

class ServerActiveObject;

struct StaticObject
{
	u8 type = 0;
	v3f pos;
	std::string data;

	StaticObject() = default;
	StaticObject(const ServerActiveObject *s_obj, const v3f &pos_);

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is, u8 version);
};

class StaticObjectList
{
public:
	/*
		Inserts an object to the container.
		Id must be unique (active) or 0 (stored).
	*/
	void insert(u16 id, const StaticObject &obj)
	{
		if (id == 0) {
			m_stored.push_back(obj);
		} else {
			if (m_active.find(id) != m_active.end()) {
				dstream << "ERROR: StaticObjectList::insert(): "
						<< "id already exists" << std::endl;
				FATAL_ERROR("StaticObjectList::insert()");
			}
			setActive(id, obj);
		}
	}

	void remove(u16 id)
	{
		assert(id != 0); // Pre-condition
		if (m_active.find(id) == m_active.end()) {
			warningstream << "StaticObjectList::remove(): id=" << id << " not found"
						  << std::endl;
			return;
		}
		m_active.erase(id);
	}

	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);

	// Never permit to modify outside of here. Only this object is responsible of m_stored and m_active modifications
	const std::vector<StaticObject>& getAllStored() const { return m_stored; }
	const std::map<u16, StaticObject> &getAllActives() const { return m_active; }

	inline void setActive(u16 id, const StaticObject &obj) { m_active[id] = obj; }
	inline size_t getActiveSize() const { return m_active.size(); }
	inline size_t getStoredSize() const { return m_stored.size(); }
	inline void clearStored() { m_stored.clear(); }
	void pushStored(const StaticObject &obj) { m_stored.push_back(obj); }

	bool storeActiveObject(u16 id);

	inline void clear()
	{
		m_active.clear();
		m_stored.clear();
	}

	inline size_t size()
	{
		return m_active.size() + m_stored.size();
	}

private:
	/*
		NOTE: When an object is transformed to active, it is removed
		from m_stored and inserted to m_active.
	*/
	std::vector<StaticObject> m_stored;
	std::map<u16, StaticObject> m_active;
};
