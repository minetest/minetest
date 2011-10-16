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
	TestSAO(ServerEnvironment *env, u16 id, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_TEST;}
	static ServerActiveObject* create(ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
private:
	float m_timer1;
	float m_age;
};

class ItemSAO : public ServerActiveObject
{
public:
	ItemSAO(ServerEnvironment *env, u16 id, v3f pos,
			const std::string inventorystring);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_ITEM;}
	static ServerActiveObject* create(ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	InventoryItem* createInventoryItem();
	InventoryItem* createPickedUpItem(){return createInventoryItem();}
	void rightClick(Player *player);
private:
	std::string m_inventorystring;
	v3f m_speed_f;
	v3f m_last_sent_position;
	IntervalLimiter m_move_interval;
};

class RatSAO : public ServerActiveObject
{
public:
	RatSAO(ServerEnvironment *env, u16 id, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_RAT;}
	static ServerActiveObject* create(ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	InventoryItem* createPickedUpItem();
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
	Oerkki1SAO(ServerEnvironment *env, u16 id, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_OERKKI1;}
	static ServerActiveObject* create(ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	InventoryItem* createPickedUpItem(){return NULL;}
	u16 punch(const std::string &toolname, v3f dir);
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
	FireflySAO(ServerEnvironment *env, u16 id, v3f pos);
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_FIREFLY;}
	static ServerActiveObject* create(ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	void step(float dtime, bool send_recommended);
	std::string getClientInitializationData();
	std::string getStaticData();
	InventoryItem* createPickedUpItem();
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
	MobV2SAO(ServerEnvironment *env, u16 id, v3f pos,
			Settings *init_properties);
	virtual ~MobV2SAO();
	u8 getType() const
		{return ACTIVEOBJECT_TYPE_MOBV2;}
	static ServerActiveObject* create(ServerEnvironment *env, u16 id, v3f pos,
			const std::string &data);
	std::string getStaticData();
	std::string getClientInitializationData();
	void step(float dtime, bool send_recommended);
	InventoryItem* createPickedUpItem(){return NULL;}
	u16 punch(const std::string &toolname, v3f dir,
			const std::string &playername);
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

#endif

