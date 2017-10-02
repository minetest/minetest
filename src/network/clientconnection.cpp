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

#include <asio.hpp>
#include <util/serialize.h>
#include "clientconnection.h"
#include "constants.h"
#include "networkpacket.h"
#include "util/async_task.h"

namespace network {

ClientConnection::ClientConnection(asio::io_service &io_service) :
	ConnectionWorker(io_service, tcp::socket(io_service)),
	m_state(STATE_STARTUP),
	m_io_service(io_service),
	m_udp_socket(io_service, udp::endpoint(udp::v4(), 0))
{
}

ClientConnection::~ClientConnection()
{
	disconnect();
}

bool ClientConnection::connect(const std::string &addr, u16 port)
{
	tcp::resolver resolver(m_io_service);
	try {
		m_endpoint_iterator = resolver.resolve({addr, std::to_string(port)});
	} catch (const asio::system_error &e) {
		std::unique_lock<std::mutex> lock(m_last_error_mtx);
		m_last_error = "Failed to resolve address: "
			+ std::string(e.what());
		return false;
	}

	m_address = addr;
	m_port = port;

	auto self(shared_from_this());
	asio::async_connect(m_socket, m_endpoint_iterator,
		[this, self](std::error_code ec, const tcp::resolver::iterator &) {
			if (!ec) {
				m_state = STATE_CONNECTED;
				readHeader();
			} else {
				std::unique_lock<std::mutex> lock(m_last_error_mtx);
				m_last_error = "Failed to connect to server: " +
					ec.message();
			}
		}
	);

	return true;
}

void ClientConnection::disconnect()
{
	if (m_socket.is_open()) {
		m_socket.close();
	}
}

bool ClientConnection::isConnected() const
{
	return m_socket.is_open();
}

void ClientConnection::readBody()
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
					pkt->putRawPacket(m_packetbuf, PEER_ID_SERVER);
					{
						std::unique_lock<std::mutex> lock(m_recv_queue_mtx);
						m_recv_queue.push(pkt);
					}

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

NetworkPacket *ClientConnection::getNextPacket()
{
	std::unique_lock<std::mutex> lock(m_recv_queue_mtx);
	if (m_recv_queue.empty())
		return nullptr;

	NetworkPacket *pkt = m_recv_queue.front();
	m_recv_queue.pop();
	return pkt;
}

bool ClientConnection::isConnectionInError()
{
	std::unique_lock<std::mutex> lock(m_last_error_mtx);
	return !m_last_error.empty();
}

/**
 * Copy m_last_error
 * Locked by m_last_error_mtx
 *
 * @return m_last_error copy
 */
std::string ClientConnection::getConnectionError()
{
	std::unique_lock<std::mutex> lock(m_last_error_mtx);
	return m_last_error;
}

/**
 * set ClientConnection session id
 * if already set, an assertion is triggered
 *
 * Re-use TCP remote informations to send UDP pings to server (every 5 seconds)
 *
 * This permits to ensure NAT traversal for outgoing and incoming server packets
 * @param session_id
 */
void ClientConnection::setSessionId(session_t session_id)
{
	// we should not set it twice
	assert(m_session_id == 0);

	m_session_id = session_id;

	udp::resolver resolver(m_io_service);

	m_udp_endpoint = udp::endpoint(
		m_socket.remote_endpoint().address(),
		m_socket.remote_endpoint().port()
	);

	auto self(shared_from_this());
	async_task(5000, [this, self] () {
		sendUdpPing();
	});
}

/**
 * UDP ping is a tiny packet
 * - protocol id (4 bytes)
 * - command (1 byte)
 * - session ID (8 bytes)
 */
void ClientConnection::sendUdpPing()
{
	std::vector<u8> buf(sizeof(u32) + sizeof(u8) + sizeof(u64));
	writeU32(&buf[0], PROTOCOL_ID);
	writeU8(&buf[4], UDPCMD_PING);
	writeU64(&buf[5], m_session_id);

	// Send UDP ping asynchronously

	auto self(shared_from_this());
	m_udp_socket.async_send_to(asio::buffer(buf, buf.size()), m_udp_endpoint,
		[this, self] (std::error_code ec, std::size_t /*length*/) {
			// If no error happen, send next ping in 5 seconds
			if (!ec) {
				async_task(5000, [this, self]() {
					sendUdpPing();
				});
			} else {
				disconnect();
			}
		}
	);
}

void ClientConnection::send(NetworkPacket *pkt)
{
	// @TODO handle udp non reliable packets too
	// Don't forget to handle clientCommandFactoryTable[pkt->getCommand()].reliable

	ConnectionWorker::enqueueForSending(pkt);
}

ClientConnectionThread::ClientConnectionThread() :
	Thread("ClientConnectionThread"),
	m_client_connection(std::make_shared<ClientConnection>(m_io_service))
{
}

ClientConnectionThread::~ClientConnectionThread()
{
	stop();
}

void *ClientConnectionThread::run()
{
	m_io_service.run();

	return nullptr;
}

}

