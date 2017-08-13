/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "network/connection.h"
#include "clientenvironment.h"
#include "irrlichttypes_extrabloated.h"
#include "threading/mutex.h"
#include <ostream>
#include <map>
#include <set>
#include <vector>
#include "clientobject.h"
#include "gamedef.h"
#include "inventorymanager.h"
#include "localplayer.h"
#include "hud.h"
#include "particles.h"
#include "mapnode.h"
#include "tileanimation.h"
#include "mesh_generator_thread.h"

#define CLIENT_CHAT_MESSAGE_LIMIT_PER_10S 10.0f

struct MeshMakeData;
class MapBlockMesh;
class IWritableTextureSource;
class IWritableShaderSource;
class IWritableItemDefManager;
class IWritableNodeDefManager;
//class IWritableCraftDefManager;
class ClientMediaDownloader;
struct MapDrawControl;
class MtEventManager;
struct PointedThing;
class MapDatabase;
class Minimap;
struct MinimapMapblock;
class Camera;
class NetworkPacket;

enum LocalClientState {
	LC_Created,
	LC_Init,
	LC_Ready
};

enum ClientEventType
{
	CE_NONE,
	CE_PLAYER_DAMAGE,
	CE_PLAYER_FORCE_MOVE,
	CE_DEATHSCREEN,
	CE_SHOW_FORMSPEC,
	CE_SHOW_LOCAL_FORMSPEC,
	CE_SPAWN_PARTICLE,
	CE_ADD_PARTICLESPAWNER,
	CE_DELETE_PARTICLESPAWNER,
	CE_HUDADD,
	CE_HUDRM,
	CE_HUDCHANGE,
	CE_SET_SKY,
	CE_OVERRIDE_DAY_NIGHT_RATIO,
	CE_CLOUD_PARAMS,
};

struct ClientEvent
{
	ClientEventType type;
	union{
		//struct{
		//} none;
		struct{
			u8 amount;
		} player_damage;
		struct{
			f32 pitch;
			f32 yaw;
		} player_force_move;
		struct{
			bool set_camera_point_target;
			f32 camera_point_target_x;
			f32 camera_point_target_y;
			f32 camera_point_target_z;
		} deathscreen;
		struct{
			std::string *formspec;
			std::string *formname;
		} show_formspec;
		//struct{
		//} textures_updated;
		struct{
			v3f *pos;
			v3f *vel;
			v3f *acc;
			f32 expirationtime;
			f32 size;
			bool collisiondetection;
			bool collision_removal;
			bool vertical;
			std::string *texture;
			struct TileAnimationParams animation;
			u8 glow;
		} spawn_particle;
		struct{
			u16 amount;
			f32 spawntime;
			v3f *minpos;
			v3f *maxpos;
			v3f *minvel;
			v3f *maxvel;
			v3f *minacc;
			v3f *maxacc;
			f32 minexptime;
			f32 maxexptime;
			f32 minsize;
			f32 maxsize;
			bool collisiondetection;
			bool collision_removal;
			u16 attached_id;
			bool vertical;
			std::string *texture;
			u32 id;
			struct TileAnimationParams animation;
			u8 glow;
		} add_particlespawner;
		struct{
			u32 id;
		} delete_particlespawner;
		struct{
			u32 id;
			u8 type;
			v2f *pos;
			std::string *name;
			v2f *scale;
			std::string *text;
			u32 number;
			u32 item;
			u32 dir;
			v2f *align;
			v2f *offset;
			v3f *world_pos;
			v2s32 * size;
		} hudadd;
		struct{
			u32 id;
		} hudrm;
		struct{
			u32 id;
			HudElementStat stat;
			v2f *v2fdata;
			std::string *sdata;
			u32 data;
			v3f *v3fdata;
			v2s32 * v2s32data;
		} hudchange;
		struct{
			video::SColor *bgcolor;
			std::string *type;
			std::vector<std::string> *params;
			bool clouds;
		} set_sky;
		struct{
			bool do_override;
			float ratio_f;
		} override_day_night_ratio;
		struct {
			f32 density;
			u32 color_bright;
			u32 color_ambient;
			f32 height;
			f32 thickness;
			f32 speed_x;
			f32 speed_y;
		} cloud_params;
	};
};

