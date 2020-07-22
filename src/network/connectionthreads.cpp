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

#include "connectionthreads.h"
#include "log.h"
#include "profiler.h"
#include "settings.h"
#include "network/networkpacket.h"
#include "util/serialize.h"

namespace con
{

/******************************************************************************/
/* defines used for debugging and profiling                                   */
/******************************************************************************/
#ifdef NDEBUG
#define LOG(a) a
#define PROFILE(a)
#undef DEBUG_CONNECTION_KBPS
#else
/* this mutex is used to achieve log message consistency */
std::mutex log_conthread_mutex;
#define LOG(a)                                                                \
	{                                                                         \
	MutexAutoLock loglock(log_conthread_mutex);                                 \
	a;                                                                        \
	}
#define PROFILE(a) a
//#define DEBUG_CONNECTION_KBPS
#undef DEBUG_CONNECTION_KBPS
#endif

/* maximum number of retries for reliable packets */
#define MAX_RELIABLE_RETRY 5

#define WINDOW_SIZE 5

static session_t readPeerId(u8 *packetdata)
{
	return readU16(&packetdata[4]);
}
static u8 readChannel(u8 *packetdata)
{
	return readU8(&packetdata[6]);
}

/******************************************************************************/
/* Connection Threads                                                         */
/******************************************************************************/

ConnectionSendThread::ConnectionSendThread(unsigned int max_packet_size,
	float timeout) :
	Thread("ConnectionSend"),
	m_max_packet_size(max_packet_size),
	m_timeout(timeout),
	m_max_data_packets_per_iteration(g_settings->getU16("max_packets_per_iteration"))
{
	SANITY_CHECK(m_max_data_packets_per_iteration > 1);
}

void *ConnectionSendThread::run()
{
	assert(m_connection);

	LOG(dout_con << m_connection->getDesc()
		<< "ConnectionSend thread started" << std::endl);

	u64 curtime = porting::getTimeMs();
	u64 lasttime = curtime;

	PROFILE(std::stringstream ThreadIdentifier);
	PROFILE(ThreadIdentifier << "ConnectionSend: [" << m_connection->getDesc() << "]");

	/* if stop is requested don't stop immediately but try to send all        */
	/* packets first */
	while (!stopRequested() || packetsQueued()) {
		BEGIN_DEBUG_EXCEPTION_HANDLER
		PROFILE(ScopeProfiler sp(g_profiler, ThreadIdentifier.str(), SPT_AVG));

		m_iteration_packets_avaialble = m_max_data_packets_per_iteration;

		/* wait for trigger or timeout */
		m_send_sleep_semaphore.wait(50);

		/* remove all triggers */
		while (m_send_sleep_semaphore.wait(0)) {
		}

		lasttime = curtime;
		curtime = porting::getTimeMs();
		float dtime = CALC_DTIME(lasttime, curtime);

		/* first resend timed-out packets */
		runTimeouts(dtime);
		if (m_iteration_packets_avaialble == 0) {
			LOG(warningstream << m_connection->getDesc()
				<< " Packet quota used up after re-sending packets, "
				<< "max=" << m_max_data_packets_per_iteration << std::endl);
		}

		/* translate commands to packets */
		ConnectionCommand c = m_connection->m_command_queue.pop_frontNoEx(0);
		while (c.type != CONNCMD_NONE) {
			if (c.reliable)
				processReliableCommand(c);
			else
				processNonReliableCommand(c);

			c = m_connection->m_command_queue.pop_frontNoEx(0);
		}

		/* send queued packets */
		sendPackets(dtime);

		END_DEBUG_EXCEPTION_HANDLER
	}

	PROFILE(g_profiler->remove(ThreadIdentifier.str()));
	return NULL;
}

void ConnectionSendThread::Trigger()
{
	m_send_sleep_semaphore.post();
}

bool ConnectionSendThread::packetsQueued()
{
	std::vector<session_t> peerIds = m_connection->getPeerIDs();

	if (!m_outgoing_queue.empty() && !peerIds.empty())
		return true;

	for (session_t peerId : peerIds) {
		PeerHelper peer = m_connection->getPeerNoEx(peerId);

		if (!peer)
			continue;

		if (dynamic_cast<UDPPeer *>(&peer) == 0)
			continue;

		for (Channel &channel : (dynamic_cast<UDPPeer *>(&peer))->channels) {
			if (!channel.queued_commands.empty()) {
				return true;
			}
		}
	}


	return false;
}

void ConnectionSendThread::runTimeouts(float dtime)
{
	std::vector<session_t> timeouted_peers;
	std::vector<session_t> peerIds = m_connection->getPeerIDs();

	for (session_t &peerId : peerIds) {
		PeerHelper peer = m_connection->getPeerNoEx(peerId);

		if (!peer)
			continue;

		UDPPeer *udpPeer = dynamic_cast<UDPPeer *>(&peer);
		if (!udpPeer)
			continue;

		PROFILE(std::stringstream peerIdentifier);
		PROFILE(peerIdentifier << "runTimeouts[" << m_connection->getDesc()
			<< ";" << peerId << ";RELIABLE]");
		PROFILE(ScopeProfiler
		peerprofiler(g_profiler, peerIdentifier.str(), SPT_AVG));

		SharedBuffer<u8> data(2); // data for sending ping, required here because of goto

		/*
			Check peer timeout
		*/
		if (peer->isTimedOut(m_timeout)) {
			infostream << m_connection->getDesc()
				<< "RunTimeouts(): Peer " << peer->id
				<< " has timed out."
				<< std::endl;
			// Add peer to the list
			timeouted_peers.push_back(peer->id);
			// Don't bother going through the buffers of this one
			continue;
		}

		float resend_timeout = udpPeer->getResendTimeout();
		bool retry_count_exceeded = false;
		for (Channel &channel : udpPeer->channels) {
			std::list<BufferedPacket> timed_outs;

			// Remove timed out incomplete unreliable split packets
			channel.incoming_splits.removeUnreliableTimedOuts(dtime, m_timeout);

			// Increment reliable packet times
			channel.outgoing_reliables_sent.incrementTimeouts(dtime);

			unsigned int numpeers = m_connection->m_peers.size();

			if (numpeers == 0)
				return;

			// Re-send timed out outgoing reliables
			timed_outs = channel.outgoing_reliables_sent.getTimedOuts(resend_timeout,
				(m_max_data_packets_per_iteration / numpeers));

			channel.UpdatePacketLossCounter(timed_outs.size());
			g_profiler->graphAdd("packets_lost", timed_outs.size());

			m_iteration_packets_avaialble -= timed_outs.size();

			for (std::list<BufferedPacket>::iterator k = timed_outs.begin();
				k != timed_outs.end(); ++k) {
				session_t peer_id = readPeerId(*(k->data));
				u8 channelnum = readChannel(*(k->data));
				u16 seqnum = readU16(&(k->data[BASE_HEADER_SIZE + 1]));

				channel.UpdateBytesLost(k->data.getSize());
				k->resend_count++;

				if (k->resend_count > MAX_RELIABLE_RETRY) {
					retry_count_exceeded = true;
					timeouted_peers.push_back(peer->id);
					/* no need to check additional packets if a single one did timeout*/
					break;
				}

				LOG(derr_con << m_connection->getDesc()
					<< "RE-SENDING timed-out RELIABLE to "
					<< k->address.serializeString()
					<< "(t/o=" << resend_timeout << "): "
					<< "from_peer_id=" << peer_id
					<< ", channel=" << ((int) channelnum & 0xff)
					<< ", seqnum=" << seqnum
					<< std::endl);

				rawSend(*k);

				// do not handle rtt here as we can't decide if this packet was
				// lost or really takes more time to transmit
			}

			if (retry_count_exceeded) {
				break; /* no need to check other channels if we already did timeout */
			}

			channel.UpdateTimers(dtime);
		}

		/* skip to next peer if we did timeout */
		if (retry_count_exceeded)
			continue;

		/* send ping if necessary */
		if (udpPeer->Ping(dtime, data)) {
			LOG(dout_con << m_connection->getDesc()
				<< "Sending ping for peer_id: " << udpPeer->id << std::endl);
			/* this may fail if there ain't a sequence number left */
			if (!rawSendAsPacket(udpPeer->id, 0, data, true)) {
				//retrigger with reduced ping interval
				udpPeer->Ping(4.0, data);
			}
		}

		udpPeer->RunCommandQueues(m_max_packet_size,
			m_max_commands_per_iteration,
			m_max_packets_requeued);
	}

	// Remove timed out peers
	for (u16 timeouted_peer : timeouted_peers) {
		LOG(dout_con << m_connection->getDesc()
			<< "RunTimeouts(): Removing peer " << timeouted_peer << std::endl);
		m_connection->deletePeer(timeouted_peer, true);
	}
}

void ConnectionSendThread::rawSend(const BufferedPacket &packet)
{
	try {
		m_connection->m_udpSocket.Send(packet.address, *packet.data,
			packet.data.getSize());
		LOG(dout_con << m_connection->getDesc()
			<< " rawSend: " << packet.data.getSize()
			<< " bytes sent" << std::endl);
	} catch (SendFailedException &e) {
		LOG(derr_con << m_connection->getDesc()
			<< "Connection::rawSend(): SendFailedException: "
			<< packet.address.serializeString() << std::endl);
	}
}

void ConnectionSendThread::sendAsPacketReliable(BufferedPacket &p, Channel *channel)
{
	try {
		p.absolute_send_time = porting::getTimeMs();
		// Buffer the packet
		channel->outgoing_reliables_sent.insert(p,
			(channel->readOutgoingSequenceNumber() - MAX_RELIABLE_WINDOW_SIZE)
				% (MAX_RELIABLE_WINDOW_SIZE + 1));
	}
	catch (AlreadyExistsException &e) {
		LOG(derr_con << m_connection->getDesc()
			<< "WARNING: Going to send a reliable packet"
			<< " in outgoing buffer" << std::endl);
	}

	// Send the packet
	rawSend(p);
}

bool ConnectionSendThread::rawSendAsPacket(session_t peer_id, u8 channelnum,
	const SharedBuffer<u8> &data, bool reliable)
{
	PeerHelper peer = m_connection->getPeerNoEx(peer_id);
	if (!peer) {
		LOG(errorstream << m_connection->getDesc()
			<< " dropped " << (reliable ? "reliable " : "")
			<< "packet for non existent peer_id: " << peer_id << std::endl);
		return false;
	}
	Channel *channel = &(dynamic_cast<UDPPeer *>(&peer)->channels[channelnum]);

	if (reliable) {
		bool have_sequence_number_for_raw_packet = true;
		u16 seqnum =
			channel->getOutgoingSequenceNumber(have_sequence_number_for_raw_packet);

		if (!have_sequence_number_for_raw_packet)
			return false;

		SharedBuffer<u8> reliable = makeReliablePacket(data, seqnum);
		Address peer_address;
		peer->getAddress(MTP_MINETEST_RELIABLE_UDP, peer_address);

		// Add base headers and make a packet
		BufferedPacket p = con::makePacket(peer_address, reliable,
			m_connection->GetProtocolID(), m_connection->GetPeerID(),
			channelnum);

		// first check if our send window is already maxed out
		if (channel->outgoing_reliables_sent.size()
			< channel->getWindowSize()) {
			LOG(dout_con << m_connection->getDesc()
				<< " INFO: sending a reliable packet to peer_id " << peer_id
				<< " channel: " << (u32)channelnum
				<< " seqnum: " << seqnum << std::endl);
			sendAsPacketReliable(p, channel);
			return true;
		}

		LOG(dout_con << m_connection->getDesc()
			<< " INFO: queueing reliable packet for peer_id: " << peer_id
			<< " channel: " << (u32)channelnum
			<< " seqnum: " << seqnum << std::endl);
		channel->queued_reliables.push(p);
		return false;
	}

	Address peer_address;
	if (peer->getAddress(MTP_UDP, peer_address)) {
		// Add base headers and make a packet
		BufferedPacket p = con::makePacket(peer_address, data,
			m_connection->GetProtocolID(), m_connection->GetPeerID(),
			channelnum);

		// Send the packet
		rawSend(p);
		return true;
	}

	LOG(dout_con << m_connection->getDesc()
		<< " INFO: dropped unreliable packet for peer_id: " << peer_id
		<< " because of (yet) missing udp address" << std::endl);
	return false;
}

void ConnectionSendThread::processReliableCommand(ConnectionCommand &c)
{
	assert(c.reliable);  // Pre-condition

	switch (c.type) {
		case CONNCMD_NONE:
			LOG(dout_con << m_connection->getDesc()
				<< "UDP processing reliable CONNCMD_NONE" << std::endl);
			return;

		case CONNCMD_SEND:
			LOG(dout_con << m_connection->getDesc()
				<< "UDP processing reliable CONNCMD_SEND" << std::endl);
			sendReliable(c);
			return;

		case CONNCMD_SEND_TO_ALL:
			LOG(dout_con << m_connection->getDesc()
				<< "UDP processing CONNCMD_SEND_TO_ALL" << std::endl);
			sendToAllReliable(c);
			return;

		case CONCMD_CREATE_PEER:
			LOG(dout_con << m_connection->getDesc()
				<< "UDP processing reliable CONCMD_CREATE_PEER" << std::endl);
			if (!rawSendAsPacket(c.peer_id, c.channelnum, c.data, c.reliable)) {
				/* put to queue if we couldn't send it immediately */
				sendReliable(c);
			}
			return;

		case CONNCMD_SERVE:
		case CONNCMD_CONNECT:
		case CONNCMD_DISCONNECT:
		case CONCMD_ACK:
			FATAL_ERROR("Got command that shouldn't be reliable as reliable command");
		default:
			LOG(dout_con << m_connection->getDesc()
				<< " Invalid reliable command type: " << c.type << std::endl);
	}
}


void ConnectionSendThread::processNonReliableCommand(ConnectionCommand &c)
{
	assert(!c.reliable); // Pre-condition

	switch (c.type) {
		case CONNCMD_NONE:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_NONE" << std::endl);
			return;
		case CONNCMD_SERVE:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_SERVE port="
				<< c.address.serializeString() << std::endl);
			serve(c.address);
			return;
		case CONNCMD_CONNECT:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_CONNECT" << std::endl);
			connect(c.address);
			return;
		case CONNCMD_DISCONNECT:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_DISCONNECT" << std::endl);
			disconnect();
			return;
		case CONNCMD_DISCONNECT_PEER:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_DISCONNECT_PEER" << std::endl);
			disconnect_peer(c.peer_id);
			return;
		case CONNCMD_SEND:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_SEND" << std::endl);
			send(c.peer_id, c.channelnum, c.data);
			return;
		case CONNCMD_SEND_TO_ALL:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONNCMD_SEND_TO_ALL" << std::endl);
			sendToAll(c.channelnum, c.data);
			return;
		case CONCMD_ACK:
			LOG(dout_con << m_connection->getDesc()
				<< " UDP processing CONCMD_ACK" << std::endl);
			sendAsPacket(c.peer_id, c.channelnum, c.data, true);
			return;
		case CONCMD_CREATE_PEER:
			FATAL_ERROR("Got command that should be reliable as unreliable command");
		default:
			LOG(dout_con << m_connection->getDesc()
				<< " Invalid command type: " << c.type << std::endl);
	}
}

