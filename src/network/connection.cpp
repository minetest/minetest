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

#include <asio/buffer.hpp>
#include <util/serialize.h>
#include <asio/write.hpp>
#include "connection.h"
#include "networkpacket.h"

namespace network
{
void ConnectionWorker::enqueueForSending(NetworkPacket *pkt)
{
	// Write packet to buffer
	std::shared_ptr<std::vector<u8>> write_buf = std::make_shared<std::vector<u8>>();
	write_buf->resize(pkt->getSize() + 2 + 4);
	writeU32(&(*write_buf)[0], pkt->getSize() + 2);
	writeU16(&(*write_buf)[4], pkt->getCommand());
	memcpy(&(*write_buf)[6], pkt->getU8Ptr(0), pkt->getSize());

	m_io_service.post([this, write_buf]() {
			bool write_in_progress = !m_send_queue.empty();
			m_send_queue.push_back(write_buf);
			if (!write_in_progress) {
				sendPacket();
			}
		}
	);
}

void ConnectionWorker::sendPacket()
{

	asio::async_write(m_socket, asio::buffer(m_send_queue.front()->data(),
		m_send_queue.front()->size()),
		[this](std::error_code ec, std::size_t /*length*/) {
			m_send_queue.pop_front();

			if (!ec) {
				if (!m_send_queue.empty()) {
					sendPacket();
				}
			} else {
				disconnect();
			}
		}
	);
}

void ConnectionWorker::readHeader()
{
	auto self(shared_from_this());
	m_socket.async_read_some(asio::buffer(m_hdr_buf, HEADER_LEN),
		[this, self](std::error_code ec, std::size_t length) {
			if (!ec) {
				m_packet_waited_len = readU32(&m_hdr_buf[0]);
				if (m_packet_waited_len == 0) {
					disconnect();
					return;
				}

				// New header, prepare packet buffer
				m_packetbuf.clear();
				m_packetbuf.resize(m_packet_waited_len);
				m_packetbuf_cursor = 0;

				readBody();
			} else {
				disconnect();
			}
		}
	);
}
}