/*
	Packet counter
*/

class PacketCounter
{
public:
	PacketCounter()
	{
	}

	void add(u16 command)
	{
		std::map<u16, u16>::iterator n = m_packets.find(command);
		if(n == m_packets.end())
		{
			m_packets[command] = 1;
		}
		else
		{
			n->second++;
		}
	}

	void clear()
	{
		for(std::map<u16, u16>::iterator
				i = m_packets.begin();
				i != m_packets.end(); ++i)
		{
			i->second = 0;
		}
	}

	void print(std::ostream &o)
	{
		for(std::map<u16, u16>::iterator
				i = m_packets.begin();
				i != m_packets.end(); ++i)
		{
			o<<"cmd "<<i->first
					<<" count "<<i->second
					<<std::endl;
		}
	}

private:
	// command, count
	std::map<u16, u16> m_packets;
};

class ClientScripting;
struct GameUIFlags;

class Client : public con::PeerHandler, public InventoryManager, public IGameDef
{
public:
	/*
		NOTE: Nothing is thread-safe here.
	*/

	Client(
			IrrlichtDevice *device,
			const char *playername,
			const std::string &password,
			const std::string &address_name,
			MapDrawControl &control,
			IWritableTextureSource *tsrc,
			IWritableShaderSource *shsrc,
			IWritableItemDefManager *itemdef,
			IWritableNodeDefManager *nodedef,
			ISoundManager *sound,
			MtEventManager *event,
			bool ipv6,
			GameUIFlags *game_ui_flags
	);

	~Client();

	void initMods();

	/*
	 request all threads managed by client to be stopped
	 */
	void Stop();


	bool isShutdown();

	/*
		The name of the local player should already be set when
		calling this, as it is sent in the initialization.
	*/
	void connect(Address address, bool is_local_server);

	/*
		Stuff that references the environment is valid only as
		long as this is not called. (eg. Players)
		If this throws a PeerNotFoundException, the connection has
		timed out.
	*/
	void step(float dtime);

	/*
	 * Command Handlers
	 */

	void handleCommand(NetworkPacket* pkt);

	void handleCommand_Null(NetworkPacket* pkt) {};
	void handleCommand_Deprecated(NetworkPacket* pkt);
	void handleCommand_Hello(NetworkPacket* pkt);
	void handleCommand_AuthAccept(NetworkPacket* pkt);
	void handleCommand_AcceptSudoMode(NetworkPacket* pkt);
	void handleCommand_DenySudoMode(NetworkPacket* pkt);
	void handleCommand_InitLegacy(NetworkPacket* pkt);
	void handleCommand_AccessDenied(NetworkPacket* pkt);
	void handleCommand_RemoveNode(NetworkPacket* pkt);
	void handleCommand_AddNode(NetworkPacket* pkt);
	void handleCommand_BlockData(NetworkPacket* pkt);
	void handleCommand_Inventory(NetworkPacket* pkt);
	void handleCommand_TimeOfDay(NetworkPacket* pkt);
	void handleCommand_ChatMessage(NetworkPacket* pkt);
	void handleCommand_ActiveObjectRemoveAdd(NetworkPacket* pkt);
	void handleCommand_ActiveObjectMessages(NetworkPacket* pkt);
	void handleCommand_Movement(NetworkPacket* pkt);
	void handleCommand_HP(NetworkPacket* pkt);
	void handleCommand_Breath(NetworkPacket* pkt);
	void handleCommand_MovePlayer(NetworkPacket* pkt);
	void handleCommand_DeathScreen(NetworkPacket* pkt);
	void handleCommand_AnnounceMedia(NetworkPacket* pkt);
	void handleCommand_Media(NetworkPacket* pkt);
	void handleCommand_NodeDef(NetworkPacket* pkt);
	void handleCommand_ItemDef(NetworkPacket* pkt);
	void handleCommand_PlaySound(NetworkPacket* pkt);
	void handleCommand_StopSound(NetworkPacket* pkt);
	void handleCommand_FadeSound(NetworkPacket *pkt);
	void handleCommand_Privileges(NetworkPacket* pkt);
	void handleCommand_InventoryFormSpec(NetworkPacket* pkt);
	void handleCommand_DetachedInventory(NetworkPacket* pkt);
	void handleCommand_ShowFormSpec(NetworkPacket* pkt);
	void handleCommand_SpawnParticle(NetworkPacket* pkt);
	void handleCommand_AddParticleSpawner(NetworkPacket* pkt);
	void handleCommand_DeleteParticleSpawner(NetworkPacket* pkt);
	void handleCommand_HudAdd(NetworkPacket* pkt);
	void handleCommand_HudRemove(NetworkPacket* pkt);
	void handleCommand_HudChange(NetworkPacket* pkt);
	void handleCommand_HudSetFlags(NetworkPacket* pkt);
	void handleCommand_HudSetParam(NetworkPacket* pkt);
	void handleCommand_HudSetSky(NetworkPacket* pkt);
	void handleCommand_CloudParams(NetworkPacket* pkt);
	void handleCommand_OverrideDayNightRatio(NetworkPacket* pkt);
	void handleCommand_LocalPlayerAnimations(NetworkPacket* pkt);
	void handleCommand_EyeOffset(NetworkPacket* pkt);
	void handleCommand_SrpBytesSandB(NetworkPacket* pkt);

