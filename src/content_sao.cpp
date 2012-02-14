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

#include "content_sao.h"
#include "collision.h"
#include "environment.h"
#include "settings.h"
#include "main.h" // For g_profiler
#include "profiler.h"
#include "serialization.h" // For compressZlib
#include "materials.h" // For MaterialProperties and ToolDiggingProperties
#include "gamedef.h"

core::map<u16, ServerActiveObject::Factory> ServerActiveObject::m_types;

/* Some helper functions */

// Y is copied, X and Z change is limited
void accelerate_xz(v3f &speed, v3f target_speed, f32 max_increase)
{
	v3f d_wanted = target_speed - speed;
	d_wanted.Y = 0;
	f32 dl_wanted = d_wanted.getLength();
	f32 dl = dl_wanted;
	if(dl > max_increase)
		dl = max_increase;
	
	v3f d = d_wanted.normalize() * dl;

	speed.X += d.X;
	speed.Z += d.Z;
	speed.Y = target_speed.Y;
}

/*
	TestSAO
*/

// Prototype
TestSAO proto_TestSAO(NULL, v3f(0,0,0));

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

		data += itos(0); // 0 = position
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


/*
	ItemSAO
*/

// Prototype
ItemSAO proto_ItemSAO(NULL, v3f(0,0,0), "");

ItemSAO::ItemSAO(ServerEnvironment *env, v3f pos,
		const std::string itemstring):
	ServerActiveObject(env, pos),
	m_itemstring(itemstring),
	m_itemstring_changed(false),
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
	std::string itemstring = deSerializeString(is);
	infostream<<"ItemSAO::create(): Creating item \""
			<<itemstring<<"\""<<std::endl;
	return new ItemSAO(env, pos, itemstring);
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
		// command (0 = update position)
		writeU8(os, 0);
		// pos
		writeV3F1000(os, m_base_position);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
	if(m_itemstring_changed)
	{
		m_itemstring_changed = false;

		std::ostringstream os(std::ios::binary);
		// command (1 = update itemstring)
		writeU8(os, 1);
		// itemstring
		os<<serializeString(m_itemstring);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

std::string ItemSAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	// itemstring
	os<<serializeString(m_itemstring);
	return os.str();
}

std::string ItemSAO::getStaticData()
{
	infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// itemstring
	os<<serializeString(m_itemstring);
	return os.str();
}

ItemStack ItemSAO::createItemStack()
{
	try{
		IItemDefManager *idef = m_env->getGameDef()->idef();
		ItemStack item;
		item.deSerialize(m_itemstring, idef);
		infostream<<__FUNCTION_NAME<<": m_itemstring=\""<<m_itemstring
				<<"\" -> item=\""<<item.getItemString()<<"\""
				<<std::endl;
		return item;
	}
	catch(SerializationError &e)
	{
		infostream<<__FUNCTION_NAME<<": serialization error: "
				<<"m_itemstring=\""<<m_itemstring<<"\""<<std::endl;
		return ItemStack();
	}
}

void ItemSAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	// Allow removing items in creative mode
	if(g_settings->getBool("creative_mode") == true)
	{
		m_removed = true;
		return;
	}

	ItemStack item = createItemStack();
	Inventory *inv = puncher->getInventory();
	if(inv != NULL)
	{
		std::string wieldlist = puncher->getWieldList();
		ItemStack leftover = inv->addItem(wieldlist, item);
		puncher->setInventoryModified();
		if(leftover.empty())
		{
			m_removed = true;
		}
		else
		{
			m_itemstring = leftover.getItemString();
			m_itemstring_changed = true;
		}
	}
}

/*
	RatSAO
*/

// Prototype
RatSAO proto_RatSAO(NULL, v3f(0,0,0));

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
	// Allow removing rats in creative mode
	if(g_settings->getBool("creative_mode") == true)
	{
		m_removed = true;
		return;
	}

	IItemDefManager *idef = m_env->getGameDef()->idef();
	ItemStack item("rat", 1, 0, "", idef);
	Inventory *inv = puncher->getInventory();
	if(inv != NULL)
	{
		std::string wieldlist = puncher->getWieldList();
		ItemStack leftover = inv->addItem(wieldlist, item);
		puncher->setInventoryModified();
		if(leftover.empty())
			m_removed = true;
	}
}

