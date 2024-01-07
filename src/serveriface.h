/*
Minetest
Copyright (C) 2024 SFENCE <sfence.software@gmail.com>

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

#include "network/address.h"
#include "network/connection.h"
#include "threading/mutex_auto_lock.h"

#include <list>
#include <vector>
#include <set>
#include <unordered_set>
#include <memory>
#include <mutex>

class RemoteServer
{
public:
	// peer_id=0 means this server has no associated peer
	// NOTE: If server is made allowed to exist while peer doesn't,
	//       this has to be set to 0 when there is no peer.
	//       Also, the server must be moved to some other container.
	session_t peer_id = PEER_ID_INEXISTENT;

	/* Authentication information */
	/*
	std::string enc_pwd = "";
	bool create_player_on_auth_success = false;
	AuthMechanism chosen_mech  = AUTH_MECHANISM_NONE;
	void *auth_data = nullptr;
	u32 allowed_auth_mechs = 0;
	u32 allowed_sudo_mechs = 0;

	void resetChosenMech();
	*/


	RemoteServer() = default;
	RemoteServer(session_t peer_id, const Address &address);
	~RemoteServer() = default;

	/* get uptime */
	u64 uptime() const;

	const Address &getAddress() const { return m_addr; }
private:
	// Cached here so retrieval doesn't have to go to connection API
	Address m_addr;

	/*
		time this server was created
	 */
	const u64 m_connection_time = porting::getTimeS();
};

typedef std::unordered_map<u16, RemoteServer*> RemoteServerMap;

class ServerInterface {
public:

	friend class Server;

	ServerInterface(const std::shared_ptr<con::Connection> &con);
	~ServerInterface();

	/* get list of active server id's */
	std::vector<session_t> getServerIDs();

	/* send message to server */
	void send(session_t peer_id, u8 channelnum, NetworkPacket *pkt, bool reliable);

	/* send to all servers */
	void sendToAll(NetworkPacket *pkt);

	/* delete a server */
	void DeleteServer(session_t peer_id);

	/* create server */
	void CreateServer(session_t peer_id, const Address &address);

	/* get a server by peer_id */
	RemoteServer *getServerNoEx(session_t peer_id);

	/* get server by peer_id (make sure you have list lock before!*/
	RemoteServer *lockedGetServerNoEx(session_t peer_id);

protected:
	class AutoLock {
	public:
		AutoLock(ServerInterface &iface): m_lock(iface.m_servers_mutex) {}

	private:
		RecursiveMutexAutoLock m_lock;
	};

	RemoteServerMap& getServerList() { return m_servers; }

private:
	// Connection
	std::shared_ptr<con::Connection> m_con;
	std::recursive_mutex m_servers_mutex;
	// Connected servers (behind the con mutex)
	RemoteServerMap m_servers;
};
