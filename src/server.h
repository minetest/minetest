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
#include "environment.h"
#include "irrlichttypes_bloated.h"
#include <string>
#include "porting.h"
#include "map.h"
#include "inventory.h"
#include "ban.h"
#include "hud.h"
#include "gamedef.h"
#include "serialization.h" // For SER_FMT_VER_INVALID
#include "mods.h"
#include "inventorymanager.h"
#include "subgame.h"
#include "sound.h"
#include "util/thread.h"
#include "util/string.h"
#include "rollback_interface.h" // Needed for rollbackRevertActions()
#include <list> // Needed for rollbackRevertActions()
#include <algorithm>

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

class IWritableItemDefManager;
class IWritableNodeDefManager;
class IWritableCraftDefManager;
class EventManager;
class PlayerSAO;
class IRollbackManager;
class EmergeManager;
//struct HudElement; ?????????
class ScriptApi;


class ServerError : public std::exception
{
public:
	ServerError(const std::string &s)
	{
		m_s = "ServerError: ";
		m_s += s;
	}
	virtual ~ServerError() throw()
	{}
	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
	std::string m_s;
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

class Server;

class ServerThread : public SimpleThread
{
	Server *m_server;

public:

	ServerThread(Server *server):
		SimpleThread(),
		m_server(server)
	{
	}

	void * Thread();
};

struct PlayerInfo
{
	u16 id;
	char name[PLAYERNAME_SIZE];
	v3f position;
	Address address;
	float avg_rtt;

	PlayerInfo();
	void PrintLine(std::ostream *s);
};

/*
	Used for queueing and sorting block transfers in containers

	Lower priority number means higher priority.
*/
struct PrioritySortedBlockTransfer
{
	PrioritySortedBlockTransfer(float a_priority, v3s16 a_pos, u16 a_peer_id)
	{
		priority = a_priority;
		pos = a_pos;
		peer_id = a_peer_id;
	}
	bool operator < (const PrioritySortedBlockTransfer &other) const
	{
		return priority < other.priority;
	}
	float priority;
	v3s16 pos;
	u16 peer_id;
};

struct MediaRequest
{
	std::string name;

	MediaRequest(const std::string &name_=""):
		name(name_)
	{}
};

struct MediaInfo
{
	std::string path;
	std::string sha1_digest;

	MediaInfo(const std::string path_="",
			const std::string sha1_digest_=""):
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

class RemoteClient
{
public:
	// peer_id=0 means this client has no associated peer
	// NOTE: If client is made allowed to exist while peer doesn't,
	//       this has to be set to 0 when there is no peer.
	//       Also, the client must be moved to some other container.
	u16 peer_id;
	// The serialization version to use with the client
	u8 serialization_version;
	//
	u16 net_proto_version;
	// Version is stored in here after INIT before INIT2
	u8 pending_serialization_version;

	bool definitions_sent;

	RemoteClient():
		m_time_from_building(9999),
		m_excess_gotblocks(0)
	{
		peer_id = 0;
		serialization_version = SER_FMT_VER_INVALID;
		net_proto_version = 0;
		pending_serialization_version = SER_FMT_VER_INVALID;
		definitions_sent = false;
		m_nearest_unsent_d = 0;
		m_nearest_unsent_reset_timer = 0.0;
		m_nothing_to_send_counter = 0;
		m_nothing_to_send_pause_timer = 0;
	}
	~RemoteClient()
	{
	}

	/*
		Finds block that should be sent next to the client.
		Environment should be locked when this is called.
		dtime is used for resetting send radius at slow interval
	*/
	void GetNextBlocks(Server *server, float dtime,
			std::vector<PrioritySortedBlockTransfer> &dest);

	void GotBlock(v3s16 p);

	void SentBlock(v3s16 p);

	void SetBlockNotSent(v3s16 p);
	void SetBlocksNotSent(std::map<v3s16, MapBlock*> &blocks);

	s32 SendingCount()
	{
		return m_blocks_sending.size();
	}

	// Increments timeouts and removes timed-out blocks from list
	// NOTE: This doesn't fix the server-not-sending-block bug
	//       because it is related to emerging, not sending.
	//void RunSendingTimeouts(float dtime, float timeout);

