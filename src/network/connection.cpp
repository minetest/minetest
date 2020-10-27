/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <iomanip>
#include <cerrno>
#include <algorithm>
#include <cmath>
#include "connection.h"
#include "serialization.h"
#include "log.h"
#include "porting.h"
#include "network/connectionthreads.h"
#include "network/networkpacket.h"
#include "network/peerhandler.h"
#include "util/serialize.h"
#include "util/numeric.h"
#include "util/string.h"
#include "settings.h"
#include "profiler.h"

namespace con
{

/******************************************************************************/
/* defines used for debugging and profiling                                   */
/******************************************************************************/
#ifdef NDEBUG
	#define LOG(a) a
	#define PROFILE(a)
#else
	#if 0
	/* this mutex is used to achieve log message consistency */
	std::mutex log_message_mutex;
	#define LOG(a)                                                                 \
		{                                                                          \
		MutexAutoLock loglock(log_message_mutex);                                 \
		a;                                                                         \
		}
	#else
	// Prevent deadlocks until a solution is found after 5.2.0 (TODO)
	#define LOG(a) a
	#endif

	#define PROFILE(a) a
#endif

#define PING_TIMEOUT 5.0

BufferedPacket makePacket(Address &address, const SharedBuffer<u8> &data,
		u32 protocol_id, session_t sender_peer_id, u8 channel)
{
	u32 packet_size = data.getSize() + BASE_HEADER_SIZE;
	BufferedPacket p(packet_size);
	p.address = address;

	writeU32(&p.data[0], protocol_id);
	writeU16(&p.data[4], sender_peer_id);
	writeU8(&p.data[6], channel);

	memcpy(&p.data[BASE_HEADER_SIZE], *data, data.getSize());

	return p;
}

SharedBuffer<u8> makeOriginalPacket(const SharedBuffer<u8> &data)
{
	u32 header_size = 1;
	u32 packet_size = data.getSize() + header_size;
	SharedBuffer<u8> b(packet_size);

	writeU8(&(b[0]), PACKET_TYPE_ORIGINAL);
	if (data.getSize() > 0) {
		memcpy(&(b[header_size]), *data, data.getSize());
	}
	return b;
}

// Split data in chunks and add TYPE_SPLIT headers to them
void makeSplitPacket(const SharedBuffer<u8> &data, u32 chunksize_max, u16 seqnum,
		std::list<SharedBuffer<u8>> *chunks)
{
	// Chunk packets, containing the TYPE_SPLIT header
	u32 chunk_header_size = 7;
	u32 maximum_data_size = chunksize_max - chunk_header_size;
	u32 start = 0;
	u32 end = 0;
	u32 chunk_num = 0;
	u16 chunk_count = 0;
	do {
		end = start + maximum_data_size - 1;
		if (end > data.getSize() - 1)
			end = data.getSize() - 1;

		u32 payload_size = end - start + 1;
		u32 packet_size = chunk_header_size + payload_size;

		SharedBuffer<u8> chunk(packet_size);

		writeU8(&chunk[0], PACKET_TYPE_SPLIT);
		writeU16(&chunk[1], seqnum);
		// [3] u16 chunk_count is written at next stage
		writeU16(&chunk[5], chunk_num);
		memcpy(&chunk[chunk_header_size], &data[start], payload_size);

		chunks->push_back(chunk);
		chunk_count++;

		start = end + 1;
		chunk_num++;
	}
	while (end != data.getSize() - 1);

	for (SharedBuffer<u8> &chunk : *chunks) {
		// Write chunk_count
		writeU16(&(chunk[3]), chunk_count);
	}
}

void makeAutoSplitPacket(const SharedBuffer<u8> &data, u32 chunksize_max,
		u16 &split_seqnum, std::list<SharedBuffer<u8>> *list)
{
	u32 original_header_size = 1;

	if (data.getSize() + original_header_size > chunksize_max) {
		makeSplitPacket(data, chunksize_max, split_seqnum, list);
		split_seqnum++;
		return;
	}

	list->push_back(makeOriginalPacket(data));
}

SharedBuffer<u8> makeReliablePacket(const SharedBuffer<u8> &data, u16 seqnum)
{
	u32 header_size = 3;
	u32 packet_size = data.getSize() + header_size;
	SharedBuffer<u8> b(packet_size);

	writeU8(&b[0], PACKET_TYPE_RELIABLE);
	writeU16(&b[1], seqnum);

	memcpy(&b[header_size], *data, data.getSize());

	return b;
}

/*
	ReliablePacketBuffer
*/

void ReliablePacketBuffer::print()
{
	MutexAutoLock listlock(m_list_mutex);
	LOG(dout_con<<"Dump of ReliablePacketBuffer:" << std::endl);
	unsigned int index = 0;
	for (BufferedPacket &bufferedPacket : m_list) {
		u16 s = readU16(&(bufferedPacket.data[BASE_HEADER_SIZE+1]));
		LOG(dout_con<<index<< ":" << s << std::endl);
		index++;
	}
}

bool ReliablePacketBuffer::empty()
{
	MutexAutoLock listlock(m_list_mutex);
	return m_list.empty();
}

u32 ReliablePacketBuffer::size()
{
	MutexAutoLock listlock(m_list_mutex);
	return m_list.size();
}

RPBSearchResult ReliablePacketBuffer::findPacket(u16 seqnum)
{
	std::list<BufferedPacket>::iterator i = m_list.begin();
	for(; i != m_list.end(); ++i)
	{
		u16 s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		if (s == seqnum)
			break;
	}
	return i;
}

RPBSearchResult ReliablePacketBuffer::notFound()
{
	return m_list.end();
}

bool ReliablePacketBuffer::getFirstSeqnum(u16& result)
{
	MutexAutoLock listlock(m_list_mutex);
	if (m_list.empty())
		return false;
	const BufferedPacket &p = *m_list.begin();
	result = readU16(&p.data[BASE_HEADER_SIZE + 1]);
	return true;
}

BufferedPacket ReliablePacketBuffer::popFirst()
{
	MutexAutoLock listlock(m_list_mutex);
	if (m_list.empty())
		throw NotFoundException("Buffer is empty");
	BufferedPacket p = *m_list.begin();
	m_list.erase(m_list.begin());

	if (m_list.empty()) {
		m_oldest_non_answered_ack = 0;
	} else {
		m_oldest_non_answered_ack =
				readU16(&m_list.begin()->data[BASE_HEADER_SIZE + 1]);
	}
	return p;
}

BufferedPacket ReliablePacketBuffer::popSeqnum(u16 seqnum)
{
	MutexAutoLock listlock(m_list_mutex);
	RPBSearchResult r = findPacket(seqnum);
	if (r == notFound()) {
		LOG(dout_con<<"Sequence number: " << seqnum
				<< " not found in reliable buffer"<<std::endl);
		throw NotFoundException("seqnum not found in buffer");
	}
	BufferedPacket p = *r;


	RPBSearchResult next = r;
	++next;
	if (next != notFound()) {
		u16 s = readU16(&(next->data[BASE_HEADER_SIZE+1]));
		m_oldest_non_answered_ack = s;
	}

	m_list.erase(r);

	if (m_list.empty()) {
		m_oldest_non_answered_ack = 0;
	} else {
		m_oldest_non_answered_ack =
				readU16(&m_list.begin()->data[BASE_HEADER_SIZE + 1]);
	}
	return p;
}

void ReliablePacketBuffer::insert(BufferedPacket &p, u16 next_expected)
{
	MutexAutoLock listlock(m_list_mutex);
	if (p.data.getSize() < BASE_HEADER_SIZE + 3) {
		errorstream << "ReliablePacketBuffer::insert(): Invalid data size for "
			"reliable packet" << std::endl;
		return;
	}
	u8 type = readU8(&p.data[BASE_HEADER_SIZE + 0]);
	if (type != PACKET_TYPE_RELIABLE) {
		errorstream << "ReliablePacketBuffer::insert(): type is not reliable"
			<< std::endl;
		return;
	}
	u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE + 1]);

	if (!seqnum_in_window(seqnum, next_expected, MAX_RELIABLE_WINDOW_SIZE)) {
		errorstream << "ReliablePacketBuffer::insert(): seqnum is outside of "
			"expected window " << std::endl;
		return;
	}
	if (seqnum == next_expected) {
		errorstream << "ReliablePacketBuffer::insert(): seqnum is next expected"
			<< std::endl;
		return;
	}

	sanity_check(m_list.size() <= SEQNUM_MAX); // FIXME: Handle the error?

	// Find the right place for the packet and insert it there
	// If list is empty, just add it
	if (m_list.empty())
	{
		m_list.push_back(p);
		m_oldest_non_answered_ack = seqnum;
		// Done.
		return;
	}

	// Otherwise find the right place
	std::list<BufferedPacket>::iterator i = m_list.begin();
	// Find the first packet in the list which has a higher seqnum
	u16 s = readU16(&(i->data[BASE_HEADER_SIZE+1]));

	/* case seqnum is smaller then next_expected seqnum */
	/* this is true e.g. on wrap around */
	if (seqnum < next_expected) {
		while(((s < seqnum) || (s >= next_expected)) && (i != m_list.end())) {
			++i;
			if (i != m_list.end())
				s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		}
	}
	/* non wrap around case (at least for incoming and next_expected */
	else
	{
		while(((s < seqnum) && (s >= next_expected)) && (i != m_list.end())) {
			++i;
			if (i != m_list.end())
				s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		}
	}

	if (s == seqnum) {
		/* nothing to do this seems to be a resent packet */
		/* for paranoia reason data should be compared */
		if (
			(readU16(&(i->data[BASE_HEADER_SIZE+1])) != seqnum) ||
			(i->data.getSize() != p.data.getSize()) ||
			(i->address != p.address)
			)
		{
			/* if this happens your maximum transfer window may be to big */
			fprintf(stderr,
					"Duplicated seqnum %d non matching packet detected:\n",
					seqnum);
			fprintf(stderr, "Old: seqnum: %05d size: %04d, address: %s\n",
					readU16(&(i->data[BASE_HEADER_SIZE+1])),i->data.getSize(),
					i->address.serializeString().c_str());
			fprintf(stderr, "New: seqnum: %05d size: %04u, address: %s\n",
					readU16(&(p.data[BASE_HEADER_SIZE+1])),p.data.getSize(),
					p.address.serializeString().c_str());
			throw IncomingDataCorruption("duplicated packet isn't same as original one");
		}
	}
	/* insert or push back */
	else if (i != m_list.end()) {
		m_list.insert(i, p);
	} else {
		m_list.push_back(p);
	}

	/* update last packet number */
	m_oldest_non_answered_ack = readU16(&(*m_list.begin()).data[BASE_HEADER_SIZE+1]);
}

