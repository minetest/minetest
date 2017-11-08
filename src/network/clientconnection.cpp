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
#include "clientopcodes.h"
#include "constants.h"
#include "networkpacket.h"

namespace network {

/*
 * ClientConnection
 */
ClientConnection::ClientConnection(asio::io_service &io_service) :
	ConnectionWorker(io_service, tcp::socket(io_service)),
	m_state(STATE_DISCONNECTED),
	m_io_service(io_service),
	m_udp_socket(io_service, udp::endpoint(udp::v6(), 0)),
	m_udp_ping_thread(new ClientUDPPingThread(this))
{
	m_udp_socket.set_option(asio::socket_base::reuse_address(true));
}

ClientConnection::~ClientConnection()
{
	m_udp_ping_thread->stop();
	m_udp_ping_thread->wait();
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
		[this, self, addr, port](std::error_code ec, const tcp::resolver::iterator &) {
			if (!ec) {
				// Enable TCP keepalives
				m_socket.set_option(asio::socket_base::keep_alive(true));
				m_state = STATE_CONNECTED;
				readHeader();
			} else {
				m_connect_try++;
				// On connection failure retry 2 seconds later
				std::this_thread::sleep_for(std::chrono::seconds(2));
				if (m_connect_try < 3) {
					connect(addr, port);
				} else {
					std::unique_lock<std::mutex> lock(m_last_error_mtx);
					m_last_error = "Failed to connect to server: " +
						ec.message();
					// Reset tries
					m_connect_try = 0;
				}
			}
		}
	);

	return true;
}

void ClientConnection::disconnect()
{
	try {
		m_socket.shutdown(m_socket.shutdown_both);
		m_socket.close();
	} catch (std::exception &e) {}

	try {
		m_udp_socket.shutdown(m_socket.shutdown_both);
		m_udp_socket.close();
	} catch (std::exception &e) {}
	m_state = STATE_DISCONNECTED;
	m_last_error = "Connection lost.";
}

void ClientConnection::notifySocketClosed()
{
	disconnect();
}

bool ClientConnection::isConnected() const
{
	return m_socket.is_open() && m_state >= STATE_CONNECTED;
}

void ClientConnection::pushPacketToQueue()
{
	NetworkPacket *pkt = new NetworkPacket();
	pkt->putRawPacket(m_data_buf, PEER_ID_SERVER);
	std::unique_lock<std::mutex> lock(m_recv_queue_mtx);
	m_recv_queue.push(pkt);
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

#if defined(_WIN32)
	// On Windows if remote endpoint is IPv4, we have a problem with UDP IPv6 opened
	// socket. Close it and re-open it in V4 mode
	if (m_socket.remote_endpoint().address().is_v4()
		&& m_udp_socket.local_endpoint().address().is_v6()) {
		m_udp_socket.close();
		m_udp_socket.open(udp::v4());
		m_udp_socket.bind(udp::endpoint(udp::v4(), 0));
		m_udp_socket.set_option(asio::socket_base::reuse_address(true));
	}
#endif

	m_udp_endpoint = udp::endpoint(
		m_socket.remote_endpoint().address(),
		m_socket.remote_endpoint().port()
	);
	m_udp_endpoint_set = true;

	m_udp_ping_thread->start();

	receiveUDPData();
}

/**
 * UDP ping is a tiny packet
 * - protocol id (4 bytes)
 * - command (1 byte)
 * - session ID (8 bytes)
 */
void ClientConnection::sendUDPPing()
{
	// if session is unknown don't send
	if (m_session_id == 0 || !m_udp_endpoint_set)
		return;

	if (m_udp_sendbuf.size() != UDP_HEADER_LEN + sizeof(s64))
		m_udp_sendbuf.resize(UDP_HEADER_LEN + sizeof(s64));

	writeU32(&m_udp_sendbuf[0], PROTOCOL_ID);
	writeU8(&m_udp_sendbuf[4], UDPCMD_PING);
	writeU64(&m_udp_sendbuf[5], m_session_id);
	writeS64(&m_udp_sendbuf[13], std::time(nullptr));

	// Send UDP ping asynchronously
	auto self(shared_from_this());
	m_udp_socket.async_send_to(asio::buffer(m_udp_sendbuf.data(), m_udp_sendbuf.size()),
		m_udp_endpoint, [this, self] (std::error_code ec, std::size_t /*length*/) {
			// If error happen, disconnect user
			if (ec) {
				infostream << "Failed to send UDP packet to "
					<< m_udp_endpoint << ". Error: " << ec.message() << std::endl;
				disconnect();
			}
		}
	);
}

