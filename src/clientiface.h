/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#ifndef _CLIENTIFACE_H_
#define _CLIENTIFACE_H_

#include "irr_v3d.h"                   // for irrlicht datatypes

#include "constants.h"
#include "serialization.h"             // for SER_FMT_VER_INVALID
#include "voxel.h"                     // for VoxelArea
#include "threading/mutex.h"
#include "network/networkpacket.h"

#include <list>
#include <vector>
#include <map>
#include <set>

class MapBlock;
class ServerEnvironment;
class EmergeManager;

/*
 * State Transitions

      Start
  (peer connect)
        |
        v
      /-----------------\
      |                 |
      |    Created      |
      |                 |
      \-----------------/
               |                  depending of the incoming packet
               +---------------------------------------
               v                                      v
+-----------------------------+        +-----------------------------+
|IN:                          |        |IN:                          |
| TOSERVER_INIT_LEGACY        |-----   | TOSERVER_INIT               |      invalid playername,
+-----------------------------+    |   +-----------------------------+  password (for _LEGACY),
               |                   |                  |                       or denied by mod
               | Auth ok           -------------------+---------------------------------
               v                                      v                                |
+-----------------------------+        +-----------------------------+                 |
|OUT:                         |        |OUT:                         |                 |
| TOCLIENT_INIT_LEGACY        |        | TOCLIENT_HELLO              |                 |
+-----------------------------+        +-----------------------------+                 |
               |                                      |                                |
               |                                      |                                |
               v                                      v                                |
      /-----------------\                    /-----------------\                       |
      |                 |                    |                 |                       |
      |  AwaitingInit2  |<---------          |    HelloSent    |                       |
      |                 |         |          |                 |                       |
      \-----------------/         |          \-----------------/                       |
               |                  |                   |                                |
+-----------------------------+   |    *-----------------------------*     Auth fails  |
|IN:                          |   |    |Authentication, depending on |-----------------+
| TOSERVER_INIT2              |   |    | packet sent by client       |                 |
+-----------------------------+   |    *-----------------------------*                 |
               |                  |                   |                                |
               |                  |                   | Authentication                 |
               v                  |                   |  successful                    |
      /-----------------\         |                   v                                |
      |                 |         |    +-----------------------------+                 |
      |    InitDone     |         |    |OUT:                         |                 |
      |                 |         |    | TOCLIENT_AUTH_ACCEPT        |                 |
      \-----------------/         |    +-----------------------------+                 |
               |                  |                   |                                |
+-----------------------------+   ---------------------                                |
|OUT:                         |                                                        |
| TOCLIENT_MOVEMENT           |                                                        |
| TOCLIENT_ITEMDEF            |                                                        |
| TOCLIENT_NODEDEF            |                                                        |
| TOCLIENT_ANNOUNCE_MEDIA     |                                                        |
| TOCLIENT_DETACHED_INVENTORY |                                                        |
| TOCLIENT_TIME_OF_DAY        |                                                        |
+-----------------------------+                                                        |
               |                                                                       |
               |                                                                       |
               |      -----------------------------                                    |
               v      |                           |                                    |
      /-----------------\                         v                                    |
      |                 |             +-----------------------------+                  |
      | DefinitionsSent |             |IN:                          |                  |
      |                 |             | TOSERVER_REQUEST_MEDIA      |                  |
      \-----------------/             | TOSERVER_RECEIVED_MEDIA     |                  |
               |                      +-----------------------------+                  |
               |      ^                           |                                    |
               |      -----------------------------                                    |
               v                                                                       |
+-----------------------------+                        --------------------------------+
|IN:                          |                        |                               |
| TOSERVER_CLIENT_READY       |                        v                               |
+-----------------------------+        +-------------------------------+               |
               |                       |OUT:                           |               |
               v                       | TOCLIENT_ACCESS_DENIED_LEGAGY |               |
+-----------------------------+        +-------------------------------+               |
|OUT:                         |                        |                               |
| TOCLIENT_MOVE_PLAYER        |                        v                               |
| TOCLIENT_PRIVILEGES         |                /-----------------\                     |
| TOCLIENT_INVENTORY_FORMSPEC |                |                 |                     |
| UpdateCrafting              |                |     Denied      |                     |
| TOCLIENT_INVENTORY          |                |                 |                     |
| TOCLIENT_HP (opt)           |                \-----------------/                     |
| TOCLIENT_BREATH             |                                                        |
| TOCLIENT_DEATHSCREEN        |                                                        |
+-----------------------------+                                                        |
              |                                                                        |
              v                                                                        |
      /-----------------\      async mod action (ban, kick)                            |
      |                 |---------------------------------------------------------------
 ---->|     Active      |
 |    |                 |----------------------------------------------
 |    \-----------------/      timeout                                v
 |       |           |                                  +-----------------------------+
 |       |           |                                  |OUT:                         |
 |       |           |                                  | TOCLIENT_DISCONNECT         |
 |       |           |                                  +-----------------------------+
 |       |           |                                                |
 |       |           v                                                v
 |       |  +-----------------------------+                    /-----------------\
 |       |  |IN:                          |                    |                 |
 |       |  | TOSERVER_DISCONNECT         |------------------->|  Disconnecting  |
 |       |  +-----------------------------+                    |                 |
 |       |                                                     \-----------------/
 |       | any auth packet which was
 |       | allowed in TOCLIENT_AUTH_ACCEPT
 |       v
 |    *-----------------------------* Auth      +-------------------------------+
 |    |Authentication, depending on | succeeds  |OUT:                           |
 |    | packet sent by client       |---------->| TOCLIENT_ACCEPT_SUDO_MODE     |
 |    *-----------------------------*           +-------------------------------+
 |                  |                                            |
 |                  | Auth fails                        /-----------------\
 |                  v                                   |                 |
 |    +-------------------------------+                 |    SudoMode     |
 |    |OUT:                           |                 |                 |
 |    | TOCLIENT_DENY_SUDO_MODE       |                 \-----------------/
 |    +-------------------------------+                          |
 |                  |                                            v
 |                  |                               +-----------------------------+
 |                  |    sets password accordingly  |IN:                          |
 -------------------+-------------------------------| TOSERVER_FIRST_SRP          |
                                                    +-----------------------------+

*/
namespace con {
	class Connection;
}

