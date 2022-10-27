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

#pragma once

#include "irr_v3d.h"                   // for irrlicht datatypes

#include "constants.h"
#include "serialization.h"             // for SER_FMT_VER_INVALID
#include "network/networkpacket.h"
#include "network/networkprotocol.h"
#include "network/address.h"
#include "porting.h"
#include "threading/mutex_auto_lock.h"

#include <list>
#include <vector>
#include <set>
#include <memory>
#include <mutex>

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
               ----------------------------------------
                                                      v
                                       +-----------------------------+
                                       |IN:                          |
                                       | TOSERVER_INIT               |
                                       +-----------------------------+
                                                      | invalid playername
                                                      | or denied by mod
                                                      v
                                       +-----------------------------+
                                       |OUT:                         |
                                       | TOCLIENT_HELLO              |
                                       +-----------------------------+
                                                      |
                                                      |
                                                      v
      /-----------------\                    /-----------------\
      |                 |                    |                 |
      |  AwaitingInit2  |<---------          |    HelloSent    |
      |                 |         |          |                 |
      \-----------------/         |          \-----------------/
               |                  |                   |
+-----------------------------+   |    *-----------------------------*     Auth fails
|IN:                          |   |    |Authentication, depending on |------------------
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
      \-----------------/             |                             |                  |
               |                      +-----------------------------+                  |
               |      ^                           |                                    |
               |      -----------------------------                                    |
               v                                                                       v
+-----------------------------+                        --------------------------------+
|IN:                          |                        |                               ^
| TOSERVER_CLIENT_READY       |                        v                               |
+-----------------------------+            +------------------------+                  |
               |                           |OUT:                    |                  |
               v                           | TOCLIENT_ACCESS_DENIED |                  |
+-----------------------------+            +------------------------+                  |
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
	CSE_GotInit2,
	CSE_SetDenied,
	CSE_SetDefinitionsSent,
	CSE_SetClientReady,
	CSE_SudoSuccess,
	CSE_SudoLeave,
	CSE_Disconnect
};

/*
	Used for queueing and sorting block transfers in containers

	Lower priority number means higher priority.
*/
struct PrioritySortedBlockTransfer
{
	PrioritySortedBlockTransfer(float a_priority, const v3s16 &a_pos, session_t a_peer_id)
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
	session_t peer_id;
};

class RemoteClient
{
public:
	// peer_id=0 means this client has no associated peer
	// NOTE: If client is made allowed to exist while peer doesn't,
	//       this has to be set to 0 when there is no peer.
	//       Also, the client must be moved to some other container.
	session_t peer_id = PEER_ID_INEXISTENT;
	// The serialization version to use with the client
	u8 serialization_version = SER_FMT_VER_INVALID;
	//
	u16 net_proto_version = 0;

	/* Authentication information */
	std::string enc_pwd = "";
	bool create_player_on_auth_success = false;
	AuthMechanism chosen_mech  = AUTH_MECHANISM_NONE;
	void *auth_data = nullptr;
	u32 allowed_auth_mechs = 0;
	u32 allowed_sudo_mechs = 0;

	void resetChosenMech();

	bool isSudoMechAllowed(AuthMechanism mech)
	{ return allowed_sudo_mechs & mech; }
	bool isMechAllowed(AuthMechanism mech)
	{ return allowed_auth_mechs & mech; }

	RemoteClient();
	~RemoteClient() = default;

	/*
		Finds block that should be sent next to the client.
		Environment should be locked when this is called.
		dtime is used for resetting send radius at slow interval
	*/
	void GetNextBlocks(ServerEnvironment *env, EmergeManager* emerge,
			float dtime, std::vector<PrioritySortedBlockTransfer> &dest);

	void GotBlock(v3s16 p);

	void SentBlock(v3s16 p);

	void SetBlockNotSent(v3s16 p);
	void SetBlocksNotSent(std::map<v3s16, MapBlock*> &blocks);

	/**
	 * tell client about this block being modified right now.
	 * this information is required to requeue the block in case it's "on wire"
	 * while modification is processed by server
	 * @param p position of modified block
	 */
	void ResendBlockIfOnWire(v3s16 p);

	u32 getSendingCount() const { return m_blocks_sending.size(); }

