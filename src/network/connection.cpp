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
		write_buf->data.resize(pkt->getSize() + 2 + 4 + 4);
		writeU32(&(write_buf->data)[0], PROTOCOL_ID);
		writeU32(&(write_buf->data)[4], pkt->getSize() + 2); // total payload size
		writeU16(&(write_buf->data)[8], pkt->getCommand()); // command
		memcpy(&(write_buf->data)[10], pkt->getU8Ptr(0), pkt->getSize()); // payload
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
			std::unique_lock<std::recursive_mutex> lock(m_send_queue_mtx);

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
	if (!m_socket.is_open()) {
		notifySocketClosed();
		return;
	}

	std::unique_lock<std::recursive_mutex> lock(m_send_queue_mtx);

	SendBufferPtr sendBuf = m_send_queue.front();
	bool reliable = sendBuf->reliable;

	auto self(shared_from_this());

	auto async_write_callback = [this, self, reliable] (std::error_code ec, std::size_t) {
		std::unique_lock<std::recursive_mutex> lock_(m_send_queue_mtx);
		m_send_queue.pop_front();

		if (!ec) {
			if (!m_send_queue.empty()) {
				sendPacket();
			}
		} else {
			errorstream << "Failed to send " << (!reliable ? "un": "")
				<< "reliable packet";

			if (m_socket.is_open()) {
				errorstream << " to " << m_socket.remote_endpoint();
			}
			errorstream << ", disconnecting user. Error: " << ec.message()
				<< std::endl;
			disconnect();
		}
	};

	/*
	 * If packet is reliable, send to opened TCP socket
	 * If packet is unreliable and UDP endpoint is set send it
	 * In other cases drop packet
	 */
	if (sendBuf->reliable) {
		asio::async_write(m_socket, asio::buffer(sendBuf->data.data(),
			sendBuf->data.size()), async_write_callback);
	} else if (m_udp_endpoint_set) {
		getUDPSocket().async_send_to(asio::buffer(sendBuf->data.data(),
				sendBuf->data.size()), m_udp_endpoint, async_write_callback);
	} else {
		m_send_queue.pop_front();
	}
}

void ConnectionWorker::readHeader()
{
	size_t bytes_to_read = std::min<size_t>(HEADER_LEN - m_read_cursor,
		HEADER_LEN);

	auto self(shared_from_this());
	m_socket.async_read_some(asio::buffer(&m_hdr_buf[m_read_cursor], bytes_to_read),
		[this, self](std::error_code ec, std::size_t length) {
			if (!ec) {
				m_read_cursor += length;

				assert(length <= HEADER_LEN);

				// If header is incomplete continue reading
				if (m_read_cursor < HEADER_LEN) {
					readHeader();
					return;
				}

				u32 protocol_id = readU32(&m_hdr_buf[0]);
				if (protocol_id != PROTOCOL_ID) {
					verbosestream << "Invalid protocol ID in header. Disconnecting "
						<< m_socket.remote_endpoint() << std::endl;
					disconnect();
					return;
				}

				m_packet_waited_len = readU32(&m_hdr_buf[4]);
				if (m_packet_waited_len == 0) {
					errorstream << "Packet waited length is zero. Disconnecting "
						<< m_socket.remote_endpoint() << std::endl;
					disconnect();
					return;
				}

				if (m_packet_waited_len >= MAX_ALLOWED_PACKET_SIZE) {
					errorstream << "Packet waited length is too big ("
						<< m_packet_waited_len << " > " << MAX_ALLOWED_PACKET_SIZE
						<< ". Disconnecting from " << m_socket.remote_endpoint()
						<< std::endl;
					disconnect();
					return;
				}

				if (m_packet_waited_len > U16_MAX)
					errorstream << "Huge packet awaited, size: " << m_packet_waited_len << std::endl;
				m_read_cursor = 0;

				// New header, prepare packet buffer
				m_data_buf.clear();

				readBody();
			} else {
				errorstream << "Failed to read packet header from ";
				try {
					errorstream << m_socket.remote_endpoint();
				} catch (std::exception &) {
					errorstream << "peer";
				}

				errorstream << ", disconnecting user. Error: " << ec.message()
					<< std::endl;
				disconnect();
			}
		}
	);
}

void ConnectionWorker::readBody()
{
	// Read maximum of m_packet_waited_len or difference between packet length and
	// current cursor position if we didn't read all data (this should not happen)
	size_t bytes_to_read = std::min<size_t>(m_packet_waited_len - m_read_cursor,
		MAX_DATABUF_SIZE);

	m_data_buf.resize(m_data_buf.size() + bytes_to_read);

	auto self(shared_from_this());
	m_socket.async_read_some(asio::buffer(&m_data_buf[m_read_cursor], bytes_to_read),
		[this, self](std::error_code ec, std::size_t length) {
			if (!ec) {
				m_read_cursor += length;

				// The whole packet is read
				if (m_read_cursor == m_packet_waited_len) {
					pushPacketToQueue();

					m_read_cursor = 0;

					// Read next packet
					readHeader();
				// We read more data than needed, it's not normal, disconnect user
				} else if (m_read_cursor > m_packet_waited_len) {
					errorstream << "Failed to send packet to "
						<< m_socket.remote_endpoint()
						<< ". Error: Packet cursor outside waited len ("
						<< m_read_cursor << " < " << m_packet_waited_len << ")"
						<< std::endl;
					disconnect();
				} else {
					// Else we need to read the next chunk
					readBody();
				}

			} else {
				errorstream << "Failed to read packet body from ";
				try {
					errorstream << m_socket.remote_endpoint();
				} catch (std::exception &) {
					errorstream << "peer";
				}
				errorstream << ", disconnecting user. Error: " << ec.message()
					<< std::endl;
				disconnect();
			}
		}
	);
}
}