void ReliablePacketBuffer::incrementTimeouts(float dtime)
{
	MutexAutoLock listlock(m_list_mutex);
	for (BufferedPacket &bufferedPacket : m_list) {
		bufferedPacket.time += dtime;
		bufferedPacket.totaltime += dtime;
	}
}

std::list<BufferedPacket> ReliablePacketBuffer::getTimedOuts(float timeout,
													unsigned int max_packets)
{
	MutexAutoLock listlock(m_list_mutex);
	std::list<BufferedPacket> timed_outs;
	for (BufferedPacket &bufferedPacket : m_list) {
		if (bufferedPacket.time >= timeout) {
			timed_outs.push_back(bufferedPacket);

			//this packet will be sent right afterwards reset timeout here
			bufferedPacket.time = 0.0f;
			if (timed_outs.size() >= max_packets)
				break;
		}
	}
	return timed_outs;
}

/*
	IncomingSplitPacket
*/

bool IncomingSplitPacket::insert(u32 chunk_num, SharedBuffer<u8> &chunkdata)
{
	sanity_check(chunk_num < chunk_count);

	// If chunk already exists, ignore it.
	// Sometimes two identical packets may arrive when there is network
	// lag and the server re-sends stuff.
	if (chunks.find(chunk_num) != chunks.end())
		return false;

	// Set chunk data in buffer
	chunks[chunk_num] = chunkdata;

	return true;
}