void ClientConnection::receiveUDPData()
{
	auto self(shared_from_this());

	// add a callback to receive data
	m_udp_socket.async_receive_from(asio::buffer(m_udp_databuffer, MAX_DATABUF_SIZE),
		m_udp_sender_endpoint, [this, self] (std::error_code ec, std::size_t recv_size) {
			if (!ec) {
				// If UDP header is not 13 bytes long, ignore
				if (recv_size < UDP_HEADER_LEN)
					return;

				// if session is unknown ignore incoming packets
				// they are rogue packets
				if (m_session_id == 0 || !m_udp_endpoint_set)
					return;

				u32 protocol_id = readU32(&m_udp_databuffer[0]);

				// Ignore invalid protocol id
				if (protocol_id != PROTOCOL_ID) {
					return;
				}

				readUDPBody(recv_size);

				receiveUDPData();
			} else {
				infostream << "Failed to receive UDP packet from ";
				try {
					infostream << m_udp_socket.remote_endpoint();
				} catch (std::exception &) {
					infostream << "peer";
				}
				infostream << ". Error: " << ec.message() << std::endl;
			}
		}
	);
}

static asio::ip::address ip4toip6mapped(const std::string &ip)
{
	asio::ip::address a = asio::ip::address::from_string(ip);
	if (a.is_v4())
		return asio::ip::address_v6::v4_mapped(a.to_v4());

	return a;
}

void ClientConnection::readUDPBody(std::size_t size)
{
	u8 command = readU8(&m_udp_databuffer[4]);
	u64 session_id = readU64(&m_udp_databuffer[5]);

	asio::ip::address remote_addr = m_udp_endpoint.address();
	asio::ip::address incoming_addr = m_udp_sender_endpoint.address();

	// If incoming address is IPv6 and we talk to IPv4, try to convert remote to v6
	// because incoming 127.0.0.1 is represented as :ffff:127.0.0.1 in singleplayer
	if (incoming_addr.is_v6() && remote_addr.is_v4()) {
		remote_addr = ip4toip6mapped(remote_addr.to_string());
	}

	if (incoming_addr != remote_addr) {
		errorstream << remote_addr << " tries to spoof session ID " << session_id
			<< " attached with IP " << remote_addr << "!!" << std::endl;
		return;
	}

	switch (command) {
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
			pkt->putRawPacket(udp_packetbuf, PEER_ID_SERVER);
			{
				std::unique_lock<std::mutex> lock(m_recv_queue_mtx);
				m_recv_queue.push(pkt);
			}
			break;
		}
		default:
			// Ignore invalid commands
			break;
	}
}

void ClientConnection::send(NetworkPacket *pkt)
{
	bool reliable = serverCommandFactoryTable[pkt->getCommand()].reliable;
	ConnectionWorker::enqueueForSending(pkt, reliable);
}

ClientConnectionThread::ClientConnectionThread() :
	Thread("ClientConnectionThread"),
	m_client_connection(std::make_shared<ClientConnection>(m_io_service))
{
}

ClientConnectionThread::~ClientConnectionThread()
{
}

void ClientConnectionThread::stop()
{
	m_io_service.post([this] {
		m_client_connection->disconnect();
	});
	m_io_service.stop();
	Thread::stop();
}

void *ClientConnectionThread::run()
{
	while (!stopRequested()) {
		m_io_service.run_one();
	}
	return nullptr;
}

void* ClientUDPPingThread::run()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (!stopRequested()) {
		m_client_connection->sendUDPPing();
		m_sem.wait(5 * 1000);
	}

	return nullptr;
}

}

