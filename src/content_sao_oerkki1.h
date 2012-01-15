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

#ifndef CONTENT_SAO_OERKKI1_H_
#define CONTENT_SAO_OERKKI1_H_

#include "content_sao.h"

class Oerkki1SAO : public ServerActiveObject
{
public:
	Oerkki1SAO(ServerEnvironment *env, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_OERKKI1;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
	bool isPeaceful(){return false;}
private:
	void doDamage(u16 d);

	bool m_is_active;
	IntervalLimiter m_inactive_interval;
	v3f m_speed_f;
	v3f m_oldpos;
	v3f m_last_sent_position;
	float m_yaw;
	float m_counter1;
	float m_counter2;
	float m_age;
	bool m_touching_ground;
	u8 m_hp;
	float m_after_jump_timer;
};

#endif /* CONTENT_SAO_OERKKI1_H_ */
