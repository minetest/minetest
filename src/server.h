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

#pragma once

#include "irr_v3d.h"
#include "map.h"
#include "hud.h"
#include "gamedef.h"
#include "serialization.h" // For SER_FMT_VER_INVALID
#include "content/mods.h"
#include "inventorymanager.h"
#include "content/subgames.h"
#include "tileanimation.h" // TileAnimationParams
#include "particles.h" // ParticleParams
#include "network/peerhandler.h"
#include "network/address.h"
#include "util/numeric.h"
#include "util/thread.h"
#include "util/basic_macros.h"
#include "util/metricsbackend.h"
#include "serverenvironment.h"
#include "clientiface.h"
#include "chatmessage.h"
#include "translation.h"
#include <string>
#include <list>
#include <map>
#include <vector>

class ChatEvent;
struct ChatEventChat;
struct ChatInterface;
class IWritableItemDefManager;
class NodeDefManager;
class IWritableCraftDefManager;
class BanManager;
class EventManager;
class Inventory;
class ModChannelMgr;
class RemotePlayer;
class PlayerSAO;
struct PlayerHPChangeReason;
class IRollbackManager;
struct RollbackAction;
class EmergeManager;
class ServerScripting;
class ServerEnvironment;
struct SimpleSoundSpec;
struct CloudParams;
struct SkyboxParams;
struct SunParams;
struct MoonParams;
struct StarParams;
class ServerThread;
class ServerModManager;
class ServerInventoryManager;

enum ClientDeletionReason {
	CDR_LEAVE,
	CDR_TIMEOUT,
	CDR_DENY
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
	enum Type {
		SSP_LOCAL,
		SSP_POSITIONAL,
		SSP_OBJECT
	} type = SSP_LOCAL;
	float gain = 1.0f;
	float fade = 0.0f;
	float pitch = 1.0f;
	bool loop = false;
	float max_hear_distance = 32 * BS;
	v3f pos;
	u16 object = 0;
	std::string to_player = "";
	std::string exclude_player = "";

	v3f getPos(ServerEnvironment *env, bool *pos_exists) const;
};

struct ServerPlayingSound
{
	ServerSoundParams params;
	SimpleSoundSpec spec;
	std::unordered_set<session_t> clients; // peer ids
};

