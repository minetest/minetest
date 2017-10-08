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

#include "serverconnection.h"
#include <iostream>
#include <porting.h>
#include <asio/write.hpp>
#include "networkpacket.h"
#include "settings.h"
#include "util/serialize.h"
#include "networkexceptions.h"
#include "sessionchange.h"
#include "server.h"

#define HEADER_LEN 4

namespace network
{

asio::ip::address ServerSession::getAddress()
{
	return m_socket.remote_endpoint().address();
}

void ServerSession::setUDPEndpoint(const udp::endpoint &endpoint)
{
	// Ensure we didn't set the endpoint twice
	assert(!m_udp_endpoint_set);
	m_udp_endpoint = udp::endpoint(endpoint.address(), endpoint.port());
	m_udp_endpoint_set = true;
}

udp::socket& ServerSession::getUDPSocket()
{
	return m_server_con->getUDPSocket();
}

void ServerSession::disconnect()
{
	m_io_service.post([this]() {
		// Close socket and silently ignore if it's already closed
		try {
			if (m_socket.is_open())
				m_socket.close();
		} catch (std::exception &e) {}
	});

	m_server_con->unregisterSession(m_session_id);
}

void ServerSession::pushPacketToQueue()
{
	// Create a new NetworkPacket
	NetworkPacket *pkt = new NetworkPacket();
	pkt->putRawPacket(m_data_buf, m_session_id);
	m_server_con->enqueuePacket(pkt);
}

void ServerSession::notifySocketClosed()
{
	disconnect();
}

ServerConnection::ServerConnection(Server *server, asio::io_service &io_service,
		u16 port) :
	m_io_service(io_service),
	m_tcp_endpoint(g_settings->getBool("ipv6_server") ? tcp::v6() : tcp::v4(), port),
	m_tcp_acceptor(io_service, m_tcp_endpoint),
	m_tcp_socket(io_service),
	m_udp_socket(io_service, udp::endpoint(
		g_settings->getBool("ipv6_server") ? udp::v6() : udp::v4(), port)),
	m_server(server)
{
	m_udp_socket.set_option(asio::socket_base::reuse_address(true));
}

void ServerConnection::start()
{
	do_accept();
}

void ServerConnection::stop()
{
	m_io_service.post([this]() {
		m_tcp_acceptor.close();
		m_udp_socket.close();
	});
	m_sessions.clear();
}

void ServerConnection::do_accept()
{
	auto self(shared_from_this());

	m_tcp_acceptor.async_accept(m_tcp_socket, [this, self](std::error_code ec) {
		if (!ec) {
			try {
				m_tcp_socket.set_option(asio::socket_base::reuse_address(true));
				m_tcp_socket.set_option(asio::socket_base::keep_alive(true));
			} catch (std::exception &e) {
				errorstream << "Failed to set TCP options for peer, ignoring connection."
					<< std::endl;

				// Return in the accept "loop"
				do_accept();
				return;
			}

			ServerSessionPtr serverSession =
				std::make_shared<ServerSession>(m_io_service, std::move(m_tcp_socket), self);

			if (!registerSession(serverSession)) {
				errorstream << "Failed to register session, disconnecting user."
					<< std::endl;
				serverSession->disconnect();
				return;
			}

			serverSession->start();
		}

		do_accept();
	});

	receiveUDPData();
}

void ServerConnection::receiveUDPData()
{
	auto self(shared_from_this());

	// add a callback to receive data
	m_udp_socket.async_receive_from(asio::buffer(m_udp_databuffer, MAX_DATABUF_SIZE),
		m_udp_sender_endpoint, [this, self] (std::error_code ec, std::size_t recv_size) {
			if (!ec) {
				// If UDP header is not 13 bytes long, ignore
				if (recv_size < UDP_HEADER_LEN) {
					return;
				}

				u32 protocol_id = readU32(&m_udp_databuffer[0]);

				// Ignore invalid protocol id
				if (protocol_id != PROTOCOL_ID) {
					return;
				}

				readUDPBody(recv_size);

				receiveUDPData();
			} else {
				errorstream << "Failed to receive UDP packet from ";
				try {
					errorstream << m_udp_socket.remote_endpoint();
				} catch (std::exception &) {
					errorstream << "peer";
				}
				errorstream << ". Error: " << ec.message() << std::endl;
			}
		}
	);
}

void ServerConnection::readUDPBody(std::size_t size)
{
	u8 command = readU8(&m_udp_databuffer[4]);
	u64 session_id = readU64(&m_udp_databuffer[5]);

	auto session_it = m_sessions.find(session_id);
	// Unregistered session, ignore
	if (session_it == m_sessions.end())
		return;

	std::shared_ptr<ServerSession> sessionPtr = session_it->second;

	if (m_udp_sender_endpoint.address() != sessionPtr->getAddress()) {
		errorstream << m_udp_sender_endpoint.address()
			<< "tries to spoof session ID " << session_id << " attached with IP "
			<< sessionPtr->getAddress() << "!!" << std::endl;
		return;
	}

	switch (command) {
		case UDPCMD_PING:
			// Set udp endpoint if not set
			if (!sessionPtr->isUDPEndpointSet())
				sessionPtr->setUDPEndpoint(m_udp_sender_endpoint);
			break;
		case UDPCMD_DATA: {
			// If size < header + payload length + command id, ignore it's invalid
			if (size < UDP_HEADER_LEN + 4 + 2)
				return;

			u32 packet_size = readU32(&m_udp_databuffer[13]);

			std::vector<u8> udp_packetbuf;
			udp_packetbuf.resize(packet_size);
			memcpy(&udp_packetbuf[0], &m_udp_databuffer[17], packet_size);

			// Create a new NetworkPacket
			NetworkPacket *pkt = new NetworkPacket();
			pkt->putRawPacket(udp_packetbuf, session_id);
			enqueuePacket(pkt);
			break;
		}
		default:
			// Ignore invalid commands
			break;
	}
}

void ServerConnection::send(NetworkPacket *pkt, bool reliable)
{
	send(pkt->getPeerId(), pkt, reliable);
}

void ServerConnection::send(session_t session_id, NetworkPacket *pkt, bool reliable)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	auto session_it = m_sessions.find(session_id);

