/*
Minetest
Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SERVER_ENVIRONMENT_HEADER
#define SERVER_ENVIRONMENT_HEADER

#include "environment.h"
#include "mapnode.h"
#include "mapblock.h"
#include <set>

class IGameDef;
class ServerMap;
struct GameParams;
class RemotePlayer;
class PlayerDatabase;
class PlayerSAO;
class ServerEnvironment;
class ActiveBlockModifier;
class ServerActiveObject;
class Server;
class ServerScripting;

/*
	{Active, Loading} block modifier interface.

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
	// Whether to modify chance to simulate time lost by an unnattended block
	virtual bool getSimpleCatchUp() = 0;
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

struct LoadingBlockModifierDef
{
	// Set of contents to trigger on
	std::set<std::string> trigger_contents;
	std::string name;
	bool run_at_every_load;

	virtual ~LoadingBlockModifierDef() {}
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n){};
};

struct LBMContentMapping
{
	typedef std::map<content_t, std::vector<LoadingBlockModifierDef *> > container_map;
	container_map map;

	std::vector<LoadingBlockModifierDef *> lbm_list;

	// Needs to be separate method (not inside destructor),
	// because the LBMContentMapping may be copied and destructed
	// many times during operation in the lbm_lookup_map.
	void deleteContents();
	void addLBM(LoadingBlockModifierDef *lbm_def, IGameDef *gamedef);
	const std::vector<LoadingBlockModifierDef *> *lookup(content_t c) const;
};

class LBMManager
{
public:
	LBMManager():
		m_query_mode(false)
	{}

	~LBMManager();

	// Don't call this after loadIntroductionTimes() ran.
	void addLBMDef(LoadingBlockModifierDef *lbm_def);

	void loadIntroductionTimes(const std::string &times,
		IGameDef *gamedef, u32 now);

	// Don't call this before loadIntroductionTimes() ran.
	std::string createIntroductionTimesString();

	// Don't call this before loadIntroductionTimes() ran.
	void applyLBMs(ServerEnvironment *env, MapBlock *block, u32 stamp);

	// Warning: do not make this std::unordered_map, order is relevant here
	typedef std::map<u32, LBMContentMapping> lbm_lookup_map;

private:
	// Once we set this to true, we can only query,
	// not modify
	bool m_query_mode;

	// For m_query_mode == false:
	// The key of the map is the LBM def's name.
	// TODO make this std::unordered_map
	std::map<std::string, LoadingBlockModifierDef *> m_lbm_defs;

	// For m_query_mode == true:
	// The key of the map is the LBM def's first introduction time.
	lbm_lookup_map m_lbm_lookup;

	// Returns an iterator to the LBMs that were introduced
	// after the given time. This is guaranteed to return
	// valid values for everything
	lbm_lookup_map::const_iterator getLBMsIntroducedAfter(u32 time)
	{ return m_lbm_lookup.lower_bound(time); }
};

/*
	List of active blocks, used by ServerEnvironment
*/

class ActiveBlockList
{
public:
	void update(std::vector<v3s16> &active_positions,
		s16 radius,
		std::set<v3s16> &blocks_removed,
		std::set<v3s16> &blocks_added);

	bool contains(v3s16 p){
		return (m_list.find(p) != m_list.end());
	}

	void clear(){
		m_list.clear();
	}

	std::set<v3s16> m_list;
	std::set<v3s16> m_forceloaded_list;

private:
};

/*
	Operation mode for ServerEnvironment::clearObjects()
*/
enum ClearObjectsMode {
	// Load and go through every mapblock, clearing objects
		CLEAR_OBJECTS_MODE_FULL,

	// Clear objects immediately in loaded mapblocks;
	// clear objects in unloaded mapblocks only when the mapblocks are next activated.
		CLEAR_OBJECTS_MODE_QUICK,
};

/*
	The server-side environment.

	This is not thread-safe. Server uses an environment mutex.
*/

typedef UNORDERED_MAP<u16, ServerActiveObject *> ActiveObjectMap;

class ServerEnvironment : public Environment
{
public:
	ServerEnvironment(ServerMap *map, ServerScripting *scriptIface,
		Server *server, const std::string &path_world);
	~ServerEnvironment();

	Map & getMap();

	ServerMap & getServerMap();

	//TODO find way to remove this fct!
	ServerScripting* getScriptIface()
	{ return m_script; }

	Server *getGameDef()
	{ return m_server; }

	float getSendRecommendedInterval()
	{ return m_recommended_send_interval; }