/*
	Oerkki1SAO
*/

// Prototype
Oerkki1SAO proto_Oerkki1SAO(NULL, v3f(0,0,0));

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

	IItemDefManager *idef = m_env->getGameDef()->idef();
        ItemStack punchitem = puncher->getWieldedItem();
        ToolDiggingProperties tp =
	                punchitem.getToolDiggingProperties(idef);

	HittingProperties hitprop = getHittingProperties(&mp, &tp,
			time_from_last_punch);

	doDamage(hitprop.hp);
	if(g_settings->getBool("creative_mode") == false)
	{
		punchitem.addWear(hitprop.wear, idef);
		puncher->setWieldedItem(punchitem);
	}
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
		writeU8(os, 1);
		// amount
		writeU8(os, d);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

/*
	FireflySAO
*/

// Prototype
FireflySAO proto_FireflySAO(NULL, v3f(0,0,0));

FireflySAO::FireflySAO(ServerEnvironment *env, v3f pos):
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
}

ServerActiveObject* FireflySAO::create(ServerEnvironment *env, v3f pos,
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
	return new FireflySAO(env, pos);
}

void FireflySAO::step(float dtime, bool send_recommended)
{
	ScopeProfiler sp2(g_profiler, "FireflySAO::step avg", SPT_AVG);

	assert(m_env);

	if(m_is_active == false)
	{
		if(m_inactive_interval.step(dtime, 0.5)==false)
			return;
	}

	/*
		The AI
	*/

	// Apply (less) gravity
	m_speed_f.Y -= dtime*3*BS;

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
		f32 speed = BS/2;
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

	core::aabbox3d<f32> box(-BS/3.,-BS*2/3.0,-BS/3., BS/3.,BS*4./3.,BS/3.);
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

std::string FireflySAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	return os.str();
}

std::string FireflySAO::getStaticData()
{
	//infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	return os.str();
}

/*
	MobV2SAO
*/

// Prototype
MobV2SAO proto_MobV2SAO(NULL, v3f(0,0,0), NULL);

MobV2SAO::MobV2SAO(ServerEnvironment *env, v3f pos,
		Settings *init_properties):
	ServerActiveObject(env, pos),
	m_move_type("ground_nodes"),
	m_speed(0,0,0),
	m_last_sent_position(0,0,0),
	m_oldpos(0,0,0),
	m_yaw(0),
	m_counter1(0),
	m_counter2(0),
	m_age(0),
	m_touching_ground(false),
	m_hp(10),
	m_walk_around(false),
	m_walk_around_timer(0),
	m_next_pos_exists(false),
	m_shoot_reload_timer(0),
	m_shooting(false),
	m_shooting_timer(0),
	m_falling(false),
	m_disturb_timer(100000),
	m_random_disturb_timer(0),
	m_shoot_y(0)
{
	ServerActiveObject::registerType(getType(), create);
	
	m_properties = new Settings();
	if(init_properties)
		m_properties->update(*init_properties);
	
	m_properties->setV3F("pos", pos);
	
	setPropertyDefaults();
	readProperties();
}
	
MobV2SAO::~MobV2SAO()
{
	delete m_properties;
}

ServerActiveObject* MobV2SAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	Settings properties;
	properties.parseConfigLines(is, "MobArgsEnd");
	MobV2SAO *o = new MobV2SAO(env, pos, &properties);
	return o;
}

std::string MobV2SAO::getStaticData()
{
	updateProperties();

	std::ostringstream os(std::ios::binary);
	m_properties->writeLines(os);
	return os.str();
}