#define CI_ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

// Also make sure to update the ClientInterface::statenames
// array when modifying these enums

enum ClientState
{
	CS_Invalid,
	CS_Disconnecting,
	CS_Denied,
	CS_Created,
	CS_AwaitingInit2,
	CS_HelloSent,
	CS_InitDone,
	CS_DefinitionsSent,
	CS_Active,
	CS_SudoMode
};

enum ClientStateEvent
{
	CSE_Hello,
	CSE_AuthAccept,
	CSE_InitLegacy,
	CSE_GotInit2,
	CSE_SetDenied,
	CSE_SetDefinitionsSent,
	CSE_SetClientReady,
	CSE_SudoSuccess,
	CSE_SudoLeave,
	CSE_Disconnect
};

class RemoteClient;
struct AutosendCycle;
class ServerFarMap;

struct WMSSuggestion {
	WantedMapSend wms;
	bool is_fully_loaded; // Can be false for FarBlocks
	//bool is_fully_generated; // TODO
	// TODO: Using enum ServerFarBlock::LoadState would be more suitable

	WMSSuggestion():
		is_fully_loaded(true)
	{}
	WMSSuggestion(WantedMapSend wms):
		wms(wms),
		is_fully_loaded(true)
	{}
	std::string describe() const {
		if (wms.type == WMST_FARBLOCK) {
			return wms.describe()+
					": is_fully_loaded="+(is_fully_loaded?"1":"0");
		} else {
			return wms.describe();
		}
	}
};

class AutosendAlgorithm
{
public:
	AutosendAlgorithm(RemoteClient *client);
	~AutosendAlgorithm();

	// Shall be called every time before starting to ask a bunch of blocks by
	// calling getNextBlock()
	void cycle(float dtime, ServerEnvironment *env);

	// Finds a block that should be sent next to the client.
	// Environment should be locked when this is called.
	WMSSuggestion getNextBlock(EmergeManager *emerge, ServerFarMap *far_map);

	void setParameters(s16 radius_map, s16 radius_far, float far_weight,
			float fov) {
		m_radius_map = radius_map;
		m_radius_far = radius_far;
		m_far_weight = far_weight;
		m_fov = fov;
	}

	// If something is modified near a player, this should probably be called so
	// that it gets sent as quickly as possible while being prioritized
	// correctly
	void resetMapblockSearchRadius();

	std::string describeStatus();

private:
	RemoteClient *m_client;
	AutosendCycle *m_cycle;
	s16 m_radius_map; // Updated by the client; 0 disables autosend.
	s16 m_radius_far; // Updated by the client; 0 disables autosend.
	float m_far_weight; // Updated by the client; 0 is invalid.
	float m_fov; // Updated by the client; 0 disables FOV limit.
	v3s16 m_last_focus_point;
	float m_nearest_unsent_reset_timer;

	friend struct AutosendCycle;
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

	/* Authentication information */
	std::string enc_pwd;
	bool create_player_on_auth_success;
	AuthMechanism chosen_mech;
	void * auth_data;
	u32 allowed_auth_mechs;
	u32 allowed_sudo_mechs;

