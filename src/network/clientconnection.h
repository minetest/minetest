/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include "threading/thread.h"
#include "irrlichttypes.h"
#include "connection.h"
#include "networkprotocol.h"

using asio::ip::tcp;

class NetworkPacket;

namespace network
{

class ClientConnection : public ConnectionWorker
{
public:
	ClientConnection(asio::io_service &io_service);
	~ClientConnection();

	void disconnect();
	virtual void send(NetworkPacket *pkt);
	bool connect(const std::string &addr, u16 port);
	bool isConnected() const;
	NetworkPacket *getNextPacket();
	u16 getServerPort() const
	{
		return m_port;
	}

	const std::string &getServerAddress() const
	{
		return m_address;
	}

	bool isConnectionInError();
	std::string getConnectionError();

	/**
	 * Connection state
	 * startup: disconnected, will connect
	 * connected: socket opened
	 * started: client ready in game
	 */
	enum State : u8
	{
		STATE_STARTUP,
		STATE_CONNECTED,
		STATE_STARTED,
	};

	void setState(State s)
	{
		m_state = s;
	}

	State getState() const
	{
		return m_state;
	}

	void setSessionId(session_t session_id);
private:
	void readBody();

	std::atomic<State> m_state;

	std::string m_address;
	u16 m_port = 0;

	session_t m_session_id = 0;

	std::mutex m_last_error_mtx;
	std::string m_last_error;

	asio::io_service &m_io_service;
	tcp::resolver::iterator m_endpoint_iterator;

	std::mutex m_recv_queue_mtx;
	std::queue<NetworkPacket *> m_recv_queue;
};

class ClientConnectionThread : public Thread
{
public:
	ClientConnectionThread();
	~ClientConnectionThread();

	std::shared_ptr<ClientConnection> get_connection()
	{
		return m_client_connection;
	}

protected:
	void *run();
private:
	// This must be initialized before m_client_connection
	asio::io_service m_io_service;
	std::shared_ptr<ClientConnection> m_client_connection;
};

}
