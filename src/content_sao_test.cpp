/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "content_sao.h"


class TestSAO : public ServerActiveObject
{
public:
	TestSAO(ServerEnvironment *env, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_TEST;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
private:
	float m_timer1;
	float m_age;
};


/*
	TestSAO
*/

TestSAO::TestSAO(ServerEnvironment *env, v3f pos):
	ServerActiveObject(env, pos),
	m_timer1(0),
	m_age(0)
{
	ServerActiveObject::registerType(getType(), create);
}

ServerActiveObject* TestSAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	return new TestSAO(env, pos);
}

void TestSAO::step(float dtime, bool send_recommended)
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

	if(send_recommended == false)
		return;

	m_timer1 -= dtime;
	if(m_timer1 < 0.0)
	{
		m_timer1 += 0.125;

		std::string data;

		data += itos(AO_Message_type::SetPosition); // 0 = position
		data += " ";
		data += itos(m_base_position.X);
		data += " ";
		data += itos(m_base_position.Y);
		data += " ";
		data += itos(m_base_position.Z);

		ActiveObjectMessage aom(getId(), false, data);
		m_messages_out.push_back(aom);
	}
}