	void ProcessData(NetworkPacket *pkt);

	void Send(NetworkPacket* pkt);

	void interact(u8 action, const PointedThing& pointed);

	void sendNodemetaFields(v3s16 p, const std::string &formname,
		const StringMap &fields);
	void sendInventoryFields(const std::string &formname,
		const StringMap &fields);
	void sendInventoryAction(InventoryAction *a);
	void sendChatMessage(const std::wstring &message);
	void clearOutChatQueue();
	void sendChangePassword(const std::string &oldpassword,
		const std::string &newpassword);
	void sendDamage(u8 damage);
	void sendBreath(u16 breath);
	void sendRespawn();
	void sendReady();

	ClientEnvironment& getEnv() { return m_env; }
	ITextureSource *tsrc() { return getTextureSource(); }
	ISoundManager *sound() { return getSoundManager(); }
	static const std::string &getBuiltinLuaPath();
	static const std::string &getClientModsLuaPath();

	virtual const std::vector<ModSpec> &getMods() const;
	virtual const ModSpec* getModSpec(const std::string &modname) const;

	// Causes urgent mesh updates (unlike Map::add/removeNodeWithEvent)
	void removeNode(v3s16 p);
	MapNode getNode(v3s16 p, bool *is_valid_position);
	void addNode(v3s16 p, MapNode n, bool remove_metadata = true);

	void setPlayerControl(PlayerControl &control);

	void selectPlayerItem(u16 item);
	u16 getPlayerItem() const
	{ return m_playeritem; }

	// Returns true if the inventory of the local player has been
	// updated from the server. If it is true, it is set to false.
	bool getLocalInventoryUpdated();
	// Copies the inventory of the local player to parameter
	void getLocalInventory(Inventory &dst);

	/* InventoryManager interface */
	Inventory* getInventory(const InventoryLocation &loc);
	void inventoryAction(InventoryAction *a);

	const std::list<std::string> &getConnectedPlayerNames()
	{
		return m_env.getPlayerNames();
	}

	float getAnimationTime();

	int getCrackLevel();
	v3s16 getCrackPos();
	void setCrack(int level, v3s16 pos);

	u16 getHP();

	bool checkPrivilege(const std::string &priv) const
	{ return (m_privileges.count(priv) != 0); }

	bool getChatMessage(std::wstring &message);
	void typeChatMessage(const std::wstring& message);

	u64 getMapSeed(){ return m_map_seed; }

