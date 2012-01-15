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

#include "content_sao_item.h"

/*
	ItemSAO
*/

ItemSAO::ItemSAO(ServerEnvironment *env, v3f pos,
		const std::string inventorystring):
	ServerActiveObject(env, pos),
	m_inventorystring(inventorystring),
	m_speed_f(0,0,0),
	m_last_sent_position(0,0,0)
{
	ServerActiveObject::registerType(getType(), create);
}

ServerActiveObject* ItemSAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	char buf[1];
	// read version
	is.read(buf, 1);
	u8 version = buf[0];
	// check if version is supported
	if(version != 0)
		return NULL;
	std::string inventorystring = deSerializeString(is);
	infostream<<"ItemSAO::create(): Creating item \""
			<<inventorystring<<"\""<<std::endl;
	return new ItemSAO(env, pos, inventorystring);
}

void ItemSAO::step(float dtime, bool send_recommended)
{
	ScopeProfiler sp2(g_profiler, "ItemSAO::step avg", SPT_AVG);

	assert(m_env);

	const float interval = 0.2;
	if(m_move_interval.step(dtime, interval)==false)
		return;
	dtime = interval;

	core::aabbox3d<f32> box(-BS/3.,0.0,-BS/3., BS/3.,BS*2./3.,BS/3.);
	collisionMoveResult moveresult;
	// Apply gravity
	m_speed_f += v3f(0, -dtime*9.81*BS, 0);
	// Maximum movement without glitches
	f32 pos_max_d = BS*0.25;
	// Limit speed
	if(m_speed_f.getLength()*dtime > pos_max_d)
		m_speed_f *= pos_max_d / (m_speed_f.getLength()*dtime);
	v3f pos_f = getBasePosition();
	v3f pos_f_old = pos_f;
	IGameDef *gamedef = m_env->getGameDef();
	moveresult = collisionMoveSimple(&m_env->getMap(), gamedef,
			pos_max_d, box, dtime, pos_f, m_speed_f);

	if(send_recommended == false)
		return;

	if(pos_f.getDistanceFrom(m_last_sent_position) > 0.05*BS)
	{
		setBasePosition(pos_f);
		m_last_sent_position = pos_f;

		std::ostringstream os(std::ios::binary);
		char buf[6];
		// command (0 = update position)
		buf[0] = AO_Message_type::SetPosition;
		os.write(buf, 1);
		// pos
		writeS32((u8*)buf, m_base_position.X*1000);
		os.write(buf, 4);
		writeS32((u8*)buf, m_base_position.Y*1000);
		os.write(buf, 4);
		writeS32((u8*)buf, m_base_position.Z*1000);
		os.write(buf, 4);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

std::string ItemSAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	char buf[6];
	// version
	buf[0] = 0;
	os.write(buf, 1);
	// pos
	writeS32((u8*)buf, m_base_position.X*1000);
	os.write(buf, 4);
	writeS32((u8*)buf, m_base_position.Y*1000);
	os.write(buf, 4);
	writeS32((u8*)buf, m_base_position.Z*1000);
	os.write(buf, 4);
	// inventorystring
	os<<serializeString(m_inventorystring);
	return os.str();
}

std::string ItemSAO::getStaticData()
{
	infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	char buf[1];
	// version
	buf[0] = 0;
	os.write(buf, 1);
	// inventorystring
	os<<serializeString(m_inventorystring);
	return os.str();
}

InventoryItem * ItemSAO::createInventoryItem()
{
	try{
		std::istringstream is(m_inventorystring, std::ios_base::binary);
		IGameDef *gamedef = m_env->getGameDef();
		InventoryItem *item = InventoryItem::deSerialize(is, gamedef);
		infostream<<__FUNCTION_NAME<<": m_inventorystring=\""
				<<m_inventorystring<<"\" -> item="<<item
				<<std::endl;
		return item;
	}
	catch(SerializationError &e)
	{
		infostream<<__FUNCTION_NAME<<": serialization error: "
				<<"m_inventorystring=\""<<m_inventorystring<<"\""<<std::endl;
		return NULL;
	}
}

void ItemSAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	InventoryItem *item = createInventoryItem();
	bool fits = puncher->addToInventory(item);
	if(fits)
		m_removed = true;
	else
		delete item;
}

// Prototype
ItemSAO proto_ItemSAO(NULL, v3f(0,0,0), "");
