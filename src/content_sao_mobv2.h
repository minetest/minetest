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


#ifndef CONTENT_SAO_MOBV2_H_
#define CONTENT_SAO_MOBV2_H_

#include "content_sao.h"

class MobV2SAO : public ServerActiveObject
{
public:
	MobV2SAO(ServerEnvironment *env, v3f pos,
			Settings *init_properties);
	virtual ~MobV2SAO();
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_MOBV2;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	std::string getStaticData();
	std::string getClientInitializationData();
	void step(float dtime, bool send_recommended);
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
	bool isPeaceful();
private:
	void sendPosition();
	void setPropertyDefaults();
	void readProperties();
	void updateProperties();
	void doDamage(u16 d);

	std::string m_move_type;
	v3f m_speed;
	v3f m_last_sent_position;
	v3f m_oldpos;
	float m_yaw;
	float m_counter1;
	float m_counter2;
	float m_age;
	bool m_touching_ground;
	int m_hp;
	bool m_walk_around;
	float m_walk_around_timer;
	bool m_next_pos_exists;
	v3s16 m_next_pos_i;
	float m_shoot_reload_timer;
	bool m_shooting;
	float m_shooting_timer;
	float m_die_age;
	v2f m_size;
	bool m_falling;
	float m_disturb_timer;
	std::string m_disturbing_player;
	float m_random_disturb_timer;
	float m_shoot_y;

	Settings *m_properties;
};

#endif /* CONTENT_SAO_MOBV2_H_ */