	void kickAllPlayers(AccessDeniedCode reason,
		const std::string &str_reason, bool reconnect);
	// Save players
	void saveLoadedPlayers();
	void savePlayer(RemotePlayer *player);
	PlayerSAO *loadPlayer(RemotePlayer *player, bool *new_player, u16 peer_id,
		bool is_singleplayer);
	void addPlayer(RemotePlayer *player);
	void removePlayer(RemotePlayer *player);
	bool removePlayerFromDatabase(const std::string &name);

	/*
		Save and load time of day and game timer
	*/
	void saveMeta();
	void loadMeta();
	// to be called instead of loadMeta if
	// env_meta.txt doesn't exist (e.g. new world)
	void loadDefaultMeta();

	u32 addParticleSpawner(float exptime);
	u32 addParticleSpawner(float exptime, u16 attached_id);
	void deleteParticleSpawner(u32 id, bool remove_from_object = true);

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
		(note:  not used, pending removal from engine)
	*/
	//bool addActiveObjectAsStatic(ServerActiveObject *object);

	/*
		Find out what new objects have been added to
		inside a radius around a position
	*/
	void getAddedActiveObjects(PlayerSAO *playersao, s16 radius,
		s16 player_radius,
		std::set<u16> &current_objects,
		std::queue<u16> &added_objects);

	/*
		Find out what new objects have been removed from
		inside a radius around a position
	*/
	void getRemovedActiveObjects(PlayerSAO *playersao, s16 radius,
		s16 player_radius,
		std::set<u16> &current_objects,
		std::queue<u16> &removed_objects);

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
		{Active,Loading}BlockModifiers
		-------------------------------------------
	*/

	void addActiveBlockModifier(ActiveBlockModifier *abm);
	void addLoadingBlockModifierDef(LoadingBlockModifierDef *lbm);

	/*
		Other stuff
		-------------------------------------------
	*/

	// Script-aware node setters
	bool setNode(v3s16 p, const MapNode &n);
	bool removeNode(v3s16 p);
	bool swapNode(v3s16 p, const MapNode &n);

	// Find all active objects inside a radius around a point
	void getObjectsInsideRadius(std::vector<u16> &objects, v3f pos, float radius);

	// Clear objects, loading and going through every MapBlock
	void clearObjects(ClearObjectsMode mode);

	// This makes stuff happen
	void step(f32 dtime);

	//check if there's a line of sight between two positions
	bool line_of_sight(v3f pos1, v3f pos2, float stepsize=1.0, v3s16 *p=NULL);

	u32 getGameTime() const { return m_game_time; }

	void reportMaxLagEstimate(float f) { m_max_lag_estimate = f; }
	float getMaxLagEstimate() { return m_max_lag_estimate; }

	std::set<v3s16>* getForceloadedBlocks() { return &m_active_blocks.m_forceloaded_list; };

	// Sets the static object status all the active objects in the specified block
	// This is only really needed for deleting blocks from the map
	void setStaticForActiveObjectsInBlock(v3s16 blockpos,
		bool static_exists, v3s16 static_block=v3s16(0,0,0));

	RemotePlayer *getPlayer(const u16 peer_id);
	RemotePlayer *getPlayer(const char* name);

	static bool migratePlayersDatabase(const GameParams &game_params,
			const Settings &cmd_args);
private:

	static PlayerDatabase *openPlayerDatabase(const std::string &name,
			const std::string &savedir, const Settings &conf);
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
	u16 addActiveObjectRaw(ServerActiveObject *object, bool set_changed, u32 dtime_s);

	/*
		Remove all objects that satisfy (m_removed && m_known_by_count==0)
	*/
	void removeRemovedObjects();

	/*
		Convert stored objects from block to active
	*/
	void activateObjects(MapBlock *block, u32 dtime_s);

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
	ServerScripting* m_script;
	// Server definition
	Server *m_server;
	// World path
	const std::string m_path_world;
	// Active object list
	ActiveObjectMap m_active_objects;
	// Outgoing network message buffer for active objects
	std::queue<ActiveObjectMessage> m_active_object_messages;
	// Some timers
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
	// Time of last clearObjects call (game time).
	// When a mapblock older than this is loaded, its objects are cleared.
	u32 m_last_clear_objects_time;
	// Active block modifiers
	std::vector<ABMWithState> m_abms;
	LBMManager m_lbm_mgr;
	// An interval for generally sending object positions and stuff
	float m_recommended_send_interval;
	// Estimate for general maximum lag as determined by server.
	// Can raise to high values like 15s with eg. map generation mods.
	float m_max_lag_estimate;

	// peer_ids in here should be unique, except that there may be many 0s
	std::vector<RemotePlayer*> m_players;

	PlayerDatabase *m_player_database;

	// Particles
	IntervalLimiter m_particle_management_interval;
	UNORDERED_MAP<u32, float> m_particle_spawners;
	UNORDERED_MAP<u32, u16> m_particle_spawner_attachments;
};

#endif
