/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "connection.h"
#include "irr_v3d.h"
#include "map.h"
#include "hud.h"
#include "gamedef.h"
#include "serialization.h" // For SER_FMT_VER_INVALID
#include "mods.h"
#include "inventorymanager.h"
#include "subgame.h"
#include "rollback_interface.h" // Needed for rollbackRevertActions()
#include "util/numeric.h"
#include "util/thread.h"
#include "environment.h"
#include "clientiface.h"
#include <string>
#include <list>
#include <map>
#include <vector>

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

class IWritableItemDefManager;
class IWritableNodeDefManager;
class IWritableCraftDefManager;
class BanManager;
class EventManager;
class Inventory;
class Player;
class PlayerSAO;
class IRollbackManager;
class EmergeManager;
class GameScripting;
class ServerEnvironment;
struct SimpleSoundSpec;
class ServerThread;

enum ClientDeletionReason {
	CDR_LEAVE,
	CDR_TIMEOUT,
	CDR_DENY
};

/*
	Some random functions
*/
v3f findSpawnPos(ServerMap &map);

class MapEditEventIgnorer
{
public:
	MapEditEventIgnorer(bool *flag):
		m_flag(flag)
	{
		if(*m_flag == false)
			*m_flag = true;
		else
			m_flag = NULL;
	}

	~MapEditEventIgnorer()
	{
		if(m_flag)
		{
			assert(*m_flag);
			*m_flag = false;
		}
	}

private:
	bool *m_flag;
};

class MapEditEventAreaIgnorer
{
public:
	MapEditEventAreaIgnorer(VoxelArea *ignorevariable, const VoxelArea &a):
		m_ignorevariable(ignorevariable)
	{
		if(m_ignorevariable->getVolume() == 0)
			*m_ignorevariable = a;
		else
			m_ignorevariable = NULL;
	}

	~MapEditEventAreaIgnorer()
	{
		if(m_ignorevariable)
		{
			assert(m_ignorevariable->getVolume() != 0);
			*m_ignorevariable = VoxelArea();
		}
	}

private:
	VoxelArea *m_ignorevariable;
};

struct MediaInfo
{
	std::string path;
	std::string sha1_digest;

	MediaInfo(const std::string &path_="",
	          const std::string &sha1_digest_=""):
		path(path_),
		sha1_digest(sha1_digest_)
	{
	}
};

struct ServerSoundParams
{
	float gain;
	std::string to_player;
	enum Type{
		SSP_LOCAL=0,
		SSP_POSITIONAL=1,
		SSP_OBJECT=2
	} type;
	v3f pos;
	u16 object;
	float max_hear_distance;
	bool loop;

	ServerSoundParams():
		gain(1.0),
		to_player(""),
		type(SSP_LOCAL),
		pos(0,0,0),
		object(0),
		max_hear_distance(32*BS),
		loop(false)
	{}

	v3f getPos(ServerEnvironment *env, bool *pos_exists) const;
};

struct ServerPlayingSound
{
	ServerSoundParams params;
	std::set<u16> clients; // peer ids
};

