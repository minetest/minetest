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
	
	// Step object in time
	virtual void step(float dtime){}
	
	// Process a message sent by the server side object
	virtual void processMessage(const std::string &data){}

	/*
		This takes the return value of getClientInitializationData
		TODO: Usage of this
	*/
	virtual void initialize(const std::string &data){}
	
	// Create a certain type of ClientActiveObject
	static ClientActiveObject* create(u8 type);

protected:
};

class TestCAO : public ClientActiveObject
{
public:
	TestCAO(u16 id);
	virtual ~TestCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_TEST;
	}
	
	void addToScene(scene::ISceneManager *smgr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime);

	void processMessage(const std::string &data);

private:
	scene::IMeshSceneNode *m_node;
	v3f m_position;
};

#endif

