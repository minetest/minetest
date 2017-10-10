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

#include <atomic>
#include <vector>
#include <mutex>
#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#include "irrlichttypes.h"
#include "networkprotocol.h"

using asio::ip::tcp;
using asio::ip::udp;

class NetworkPacket;

#define MAX_DATABUF_SIZE 4096

namespace network
{

enum UdpCommandId : u8
{
	UDPCMD_PING = 1,
	UDPCMD_DATA = 2,
};

struct SendBuffer
{
	std::vector<u8> data;
	bool reliable = true;
};

typedef std::shared_ptr<SendBuffer> SendBufferPtr;

class ConnectionWorker : public std::enable_shared_from_this<ConnectionWorker>
{
public:
	ConnectionWorker(asio::io_service &io_service, tcp::socket socket) :
		m_io_service(io_service),
		m_socket(std::move(socket)),
		m_udp_endpoint_set(false)
	{}
	virtual ~ConnectionWorker() {}

	virtual void disconnect() = 0;
protected:
	void readHeader();
	virtual void readBody();

	virtual void pushPacketToQueue() = 0;
	virtual void notifySocketClosed() = 0;

	virtual udp::socket &getUDPSocket() = 0;

	void enqueueForSending(NetworkPacket *pkt, bool reliable);
	void sendPacket();

	static const uint8_t HEADER_LEN = 8;
	static const uint8_t UDP_HEADER_LEN = 13;
	// 100MB is the maximum allowed packet size
	static constexpr const uint32_t MAX_ALLOWED_PACKET_SIZE = 100 * 1024 * 1024;

	asio::io_service &m_io_service;

	tcp::socket m_socket;

	udp::endpoint m_udp_endpoint;
	std::atomic_bool m_udp_endpoint_set;

	session_t m_session_id = 0;

	// Data buffers when reading from asio sockets
	std::array<u8, HEADER_LEN> m_hdr_buf{};

	// raw packet
	uint32_t m_packet_waited_len = 0;
	std::vector<u8> m_data_buf;
	// writing cursor when reading data chunks
	uint32_t m_read_cursor = 0;

	// Write buffers (sending)
	std::deque<SendBufferPtr> m_send_queue;
	std::recursive_mutex m_send_queue_mtx;
};

class UDPConnection
{
protected:
	std::vector<u8> m_udp_sendbuf;
};

}