	bool isSudoMechAllowed(AuthMechanism mech)
	{ return allowed_sudo_mechs & mech; }
	bool isMechAllowed(AuthMechanism mech)
	{ return allowed_auth_mechs & mech; }

	RemoteClient(ServerEnvironment *env):
		peer_id(PEER_ID_INEXISTENT),
		serialization_version(SER_FMT_VER_INVALID),
		net_proto_version(0),
		create_player_on_auth_success(false),
		chosen_mech(AUTH_MECHANISM_NONE),
		auth_data(NULL),
		m_time_from_building(9999),
		m_env(env),
		m_pending_serialization_version(SER_FMT_VER_INVALID),
		m_state(CS_Created),
		m_autosend(this),
		m_fallback_autosend_active(true),
		m_excess_gotblocks(0),
		m_name(""),
		m_version_major(0),
		m_version_minor(0),
		m_version_patch(0),
		m_full_version("unknown"),
		m_deployed_compression(0),
		m_connection_time(getTime(PRECISION_SECONDS))
	{
	}
	~RemoteClient()
	{
	}

	// Finds a block that should be sent next to the client.
	// Environment should be locked when this is called.
	WMSSuggestion getNextBlock(EmergeManager *emerge, ServerFarMap *far_map);

	// Shall be called every time before starting to ask a bunch of blocks by
	// calling getNextBlock()
	void cycleAutosendAlgorithm(float dtime);

	void GotBlock(const WantedMapSend &wms);
	void SendingBlock(const WantedMapSend &wms);

	void SetBlockUpdated(const WantedMapSend &wms);
	// Also sets the corresponding FarBlock
	void SetMapBlockUpdated(v3s16 p);
	void SetMapBlocksUpdated(std::map<v3s16, MapBlock*> &blocks);

	/**
	 * tell client about this block being modified right now.
	 * this information is required to requeue the block in case it's "on wire"
	 * while modification is processed by server
	 * @param wms position of modified block
	 */
	void ResendBlockIfOnWire(const WantedMapSend &wms);
	void ResendMapBlockIfOnWire(v3s16 p){
			return ResendBlockIfOnWire(WantedMapSend(WMST_MAPBLOCK, p)); }

	// Total number of MapBlocks and FarBlocks on the wire
	s32 SendingCount()
	{
		return m_blocks_sending.size();
	}

	void setAutosendParameters(s16 radius_map, s16 radius_far, float far_weight,
			float fov)
	{
		m_autosend.setParameters(radius_map, radius_far, far_weight, fov);

		// Disable fallback algorithm
		m_fallback_autosend_active = false;
	}

	void setMapSendQueue(const std::vector<WantedMapSend> &map_send_queue)
	{
		m_map_send_queue = map_send_queue;

		// Disable fallback algorithm
		m_fallback_autosend_active = false;
	}

	void PrintInfo(std::ostream &o)
	{
		o<<"RemoteClient "<<peer_id<<": "
				<<"m_blocks_sent.size()="<<m_blocks_sent.size()
				<<", m_blocks_sending.size()="<<m_blocks_sending.size()
				<<", m_excess_gotblocks="<<m_excess_gotblocks
				<<", m_autosend.describeStatus()="<<m_autosend.describeStatus()
				<<std::endl;
		m_excess_gotblocks = 0;
	}

	// Time from last placing or removing blocks
	float m_time_from_building;

	/*
		Set of active objects that the client knows of.
	*/
	std::set<u16> m_known_objects;

	ClientState getState()
		{ return m_state; }

	std::string getName()
		{ return m_name; }

	void setName(std::string name)
		{ m_name = name; }

	/* update internal client state */
	void notifyEvent(ClientStateEvent event);

	/* set expected serialization version */
	void setPendingSerializationVersion(u8 version)
		{ m_pending_serialization_version = version; }

	void setDeployedCompressionMode(u16 byteFlag)
		{ m_deployed_compression = byteFlag; }

	void confirmSerializationVersion()
		{ serialization_version = m_pending_serialization_version; }

	/* uptime */
	u32 uptime();

	/* version information */
	void setVersionInfo(u8 major, u8 minor, u8 patch, std::string full) {
		m_version_major = major;
		m_version_minor = minor;
		m_version_patch = patch;
		m_full_version = full;
	}
	u8 getMajor() { return m_version_major; }
	u8 getMinor() { return m_version_minor; }
	u8 getPatch() { return m_version_patch; }
	std::string getVersion() { return m_full_version; }

private:
	ServerEnvironment *m_env;

	// Version is stored in here after INIT before INIT2
	u8 m_pending_serialization_version;

	ClientState m_state;