void ConnectionSendThread::serve(Address bind_address)
{
	LOG(dout_con << m_connection->getDesc()
		<< "UDP serving at port " << bind_address.serializeString() << std::endl);
	try {
		m_connection->m_udpSocket.Bind(bind_address);
		m_connection->SetPeerID(PEER_ID_SERVER);
	}
	catch (SocketException &e) {
		// Create event
		ConnectionEvent ce;
		ce.bindFailed();
		m_connection->putEvent(ce);
	}
}

void ConnectionSendThread::connect(Address address)
{
	LOG(dout_con << m_connection->getDesc() << " connecting to "
		<< address.serializeString()
		<< ":" << address.getPort() << std::endl);

	UDPPeer *peer = m_connection->createServerPeer(address);

	// Create event
	ConnectionEvent e;
	e.peerAdded(peer->id, peer->address);
	m_connection->putEvent(e);

	Address bind_addr;

	if (address.isIPv6())
		bind_addr.setAddress((IPv6AddressBytes *) NULL);
	else
		bind_addr.setAddress(0, 0, 0, 0);

	m_connection->m_udpSocket.Bind(bind_addr);

	// Send a dummy packet to server with peer_id = PEER_ID_INEXISTENT
	m_connection->SetPeerID(PEER_ID_INEXISTENT);
	NetworkPacket pkt(0, 0);
	m_connection->Send(PEER_ID_SERVER, 0, &pkt, true);
}

