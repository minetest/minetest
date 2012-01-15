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

#include "content_sao_oerkki1.h"

/*
	Oerkki1SAO
*/

Oerkki1SAO::Oerkki1SAO(ServerEnvironment *env, v3f pos):
	ServerActiveObject(env, pos),
	m_is_active(false),
	m_speed_f(0,0,0)
{
	ServerActiveObject::registerType(getType(), create);

	m_oldpos = v3f(0,0,0);
	m_last_sent_position = v3f(0,0,0);
	m_yaw = 0;
	m_counter1 = 0;
	m_counter2 = 0;
	m_age = 0;
	m_touching_ground = false;
	m_hp = 20;
	m_after_jump_timer = 0;
}

ServerActiveObject* Oerkki1SAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	// read version
	u8 version = readU8(is);
	// read hp
	u8 hp = readU8(is);
	// check if version is supported
	if(version != 0)
		return NULL;
	Oerkki1SAO *o = new Oerkki1SAO(env, pos);
	o->m_hp = hp;
	return o;
}

void Oerkki1SAO::step(float dtime, bool send_recommended)
{
	ScopeProfiler sp2(g_profiler, "Oerkki1SAO::step avg", SPT_AVG);

	assert(m_env);

	if(m_is_active == false)
	{
		if(m_inactive_interval.step(dtime, 0.5)==false)
			return;
	}

	/*
		The AI
	*/

	m_age += dtime;
	if(m_age > 120)
	{
		// Die
		m_removed = true;
		return;
	}

	m_after_jump_timer -= dtime;

	v3f old_speed = m_speed_f;

	// Apply gravity
	m_speed_f.Y -= dtime*9.81*BS;

	/*
		Move around if some player is close
	*/
	bool player_is_close = false;
	bool player_is_too_close = false;
	v3f near_player_pos;
	// Check connected players
	core::list<Player*> players = m_env->getPlayers(true);
	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		Player *player = *i;
		v3f playerpos = player->getPosition();
		f32 dist = m_base_position.getDistanceFrom(playerpos);
		if(dist < BS*0.6)
		{
			m_removed = true;
			return;
			player_is_too_close = true;
			near_player_pos = playerpos;
		}
		else if(dist < BS*15.0 && !player_is_too_close)
		{
			player_is_close = true;
			near_player_pos = playerpos;
		}
	}

	m_is_active = player_is_close;

	v3f target_speed = m_speed_f;

	if(!player_is_close)
	{
		target_speed = v3f(0,0,0);
	}
	else
	{
		// Move around

		v3f ndir = near_player_pos - m_base_position;
		ndir.Y = 0;
		ndir.normalize();

		f32 nyaw = 180./PI*atan2(ndir.Z,ndir.X);
		if(nyaw < m_yaw - 180)
			nyaw += 360;
		else if(nyaw > m_yaw + 180)
			nyaw -= 360;
		m_yaw = 0.95*m_yaw + 0.05*nyaw;
		m_yaw = wrapDegrees(m_yaw);

		f32 speed = 2*BS;

		if((m_touching_ground || m_after_jump_timer > 0.0)
				&& !player_is_too_close)
		{
			v3f dir(cos(m_yaw/180*PI),0,sin(m_yaw/180*PI));
			target_speed.X = speed * dir.X;
			target_speed.Z = speed * dir.Z;
		}

		if(m_touching_ground && (m_oldpos - m_base_position).getLength()
				< dtime*speed/2)
		{
			m_counter1 -= dtime;
			if(m_counter1 < 0.0)
			{
				m_counter1 += 0.2;
				// Jump
				target_speed.Y = 5.0*BS;
				m_after_jump_timer = 1.0;
			}
		}

		{
			m_counter2 -= dtime;
			if(m_counter2 < 0.0)
			{
				m_counter2 += (float)(myrand()%100)/100*3.0;
				//m_yaw += ((float)(myrand()%200)-100)/100*180;
				m_yaw += ((float)(myrand()%200)-100)/100*90;
				m_yaw = wrapDegrees(m_yaw);
			}
		}
	}

	if((m_speed_f - target_speed).getLength() > BS*4 || player_is_too_close)
		accelerate_xz(m_speed_f, target_speed, dtime*BS*8);
	else
		accelerate_xz(m_speed_f, target_speed, dtime*BS*4);

	m_oldpos = m_base_position;

	/*
		Move it, with collision detection
	*/

	core::aabbox3d<f32> box(-BS/3.,0.0,-BS/3., BS/3.,BS*5./3.,BS/3.);
	collisionMoveResult moveresult;
	// Maximum movement without glitches
	f32 pos_max_d = BS*0.25;
	/*// Limit speed
	if(m_speed_f.getLength()*dtime > pos_max_d)
		m_speed_f *= pos_max_d / (m_speed_f.getLength()*dtime);*/
	v3f pos_f = getBasePosition();
	v3f pos_f_old = pos_f;
	IGameDef *gamedef = m_env->getGameDef();
	moveresult = collisionMovePrecise(&m_env->getMap(), gamedef,
			pos_max_d, box, dtime, pos_f, m_speed_f);
	m_touching_ground = moveresult.touching_ground;

	// Do collision damage
	float tolerance = BS*30;
	float factor = BS*0.5;
	v3f speed_diff = old_speed - m_speed_f;
	// Increase effect in X and Z
	speed_diff.X *= 2;
	speed_diff.Z *= 2;
	float vel = speed_diff.getLength();
	if(vel > tolerance)
	{
		f32 damage_f = (vel - tolerance)/BS*factor;
		u16 damage = (u16)(damage_f+0.5);
		doDamage(damage);
	}

	setBasePosition(pos_f);

	if(send_recommended == false && m_speed_f.getLength() < 3.0*BS)
		return;

	if(pos_f.getDistanceFrom(m_last_sent_position) > 0.05*BS)
	{
		m_last_sent_position = pos_f;

		std::ostringstream os(std::ios::binary);
		// command (0 = update position)
		writeU8(os, AO_Message_type::SetPosition);
		// pos
		writeV3F1000(os, m_base_position);
		// yaw
		writeF1000(os, m_yaw);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

std::string Oerkki1SAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	return os.str();
}

std::string Oerkki1SAO::getStaticData()
{
	//infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// hp
	writeU8(os, m_hp);
	return os.str();
}

void Oerkki1SAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	if(!puncher)
		return;

	v3f dir = (getBasePosition() - puncher->getBasePosition()).normalize();
	m_speed_f += dir*12*BS;

	// "Material" properties of an oerkki
	MaterialProperties mp;
	mp.diggability = DIGGABLE_NORMAL;
	mp.crackiness = -1.0;
	mp.cuttability = 1.0;

	ToolDiggingProperties tp;
	puncher->getWieldDiggingProperties(&tp);

	HittingProperties hitprop = getHittingProperties(&mp, &tp,
			time_from_last_punch);

	doDamage(hitprop.hp);
	puncher->damageWieldedItem(hitprop.wear);
}

void Oerkki1SAO::doDamage(u16 d)
{
	infostream<<"oerkki damage: "<<d<<std::endl;

	if(d < m_hp)
	{
		m_hp -= d;
	}
	else
	{
		// Die
		m_hp = 0;
		m_removed = true;
	}

	{
		std::ostringstream os(std::ios::binary);
		// command (1 = damage)
		writeU8(os, AO_Message_type::TakeDamage);
		// amount
		writeU8(os, d);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

// Prototype
Oerkki1SAO proto_Oerkki1SAO(NULL, v3f(0,0,0));
