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

class TestSAO : public ServerActiveObject
{
public:
	TestSAO(ServerEnvironment *env, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_TEST;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
private:
	float m_timer1;
	float m_age;
};

class ItemSAO : public ServerActiveObject
{
public:
	ItemSAO(ServerEnvironment *env, v3f pos, const std::string itemstring);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_ITEM;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	ItemStack createItemStack();
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
	float getMinimumSavedMovement(){ return 0.1*BS; }
private:
	std::string m_itemstring;
	bool m_itemstring_changed;
	v3f m_speed_f;
	v3f m_last_sent_position;
	IntervalLimiter m_move_interval;
};

class RatSAO : public ServerActiveObject
{
public:
	RatSAO(ServerEnvironment *env, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_RAT;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	void punch(ServerActiveObject *puncher, float time_from_last_punch);
private:
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
};

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

class FireflySAO : public ServerActiveObject
{
public:
	FireflySAO(ServerEnvironment *env, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_FIREFLY;}
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
private:
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
};

class Settings;

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

#endif