void ConnectionSendThread::disconnect()
{
	LOG(dout_con << m_connection->getDesc() << " disconnecting" << std::endl);

	// Create and send DISCO packet
	SharedBuffer<u8> data(2);
	writeU8(&data[0], PACKET_TYPE_CONTROL);
	writeU8(&data[1], CONTROLTYPE_DISCO);


	// Send to all
	std::vector<session_t> peerids = m_connection->getPeerIDs();

	for (session_t peerid : peerids) {
		sendAsPacket(peerid, 0, data, false);
	}
}

void ConnectionSendThread::disconnect_peer(session_t peer_id)
{
	LOG(dout_con << m_connection->getDesc() << " disconnecting peer" << std::endl);

	// Create and send DISCO packet
	SharedBuffer<u8> data(2);
	writeU8(&data[0], PACKET_TYPE_CONTROL);
	writeU8(&data[1], CONTROLTYPE_DISCO);
	sendAsPacket(peer_id, 0, data, false);

	PeerHelper peer = m_connection->getPeerNoEx(peer_id);

	if (!peer)
		return;

	if (dynamic_cast<UDPPeer *>(&peer) == 0) {
		return;
	}

	dynamic_cast<UDPPeer *>(&peer)->m_pending_disconnect = true;
}

void ConnectionSendThread::send(session_t peer_id, u8 channelnum,
	const SharedBuffer<u8> &data)
{
	assert(channelnum < CHANNEL_COUNT); // Pre-condition

	PeerHelper peer = m_connection->getPeerNoEx(peer_id);
	if (!peer) {
		LOG(dout_con << m_connection->getDesc() << " peer: peer_id=" << peer_id
			<< ">>>NOT<<< found on sending packet"
			<< ", channel " << (channelnum % 0xFF)
			<< ", size: " << data.getSize() << std::endl);
		return;
	}

	LOG(dout_con << m_connection->getDesc() << " sending to peer_id=" << peer_id
		<< ", channel " << (channelnum % 0xFF)
		<< ", size: " << data.getSize() << std::endl);

	u16 split_sequence_number = peer->getNextSplitSequenceNumber(channelnum);

	u32 chunksize_max = m_max_packet_size - BASE_HEADER_SIZE;
	std::list<SharedBuffer<u8>> originals;

	makeAutoSplitPacket(data, chunksize_max, split_sequence_number, &originals);

	peer->setNextSplitSequenceNumber(channelnum, split_sequence_number);

	for (const SharedBuffer<u8> &original : originals) {
		sendAsPacket(peer_id, channelnum, original);
	}
}