	/*
		Autosend Algorithm

		Normally this is used to select blocks for sending and emerging. The
		client can disable this and set m_map_send_queue instead.
	*/
	AutosendAlgorithm m_autosend;
	// If the client has not given us autosend parameters, this is true and
	// autosend parameters are automatically set by us in order to implement old
	// server behavior.
	bool m_fallback_autosend_active;

	/*
		Client can periodically update this queue to do custom map transfers.

		Autosend always takes priority over this.
	*/
	std::vector<WantedMapSend> m_map_send_queue;

	/*
		Blocks that have been sent to client.
		- These don't have to be sent again.
		- A block is cleared from here when client says it has
		  deleted it from its memory
		- Value is timestamp when block was sent. It is used for rate-limiting
		  updates.
	*/
	std::map<WantedMapSend, time_t> m_blocks_sent;

	/*
		Blocks that are currently on the line.
		- This is used for throttling the sending of blocks.
		- The size of this list is limited to some value
		- Block is added when it is sent with BLOCKDATA.
		- Block is removed when GOTBLOCKS is received.
		- Value is timestamp when block was sent. It is used for rate-limiting
		  updates.
	*/
	std::map<WantedMapSend, time_t> m_blocks_sending;

	/*
		Blocks that have been updated since they were last sent to the client.
		The autosend algorithm re-sends these based on some priority.
	*/
	std::set<WantedMapSend> m_blocks_updated_since_last_send;

	/*
		Blocks that have been updated while they are still being sent. Subset of
		m_blocks_sending. This is not strictly needed, but allows for this
		complex system to validate the correctness of itself.
	*/
	std::set<WantedMapSend> m_blocks_updated_while_sending;

	/*
		Count of excess GotBlocks().
		There is an excess amount because the client sometimes
		gets a block so late that the server sends it again,
		and the client then sends two GOTBLOCKs.
		This is resetted by PrintInfo()
	*/
	u32 m_excess_gotblocks;

	/*
		name of player using this client
	*/
	std::string m_name;

	/*
		client information
	 */
	u8 m_version_major;
	u8 m_version_minor;
	u8 m_version_patch;

	std::string m_full_version;

	u16 m_deployed_compression;

	/*
		time this client was created
	 */
	const u32 m_connection_time;

	friend class AutosendAlgorithm;
	friend struct AutosendCycle;
};

class ClientInterface {
public:

	friend class Server;

	ClientInterface(con::Connection* con);
	~ClientInterface();

	/* run sync step */
	void step(float dtime);

	/* get list of active client id's */
	std::vector<u16> getClientIDs(ClientState min_state=CS_Active);

	/* get list of client player names */
	std::vector<std::string> getPlayerNames();

	/* send message to client */
	void send(u16 peer_id, u8 channelnum, NetworkPacket* pkt, bool reliable);

	/* send to all clients */
	void sendToAll(u16 channelnum, NetworkPacket* pkt, bool reliable);

	/* delete a client */
	void DeleteClient(u16 peer_id);

	/* create client */
	void CreateClient(u16 peer_id);

	/* get a client by peer_id */
	RemoteClient* getClientNoEx(u16 peer_id,  ClientState state_min=CS_Active);

	/* get client by peer_id (make sure you have list lock before!*/
	RemoteClient* lockedGetClientNoEx(u16 peer_id,  ClientState state_min=CS_Active);

	/* get state of client by id*/
	ClientState getClientState(u16 peer_id);

	/* set client playername */
	void setPlayerName(u16 peer_id,std::string name);

	/* get protocol version of client */
	u16 getProtocolVersion(u16 peer_id);

	/* set client version */
	void setClientVersion(u16 peer_id, u8 major, u8 minor, u8 patch, std::string full);

	/* event to update client state */
	void event(u16 peer_id, ClientStateEvent event);

	/* Set environment. Do not call this function if environment is already set */
	void setEnv(ServerEnvironment *env)
	{
		assert(m_env == NULL); // pre-condition
		m_env = env;
	}

	static std::string state2Name(ClientState state);

protected:
	//TODO find way to avoid this functions
	void lock() { m_clients_mutex.lock(); }
	void unlock() { m_clients_mutex.unlock(); }

	std::map<u16, RemoteClient*>& getClientList()
		{ return m_clients; }

private:
	/* update internal player list */
	void UpdatePlayerList();

	// Connection
	con::Connection* m_con;
	Mutex m_clients_mutex;
	// Connected clients (behind the con mutex)
	std::map<u16, RemoteClient*> m_clients;
	std::vector<std::string> m_clients_names; //for announcing masterserver

	// Environment
	ServerEnvironment *m_env;
	Mutex m_env_mutex;

	float m_print_info_timer;

	static const char *statenames[];
};

#endif