std::string MobV2SAO::getClientInitializationData()
{
	//infostream<<__FUNCTION_NAME<<std::endl;

	updateProperties();

	std::ostringstream os(std::ios::binary);

	// version
	writeU8(os, 0);
	
	Settings client_properties;
	
	/*client_properties.set("version", "0");
	client_properties.updateValue(*m_properties, "pos");
	client_properties.updateValue(*m_properties, "yaw");
	client_properties.updateValue(*m_properties, "hp");*/

	// Just send everything for simplicity
	client_properties.update(*m_properties);

	std::ostringstream os2(std::ios::binary);
	client_properties.writeLines(os2);
	compressZlib(os2.str(), os);

	return os.str();
}

bool checkFreePosition(Map *map, v3s16 p0, v3s16 size)
{
	for(int dx=0; dx<size.X; dx++)
	for(int dy=0; dy<size.Y; dy++)
	for(int dz=0; dz<size.Z; dz++){
		v3s16 dp(dx, dy, dz);
		v3s16 p = p0 + dp;
		MapNode n = map->getNodeNoEx(p);
		if(n.getContent() != CONTENT_AIR)
			return false;
	}
	return true;
}

bool checkWalkablePosition(Map *map, v3s16 p0)
{
	v3s16 p = p0 + v3s16(0,-1,0);
	MapNode n = map->getNodeNoEx(p);
	if(n.getContent() != CONTENT_AIR)
		return true;
	return false;
}

bool checkFreeAndWalkablePosition(Map *map, v3s16 p0, v3s16 size)
{
	if(!checkFreePosition(map, p0, size))
		return false;
	if(!checkWalkablePosition(map, p0))
		return false;
	return true;
}

static void get_random_u32_array(u32 a[], u32 len)
{
	u32 i, n;
	for(i=0; i<len; i++)
		a[i] = i;
	n = len;
	while(n > 1){
		u32 k = myrand() % n;
		n--;
		u32 temp = a[n];
		a[n] = a[k];
		a[k] = temp;
	}
}

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

static void explodeSquare(Map *map, v3s16 p0, v3s16 size)
{
	core::map<v3s16, MapBlock*> modified_blocks;

	for(int dx=0; dx<size.X; dx++)
	for(int dy=0; dy<size.Y; dy++)
	for(int dz=0; dz<size.Z; dz++){
		v3s16 dp(dx - size.X/2, dy - size.Y/2, dz - size.Z/2);
		v3s16 p = p0 + dp;
		MapNode n = map->getNodeNoEx(p);
		if(n.getContent() == CONTENT_IGNORE)
			continue;
		//map->removeNodeWithEvent(p);
		map->removeNodeAndUpdate(p, modified_blocks);
	}

	// Send a MEET_OTHER event
	MapEditEvent event;
	event.type = MEET_OTHER;
	for(core::map<v3s16, MapBlock*>::Iterator
		  i = modified_blocks.getIterator();
		  i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		event.modified_blocks.insert(p, true);
	}
	map->dispatchEvent(&event);
}