void ConnectionSendThread::sendReliable(ConnectionCommand &c)
{
	PeerHelper peer = m_connection->getPeerNoEx(c.peer_id);
	if (!peer)
		return;

	peer->PutReliableSendCommand(c, m_max_packet_size);
}

void ConnectionSendThread::sendToAll(u8 channelnum, const SharedBuffer<u8> &data)
{
	std::vector<session_t> peerids = m_connection->getPeerIDs();

	for (session_t peerid : peerids) {
		send(peerid, channelnum, data);
	}
}

void ConnectionSendThread::sendToAllReliable(ConnectionCommand &c)
{
	std::vector<session_t> peerids = m_connection->getPeerIDs();

	for (session_t peerid : peerids) {
		PeerHelper peer = m_connection->getPeerNoEx(peerid);

		if (!peer)
			continue;

		peer->PutReliableSendCommand(c, m_max_packet_size);
	}
}

void ConnectionSendThread::sendPackets(float dtime)
{
	std::vector<session_t> peerIds = m_connection->getPeerIDs();
	std::vector<session_t> pendingDisconnect;
	std::map<session_t, bool> pending_unreliable;

	const unsigned int peer_packet_quota = m_iteration_packets_avaialble
		/ MYMAX(peerIds.size(), 1);

	for (session_t peerId : peerIds) {
		PeerHelper peer = m_connection->getPeerNoEx(peerId);
		//peer may have been removed
		if (!peer) {
			LOG(dout_con << m_connection->getDesc() << " Peer not found: peer_id="
				<< peerId
				<< std::endl);
			continue;
		}
		peer->m_increment_packets_remaining = peer_packet_quota;

		UDPPeer *udpPeer = dynamic_cast<UDPPeer *>(&peer);

		if (!udpPeer) {
			continue;
		}

		if (udpPeer->m_pending_disconnect) {
			pendingDisconnect.push_back(peerId);
		}

		PROFILE(std::stringstream
		peerIdentifier);
		PROFILE(
			peerIdentifier << "sendPackets[" << m_connection->getDesc() << ";" << peerId
				<< ";RELIABLE]");
		PROFILE(ScopeProfiler
		peerprofiler(g_profiler, peerIdentifier.str(), SPT_AVG));

		LOG(dout_con << m_connection->getDesc()
			<< " Handle per peer queues: peer_id=" << peerId
			<< " packet quota: " << peer->m_increment_packets_remaining << std::endl);

		// first send queued reliable packets for all peers (if possible)
		for (unsigned int i = 0; i < CHANNEL_COUNT; i++) {
			Channel &channel = udpPeer->channels[i];
			u16 next_to_ack = 0;

			channel.outgoing_reliables_sent.getFirstSeqnum(next_to_ack);
			u16 next_to_receive = 0;
			channel.incoming_reliables.getFirstSeqnum(next_to_receive);

			LOG(dout_con << m_connection->getDesc() << "\t channel: "
				<< i << ", peer quota:"
				<< peer->m_increment_packets_remaining
				<< std::endl
				<< "\t\t\treliables on wire: "
				<< channel.outgoing_reliables_sent.size()
				<< ", waiting for ack for " << next_to_ack
				<< std::endl
				<< "\t\t\tincoming_reliables: "
				<< channel.incoming_reliables.size()
				<< ", next reliable packet: "
				<< channel.readNextIncomingSeqNum()
				<< ", next queued: " << next_to_receive
				<< std::endl
				<< "\t\t\treliables queued : "
				<< channel.queued_reliables.size()
				<< std::endl
				<< "\t\t\tqueued commands  : "
				<< channel.queued_commands.size()
				<< std::endl);

			while (!channel.queued_reliables.empty() &&
					channel.outgoing_reliables_sent.size()
					< channel.getWindowSize() &&
					peer->m_increment_packets_remaining > 0) {
				BufferedPacket p = channel.queued_reliables.front();
				channel.queued_reliables.pop();
				LOG(dout_con << m_connection->getDesc()
					<< " INFO: sending a queued reliable packet "
					<< " channel: " << i
					<< ", seqnum: " << readU16(&p.data[BASE_HEADER_SIZE + 1])
					<< std::endl);
				sendAsPacketReliable(p, &channel);
				peer->m_increment_packets_remaining--;
			}
		}
	}

	if (!m_outgoing_queue.empty()) {
		LOG(dout_con << m_connection->getDesc()
			<< " Handle non reliable queue ("
			<< m_outgoing_queue.size() << " pkts)" << std::endl);
	}

	unsigned int initial_queuesize = m_outgoing_queue.size();
	/* send non reliable packets*/
	for (unsigned int i = 0; i < initial_queuesize; i++) {
		OutgoingPacket packet = m_outgoing_queue.front();
		m_outgoing_queue.pop();

		if (packet.reliable)
			continue;

		PeerHelper peer = m_connection->getPeerNoEx(packet.peer_id);
		if (!peer) {
			LOG(dout_con << m_connection->getDesc()
				<< " Outgoing queue: peer_id=" << packet.peer_id
				<< ">>>NOT<<< found on sending packet"
				<< ", channel " << (packet.channelnum % 0xFF)
				<< ", size: " << packet.data.getSize() << std::endl);
			continue;
		}

		/* send acks immediately */
		if (packet.ack || peer->m_increment_packets_remaining > 0 || stopRequested()) {
			rawSendAsPacket(packet.peer_id, packet.channelnum,
				packet.data, packet.reliable);
			if (peer->m_increment_packets_remaining > 0)
				peer->m_increment_packets_remaining--;
		} else {
			m_outgoing_queue.push(packet);
			pending_unreliable[packet.peer_id] = true;
		}
	}

	if (peer_packet_quota > 0) {
		for (session_t peerId : peerIds) {
			PeerHelper peer = m_connection->getPeerNoEx(peerId);
			if (!peer)
				continue;
			if (peer->m_increment_packets_remaining == 0) {
				LOG(warningstream << m_connection->getDesc()
					<< " Packet quota used up for peer_id=" << peerId
					<< ", was " << peer_packet_quota << " pkts" << std::endl);
			}
		}
	}

	for (session_t peerId : pendingDisconnect) {
		if (!pending_unreliable[peerId]) {
			m_connection->deletePeer(peerId, false);
		}
	}
}