	void addUpdateMeshTask(v3s16 blockpos, bool ack_to_server=false, bool urgent=false);
	// Including blocks at appropriate edges
	void addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server=false, bool urgent=false);
	void addUpdateMeshTaskForNode(v3s16 nodepos, bool ack_to_server=false, bool urgent=false);

	void updateCameraOffset(v3s16 camera_offset)
	{ m_mesh_update_thread.m_camera_offset = camera_offset; }

	bool hasClientEvents() const { return !m_client_event_queue.empty(); }
	// Get event from queue. If queue is empty, it triggers an assertion failure.
	ClientEvent getClientEvent();

	bool accessDenied() const { return m_access_denied; }

	bool reconnectRequested() const { return m_access_denied_reconnect; }

	void setFatalError(const std::string &reason)
	{
		m_access_denied = true;
		m_access_denied_reason = reason;
	}

	// Renaming accessDeniedReason to better name could be good as it's used to
	// disconnect client when CSM failed.
	const std::string &accessDeniedReason() const { return m_access_denied_reason; }

	bool itemdefReceived()
	{ return m_itemdef_received; }
	bool nodedefReceived()
	{ return m_nodedef_received; }
	bool mediaReceived()
	{ return m_media_downloader == NULL; }

	u8 getProtoVersion()
	{ return m_proto_ver; }

	bool connectedToServer()
	{ return m_con.Connected(); }

	float mediaReceiveProgress();

	void afterContentReceived(IrrlichtDevice *device);

	float getRTT();
	float getCurRate();

	Minimap* getMinimap() { return m_minimap; }
	void setCamera(Camera* camera) { m_camera = camera; }

	Camera* getCamera () { return m_camera; }

	bool shouldShowMinimap() const;

	// IGameDef interface
	virtual IItemDefManager* getItemDefManager();
	virtual INodeDefManager* getNodeDefManager();
	virtual ICraftDefManager* getCraftDefManager();
	ITextureSource* getTextureSource();
	virtual IShaderSource* getShaderSource();
	IShaderSource *shsrc() { return getShaderSource(); }
	scene::ISceneManager* getSceneManager();
	virtual u16 allocateUnknownNodeId(const std::string &name);
	virtual ISoundManager* getSoundManager();
	virtual MtEventManager* getEventManager();
	virtual ParticleManager* getParticleManager();
	bool checkLocalPrivilege(const std::string &priv)
	{ return checkPrivilege(priv); }
	virtual scene::IAnimatedMesh* getMesh(const std::string &filename);

	virtual std::string getModStoragePath() const;
	virtual bool registerModStorage(ModMetadata *meta);
	virtual void unregisterModStorage(const std::string &name);

	// The following set of functions is used by ClientMediaDownloader
	// Insert a media file appropriately into the appropriate manager
	bool loadMedia(const std::string &data, const std::string &filename);
	// Send a request for conventional media transfer
	void request_media(const std::vector<std::string> &file_requests);

	LocalClientState getState() { return m_state; }

	void makeScreenshot(IrrlichtDevice *device);

	inline void pushToChatQueue(const std::wstring &input)
	{
		m_chat_queue.push(input);
	}

	ClientScripting *getScript() { return m_script; }
	const bool moddingEnabled() const { return m_modding_enabled; }

	inline void pushToEventQueue(const ClientEvent &event)
	{
		m_client_event_queue.push(event);
	}

	void showGameChat(const bool show = true);
	void showGameHud(const bool show = true);
	void showMinimap(const bool show = true);
	void showProfiler(const bool show = true);
	void showGameFog(const bool show = true);
	void showGameDebug(const bool show = true);

	IrrlichtDevice *getDevice() const { return m_device; }

	const Address getServerAddress()
	{
		return m_con.GetPeerAddress(PEER_ID_SERVER);
	}

	const std::string &getAddressName() const
	{
		return m_address_name;
	}

