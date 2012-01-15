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

#ifndef CONTENT_SAO_LUA_H_
#define CONTENT_SAO_LUA_H_

#include "content_sao.h"
#include "serverlinkableobject.h"
#include "scriptapi.h"
#include "luaentity_common.h"

class LuaEntitySAO : public ServerActiveObject, public ServerLinkableObject
{
public:
	LuaEntitySAO(ServerEnvironment *env, v3f pos,
			const std::string &name, const std::string &state);
	~LuaEntitySAO();
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_LUAENTITY;}
	virtual void addedToEnvironment();
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
	void rightClick(ServerActiveObject *clicker);
	void setPos(v3f pos);
	void moveTo(v3f pos, bool continuous);
	float getMinimumSavedMovement();
	/* LuaEntitySAO-specific */
	void setVelocity(v3f velocity);
	v3f getVelocity();
	void setAcceleration(v3f acceleration);
	v3f getAcceleration();
	void setYaw(float yaw);
	float getYaw();
	void setTextureMod(const std::string &mod);
	void setSprite(v2s16 p, int num_frames, float framelength,
			bool select_horiz_by_yawpitch);
	std::string getName();
	bool sendLinkMsg(ServerActiveObject* parent,v3f offset);
	bool sendUnlinkMsg();

private:
	void sendPosition(bool do_interpolate, bool is_movement_end);

	std::string m_init_name;
	std::string m_init_state;
	bool m_registered;
	struct LuaEntityProperties *m_prop;

	v3f m_velocity;
	v3f m_acceleration;
	float m_yaw;
	float m_last_sent_yaw;
	v3f m_last_sent_position;
	v3f m_last_sent_velocity;
	float m_last_sent_position_timer;
	float m_last_sent_move_precision;
};


#endif /* CONTENT_SAO_LUA_H_ */