void ConnectionSendThread::sendAsPacket(session_t peer_id, u8 channelnum,
	const SharedBuffer<u8> &data, bool ack)
{
	OutgoingPacket packet(peer_id, channelnum, data, false, ack);
	m_outgoing_queue.push(packet);
}

ConnectionReceiveThread::ConnectionReceiveThread(unsigned int max_packet_size) :
	Thread("ConnectionReceive")
{
}

void *ConnectionReceiveThread::run()
{
	assert(m_connection);

	LOG(dout_con << m_connection->getDesc()
		<< "ConnectionReceive thread started" << std::endl);

	PROFILE(std::stringstream
	ThreadIdentifier);
	PROFILE(ThreadIdentifier << "ConnectionReceive: [" << m_connection->getDesc() << "]");

	// use IPv6 minimum allowed MTU as receive buffer size as this is
	// theoretical reliable upper boundary of a udp packet for all IPv6 enabled
	// infrastructure
	const unsigned int packet_maxsize = 1500;
	SharedBuffer<u8> packetdata(packet_maxsize);

	bool packet_queued = true;

#ifdef DEBUG_CONNECTION_KBPS
	u64 curtime = porting::getTimeMs();
	u64 lasttime = curtime;
	float debug_print_timer = 0.0;
#endif

	while (!stopRequested()) {
		BEGIN_DEBUG_EXCEPTION_HANDLER
		PROFILE(ScopeProfiler
		sp(g_profiler, ThreadIdentifier.str(), SPT_AVG));

#ifdef DEBUG_CONNECTION_KBPS
		lasttime = curtime;
		curtime = porting::getTimeMs();
		float dtime = CALC_DTIME(lasttime,curtime);
#endif

		/* receive packets */
		receive(packetdata, packet_queued);

#ifdef DEBUG_CONNECTION_KBPS
		debug_print_timer += dtime;
		if (debug_print_timer > 20.0) {
			debug_print_timer -= 20.0;

			std::vector<session_t> peerids = m_connection->getPeerIDs();

			for (auto id : peerids)
			{
				PeerHelper peer = m_connection->getPeerNoEx(id);
				if (!peer)
					continue;

				float peer_current = 0.0;
				float peer_loss = 0.0;
				float avg_rate = 0.0;
				float avg_loss = 0.0;

				for(u16 j=0; j<CHANNEL_COUNT; j++)
				{
					peer_current +=peer->channels[j].getCurrentDownloadRateKB();
					peer_loss += peer->channels[j].getCurrentLossRateKB();
					avg_rate += peer->channels[j].getAvgDownloadRateKB();
					avg_loss += peer->channels[j].getAvgLossRateKB();
				}

				std::stringstream output;
				output << std::fixed << std::setprecision(1);
				output << "OUT to Peer " << *i << " RATES (good / loss) " << std::endl;
				output << "\tcurrent (sum): " << peer_current << "kb/s "<< peer_loss << "kb/s" << std::endl;
				output << "\taverage (sum): " << avg_rate << "kb/s "<< avg_loss << "kb/s" << std::endl;
				output << std::setfill(' ');
				for(u16 j=0; j<CHANNEL_COUNT; j++)
				{
					output << "\tcha " << j << ":"
						<< " CUR: " << std::setw(6) << peer->channels[j].getCurrentDownloadRateKB() <<"kb/s"
						<< " AVG: " << std::setw(6) << peer->channels[j].getAvgDownloadRateKB() <<"kb/s"
						<< " MAX: " << std::setw(6) << peer->channels[j].getMaxDownloadRateKB() <<"kb/s"
						<< " /"
						<< " CUR: " << std::setw(6) << peer->channels[j].getCurrentLossRateKB() <<"kb/s"
						<< " AVG: " << std::setw(6) << peer->channels[j].getAvgLossRateKB() <<"kb/s"
						<< " MAX: " << std::setw(6) << peer->channels[j].getMaxLossRateKB() <<"kb/s"
						<< " / WS: " << peer->channels[j].getWindowSize()
						<< std::endl;
				}

				fprintf(stderr,"%s\n",output.str().c_str());
			}
		}
#endif
		END_DEBUG_EXCEPTION_HANDLER
	}

	PROFILE(g_profiler->remove(ThreadIdentifier.str()));
	return NULL;
}

