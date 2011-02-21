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

#ifndef SERVEROBJECT_HEADER
#define SERVEROBJECT_HEADER

#include "common_irrlicht.h"
#include "activeobject.h"
#include "utility.h"

/*

Some planning
-------------

* Server environment adds an active object, which gets the id 1
* The active object list is scanned for each client once in a while,
  and it finds out what objects have been added that are not known
  by the client yet. This scan is initiated by the server and the
  result ends up directly to the server.
* A network packet is created with the info and sent to the client.

*/

class ServerEnvironment;

class ServerActiveObject : public ActiveObject
{
public:
	ServerActiveObject(ServerEnvironment *env, u16 id, v3f pos=v3f(0,0,0));
	virtual ~ServerActiveObject();

	v3f getBasePosition()
	{
		return m_base_position;
	}
	
	void setBasePosition(v3f pos)
	{
		m_base_position = pos;
	}

	ServerEnvironment* getEnv()
	{
		return m_env;
	}
	
	/*
		Step object in time.
		Messages added to messages are sent to client over network.
	*/
	virtual void step(float dtime, Queue<ActiveObjectMessage> &messages){}
	
	/*
		The return value of this is passed to the client-side object
		when it is created
	*/
	virtual std::string getClientInitializationData(){return "";}
	
	/*
		The return value of this is passed to the server-side object
		when it is loaded from disk or from a static object
	*/
	virtual std::string getServerInitializationData(){return "";}
	
	/*
		This takes the return value of getServerInitializationData
	*/
	virtual void initialize(const std::string &data){}
	
	// Number of players which know about this object
	u16 m_known_by_count;
	/*
		Whether this object is to be removed when nobody knows about
		it anymore.
		Removal is delayed to preserve the id for the time during which
		it could be confused to some other object by some client.
	*/
	bool m_removed;
	
protected:
	ServerEnvironment *m_env;
	v3f m_base_position;
};

class TestSAO : public ServerActiveObject
{
public:
	TestSAO(ServerEnvironment *env, u16 id, v3f pos);
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_TEST;
	}
	void step(float dtime, Queue<ActiveObjectMessage> &messages);
private:
	float m_timer1;
	float m_age;
};

extern "C"{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class LuaSAO : public ServerActiveObject
{
public:
	LuaSAO(ServerEnvironment *env, u16 id, v3f pos);
	virtual ~LuaSAO();

	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_LUA;
	}

	virtual std::string getClientInitializationData();
	
	virtual std::string getServerInitializationData();
	
	void initialize(const std::string &data);
	
	void loadScripts(const std::string &script_name);

	void step(float dtime, Queue<ActiveObjectMessage> &messages);

	/*
		Stuff available for usage for the lua callbacks
	*/
	// This is moved onwards at the end of step()
	Queue<ActiveObjectMessage> m_message_queue;

private:
	lua_State* L;
	std::string m_script_name;
};

#endif

