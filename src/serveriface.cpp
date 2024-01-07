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

#include <sstream>
#include "serveriface.h"
#include "network/connection.h"
#include "network/clientopcodes.h"
#include "network/networkpacket.h"
#include "settings.h"
#include "emerge.h"
#include "log.h"
#include "util/srp.h"
#include "face_position_cache.h"

RemoteServer::RemoteServer(session_t peer_id_, const Address &address) :
	peer_id(peer_id_),
	m_addr(address)
{
}

/*
void RemoteServer::resetChosenMech()
{
	if (auth_data) {
		srp_verifier_delete((SRPVerifier *) auth_data);
		auth_data = nullptr;
	}
	chosen_mech = AUTH_MECHANISM_NONE;
}
*/

u64 RemoteServer::uptime() const
{
	return porting::getTimeS() - m_connection_time;
}

ServerInterface::ServerInterface(const std::shared_ptr<con::Connection> & con)
:
	m_con(con)
{

}
ServerInterface::~ServerInterface()
{
	/*
		Delete servers
	*/
	{
		RecursiveMutexAutoLock serverslock(m_servers_mutex);

		for (auto &server_it : m_servers) {
			// Delete server
			delete server_it.second;
		}
	}
}

std::vector<session_t> ServerInterface::getServerIDs()
{
	std::vector<session_t> reply;
	RecursiveMutexAutoLock serverslock(m_servers_mutex);

	for (const auto &m_server : m_servers) {
		reply.push_back(m_server.second->peer_id);
	}

	return reply;
}

void ServerInterface::send(session_t peer_id, u8 channelnum,
		NetworkPacket *pkt, bool reliable)
{
	m_con->Send(peer_id, channelnum, pkt, reliable);
}

void ServerInterface::sendToAll(NetworkPacket *pkt)
{
	RecursiveMutexAutoLock serverslock(m_servers_mutex);
	for (auto &server_it : m_servers) {
		RemoteServer *server = server_it.second;

		m_con->Send(server->peer_id,
				serverCommandFactoryTable[pkt->getCommand()].channel, pkt,
				serverCommandFactoryTable[pkt->getCommand()].reliable);
	}
}

RemoteServer* ServerInterface::getServerNoEx(session_t peer_id)
{
	RecursiveMutexAutoLock serverslock(m_servers_mutex);
	RemoteServerMap::const_iterator n = m_servers.find(peer_id);
	// The server may not exist; servers are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_servers.end())
		return NULL;

	return n->second;
}

RemoteServer* ServerInterface::lockedGetServerNoEx(session_t peer_id)
{
	RemoteServerMap::const_iterator n = m_servers.find(peer_id);
	// The server may not exist; servers are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_servers.end())
		return NULL;

	return n->second;
}

void ServerInterface::DeleteServer(session_t peer_id)
{
	RecursiveMutexAutoLock conlock(m_servers_mutex);

	// Error check
	RemoteServerMap::iterator n = m_servers.find(peer_id);
	// The server may not exist; servers are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_servers.end())
		return;

	// Delete server
	delete m_servers[peer_id];
	m_servers.erase(peer_id);
}

void ServerInterface::CreateServer(session_t peer_id, const Address &address)
{
	RecursiveMutexAutoLock conlock(m_servers_mutex);

	// Error check
	RemoteServerMap::iterator n = m_servers.find(peer_id);
	// The server shouldn't already exist
	if (n != m_servers.end()) return;

	// Create server
	RemoteServer *server = new RemoteServer(peer_id, address);
	m_servers[server->peer_id] = server;
}