	void PrintInfo(std::ostream &o)
	{
		o<<"RemoteClient "<<peer_id<<": "
				<<"m_blocks_sent.size()="<<m_blocks_sent.size()
				<<", m_blocks_sending.size()="<<m_blocks_sending.size()
				<<", m_nearest_unsent_d="<<m_nearest_unsent_d
				<<", m_excess_gotblocks="<<m_excess_gotblocks
				<<std::endl;
		m_excess_gotblocks = 0;
	}

	// Time from last placing or removing blocks
	float m_time_from_building;

	/*JMutex m_dig_mutex;
	float m_dig_time_remaining;
	// -1 = not digging
	s16 m_dig_tool_item;
	v3s16 m_dig_position;*/

	/*
		List of active objects that the client knows of.
		Value is dummy.
	*/
	std::set<u16> m_known_objects;

private:
	/*
		Blocks that have been sent to client.
		- These don't have to be sent again.
		- A block is cleared from here when client says it has
		  deleted it from it's memory

		Key is position, value is dummy.
		No MapBlock* is stored here because the blocks can get deleted.
	*/
	std::set<v3s16> m_blocks_sent;
	s16 m_nearest_unsent_d;
	v3s16 m_last_center;
	float m_nearest_unsent_reset_timer;

	/*
		Blocks that are currently on the line.
		This is used for throttling the sending of blocks.
		- The size of this list is limited to some value
		Block is added when it is sent with BLOCKDATA.
		Block is removed when GOTBLOCKS is received.
		Value is time from sending. (not used at the moment)
	*/
	std::map<v3s16, float> m_blocks_sending;

	/*
		Count of excess GotBlocks().
		There is an excess amount because the client sometimes
		gets a block so late that the server sends it again,
		and the client then sends two GOTBLOCKs.
		This is resetted by PrintInfo()
	*/
	u32 m_excess_gotblocks;

	// CPU usage optimization
	u32 m_nothing_to_send_counter;
	float m_nothing_to_send_pause_timer;
};

class Server : public con::PeerHandler, public MapEventReceiver,
		public InventoryManager, public IGameDef,
		public IBackgroundBlockEmerger
{
public:
	/*
		NOTE: Every public method should be thread-safe
	*/

	Server(
		const std::string &path_world,
		const std::string &path_config,
		const SubgameSpec &gamespec,
		bool simple_singleplayer_mode
	);
	~Server();
	void start(unsigned short port);
	void stop();
	// This is mainly a way to pass the time to the server.
	// Actual processing is done in an another thread.
	void step(float dtime);
	// This is run by ServerThread and does the actual processing
	void AsyncRunStep();
	void Receive();
	void ProcessData(u8 *data, u32 datasize, u16 peer_id);

	//std::list<PlayerInfo> getPlayerInfo();

	// Environment must be locked when called
	void setTimeOfDay(u32 time)
	{
		m_env->setTimeOfDay(time);
		m_time_of_day_send_timer = 0;
	}

	bool getShutdownRequested()
	{
		return m_shutdown_requested;
	}

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

	void requestShutdown(void)
	{
		m_shutdown_requested = true;
	}

	// Returns -1 if failed, sound handle on success
	// Envlock + conlock
	s32 playSound(const SimpleSoundSpec &spec, const ServerSoundParams &params);
	void stopSound(s32 handle);

	// Envlock + conlock
	std::set<std::string> getPlayerEffectivePrivs(const std::string &name);
	bool checkPriv(const std::string &name, const std::string &priv);
	void reportPrivsModified(const std::string &name=""); // ""=all
	void reportInventoryFormspecModified(const std::string &name);

	// Saves g_settings to configpath given at initialization
	void saveConfig();

	void setIpBanned(const std::string &ip, const std::string &name)
	{
		m_banmanager.add(ip, name);
		return;
	}

	void unsetIpBanned(const std::string &ip_or_name)
	{
		m_banmanager.remove(ip_or_name);
		return;
	}

	std::string getBanDescription(const std::string &ip_or_name)
	{
		return m_banmanager.getBanDescription(ip_or_name);
	}

	Address getPeerAddress(u16 peer_id)
	{
		return m_con.GetPeerAddress(peer_id);
	}

	// Envlock and conlock should be locked when calling this
	void notifyPlayer(const char *name, const std::wstring msg, const bool prepend);
	void notifyPlayers(const std::wstring msg);
	void spawnParticle(const char *playername,
		v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, std::string texture);

	void spawnParticleAll(v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, std::string texture);

	u32 addParticleSpawner(const char *playername,
		u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, std::string texture);

	u32 addParticleSpawnerAll(u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, std::string texture);

	void deleteParticleSpawner(const char *playername, u32 id);
	void deleteParticleSpawnerAll(u32 id);

	void queueBlockEmerge(v3s16 blockpos, bool allow_generate);

	// Creates or resets inventory
	Inventory* createDetachedInventory(const std::string &name);

	// Envlock and conlock should be locked when using scriptapi
	ScriptApi *getScriptIface(){ return m_script; }

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

	IWritableItemDefManager* getWritableItemDefManager();
	IWritableNodeDefManager* getWritableNodeDefManager();
	IWritableCraftDefManager* getWritableCraftDefManager();

	const ModSpec* getModSpec(const std::string &modname);
	void getModNames(std::list<std::string> &modlist);
	std::string getBuiltinLuaPath();

	std::string getWorldPath(){ return m_path_world; }

	bool isSingleplayer(){ return m_simple_singleplayer_mode; }

	void setAsyncFatalError(const std::string &error)
	{
		m_async_fatal_error.set(error);
	}

	bool showFormspec(const char *name, const std::string &formspec, const std::string &formname);
	
	u32 hudAdd(Player *player, HudElement *element);
	bool hudRemove(Player *player, u32 id);
	bool hudChange(Player *player, u32 id, HudElementStat stat, void *value);
	bool hudSetFlags(Player *player, u32 flags, u32 mask);
	
private:

	// con::PeerHandler implementation.
	// These queue stuff to be processed by handlePeerChanges().
	// As of now, these create and remove clients and players.
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);

