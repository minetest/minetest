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
#include <unordered_map>
#include <queue>
#include "connection.h"
#include "threading/thread.h"

class Server;
class ClientIface;

using asio::ip::tcp;
using asio::ip::udp;

namespace network
{

class ServerConnection;

class ServerSession : public ConnectionWorker
{
	friend class ServerConnection;
public:
	ServerSession(asio::io_service &io_service,
			tcp::socket socket, std::shared_ptr<ServerConnection> serverCon) :
		ConnectionWorker(io_service, std::move(socket)),
		m_udp_endpoint_set(false),
		m_server_con(serverCon)
	{}

	~ServerSession()
	{
		disconnect();
	}

	void start()
	{
		readHeader();
	}

	void disconnect();

	asio::ip::address getAddress();

	void setUDPEndpoint(const udp::endpoint &endpoint);

	bool isUDPEndpointSet() const
	{
		return m_udp_endpoint_set;
	}

	udp::socket &getUDPSocket();
private:
	void pushPacketToQueue();

	std::atomic_bool m_udp_endpoint_set;
	std::shared_ptr<ServerConnection> m_server_con;
};

typedef std::shared_ptr<ServerSession> ServerSessionPtr;

class ServerConnection : public std::enable_shared_from_this<ServerConnection>
{
public:
	ServerConnection(Server *server, asio::io_service &io_service, u16 port);

	virtual void send(NetworkPacket *pkt, bool reliable);
	void send(session_t session_id, NetworkPacket *pkt, bool reliable);

	void disconnect(session_t session_id);

	void enqueuePacket(NetworkPacket *pkt);
	NetworkPacket *getNextPacket();

	asio::ip::address getListeningAddress() const;
	asio::ip::address getPeerAddress(session_t session_id);

	void start();
	void stop();

	void unregisterSession(session_t session_id);

	void setClientIface(ClientIface *sessMgr)
	{
		m_server_session_mgr = sessMgr;
	}

	udp::socket &getUDPSocket()
	{
		return m_udp_socket;
	}
private:
	void do_accept();
	void receiveUDPData();
	session_t registerSession(ServerSessionPtr session);

	void readUDPBody(std::size_t size);

	asio::io_service &m_io_service;

	tcp::endpoint m_tcp_endpoint;
	tcp::acceptor m_tcp_acceptor;
	tcp::socket m_tcp_socket;

	udp::endpoint m_udp_sender_endpoint;
	udp::socket m_udp_socket;

	std::array<u8, MAX_DATABUF_SIZE> m_udp_databuffer;
	session_t m_session_id_cursor = 0;

	std::recursive_mutex m_sessions_mtx;
	std::unordered_map<session_t, ServerSessionPtr> m_sessions;

	std::mutex m_recv_queue_mtx;
	std::queue<NetworkPacket *> m_recv_queue;

	Server *m_server;
	ClientIface *m_server_session_mgr = nullptr;

	static const uint8_t UDP_HEADER_LEN = 13;
};

class ServerConnectionThread : public Thread
{
public:
	ServerConnectionThread(Server *server, u16 port = 0);
	~ServerConnectionThread();

	void stop();

	std::shared_ptr<ServerConnection> get_connection()
	{
		return m_server_connection;
	}

	std::string getBindAddr() const;
	u16 getPort() const
	{
		return m_port;
	}
protected:
	void *run();
private:
	u16 m_port;
	asio::io_service m_io_service;
	std::shared_ptr<ServerConnection> m_server_connection;
};

}

