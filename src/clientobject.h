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

#ifndef CLIENTOBJECT_HEADER
#define CLIENTOBJECT_HEADER

#include "common_irrlicht.h"
#include "activeobject.h"

/*

Some planning
-------------

* Client receives a network packet with information of added objects
  in it
* Client supplies the information to its ClientEnvironment
* The environment adds the specified objects to itself

*/

/*
	SmoothTranslator
*/

struct SmoothTranslator
{
	v3f vect_old;
	f32 anim_counter;
	f32 anim_time;
	f32 anim_time_counter;
	v3f vect_show;
	v3f vect_aim;

	SmoothTranslator():
		vect_old(0,0,0),
		anim_counter(0),
		anim_time(0),
		anim_time_counter(0),
		vect_show(0,0,0),
		vect_aim(0,0,0)
	{}

	void init(v3f vect)
	{
		vect_old = vect;
		vect_show = vect;
		vect_aim = vect;
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
		moveratio = moveratio * 0.8;
		if(moveratio > 1.5)
			moveratio = 1.5;
		vect_show = vect_old + vect_move * moveratio;
	}
};

class ClientEnvironment;

class ClientActiveObject : public ActiveObject
{
public:
	ClientActiveObject(u16 id);
	virtual ~ClientActiveObject();

	virtual void addToScene(scene::ISceneManager *smgr){}
	virtual void removeFromScene(){}
	// 0 <= light_at_pos <= LIGHT_SUN
	virtual void updateLight(u8 light_at_pos){}
	virtual v3s16 getLightPosition(){return v3s16(0,0,0);}
	virtual core::aabbox3d<f32>* getSelectionBox(){return NULL;}
	virtual core::aabbox3d<f32>* getCollisionBox(){return NULL;}
	virtual v3f getPosition(){return v3f(0,0,0);}
	
	// Step object in time
	virtual void step(float dtime, ClientEnvironment *env){}
	
	// Process a message sent by the server side object
	virtual void processMessage(const std::string &data){}

	virtual std::string infoText() {return "";}

	/*
		This takes the return value of
		ServerActiveObject::getClientInitializationData
	*/
	virtual void initialize(const std::string &data){}
	
	// Create a certain type of ClientActiveObject
	static ClientActiveObject* create(u8 type);

protected:
	// Used for creating objects based on type
	typedef ClientActiveObject* (*Factory)();
	static void registerType(u16 type, Factory f);
private:
	// Used for creating objects based on type
	static core::map<u16, Factory> m_types;
};

struct DistanceSortedActiveObject
{
	ClientActiveObject *obj;
	f32 d;

	DistanceSortedActiveObject(ClientActiveObject *a_obj, f32 a_d)
	{
		obj = a_obj;
		d = a_d;
	}

	bool operator < (DistanceSortedActiveObject &other)
	{
		return d < other.d;
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
		{return m_position;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
};

#endif