void MobV2SAO::step(float dtime, bool send_recommended)
{
	ScopeProfiler sp2(g_profiler, "MobV2SAO::step avg", SPT_AVG);

	assert(m_env);
	Map *map = &m_env->getMap();

	m_age += dtime;

	if(m_die_age >= 0.0 && m_age >= m_die_age){
		m_removed = true;
		return;
	}

	m_random_disturb_timer += dtime;
	if(m_random_disturb_timer >= 5.0)
	{
		m_random_disturb_timer = 0;
		// Check connected players
		core::list<Player*> players = m_env->getPlayers(true);
		core::list<Player*>::Iterator i;
		for(i = players.begin();
				i != players.end(); i++)
		{
			Player *player = *i;
			v3f playerpos = player->getPosition();
			f32 dist = m_base_position.getDistanceFrom(playerpos);
			if(dist < BS*16)
			{
				if(myrand_range(0,3) == 0){
					actionstream<<"Mob id="<<m_id<<" at "
							<<PP(m_base_position/BS)
							<<" got randomly disturbed by "
							<<player->getName()<<std::endl;
					m_disturbing_player = player->getName();
					m_disturb_timer = 0;
					break;
				}
			}
		}
	}

	Player *disturbing_player =
			m_env->getPlayer(m_disturbing_player.c_str());
	v3f disturbing_player_off = v3f(0,1,0);
	v3f disturbing_player_norm = v3f(0,1,0);
	float disturbing_player_distance = 1000000;
	float disturbing_player_dir = 0;
	if(disturbing_player){
		disturbing_player_off =
				disturbing_player->getPosition() - m_base_position;
		disturbing_player_distance = disturbing_player_off.getLength();
		disturbing_player_norm = disturbing_player_off;
		disturbing_player_norm.normalize();
		disturbing_player_dir = 180./PI*atan2(disturbing_player_norm.Z,
				disturbing_player_norm.X);
	}

	m_disturb_timer += dtime;
	
	if(!m_falling)
	{
		m_shooting_timer -= dtime;
		if(m_shooting_timer <= 0.0 && m_shooting){
			m_shooting = false;
			
			std::string shoot_type = m_properties->get("shoot_type");
			v3f shoot_pos(0,0,0);
			shoot_pos.Y += m_properties->getFloat("shoot_y") * BS;
			if(shoot_type == "fireball"){
				v3f dir(cos(m_yaw/180*PI),0,sin(m_yaw/180*PI));
				dir.Y = m_shoot_y;
				dir.normalize();
				v3f speed = dir * BS * 10.0;
				v3f pos = m_base_position + shoot_pos;
				infostream<<__FUNCTION_NAME<<": Mob id="<<m_id
						<<" shooting fireball from "<<PP(pos)
						<<" at speed "<<PP(speed)<<std::endl;
				Settings properties;
				properties.set("looks", "fireball");
				properties.setV3F("speed", speed);
				properties.setFloat("die_age", 5.0);
				properties.set("move_type", "constant_speed");
				properties.setFloat("hp", 1000);
				properties.set("lock_full_brightness", "true");
				properties.set("player_hit_damage", "9");
				properties.set("player_hit_distance", "2");
				properties.set("player_hit_interval", "1");
				ServerActiveObject *obj = new MobV2SAO(m_env,
						pos, &properties);
				//m_env->addActiveObjectAsStatic(obj);
				m_env->addActiveObject(obj);
			} else {
				infostream<<__FUNCTION_NAME<<": Mob id="<<m_id
						<<": Unknown shoot_type="<<shoot_type
						<<std::endl;
			}
		}

		m_shoot_reload_timer += dtime;

		float reload_time = 15.0;
		if(m_disturb_timer <= 15.0)
			reload_time = 3.0;

		bool shoot_without_player = false;
		if(m_properties->getBool("mindless_rage"))
			shoot_without_player = true;

		if(!m_shooting && m_shoot_reload_timer >= reload_time &&
				!m_next_pos_exists &&
				(m_disturb_timer <= 60.0 || shoot_without_player))
		{
			m_shoot_y = 0;
			if(m_disturb_timer < 60.0 && disturbing_player &&
					disturbing_player_distance < 16*BS &&
					fabs(disturbing_player_norm.Y) < 0.8){
				m_yaw = disturbing_player_dir;
				sendPosition();
				m_shoot_y += disturbing_player_norm.Y;
			} else {
				m_shoot_y = 0.01 * myrand_range(-30,10);
			}
			m_shoot_reload_timer = 0.0;
			m_shooting = true;
			m_shooting_timer = 1.5;
			{
				std::ostringstream os(std::ios::binary);
				// command (2 = shooting)
				writeU8(os, 2);
				// time
				writeF1000(os, m_shooting_timer + 0.1);
				// bright?
				writeU8(os, true);
				// create message and add to list
				ActiveObjectMessage aom(getId(), false, os.str());
				m_messages_out.push_back(aom);
			}
		}
	}
	
	if(m_move_type == "ground_nodes")
	{
		if(!m_shooting){
			m_walk_around_timer -= dtime;
			if(m_walk_around_timer <= 0.0){
				m_walk_around = !m_walk_around;
				if(m_walk_around)
					m_walk_around_timer = 0.1*myrand_range(10,50);
				else
					m_walk_around_timer = 0.1*myrand_range(30,70);
			}
		}

		/* Move */
		if(m_next_pos_exists){
			v3f pos_f = m_base_position;
			v3f next_pos_f = intToFloat(m_next_pos_i, BS);

			v3f v = next_pos_f - pos_f;
			m_yaw = atan2(v.Z, v.X) / PI * 180;
			
			v3f diff = next_pos_f - pos_f;
			v3f dir = diff;
			dir.normalize();
			float speed = BS * 0.5;
			if(m_falling)
				speed = BS * 3.0;
			dir *= dtime * speed;
			bool arrived = false;
			if(dir.getLength() > diff.getLength()){
				dir = diff;
				arrived = true;
			}
			pos_f += dir;
			m_base_position = pos_f;

			if((pos_f - next_pos_f).getLength() < 0.1 || arrived){
				m_next_pos_exists = false;
			}
		}

		v3s16 pos_i = floatToInt(m_base_position, BS);
		v3s16 size_blocks = v3s16(m_size.X+0.5,m_size.Y+0.5,m_size.X+0.5);
		v3s16 pos_size_off(0,0,0);
		if(m_size.X >= 2.5){
			pos_size_off.X = -1;
			pos_size_off.Y = -1;
		}
		
		if(!m_next_pos_exists){
			/* Check whether to drop down */
			if(checkFreePosition(map,
					pos_i + pos_size_off + v3s16(0,-1,0), size_blocks)){
				m_next_pos_i = pos_i + v3s16(0,-1,0);
				m_next_pos_exists = true;
				m_falling = true;
			} else {
				m_falling = false;
			}
		}

		if(m_walk_around)
		{
			if(!m_next_pos_exists){
				/* Find some position where to go next */
				v3s16 dps[3*3*3];
				int num_dps = 0;
				for(int dx=-1; dx<=1; dx++)
				for(int dy=-1; dy<=1; dy++)
				for(int dz=-1; dz<=1; dz++){
					if(dx == 0 && dy == 0)
						continue;
					if(dx != 0 && dz != 0 && dy != 0)
						continue;
					dps[num_dps++] = v3s16(dx,dy,dz);
				}
				u32 order[3*3*3];
				get_random_u32_array(order, num_dps);
				for(int i=0; i<num_dps; i++){
					v3s16 p = dps[order[i]] + pos_i;
					bool is_free = checkFreeAndWalkablePosition(map,
							p + pos_size_off, size_blocks);
					if(!is_free)
						continue;
					m_next_pos_i = p;
					m_next_pos_exists = true;
					break;
				}
			}
		}
	}
	else if(m_move_type == "constant_speed")
	{
		m_base_position += m_speed * dtime;
		
		v3s16 pos_i = floatToInt(m_base_position, BS);
		v3s16 size_blocks = v3s16(m_size.X+0.5,m_size.Y+0.5,m_size.X+0.5);
		v3s16 pos_size_off(0,0,0);
		if(m_size.X >= 2.5){
			pos_size_off.X = -1;
			pos_size_off.Y = -1;
		}
		bool free = checkFreePosition(map, pos_i + pos_size_off, size_blocks);
		if(!free){
			explodeSquare(map, pos_i, v3s16(3,3,3));
			m_removed = true;
			return;
		}
	}
	else
	{
		errorstream<<"MobV2SAO::step(): id="<<m_id<<" unknown move_type=\""
				<<m_move_type<<"\""<<std::endl;
	}

	if(send_recommended == false)
		return;

	if(m_base_position.getDistanceFrom(m_last_sent_position) > 0.05*BS)
	{
		sendPosition();
	}
}

