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

#include "content_sao_rat.h"

/*
	RatSAO
*/
RatSAO::RatSAO(ServerEnvironment *env, v3f pos):
	ServerActiveObject(env, pos),
	m_is_active(false),
	m_speed_f(0,0,0)
{
	ServerActiveObject::registerType(getType(), create);

	m_oldpos = v3f(0,0,0);
	m_last_sent_position = v3f(0,0,0);
	m_yaw = myrand_range(0,PI*2);
	m_counter1 = 0;
	m_counter2 = 0;
	m_age = 0;
	m_touching_ground = false;
}

ServerActiveObject* RatSAO::create(ServerEnvironment *env, v3f pos,
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
	return new RatSAO(env, pos);
}

void RatSAO::step(float dtime, bool send_recommended)
{
	ScopeProfiler sp2(g_profiler, "RatSAO::step avg", SPT_AVG);

	assert(m_env);

	if(m_is_active == false)
	{
		if(m_inactive_interval.step(dtime, 0.5)==false)
			return;
	}

	/*
		The AI
	*/

	/*m_age += dtime;
	if(m_age > 60)
	{
		// Die
		m_removed = true;
		return;
	}*/

	// Apply gravity
	m_speed_f.Y -= dtime*9.81*BS;

	/*
		Move around if some player is close
	*/
	bool player_is_close = false;
	// Check connected players
	core::list<Player*> players = m_env->getPlayers(true);
	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		Player *player = *i;
		v3f playerpos = player->getPosition();
		if(m_base_position.getDistanceFrom(playerpos) < BS*10.0)
		{
			player_is_close = true;
			break;
		}
	}

	m_is_active = player_is_close;

	if(player_is_close == false)
	{
		m_speed_f.X = 0;
		m_speed_f.Z = 0;
	}
	else
	{
		// Move around
		v3f dir(cos(m_yaw/180*PI),0,sin(m_yaw/180*PI));
		f32 speed = 2*BS;
		m_speed_f.X = speed * dir.X;
		m_speed_f.Z = speed * dir.Z;

		if(m_touching_ground && (m_oldpos - m_base_position).getLength()
				< dtime*speed/2)
		{
			m_counter1 -= dtime;
			if(m_counter1 < 0.0)
			{
				m_counter1 += 1.0;
				m_speed_f.Y = 5.0*BS;
			}
		}

		{
			m_counter2 -= dtime;
			if(m_counter2 < 0.0)
			{
				m_counter2 += (float)(myrand()%100)/100*3.0;
				m_yaw += ((float)(myrand()%200)-100)/100*180;
				m_yaw = wrapDegrees(m_yaw);
			}
		}
	}

	m_oldpos = m_base_position;

	/*
		Move it, with collision detection
	*/

	core::aabbox3d<f32> box(-BS/3.,0.0,-BS/3., BS/3.,BS*2./3.,BS/3.);
	collisionMoveResult moveresult;
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
	m_touching_ground = moveresult.touching_ground;

	setBasePosition(pos_f);

	if(send_recommended == false)
		return;

	if(pos_f.getDistanceFrom(m_last_sent_position) > 0.05*BS)
	{
		m_last_sent_position = pos_f;

		std::ostringstream os(std::ios::binary);
		// command (0 = update position)
		writeU8(os, 0);
		// pos
		writeV3F1000(os, m_base_position);
		// yaw
		writeF1000(os, m_yaw);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

std::string RatSAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	return os.str();
}

std::string RatSAO::getStaticData()
{
	//infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	return os.str();
}

void RatSAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	std::istringstream is("CraftItem rat 1", std::ios_base::binary);
	IGameDef *gamedef = m_env->getGameDef();
	InventoryItem *item = InventoryItem::deSerialize(is, gamedef);
	bool fits = puncher->addToInventory(item);
	if(fits)
		m_removed = true;
	else
		delete item;
}

// Prototype
RatSAO proto_RatSAO(NULL, v3f(0,0,0));
