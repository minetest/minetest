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
#include "inventory.h"

core::map<u16, ServerActiveObject::Factory> ServerActiveObject::m_types;

ServerActiveObject::ServerActiveObject(ServerEnvironment *env, u16 id, v3f pos):
	ActiveObject(id),
	m_known_by_count(0),
	m_removed(false),
	m_static_exists(false),
	m_static_block(1337,1337,1337),
	m_env(env),
	m_base_position(pos)
{
}

ServerActiveObject::~ServerActiveObject()
{
}

ServerActiveObject* ServerActiveObject::create(u8 type,
		ServerEnvironment *env, u16 id, v3f pos,
		const std::string &data)
{
	// Find factory function
	core::map<u16, Factory>::Node *n;
	n = m_types.find(type);
	if(n == NULL)
	{
		// If factory is not found, just return.
		dstream<<"WARNING: ServerActiveObject: No factory for type="
				<<type<<std::endl;
		return NULL;
	}

	Factory f = n->getValue();
	ServerActiveObject *object = (*f)(env, id, pos, data);
	return object;
}

void ServerActiveObject::registerType(u16 type, Factory f)
{
	core::map<u16, Factory>::Node *n;
	n = m_types.find(type);
	if(n)
		return;
	m_types.insert(type, f);
}


/*
	TestSAO
*/

// Prototype
TestSAO proto_TestSAO(NULL, 0, v3f(0,0,0));

TestSAO::TestSAO(ServerEnvironment *env, u16 id, v3f pos):
	ServerActiveObject(env, id, pos),
	m_timer1(0),
	m_age(0)
{
	ServerActiveObject::registerType(getType(), create);
}

ServerActiveObject* TestSAO::create(ServerEnvironment *env, u16 id, v3f pos,
		const std::string &data)
{
	return new TestSAO(env, id, pos);
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


/*
	ItemSAO
*/

// Prototype
ItemSAO proto_ItemSAO(NULL, 0, v3f(0,0,0), "");

ItemSAO::ItemSAO(ServerEnvironment *env, u16 id, v3f pos,
		const std::string inventorystring):
	ServerActiveObject(env, id, pos),
	m_inventorystring(inventorystring)
{
	dstream<<"Server: ItemSAO created with inventorystring=\""
			<<m_inventorystring<<"\""<<std::endl;
	ServerActiveObject::registerType(getType(), create);
}

ServerActiveObject* ItemSAO::create(ServerEnvironment *env, u16 id, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	char buf[1];
	is.read(buf, 1); // read version
	std::string inventorystring = deSerializeString(is);
	dstream<<"ItemSAO::create(): Creating item \""
			<<inventorystring<<"\""<<std::endl;
	return new ItemSAO(env, id, pos, inventorystring);
}

void ItemSAO::step(float dtime, Queue<ActiveObjectMessage> &messages)
{
}

std::string ItemSAO::getClientInitializationData()
{
	dstream<<__FUNCTION_NAME<<std::endl;
	std::string data;
	data += itos(m_base_position.X);
	data += ",";
	data += itos(m_base_position.Y);
	data += ",";
	data += itos(m_base_position.Z);
	data += ":";
	data += m_inventorystring;
	return data;
}

std::string ItemSAO::getStaticData()
{
	dstream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	char buf[1];
	buf[0] = 0; //version
	os.write(buf, 1);
	os<<serializeString(m_inventorystring);
	return os.str();
}

InventoryItem * ItemSAO::createInventoryItem()
{
	try{
		std::istringstream is(m_inventorystring, std::ios_base::binary);
		InventoryItem *item = InventoryItem::deSerialize(is);
		dstream<<__FUNCTION_NAME<<": m_inventorystring=\""
				<<m_inventorystring<<"\" -> item="<<item
				<<std::endl;
		return item;
	}
	catch(SerializationError &e)
	{
		dstream<<__FUNCTION_NAME<<": serialization error: "
				<<"m_inventorystring=\""<<m_inventorystring<<"\""<<std::endl;
		return NULL;
	}
}