void MobV2SAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	if(!puncher)
		return;
	
	v3f dir = (getBasePosition() - puncher->getBasePosition()).normalize();

	// A quick hack; SAO description is player name for player
	std::string playername = puncher->getDescription();

	Map *map = &m_env->getMap();
	
	actionstream<<playername<<" punches mob id="<<m_id
			<<" at "<<PP(m_base_position/BS)<<std::endl;

	m_disturb_timer = 0;
	m_disturbing_player = playername;
	m_next_pos_exists = false; // Cancel moving immediately
	
	m_yaw = wrapDegrees_180(180./PI*atan2(dir.Z, dir.X) + 180.);
	v3f new_base_position = m_base_position + dir * BS;
	{
		v3s16 pos_i = floatToInt(new_base_position, BS);
		v3s16 size_blocks = v3s16(m_size.X+0.5,m_size.Y+0.5,m_size.X+0.5);
		v3s16 pos_size_off(0,0,0);
		if(m_size.X >= 2.5){
			pos_size_off.X = -1;
			pos_size_off.Y = -1;
		}
		bool free = checkFreePosition(map, pos_i + pos_size_off, size_blocks);
		if(free)
			m_base_position = new_base_position;
	}
	sendPosition();
	

	// "Material" properties of the MobV2
	MaterialProperties mp;
	mp.diggability = DIGGABLE_NORMAL;
	mp.crackiness = -1.0;
	mp.cuttability = 1.0;

	IItemDefManager *idef = m_env->getGameDef()->idef();
        ItemStack punchitem = puncher->getWieldedItem();
        ToolDiggingProperties tp =
	                punchitem.getToolDiggingProperties(idef);

	HittingProperties hitprop = getHittingProperties(&mp, &tp,
			time_from_last_punch);

	doDamage(hitprop.hp);
	if(g_settings->getBool("creative_mode") == false)
	{
		punchitem.addWear(hitprop.wear, idef);
		puncher->setWieldedItem(punchitem);
	}
}

