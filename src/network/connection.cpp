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
void ConnectionWorker::enqueueForSending(NetworkPacket *pkt, bool reliable)
{
	// Write packet to buffer
	SendBufferPtr write_buf = std::make_shared<SendBuffer>();
	write_buf->reliable = reliable;

	if (reliable) {
		write_buf->data.resize(pkt->getSize() + 2 + 4);
		writeU32(&(write_buf->data)[0], pkt->getSize() + 2); // total payload size
		writeU16(&(write_buf->data)[4], pkt->getCommand()); // command
		memcpy(&(write_buf->data)[6], pkt->getU8Ptr(0), pkt->getSize()); // payload
	}
	else {
		// Session id not already inited, ignore sending
		if (m_session_id == 0)
			return;

		write_buf->data.resize(pkt->getSize() + 2 + 4 + UDP_HEADER_LEN);
		writeU32(&(write_buf->data)[0], PROTOCOL_ID);
		writeU8(&(write_buf->data)[4], UDPCMD_DATA);
		writeU64(&(write_buf->data)[5], m_session_id);
		writeU32(&(write_buf->data)[13], pkt->getSize() + 2);
		writeU16(&(write_buf->data)[17], pkt->getCommand());
		memcpy(&(write_buf->data)[19], pkt->getU8Ptr(0), pkt->getSize());
	}

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
	// Socket not opened, ignore sending packets
	if (!m_socket.is_open())
		return;

	SendBufferPtr sendBuf = m_send_queue.front();

	auto self(shared_from_this());

	if (sendBuf->reliable) {
		asio::async_write(m_socket, asio::buffer(sendBuf->data.data(),
			sendBuf->data.size()),
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
	else {
		getUDPSocket().async_send_to(asio::buffer(sendBuf->data.data(),
				sendBuf->data.size()), m_udp_endpoint,
			[this, self] (std::error_code ec, std::size_t ) {
				m_send_queue.pop_front();

				if (!ec) {
					if (!m_send_queue.empty()) {
						sendPacket();
					}
				}
				if (ec) {
					disconnect();
				}
			}
		);
	}
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

void ConnectionWorker::readBody()
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
					pushPacketToQueue();
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
}
