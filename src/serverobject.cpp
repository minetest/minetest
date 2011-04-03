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

#include "serverobject.h"
#include <fstream>
#include "environment.h"

ServerActiveObject::ServerActiveObject(ServerEnvironment *env, u16 id, v3f pos):
	ActiveObject(id),
	m_known_by_count(0),
	m_removed(false),
	m_env(env),
	m_base_position(pos)
{
}

ServerActiveObject::~ServerActiveObject()
{
}

/*
	TestSAO
*/

TestSAO::TestSAO(ServerEnvironment *env, u16 id, v3f pos):
	ServerActiveObject(env, id, pos),
	m_timer1(0),
	m_age(0)
{
}

void TestSAO::step(float dtime, Queue<ActiveObjectMessage> &messages)
{
	m_age += dtime;
	if(m_age > 10)
	{
		m_removed = true;
		return;
	}

	m_base_position.Y += dtime * BS * 2;
	if(m_base_position.Y > 8*BS)
		m_base_position.Y = 2*BS;

	m_timer1 -= dtime;
	if(m_timer1 < 0.0)
	{
		m_timer1 += 0.125;
		//dstream<<"TestSAO: id="<<getId()<<" sending data"<<std::endl;

		std::string data;

		data += itos(0); // 0 = position
		data += " ";
		data += itos(m_base_position.X);
		data += " ";
		data += itos(m_base_position.Y);
		data += " ";
		data += itos(m_base_position.Z);

		ActiveObjectMessage aom(getId(), false, data);
		messages.push_back(aom);
	}
}