// Receive packets from the network and buffers and create ConnectionEvents
void ConnectionReceiveThread::receive(SharedBuffer<u8> &packetdata,
		bool &packet_queued)
{
	try {
		// First, see if there any buffered packets we can process now
		if (packet_queued) {
			bool data_left = true;
			session_t peer_id;
			SharedBuffer<u8> resultdata;
			while (data_left) {
				try {
					data_left = getFromBuffers(peer_id, resultdata);
					if (data_left) {
						ConnectionEvent e;
						e.dataReceived(peer_id, resultdata);
						m_connection->putEvent(e);
					}
				}
				catch (ProcessedSilentlyException &e) {
					/* try reading again */
				}
			}
			packet_queued = false;
		}

		// Call Receive() to wait for incoming data
		Address sender;
		s32 received_size = m_connection->m_udpSocket.Receive(sender,
			*packetdata, packetdata.getSize());
		if (received_size < 0)
			return;

		if ((received_size < BASE_HEADER_SIZE) ||
			(readU32(&packetdata[0]) != m_connection->GetProtocolID())) {
			LOG(derr_con << m_connection->getDesc()
				<< "Receive(): Invalid incoming packet, "
				<< "size: " << received_size
				<< ", protocol: "
				<< ((received_size >= 4) ? readU32(&packetdata[0]) : -1)
				<< std::endl);
			return;
		}

		session_t peer_id = readPeerId(*packetdata);
		u8 channelnum = readChannel(*packetdata);

		if (channelnum > CHANNEL_COUNT - 1) {
			LOG(derr_con << m_connection->getDesc()
				<< "Receive(): Invalid channel " << (u32)channelnum << std::endl);
			return;
		}

		/* Try to identify peer by sender address (may happen on join) */
		if (peer_id == PEER_ID_INEXISTENT) {
			peer_id = m_connection->lookupPeer(sender);
			// We do not have to remind the peer of its
			// peer id as the CONTROLTYPE_SET_PEER_ID
			// command was sent reliably.
		}

		/* The peer was not found in our lists. Add it. */
		if (peer_id == PEER_ID_INEXISTENT) {
			peer_id = m_connection->createPeer(sender, MTP_MINETEST_RELIABLE_UDP, 0);
		}

		PeerHelper peer = m_connection->getPeerNoEx(peer_id);
		if (!peer) {
			LOG(dout_con << m_connection->getDesc()
				<< " got packet from unknown peer_id: "
				<< peer_id << " Ignoring." << std::endl);
			return;
		}

		// Validate peer address

		Address peer_address;
		if (peer->getAddress(MTP_UDP, peer_address)) {
			if (peer_address != sender) {
				LOG(derr_con << m_connection->getDesc()
					<< " Peer " << peer_id << " sending from different address."
					" Ignoring." << std::endl);
				return;
			}
		} else {
			LOG(derr_con << m_connection->getDesc()
				<< " Peer " << peer_id << " doesn't have an address?!"
				" Ignoring." << std::endl);
			return;
		}

		peer->ResetTimeout();

		Channel *channel = nullptr;
		if (dynamic_cast<UDPPeer *>(&peer)) {
			channel = &dynamic_cast<UDPPeer *>(&peer)->channels[channelnum];
		} else {
			LOG(derr_con << m_connection->getDesc()
				<< "Receive(): peer_id=" << peer_id << " isn't an UDPPeer?!"
				" Ignoring." << std::endl);
			return;
		}

		channel->UpdateBytesReceived(received_size);

		// Throw the received packet to channel->processPacket()

		// Make a new SharedBuffer from the data without the base headers
		SharedBuffer<u8> strippeddata(received_size - BASE_HEADER_SIZE);
		memcpy(*strippeddata, &packetdata[BASE_HEADER_SIZE],
			strippeddata.getSize());

		try {
			// Process it (the result is some data with no headers made by us)
			SharedBuffer<u8> resultdata = processPacket
				(channel, strippeddata, peer_id, channelnum, false);

			LOG(dout_con << m_connection->getDesc()
				<< " ProcessPacket from peer_id: " << peer_id
				<< ", channel: " << (u32)channelnum << ", returned "
				<< resultdata.getSize() << " bytes" << std::endl);

			ConnectionEvent e;
			e.dataReceived(peer_id, resultdata);
			m_connection->putEvent(e);
		}
		catch (ProcessedSilentlyException &e) {
		}
		catch (ProcessedQueued &e) {
			// we set it to true anyway (see below)
		}

		/* Every time we receive a packet it can happen that a previously
		 * buffered packet is now ready to process. */
		packet_queued = true;
	}
	catch (InvalidIncomingDataException &e) {
	}
}

bool ConnectionReceiveThread::getFromBuffers(session_t &peer_id, SharedBuffer<u8> &dst)
{
	std::vector<session_t> peerids = m_connection->getPeerIDs();

	for (session_t peerid : peerids) {
		PeerHelper peer = m_connection->getPeerNoEx(peerid);
		if (!peer)
			continue;

		if (dynamic_cast<UDPPeer *>(&peer) == 0)
			continue;

		for (Channel &channel : (dynamic_cast<UDPPeer *>(&peer))->channels) {
			if (checkIncomingBuffers(&channel, peer_id, dst)) {
				return true;
			}
		}
	}
	return false;
}

bool ConnectionReceiveThread::checkIncomingBuffers(Channel *channel,
	session_t &peer_id, SharedBuffer<u8> &dst)
{
	u16 firstseqnum = 0;
	if (channel->incoming_reliables.getFirstSeqnum(firstseqnum)) {
		if (firstseqnum == channel->readNextIncomingSeqNum()) {
			BufferedPacket p = channel->incoming_reliables.popFirst();
			peer_id = readPeerId(*p.data);
			u8 channelnum = readChannel(*p.data);
			u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE + 1]);

			LOG(dout_con << m_connection->getDesc()
				<< "UNBUFFERING TYPE_RELIABLE"
				<< " seqnum=" << seqnum
				<< " peer_id=" << peer_id
				<< " channel=" << ((int) channelnum & 0xff)
				<< std::endl);

			channel->incNextIncomingSeqNum();

			u32 headers_size = BASE_HEADER_SIZE + RELIABLE_HEADER_SIZE;
			// Get out the inside packet and re-process it
			SharedBuffer<u8> payload(p.data.getSize() - headers_size);
			memcpy(*payload, &p.data[headers_size], payload.getSize());

			dst = processPacket(channel, payload, peer_id, channelnum, true);
			return true;
		}
	}
	return false;
}