private:

	// Virtual methods from con::PeerHandler
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);

	void initLocalMapSaving(const Address &address,
			const std::string &hostname,
			bool is_local_server);

	void ReceiveAll();
	void Receive();

	void sendPlayerPos();
	// Send the item number 'item' as player item to the server
	void sendPlayerItem(u16 item);

	void deleteAuthData();
	// helper method shared with clientpackethandler
	static AuthMechanism choseAuthMech(const u32 mechs);

	void sendLegacyInit(const char* playerName, const char* playerPassword);
	void sendInit(const std::string &playerName);
	void startAuth(AuthMechanism chosen_auth_mechanism);
	void sendDeletedBlocks(std::vector<v3s16> &blocks);
	void sendGotBlocks(v3s16 block);
	void sendRemovedSounds(std::vector<s32> &soundList);

	// Helper function
	inline std::string getPlayerName()
	{ return m_env.getLocalPlayer()->getName(); }

	bool canSendChatMessage() const;

	float m_packetcounter_timer;
	float m_connection_reinit_timer;
	float m_avg_rtt_timer;
	float m_playerpos_send_timer;
	IntervalLimiter m_map_timer_and_unload_interval;

	IWritableTextureSource *m_tsrc;
	IWritableShaderSource *m_shsrc;
	IWritableItemDefManager *m_itemdef;
	IWritableNodeDefManager *m_nodedef;
	ISoundManager *m_sound;
	MtEventManager *m_event;


	MeshUpdateThread m_mesh_update_thread;
	ClientEnvironment m_env;
	ParticleManager m_particle_manager;
	con::Connection m_con;
	std::string m_address_name;
	IrrlichtDevice *m_device;
	Camera *m_camera;
	Minimap *m_minimap;
	bool m_minimap_disabled_by_server;
	// Server serialization version
	u8 m_server_ser_ver;

	// Used version of the protocol with server
	// Values smaller than 25 only mean they are smaller than 25,
	// and aren't accurate. We simply just don't know, because
	// the server didn't send the version back then.
	// If 0, server init hasn't been received yet.
	u8 m_proto_ver;

	u16 m_playeritem;
	bool m_inventory_updated;
	Inventory *m_inventory_from_server;
	float m_inventory_from_server_age;
	PacketCounter m_packetcounter;
	// Block mesh animation parameters
	float m_animation_time;
	int m_crack_level;
	v3s16 m_crack_pos;
	// 0 <= m_daynight_i < DAYNIGHT_CACHE_COUNT
	//s32 m_daynight_i;
	//u32 m_daynight_ratio;
	std::queue<std::wstring> m_chat_queue;
	std::queue<std::wstring> m_out_chat_queue;
	u32 m_last_chat_message_sent;
	float m_chat_message_allowance;

	// The authentication methods we can use to enter sudo mode (=change password)
	u32 m_sudo_auth_methods;

	// The seed returned by the server in TOCLIENT_INIT is stored here
	u64 m_map_seed;

	// Auth data
	std::string m_playername;
	std::string m_password;
	// If set, this will be sent (and cleared) upon a TOCLIENT_ACCEPT_SUDO_MODE
	std::string m_new_password;
	// Usable by auth mechanisms.
	AuthMechanism m_chosen_auth_mech;
	void * m_auth_data;


	bool m_access_denied;
	bool m_access_denied_reconnect;
	std::string m_access_denied_reason;
	std::queue<ClientEvent> m_client_event_queue;
	bool m_itemdef_received;
	bool m_nodedef_received;
	ClientMediaDownloader *m_media_downloader;

	// time_of_day speed approximation for old protocol
	bool m_time_of_day_set;
	float m_last_time_of_day_f;
	float m_time_of_day_update_timer;

	// An interval for generally sending object positions and stuff
	float m_recommended_send_interval;

	// Sounds
	float m_removed_sounds_check_timer;
	// Mapping from server sound ids to our sound ids
	UNORDERED_MAP<s32, int> m_sounds_server_to_client;
	// And the other way!
	UNORDERED_MAP<int, s32> m_sounds_client_to_server;
	// And relations to objects
	UNORDERED_MAP<int, u16> m_sounds_to_objects;

	// Privileges
	UNORDERED_SET<std::string> m_privileges;

	// Detached inventories
	// key = name
	UNORDERED_MAP<std::string, Inventory*> m_detached_inventories;

	// Storage for mesh data for creating multiple instances of the same mesh
	StringMap m_mesh_data;

	// own state
	LocalClientState m_state;

	// Used for saving server map to disk client-side
	MapDatabase *m_localdb;
	IntervalLimiter m_localdb_save_interval;
	u16 m_cache_save_interval;

	ClientScripting *m_script;
	bool m_modding_enabled;
	UNORDERED_MAP<std::string, ModMetadata *> m_mod_storages;
	float m_mod_storage_save_timer;
	GameUIFlags *m_game_ui_flags;

	bool m_shutdown;
	DISABLE_CLASS_COPY(Client);
};

#endif // !CLIENT_HEADER
