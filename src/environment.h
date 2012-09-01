/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
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
	- The current time in the game
	- etc.
*/

#include <set>
#include "irrlichttypes_extrabloated.h"
#include "player.h"
#include <ostream>
#include "activeobject.h"
#include "util/container.h"
#include "util/numeric.h"
#include "mapnode.h"
#include "mapblock.h"

class Server;
class ServerEnvironment;
class ActiveBlockModifier;
class ServerActiveObject;
typedef struct lua_State lua_State;
class ITextureSource;
class IGameDef;
class Map;
class ServerMap;
class ClientMap;

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
	
	u32 getDayNightRatio();
	
	// 0-23999
	virtual void setTimeOfDay(u32 time)
	{
		m_time_of_day = time;
		m_time_of_day_f = (float)time / 24000.0;
	}

	u32 getTimeOfDay()
	{ return m_time_of_day; }

	float getTimeOfDayF()
	{ return m_time_of_day_f; }

	void stepTimeOfDay(float dtime);

	void setTimeOfDaySpeed(float speed)
	{ m_time_of_day_speed = speed; }
	
	float getTimeOfDaySpeed()
	{ return m_time_of_day_speed; }

protected:
	// peer_ids in here should be unique, except that there may be many 0s
	core::list<Player*> m_players;
	// Time of day in milli-hours (0-23999); determines day and night
	u32 m_time_of_day;
	// Time of day in 0...1
	float m_time_of_day_f;
	float m_time_of_day_speed;
	// Used to buffer dtime for adding to m_time_of_day
	float m_time_counter;
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
	
	// Set of contents to trigger on
	virtual std::set<std::string> getTriggerContents()=0;
	// Set of required neighbors (trigger doesn't happen if none are found)
	// Empty = do not check neighbors
	virtual std::set<std::string> getRequiredNeighbors()
	{ return std::set<std::string>(); }
	// Trigger interval in seconds
	virtual float getTriggerInterval() = 0;
	// Random chance of (1 / return value), 0 is disallowed
	virtual u32 getTriggerChance() = 0;
	// This is called usually at interval for 1/chance of the nodes
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n){};
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider){};
};

struct ABMWithState
{
	ActiveBlockModifier *abm;
	float timer;

	ABMWithState(ActiveBlockModifier *abm_);
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

class IBackgroundBlockEmerger
{
public:
	virtual void queueBlockEmerge(v3s16 blockpos, bool allow_generate)=0;
};

/*
	The server-side environment.

	This is not thread-safe. Server uses an environment mutex.
*/

class ServerEnvironment : public Environment
{
public:
	ServerEnvironment(ServerMap *map, lua_State *L, IGameDef *gamedef,
			IBackgroundBlockEmerger *emerger);
	~ServerEnvironment();

	Map & getMap();

	ServerMap & getServerMap();

	lua_State* getLua()
		{ return m_lua; }

	IGameDef *getGameDef()
		{ return m_gamedef; }

	float getSendRecommendedInterval()
	{
		return 0.10;
	}

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
		Add an active object as a static object to the corresponding
		MapBlock.
		Caller allocates memory, ServerEnvironment frees memory.
		Return value: true if succeeded, false if failed.
	*/
	bool addActiveObjectAsStatic(ServerActiveObject *object);
	
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
		Activate objects and dynamically modify for the dtime determined
		from timestamp and additional_dtime
	*/
	void activateBlock(MapBlock *block, u32 additional_dtime=0);

	/*
		ActiveBlockModifiers
		-------------------------------------------
	*/

	void addActiveBlockModifier(ActiveBlockModifier *abm);

	/*
		Other stuff
		-------------------------------------------
	*/
	
	// Find all active objects inside a radius around a point
	std::set<u16> getObjectsInsideRadius(v3f pos, float radius);
	
	// Clear all objects, loading and going through every MapBlock
	void clearAllObjects();
	
	// This makes stuff happen
	void step(f32 dtime);
	
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
	// Lua state
	lua_State *m_lua;
	// Game definition
	IGameDef *m_gamedef;
	// Background block emerger (the server, in practice)
	IBackgroundBlockEmerger *m_emerger;
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
	IntervalLimiter m_active_block_modifier_interval;
	IntervalLimiter m_active_blocks_nodemetadata_interval;
	int m_active_block_interval_overload_skip;
	// Time from the beginning of the game in seconds.
	// Incremented in step().
	u32 m_game_time;
	// A helper variable for incrementing the latter
	float m_game_time_fraction_counter;
	core::list<ABMWithState> m_abms;
};

#ifndef SERVER

#include "clientobject.h"
class ClientSimpleObject;

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
			bool send_to_server;
		} player_damage;
	};
};

class ClientEnvironment : public Environment
{
public:
	ClientEnvironment(ClientMap *map, scene::ISceneManager *smgr,
			ITextureSource *texturesource, IGameDef *gamedef,
			IrrlichtDevice *device);
	~ClientEnvironment();

	Map & getMap();
	ClientMap & getClientMap();

	IGameDef *getGameDef()
	{ return m_gamedef; }

	void step(f32 dtime);

	virtual void addPlayer(Player *player);
	LocalPlayer * getLocalPlayer();
	
	/*
		ClientSimpleObjects
	*/

	void addSimpleObject(ClientSimpleObject *simple);

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

	void damageLocalPlayer(u8 damage, bool handle_hp=true);

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
	ITextureSource *m_texturesource;
	IGameDef *m_gamedef;
	IrrlichtDevice *m_irr;
	core::map<u16, ClientActiveObject*> m_active_objects;
	core::list<ClientSimpleObject*> m_simple_objects;
	Queue<ClientEnvEvent> m_client_event_queue;
	IntervalLimiter m_active_object_light_update_interval;
	IntervalLimiter m_lava_hurt_interval;
};

#endif

#endif