bool MobV2SAO::isPeaceful()
{
	return m_properties->getBool("is_peaceful");
}

void MobV2SAO::sendPosition()
{
	m_last_sent_position = m_base_position;

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

void MobV2SAO::setPropertyDefaults()
{
	m_properties->setDefault("is_peaceful", "false");
	m_properties->setDefault("move_type", "ground_nodes");
	m_properties->setDefault("speed", "(0,0,0)");
	m_properties->setDefault("age", "0");
	m_properties->setDefault("yaw", "0");
	m_properties->setDefault("pos", "(0,0,0)");
	m_properties->setDefault("hp", "0");
	m_properties->setDefault("die_age", "-1");
	m_properties->setDefault("size", "(1,2)");
	m_properties->setDefault("shoot_type", "fireball");
	m_properties->setDefault("shoot_y", "0");
	m_properties->setDefault("mindless_rage", "false");
}
void MobV2SAO::readProperties()
{
	m_move_type = m_properties->get("move_type");
	m_speed = m_properties->getV3F("speed");
	m_age = m_properties->getFloat("age");
	m_yaw = m_properties->getFloat("yaw");
	m_base_position = m_properties->getV3F("pos");
	m_hp = m_properties->getS32("hp");
	m_die_age = m_properties->getFloat("die_age");
	m_size = m_properties->getV2F("size");
}
void MobV2SAO::updateProperties()
{
	m_properties->set("move_type", m_move_type);
	m_properties->setV3F("speed", m_speed);
	m_properties->setFloat("age", m_age);
	m_properties->setFloat("yaw", m_yaw);
	m_properties->setV3F("pos", m_base_position);
	m_properties->setS32("hp", m_hp);
	m_properties->setFloat("die_age", m_die_age);
	m_properties->setV2F("size", m_size);

	m_properties->setS32("version", 0);
}

void MobV2SAO::doDamage(u16 d)
{
	infostream<<"MobV2 hp="<<m_hp<<" damage="<<d<<std::endl;
	
	if(d < m_hp)
	{
		m_hp -= d;
	}
	else
	{
		actionstream<<"A "<<(isPeaceful()?"peaceful":"non-peaceful")
				<<" mob id="<<m_id<<" dies at "<<PP(m_base_position)<<std::endl;
		// Die
		m_hp = 0;
		m_removed = true;
	}

	{
		std::ostringstream os(std::ios::binary);
		// command (1 = damage)
		writeU8(os, 1);
		// amount
		writeU16(os, d);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}


/*
	LuaEntitySAO
*/

#include "scriptapi.h"
#include "luaentity_common.h"

// Prototype
LuaEntitySAO proto_LuaEntitySAO(NULL, v3f(0,0,0), "_prototype", "");

LuaEntitySAO::LuaEntitySAO(ServerEnvironment *env, v3f pos,
		const std::string &name, const std::string &state):
	ServerActiveObject(env, pos),
	m_init_name(name),
	m_init_state(state),
	m_registered(false),
	m_prop(new LuaEntityProperties),
	m_velocity(0,0,0),
	m_acceleration(0,0,0),
	m_yaw(0),
	m_last_sent_yaw(0),
	m_last_sent_position(0,0,0),
	m_last_sent_velocity(0,0,0),
	m_last_sent_position_timer(0),
	m_last_sent_move_precision(0)
{
	// Only register type if no environment supplied
	if(env == NULL){
		ServerActiveObject::registerType(getType(), create);
		return;
	}
}

LuaEntitySAO::~LuaEntitySAO()
{
	if(m_registered){
		lua_State *L = m_env->getLua();
		scriptapi_luaentity_rm(L, m_id);
	}
	delete m_prop;
}

void LuaEntitySAO::addedToEnvironment()
{
	ServerActiveObject::addedToEnvironment();
	
	// Create entity from name and state
	lua_State *L = m_env->getLua();
	m_registered = scriptapi_luaentity_add(L, m_id, m_init_name.c_str(),
			m_init_state.c_str());
	
	if(m_registered){
		// Get properties
		scriptapi_luaentity_get_properties(L, m_id, m_prop);
	}
}

ServerActiveObject* LuaEntitySAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	// read version
	u8 version = readU8(is);
	// check if version is supported
	if(version != 0)
		return NULL;
	// read name
	std::string name = deSerializeString(is);
	// read state
	std::string state = deSerializeLongString(is);
	// create object
	infostream<<"LuaEntitySAO::create(name=\""<<name<<"\" state=\""
			<<state<<"\")"<<std::endl;
	return new LuaEntitySAO(env, pos, name, state);
}

void LuaEntitySAO::step(float dtime, bool send_recommended)
{
	m_last_sent_position_timer += dtime;
	
	if(m_prop->physical){
		core::aabbox3d<f32> box = m_prop->collisionbox;
		box.MinEdge *= BS;
		box.MaxEdge *= BS;
		collisionMoveResult moveresult;
		f32 pos_max_d = BS*0.25; // Distance per iteration
		v3f p_pos = getBasePosition();
		v3f p_velocity = m_velocity;
		IGameDef *gamedef = m_env->getGameDef();
		moveresult = collisionMovePrecise(&m_env->getMap(), gamedef,
				pos_max_d, box, dtime, p_pos, p_velocity);
		// Apply results
		setBasePosition(p_pos);
		m_velocity = p_velocity;

		m_velocity += dtime * m_acceleration;
	} else {
		m_base_position += dtime * m_velocity + 0.5 * dtime
				* dtime * m_acceleration;
		m_velocity += dtime * m_acceleration;
	}

	if(m_registered){
		lua_State *L = m_env->getLua();
		scriptapi_luaentity_step(L, m_id, dtime);
	}

	if(send_recommended == false)
		return;
	
	// TODO: force send when acceleration changes enough?
	float minchange = 0.2*BS;
	if(m_last_sent_position_timer > 1.0){
		minchange = 0.01*BS;
	} else if(m_last_sent_position_timer > 0.2){
		minchange = 0.05*BS;
	}
	float move_d = m_base_position.getDistanceFrom(m_last_sent_position);
	move_d += m_last_sent_move_precision;
	float vel_d = m_velocity.getDistanceFrom(m_last_sent_velocity);
	if(move_d > minchange || vel_d > minchange ||
			fabs(m_yaw - m_last_sent_yaw) > 1.0){
		sendPosition(true, false);
	}
}

std::string LuaEntitySAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	// yaw
	writeF1000(os, m_yaw);
	// properties
	std::ostringstream prop_os(std::ios::binary);
	m_prop->serialize(prop_os);
	os<<serializeLongString(prop_os.str());
	// return result
	return os.str();
}

