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
#include <asio/ip/udp.hpp>
#include <utility>
#include "threading/thread.h"
#include "irrlichttypes.h"
#include "connection.h"
#include "networkprotocol.h"

using asio::ip::tcp;
using asio::ip::udp;

class NetworkPacket;

namespace network
{

class ClientUDPPingThread;

class ClientConnection : public ConnectionWorker, private UDPConnection
{
public:
	ClientConnection(asio::io_service &io_service);
	~ClientConnection();

	void disconnect();
	virtual void send(NetworkPacket *pkt);
	bool connect(const std::string &addr, u16 port);
	bool isConnected() const;
	NetworkPacket *getNextPacket();
	u16 getServerPort() const { return m_port; }

	const std::string &getServerAddress() const { return m_address; }

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
		STATE_DISCONNECTED,
		STATE_CONNECTED,
		STATE_STARTED,
	};

	void setState(State s) { m_state = s; }

	State getState() const { return m_state; }

	void setSessionId(session_t session_id);

	void sendUDPPing();

private:
	void pushPacketToQueue();
	void notifySocketClosed();
	void receiveUDPData();
	void readUDPBody(std::size_t size);

	udp::socket &getUDPSocket() { return m_udp_socket; }

	std::atomic<State> m_state;

	std::string m_address;
	u16 m_port = 0;

	std::mutex m_last_error_mtx;
	std::string m_last_error;

	asio::io_service &m_io_service;
	tcp::resolver::iterator m_endpoint_iterator;

	udp::endpoint m_udp_sender_endpoint;
	udp::socket m_udp_socket;

	std::array<u8, MAX_DATABUF_SIZE> m_udp_databuffer;

	std::mutex m_recv_queue_mtx;
	std::queue<NetworkPacket *> m_recv_queue;

	std::unique_ptr<ClientUDPPingThread> m_udp_ping_thread;

	u8 m_connect_try = 0;
};

class ClientUDPPingThread : public Thread
{
public:
	ClientUDPPingThread(ClientConnection *conn)
	    : Thread("ClientUDPPingThread"), m_client_connection(conn)
	{
	}

protected:
	void *run();

private:
	ClientConnection *m_client_connection;
};

class ClientConnectionThread : public Thread
{
public:
	ClientConnectionThread();
	~ClientConnectionThread();

	std::shared_ptr<ClientConnection> get_connection() { return m_client_connection; }

	void stop();
protected:
	void *run();

private:
	// This must be initialized before m_client_connection
	asio::io_service m_io_service;
	std::shared_ptr<ClientConnection> m_client_connection;
};
}