class Server : public con::PeerHandler, public MapEventReceiver,
		public InventoryManager, public IGameDef
{
public:
	/*
		NOTE: Every public method should be thread-safe
	*/

	Server(
		const std::string &path_world,
		const SubgameSpec &gamespec,
		bool simple_singleplayer_mode,
		bool ipv6
	);
	~Server();
	void start(Address bind_addr);
	void stop();
	// This is mainly a way to pass the time to the server.
	// Actual processing is done in an another thread.
	void step(float dtime);
	// This is run by ServerThread and does the actual processing
	void AsyncRunStep(bool initial_step=false);
	void Receive();
	PlayerSAO* StageTwoClientInit(u16 peer_id);
	void ProcessData(u8 *data, u32 datasize, u16 peer_id);

	// Environment must be locked when called
	void setTimeOfDay(u32 time);

	/*
		Shall be called with the environment locked.
		This is accessed by the map, which is inside the environment,
		so it shouldn't be a problem.
	*/
	void onMapEditEvent(MapEditEvent *event);

	/*
		Shall be called with the environment and the connection locked.
	*/
	Inventory* getInventory(const InventoryLocation &loc);
	void setInventoryModified(const InventoryLocation &loc);

	// Connection must be locked when called
	std::wstring getStatusString();

	// read shutdown state
	inline bool getShutdownRequested()
			{ return m_shutdown_requested; }

	// request server to shutdown
	inline void requestShutdown(void)
			{ m_shutdown_requested = true; }

	// Returns -1 if failed, sound handle on success
	// Envlock
	s32 playSound(const SimpleSoundSpec &spec, const ServerSoundParams &params);
	void stopSound(s32 handle);

	// Envlock
	std::set<std::string> getPlayerEffectivePrivs(const std::string &name);
	bool checkPriv(const std::string &name, const std::string &priv);
	void reportPrivsModified(const std::string &name=""); // ""=all
	void reportInventoryFormspecModified(const std::string &name);

	void setIpBanned(const std::string &ip, const std::string &name);
	void unsetIpBanned(const std::string &ip_or_name);
	std::string getBanDescription(const std::string &ip_or_name);

	void notifyPlayer(const char *name, const std::wstring &msg);
	void notifyPlayers(const std::wstring &msg);
	void spawnParticle(const char *playername,
		v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, bool vertical, std::string texture);

	void spawnParticleAll(v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, bool vertical, std::string texture);

	u32 addParticleSpawner(const char *playername,
		u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, bool vertical, std::string texture);

	u32 addParticleSpawnerAll(u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, bool vertical, std::string texture);

	void deleteParticleSpawner(const char *playername, u32 id);
	void deleteParticleSpawnerAll(u32 id);

	// Creates or resets inventory
	Inventory* createDetachedInventory(const std::string &name);

	// Envlock and conlock should be locked when using scriptapi
	GameScripting *getScriptIface(){ return m_script; }

	// Envlock should be locked when using the rollback manager
	IRollbackManager *getRollbackManager(){ return m_rollback; }

	//TODO: determine what (if anything) should be locked to access EmergeManager
	EmergeManager *getEmergeManager(){ return m_emerge; }

	// actions: time-reversed list
	// Return value: success/failure
	bool rollbackRevertActions(const std::list<RollbackAction> &actions,
			std::list<std::string> *log);

	// IGameDef interface
	// Under envlock
	virtual IItemDefManager* getItemDefManager();
	virtual INodeDefManager* getNodeDefManager();
	virtual ICraftDefManager* getCraftDefManager();
	virtual ITextureSource* getTextureSource();
	virtual IShaderSource* getShaderSource();
	virtual u16 allocateUnknownNodeId(const std::string &name);
	virtual ISoundManager* getSoundManager();
	virtual MtEventManager* getEventManager();
	virtual IRollbackReportSink* getRollbackReportSink();
	virtual scene::ISceneManager* getSceneManager();
	
	IWritableItemDefManager* getWritableItemDefManager();
	IWritableNodeDefManager* getWritableNodeDefManager();
	IWritableCraftDefManager* getWritableCraftDefManager();

	const ModSpec* getModSpec(const std::string &modname);
	void getModNames(std::list<std::string> &modlist);
	std::string getBuiltinLuaPath();
	inline std::string getWorldPath()
			{ return m_path_world; }

	inline bool isSingleplayer()
			{ return m_simple_singleplayer_mode; }

	inline void setAsyncFatalError(const std::string &error)
			{ m_async_fatal_error.set(error); }

	bool showFormspec(const char *name, const std::string &formspec, const std::string &formname);
	Map & getMap() { return m_env->getMap(); }
	ServerEnvironment & getEnv() { return *m_env; }
	
	u32 hudAdd(Player *player, HudElement *element);
	bool hudRemove(Player *player, u32 id);
	bool hudChange(Player *player, u32 id, HudElementStat stat, void *value);
	bool hudSetFlags(Player *player, u32 flags, u32 mask);
	bool hudSetHotbarItemcount(Player *player, s32 hotbar_itemcount);
	void hudSetHotbarImage(Player *player, std::string name);
	void hudSetHotbarSelectedImage(Player *player, std::string name);

	inline Address getPeerAddress(u16 peer_id)
			{ return m_con.GetPeerAddress(peer_id); }
			
	bool setLocalPlayerAnimations(Player *player, v2s32 animation_frames[4], f32 frame_speed);
	bool setPlayerEyeOffset(Player *player, v3f first, v3f third);

	bool setSky(Player *player, const video::SColor &bgcolor,
			const std::string &type, const std::vector<std::string> &params);
	
	bool overrideDayNightRatio(Player *player, bool do_override,
			float brightness);

	/* con::PeerHandler implementation. */
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);

	void DenyAccess(u16 peer_id, const std::wstring &reason);
	bool getClientConInfo(u16 peer_id, con::rtt_stat_type type,float* retval);
	bool getClientInfo(u16 peer_id,ClientState* state, u32* uptime,
			u8* ser_vers, u16* prot_vers, u8* major, u8* minor, u8* patch,
			std::string* vers_string);