SharedBuffer<u8> IncomingSplitPacket::reassemble()
{
	sanity_check(allReceived());

	// Calculate total size
	u32 totalsize = 0;
	for (const auto &chunk : chunks)
		totalsize += chunk.second.getSize();

	SharedBuffer<u8> fulldata(totalsize);

	// Copy chunks to data buffer
	u32 start = 0;
	for (u32 chunk_i = 0; chunk_i < chunk_count; chunk_i++) {
		const SharedBuffer<u8> &buf = chunks[chunk_i];
		memcpy(&fulldata[start], *buf, buf.getSize());
		start += buf.getSize();
	}

	return fulldata;
}

/*
	IncomingSplitBuffer
*/

IncomingSplitBuffer::~IncomingSplitBuffer()
{
	MutexAutoLock listlock(m_map_mutex);
	for (auto &i : m_buf) {
		delete i.second;
	}
}

SharedBuffer<u8> IncomingSplitBuffer::insert(const BufferedPacket &p, bool reliable)
{
	MutexAutoLock listlock(m_map_mutex);
	u32 headersize = BASE_HEADER_SIZE + 7;
	if (p.data.getSize() < headersize) {
		errorstream << "Invalid data size for split packet" << std::endl;
		return SharedBuffer<u8>();
	}
	u8 type = readU8(&p.data[BASE_HEADER_SIZE+0]);
	u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);
	u16 chunk_count = readU16(&p.data[BASE_HEADER_SIZE+3]);
	u16 chunk_num = readU16(&p.data[BASE_HEADER_SIZE+5]);

	if (type != PACKET_TYPE_SPLIT) {
		errorstream << "IncomingSplitBuffer::insert(): type is not split"
			<< std::endl;
		return SharedBuffer<u8>();
	}
	if (chunk_num >= chunk_count) {
		errorstream << "IncomingSplitBuffer::insert(): chunk_num=" << chunk_num
				<< " >= chunk_count=" << chunk_count << std::endl;
		return SharedBuffer<u8>();
	}

	// Add if doesn't exist
	IncomingSplitPacket *sp;
	if (m_buf.find(seqnum) == m_buf.end()) {
		sp = new IncomingSplitPacket(chunk_count, reliable);
		m_buf[seqnum] = sp;
	} else {
		sp = m_buf[seqnum];
	}

	if (chunk_count != sp->chunk_count) {
		errorstream << "IncomingSplitBuffer::insert(): chunk_count="
				<< chunk_count << " != sp->chunk_count=" << sp->chunk_count
				<< std::endl;
		return SharedBuffer<u8>();
	}
	if (reliable != sp->reliable)
		LOG(derr_con<<"Connection: WARNING: reliable="<<reliable
				<<" != sp->reliable="<<sp->reliable
				<<std::endl);

	// Cut chunk data out of packet
	u32 chunkdatasize = p.data.getSize() - headersize;
	SharedBuffer<u8> chunkdata(chunkdatasize);
	memcpy(*chunkdata, &(p.data[headersize]), chunkdatasize);

	if (!sp->insert(chunk_num, chunkdata))
		return SharedBuffer<u8>();

	// If not all chunks are received, return empty buffer
	if (!sp->allReceived())
		return SharedBuffer<u8>();

	SharedBuffer<u8> fulldata = sp->reassemble();

	// Remove sp from buffer
	m_buf.erase(seqnum);
	delete sp;

	return fulldata;
}

void IncomingSplitBuffer::removeUnreliableTimedOuts(float dtime, float timeout)
{
	std::deque<u16> remove_queue;
	{
		MutexAutoLock listlock(m_map_mutex);
		for (auto &i : m_buf) {
			IncomingSplitPacket *p = i.second;
			// Reliable ones are not removed by timeout
			if (p->reliable)
				continue;
			p->time += dtime;
			if (p->time >= timeout)
				remove_queue.push_back(i.first);
		}
	}
	for (u16 j : remove_queue) {
		MutexAutoLock listlock(m_map_mutex);
		LOG(dout_con<<"NOTE: Removing timed out unreliable split packet"<<std::endl);
		delete m_buf[j];
		m_buf.erase(j);
	}
}

/*
	ConnectionCommand
 */

void ConnectionCommand::send(session_t peer_id_, u8 channelnum_, NetworkPacket *pkt,
	bool reliable_)
{
	type = CONNCMD_SEND;
	peer_id = peer_id_;
	channelnum = channelnum_;
	data = pkt->oldForgePacket();
	reliable = reliable_;
}

/*
	Channel
*/

u16 Channel::readNextIncomingSeqNum()
{
	MutexAutoLock internal(m_internal_mutex);
	return next_incoming_seqnum;
}

u16 Channel::incNextIncomingSeqNum()
{
	MutexAutoLock internal(m_internal_mutex);
	u16 retval = next_incoming_seqnum;
	next_incoming_seqnum++;
	return retval;
}

u16 Channel::readNextSplitSeqNum()
{
	MutexAutoLock internal(m_internal_mutex);
	return next_outgoing_split_seqnum;
}
void Channel::setNextSplitSeqNum(u16 seqnum)
{
	MutexAutoLock internal(m_internal_mutex);
	next_outgoing_split_seqnum = seqnum;
}

