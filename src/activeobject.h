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

#ifndef ACTIVEOBJECT_HEADER
#define ACTIVEOBJECT_HEADER

#include "irrlichttypes_bloated.h"
#include <string>

#define ACTIVEOBJECT_TYPE_INVALID 0
// Other types are defined in content_object.h

struct ActiveObjectMessage
{
	ActiveObjectMessage(u16 id_, bool reliable_=true, std::string data_=""):
		id(id_),
		reliable(reliable_),
		datastring(data_)
	{}

	u16 id;
	bool reliable;
	std::string datastring;
};

/*
	Parent class for ServerActiveObject and ClientActiveObject
*/
class ActiveObject
{
public:
	ActiveObject(u16 id):
		m_id(id)
	{
	}
	
	u16 getId()
	{
		return m_id;
	}

	void setId(u16 id)
	{
		m_id = id;
	}

	virtual u8 getType() const = 0;

protected:
	u16 m_id; // 0 is invalid, "no id"
};

#endif

