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

#ifndef CONTENT_SAO_HEADER
#define CONTENT_SAO_HEADER

#include "serverobject.h"
#include "content_object.h"
#include "itemgroup.h"

ServerActiveObject* createItemSAO(ServerEnvironment *env, v3f pos,
		const std::string itemstring);

/*
	LuaEntitySAO
	
	This is the only SAO that needs to have a bunch of it's internals exposed.
*/

struct LuaEntityProperties;

class LuaEntitySAO : public ServerActiveObject
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
	int punch(v3f dir,
			const ToolCapabilities *toolcap=NULL,
			ServerActiveObject *puncher=NULL,
			float time_from_last_punch=1000000);
	void rightClick(ServerActiveObject *clicker);
	void setPos(v3f pos);
	void moveTo(v3f pos, bool continuous);
	float getMinimumSavedMovement();
	std::string getDescription();
	void setHP(s16 hp);
	s16 getHP();
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
	void setArmorGroups(const ItemGroupList &armor_groups);
private:
	void sendPosition(bool do_interpolate, bool is_movement_end);

	std::string m_init_name;
	std::string m_init_state;
	bool m_registered;
	struct LuaEntityProperties *m_prop;
	
	s16 m_hp;
	v3f m_velocity;
	v3f m_acceleration;
	float m_yaw;
	ItemGroupList m_armor_groups;

	float m_last_sent_yaw;
	v3f m_last_sent_position;
	v3f m_last_sent_velocity;
	float m_last_sent_position_timer;
	float m_last_sent_move_precision;
	bool m_armor_groups_sent;
};

#endif

