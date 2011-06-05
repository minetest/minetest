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
	
	//void setDayNightRatio(u32 r);
	u32 getDayNightRatio();
	
	// 0-23999
	virtual void setTimeOfDay(u32 time)
	{
		m_time_of_day = time;
	}

	u32 getTimeOfDay()
	{
		return m_time_of_day;
	}

protected:
	// peer_ids in here should be unique, except that there may be many 0s
	core::list<Player*> m_players;
	// Brightness
	//u32 m_daynight_ratio;
	// Time of day in milli-hours (0-23999); determines day and night
	u32 m_time_of_day;
};

/*
	List of active blocks, used by ServerEnvironment
*/

class ActiveBlockList
{
public:
	void update(core::list<v3s16> &active_positions,
			s16 radius,
			core::map<v3s16, bool> &blocks_removed,
			core::map<v3s16, bool> &blocks_added);

	bool contains(v3s16 p){
		return (m_list.find(p) != NULL);
	}

	void clear(){
		m_list.clear();
	}

	core::map<v3s16, bool> m_list;

private:
};

/*
	The server-side environment.

	This is not thread-safe. Server uses an environment mutex.
*/

#include "serverobject.h"

class Server;
class ActiveBlockModifier;

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
	
	/*
		Save players
	*/
	void serializePlayers(const std::string &savedir);
	void deSerializePlayers(const std::string &savedir);

	/*
		Save and load time of day and game timer
	*/
	void saveMeta(const std::string &savedir);
	void loadMeta(const std::string &savedir);

	/*
		External ActiveObject interface
		-------------------------------------------
	*/

	ServerActiveObject* getActiveObject(u16 id);

	/*
		Add an active object to the environment.
		Environment handles deletion of object.
		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObject(ServerActiveObject *object);
	
	/*
		Find out what new objects have been added to
		inside a radius around a position
	*/
	void getAddedActiveObjects(v3s16 pos, s16 radius,
			core::map<u16, bool> &current_objects,
			core::map<u16, bool> &added_objects);

	/*
		Find out what new objects have been removed from
		inside a radius around a position
	*/
	void getRemovedActiveObjects(v3s16 pos, s16 radius,
			core::map<u16, bool> &current_objects,
			core::map<u16, bool> &removed_objects);
	
	/*
		Get the next message emitted by some active object.
		Returns a message with id=0 if no messages are available.
	*/
	ActiveObjectMessage getActiveObjectMessage();

	/*
		ActiveBlockModifiers (TODO)
		-------------------------------------------
	*/

	void addActiveBlockModifier(ActiveBlockModifier *abm);

private:

	/*
		Internal ActiveObject interface
		-------------------------------------------
	*/

	/*
		Add an active object to the environment.

		Called by addActiveObject.

		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObjectRaw(ServerActiveObject *object, bool set_changed);
	
	/*
		Remove all objects that satisfy (m_removed && m_known_by_count==0)
	*/
	void removeRemovedObjects();
	
	/*
		Convert stored objects from block to active
	*/
	void activateObjects(MapBlock *block);
	
	/*
		Convert objects that are not in active blocks to static.

		If m_known_by_count != 0, active object is not deleted, but static
		data is still updated.

		If force_delete is set, active object is deleted nevertheless. It
		shall only be set so in the destructor of the environment.
	*/
	void deactivateFarObjects(bool force_delete);

	/*
		Member variables
	*/
	
	// The map
	ServerMap *m_map;
	// Pointer to server (which is handling this environment)
	Server *m_server;
	// Active object list
	core::map<u16, ServerActiveObject*> m_active_objects;
	// Outgoing network message buffer for active objects
	Queue<ActiveObjectMessage> m_active_object_messages;
	// Some timers
	float m_random_spawn_timer; // used for experimental code
	float m_send_recommended_timer;
	IntervalLimiter m_object_management_interval;
	// List of active blocks
	ActiveBlockList m_active_blocks;
	IntervalLimiter m_active_blocks_management_interval;
	IntervalLimiter m_active_blocks_test_interval;
	IntervalLimiter m_active_blocks_nodemetadata_interval;
	// Time from the beginning of the game in seconds.
	// Incremented in step().
	u32 m_game_time;
	// A helper variable for incrementing the latter
	float m_game_time_fraction_counter;
};

/*
	Active block modifier interface.

	These are fed into ServerEnvironment at initialization time;
	ServerEnvironment handles deleting them.
*/

class ActiveBlockModifier
{
public:
	ActiveBlockModifier(){};
	virtual ~ActiveBlockModifier(){};

	//virtual core::list<u8> update(ServerEnvironment *env) = 0;
	virtual u32 getTriggerContentCount(){ return 1;}
	virtual u8 getTriggerContent(u32 i) = 0;
	virtual float getActiveInterval() = 0;
	// chance of (1 / return value), 0 is disallowed
	virtual u32 getActiveChance() = 0;
	// This is called usually at interval for 1/chance of the nodes
	virtual void triggerEvent(ServerEnvironment *env, v3s16 p) = 0;
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

	void setTimeOfDay(u32 time)
	{
		u32 old_dr = getDayNightRatio();

		Environment::setTimeOfDay(time);

		if(getDayNightRatio() != old_dr)
		{
			dout_client<<DTIME<<"ClientEnvironment: DayNightRatio changed"
					<<" -> expiring meshes"<<std::endl;
			expireMeshes(true);
		}
	}

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