std::string LuaEntitySAO::getStaticData()
{
	infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// name
	os<<serializeString(m_init_name);
	// state
	if(m_registered){
		lua_State *L = m_env->getLua();
		std::string state = scriptapi_luaentity_get_staticdata(L, m_id);
		os<<serializeLongString(state);
	} else {
		os<<serializeLongString(m_init_state);
	}
	return os.str();
}

void LuaEntitySAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	if(!m_registered){
		// Delete unknown LuaEntities when punched
		m_removed = true;
		return;
	}
	lua_State *L = m_env->getLua();
	scriptapi_luaentity_punch(L, m_id, puncher, time_from_last_punch);
}

void LuaEntitySAO::rightClick(ServerActiveObject *clicker)
{
	if(!m_registered)
		return;
	lua_State *L = m_env->getLua();
	scriptapi_luaentity_rightclick(L, m_id, clicker);
}

void LuaEntitySAO::setPos(v3f pos)
{
	m_base_position = pos;
	sendPosition(false, true);
}

void LuaEntitySAO::moveTo(v3f pos, bool continuous)
{
	m_base_position = pos;
	if(!continuous)
		sendPosition(true, true);
}

float LuaEntitySAO::getMinimumSavedMovement()
{
	return 0.1 * BS;
}

void LuaEntitySAO::setVelocity(v3f velocity)
{
	m_velocity = velocity;
}