SharedBuffer<u8> ConnectionReceiveThread::processPacket(Channel *channel,
	const SharedBuffer<u8> &packetdata, session_t peer_id, u8 channelnum, bool reliable)
{
	PeerHelper peer = m_connection->getPeerNoEx(peer_id);

	if (!peer) {
		errorstream << "Peer not found (possible timeout)" << std::endl;
		throw ProcessedSilentlyException("Peer not found (possible timeout)");
	}

	if (packetdata.getSize() < 1)
		throw InvalidIncomingDataException("packetdata.getSize() < 1");

	u8 type = readU8(&(packetdata[0]));

	if (MAX_UDP_PEERS <= 65535 && peer_id >= MAX_UDP_PEERS) {
		std::string errmsg = "Invalid peer_id=" + itos(peer_id);
		errorstream << errmsg << std::endl;
		throw InvalidIncomingDataException(errmsg.c_str());
	}

	if (type >= PACKET_TYPE_MAX) {
		derr_con << m_connection->getDesc() << "Got invalid type=" << ((int) type & 0xff)
			<< std::endl;
		throw InvalidIncomingDataException("Invalid packet type");
	}

	const PacketTypeHandler &pHandle = packetTypeRouter[type];
	return (this->*pHandle.handler)(channel, packetdata, &peer, channelnum, reliable);
}

const ConnectionReceiveThread::PacketTypeHandler
	ConnectionReceiveThread::packetTypeRouter[PACKET_TYPE_MAX] = {
	{&ConnectionReceiveThread::handlePacketType_Control},
	{&ConnectionReceiveThread::handlePacketType_Original},
	{&ConnectionReceiveThread::handlePacketType_Split},
	{&ConnectionReceiveThread::handlePacketType_Reliable},
};

SharedBuffer<u8> ConnectionReceiveThread::handlePacketType_Control(Channel *channel,
	const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum, bool reliable)
{
	if (packetdata.getSize() < 2)
		throw InvalidIncomingDataException("packetdata.getSize() < 2");

	u8 controltype = readU8(&(packetdata[1]));

	if (controltype == CONTROLTYPE_ACK) {
		assert(channel != NULL);

		if (packetdata.getSize() < 4) {
			throw InvalidIncomingDataException(
				"packetdata.getSize() < 4 (ACK header size)");
		}

		u16 seqnum = readU16(&packetdata[2]);
		LOG(dout_con << m_connection->getDesc() << " [ CONTROLTYPE_ACK: channelnum="
			<< ((int) channelnum & 0xff) << ", peer_id=" << peer->id << ", seqnum="
			<< seqnum << " ]" << std::endl);

		try {
			BufferedPacket p = channel->outgoing_reliables_sent.popSeqnum(seqnum);

			// only calculate rtt from straight sent packets
			if (p.resend_count == 0) {
				// Get round trip time
				u64 current_time = porting::getTimeMs();

				// a overflow is quite unlikely but as it'd result in major
				// rtt miscalculation we handle it here
				if (current_time > p.absolute_send_time) {
					float rtt = (current_time - p.absolute_send_time) / 1000.0;

					// Let peer calculate stuff according to it
					// (avg_rtt and resend_timeout)
					dynamic_cast<UDPPeer *>(peer)->reportRTT(rtt);
				} else if (p.totaltime > 0) {
					float rtt = p.totaltime;

					// Let peer calculate stuff according to it
					// (avg_rtt and resend_timeout)
					dynamic_cast<UDPPeer *>(peer)->reportRTT(rtt);
				}
			}
			// put bytes for max bandwidth calculation
			channel->UpdateBytesSent(p.data.getSize(), 1);
			if (channel->outgoing_reliables_sent.size() == 0)
				m_connection->TriggerSend();
		} catch (NotFoundException &e) {
			LOG(derr_con << m_connection->getDesc()
				<< "WARNING: ACKed packet not in outgoing queue"
				<< " seqnum=" << seqnum << std::endl);
			channel->UpdatePacketTooLateCounter();
		}

		throw ProcessedSilentlyException("Got an ACK");
	} else if (controltype == CONTROLTYPE_SET_PEER_ID) {
		// Got a packet to set our peer id
		if (packetdata.getSize() < 4)
			throw InvalidIncomingDataException
				("packetdata.getSize() < 4 (SET_PEER_ID header size)");
		session_t peer_id_new = readU16(&packetdata[2]);
		LOG(dout_con << m_connection->getDesc() << "Got new peer id: " << peer_id_new
			<< "... " << std::endl);

		if (m_connection->GetPeerID() != PEER_ID_INEXISTENT) {
			LOG(derr_con << m_connection->getDesc()
				<< "WARNING: Not changing existing peer id." << std::endl);
		} else {
			LOG(dout_con << m_connection->getDesc() << "changing own peer id"
				<< std::endl);
			m_connection->SetPeerID(peer_id_new);
		}

		throw ProcessedSilentlyException("Got a SET_PEER_ID");
	} else if (controltype == CONTROLTYPE_PING) {
		// Just ignore it, the incoming data already reset
		// the timeout counter
		LOG(dout_con << m_connection->getDesc() << "PING" << std::endl);
		throw ProcessedSilentlyException("Got a PING");
	} else if (controltype == CONTROLTYPE_DISCO) {
		// Just ignore it, the incoming data already reset
		// the timeout counter
		LOG(dout_con << m_connection->getDesc() << "DISCO: Removing peer "
			<< peer->id << std::endl);

		if (!m_connection->deletePeer(peer->id, false)) {
			derr_con << m_connection->getDesc() << "DISCO: Peer not found" << std::endl;
		}

		throw ProcessedSilentlyException("Got a DISCO");
	} else {
		LOG(derr_con << m_connection->getDesc()
			<< "INVALID TYPE_CONTROL: invalid controltype="
			<< ((int) controltype & 0xff) << std::endl);
		throw InvalidIncomingDataException("Invalid control type");
	}
}