private:

	friend class EmergeThread;
	friend class RemoteClient;

	void SendMovement(u16 peer_id);
	void SendHP(u16 peer_id, u8 hp);
	void SendBreath(u16 peer_id, u16 breath);
	void SendAccessDenied(u16 peer_id,const std::wstring &reason);
	void SendDeathscreen(u16 peer_id,bool set_camera_point_target, v3f camera_point_target);
	void SendItemDef(u16 peer_id,IItemDefManager *itemdef, u16 protocol_version);
	void SendNodeDef(u16 peer_id,INodeDefManager *nodedef, u16 protocol_version);

	/* mark blocks not sent for all clients */
	void SetBlocksNotSent(std::map<v3s16, MapBlock *>& block);

	// Envlock and conlock should be locked when calling these
	void SendInventory(u16 peer_id);
	void SendChatMessage(u16 peer_id, const std::wstring &message);
	void SendTimeOfDay(u16 peer_id, u16 time, f32 time_speed);
	void SendPlayerHP(u16 peer_id);
	void SendPlayerBreath(u16 peer_id);
	void SendMovePlayer(u16 peer_id);
	void SendLocalPlayerAnimations(u16 peer_id, v2s32 animation_frames[4], f32 animation_speed);
	void SendEyeOffset(u16 peer_id, v3f first, v3f third);
	void SendPlayerPrivileges(u16 peer_id);
	void SendPlayerInventoryFormspec(u16 peer_id);
	void SendShowFormspecMessage(u16 peer_id, const std::string &formspec, const std::string &formname);
	void SendHUDAdd(u16 peer_id, u32 id, HudElement *form);
	void SendHUDRemove(u16 peer_id, u32 id);
	void SendHUDChange(u16 peer_id, u32 id, HudElementStat stat, void *value);
	void SendHUDSetFlags(u16 peer_id, u32 flags, u32 mask);
	void SendHUDSetParam(u16 peer_id, u16 param, const std::string &value);
	void SendSetSky(u16 peer_id, const video::SColor &bgcolor,
			const std::string &type, const std::vector<std::string> &params);
	void SendOverrideDayNightRatio(u16 peer_id, bool do_override, float ratio);
	
	/*
		Send a node removal/addition event to all clients except ignore_id.
		Additionally, if far_players!=NULL, players further away than
		far_d_nodes are ignored and their peer_ids are added to far_players
	*/
	// Envlock and conlock should be locked when calling these
	void sendRemoveNode(v3s16 p, u16 ignore_id=0,
			std::list<u16> *far_players=NULL, float far_d_nodes=100);
	void sendAddNode(v3s16 p, MapNode n, u16 ignore_id=0,
			std::list<u16> *far_players=NULL, float far_d_nodes=100,
			bool remove_metadata=true);
	void setBlockNotSent(v3s16 p);

	// Environment and Connection must be locked when called
	void SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver, u16 net_proto_version);

	// Sends blocks to clients (locks env and con on its own)
	void SendBlocks(float dtime);

	void fillMediaCache();
	void sendMediaAnnouncement(u16 peer_id);
	void sendRequestedMedia(u16 peer_id,
			const std::list<std::string> &tosend);

	void sendDetachedInventory(const std::string &name, u16 peer_id);
	void sendDetachedInventories(u16 peer_id);

	// Adds a ParticleSpawner on peer with peer_id (PEER_ID_INEXISTENT == all)
	void SendAddParticleSpawner(u16 peer_id, u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, bool vertical, std::string texture, u32 id);

	void SendDeleteParticleSpawner(u16 peer_id, u32 id);

	// Spawns particle on peer with peer_id (PEER_ID_INEXISTENT == all)
	void SendSpawnParticle(u16 peer_id,
		v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, bool vertical, std::string texture);

	/*
		Something random
	*/

	void DiePlayer(u16 peer_id);
	void RespawnPlayer(u16 peer_id);
	void DeleteClient(u16 peer_id, ClientDeletionReason reason);
	void UpdateCrafting(u16 peer_id);

	// When called, connection mutex should be locked
	RemoteClient* getClient(u16 peer_id,ClientState state_min=CS_Active);
	RemoteClient* getClientNoEx(u16 peer_id,ClientState state_min=CS_Active);

	// When called, environment mutex should be locked
	std::string getPlayerName(u16 peer_id);
	PlayerSAO* getPlayerSAO(u16 peer_id);

	/*
		Get a player from memory or creates one.
		If player is already connected, return NULL
		Does not verify/modify auth info and password.

		Call with env and con locked.
	*/
	PlayerSAO *emergePlayer(const char *name, u16 peer_id);

	void handlePeerChanges();

	/*
		Variables
	*/

	// World directory
	std::string m_path_world;
	// Subgame specification
	SubgameSpec m_gamespec;
	// If true, do not allow multiple players and hide some multiplayer
	// functionality
	bool m_simple_singleplayer_mode;

	// Thread can set; step() will throw as ServerError
	MutexedVariable<std::string> m_async_fatal_error;

	// Some timers
	float m_liquid_transform_timer;
	float m_liquid_transform_every;
	float m_print_info_timer;
	float m_masterserver_timer;
	float m_objectdata_timer;
	float m_emergethread_trigger_timer;
	float m_savemap_timer;
	IntervalLimiter m_map_timer_and_unload_interval;

	// Environment
	ServerEnvironment *m_env;
	JMutex m_env_mutex;

	// server connection
	con::Connection m_con;

	// Ban checking
	BanManager *m_banmanager;

	// Rollback manager (behind m_env_mutex)
	IRollbackManager *m_rollback;
	bool m_rollback_sink_enabled;
	bool m_enable_rollback_recording; // Updated once in a while

	// Emerge manager
	EmergeManager *m_emerge;

	// Scripting
	// Envlock and conlock should be locked when using Lua
	GameScripting *m_script;

	// Item definition manager
	IWritableItemDefManager *m_itemdef;

	// Node definition manager
	IWritableNodeDefManager *m_nodedef;

	// Craft definition manager
	IWritableCraftDefManager *m_craftdef;

	// Event manager
	EventManager *m_event;

	// Mods
	std::vector<ModSpec> m_mods;

	/*
		Threads
	*/

	// A buffer for time steps
	// step() increments and AsyncRunStep() run by m_thread reads it.
	float m_step_dtime;
	JMutex m_step_dtime_mutex;

	// current server step lag counter
	float m_lag;

	// The server mainly operates in this thread
	ServerThread *m_thread;

	/*
		Time related stuff
	*/

	// Timer for sending time of day over network
	float m_time_of_day_send_timer;
	// Uptime of server in seconds
	MutexedVariable<double> m_uptime;

	/*
	 Client interface
	 */
	ClientInterface m_clients;

	/*
		Peer change queue.
		Queues stuff from peerAdded() and deletingPeer() to
		handlePeerChanges()
	*/
	Queue<con::PeerChange> m_peer_change_queue;

	/*
		Random stuff
	*/

	// Mod parent directory paths
	std::list<std::string> m_modspaths;

	bool m_shutdown_requested;

	/*
		Map edit event queue. Automatically receives all map edits.
		The constructor of this class registers us to receive them through
		onMapEditEvent

		NOTE: Should these be moved to actually be members of
		ServerEnvironment?
	*/

	/*
		Queue of map edits from the environment for sending to the clients
		This is behind m_env_mutex
	*/
	Queue<MapEditEvent*> m_unsent_map_edit_queue;
	/*
		Set to true when the server itself is modifying the map and does
		all sending of information by itself.
		This is behind m_env_mutex
	*/
	bool m_ignore_map_edit_events;
	/*
		If a non-empty area, map edit events contained within are left
		unsent. Done at map generation time to speed up editing of the
		generated area, as it will be sent anyway.
		This is behind m_env_mutex
	*/
	VoxelArea m_ignore_map_edit_events_area;
	/*
		If set to !=0, the incoming MapEditEvents are modified to have
		this peed id as the disabled recipient
		This is behind m_env_mutex
	*/
	u16 m_ignore_map_edit_events_peer_id;

	// media files known to server
	std::map<std::string,MediaInfo> m_media;

	/*
		Sounds
	*/
	std::map<s32, ServerPlayingSound> m_playing_sounds;
	s32 m_next_sound_id;

	/*
		Detached inventories (behind m_env_mutex)
	*/
	// key = name
	std::map<std::string, Inventory*> m_detached_inventories;

	/*
		Particles
	*/
	std::vector<u32> m_particlespawner_ids;
};

/*
	Runs a simple dedicated server loop.

	Shuts down when run is set to false.
*/
void dedicated_server_loop(Server &server, bool &run);

#endif

