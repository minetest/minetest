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

#ifndef ENVIRONMENT_HEADER
#define ENVIRONMENT_HEADER

/*
	This class is the game's environment.
	It contains:
	- The map
	- Players
	- Other objects
	- The current time in the game (actually it only contains the brightness)
	- etc.
*/

#include <list>
#include "common_irrlicht.h"
#include "player.h"
#include "map.h"
#include <ostream>
#include "utility.h"

class Environment
{
public:
	// Environment will delete the map passed to the constructor
	Environment();
	virtual ~Environment();

	/*
		Step everything in environment.
		- Move players
		- Step mobs
		- Run timers of map
	*/
	virtual void step(f32 dtime) = 0;

	virtual Map & getMap() = 0;

	virtual void addPlayer(Player *player);
	void removePlayer(u16 peer_id);
	Player * getPlayer(u16 peer_id);
	Player * getPlayer(const char *name);
	Player * getRandomConnectedPlayer();
	Player * getNearestConnectedPlayer(v3f pos);
	core::list<Player*> getPlayers();
	core::list<Player*> getPlayers(bool ignore_disconnected);
	void printPlayers(std::ostream &o);
	
	void setDayNightRatio(u32 r);
	u32 getDayNightRatio();

protected:
	// peer_ids in here should be unique, except that there may be many 0s
	core::list<Player*> m_players;
	// Brightness
	u32 m_daynight_ratio;
};

/*
	The server-side environment.

	This is not thread-safe. Server uses an environment mutex.
*/

#include "serverobject.h"

class Server;

class ServerEnvironment : public Environment
{
public:
	ServerEnvironment(ServerMap *map, Server *server);
	~ServerEnvironment();

	Map & getMap()
	{
		return *m_map;
	}

	ServerMap & getServerMap()
	{
		return *m_map;
	}

	Server * getServer()
	{
		return m_server;
	}

	void step(f32 dtime);

	void serializePlayers(const std::string &savedir);
	void deSerializePlayers(const std::string &savedir);

	/*
		ActiveObjects
	*/

	ServerActiveObject* getActiveObject(u16 id);

	/*
		Adds an active object to the environment.
		Environment handles deletion of object.
		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObject(ServerActiveObject *object);
	
	/*
		Finds out what new objects have been added to
		inside a radius around a position
	*/
	void getAddedActiveObjects(v3s16 pos, s16 radius,
			core::map<u16, bool> &current_objects,
			core::map<u16, bool> &added_objects);

	/*
		Finds out what new objects have been removed from
		inside a radius around a position
	*/
	void getRemovedActiveObjects(v3s16 pos, s16 radius,
			core::map<u16, bool> &current_objects,
			core::map<u16, bool> &removed_objects);
	
	/*
		Gets the next message emitted by some active object.
		Returns a message with id=0 if no messages are available.
	*/
	ActiveObjectMessage getActiveObjectMessage();
	
private:
	ServerMap *m_map;
	Server *m_server;
	core::map<u16, ServerActiveObject*> m_active_objects;
	Queue<ActiveObjectMessage> m_active_object_messages;
	float m_random_spawn_timer;
	float m_send_recommended_timer;
	IntervalLimiter m_object_management_interval;
};

#ifndef SERVER

#include "clientobject.h"

/*
	The client-side environment.

	This is not thread-safe.
	Must be called from main (irrlicht) thread (uses the SceneManager)
	Client uses an environment mutex.
*/

enum ClientEnvEventType
{
	CEE_NONE,
	CEE_PLAYER_DAMAGE
};

struct ClientEnvEvent
{
	ClientEnvEventType type;
	union {
		struct{
		} none;
		struct{
			u8 amount;
		} player_damage;
	};
};

class ClientEnvironment : public Environment
{
public:
	ClientEnvironment(ClientMap *map, scene::ISceneManager *smgr);
	~ClientEnvironment();

	Map & getMap()
	{
		return *m_map;
	}

	ClientMap & getClientMap()
	{
		return *m_map;
	}

	void step(f32 dtime);

	virtual void addPlayer(Player *player);
	LocalPlayer * getLocalPlayer();

	void updateMeshes(v3s16 blockpos);
	void expireMeshes(bool only_daynight_diffed);

	/*
		ActiveObjects
	*/
	
	ClientActiveObject* getActiveObject(u16 id);

	/*
		Adds an active object to the environment.
		Environment handles deletion of object.
		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObject(ClientActiveObject *object);

	void addActiveObject(u16 id, u8 type, const std::string &init_data);
	void removeActiveObject(u16 id);

	void processActiveObjectMessage(u16 id, const std::string &data);

	/*
		Callbacks for activeobjects
	*/

	void damageLocalPlayer(u8 damage);

	/*
		Client likes to call these
	*/
	
	// Get all nearby objects
	void getActiveObjects(v3f origin, f32 max_d,
			core::array<DistanceSortedActiveObject> &dest);
	
	// Get event from queue. CEE_NONE is returned if queue is empty.
	ClientEnvEvent getClientEvent();
	
private:
	ClientMap *m_map;
	scene::ISceneManager *m_smgr;
	core::map<u16, ClientActiveObject*> m_active_objects;
	Queue<ClientEnvEvent> m_client_event_queue;
};

#endif

#endif