v3f LuaEntitySAO::getVelocity()
{
	return m_velocity;
}

void LuaEntitySAO::setAcceleration(v3f acceleration)
{
	m_acceleration = acceleration;
}

v3f LuaEntitySAO::getAcceleration()
{
	return m_acceleration;
}

void LuaEntitySAO::setYaw(float yaw)
{
	m_yaw = yaw;
}

float LuaEntitySAO::getYaw()
{
	return m_yaw;
}

void LuaEntitySAO::setTextureMod(const std::string &mod)
{
	std::ostringstream os(std::ios::binary);
	// command (1 = set texture modification)
	writeU8(os, 1);
	// parameters
	os<<serializeString(mod);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}

void LuaEntitySAO::setSprite(v2s16 p, int num_frames, float framelength,
		bool select_horiz_by_yawpitch)
{
	std::ostringstream os(std::ios::binary);
	// command (2 = set sprite)
	writeU8(os, 2);
	// parameters
	writeV2S16(os, p);
	writeU16(os, num_frames);
	writeF1000(os, framelength);
	writeU8(os, select_horiz_by_yawpitch);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}

std::string LuaEntitySAO::getName()
{
	return m_init_name;
}

void LuaEntitySAO::sendPosition(bool do_interpolate, bool is_movement_end)
{
	m_last_sent_move_precision = m_base_position.getDistanceFrom(
			m_last_sent_position);
	m_last_sent_position_timer = 0;
	m_last_sent_yaw = m_yaw;
	m_last_sent_position = m_base_position;
	m_last_sent_velocity = m_velocity;
	//m_last_sent_acceleration = m_acceleration;

	float update_interval = m_env->getSendRecommendedInterval();

	std::ostringstream os(std::ios::binary);
	// command (0 = update position)
	writeU8(os, 0);

	// do_interpolate
	writeU8(os, do_interpolate);
	// pos
	writeV3F1000(os, m_base_position);
	// velocity
	writeV3F1000(os, m_velocity);
	// acceleration
	writeV3F1000(os, m_acceleration);
	// yaw
	writeF1000(os, m_yaw);
	// is_end_position (for interpolation)
	writeU8(os, is_movement_end);
	// update_interval (for interpolation)
	writeF1000(os, update_interval);

	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}