u16 Channel::getOutgoingSequenceNumber(bool& successful)
{
	MutexAutoLock internal(m_internal_mutex);
	u16 retval = next_outgoing_seqnum;
	u16 lowest_unacked_seqnumber;

	/* shortcut if there ain't any packet in outgoing list */
	if (outgoing_reliables_sent.empty())
	{
		next_outgoing_seqnum++;
		return retval;
	}

	if (outgoing_reliables_sent.getFirstSeqnum(lowest_unacked_seqnumber))
	{
		if (lowest_unacked_seqnumber < next_outgoing_seqnum) {
			// ugly cast but this one is required in order to tell compiler we
			// know about difference of two unsigned may be negative in general
			// but we already made sure it won't happen in this case
			if (((u16)(next_outgoing_seqnum - lowest_unacked_seqnumber)) > window_size) {
				successful = false;
				return 0;
			}
		}
		else {
			// ugly cast but this one is required in order to tell compiler we
			// know about difference of two unsigned may be negative in general
			// but we already made sure it won't happen in this case
			if ((next_outgoing_seqnum + (u16)(SEQNUM_MAX - lowest_unacked_seqnumber)) >
				window_size) {
				successful = false;
				return 0;
			}
		}
	}

	next_outgoing_seqnum++;
	return retval;
}

u16 Channel::readOutgoingSequenceNumber()
{
	MutexAutoLock internal(m_internal_mutex);
	return next_outgoing_seqnum;
}

bool Channel::putBackSequenceNumber(u16 seqnum)
{
	if (((seqnum + 1) % (SEQNUM_MAX+1)) == next_outgoing_seqnum) {

		next_outgoing_seqnum = seqnum;
		return true;
	}
	return false;
}

void Channel::UpdateBytesSent(unsigned int bytes, unsigned int packets)
{
	MutexAutoLock internal(m_internal_mutex);
	current_bytes_transfered += bytes;
	current_packet_successful += packets;
}

void Channel::UpdateBytesReceived(unsigned int bytes) {
	MutexAutoLock internal(m_internal_mutex);
	current_bytes_received += bytes;
}

void Channel::UpdateBytesLost(unsigned int bytes)
{
	MutexAutoLock internal(m_internal_mutex);
	current_bytes_lost += bytes;
}


void Channel::UpdatePacketLossCounter(unsigned int count)
{
	MutexAutoLock internal(m_internal_mutex);
	current_packet_loss += count;
}

void Channel::UpdatePacketTooLateCounter()
{
	MutexAutoLock internal(m_internal_mutex);
	current_packet_too_late++;
}

void Channel::UpdateTimers(float dtime)
{
	bpm_counter += dtime;
	packet_loss_counter += dtime;

	if (packet_loss_counter > 1.0f) {
		packet_loss_counter -= 1.0f;

		unsigned int packet_loss = 11; /* use a neutral value for initialization */
		unsigned int packets_successful = 0;
		//unsigned int packet_too_late = 0;

		bool reasonable_amount_of_data_transmitted = false;

		{
			MutexAutoLock internal(m_internal_mutex);
			packet_loss = current_packet_loss;
			//packet_too_late = current_packet_too_late;
			packets_successful = current_packet_successful;

			if (current_bytes_transfered > (unsigned int) (window_size*512/2)) {
				reasonable_amount_of_data_transmitted = true;
			}
			current_packet_loss = 0;
			current_packet_too_late = 0;
			current_packet_successful = 0;
		}

		/* dynamic window size */
		float successful_to_lost_ratio = 0.0f;
		bool done = false;

		if (packets_successful > 0) {
			successful_to_lost_ratio = packet_loss/packets_successful;
		} else if (packet_loss > 0) {
			window_size = std::max(
					(window_size - 10),
					MIN_RELIABLE_WINDOW_SIZE);
			done = true;
		}

		if (!done) {
			if ((successful_to_lost_ratio < 0.01f) &&
				(window_size < MAX_RELIABLE_WINDOW_SIZE)) {
				/* don't even think about increasing if we didn't even
				 * use major parts of our window */
				if (reasonable_amount_of_data_transmitted)
					window_size = std::min(
							(window_size + 100),
							MAX_RELIABLE_WINDOW_SIZE);
			} else if ((successful_to_lost_ratio < 0.05f) &&
					(window_size < MAX_RELIABLE_WINDOW_SIZE)) {
				/* don't even think about increasing if we didn't even
				 * use major parts of our window */
				if (reasonable_amount_of_data_transmitted)
					window_size = std::min(
							(window_size + 50),
							MAX_RELIABLE_WINDOW_SIZE);
			} else if (successful_to_lost_ratio > 0.15f) {
				window_size = std::max(
						(window_size - 100),
						MIN_RELIABLE_WINDOW_SIZE);
			} else if (successful_to_lost_ratio > 0.1f) {
				window_size = std::max(
						(window_size - 50),
						MIN_RELIABLE_WINDOW_SIZE);
			}
		}
	}

	if (bpm_counter > 10.0f) {
		{
			MutexAutoLock internal(m_internal_mutex);
			cur_kbps                 =
					(((float) current_bytes_transfered)/bpm_counter)/1024.0f;
			current_bytes_transfered = 0;
			cur_kbps_lost            =
					(((float) current_bytes_lost)/bpm_counter)/1024.0f;
			current_bytes_lost       = 0;
			cur_incoming_kbps        =
					(((float) current_bytes_received)/bpm_counter)/1024.0f;
			current_bytes_received   = 0;
			bpm_counter              = 0.0f;
		}

		if (cur_kbps > max_kbps) {
			max_kbps = cur_kbps;
		}

		if (cur_kbps_lost > max_kbps_lost) {
			max_kbps_lost = cur_kbps_lost;
		}

		if (cur_incoming_kbps > max_incoming_kbps) {
			max_incoming_kbps = cur_incoming_kbps;
		}

		rate_samples       = MYMIN(rate_samples+1,10);
		float old_fraction = ((float) (rate_samples-1) )/( (float) rate_samples);
		avg_kbps           = avg_kbps * old_fraction +
				cur_kbps * (1.0 - old_fraction);
		avg_kbps_lost      = avg_kbps_lost * old_fraction +
				cur_kbps_lost * (1.0 - old_fraction);
		avg_incoming_kbps  = avg_incoming_kbps * old_fraction +
				cur_incoming_kbps * (1.0 - old_fraction);
	}
}


/*
	Peer
*/

PeerHelper::PeerHelper(Peer* peer) :
	m_peer(peer)
{
	if (peer && !peer->IncUseCount())
		m_peer = nullptr;
}