	bool isBlockSent(v3s16 p) const
	{
		return m_blocks_sent.find(p) != m_blocks_sent.end();
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
	float m_time_from_building = 9999;

	/*
		List of active objects that the client knows of.
	*/
	std::set<u16> m_known_objects;

	ClientState getState() const { return m_state; }

	std::string getName() const { return m_name; }

	void setName(const std::string &name) { m_name = name; }

	/* update internal client state */
	void notifyEvent(ClientStateEvent event);

	/* set expected serialization version */
	void setPendingSerializationVersion(u8 version)
		{ m_pending_serialization_version = version; }

	void setDeployedCompressionMode(u16 byteFlag)
		{ m_deployed_compression = byteFlag; }

	void confirmSerializationVersion()
		{ serialization_version = m_pending_serialization_version; }

	/* get uptime */
	u64 uptime() const;

	/* set version information */
	void setVersionInfo(u8 major, u8 minor, u8 patch, const std::string &full)
	{
		m_version_major = major;
		m_version_minor = minor;
		m_version_patch = patch;
		m_full_version = full;
	}

	/* read version information */
	u8 getMajor() const { return m_version_major; }
	u8 getMinor() const { return m_version_minor; }
	u8 getPatch() const { return m_version_patch; }
	const std::string &getFullVer() const { return m_full_version; }

	void setLangCode(const std::string &code) { m_lang_code = code; }
	const std::string &getLangCode() const { return m_lang_code; }

	void setCachedAddress(const Address &addr) { m_addr = addr; }
	const Address &getAddress() const { return m_addr; }

private:
	// Version is stored in here after INIT before INIT2
	u8 m_pending_serialization_version = SER_FMT_VER_INVALID;

	/* current state of client */
	ClientState m_state = CS_Created;

	// Cached here so retrieval doesn't have to go to connection API
	Address m_addr;

	// Client sent language code
	std::string m_lang_code;

	/*
		Blocks that have been sent to client.
		- These don't have to be sent again.
		- A block is cleared from here when client says it has
		  deleted it from it's memory

		List of block positions.
		No MapBlock* is stored here because the blocks can get deleted.
	*/
	std::set<v3s16> m_blocks_sent;
	s16 m_nearest_unsent_d = 0;
	v3s16 m_last_center;
	v3f m_last_camera_dir;

	const u16 m_max_simul_sends;
	const float m_min_time_from_building;
	const s16 m_max_send_distance;
	const s16 m_block_optimize_distance;
	const s16 m_max_gen_distance;
	const bool m_occ_cull;

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
		Blocks that have been modified since blocks were
		sent to the client last (getNextBlocks()).
		This is used to reset the unsent distance, so that
		modified blocks are resent to the client.

		List of block positions.
	*/
	std::set<v3s16> m_blocks_modified;

	/*
		Count of excess GotBlocks().
		There is an excess amount because the client sometimes
		gets a block so late that the server sends it again,
		and the client then sends two GOTBLOCKs.
		This is reset by PrintInfo()
	*/
	u32 m_excess_gotblocks = 0;

	// CPU usage optimization
	float m_nothing_to_send_pause_timer = 0.0f;

	/*
		name of player using this client
	*/
	std::string m_name = "";

	/*
		client information
	*/
	u8 m_version_major = 0;
	u8 m_version_minor = 0;
	u8 m_version_patch = 0;

	std::string m_full_version = "unknown";

	u16 m_deployed_compression = 0;

	/*
		time this client was created
	 */
	const u64 m_connection_time = porting::getTimeS();
};

typedef std::unordered_map<u16, RemoteClient*> RemoteClientMap;

class ClientInterface {
public:

	friend class Server;

	ClientInterface(const std::shared_ptr<con::Connection> &con);
	~ClientInterface();

	/* run sync step */
	void step(float dtime);

	/* get list of active client id's */
	std::vector<session_t> getClientIDs(ClientState min_state=CS_Active);

	/* mark block as not sent to active client sessions */
	void markBlockposAsNotSent(const v3s16 &pos);

	/* verify is server user limit was reached */
	bool isUserLimitReached();

	/* get list of client player names */
	const std::vector<std::string> &getPlayerNames() const { return m_clients_names; }

	/* send message to client */
	void send(session_t peer_id, u8 channelnum, NetworkPacket *pkt, bool reliable);

	/* send to all clients */
	void sendToAll(NetworkPacket *pkt);

	/* delete a client */
	void DeleteClient(session_t peer_id);

	/* create client */
	void CreateClient(session_t peer_id);

	/* get a client by peer_id */
	RemoteClient *getClientNoEx(session_t peer_id,  ClientState state_min = CS_Active);

	/* get client by peer_id (make sure you have list lock before!*/
	RemoteClient *lockedGetClientNoEx(session_t peer_id,  ClientState state_min = CS_Active);

	/* get state of client by id*/
	ClientState getClientState(session_t peer_id);

	/* set client playername */
	void setPlayerName(session_t peer_id, const std::string &name);

	/* get protocol version of client */
	u16 getProtocolVersion(session_t peer_id);

	/* set client version */
	void setClientVersion(session_t peer_id, u8 major, u8 minor, u8 patch,
			const std::string &full);

	/* event to update client state */
	void event(session_t peer_id, ClientStateEvent event);

	/* Set environment. Do not call this function if environment is already set */
	void setEnv(ServerEnvironment *env)
	{
		assert(m_env == NULL); // pre-condition
		m_env = env;
	}

	static std::string state2Name(ClientState state);
protected:
	class AutoLock {
	public:
		AutoLock(ClientInterface &iface): m_lock(iface.m_clients_mutex) {}

	private:
		RecursiveMutexAutoLock m_lock;
	};

	RemoteClientMap& getClientList() { return m_clients; }

private:
	/* update internal player list */
	void UpdatePlayerList();

	// Connection
	std::shared_ptr<con::Connection> m_con;
	std::recursive_mutex m_clients_mutex;
	// Connected clients (behind the con mutex)
	RemoteClientMap m_clients;
	std::vector<std::string> m_clients_names; //for announcing masterserver

	// Environment
	ServerEnvironment *m_env;

	float m_print_info_timer;

	static const char *statenames[];
};