	/*
		Static send methods
	*/

	static void SendMovement(con::Connection &con, u16 peer_id);
	static void SendHP(con::Connection &con, u16 peer_id, u8 hp);
	static void SendAccessDenied(con::Connection &con, u16 peer_id,
			const std::wstring &reason);
	static void SendDeathscreen(con::Connection &con, u16 peer_id,
			bool set_camera_point_target, v3f camera_point_target);
	static void SendItemDef(con::Connection &con, u16 peer_id,
			IItemDefManager *itemdef, u16 protocol_version);
	static void SendNodeDef(con::Connection &con, u16 peer_id,
			INodeDefManager *nodedef, u16 protocol_version);

	/*
		Non-static send methods.
		Conlock should be always used.
		Envlock usage is documented badly but it's easy to figure out
		which ones access the environment.
	*/

	// Envlock and conlock should be locked when calling these
	void SendInventory(u16 peer_id);
	void SendChatMessage(u16 peer_id, const std::wstring &message);
	void BroadcastChatMessage(const std::wstring &message);
	void SendPlayerHP(u16 peer_id);
	void SendMovePlayer(u16 peer_id);
	void SendPlayerPrivileges(u16 peer_id);
	void SendPlayerInventoryFormspec(u16 peer_id);
	void SendShowFormspecMessage(u16 peer_id, const std::string formspec, const std::string formname);
	void SendHUDAdd(u16 peer_id, u32 id, HudElement *form);
	void SendHUDRemove(u16 peer_id, u32 id);
	void SendHUDChange(u16 peer_id, u32 id, HudElementStat stat, void *value);
	void SendHUDSetFlags(u16 peer_id, u32 flags, u32 mask);
	
	/*
		Send a node removal/addition event to all clients except ignore_id.
		Additionally, if far_players!=NULL, players further away than
		far_d_nodes are ignored and their peer_ids are added to far_players
	*/
	// Envlock and conlock should be locked when calling these
	void sendRemoveNode(v3s16 p, u16 ignore_id=0,
			std::list<u16> *far_players=NULL, float far_d_nodes=100);
	void sendAddNode(v3s16 p, MapNode n, u16 ignore_id=0,
			std::list<u16> *far_players=NULL, float far_d_nodes=100);
	void setBlockNotSent(v3s16 p);