PeerHelper::~PeerHelper()
{
	if (m_peer)
		m_peer->DecUseCount();

	m_peer = nullptr;
}

PeerHelper& PeerHelper::operator=(Peer* peer)
{
	m_peer = peer;
	if (peer && !peer->IncUseCount())
		m_peer = nullptr;
	return *this;
}

Peer* PeerHelper::operator->() const
{
	return m_peer;
}

Peer* PeerHelper::operator&() const
{
	return m_peer;
}

bool PeerHelper::operator!()
{
	return ! m_peer;
}

bool PeerHelper::operator!=(void* ptr)
{
	return ((void*) m_peer != ptr);
}

bool Peer::IncUseCount()
{
	MutexAutoLock lock(m_exclusive_access_mutex);

	if (!m_pending_deletion) {
		this->m_usage++;
		return true;
	}

	return false;
}

void Peer::DecUseCount()
{
	{
		MutexAutoLock lock(m_exclusive_access_mutex);
		sanity_check(m_usage > 0);
		m_usage--;

		if (!((m_pending_deletion) && (m_usage == 0)))
			return;
	}
	delete this;
}

void Peer::RTTStatistics(float rtt, const std::string &profiler_id,
		unsigned int num_samples) {

	if (m_last_rtt > 0) {
		/* set min max values */
		if (rtt < m_rtt.min_rtt)
			m_rtt.min_rtt = rtt;
		if (rtt >= m_rtt.max_rtt)
			m_rtt.max_rtt = rtt;

		/* do average calculation */
		if (m_rtt.avg_rtt < 0.0)
			m_rtt.avg_rtt  = rtt;
		else
			m_rtt.avg_rtt  = m_rtt.avg_rtt * (num_samples/(num_samples-1)) +
								rtt * (1/num_samples);

		/* do jitter calculation */

		//just use some neutral value at beginning
		float jitter = m_rtt.jitter_min;

		if (rtt > m_last_rtt)
			jitter = rtt-m_last_rtt;

		if (rtt <= m_last_rtt)
			jitter = m_last_rtt - rtt;

		if (jitter < m_rtt.jitter_min)
			m_rtt.jitter_min = jitter;
		if (jitter >= m_rtt.jitter_max)
			m_rtt.jitter_max = jitter;

		if (m_rtt.jitter_avg < 0.0)
			m_rtt.jitter_avg  = jitter;
		else
			m_rtt.jitter_avg  = m_rtt.jitter_avg * (num_samples/(num_samples-1)) +
								jitter * (1/num_samples);

		if (!profiler_id.empty()) {
			g_profiler->graphAdd(profiler_id + " RTT [ms]", rtt * 1000.f);
			g_profiler->graphAdd(profiler_id + " jitter [ms]", jitter * 1000.f);
		}
	}
	/* save values required for next loop */
	m_last_rtt = rtt;
}

bool Peer::isTimedOut(float timeout)
{
	MutexAutoLock lock(m_exclusive_access_mutex);
	u64 current_time = porting::getTimeMs();

	float dtime = CALC_DTIME(m_last_timeout_check,current_time);
	m_last_timeout_check = current_time;

	m_timeout_counter += dtime;

	return m_timeout_counter > timeout;
}

void Peer::Drop()
{
	{
		MutexAutoLock usage_lock(m_exclusive_access_mutex);
		m_pending_deletion = true;
		if (m_usage != 0)
			return;
	}

	PROFILE(std::stringstream peerIdentifier1);
	PROFILE(peerIdentifier1 << "runTimeouts[" << m_connection->getDesc()
			<< ";" << id << ";RELIABLE]");
	PROFILE(g_profiler->remove(peerIdentifier1.str()));
	PROFILE(std::stringstream peerIdentifier2);
	PROFILE(peerIdentifier2 << "sendPackets[" << m_connection->getDesc()
			<< ";" << id << ";RELIABLE]");
	PROFILE(ScopeProfiler peerprofiler(g_profiler, peerIdentifier2.str(), SPT_AVG));

	delete this;
}

UDPPeer::UDPPeer(u16 a_id, Address a_address, Connection* connection) :
	Peer(a_address,a_id,connection)
{
	for (Channel &channel : channels)
		channel.setWindowSize(START_RELIABLE_WINDOW_SIZE);
}

bool UDPPeer::getAddress(MTProtocols type,Address& toset)
{
	if ((type == MTP_UDP) || (type == MTP_MINETEST_RELIABLE_UDP) || (type == MTP_PRIMARY))
	{
		toset = address;
		return true;
	}

	return false;
}

void UDPPeer::reportRTT(float rtt)
{
	if (rtt < 0.0) {
		return;
	}
	RTTStatistics(rtt,"rudp",MAX_RELIABLE_WINDOW_SIZE*10);

	float timeout = getStat(AVG_RTT) * RESEND_TIMEOUT_FACTOR;
	if (timeout < RESEND_TIMEOUT_MIN)
		timeout = RESEND_TIMEOUT_MIN;
	if (timeout > RESEND_TIMEOUT_MAX)
		timeout = RESEND_TIMEOUT_MAX;

	MutexAutoLock usage_lock(m_exclusive_access_mutex);
	resend_timeout = timeout;
}

bool UDPPeer::Ping(float dtime,SharedBuffer<u8>& data)
{
	m_ping_timer += dtime;
	if (m_ping_timer >= PING_TIMEOUT)
	{
		// Create and send PING packet
		writeU8(&data[0], PACKET_TYPE_CONTROL);
		writeU8(&data[1], CONTROLTYPE_PING);
		m_ping_timer = 0.0;
		return true;
	}
	return false;
}

