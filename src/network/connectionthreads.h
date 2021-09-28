/*
Minetest
Copyright (C) 2013-2017 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 celeron55, Loic Blot <loic.blot@unix-experience.fr>

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

#include <cassert>
#include "threading/thread.h"
#include "connection.h"

namespace con
{

class Connection;

struct OutgoingPacket
{
	session_t peer_id;
	u8 channelnum;
	SharedBuffer<u8> data;
	bool reliable;
	bool ack;

	OutgoingPacket(session_t peer_id_, u8 channelnum_, const SharedBuffer<u8> &data_,
			bool reliable_,bool ack_=false):
		peer_id(peer_id_),
		channelnum(channelnum_),
		data(data_),
		reliable(reliable_),
		ack(ack_)
	{
	}
};

class ConnectionSendThread : public Thread
{

public:
	friend class UDPPeer;

	ConnectionSendThread(unsigned int max_packet_size, float timeout);

	void *run();

	void Trigger();

	void setParent(Connection *parent)
	{
		assert(parent != NULL); // Pre-condition
		m_connection = parent;
	}

	void setPeerTimeout(float peer_timeout) { m_timeout = peer_timeout; }

private:
	void runTimeouts(float dtime);
	void rawSend(const BufferedPacket *p);
	bool rawSendAsPacket(session_t peer_id, u8 channelnum,
			const SharedBuffer<u8> &data, bool reliable);

	void processReliableCommand(ConnectionCommandPtr &c);
	void processNonReliableCommand(ConnectionCommandPtr &c);
	void serve(Address bind_address);
	void connect(Address address);
	void disconnect();
	void disconnect_peer(session_t peer_id);
	void send(session_t peer_id, u8 channelnum, const SharedBuffer<u8> &data);
	void sendReliable(ConnectionCommandPtr &c);
	void sendToAll(u8 channelnum, const SharedBuffer<u8> &data);
	void sendToAllReliable(ConnectionCommandPtr &c);

	void sendPackets(float dtime);

	void sendAsPacket(session_t peer_id, u8 channelnum, const SharedBuffer<u8> &data,
			bool ack = false);

	void sendAsPacketReliable(BufferedPacketPtr &p, Channel *channel);

	bool packetsQueued();

	Connection *m_connection = nullptr;
	unsigned int m_max_packet_size;
	float m_timeout;
	std::queue<OutgoingPacket> m_outgoing_queue;
	Semaphore m_send_sleep_semaphore;

	unsigned int m_iteration_packets_avaialble;
	unsigned int m_max_commands_per_iteration = 1;
	unsigned int m_max_data_packets_per_iteration;
	unsigned int m_max_packets_requeued = 256;
};

class ConnectionReceiveThread : public Thread
{
public:
	ConnectionReceiveThread(unsigned int max_packet_size);

	void *run();

	void setParent(Connection *parent)
	{
		assert(parent); // Pre-condition
		m_connection = parent;
	}

private:
	void receive(SharedBuffer<u8> &packetdata, bool &packet_queued);

	// Returns next data from a buffer if possible
	// If found, returns true; if not, false.
	// If found, sets peer_id and dst
	bool getFromBuffers(session_t &peer_id, SharedBuffer<u8> &dst);

	bool checkIncomingBuffers(
			Channel *channel, session_t &peer_id, SharedBuffer<u8> &dst);

	/*
		Processes a packet with the basic header stripped out.
		Parameters:
			packetdata: Data in packet (with no base headers)
			peer_id: peer id of the sender of the packet in question
			channelnum: channel on which the packet was sent
			reliable: true if recursing into a reliable packet
	*/
	SharedBuffer<u8> processPacket(Channel *channel,
			const SharedBuffer<u8> &packetdata, session_t peer_id,
			u8 channelnum, bool reliable);

	SharedBuffer<u8> handlePacketType_Control(Channel *channel,
			const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum,
			bool reliable);
	SharedBuffer<u8> handlePacketType_Original(Channel *channel,
			const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum,
			bool reliable);
	SharedBuffer<u8> handlePacketType_Split(Channel *channel,
			const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum,
			bool reliable);
	SharedBuffer<u8> handlePacketType_Reliable(Channel *channel,
			const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum,
			bool reliable);

	struct PacketTypeHandler
	{
		SharedBuffer<u8> (ConnectionReceiveThread::*handler)(Channel *channel,
				const SharedBuffer<u8> &packet, Peer *peer, u8 channelnum,
				bool reliable);
	};

	static const PacketTypeHandler packetTypeRouter[PACKET_TYPE_MAX];

	Connection *m_connection = nullptr;
};
}