	// Environment and Connection must be locked when called
	void SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver);

	// Sends blocks to clients (locks env and con on its own)
	void SendBlocks(float dtime);

	void fillMediaCache();
	void sendMediaAnnouncement(u16 peer_id);
	void sendRequestedMedia(u16 peer_id,
			const std::list<MediaRequest> &tosend);

	void sendDetachedInventory(const std::string &name, u16 peer_id);
	void sendDetachedInventoryToAll(const std::string &name);
	void sendDetachedInventories(u16 peer_id);

	// Adds a ParticleSpawner on peer with peer_id
	void SendAddParticleSpawner(u16 peer_id, u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, std::string texture, u32 id);

	// Adds a ParticleSpawner on all peers
	void SendAddParticleSpawnerAll(u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, std::string texture, u32 id);

	// Deletes ParticleSpawner on a single client
	void SendDeleteParticleSpawner(u16 peer_id, u32 id);

	// Deletes ParticleSpawner on all clients
	void SendDeleteParticleSpawnerAll(u32 id);

	// Spawns particle on single client
	void SendSpawnParticle(u16 peer_id,
		v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, std::string texture);

	// Spawns particle on all clients
	void SendSpawnParticleAll(v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, std::string texture);

	/*
		Something random
	*/

	void DiePlayer(u16 peer_id);
	void RespawnPlayer(u16 peer_id);

	void UpdateCrafting(u16 peer_id);

	// When called, connection mutex should be locked
	RemoteClient* getClient(u16 peer_id);

	// When called, environment mutex should be locked
	std::string getPlayerName(u16 peer_id)
	{
		Player *player = m_env->getPlayer(peer_id);
		if(player == NULL)
			return "[id="+itos(peer_id)+"]";
		return player->getName();
	}

	// When called, environment mutex should be locked
	PlayerSAO* getPlayerSAO(u16 peer_id)
	{
		Player *player = m_env->getPlayer(peer_id);
		if(player == NULL)
			return NULL;
		return player->getPlayerSAO();
	}

	/*
		Get a player from memory or creates one.
		If player is already connected, return NULL
		Does not verify/modify auth info and password.

		Call with env and con locked.
	*/
	PlayerSAO *emergePlayer(const char *name, u16 peer_id);

	// Locks environment and connection by its own
	struct PeerChange;
	void handlePeerChange(PeerChange &c);
	void handlePeerChanges();

	/*
		Variables
	*/

	// World directory
	std::string m_path_world;
	// Path to user's configuration file ("" = no configuration file)
	std::string m_path_config;
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

	// NOTE: If connection and environment are both to be locked,
	// environment shall be locked first.

	// Environment
	ServerEnvironment *m_env;
	JMutex m_env_mutex;

	// Connection
	con::Connection m_con;
	JMutex m_con_mutex;
	// Connected clients (behind the con mutex)
	std::map<u16, RemoteClient*> m_clients;
	u16 m_clients_number; //for announcing masterserver

	// Bann checking
	BanManager m_banmanager;

	// Rollback manager (behind m_env_mutex)
	IRollbackManager *m_rollback;
	bool m_rollback_sink_enabled;
	bool m_enable_rollback_recording; // Updated once in a while

	// Emerge manager
	EmergeManager *m_emerge;

	// Scripting
	// Envlock and conlock should be locked when using Lua
	ScriptApi *m_script;

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

	// The server mainly operates in this thread
	ServerThread m_thread;

	/*
		Time related stuff
	*/

	// Timer for sending time of day over network
	float m_time_of_day_send_timer;
	// Uptime of server in seconds
	MutexedVariable<double> m_uptime;

	/*
		Peer change queue.
		Queues stuff from peerAdded() and deletingPeer() to
		handlePeerChanges()
	*/
	enum PeerChangeType
	{
		PEER_ADDED,
		PEER_REMOVED
	};
	struct PeerChange
	{
		PeerChangeType type;
		u16 peer_id;
		bool timeout;
	};
	Queue<PeerChange> m_peer_change_queue;

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

	friend class EmergeThread;
	friend class RemoteClient;

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