void UDPPeer::PutReliableSendCommand(ConnectionCommand &c,
		unsigned int max_packet_size)
{
	if (m_pending_disconnect)
		return;

	Channel &chan = channels[c.channelnum];

	if (chan.queued_commands.empty() &&
			/* don't queue more packets then window size */
			(chan.queued_reliables.size() < chan.getWindowSize() / 2)) {
		LOG(dout_con<<m_connection->getDesc()
				<<" processing reliable command for peer id: " << c.peer_id
				<<" data size: " << c.data.getSize() << std::endl);
		if (!processReliableSendCommand(c,max_packet_size)) {
			chan.queued_commands.push_back(c);
		}
	}
	else {
		LOG(dout_con<<m_connection->getDesc()
				<<" Queueing reliable command for peer id: " << c.peer_id
				<<" data size: " << c.data.getSize() <<std::endl);
		chan.queued_commands.push_back(c);
		if (chan.queued_commands.size() >= chan.getWindowSize() / 2) {
			LOG(derr_con << m_connection->getDesc()
					<< "Possible packet stall to peer id: " << c.peer_id
					<< " queued_commands=" << chan.queued_commands.size()
					<< std::endl);
		}
	}
}

bool UDPPeer::processReliableSendCommand(
				ConnectionCommand &c,
				unsigned int max_packet_size)
{
	if (m_pending_disconnect)
		return true;

	Channel &chan = channels[c.channelnum];

	u32 chunksize_max = max_packet_size
							- BASE_HEADER_SIZE
							- RELIABLE_HEADER_SIZE;

	sanity_check(c.data.getSize() < MAX_RELIABLE_WINDOW_SIZE*512);

	std::list<SharedBuffer<u8>> originals;
	u16 split_sequence_number = chan.readNextSplitSeqNum();

	if (c.raw) {
		originals.emplace_back(c.data);
	} else {
		makeAutoSplitPacket(c.data, chunksize_max,split_sequence_number, &originals);
		chan.setNextSplitSeqNum(split_sequence_number);
	}

	bool have_sequence_number = true;
	bool have_initial_sequence_number = false;
	std::queue<BufferedPacket> toadd;
	volatile u16 initial_sequence_number = 0;

	for (SharedBuffer<u8> &original : originals) {
		u16 seqnum = chan.getOutgoingSequenceNumber(have_sequence_number);

		/* oops, we don't have enough sequence numbers to send this packet */
		if (!have_sequence_number)
			break;

		if (!have_initial_sequence_number)
		{
			initial_sequence_number = seqnum;
			have_initial_sequence_number = true;
		}

		SharedBuffer<u8> reliable = makeReliablePacket(original, seqnum);

		// Add base headers and make a packet
		BufferedPacket p = con::makePacket(address, reliable,
				m_connection->GetProtocolID(), m_connection->GetPeerID(),
				c.channelnum);

		toadd.push(p);
	}

	if (have_sequence_number) {
		volatile u16 pcount = 0;
		while (!toadd.empty()) {
			BufferedPacket p = toadd.front();
			toadd.pop();
//			LOG(dout_con<<connection->getDesc()
//					<< " queuing reliable packet for peer_id: " << c.peer_id
//					<< " channel: " << (c.channelnum&0xFF)
//					<< " seqnum: " << readU16(&p.data[BASE_HEADER_SIZE+1])
//					<< std::endl)
			chan.queued_reliables.push(p);
			pcount++;
		}
		sanity_check(chan.queued_reliables.size() < 0xFFFF);
		return true;
	}

	volatile u16 packets_available = toadd.size();
	/* we didn't get a single sequence number no need to fill queue */
	if (!have_initial_sequence_number) {
		return false;
	}

	while (!toadd.empty()) {
		/* remove packet */
		toadd.pop();

		bool successfully_put_back_sequence_number
			= chan.putBackSequenceNumber(
				(initial_sequence_number+toadd.size() % (SEQNUM_MAX+1)));

		FATAL_ERROR_IF(!successfully_put_back_sequence_number, "error");
	}

	// DO NOT REMOVE n_queued! It avoids a deadlock of async locked
	// 'log_message_mutex' and 'm_list_mutex'.
	u32 n_queued = chan.outgoing_reliables_sent.size();

	LOG(dout_con<<m_connection->getDesc()
			<< " Windowsize exceeded on reliable sending "
			<< c.data.getSize() << " bytes"
			<< std::endl << "\t\tinitial_sequence_number: "
			<< initial_sequence_number
			<< std::endl << "\t\tgot at most            : "
			<< packets_available << " packets"
			<< std::endl << "\t\tpackets queued         : "
			<< n_queued
			<< std::endl);

	return false;
}

void UDPPeer::RunCommandQueues(
							unsigned int max_packet_size,
							unsigned int maxcommands,
							unsigned int maxtransfer)
{

	for (Channel &channel : channels) {
		unsigned int commands_processed = 0;

		if ((!channel.queued_commands.empty()) &&
				(channel.queued_reliables.size() < maxtransfer) &&
				(commands_processed < maxcommands)) {
			try {
				ConnectionCommand c = channel.queued_commands.front();

				LOG(dout_con << m_connection->getDesc()
						<< " processing queued reliable command " << std::endl);

				// Packet is processed, remove it from queue
				if (processReliableSendCommand(c,max_packet_size)) {
					channel.queued_commands.pop_front();
				} else {
					LOG(dout_con << m_connection->getDesc()
							<< " Failed to queue packets for peer_id: " << c.peer_id
							<< ", delaying sending of " << c.data.getSize()
							<< " bytes" << std::endl);
				}
			}
			catch (ItemNotFoundException &e) {
				// intentionally empty
			}
		}
	}
}

u16 UDPPeer::getNextSplitSequenceNumber(u8 channel)
{
	assert(channel < CHANNEL_COUNT); // Pre-condition
	return channels[channel].readNextSplitSeqNum();
}

void UDPPeer::setNextSplitSequenceNumber(u8 channel, u16 seqnum)
{
	assert(channel < CHANNEL_COUNT); // Pre-condition
	channels[channel].setNextSplitSeqNum(seqnum);
}

SharedBuffer<u8> UDPPeer::addSplitPacket(u8 channel, const BufferedPacket &toadd,
	bool reliable)
{
	assert(channel < CHANNEL_COUNT); // Pre-condition
	return channels[channel].incoming_splits.insert(toadd, reliable);
}