	// Unregistered session, ignore
	if (session_it == m_sessions.end())
		return;

	session_it->second->enqueueForSending(pkt, reliable);
}

void ServerConnection::disconnect(session_t session_id)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	auto session_it = m_sessions.find(session_id);
	// Unregistered session, ignore
	if (session_it == m_sessions.end())
		return;

	session_it->second->disconnect();
}

void ServerConnection::enqueuePacket(NetworkPacket *pkt)
{
	std::unique_lock<std::mutex> lock(m_recv_queue_mtx);
	m_recv_queue.push(pkt);
}

NetworkPacket *ServerConnection::getNextPacket()
{
	std::unique_lock<std::mutex> lock(m_recv_queue_mtx);
	if (m_recv_queue.empty())
		return nullptr;

	NetworkPacket *pkt = m_recv_queue.front();
	m_recv_queue.pop();
	return pkt;
}

asio::ip::address ServerConnection::getListeningAddress() const
{
	return m_tcp_endpoint.address();
}

asio::ip::address ServerConnection::getPeerAddress(session_t session_id)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	auto session_it = m_sessions.find(session_id);

	if (session_it == m_sessions.end()) {
		throw con::PeerNotFoundException(
			std::string("Session " + std::to_string(session_id) + " not found").c_str());
	}

	return session_it->second->getAddress();
}

session_t ServerConnection::registerSession(ServerSessionPtr session)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	// If we have too many sessions, don't allow new client
	if (m_sessions.size() == UINT64_MAX / 2) {
		return 0;
	}

	do {
		if (m_session_id_cursor == UINT64_MAX) {
			m_session_id_cursor = 0;
		}

		m_session_id_cursor++;
	} while (m_sessions.find(m_session_id_cursor) != m_sessions.end());

	m_sessions[m_session_id_cursor] = session;
	session->m_session_id = m_session_id_cursor;

	// Register game session to ClientIface
	m_server_session_mgr->CreateClient(session->m_session_id);

	return m_session_id_cursor;
}

void ServerConnection::unregisterSession(session_t session_id)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	auto session_it = m_sessions.find(session_id);
	if (session_it == m_sessions.end())
		return;

	m_sessions.erase(session_it);

	SessionChange sessionChange(session_id, SessionChange::REMOVED);
	m_server->pushSessionChange(sessionChange);
}

/**
 * When @param port is set to zero, read configuration
 *
 * @param port
 */
ServerConnectionThread::ServerConnectionThread(Server *server, u16 port) :
	Thread("ServerConnectionThread"),
	m_port(port == 0 ? g_settings->getU16("port") : port),
	m_server_connection(std::make_shared<ServerConnection>(server, m_io_service, m_port))
{
}

ServerConnectionThread::~ServerConnectionThread()
{
}

void *ServerConnectionThread::run()
{
	m_server_connection->start();
	while (!stopRequested()) {
		m_io_service.run();
	}
	return nullptr;
}

void ServerConnectionThread::stop()
{
	m_server_connection->stop();
	m_io_service.stop();
	Thread::stop();
}

std::string ServerConnectionThread::getBindAddr() const
{
	return m_server_connection->getListeningAddress().to_string() + ":"
		+ std::to_string(m_port);
}

}
