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

void ServerSession::disconnect()
{
	if (m_socket.is_open())
		m_socket.close();

	m_server_con->unregisterSession(m_session_id);
}

void ServerSession::send(NetworkPacket *pkt)
{
	// @TODO handle udp non reliable packets too
	// Don't forget to handle serverCommandFactoryTable[pkt->getCommand()].reliable

	ConnectionWorker::enqueueForSending(pkt);
}

void ServerSession::readBody()
{
	auto self(shared_from_this());

	size_t bytes_to_read = std::min<size_t>(m_packet_waited_len - m_packetbuf_cursor,
		MAX_DATABUF_SIZE);

	m_socket.async_read_some(asio::buffer(m_data_buf, bytes_to_read),
		[this, self](std::error_code ec, std::size_t length) {
			if (!ec) {
				memcpy(&m_packetbuf[m_packetbuf_cursor], &m_data_buf[0], length);
				m_packetbuf_cursor += length;

				// The whole packet is read
				if (m_packetbuf_cursor == m_packet_waited_len) {
					// Create a new NetworkPacket
					NetworkPacket *pkt = new NetworkPacket();
					pkt->putRawPacket(m_packetbuf, m_session_id);
					m_server_con->enqueuePacket(pkt);
					// Read next packet
					readHeader();
				// We read more data than needed, it's not normal, disconnect user
				} else if (m_packetbuf_cursor > m_packet_waited_len) {
					disconnect();
				} else {
					// Else we need to read the next chunk
					readBody();
				}

			} else {
				disconnect();
			}
		}
	);
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
}

void ServerConnection::start()
{
	do_accept();
}

void ServerConnection::stop()
{
	m_tcp_acceptor.close();
}

void ServerConnection::do_accept()
{
	auto self(shared_from_this());
	m_tcp_acceptor.async_accept(m_tcp_socket, [this, self](std::error_code ec) {
		if (!ec) {
			ServerSessionPtr serverSession =
				std::make_shared<ServerSession>(m_io_service, std::move(m_tcp_socket), self);

			if (!registerSession(serverSession)) {
				errorstream << "Failed to register session" << std::endl;
				serverSession->disconnect();
				return;
			}

			serverSession->start();
		}

		do_accept();
	});

	// Udp socket need address re-use
	asio::socket_base::reuse_address option(true);
	m_udp_socket.set_option(option);
	m_udp_socket.async_receive_from(asio::buffer(m_udp_databuffer, HEADER_LEN),
		m_udp_sender_endpoint, [] (std::error_code ec, std::size_t sent) {
			//do_receive();
		}
	);
}

void ServerConnection::send(NetworkPacket *pkt)
{
	send(pkt->getPeerId(), pkt);
}

void ServerConnection::send(session_t session_id, NetworkPacket *pkt)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	auto session_it = m_sessions.find(session_id);

	// Unregistered session, ignore
	if (session_it == m_sessions.end())
		return;

	session_it->second->enqueueForSending(pkt);
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
	if (!sessionRegistered(session_id)) {
		throw con::PeerNotFoundException(
			std::string("Session " + std::to_string(session_id) + " not found").c_str());
	}

	// @TODO
	return asio::ip::address();
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

bool ServerConnection::sessionRegistered(session_t session_id)
{
	std::unique_lock<std::recursive_mutex> lock(m_sessions_mtx);
	return m_sessions.find(session_id) != m_sessions.end();
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
	stop();
}

void *ServerConnectionThread::run()
{
	m_server_connection->start();
	m_io_service.run();

	return nullptr;
}

void ServerConnectionThread::stop()
{
	m_server_connection->stop();

	if (!m_io_service.stopped())
		m_io_service.stop();

	Thread::stop();
}

std::string ServerConnectionThread::getBindAddr() const
{
	return m_server_connection->getListeningAddress().to_string() + ":"
		+ std::to_string(m_port);
}

}