/*
	Connection
*/

Connection::Connection(u32 protocol_id, u32 max_packet_size, float timeout,
		bool ipv6, PeerHandler *peerhandler) :
	m_udpSocket(ipv6),
	m_protocol_id(protocol_id),
	m_sendThread(new ConnectionSendThread(max_packet_size, timeout)),
	m_receiveThread(new ConnectionReceiveThread(max_packet_size)),
	m_bc_peerhandler(peerhandler)

{
	/* Amount of time Receive() will wait for data, this is entirely different
	 * from the connection timeout */
	m_udpSocket.setTimeoutMs(500);

	m_sendThread->setParent(this);
	m_receiveThread->setParent(this);

	m_sendThread->start();
	m_receiveThread->start();
}


Connection::~Connection()
{
	m_shutting_down = true;
	// request threads to stop
	m_sendThread->stop();
	m_receiveThread->stop();

	//TODO for some unkonwn reason send/receive threads do not exit as they're
	// supposed to be but wait on peer timeout. To speed up shutdown we reduce
	// timeout to half a second.
	m_sendThread->setPeerTimeout(0.5);

	// wait for threads to finish
	m_sendThread->wait();
	m_receiveThread->wait();

	// Delete peers
	for (auto &peer : m_peers) {
		delete peer.second;
	}
}

/* Internal stuff */
void Connection::putEvent(ConnectionEvent &e)
{
	assert(e.type != CONNEVENT_NONE); // Pre-condition
	m_event_queue.push_back(e);
}

void Connection::TriggerSend()
{
	m_sendThread->Trigger();
}

PeerHelper Connection::getPeerNoEx(session_t peer_id)
{
	MutexAutoLock peerlock(m_peers_mutex);
	std::map<session_t, Peer *>::iterator node = m_peers.find(peer_id);

	if (node == m_peers.end()) {
		return PeerHelper(NULL);
	}

	// Error checking
	FATAL_ERROR_IF(node->second->id != peer_id, "Invalid peer id");

	return PeerHelper(node->second);
}

/* find peer_id for address */
u16 Connection::lookupPeer(Address& sender)
{
	MutexAutoLock peerlock(m_peers_mutex);
	std::map<u16, Peer*>::iterator j;
	j = m_peers.begin();
	for(; j != m_peers.end(); ++j)
	{
		Peer *peer = j->second;
		if (peer->isPendingDeletion())
			continue;

		Address tocheck;

		if ((peer->getAddress(MTP_MINETEST_RELIABLE_UDP, tocheck)) && (tocheck == sender))
			return peer->id;

		if ((peer->getAddress(MTP_UDP, tocheck)) && (tocheck == sender))
			return peer->id;
	}

	return PEER_ID_INEXISTENT;
}

bool Connection::deletePeer(session_t peer_id, bool timeout)
{
	Peer *peer = 0;

	/* lock list as short as possible */
	{
		MutexAutoLock peerlock(m_peers_mutex);
		if (m_peers.find(peer_id) == m_peers.end())
			return false;
		peer = m_peers[peer_id];
		m_peers.erase(peer_id);
		auto it = std::find(m_peer_ids.begin(), m_peer_ids.end(), peer_id);
		m_peer_ids.erase(it);
	}

	Address peer_address;
	//any peer has a primary address this never fails!
	peer->getAddress(MTP_PRIMARY, peer_address);
	// Create event
	ConnectionEvent e;
	e.peerRemoved(peer_id, timeout, peer_address);
	putEvent(e);


	peer->Drop();
	return true;
}

/* Interface */

ConnectionEvent Connection::waitEvent(u32 timeout_ms)
{
	try {
		return m_event_queue.pop_front(timeout_ms);
	} catch(ItemNotFoundException &ex) {
		ConnectionEvent e;
		e.type = CONNEVENT_NONE;
		return e;
	}
}

void Connection::putCommand(ConnectionCommand &c)
{
	if (!m_shutting_down) {
		m_command_queue.push_back(c);
		m_sendThread->Trigger();
	}
}

void Connection::Serve(Address bind_addr)
{
	ConnectionCommand c;
	c.serve(bind_addr);
	putCommand(c);
}

void Connection::Connect(Address address)
{
	ConnectionCommand c;
	c.connect(address);
	putCommand(c);
}

bool Connection::Connected()
{
	MutexAutoLock peerlock(m_peers_mutex);

	if (m_peers.size() != 1)
		return false;

	std::map<session_t, Peer *>::iterator node = m_peers.find(PEER_ID_SERVER);
	if (node == m_peers.end())
		return false;

	if (m_peer_id == PEER_ID_INEXISTENT)
		return false;

	return true;
}

void Connection::Disconnect()
{
	ConnectionCommand c;
	c.disconnect();
	putCommand(c);
}

bool Connection::Receive(NetworkPacket *pkt, u32 timeout)
{
	/*
		Note that this function can potentially wait infinitely if non-data
		events keep happening before the timeout expires.
		This is not considered to be a problem (is it?)
	*/
	for(;;) {
		ConnectionEvent e = waitEvent(timeout);
		if (e.type != CONNEVENT_NONE)
			LOG(dout_con << getDesc() << ": Receive: got event: "
					<< e.describe() << std::endl);
		switch(e.type) {
		case CONNEVENT_NONE:
			return false;
		case CONNEVENT_DATA_RECEIVED:
			// Data size is lesser than command size, ignoring packet
			if (e.data.getSize() < 2) {
				continue;
			}

			pkt->putRawPacket(*e.data, e.data.getSize(), e.peer_id);
			return true;
		case CONNEVENT_PEER_ADDED: {
			UDPPeer tmp(e.peer_id, e.address, this);
			if (m_bc_peerhandler)
				m_bc_peerhandler->peerAdded(&tmp);
			continue;
		}
		case CONNEVENT_PEER_REMOVED: {
			UDPPeer tmp(e.peer_id, e.address, this);
			if (m_bc_peerhandler)
				m_bc_peerhandler->deletingPeer(&tmp, e.timeout);
			continue;
		}
		case CONNEVENT_BIND_FAILED:
			throw ConnectionBindFailed("Failed to bind socket "
					"(port already in use?)");
		}
	}
	return false;
}

