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

#ifndef CONTENT_CAO_HEADER
#define CONTENT_CAO_HEADER

#include "clientobject.h"
#include "content_object.h"
#include "utility.h" // For IntervalLimiter
class Settings;
#include "MyBillboardSceneNode.h"

/*
	SmoothTranslator
*/

struct SmoothTranslator
{
	v3f vect_old;
	v3f vect_show;
	v3f vect_aim;
	f32 anim_counter;
	f32 anim_time;
	f32 anim_time_counter;

	SmoothTranslator():
		vect_old(0,0,0),
		vect_show(0,0,0),
		vect_aim(0,0,0),
		anim_counter(0),
		anim_time(0),
		anim_time_counter(0)
	{}

	void init(v3f vect)
	{
		vect_old = vect;
		vect_show = vect;
		vect_aim = vect;
		anim_counter = 0;
		anim_time = 0;
		anim_time_counter = 0;
	}

	void sharpen()
	{
		init(vect_show);
	}

	void update(v3f vect_new)
	{
		vect_old = vect_show;
		vect_aim = vect_new;
		if(anim_time < 0.001 || anim_time > 1.0)
			anim_time = anim_time_counter;
		else
			anim_time = anim_time * 0.9 + anim_time_counter * 0.1;
		anim_time_counter = 0;
		anim_counter = 0;
	}

	void translate(f32 dtime)
	{
		anim_time_counter = anim_time_counter + dtime;
		anim_counter = anim_counter + dtime;
		v3f vect_move = vect_aim - vect_old;
		f32 moveratio = 1.0;
		if(anim_time > 0.001)
			moveratio = anim_time_counter / anim_time;
		// Move a bit less than should, to avoid oscillation
		moveratio = moveratio * 0.5;
		if(moveratio > 1.5)
			moveratio = 1.5;
		vect_show = vect_old + vect_move * moveratio;
	}

	bool is_moving()
	{
		return ((anim_time_counter / anim_time) < 1.4);
	}
};


/*
	TestCAO
*/

class TestCAO : public ClientActiveObject
{
public:
	TestCAO();
	virtual ~TestCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_TEST;
	}
	
	static ClientActiveObject* create();

	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

private:
	scene::IMeshSceneNode *m_node;
	v3f m_position;
};

/*
	ItemCAO
*/

class ItemCAO : public ClientActiveObject
{
public:
	ItemCAO();
	virtual ~ItemCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_ITEM;
	}
	
	static ClientActiveObject* create();

	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return m_position;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	std::string m_inventorystring;
};

/*
	RatCAO
*/

class RatCAO : public ClientActiveObject
{
public:
	RatCAO();
	virtual ~RatCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_RAT;
	}
	
	static ClientActiveObject* create();

	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return pos_translator.vect_show;}
		//{return m_position;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
};

/*
	Oerkki1CAO
*/

class Oerkki1CAO : public ClientActiveObject
{
public:
	Oerkki1CAO();
	virtual ~Oerkki1CAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_OERKKI1;
	}
	
	static ClientActiveObject* create();

	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return pos_translator.vect_show;}
		//{return m_position;}

	// If returns true, punch will not be sent to the server
	bool directReportPunch(const std::string &toolname, v3f dir);

private:
	IntervalLimiter m_attack_interval;
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
	float m_damage_visual_timer;
	bool m_damage_texture_enabled;
};

/*
	FireflyCAO
*/

class FireflyCAO : public ClientActiveObject
{
public:
	FireflyCAO();
	virtual ~FireflyCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_FIREFLY;
	}
	
	static ClientActiveObject* create();

	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return m_position;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
};

/*
	MobV2CAO
*/

class MobV2CAO : public ClientActiveObject
{
public:
	MobV2CAO();
	virtual ~MobV2CAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_MOBV2;
	}
	
	static ClientActiveObject* create();

	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return pos_translator.vect_show;}
		//{return m_position;}
	bool doShowSelectionBox(){return false;}

	// If returns true, punch will not be sent to the server
	bool directReportPunch(const std::string &toolname, v3f dir);

private:
	void setLooks(const std::string &looks);
	
	IntervalLimiter m_attack_interval;
	core::aabbox3d<f32> m_selection_box;
	scene::MyBillboardSceneNode *m_node;
	v3f m_position;
	std::string m_texture_name;
	float m_yaw;
	SmoothTranslator pos_translator;
	bool m_walking;
	float m_walking_unset_timer;
	float m_walk_timer;
	int m_walk_frame;
	float m_damage_visual_timer;
	u8 m_last_light;
	bool m_shooting;
	float m_shooting_unset_timer;
	v2f m_sprite_size;
	float m_sprite_y;
	bool m_bright_shooting;
	std::string m_sprite_type;
	int m_simple_anim_frames;
	float m_simple_anim_frametime;
	bool m_lock_full_brightness;
	int m_player_hit_damage;
	float m_player_hit_distance;
	float m_player_hit_interval;
	float m_player_hit_timer;

	Settings *m_properties;
};


#endif