SharedBuffer<u8> ConnectionReceiveThread::handlePacketType_Original(Channel *channel,
	const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum, bool reliable)
{
	if (packetdata.getSize() <= ORIGINAL_HEADER_SIZE)
		throw InvalidIncomingDataException
			("packetdata.getSize() <= ORIGINAL_HEADER_SIZE");
	LOG(dout_con << m_connection->getDesc() << "RETURNING TYPE_ORIGINAL to user"
		<< std::endl);
	// Get the inside packet out and return it
	SharedBuffer<u8> payload(packetdata.getSize() - ORIGINAL_HEADER_SIZE);
	memcpy(*payload, &(packetdata[ORIGINAL_HEADER_SIZE]), payload.getSize());
	return payload;
}

SharedBuffer<u8> ConnectionReceiveThread::handlePacketType_Split(Channel *channel,
	const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum, bool reliable)
{
	Address peer_address;

	if (peer->getAddress(MTP_UDP, peer_address)) {
		// We have to create a packet again for buffering
		// This isn't actually too bad an idea.
		BufferedPacket packet = makePacket(peer_address,
			packetdata,
			m_connection->GetProtocolID(),
			peer->id,
			channelnum);

		// Buffer the packet
		SharedBuffer<u8> data = peer->addSplitPacket(channelnum, packet, reliable);

		if (data.getSize() != 0) {
			LOG(dout_con << m_connection->getDesc()
				<< "RETURNING TYPE_SPLIT: Constructed full data, "
				<< "size=" << data.getSize() << std::endl);
			return data;
		}
		LOG(dout_con << m_connection->getDesc() << "BUFFERED TYPE_SPLIT" << std::endl);
		throw ProcessedSilentlyException("Buffered a split packet chunk");
	}

	// We should never get here.
	FATAL_ERROR("Invalid execution point");
}

SharedBuffer<u8> ConnectionReceiveThread::handlePacketType_Reliable(Channel *channel,
	const SharedBuffer<u8> &packetdata, Peer *peer, u8 channelnum, bool reliable)
{
	assert(channel != NULL);

	// Recursive reliable packets not allowed
	if (reliable)
		throw InvalidIncomingDataException("Found nested reliable packets");

	if (packetdata.getSize() < RELIABLE_HEADER_SIZE)
		throw InvalidIncomingDataException("packetdata.getSize() < RELIABLE_HEADER_SIZE");

	u16 seqnum = readU16(&packetdata[1]);
	bool is_future_packet = false;
	bool is_old_packet = false;

	/* packet is within our receive window send ack */
	if (seqnum_in_window(seqnum,
		channel->readNextIncomingSeqNum(), MAX_RELIABLE_WINDOW_SIZE)) {
		m_connection->sendAck(peer->id, channelnum, seqnum);
	} else {
		is_future_packet = seqnum_higher(seqnum, channel->readNextIncomingSeqNum());
		is_old_packet = seqnum_higher(channel->readNextIncomingSeqNum(), seqnum);

		/* packet is not within receive window, don't send ack.           *
		 * if this was a valid packet it's gonna be retransmitted         */
		if (is_future_packet)
			throw ProcessedSilentlyException(
				"Received packet newer then expected, not sending ack");

		/* seems like our ack was lost, send another one for a old packet */
		if (is_old_packet) {
			LOG(dout_con << m_connection->getDesc()
				<< "RE-SENDING ACK: peer_id: " << peer->id
				<< ", channel: " << (channelnum & 0xFF)
				<< ", seqnum: " << seqnum << std::endl;)
			m_connection->sendAck(peer->id, channelnum, seqnum);

			// we already have this packet so this one was on wire at least
			// the current timeout
			// we don't know how long this packet was on wire don't do silly guessing
			// dynamic_cast<UDPPeer*>(&peer)->
			//     reportRTT(dynamic_cast<UDPPeer*>(&peer)->getResendTimeout());

			throw ProcessedSilentlyException("Retransmitting ack for old packet");
		}
	}

	if (seqnum != channel->readNextIncomingSeqNum()) {
		Address peer_address;

		// this is a reliable packet so we have a udp address for sure
		peer->getAddress(MTP_MINETEST_RELIABLE_UDP, peer_address);
		// This one comes later, buffer it.
		// Actually we have to make a packet to buffer one.
		// Well, we have all the ingredients, so just do it.
		BufferedPacket packet = con::makePacket(
			peer_address,
			packetdata,
			m_connection->GetProtocolID(),
			peer->id,
			channelnum);
		try {
			channel->incoming_reliables.insert(packet, channel->readNextIncomingSeqNum());

			LOG(dout_con << m_connection->getDesc()
				<< "BUFFERING, TYPE_RELIABLE peer_id: " << peer->id
				<< ", channel: " << (channelnum & 0xFF)
				<< ", seqnum: " << seqnum << std::endl;)

			throw ProcessedQueued("Buffered future reliable packet");
		} catch (AlreadyExistsException &e) {
		} catch (IncomingDataCorruption &e) {
			ConnectionCommand discon;
			discon.disconnect_peer(peer->id);
			m_connection->putCommand(discon);

			LOG(derr_con << m_connection->getDesc()
				<< "INVALID, TYPE_RELIABLE peer_id: " << peer->id
				<< ", channel: " << (channelnum & 0xFF)
				<< ", seqnum: " << seqnum
				<< "DROPPING CLIENT!" << std::endl;)
		}
	}

	/* we got a packet to process right now */
	LOG(dout_con << m_connection->getDesc()
		<< "RECURSIVE, TYPE_RELIABLE peer_id: " << peer->id
		<< ", channel: " << (channelnum & 0xFF)
		<< ", seqnum: " << seqnum << std::endl;)


	/* check for resend case */
	u16 queued_seqnum = 0;
	if (channel->incoming_reliables.getFirstSeqnum(queued_seqnum)) {
		if (queued_seqnum == seqnum) {
			BufferedPacket queued_packet = channel->incoming_reliables.popFirst();
			/** TODO find a way to verify the new against the old packet */
		}
	}

	channel->incNextIncomingSeqNum();

	// Get out the inside packet and re-process it
	SharedBuffer<u8> payload(packetdata.getSize() - RELIABLE_HEADER_SIZE);
	memcpy(*payload, &packetdata[RELIABLE_HEADER_SIZE], payload.getSize());

	return processPacket(channel, payload, peer->id, channelnum, true);
}

}