void Connection::Receive(NetworkPacket *pkt)
{
	bool any = Receive(pkt, m_bc_receive_timeout);
	if (!any)
		throw NoIncomingDataException("No incoming data");
}

bool Connection::TryReceive(NetworkPacket *pkt)
{
	return Receive(pkt, 0);
}

void Connection::Send(session_t peer_id, u8 channelnum,
		NetworkPacket *pkt, bool reliable)
{
	assert(channelnum < CHANNEL_COUNT); // Pre-condition

	ConnectionCommand c;

	c.send(peer_id, channelnum, pkt, reliable);
	putCommand(c);
}

Address Connection::GetPeerAddress(session_t peer_id)
{
	PeerHelper peer = getPeerNoEx(peer_id);

	if (!peer)
		throw PeerNotFoundException("No address for peer found!");
	Address peer_address;
	peer->getAddress(MTP_PRIMARY, peer_address);
	return peer_address;
}

float Connection::getPeerStat(session_t peer_id, rtt_stat_type type)
{
	PeerHelper peer = getPeerNoEx(peer_id);
	if (!peer) return -1;
	return peer->getStat(type);
}

float Connection::getLocalStat(rate_stat_type type)
{
	PeerHelper peer = getPeerNoEx(PEER_ID_SERVER);

	FATAL_ERROR_IF(!peer, "Connection::getLocalStat we couldn't get our own peer? are you serious???");

	float retval = 0.0;

	for (Channel &channel : dynamic_cast<UDPPeer *>(&peer)->channels) {
		switch(type) {
			case CUR_DL_RATE:
				retval += channel.getCurrentDownloadRateKB();
				break;
			case AVG_DL_RATE:
				retval += channel.getAvgDownloadRateKB();
				break;
			case CUR_INC_RATE:
				retval += channel.getCurrentIncomingRateKB();
				break;
			case AVG_INC_RATE:
				retval += channel.getAvgIncomingRateKB();
				break;
			case AVG_LOSS_RATE:
				retval += channel.getAvgLossRateKB();
				break;
			case CUR_LOSS_RATE:
				retval += channel.getCurrentLossRateKB();
				break;
		default:
			FATAL_ERROR("Connection::getLocalStat Invalid stat type");
		}
	}
	return retval;
}

u16 Connection::createPeer(Address& sender, MTProtocols protocol, int fd)
{
	// Somebody wants to make a new connection

	// Get a unique peer id (2 or higher)
	session_t peer_id_new = m_next_remote_peer_id;
	u16 overflow =  MAX_UDP_PEERS;

	/*
		Find an unused peer id
	*/
	MutexAutoLock lock(m_peers_mutex);
	bool out_of_ids = false;
	for(;;) {
		// Check if exists
		if (m_peers.find(peer_id_new) == m_peers.end())

			break;
		// Check for overflow
		if (peer_id_new == overflow) {
			out_of_ids = true;
			break;
		}
		peer_id_new++;
	}

	if (out_of_ids) {
		errorstream << getDesc() << " ran out of peer ids" << std::endl;
		return PEER_ID_INEXISTENT;
	}

	// Create a peer
	Peer *peer = 0;
	peer = new UDPPeer(peer_id_new, sender, this);

	m_peers[peer->id] = peer;
	m_peer_ids.push_back(peer->id);

	m_next_remote_peer_id = (peer_id_new +1 ) % MAX_UDP_PEERS;

	LOG(dout_con << getDesc()
			<< "createPeer(): giving peer_id=" << peer_id_new << std::endl);

	ConnectionCommand cmd;
	SharedBuffer<u8> reply(4);
	writeU8(&reply[0], PACKET_TYPE_CONTROL);
	writeU8(&reply[1], CONTROLTYPE_SET_PEER_ID);
	writeU16(&reply[2], peer_id_new);
	cmd.createPeer(peer_id_new,reply);
	putCommand(cmd);

	// Create peer addition event
	ConnectionEvent e;
	e.peerAdded(peer_id_new, sender);
	putEvent(e);

	// We're now talking to a valid peer_id
	return peer_id_new;
}

void Connection::PrintInfo(std::ostream &out)
{
	m_info_mutex.lock();
	out<<getDesc()<<": ";
	m_info_mutex.unlock();
}

const std::string Connection::getDesc()
{
	return std::string("con(")+
			itos(m_udpSocket.GetHandle())+"/"+itos(m_peer_id)+")";
}

void Connection::DisconnectPeer(session_t peer_id)
{
	ConnectionCommand discon;
	discon.disconnect_peer(peer_id);
	putCommand(discon);
}

void Connection::sendAck(session_t peer_id, u8 channelnum, u16 seqnum)
{
	assert(channelnum < CHANNEL_COUNT); // Pre-condition

	LOG(dout_con<<getDesc()
			<<" Queuing ACK command to peer_id: " << peer_id <<
			" channel: " << (channelnum & 0xFF) <<
			" seqnum: " << seqnum << std::endl);

	ConnectionCommand c;
	SharedBuffer<u8> ack(4);
	writeU8(&ack[0], PACKET_TYPE_CONTROL);
	writeU8(&ack[1], CONTROLTYPE_ACK);
	writeU16(&ack[2], seqnum);

	c.ack(peer_id, channelnum, ack);
	putCommand(c);
	m_sendThread->Trigger();
}

UDPPeer* Connection::createServerPeer(Address& address)
{
	if (getPeerNoEx(PEER_ID_SERVER) != 0)
	{
		throw ConnectionException("Already connected to a server");
	}

	UDPPeer *peer = new UDPPeer(PEER_ID_SERVER, address, this);

	{
		MutexAutoLock lock(m_peers_mutex);
		m_peers[peer->id] = peer;
		m_peer_ids.push_back(peer->id);
	}

	return peer;
}

} // namespace