class Server : public con::PeerHandler, public MapEventReceiver,
		public IGameDef
{
public:
	/*
		NOTE: Every public method should be thread-safe
	*/

	Server(
		const std::string &path_world,
		const SubgameSpec &gamespec,
		bool simple_singleplayer_mode,
		Address bind_addr,
		bool dedicated,
		ChatInterface *iface = nullptr,
		std::string *on_shutdown_errmsg = nullptr
	);
	~Server();
	DISABLE_CLASS_COPY(Server);

	void start();
	void stop();
	// This is mainly a way to pass the time to the server.
	// Actual processing is done in an another thread.
	void step(float dtime);
	// This is run by ServerThread and does the actual processing
	void AsyncRunStep(bool initial_step=false);
	void Receive();
	PlayerSAO* StageTwoClientInit(session_t peer_id);

	/*
	 * Command Handlers
	 */

	void handleCommand(NetworkPacket* pkt);

	void handleCommand_Null(NetworkPacket* pkt) {};
	void handleCommand_Deprecated(NetworkPacket* pkt);
	void handleCommand_Init(NetworkPacket* pkt);
	void handleCommand_Init2(NetworkPacket* pkt);
	void handleCommand_ModChannelJoin(NetworkPacket *pkt);
	void handleCommand_ModChannelLeave(NetworkPacket *pkt);
	void handleCommand_ModChannelMsg(NetworkPacket *pkt);
	void handleCommand_RequestMedia(NetworkPacket* pkt);
	void handleCommand_ClientReady(NetworkPacket* pkt);
	void handleCommand_GotBlocks(NetworkPacket* pkt);
	void handleCommand_PlayerPos(NetworkPacket* pkt);
	void handleCommand_DeletedBlocks(NetworkPacket* pkt);
	void handleCommand_InventoryAction(NetworkPacket* pkt);
	void handleCommand_ChatMessage(NetworkPacket* pkt);
	void handleCommand_Damage(NetworkPacket* pkt);
	void handleCommand_PlayerItem(NetworkPacket* pkt);
	void handleCommand_Respawn(NetworkPacket* pkt);
	void handleCommand_Interact(NetworkPacket* pkt);
	void handleCommand_RemovedSounds(NetworkPacket* pkt);
	void handleCommand_NodeMetaFields(NetworkPacket* pkt);
	void handleCommand_InventoryFields(NetworkPacket* pkt);
	void handleCommand_FirstSrp(NetworkPacket* pkt);
	void handleCommand_SrpBytesA(NetworkPacket* pkt);
	void handleCommand_SrpBytesM(NetworkPacket* pkt);

	void ProcessData(NetworkPacket *pkt);

	void Send(NetworkPacket *pkt);
	void Send(session_t peer_id, NetworkPacket *pkt);

	// Helper for handleCommand_PlayerPos and handleCommand_Interact
	void process_PlayerPos(RemotePlayer *player, PlayerSAO *playersao,
		NetworkPacket *pkt);

	// Both setter and getter need no envlock,
	// can be called freely from threads
	void setTimeOfDay(u32 time);

	/*
		Shall be called with the environment locked.
		This is accessed by the map, which is inside the environment,
		so it shouldn't be a problem.
	*/
	void onMapEditEvent(const MapEditEvent &event);

	// Connection must be locked when called
	std::wstring getStatusString();
	inline double getUptime() const { return m_uptime_counter->get(); }

	// read shutdown state
	inline bool isShutdownRequested() const { return m_shutdown_state.is_requested; }

	// request server to shutdown
	void requestShutdown(const std::string &msg, bool reconnect, float delay = 0.0f);

	// Returns -1 if failed, sound handle on success
	// Envlock
	s32 playSound(const SimpleSoundSpec &spec, const ServerSoundParams &params,
			bool ephemeral=false);
	void stopSound(s32 handle);
	void fadeSound(s32 handle, float step, float gain);

	// Envlock
	std::set<std::string> getPlayerEffectivePrivs(const std::string &name);
	bool checkPriv(const std::string &name, const std::string &priv);
	void reportPrivsModified(const std::string &name=""); // ""=all
	void reportInventoryFormspecModified(const std::string &name);
	void reportFormspecPrependModified(const std::string &name);

	void setIpBanned(const std::string &ip, const std::string &name);
	void unsetIpBanned(const std::string &ip_or_name);
	std::string getBanDescription(const std::string &ip_or_name);

	void notifyPlayer(const char *name, const std::wstring &msg);
	void notifyPlayers(const std::wstring &msg);

	void spawnParticle(const std::string &playername,
		const ParticleParameters &p);

	u32 addParticleSpawner(const ParticleSpawnerParameters &p,
		ServerActiveObject *attached, const std::string &playername);

	void deleteParticleSpawner(const std::string &playername, u32 id);

	bool dynamicAddMedia(const std::string &filepath);

	ServerInventoryManager *getInventoryMgr() const { return m_inventory_mgr.get(); }
	void sendDetachedInventory(Inventory *inventory, const std::string &name, session_t peer_id);

	// Envlock and conlock should be locked when using scriptapi
	ServerScripting *getScriptIface(){ return m_script; }

	// actions: time-reversed list
	// Return value: success/failure
	bool rollbackRevertActions(const std::list<RollbackAction> &actions,
			std::list<std::string> *log);

	// IGameDef interface
	// Under envlock
	virtual IItemDefManager* getItemDefManager();
	virtual const NodeDefManager* getNodeDefManager();
	virtual ICraftDefManager* getCraftDefManager();
	virtual u16 allocateUnknownNodeId(const std::string &name);
	IRollbackManager *getRollbackManager() { return m_rollback; }
	virtual EmergeManager *getEmergeManager() { return m_emerge; }

	IWritableItemDefManager* getWritableItemDefManager();
	NodeDefManager* getWritableNodeDefManager();
	IWritableCraftDefManager* getWritableCraftDefManager();

	virtual const std::vector<ModSpec> &getMods() const;
	virtual const ModSpec* getModSpec(const std::string &modname) const;
	void getModNames(std::vector<std::string> &modlist);
	std::string getBuiltinLuaPath();
	virtual std::string getWorldPath() const { return m_path_world; }
	virtual std::string getModStoragePath() const;

	inline bool isSingleplayer()
			{ return m_simple_singleplayer_mode; }

	inline void setAsyncFatalError(const std::string &error)
			{ m_async_fatal_error.set(error); }

	bool showFormspec(const char *name, const std::string &formspec, const std::string &formname);
	Map & getMap() { return m_env->getMap(); }
	ServerEnvironment & getEnv() { return *m_env; }
	v3f findSpawnPos();

	u32 hudAdd(RemotePlayer *player, HudElement *element);
	bool hudRemove(RemotePlayer *player, u32 id);
	bool hudChange(RemotePlayer *player, u32 id, HudElementStat stat, void *value);
	bool hudSetFlags(RemotePlayer *player, u32 flags, u32 mask);
	bool hudSetHotbarItemcount(RemotePlayer *player, s32 hotbar_itemcount);
	void hudSetHotbarImage(RemotePlayer *player, const std::string &name);
	void hudSetHotbarSelectedImage(RemotePlayer *player, const std::string &name);

	Address getPeerAddress(session_t peer_id);

	void setLocalPlayerAnimations(RemotePlayer *player, v2s32 animation_frames[4],
			f32 frame_speed);
	void setPlayerEyeOffset(RemotePlayer *player, const v3f &first, const v3f &third);

	void setSky(RemotePlayer *player, const SkyboxParams &params);
	void setSun(RemotePlayer *player, const SunParams &params);
	void setMoon(RemotePlayer *player, const MoonParams &params);
	void setStars(RemotePlayer *player, const StarParams &params);

	void setClouds(RemotePlayer *player, const CloudParams &params);

	void overrideDayNightRatio(RemotePlayer *player, bool do_override, float brightness);

	/* con::PeerHandler implementation. */
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);

	void DenySudoAccess(session_t peer_id);
	void DenyAccessVerCompliant(session_t peer_id, u16 proto_ver, AccessDeniedCode reason,
		const std::string &str_reason = "", bool reconnect = false);
	void DenyAccess(session_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason = "");
	void acceptAuth(session_t peer_id, bool forSudoMode);
	void DenyAccess_Legacy(session_t peer_id, const std::wstring &reason);
	void DisconnectPeer(session_t peer_id);
	bool getClientConInfo(session_t peer_id, con::rtt_stat_type type, float *retval);
	bool getClientInfo(session_t peer_id, ClientState *state, u32 *uptime,
			u8* ser_vers, u16* prot_vers, u8* major, u8* minor, u8* patch,
			std::string* vers_string, std::string* lang_code);

	void printToConsoleOnly(const std::string &text);

	void SendPlayerHPOrDie(PlayerSAO *player, const PlayerHPChangeReason &reason);
	void SendPlayerBreath(PlayerSAO *sao);
	void SendInventory(PlayerSAO *playerSAO, bool incremental);
	void SendMovePlayer(session_t peer_id);
	void SendPlayerSpeed(session_t peer_id, const v3f &added_vel);
	void SendPlayerFov(session_t peer_id);

	void sendDetachedInventories(session_t peer_id, bool incremental);

	virtual bool registerModStorage(ModMetadata *storage);
	virtual void unregisterModStorage(const std::string &name);

	bool joinModChannel(const std::string &channel);
	bool leaveModChannel(const std::string &channel);
	bool sendModChannelMessage(const std::string &channel, const std::string &message);
	ModChannel *getModChannel(const std::string &channel);

	// Send block to specific player only
	bool SendBlock(session_t peer_id, const v3s16 &blockpos);

	// Get or load translations for a language
	Translations *getTranslationLanguage(const std::string &lang_code);

	// Bind address
	Address m_bind_addr;

	// Environment mutex (envlock)
	std::mutex m_env_mutex;

