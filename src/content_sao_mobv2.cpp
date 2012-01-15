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

#include "content_sao_mobv2.h"

/*
	MobV2SAO
*/

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
				writeU8(os, AO_Message_type::Shoot);
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

	ToolDiggingProperties tp;
	puncher->getWieldDiggingProperties(&tp);

	HittingProperties hitprop = getHittingProperties(&mp, &tp,
			time_from_last_punch);

	doDamage(hitprop.hp);
	puncher->damageWieldedItem(hitprop.wear);
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
	writeU8(os, AO_Message_type::SetPosition);
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
		writeU8(os, AO_Message_type::TakeDamage);
		// amount
		writeU16(os, d);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, os.str());
		m_messages_out.push_back(aom);
	}
}

// Prototype
MobV2SAO proto_MobV2SAO(NULL, v3f(0,0,0), NULL);