private:
	friend class EmergeThread;
	friend class RemoteClient;
	friend class TestServerShutdownState;

	struct ShutdownState {
		friend class TestServerShutdownState;
		public:
			bool is_requested = false;
			bool should_reconnect = false;
			std::string message;

			void reset();
			void trigger(float delay, const std::string &msg, bool reconnect);
			void tick(float dtime, Server *server);
			std::wstring getShutdownTimerMessage() const;
			bool isTimerRunning() const { return m_timer > 0.0f; }
		private:
			float m_timer = 0.0f;
	};

	void init();

	void SendMovement(session_t peer_id);
	void SendHP(session_t peer_id, u16 hp);
	void SendBreath(session_t peer_id, u16 breath);
	void SendAccessDenied(session_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason, bool reconnect = false);
	void SendAccessDenied_Legacy(session_t peer_id, const std::wstring &reason);
	void SendDeathscreen(session_t peer_id, bool set_camera_point_target,
		v3f camera_point_target);
	void SendItemDef(session_t peer_id, IItemDefManager *itemdef, u16 protocol_version);
	void SendNodeDef(session_t peer_id, const NodeDefManager *nodedef,
		u16 protocol_version);

	/* mark blocks not sent for all clients */
	void SetBlocksNotSent(std::map<v3s16, MapBlock *>& block);


	virtual void SendChatMessage(session_t peer_id, const ChatMessage &message);
	void SendTimeOfDay(session_t peer_id, u16 time, f32 time_speed);
	void SendPlayerHP(session_t peer_id);

	void SendLocalPlayerAnimations(session_t peer_id, v2s32 animation_frames[4],
		f32 animation_speed);
	void SendEyeOffset(session_t peer_id, v3f first, v3f third);
	void SendPlayerPrivileges(session_t peer_id);
	void SendPlayerInventoryFormspec(session_t peer_id);
	void SendPlayerFormspecPrepend(session_t peer_id);
	void SendShowFormspecMessage(session_t peer_id, const std::string &formspec,
		const std::string &formname);
	void SendHUDAdd(session_t peer_id, u32 id, HudElement *form);
	void SendHUDRemove(session_t peer_id, u32 id);
	void SendHUDChange(session_t peer_id, u32 id, HudElementStat stat, void *value);
	void SendHUDSetFlags(session_t peer_id, u32 flags, u32 mask);
	void SendHUDSetParam(session_t peer_id, u16 param, const std::string &value);
	void SendSetSky(session_t peer_id, const SkyboxParams &params);
	void SendSetSun(session_t peer_id, const SunParams &params);
	void SendSetMoon(session_t peer_id, const MoonParams &params);
	void SendSetStars(session_t peer_id, const StarParams &params);
	void SendCloudParams(session_t peer_id, const CloudParams &params);
	void SendOverrideDayNightRatio(session_t peer_id, bool do_override, float ratio);
	void broadcastModChannelMessage(const std::string &channel,
			const std::string &message, session_t from_peer);

	/*
		Send a node removal/addition event to all clients except ignore_id.
		Additionally, if far_players!=NULL, players further away than
		far_d_nodes are ignored and their peer_ids are added to far_players
	*/
	// Envlock and conlock should be locked when calling these
	void sendRemoveNode(v3s16 p, std::unordered_set<u16> *far_players = nullptr,
			float far_d_nodes = 100);
	void sendAddNode(v3s16 p, MapNode n,
			std::unordered_set<u16> *far_players = nullptr,
			float far_d_nodes = 100, bool remove_metadata = true);

	void sendMetadataChanged(const std::list<v3s16> &meta_updates,
			float far_d_nodes = 100);

	// Environment and Connection must be locked when called
	void SendBlockNoLock(session_t peer_id, MapBlock *block, u8 ver, u16 net_proto_version);

	// Sends blocks to clients (locks env and con on its own)
	void SendBlocks(float dtime);

	bool addMediaFile(const std::string &filename, const std::string &filepath,
			std::string *filedata = nullptr, std::string *digest = nullptr);
	void fillMediaCache();
	void sendMediaAnnouncement(session_t peer_id, const std::string &lang_code);
	void sendRequestedMedia(session_t peer_id,
			const std::vector<std::string> &tosend);

	// Adds a ParticleSpawner on peer with peer_id (PEER_ID_INEXISTENT == all)
	void SendAddParticleSpawner(session_t peer_id, u16 protocol_version,
		const ParticleSpawnerParameters &p, u16 attached_id, u32 id);

	void SendDeleteParticleSpawner(session_t peer_id, u32 id);

	// Spawns particle on peer with peer_id (PEER_ID_INEXISTENT == all)
	void SendSpawnParticle(session_t peer_id, u16 protocol_version,
		const ParticleParameters &p);

	void SendActiveObjectRemoveAdd(RemoteClient *client, PlayerSAO *playersao);
	void SendActiveObjectMessages(session_t peer_id, const std::string &datas,
		bool reliable = true);
	void SendCSMRestrictionFlags(session_t peer_id);

	/*
		Something random
	*/

	void DiePlayer(session_t peer_id, const PlayerHPChangeReason &reason);
	void RespawnPlayer(session_t peer_id);
	void DeleteClient(session_t peer_id, ClientDeletionReason reason);
	void UpdateCrafting(RemotePlayer *player);
	bool checkInteractDistance(RemotePlayer *player, const f32 d, const std::string &what);

	void handleChatInterfaceEvent(ChatEvent *evt);

	// This returns the answer to the sender of wmessage, or "" if there is none
	std::wstring handleChat(const std::string &name, const std::wstring &wname,
		std::wstring wmessage_input,
		bool check_shout_priv = false,
		RemotePlayer *player = NULL);
	void handleAdminChat(const ChatEventChat *evt);

	// When called, connection mutex should be locked
	RemoteClient* getClient(session_t peer_id, ClientState state_min = CS_Active);
	RemoteClient* getClientNoEx(session_t peer_id, ClientState state_min = CS_Active);

	// When called, environment mutex should be locked
	std::string getPlayerName(session_t peer_id);
	PlayerSAO *getPlayerSAO(session_t peer_id);

	/*
		Get a player from memory or creates one.
		If player is already connected, return NULL
		Does not verify/modify auth info and password.

		Call with env and con locked.
	*/
	PlayerSAO *emergePlayer(const char *name, session_t peer_id, u16 proto_version);

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
	u16 m_max_chatmessage_length;
	// For "dedicated" server list flag
	bool m_dedicated;

	// Thread can set; step() will throw as ServerError
	MutexedVariable<std::string> m_async_fatal_error;

	// Some timers
	float m_liquid_transform_timer = 0.0f;
	float m_liquid_transform_every = 1.0f;
	float m_masterserver_timer = 0.0f;
	float m_emergethread_trigger_timer = 0.0f;
	float m_savemap_timer = 0.0f;
	IntervalLimiter m_map_timer_and_unload_interval;

	// Environment
	ServerEnvironment *m_env = nullptr;

	// server connection
	std::shared_ptr<con::Connection> m_con;

	// Ban checking
	BanManager *m_banmanager = nullptr;

	// Rollback manager (behind m_env_mutex)
	IRollbackManager *m_rollback = nullptr;

	// Emerge manager
	EmergeManager *m_emerge = nullptr;

	// Scripting
	// Envlock and conlock should be locked when using Lua
	ServerScripting *m_script = nullptr;

	// Item definition manager
	IWritableItemDefManager *m_itemdef;

	// Node definition manager
	NodeDefManager *m_nodedef;

	// Craft definition manager
	IWritableCraftDefManager *m_craftdef;

	// Event manager
	EventManager *m_event;

	// Mods
	std::unique_ptr<ServerModManager> m_modmgr;

	std::unordered_map<std::string, Translations> server_translations;

	/*
		Threads
	*/
	// A buffer for time steps
	// step() increments and AsyncRunStep() run by m_thread reads it.
	float m_step_dtime = 0.0f;
	std::mutex m_step_dtime_mutex;

	// The server mainly operates in this thread
	ServerThread *m_thread = nullptr;

	/*
		Time related stuff
	*/
	// Timer for sending time of day over network
	float m_time_of_day_send_timer = 0.0f;

	/*
	 	Client interface
	*/
	ClientInterface m_clients;

	/*
		Peer change queue.
		Queues stuff from peerAdded() and deletingPeer() to
		handlePeerChanges()
	*/
	std::queue<con::PeerChange> m_peer_change_queue;

	std::unordered_map<session_t, std::string> m_formspec_state_data;

	/*
		Random stuff
	*/

	ShutdownState m_shutdown_state;

	ChatInterface *m_admin_chat;
	std::string m_admin_nick;

	// if a mod-error occurs in the on_shutdown callback, the error message will
	// be written into this
	std::string *const m_on_shutdown_errmsg;

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
	std::queue<MapEditEvent*> m_unsent_map_edit_queue;
	/*
		If a non-empty area, map edit events contained within are left
		unsent. Done at map generation time to speed up editing of the
		generated area, as it will be sent anyway.
		This is behind m_env_mutex
	*/
	VoxelArea m_ignore_map_edit_events_area;

	// media files known to server
	std::unordered_map<std::string, MediaInfo> m_media;

	/*
		Sounds
	*/
	std::unordered_map<s32, ServerPlayingSound> m_playing_sounds;
	s32 m_next_sound_id = 0; // positive values only
	s32 nextSoundId();

	std::unordered_map<std::string, ModMetadata *> m_mod_storages;
	float m_mod_storage_save_timer = 10.0f;

	// CSM restrictions byteflag
	u64 m_csm_restriction_flags = CSMRestrictionFlags::CSM_RF_NONE;
	u32 m_csm_restriction_noderange = 8;

	// ModChannel manager
	std::unique_ptr<ModChannelMgr> m_modchannel_mgr;

	// Inventory manager
	std::unique_ptr<ServerInventoryManager> m_inventory_mgr;

	// Global server metrics backend
	std::unique_ptr<MetricsBackend> m_metrics_backend;

	// Server metrics
	MetricCounterPtr m_uptime_counter;
	MetricGaugePtr m_player_gauge;
	MetricGaugePtr m_timeofday_gauge;
	// current server step lag
	MetricGaugePtr m_lag_gauge;
	MetricCounterPtr m_aom_buffer_counter;
	MetricCounterPtr m_packet_recv_counter;
	MetricCounterPtr m_packet_recv_processed_counter;
};

/*
	Runs a simple dedicated server loop.

	Shuts down when kill is set to true.
*/
void dedicated_server_loop(Server &server, bool &kill);
