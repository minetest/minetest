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
#include <errno.h>
#include "connection.h"
#include "main.h"
#include "serialization.h"
#include "log.h"
#include "porting.h"
#include "util/serialize.h"
#include "util/numeric.h"
#include "util/string.h"
#include "settings.h"
#include "profiler.h"
#include "main.h" // for profiling

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
JMutex log_message_mutex;
#define LOG(a)                                                                 \
	{                                                                          \
	JMutexAutoLock loglock(log_message_mutex);                                 \
	a;                                                                         \
	}
#define PROFILE(a) a
//#define DEBUG_CONNECTION_KBPS
#undef DEBUG_CONNECTION_KBPS
#endif


static inline float CALC_DTIME(unsigned int lasttime, unsigned int curtime) {
	float value = ( curtime - lasttime) / 1000.0;
	return MYMAX(MYMIN(value,0.1),0.0);
}

/* maximum window size to use, 0xFFFF is theoretical maximum  don't think about
 * touching it, the less you're away from it the more likely data corruption
 * will occur
 */
#define MAX_RELIABLE_WINDOW_SIZE 0x8000
 /* starting value for window size */
#define MIN_RELIABLE_WINDOW_SIZE 0x40

#define MAX_UDP_PEERS 65535

#define PING_TIMEOUT 5.0

static u16 readPeerId(u8 *packetdata)
{
	return readU16(&packetdata[4]);
}
static u8 readChannel(u8 *packetdata)
{
	return readU8(&packetdata[6]);
}

BufferedPacket makePacket(Address &address, u8 *data, u32 datasize,
		u32 protocol_id, u16 sender_peer_id, u8 channel)
{
	u32 packet_size = datasize + BASE_HEADER_SIZE;
	BufferedPacket p(packet_size);
	p.address = address;

	writeU32(&p.data[0], protocol_id);
	writeU16(&p.data[4], sender_peer_id);
	writeU8(&p.data[6], channel);

	memcpy(&p.data[BASE_HEADER_SIZE], data, datasize);

	return p;
}

BufferedPacket makePacket(Address &address, SharedBuffer<u8> &data,
		u32 protocol_id, u16 sender_peer_id, u8 channel)
{
	return makePacket(address, *data, data.getSize(),
			protocol_id, sender_peer_id, channel);
}

SharedBuffer<u8> makeOriginalPacket(
		SharedBuffer<u8> data)
{
	u32 header_size = 1;
	u32 packet_size = data.getSize() + header_size;
	SharedBuffer<u8> b(packet_size);

	writeU8(&(b[0]), TYPE_ORIGINAL);
	if (data.getSize() > 0) {
		memcpy(&(b[header_size]), *data, data.getSize());
	}
	return b;
}

std::list<SharedBuffer<u8> > makeSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 seqnum)
{
	// Chunk packets, containing the TYPE_SPLIT header
	std::list<SharedBuffer<u8> > chunks;

	u32 chunk_header_size = 7;
	u32 maximum_data_size = chunksize_max - chunk_header_size;
	u32 start = 0;
	u32 end = 0;
	u32 chunk_num = 0;
	u16 chunk_count = 0;
	do{
		end = start + maximum_data_size - 1;
		if(end > data.getSize() - 1)
			end = data.getSize() - 1;

		u32 payload_size = end - start + 1;
		u32 packet_size = chunk_header_size + payload_size;

		SharedBuffer<u8> chunk(packet_size);

		writeU8(&chunk[0], TYPE_SPLIT);
		writeU16(&chunk[1], seqnum);
		// [3] u16 chunk_count is written at next stage
		writeU16(&chunk[5], chunk_num);
		memcpy(&chunk[chunk_header_size], &data[start], payload_size);

		chunks.push_back(chunk);
		chunk_count++;

		start = end + 1;
		chunk_num++;
	}
	while(end != data.getSize() - 1);

	for(std::list<SharedBuffer<u8> >::iterator i = chunks.begin();
		i != chunks.end(); ++i)
	{
		// Write chunk_count
		writeU16(&((*i)[3]), chunk_count);
	}

	return chunks;
}

std::list<SharedBuffer<u8> > makeAutoSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 &split_seqnum)
{
	u32 original_header_size = 1;
	std::list<SharedBuffer<u8> > list;
	if(data.getSize() + original_header_size > chunksize_max)
	{
		list = makeSplitPacket(data, chunksize_max, split_seqnum);
		split_seqnum++;
		return list;
	}
	else
	{
		list.push_back(makeOriginalPacket(data));
	}
	return list;
}

SharedBuffer<u8> makeReliablePacket(
		SharedBuffer<u8> data,
		u16 seqnum)
{
	u32 header_size = 3;
	u32 packet_size = data.getSize() + header_size;
	SharedBuffer<u8> b(packet_size);

	writeU8(&b[0], TYPE_RELIABLE);
	writeU16(&b[1], seqnum);

	memcpy(&b[header_size], *data, data.getSize());

	return b;
}

/*
	ReliablePacketBuffer
*/

ReliablePacketBuffer::ReliablePacketBuffer(): m_list_size(0) {}

void ReliablePacketBuffer::print()
{
	JMutexAutoLock listlock(m_list_mutex);
	LOG(dout_con<<"Dump of ReliablePacketBuffer:" << std::endl);
	unsigned int index = 0;
	for(std::list<BufferedPacket>::iterator i = m_list.begin();
		i != m_list.end();
		++i)
	{
		u16 s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		LOG(dout_con<<index<< ":" << s << std::endl);
		index++;
	}
}
bool ReliablePacketBuffer::empty()
{
	JMutexAutoLock listlock(m_list_mutex);
	return m_list.empty();
}

u32 ReliablePacketBuffer::size()
{
	return m_list_size;
}

bool ReliablePacketBuffer::containsPacket(u16 seqnum)
{
	return !(findPacket(seqnum) == m_list.end());
}

RPBSearchResult ReliablePacketBuffer::findPacket(u16 seqnum)
{
	std::list<BufferedPacket>::iterator i = m_list.begin();
	for(; i != m_list.end(); ++i)
	{
		u16 s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		/*dout_con<<"findPacket(): finding seqnum="<<seqnum
				<<", comparing to s="<<s<<std::endl;*/
		if(s == seqnum)
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
	JMutexAutoLock listlock(m_list_mutex);
	if(m_list.empty())
		return false;
	BufferedPacket p = *m_list.begin();
	result = readU16(&p.data[BASE_HEADER_SIZE+1]);
	return true;
}

BufferedPacket ReliablePacketBuffer::popFirst()
{
	JMutexAutoLock listlock(m_list_mutex);
	if(m_list.empty())
		throw NotFoundException("Buffer is empty");
	BufferedPacket p = *m_list.begin();
	m_list.erase(m_list.begin());
	--m_list_size;

	if (m_list_size == 0) {
		m_oldest_non_answered_ack = 0;
	} else {
		m_oldest_non_answered_ack =
				readU16(&(*m_list.begin()).data[BASE_HEADER_SIZE+1]);
	}
	return p;
}
BufferedPacket ReliablePacketBuffer::popSeqnum(u16 seqnum)
{
	JMutexAutoLock listlock(m_list_mutex);
	RPBSearchResult r = findPacket(seqnum);
	if(r == notFound()){
		LOG(dout_con<<"Sequence number: " << seqnum
				<< " not found in reliable buffer"<<std::endl);
		throw NotFoundException("seqnum not found in buffer");
	}
	BufferedPacket p = *r;


	RPBSearchResult next = r;
	next++;
	if (next != notFound()) {
		u16 s = readU16(&(next->data[BASE_HEADER_SIZE+1]));
		m_oldest_non_answered_ack = s;
	}

	m_list.erase(r);
	--m_list_size;

	if (m_list_size == 0)
	{ m_oldest_non_answered_ack = 0; }
	else
	{ m_oldest_non_answered_ack = readU16(&(*m_list.begin()).data[BASE_HEADER_SIZE+1]);	}
	return p;
}
void ReliablePacketBuffer::insert(BufferedPacket &p,u16 next_expected)
{
	JMutexAutoLock listlock(m_list_mutex);
	assert(p.data.getSize() >= BASE_HEADER_SIZE+3);
	u8 type = readU8(&p.data[BASE_HEADER_SIZE+0]);
	assert(type == TYPE_RELIABLE);
	u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);

	assert(seqnum_in_window(seqnum,next_expected,MAX_RELIABLE_WINDOW_SIZE));
	assert(seqnum != next_expected);

	++m_list_size;
	assert(m_list_size <= SEQNUM_MAX+1);

	// Find the right place for the packet and insert it there
	// If list is empty, just add it
	if(m_list.empty())
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
			i++;
			if (i != m_list.end())
				s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		}
	}
	/* non wrap around case (at least for incoming and next_expected */
	else
	{
		while(((s < seqnum) && (s >= next_expected)) && (i != m_list.end())) {
			i++;
			if (i != m_list.end())
				s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		}
	}

	if (s == seqnum) {
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
			fprintf(stderr, "New: seqnum: %05d size: %04d, address: %s\n",
					readU16(&(p.data[BASE_HEADER_SIZE+1])),p.data.getSize(),
					p.address.serializeString().c_str());
			throw IncomingDataCorruption("duplicated packet isn't same as original one");
		}

		assert(readU16(&(i->data[BASE_HEADER_SIZE+1])) == seqnum);
		assert(i->data.getSize() == p.data.getSize());
		assert(i->address == p.address);

		/* nothing to do this seems to be a resent packet */
		/* for paranoia reason data should be compared */
		--m_list_size;
	}
	/* insert or push back */
	else if (i != m_list.end()) {
		m_list.insert(i, p);
	}
	else {
		m_list.push_back(p);
	}

	/* update last packet number */
	m_oldest_non_answered_ack = readU16(&(*m_list.begin()).data[BASE_HEADER_SIZE+1]);
}

void ReliablePacketBuffer::incrementTimeouts(float dtime)
{
	JMutexAutoLock listlock(m_list_mutex);
	for(std::list<BufferedPacket>::iterator i = m_list.begin();
		i != m_list.end(); ++i)
	{
		i->time += dtime;
		i->totaltime += dtime;
	}
}

std::list<BufferedPacket> ReliablePacketBuffer::getTimedOuts(float timeout,
													unsigned int max_packets)
{
	JMutexAutoLock listlock(m_list_mutex);
	std::list<BufferedPacket> timed_outs;
	for(std::list<BufferedPacket>::iterator i = m_list.begin();
		i != m_list.end(); ++i)
	{
		if(i->time >= timeout) {
			timed_outs.push_back(*i);

			//this packet will be sent right afterwards reset timeout here
			i->time = 0.0;
			if (timed_outs.size() >= max_packets)
				break;
		}
	}
	return timed_outs;
}

/*
	IncomingSplitBuffer
*/

IncomingSplitBuffer::~IncomingSplitBuffer()
{
	JMutexAutoLock listlock(m_map_mutex);
	for(std::map<u16, IncomingSplitPacket*>::iterator i = m_buf.begin();
		i != m_buf.end(); ++i)
	{
		delete i->second;
	}
}
/*
	This will throw a GotSplitPacketException when a full
	split packet is constructed.
*/
SharedBuffer<u8> IncomingSplitBuffer::insert(BufferedPacket &p, bool reliable)
{
	JMutexAutoLock listlock(m_map_mutex);
	u32 headersize = BASE_HEADER_SIZE + 7;
	assert(p.data.getSize() >= headersize);
	u8 type = readU8(&p.data[BASE_HEADER_SIZE+0]);
	assert(type == TYPE_SPLIT);
	u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);
	u16 chunk_count = readU16(&p.data[BASE_HEADER_SIZE+3]);
	u16 chunk_num = readU16(&p.data[BASE_HEADER_SIZE+5]);

	// Add if doesn't exist
	if(m_buf.find(seqnum) == m_buf.end())
	{
		IncomingSplitPacket *sp = new IncomingSplitPacket();
		sp->chunk_count = chunk_count;
		sp->reliable = reliable;
		m_buf[seqnum] = sp;
	}

	IncomingSplitPacket *sp = m_buf[seqnum];

	// TODO: These errors should be thrown or something? Dunno.
	if(chunk_count != sp->chunk_count)
		LOG(derr_con<<"Connection: WARNING: chunk_count="<<chunk_count
				<<" != sp->chunk_count="<<sp->chunk_count
				<<std::endl);
	if(reliable != sp->reliable)
		LOG(derr_con<<"Connection: WARNING: reliable="<<reliable
				<<" != sp->reliable="<<sp->reliable
				<<std::endl);

	// If chunk already exists, ignore it.
	// Sometimes two identical packets may arrive when there is network
	// lag and the server re-sends stuff.
	if(sp->chunks.find(chunk_num) != sp->chunks.end())
		return SharedBuffer<u8>();

	// Cut chunk data out of packet
	u32 chunkdatasize = p.data.getSize() - headersize;
	SharedBuffer<u8> chunkdata(chunkdatasize);
	memcpy(*chunkdata, &(p.data[headersize]), chunkdatasize);

	// Set chunk data in buffer
	sp->chunks[chunk_num] = chunkdata;

	// If not all chunks are received, return empty buffer
	if(sp->allReceived() == false)
		return SharedBuffer<u8>();

	// Calculate total size
	u32 totalsize = 0;
	for(std::map<u16, SharedBuffer<u8> >::iterator i = sp->chunks.begin();
		i != sp->chunks.end(); ++i)
	{
		totalsize += i->second.getSize();
	}

	SharedBuffer<u8> fulldata(totalsize);

	// Copy chunks to data buffer
	u32 start = 0;
	for(u32 chunk_i=0; chunk_i<sp->chunk_count;
			chunk_i++)
	{
		SharedBuffer<u8> buf = sp->chunks[chunk_i];
		u16 chunkdatasize = buf.getSize();
		memcpy(&fulldata[start], *buf, chunkdatasize);
		start += chunkdatasize;;
	}

	// Remove sp from buffer
	m_buf.erase(seqnum);
	delete sp;

	return fulldata;
}
void IncomingSplitBuffer::removeUnreliableTimedOuts(float dtime, float timeout)
{
	std::list<u16> remove_queue;
	{
		JMutexAutoLock listlock(m_map_mutex);
		for(std::map<u16, IncomingSplitPacket*>::iterator i = m_buf.begin();
			i != m_buf.end(); ++i)
		{
			IncomingSplitPacket *p = i->second;
			// Reliable ones are not removed by timeout
			if(p->reliable == true)
				continue;
			p->time += dtime;
			if(p->time >= timeout)
				remove_queue.push_back(i->first);
		}
	}
	for(std::list<u16>::iterator j = remove_queue.begin();
		j != remove_queue.end(); ++j)
	{
		JMutexAutoLock listlock(m_map_mutex);
		LOG(dout_con<<"NOTE: Removing timed out unreliable split packet"<<std::endl);
		delete m_buf[*j];
		m_buf.erase(*j);
	}
}

/*
	Channel
*/

Channel::Channel() :
		window_size(MIN_RELIABLE_WINDOW_SIZE),
		next_incoming_seqnum(SEQNUM_INITIAL),
		next_outgoing_seqnum(SEQNUM_INITIAL),
		next_outgoing_split_seqnum(SEQNUM_INITIAL),
		current_packet_loss(0),
		current_packet_too_late(0),
		current_packet_successfull(0),
		packet_loss_counter(0),
		current_bytes_transfered(0),
		current_bytes_received(0),
		current_bytes_lost(0),
		max_kbps(0.0),
		cur_kbps(0.0),
		avg_kbps(0.0),
		max_incoming_kbps(0.0),
		cur_incoming_kbps(0.0),
		avg_incoming_kbps(0.0),
		max_kbps_lost(0.0),
		cur_kbps_lost(0.0),
		avg_kbps_lost(0.0),
		bpm_counter(0.0),
		rate_samples(0)
{
}

Channel::~Channel()
{
}

u16 Channel::readNextIncomingSeqNum()
{
	JMutexAutoLock internal(m_internal_mutex);
	return next_incoming_seqnum;
}

u16 Channel::incNextIncomingSeqNum()
{
	JMutexAutoLock internal(m_internal_mutex);
	u16 retval = next_incoming_seqnum;
	next_incoming_seqnum++;
	return retval;
}

u16 Channel::readNextSplitSeqNum()
{
	JMutexAutoLock internal(m_internal_mutex);
	return next_outgoing_split_seqnum;
}
void Channel::setNextSplitSeqNum(u16 seqnum)
{
	JMutexAutoLock internal(m_internal_mutex);
	next_outgoing_split_seqnum = seqnum;
}

u16 Channel::getOutgoingSequenceNumber(bool& successfull)
{
	JMutexAutoLock internal(m_internal_mutex);
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
				successfull = false;
				return 0;
			}
		}
		else {
			// ugly cast but this one is required in order to tell compiler we
			// know about difference of two unsigned may be negative in general
			// but we already made sure it won't happen in this case
			if ((next_outgoing_seqnum + (u16)(SEQNUM_MAX - lowest_unacked_seqnumber)) >
				window_size) {
				successfull = false;
				return 0;
			}
		}
	}

	next_outgoing_seqnum++;
	return retval;
}

u16 Channel::readOutgoingSequenceNumber()
{
	JMutexAutoLock internal(m_internal_mutex);
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
	JMutexAutoLock internal(m_internal_mutex);
	current_bytes_transfered += bytes;
	current_packet_successfull += packets;
}

void Channel::UpdateBytesReceived(unsigned int bytes) {
	JMutexAutoLock internal(m_internal_mutex);
	current_bytes_received += bytes;
}

void Channel::UpdateBytesLost(unsigned int bytes)
{
	JMutexAutoLock internal(m_internal_mutex);
	current_bytes_lost += bytes;
}


void Channel::UpdatePacketLossCounter(unsigned int count)
{
	JMutexAutoLock internal(m_internal_mutex);
	current_packet_loss += count;
}

void Channel::UpdatePacketTooLateCounter()
{
	JMutexAutoLock internal(m_internal_mutex);
	current_packet_too_late++;
}

void Channel::UpdateTimers(float dtime,bool legacy_peer)
{
	bpm_counter += dtime;
	packet_loss_counter += dtime;

	if (packet_loss_counter > 1.0)
	{
		packet_loss_counter -= 1.0;

		unsigned int packet_loss = 11; /* use a neutral value for initialization */
		unsigned int packets_successfull = 0;
		unsigned int packet_too_late = 0;

		bool reasonable_amount_of_data_transmitted = false;

		{
			JMutexAutoLock internal(m_internal_mutex);
			packet_loss = current_packet_loss;
			packet_too_late = current_packet_too_late;
			packets_successfull = current_packet_successfull;

			if (current_bytes_transfered > (unsigned int) (window_size*512/2))
			{
				reasonable_amount_of_data_transmitted = true;
			}
			current_packet_loss = 0;
			current_packet_too_late = 0;
			current_packet_successfull = 0;
		}

		/* dynamic window size is only available for non legacy peers */
		if (!legacy_peer) {
			float successfull_to_lost_ratio = 0.0;
			bool done = false;

			if (packets_successfull > 0) {
				successfull_to_lost_ratio = packet_loss/packets_successfull;
			}
			else if (packet_loss > 0)
			{
				window_size = MYMAX(
						(window_size - 10),
						MIN_RELIABLE_WINDOW_SIZE);
				done = true;
			}

			if (!done)
			{
				if ((successfull_to_lost_ratio < 0.01) &&
					(window_size < MAX_RELIABLE_WINDOW_SIZE))
				{
					/* don't even think about increasing if we didn't even
					 * use major parts of our window */
					if (reasonable_amount_of_data_transmitted)
						window_size = MYMIN(
								(window_size + 100),
								MAX_RELIABLE_WINDOW_SIZE);
				}
				else if ((successfull_to_lost_ratio < 0.05) &&
						(window_size < MAX_RELIABLE_WINDOW_SIZE))
				{
					/* don't even think about increasing if we didn't even
					 * use major parts of our window */
					if (reasonable_amount_of_data_transmitted)
						window_size = MYMIN(
								(window_size + 50),
								MAX_RELIABLE_WINDOW_SIZE);
				}
				else if (successfull_to_lost_ratio > 0.15)
				{
					window_size = MYMAX(
							(window_size - 100),
							MIN_RELIABLE_WINDOW_SIZE);
				}
				else if (successfull_to_lost_ratio > 0.1)
				{
					window_size = MYMAX(
							(window_size - 50),
							MIN_RELIABLE_WINDOW_SIZE);
				}
			}
		}
	}

	if (bpm_counter > 10.0)
	{
		{
			JMutexAutoLock internal(m_internal_mutex);
			cur_kbps                 =
					(((float) current_bytes_transfered)/bpm_counter)/1024.0;
			current_bytes_transfered = 0;
			cur_kbps_lost            =
					(((float) current_bytes_lost)/bpm_counter)/1024.0;
			current_bytes_lost       = 0;
			cur_incoming_kbps        =
					(((float) current_bytes_received)/bpm_counter)/1024.0;
			current_bytes_received   = 0;
			bpm_counter              = 0;
		}

		if (cur_kbps > max_kbps)
		{
			max_kbps = cur_kbps;
		}

		if (cur_kbps_lost > max_kbps_lost)
		{
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

PeerHelper::PeerHelper() :
	m_peer(0)
{}

PeerHelper::PeerHelper(Peer* peer) :
	m_peer(peer)
{
	if (peer != NULL)
	{
		if (!peer->IncUseCount())
		{
			m_peer = 0;
		}
	}
}

PeerHelper::~PeerHelper()
{
	if (m_peer != 0)
		m_peer->DecUseCount();

	m_peer = 0;
}

PeerHelper& PeerHelper::operator=(Peer* peer)
{
	m_peer = peer;
	if (peer != NULL)
	{
		if (!peer->IncUseCount())
		{
			m_peer = 0;
		}
	}
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

bool PeerHelper::operator!() {
	return ! m_peer;
}

bool PeerHelper::operator!=(void* ptr)
{
	return ((void*) m_peer != ptr);
}

bool Peer::IncUseCount()
{
	JMutexAutoLock lock(m_exclusive_access_mutex);

	if (!m_pending_deletion)
	{
		this->m_usage++;
		return true;
	}

	return false;
}

void Peer::DecUseCount()
{
	{
		JMutexAutoLock lock(m_exclusive_access_mutex);
		assert(m_usage > 0);
		m_usage--;

		if (!((m_pending_deletion) && (m_usage == 0)))
			return;
	}
	delete this;
}

void Peer::RTTStatistics(float rtt, std::string profiler_id,
		unsigned int num_samples) {

	if (m_last_rtt > 0) {
		/* set min max values */
		if (rtt < m_rtt.min_rtt)
			m_rtt.min_rtt = rtt;
		if (rtt >= m_rtt.max_rtt)
			m_rtt.max_rtt = rtt;

		/* do average calculation */
		if(m_rtt.avg_rtt < 0.0)
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

		if(m_rtt.jitter_avg < 0.0)
			m_rtt.jitter_avg  = jitter;
		else
			m_rtt.jitter_avg  = m_rtt.jitter_avg * (num_samples/(num_samples-1)) +
								jitter * (1/num_samples);

		if (profiler_id != "")
		{
			g_profiler->graphAdd(profiler_id + "_rtt", rtt);
			g_profiler->graphAdd(profiler_id + "_jitter", jitter);
		}
	}
	/* save values required for next loop */
	m_last_rtt = rtt;
}

bool Peer::isTimedOut(float timeout)
{
	JMutexAutoLock lock(m_exclusive_access_mutex);
	u32 current_time = porting::getTimeMs();

	float dtime = CALC_DTIME(m_last_timeout_check,current_time);
	m_last_timeout_check = current_time;

	m_timeout_counter += dtime;

	return m_timeout_counter > timeout;
}

void Peer::Drop()
{
	{
		JMutexAutoLock usage_lock(m_exclusive_access_mutex);
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
	Peer(a_address,a_id,connection),
	m_pending_disconnect(false),
	resend_timeout(0.5),
	m_legacy_peer(true)
{
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

void UDPPeer::setNonLegacyPeer()
{
	m_legacy_peer = false;
	for(unsigned int i=0; i< CHANNEL_COUNT; i++)
	{
		channels->setWindowSize(g_settings->getU16("max_packets_per_iteration"));
	}
}

void UDPPeer::reportRTT(float rtt)
{
	if (rtt < 0.0) {
		return;
	}
	RTTStatistics(rtt,"rudp",MAX_RELIABLE_WINDOW_SIZE*10);

	float timeout = getStat(AVG_RTT) * RESEND_TIMEOUT_FACTOR;
	if(timeout < RESEND_TIMEOUT_MIN)
		timeout = RESEND_TIMEOUT_MIN;
	if(timeout > RESEND_TIMEOUT_MAX)
		timeout = RESEND_TIMEOUT_MAX;

	JMutexAutoLock usage_lock(m_exclusive_access_mutex);
	resend_timeout = timeout;
}

bool UDPPeer::Ping(float dtime,SharedBuffer<u8>& data)
{
	m_ping_timer += dtime;
	if(m_ping_timer >= PING_TIMEOUT)
	{
		// Create and send PING packet
		writeU8(&data[0], TYPE_CONTROL);
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

	if ( channels[c.channelnum].queued_commands.empty() &&
			/* don't queue more packets then window size */
			(channels[c.channelnum].queued_reliables.size()
			< (channels[c.channelnum].getWindowSize()/2)))
	{
		LOG(dout_con<<m_connection->getDesc()
				<<" processing reliable command for peer id: " << c.peer_id
				<<" data size: " << c.data.getSize() << std::endl);
		if (!processReliableSendCommand(c,max_packet_size))
		{
			channels[c.channelnum].queued_commands.push_back(c);
		}
	}
	else
	{
		LOG(dout_con<<m_connection->getDesc()
				<<" Queueing reliable command for peer id: " << c.peer_id
				<<" data size: " << c.data.getSize() <<std::endl);
		channels[c.channelnum].queued_commands.push_back(c);
	}
}

bool UDPPeer::processReliableSendCommand(
				ConnectionCommand &c,
				unsigned int max_packet_size)
{
	if (m_pending_disconnect)
		return true;

	u32 chunksize_max = max_packet_size
							- BASE_HEADER_SIZE
							- RELIABLE_HEADER_SIZE;

	assert(c.data.getSize() < MAX_RELIABLE_WINDOW_SIZE*512);

	std::list<SharedBuffer<u8> > originals;
	u16 split_sequence_number = channels[c.channelnum].readNextSplitSeqNum();

	if (c.raw)
	{
		originals.push_back(c.data);
	}
	else {
		originals = makeAutoSplitPacket(c.data, chunksize_max,split_sequence_number);
		channels[c.channelnum].setNextSplitSeqNum(split_sequence_number);
	}

	bool have_sequence_number = true;
	bool have_initial_sequence_number = false;
	Queue<BufferedPacket> toadd;
	volatile u16 initial_sequence_number = 0;

	for(std::list<SharedBuffer<u8> >::iterator i = originals.begin();
		i != originals.end(); ++i)
	{
		u16 seqnum = channels[c.channelnum].getOutgoingSequenceNumber(have_sequence_number);

		/* oops, we don't have enough sequence numbers to send this packet */
		if (!have_sequence_number)
			break;

		if (!have_initial_sequence_number)
		{
			initial_sequence_number = seqnum;
			have_initial_sequence_number = true;
		}

		SharedBuffer<u8> reliable = makeReliablePacket(*i, seqnum);

		// Add base headers and make a packet
		BufferedPacket p = con::makePacket(address, reliable,
				m_connection->GetProtocolID(), m_connection->GetPeerID(),
				c.channelnum);

		toadd.push_back(p);
	}

	if (have_sequence_number) {
		volatile u16 pcount = 0;
		while(toadd.size() > 0) {
			BufferedPacket p = toadd.pop_front();
//			LOG(dout_con<<connection->getDesc()
//					<< " queuing reliable packet for peer_id: " << c.peer_id
//					<< " channel: " << (c.channelnum&0xFF)
//					<< " seqnum: " << readU16(&p.data[BASE_HEADER_SIZE+1])
//					<< std::endl)
			channels[c.channelnum].queued_reliables.push_back(p);
			pcount++;
		}
		assert(channels[c.channelnum].queued_reliables.size() < 0xFFFF);
		return true;
	}
	else {
		volatile u16 packets_available = toadd.size();
		/* we didn't get a single sequence number no need to fill queue */
		if (!have_initial_sequence_number)
		{
			return false;
		}
		while(toadd.size() > 0) {
			/* remove packet */
			toadd.pop_front();

			bool successfully_put_back_sequence_number
				= channels[c.channelnum].putBackSequenceNumber(
					(initial_sequence_number+toadd.size() % (SEQNUM_MAX+1)));

			assert(successfully_put_back_sequence_number);
		}
		LOG(dout_con<<m_connection->getDesc()
				<< " Windowsize exceeded on reliable sending "
				<< c.data.getSize() << " bytes"
				<< std::endl << "\t\tinitial_sequence_number: "
				<< initial_sequence_number
				<< std::endl << "\t\tgot at most            : "
				<< packets_available << " packets"
				<< std::endl << "\t\tpackets queued         : "
				<< channels[c.channelnum].outgoing_reliables_sent.size()
				<< std::endl);
		return false;
	}
}

void UDPPeer::RunCommandQueues(
							unsigned int max_packet_size,
							unsigned int maxcommands,
							unsigned int maxtransfer)
{

	for (unsigned int i = 0; i < CHANNEL_COUNT; i++)
	{
		unsigned int commands_processed = 0;

		if ((channels[i].queued_commands.size() > 0) &&
				(channels[i].queued_reliables.size() < maxtransfer) &&
				(commands_processed < maxcommands))
		{
			try {
				ConnectionCommand c = channels[i].queued_commands.pop_front();
				LOG(dout_con<<m_connection->getDesc()
						<<" processing queued reliable command "<<std::endl);
				if (!processReliableSendCommand(c,max_packet_size)) {
					LOG(dout_con<<m_connection->getDesc()
							<< " Failed to queue packets for peer_id: " << c.peer_id
							<< ", delaying sending of " << c.data.getSize()
							<< " bytes" << std::endl);
					channels[i].queued_commands.push_front(c);
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
	assert(channel < CHANNEL_COUNT);
	return channels[channel].readNextIncomingSeqNum();
}

void UDPPeer::setNextSplitSequenceNumber(u8 channel, u16 seqnum)
{
	assert(channel < CHANNEL_COUNT);
	channels[channel].setNextSplitSeqNum(seqnum);
}

SharedBuffer<u8> UDPPeer::addSpiltPacket(u8 channel,
											BufferedPacket toadd,
											bool reliable)
{
	assert(channel < CHANNEL_COUNT);
	return channels[channel].incoming_splits.insert(toadd,reliable);
}

/******************************************************************************/
/* Connection Threads                                                         */
/******************************************************************************/

ConnectionSendThread::ConnectionSendThread(Connection* parent,
											unsigned int max_packet_size,
											float timeout) :
	m_connection(parent),
	m_max_packet_size(max_packet_size),
	m_timeout(timeout),
	m_max_commands_per_iteration(1),
	m_max_data_packets_per_iteration(g_settings->getU16("max_packets_per_iteration")),
	m_max_packets_requeued(256)
{
}

void * ConnectionSendThread::Thread()
{
	ThreadStarted();
	log_register_thread("ConnectionSend");

	LOG(dout_con<<m_connection->getDesc()
			<<"ConnectionSend thread started"<<std::endl);

	u32 curtime = porting::getTimeMs();
	u32 lasttime = curtime;

	PROFILE(std::stringstream ThreadIdentifier);
	PROFILE(ThreadIdentifier << "ConnectionSend: [" << m_connection->getDesc() << "]");

	porting::setThreadName("ConnectionSend");

	/* if stop is requested don't stop immediately but try to send all        */
	/* packets first */
	while(!StopRequested() || packetsQueued()) {
		BEGIN_DEBUG_EXCEPTION_HANDLER
		PROFILE(ScopeProfiler sp(g_profiler, ThreadIdentifier.str(), SPT_AVG));

		m_iteration_packets_avaialble = m_max_data_packets_per_iteration;

		/* wait for trigger or timeout */
		m_send_sleep_semaphore.Wait(50);

		/* remove all triggers */
		while(m_send_sleep_semaphore.Wait(0)) {}

		lasttime = curtime;
		curtime = porting::getTimeMs();
		float dtime = CALC_DTIME(lasttime,curtime);

		/* first do all the reliable stuff */
		runTimeouts(dtime);

		/* translate commands to packets */
		ConnectionCommand c = m_connection->m_command_queue.pop_frontNoEx(0);
		while(c.type != CONNCMD_NONE)
				{
			if (c.reliable)
				processReliableCommand(c);
			else
				processNonReliableCommand(c);

			c = m_connection->m_command_queue.pop_frontNoEx(0);
		}

		/* send non reliable packets */
		sendPackets(dtime);

		END_DEBUG_EXCEPTION_HANDLER(errorstream);
	}

	PROFILE(g_profiler->remove(ThreadIdentifier.str()));
	return NULL;
}

void ConnectionSendThread::Trigger()
{
	m_send_sleep_semaphore.Post();
}

bool ConnectionSendThread::packetsQueued()
{
	std::list<u16> peerIds = m_connection->getPeerIDs();

	if ((this->m_outgoing_queue.size() > 0) && (peerIds.size() > 0))
		return true;

	for(std::list<u16>::iterator j = peerIds.begin();
			j != peerIds.end(); ++j)
	{
		PeerHelper peer = m_connection->getPeerNoEx(*j);

		if (!peer)
			continue;

		if (dynamic_cast<UDPPeer*>(&peer) == 0)
			continue;

		for(u16 i=0; i<CHANNEL_COUNT; i++)
		{
			Channel *channel = &(dynamic_cast<UDPPeer*>(&peer))->channels[i];

			if (channel->queued_commands.size() > 0)
			{
				return true;
			}
		}
	}


	return false;
}

void ConnectionSendThread::runTimeouts(float dtime)
{
	std::list<u16> timeouted_peers;
	std::list<u16> peerIds = m_connection->getPeerIDs();

	for(std::list<u16>::iterator j = peerIds.begin();
		j != peerIds.end(); ++j)
	{
		PeerHelper peer = m_connection->getPeerNoEx(*j);

		if (!peer)
			continue;

		if(dynamic_cast<UDPPeer*>(&peer) == 0)
			continue;

		PROFILE(std::stringstream peerIdentifier);
		PROFILE(peerIdentifier << "runTimeouts[" << m_connection->getDesc()
				<< ";" << *j << ";RELIABLE]");
		PROFILE(ScopeProfiler peerprofiler(g_profiler, peerIdentifier.str(), SPT_AVG));

		SharedBuffer<u8> data(2); // data for sending ping, required here because of goto

		/*
			Check peer timeout
		*/
		if(peer->isTimedOut(m_timeout))
		{
			infostream<<m_connection->getDesc()
					<<"RunTimeouts(): Peer "<<peer->id
					<<" has timed out."
					<<" (source=peer->timeout_counter)"
					<<std::endl;
			// Add peer to the list
			timeouted_peers.push_back(peer->id);
			// Don't bother going through the buffers of this one
			continue;
		}

		float resend_timeout = dynamic_cast<UDPPeer*>(&peer)->getResendTimeout();
		for(u16 i=0; i<CHANNEL_COUNT; i++)
		{
			std::list<BufferedPacket> timed_outs;
			Channel *channel = &(dynamic_cast<UDPPeer*>(&peer))->channels[i];

			if (dynamic_cast<UDPPeer*>(&peer)->getLegacyPeer())
				channel->setWindowSize(g_settings->getU16("workaround_window_size"));

			// Remove timed out incomplete unreliable split packets
			channel->incoming_splits.removeUnreliableTimedOuts(dtime, m_timeout);

			// Increment reliable packet times
			channel->outgoing_reliables_sent.incrementTimeouts(dtime);

			unsigned int numpeers = m_connection->m_peers.size();

			if (numpeers == 0)
				return;

			// Re-send timed out outgoing reliables
			timed_outs = channel->
					outgoing_reliables_sent.getTimedOuts(resend_timeout,
							(m_max_data_packets_per_iteration/numpeers));

			channel->UpdatePacketLossCounter(timed_outs.size());
			g_profiler->graphAdd("packets_lost", timed_outs.size());

			m_iteration_packets_avaialble -= timed_outs.size();

			for(std::list<BufferedPacket>::iterator k = timed_outs.begin();
				k != timed_outs.end(); ++k)
			{
				u16 peer_id = readPeerId(*(k->data));
				u8 channelnum  = readChannel(*(k->data));
				u16 seqnum  = readU16(&(k->data[BASE_HEADER_SIZE+1]));

				channel->UpdateBytesLost(k->data.getSize());
				k->resend_count++;

				LOG(derr_con<<m_connection->getDesc()
						<<"RE-SENDING timed-out RELIABLE to "
						<< k->address.serializeString()
						<< "(t/o="<<resend_timeout<<"): "
						<<"from_peer_id="<<peer_id
						<<", channel="<<((int)channelnum&0xff)
						<<", seqnum="<<seqnum
						<<std::endl);

				rawSend(*k);

				// do not handle rtt here as we can't decide if this packet was
				// lost or really takes more time to transmit
			}
			channel->UpdateTimers(dtime,dynamic_cast<UDPPeer*>(&peer)->getLegacyPeer());
		}

		/* send ping if necessary */
		if (dynamic_cast<UDPPeer*>(&peer)->Ping(dtime,data)) {
			LOG(dout_con<<m_connection->getDesc()
					<<"Sending ping for peer_id: "
					<< dynamic_cast<UDPPeer*>(&peer)->id <<std::endl);
			/* this may fail if there ain't a sequence number left */
			if (!rawSendAsPacket(dynamic_cast<UDPPeer*>(&peer)->id, 0, data, true))
			{
				//retrigger with reduced ping interval
				dynamic_cast<UDPPeer*>(&peer)->Ping(4.0,data);
			}
		}

		dynamic_cast<UDPPeer*>(&peer)->RunCommandQueues(m_max_packet_size,
								m_max_commands_per_iteration,
								m_max_packets_requeued);
	}

	// Remove timed out peers
	for(std::list<u16>::iterator i = timeouted_peers.begin();
		i != timeouted_peers.end(); ++i)
	{
		LOG(derr_con<<m_connection->getDesc()
				<<"RunTimeouts(): Removing peer "<<(*i)<<std::endl);
		m_connection->deletePeer(*i, true);
	}
}

void ConnectionSendThread::rawSend(const BufferedPacket &packet)
{
	try{
		m_connection->m_udpSocket.Send(packet.address, *packet.data,
				packet.data.getSize());
		LOG(dout_con <<m_connection->getDesc()
				<< " rawSend: " << packet.data.getSize()
				<< " bytes sent" << std::endl);
	} catch(SendFailedException &e){
		LOG(derr_con<<m_connection->getDesc()
				<<"Connection::rawSend(): SendFailedException: "
				<<packet.address.serializeString()<<std::endl);
	}
}

void ConnectionSendThread::sendAsPacketReliable(BufferedPacket& p, Channel* channel)
{
	try{
		p.absolute_send_time = porting::getTimeMs();
		// Buffer the packet
		channel->outgoing_reliables_sent.insert(p,
			(channel->readOutgoingSequenceNumber() - MAX_RELIABLE_WINDOW_SIZE)
			% (MAX_RELIABLE_WINDOW_SIZE+1));
	}
	catch(AlreadyExistsException &e)
	{
		LOG(derr_con<<m_connection->getDesc()
				<<"WARNING: Going to send a reliable packet"
				<<" in outgoing buffer" <<std::endl);
		//assert(0);
	}

	// Send the packet
	rawSend(p);
}

bool ConnectionSendThread::rawSendAsPacket(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data, bool reliable)
{
	PeerHelper peer = m_connection->getPeerNoEx(peer_id);
	if(!peer) {
		LOG(dout_con<<m_connection->getDesc()
				<<" INFO: dropped packet for non existent peer_id: "
				<< peer_id << std::endl);
		assert(reliable && "trying to send raw packet reliable but no peer found!");
		return false;
	}
	Channel *channel = &(dynamic_cast<UDPPeer*>(&peer)->channels[channelnum]);

	if(reliable)
	{
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
			LOG(dout_con<<m_connection->getDesc()
					<<" INFO: sending a reliable packet to peer_id " << peer_id
					<<" channel: " << channelnum
					<<" seqnum: " << seqnum << std::endl);
			sendAsPacketReliable(p,channel);
			return true;
		}
		else {
			LOG(dout_con<<m_connection->getDesc()
					<<" INFO: queueing reliable packet for peer_id: " << peer_id
					<<" channel: " << channelnum
					<<" seqnum: " << seqnum << std::endl);
			channel->queued_reliables.push_back(p);
			return false;
		}
	}
	else
	{
		Address peer_address;

		if (peer->getAddress(MTP_UDP, peer_address))
		{
			// Add base headers and make a packet
			BufferedPacket p = con::makePacket(peer_address, data,
					m_connection->GetProtocolID(), m_connection->GetPeerID(),
					channelnum);

			// Send the packet
			rawSend(p);
			return true;
		}
		else {
			LOG(dout_con<<m_connection->getDesc()
					<<" INFO: dropped unreliable packet for peer_id: " << peer_id
					<<" because of (yet) missing udp address" << std::endl);
			return false;
		}
	}

	//never reached
	return false;
}

void ConnectionSendThread::processReliableCommand(ConnectionCommand &c)
{
	assert(c.reliable);

	switch(c.type){
	case CONNCMD_NONE:
		LOG(dout_con<<m_connection->getDesc()
				<<"UDP processing reliable CONNCMD_NONE"<<std::endl);
		return;

	case CONNCMD_SEND:
		LOG(dout_con<<m_connection->getDesc()
				<<"UDP processing reliable CONNCMD_SEND"<<std::endl);
		sendReliable(c);
		return;

	case CONNCMD_SEND_TO_ALL:
		LOG(dout_con<<m_connection->getDesc()
				<<"UDP processing CONNCMD_SEND_TO_ALL"<<std::endl);
		sendToAllReliable(c);
		return;

	case CONCMD_CREATE_PEER:
		LOG(dout_con<<m_connection->getDesc()
				<<"UDP processing reliable CONCMD_CREATE_PEER"<<std::endl);
		if (!rawSendAsPacket(c.peer_id,c.channelnum,c.data,c.reliable))
		{
			/* put to queue if we couldn't send it immediately */
			sendReliable(c);
		}
		return;

	case CONCMD_DISABLE_LEGACY:
		LOG(dout_con<<m_connection->getDesc()
				<<"UDP processing reliable CONCMD_DISABLE_LEGACY"<<std::endl);
		if (!rawSendAsPacket(c.peer_id,c.channelnum,c.data,c.reliable))
		{
			/* put to queue if we couldn't send it immediately */
			sendReliable(c);
		}
		return;

	case CONNCMD_SERVE:
	case CONNCMD_CONNECT:
	case CONNCMD_DISCONNECT:
	case CONCMD_ACK:
		assert("Got command that shouldn't be reliable as reliable command" == 0);
	default:
		LOG(dout_con<<m_connection->getDesc()
				<<" Invalid reliable command type: " << c.type <<std::endl);
	}
}


void ConnectionSendThread::processNonReliableCommand(ConnectionCommand &c)
{
	assert(!c.reliable);

	switch(c.type){
	case CONNCMD_NONE:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_NONE"<<std::endl);
		return;
	case CONNCMD_SERVE:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_SERVE port="
				<<c.address.serializeString()<<std::endl);
		serve(c.address);
		return;
	case CONNCMD_CONNECT:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_CONNECT"<<std::endl);
		connect(c.address);
		return;
	case CONNCMD_DISCONNECT:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_DISCONNECT"<<std::endl);
		disconnect();
		return;
	case CONNCMD_DISCONNECT_PEER:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_DISCONNECT_PEER"<<std::endl);
		disconnect_peer(c.peer_id);
		return;
	case CONNCMD_SEND:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_SEND"<<std::endl);
		send(c.peer_id, c.channelnum, c.data);
		return;
	case CONNCMD_SEND_TO_ALL:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONNCMD_SEND_TO_ALL"<<std::endl);
		sendToAll(c.channelnum, c.data);
		return;
	case CONCMD_ACK:
		LOG(dout_con<<m_connection->getDesc()
				<<" UDP processing CONCMD_ACK"<<std::endl);
		sendAsPacket(c.peer_id,c.channelnum,c.data,true);
		return;
	case CONCMD_CREATE_PEER:
		assert("Got command that should be reliable as unreliable command" == 0);
	default:
		LOG(dout_con<<m_connection->getDesc()
				<<" Invalid command type: " << c.type <<std::endl);
	}
}

void ConnectionSendThread::serve(Address bind_address)
{
	LOG(dout_con<<m_connection->getDesc()
			<<"UDP serving at port " << bind_address.serializeString() <<std::endl);
	try{
		m_connection->m_udpSocket.Bind(bind_address);
		m_connection->SetPeerID(PEER_ID_SERVER);
	}
	catch(SocketException &e){
		// Create event
		ConnectionEvent ce;
		ce.bindFailed();
		m_connection->putEvent(ce);
	}
}

void ConnectionSendThread::connect(Address address)
{
	LOG(dout_con<<m_connection->getDesc()<<" connecting to "<<address.serializeString()
			<<":"<<address.getPort()<<std::endl);

	UDPPeer *peer = m_connection->createServerPeer(address);

	// Create event
	ConnectionEvent e;
	e.peerAdded(peer->id, peer->address);
	m_connection->putEvent(e);

	Address bind_addr;

	if (address.isIPv6())
		bind_addr.setAddress((IPv6AddressBytes*) NULL);
	else
		bind_addr.setAddress(0,0,0,0);

	m_connection->m_udpSocket.Bind(bind_addr);

	// Send a dummy packet to server with peer_id = PEER_ID_INEXISTENT
	m_connection->SetPeerID(PEER_ID_INEXISTENT);
	SharedBuffer<u8> data(0);
	m_connection->Send(PEER_ID_SERVER, 0, data, true);
}

void ConnectionSendThread::disconnect()
{
	LOG(dout_con<<m_connection->getDesc()<<" disconnecting"<<std::endl);

	// Create and send DISCO packet
	SharedBuffer<u8> data(2);
	writeU8(&data[0], TYPE_CONTROL);
	writeU8(&data[1], CONTROLTYPE_DISCO);


	// Send to all
	std::list<u16> peerids = m_connection->getPeerIDs();

	for (std::list<u16>::iterator i = peerids.begin();
			i != peerids.end();
			i++)
	{
		sendAsPacket(*i, 0,data,false);
	}
}

void ConnectionSendThread::disconnect_peer(u16 peer_id)
{
	LOG(dout_con<<m_connection->getDesc()<<" disconnecting peer"<<std::endl);

	// Create and send DISCO packet
	SharedBuffer<u8> data(2);
	writeU8(&data[0], TYPE_CONTROL);
	writeU8(&data[1], CONTROLTYPE_DISCO);
	sendAsPacket(peer_id, 0,data,false);

	PeerHelper peer = m_connection->getPeerNoEx(peer_id);

	if (!peer)
		return;

	if (dynamic_cast<UDPPeer*>(&peer) == 0)
	{
		return;
	}

	dynamic_cast<UDPPeer*>(&peer)->m_pending_disconnect = true;
}

void ConnectionSendThread::send(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data)
{
	assert(channelnum < CHANNEL_COUNT);

	PeerHelper peer = m_connection->getPeerNoEx(peer_id);
	if(!peer)
	{
		LOG(dout_con<<m_connection->getDesc()<<" peer: peer_id="<<peer_id
				<< ">>>NOT<<< found on sending packet"
				<< ", channel " << (channelnum % 0xFF)
				<< ", size: " << data.getSize() <<std::endl);
		return;
	}

	LOG(dout_con<<m_connection->getDesc()<<" sending to peer_id="<<peer_id
			<< ", channel " << (channelnum % 0xFF)
			<< ", size: " << data.getSize() <<std::endl);

	u16 split_sequence_number = peer->getNextSplitSequenceNumber(channelnum);

	u32 chunksize_max = m_max_packet_size - BASE_HEADER_SIZE;
	std::list<SharedBuffer<u8> > originals;

	originals = makeAutoSplitPacket(data, chunksize_max,split_sequence_number);

	peer->setNextSplitSequenceNumber(channelnum,split_sequence_number);

	for(std::list<SharedBuffer<u8> >::iterator i = originals.begin();
		i != originals.end(); ++i)
	{
		SharedBuffer<u8> original = *i;
		sendAsPacket(peer_id, channelnum, original);
	}
}

void ConnectionSendThread::sendReliable(ConnectionCommand &c)
{
	PeerHelper peer = m_connection->getPeerNoEx(c.peer_id);
	if (!peer)
		return;

	peer->PutReliableSendCommand(c,m_max_packet_size);
}

void ConnectionSendThread::sendToAll(u8 channelnum, SharedBuffer<u8> data)
{
	std::list<u16> peerids = m_connection->getPeerIDs();

	for (std::list<u16>::iterator i = peerids.begin();
			i != peerids.end();
			i++)
	{
		send(*i, channelnum, data);
	}
}

void ConnectionSendThread::sendToAllReliable(ConnectionCommand &c)
{
	std::list<u16> peerids = m_connection->getPeerIDs();

	for (std::list<u16>::iterator i = peerids.begin();
			i != peerids.end();
			i++)
	{
		PeerHelper peer = m_connection->getPeerNoEx(*i);

		if (!peer)
			continue;

		peer->PutReliableSendCommand(c,m_max_packet_size);
	}
}

void ConnectionSendThread::sendPackets(float dtime)
{
	std::list<u16> peerIds = m_connection->getPeerIDs();
	std::list<u16> pendingDisconnect;
	std::map<u16,bool> pending_unreliable;

	for(std::list<u16>::iterator
			j = peerIds.begin();
			j != peerIds.end(); ++j)
	{
		PeerHelper peer = m_connection->getPeerNoEx(*j);
		//peer may have been removed
		if (!peer) {
			LOG(dout_con<<m_connection->getDesc()<< " Peer not found: peer_id=" << *j << std::endl);
			continue;
		}
		peer->m_increment_packets_remaining = m_iteration_packets_avaialble/m_connection->m_peers.size();

		if (dynamic_cast<UDPPeer*>(&peer) == 0)
		{
			continue;
		}

		if (dynamic_cast<UDPPeer*>(&peer)->m_pending_disconnect)
		{
			pendingDisconnect.push_back(*j);
		}

		PROFILE(std::stringstream peerIdentifier);
		PROFILE(peerIdentifier << "sendPackets[" << m_connection->getDesc() << ";" << *j << ";RELIABLE]");
		PROFILE(ScopeProfiler peerprofiler(g_profiler, peerIdentifier.str(), SPT_AVG));

		LOG(dout_con<<m_connection->getDesc()
				<< " Handle per peer queues: peer_id=" << *j
				<< " packet quota: " << peer->m_increment_packets_remaining << std::endl);
		// first send queued reliable packets for all peers (if possible)
		for (unsigned int i=0; i < CHANNEL_COUNT; i++)
		{
			u16 next_to_ack = 0;
			dynamic_cast<UDPPeer*>(&peer)->channels[i].outgoing_reliables_sent.getFirstSeqnum(next_to_ack);
			u16 next_to_receive = 0;
			dynamic_cast<UDPPeer*>(&peer)->channels[i].incoming_reliables.getFirstSeqnum(next_to_receive);

			LOG(dout_con<<m_connection->getDesc()<< "\t channel: "
						<< i << ", peer quota:"
						<< peer->m_increment_packets_remaining
						<< std::endl
					<< "\t\t\treliables on wire: "
						<< dynamic_cast<UDPPeer*>(&peer)->channels[i].outgoing_reliables_sent.size()
						<< ", waiting for ack for " << next_to_ack
						<< std::endl
					<< "\t\t\tincoming_reliables: "
						<< dynamic_cast<UDPPeer*>(&peer)->channels[i].incoming_reliables.size()
						<< ", next reliable packet: "
						<< dynamic_cast<UDPPeer*>(&peer)->channels[i].readNextIncomingSeqNum()
						<< ", next queued: " << next_to_receive
						<< std::endl
					<< "\t\t\treliables queued : "
						<< dynamic_cast<UDPPeer*>(&peer)->channels[i].queued_reliables.size()
						<< std::endl
					<< "\t\t\tqueued commands  : "
						<< dynamic_cast<UDPPeer*>(&peer)->channels[i].queued_commands.size()
						<< std::endl);

			while ((dynamic_cast<UDPPeer*>(&peer)->channels[i].queued_reliables.size() > 0) &&
					(dynamic_cast<UDPPeer*>(&peer)->channels[i].outgoing_reliables_sent.size()
							< dynamic_cast<UDPPeer*>(&peer)->channels[i].getWindowSize())&&
							(peer->m_increment_packets_remaining > 0))
			{
				BufferedPacket p = dynamic_cast<UDPPeer*>(&peer)->channels[i].queued_reliables.pop_front();
				Channel* channel = &(dynamic_cast<UDPPeer*>(&peer)->channels[i]);
				LOG(dout_con<<m_connection->getDesc()
						<<" INFO: sending a queued reliable packet "
						<<" channel: " << i
						<<", seqnum: " << readU16(&p.data[BASE_HEADER_SIZE+1])
						<< std::endl);
				sendAsPacketReliable(p,channel);
				peer->m_increment_packets_remaining--;
			}
		}
	}

	if (m_outgoing_queue.size())
	{
		LOG(dout_con<<m_connection->getDesc()
				<< " Handle non reliable queue ("
				<< m_outgoing_queue.size() << " pkts)" << std::endl);
	}

	unsigned int initial_queuesize = m_outgoing_queue.size();
	/* send non reliable packets*/
	for(unsigned int i=0;i < initial_queuesize;i++) {
		OutgoingPacket packet = m_outgoing_queue.pop_front();

		assert(!packet.reliable &&
			"reliable packets are not allowed in outgoing queue!");

		PeerHelper peer = m_connection->getPeerNoEx(packet.peer_id);
		if(!peer) {
			LOG(dout_con<<m_connection->getDesc()
							<<" Outgoing queue: peer_id="<<packet.peer_id
							<< ">>>NOT<<< found on sending packet"
							<< ", channel " << (packet.channelnum % 0xFF)
							<< ", size: " << packet.data.getSize() <<std::endl);
			continue;
		}
		/* send acks immediately */
		else if (packet.ack)
		{
			rawSendAsPacket(packet.peer_id, packet.channelnum,
								packet.data, packet.reliable);
			peer->m_increment_packets_remaining =
					MYMIN(0,peer->m_increment_packets_remaining--);
		}
		else if (
			( peer->m_increment_packets_remaining > 0) ||
			(StopRequested())){
			rawSendAsPacket(packet.peer_id, packet.channelnum,
					packet.data, packet.reliable);
			peer->m_increment_packets_remaining--;
		}
		else {
			m_outgoing_queue.push_back(packet);
			pending_unreliable[packet.peer_id] = true;
		}
	}

	for(std::list<u16>::iterator
				k = pendingDisconnect.begin();
				k != pendingDisconnect.end(); ++k)
	{
		if (!pending_unreliable[*k])
		{
			m_connection->deletePeer(*k,false);
		}
	}
}

void ConnectionSendThread::sendAsPacket(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data, bool ack)
{
	OutgoingPacket packet(peer_id, channelnum, data, false, ack);
	m_outgoing_queue.push_back(packet);
}

ConnectionReceiveThread::ConnectionReceiveThread(Connection* parent,
		unsigned int max_packet_size) :
	m_connection(parent)
{
}

void * ConnectionReceiveThread::Thread()
{
	ThreadStarted();
	log_register_thread("ConnectionReceive");

	LOG(dout_con<<m_connection->getDesc()
			<<"ConnectionReceive thread started"<<std::endl);

	PROFILE(std::stringstream ThreadIdentifier);
	PROFILE(ThreadIdentifier << "ConnectionReceive: [" << m_connection->getDesc() << "]");

	porting::setThreadName("ConnectionReceive");

#ifdef DEBUG_CONNECTION_KBPS
	u32 curtime = porting::getTimeMs();
	u32 lasttime = curtime;
	float debug_print_timer = 0.0;
#endif

	while(!StopRequested()) {
		BEGIN_DEBUG_EXCEPTION_HANDLER
		PROFILE(ScopeProfiler sp(g_profiler, ThreadIdentifier.str(), SPT_AVG));

#ifdef DEBUG_CONNECTION_KBPS
		lasttime = curtime;
		curtime = porting::getTimeMs();
		float dtime = CALC_DTIME(lasttime,curtime);
#endif

		/* receive packets */
		receive();

#ifdef DEBUG_CONNECTION_KBPS
		debug_print_timer += dtime;
		if (debug_print_timer > 20.0) {
			debug_print_timer -= 20.0;

			std::list<u16> peerids = m_connection->getPeerIDs();

			for (std::list<u16>::iterator i = peerids.begin();
					i != peerids.end();
					i++)
			{
				PeerHelper peer = m_connection->getPeerNoEx(*i);
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
		END_DEBUG_EXCEPTION_HANDLER(errorstream);
	}
	PROFILE(g_profiler->remove(ThreadIdentifier.str()));
	return NULL;
}

// Receive packets from the network and buffers and create ConnectionEvents
void ConnectionReceiveThread::receive()
{
	// use IPv6 minimum allowed MTU as receive buffer size as this is
	// theoretical reliable upper boundary of a udp packet for all IPv6 enabled
	// infrastructure
	unsigned int packet_maxsize = 1500;
	SharedBuffer<u8> packetdata(packet_maxsize);

	bool packet_queued = true;

	unsigned int loop_count = 0;

	/* first of all read packets from socket */
	/* check for incoming data available */
	while( (loop_count < 10) &&
			(m_connection->m_udpSocket.WaitData(50)))
	{
		loop_count++;
	try{
		if (packet_queued)
		{
			bool no_data_left = false;
			u16 peer_id;
			SharedBuffer<u8> resultdata;
			while(!no_data_left)
			{
				try {
					no_data_left = !getFromBuffers(peer_id, resultdata);
					if (!no_data_left) {
						ConnectionEvent e;
						e.dataReceived(peer_id, resultdata);
						m_connection->putEvent(e);
					}
				}
				catch(ProcessedSilentlyException &e) {
					/* try reading again */
				}
			}
			packet_queued = false;
		}

		Address sender;
		s32 received_size = m_connection->m_udpSocket.Receive(sender, *packetdata, packet_maxsize);

		if ((received_size < 0) ||
			(received_size < BASE_HEADER_SIZE) ||
			(readU32(&packetdata[0]) != m_connection->GetProtocolID()))
		{
			LOG(derr_con<<m_connection->getDesc()
					<<"Receive(): Invalid incoming packet, "
					<<"size: " << received_size
					<<", protocol: "
					<< ((received_size >= 4) ? readU32(&packetdata[0]) : -1)
					<< std::endl);
			continue;
		}

		u16 peer_id          = readPeerId(*packetdata);
		u8 channelnum        = readChannel(*packetdata);

		if(channelnum > CHANNEL_COUNT-1){
			LOG(derr_con<<m_connection->getDesc()
					<<"Receive(): Invalid channel "<<channelnum<<std::endl);
			throw InvalidIncomingDataException("Channel doesn't exist");
		}

		/* preserve original peer_id for later usage */
		u16 packet_peer_id   = peer_id;

		/* Try to identify peer by sender address (may happen on join) */
		if(peer_id == PEER_ID_INEXISTENT)
		{
			peer_id = m_connection->lookupPeer(sender);
		}

		/* The peer was not found in our lists. Add it. */
		if(peer_id == PEER_ID_INEXISTENT)
		{
			peer_id = m_connection->createPeer(sender, MTP_MINETEST_RELIABLE_UDP, 0);
		}

		PeerHelper peer = m_connection->getPeerNoEx(peer_id);

		if (!peer) {
			LOG(dout_con<<m_connection->getDesc()
					<<" got packet from unknown peer_id: "
					<<peer_id<<" Ignoring."<<std::endl);
			continue;
		}

		// Validate peer address

		Address peer_address;

		if (peer->getAddress(MTP_UDP, peer_address)) {
			if (peer_address != sender) {
				LOG(derr_con<<m_connection->getDesc()
						<<m_connection->getDesc()
						<<" Peer "<<peer_id<<" sending from different address."
						" Ignoring."<<std::endl);
				continue;
			}
		}
		else {

			bool invalid_address = true;
			if (invalid_address) {
				LOG(derr_con<<m_connection->getDesc()
						<<m_connection->getDesc()
						<<" Peer "<<peer_id<<" unknown."
						" Ignoring."<<std::endl);
				continue;
			}
		}


		/* mark peer as seen with id */
		if (!(packet_peer_id == PEER_ID_INEXISTENT))
			peer->setSentWithID();

		peer->ResetTimeout();

		Channel *channel = 0;

		if (dynamic_cast<UDPPeer*>(&peer) != 0)
		{
			channel = &(dynamic_cast<UDPPeer*>(&peer)->channels[channelnum]);
		}

		if (channel != 0) {
			channel->UpdateBytesReceived(received_size);
		}

		// Throw the received packet to channel->processPacket()

		// Make a new SharedBuffer from the data without the base headers
		SharedBuffer<u8> strippeddata(received_size - BASE_HEADER_SIZE);
		memcpy(*strippeddata, &packetdata[BASE_HEADER_SIZE],
				strippeddata.getSize());

		try{
			// Process it (the result is some data with no headers made by us)
			SharedBuffer<u8> resultdata = processPacket
					(channel, strippeddata, peer_id, channelnum, false);

			LOG(dout_con<<m_connection->getDesc()
					<<" ProcessPacket from peer_id: " << peer_id
					<< ",channel: " << (channelnum & 0xFF) << ", returned "
					<< resultdata.getSize() << " bytes" <<std::endl);

			ConnectionEvent e;
			e.dataReceived(peer_id, resultdata);
			m_connection->putEvent(e);
		}catch(ProcessedSilentlyException &e){
		}catch(ProcessedQueued &e){
			packet_queued = true;
		}
	}catch(InvalidIncomingDataException &e){
	}
	catch(ProcessedSilentlyException &e){
	}
	}
}

bool ConnectionReceiveThread::getFromBuffers(u16 &peer_id, SharedBuffer<u8> &dst)
{
	std::list<u16> peerids = m_connection->getPeerIDs();

	for(std::list<u16>::iterator j = peerids.begin();
		j != peerids.end(); ++j)
	{
		PeerHelper peer = m_connection->getPeerNoEx(*j);
		if (!peer)
			continue;

		if(dynamic_cast<UDPPeer*>(&peer) == 0)
			continue;

		for(u16 i=0; i<CHANNEL_COUNT; i++)
		{
			Channel *channel = &(dynamic_cast<UDPPeer*>(&peer))->channels[i];

			SharedBuffer<u8> resultdata;
			bool got = checkIncomingBuffers(channel, peer_id, resultdata);
			if(got){
				dst = resultdata;
				return true;
			}
		}
	}
	return false;
}

bool ConnectionReceiveThread::checkIncomingBuffers(Channel *channel,
		u16 &peer_id, SharedBuffer<u8> &dst)
{
	u16 firstseqnum = 0;
	if (channel->incoming_reliables.getFirstSeqnum(firstseqnum))
	{
		if(firstseqnum == channel->readNextIncomingSeqNum())
		{
			BufferedPacket p = channel->incoming_reliables.popFirst();
			peer_id = readPeerId(*p.data);
			u8 channelnum = readChannel(*p.data);
			u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);

			LOG(dout_con<<m_connection->getDesc()
					<<"UNBUFFERING TYPE_RELIABLE"
					<<" seqnum="<<seqnum
					<<" peer_id="<<peer_id
					<<" channel="<<((int)channelnum&0xff)
					<<std::endl);

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
		SharedBuffer<u8> packetdata, u16 peer_id, u8 channelnum, bool reliable)
{
	PeerHelper peer = m_connection->getPeer(peer_id);

	if(packetdata.getSize() < 1)
		throw InvalidIncomingDataException("packetdata.getSize() < 1");

	u8 type = readU8(&(packetdata[0]));

	if(type == TYPE_CONTROL)
	{
		if(packetdata.getSize() < 2)
			throw InvalidIncomingDataException("packetdata.getSize() < 2");

		u8 controltype = readU8(&(packetdata[1]));

		if( (controltype == CONTROLTYPE_ACK)
				&& (peer_id <= MAX_UDP_PEERS))
		{
			assert(channel != 0);
			if(packetdata.getSize() < 4)
				throw InvalidIncomingDataException
						("packetdata.getSize() < 4 (ACK header size)");

			u16 seqnum = readU16(&packetdata[2]);
			LOG(dout_con<<m_connection->getDesc()
					<<" [ CONTROLTYPE_ACK: channelnum="
					<<((int)channelnum&0xff)<<", peer_id="<<peer_id
					<<", seqnum="<<seqnum<< " ]"<<std::endl);

			try{
				BufferedPacket p =
						channel->outgoing_reliables_sent.popSeqnum(seqnum);

				// only calculate rtt from straight sent packets
				if (p.resend_count == 0) {
					// Get round trip time
					unsigned int current_time = porting::getTimeMs();

					// a overflow is quite unlikely but as it'd result in major
					// rtt miscalculation we handle it here
					if (current_time > p.absolute_send_time)
					{
						float rtt = (current_time - p.absolute_send_time) / 1000.0;

						// Let peer calculate stuff according to it
						// (avg_rtt and resend_timeout)
						dynamic_cast<UDPPeer*>(&peer)->reportRTT(rtt);
					}
					else if (p.totaltime > 0)
					{
						float rtt = p.totaltime;

						// Let peer calculate stuff according to it
						// (avg_rtt and resend_timeout)
						dynamic_cast<UDPPeer*>(&peer)->reportRTT(rtt);
					}
				}
				//put bytes for max bandwidth calculation
				channel->UpdateBytesSent(p.data.getSize(),1);
				if (channel->outgoing_reliables_sent.size() == 0)
				{
					m_connection->TriggerSend();
				}
			}
			catch(NotFoundException &e){
				LOG(derr_con<<m_connection->getDesc()
						<<"WARNING: ACKed packet not "
						"in outgoing queue"
						<<std::endl);
				channel->UpdatePacketTooLateCounter();
			}
			throw ProcessedSilentlyException("Got an ACK");
		}
		else if((controltype == CONTROLTYPE_SET_PEER_ID)
				&& (peer_id <= MAX_UDP_PEERS))
		{
			// Got a packet to set our peer id
			if(packetdata.getSize() < 4)
				throw InvalidIncomingDataException
						("packetdata.getSize() < 4 (SET_PEER_ID header size)");
			u16 peer_id_new = readU16(&packetdata[2]);
			LOG(dout_con<<m_connection->getDesc()
					<<"Got new peer id: "<<peer_id_new<<"... "<<std::endl);

			if(m_connection->GetPeerID() != PEER_ID_INEXISTENT)
			{
				LOG(derr_con<<m_connection->getDesc()
						<<"WARNING: Not changing"
						" existing peer id."<<std::endl);
			}
			else
			{
				LOG(dout_con<<m_connection->getDesc()<<"changing own peer id"<<std::endl);
				m_connection->SetPeerID(peer_id_new);
			}

			ConnectionCommand cmd;

			SharedBuffer<u8> reply(2);
			writeU8(&reply[0], TYPE_CONTROL);
			writeU8(&reply[1], CONTROLTYPE_ENABLE_BIG_SEND_WINDOW);
			cmd.disableLegacy(PEER_ID_SERVER,reply);
			m_connection->putCommand(cmd);

			throw ProcessedSilentlyException("Got a SET_PEER_ID");
		}
		else if((controltype == CONTROLTYPE_PING)
				&& (peer_id <= MAX_UDP_PEERS))
		{
			// Just ignore it, the incoming data already reset
			// the timeout counter
			LOG(dout_con<<m_connection->getDesc()<<"PING"<<std::endl);
			throw ProcessedSilentlyException("Got a PING");
		}
		else if(controltype == CONTROLTYPE_DISCO)
		{
			// Just ignore it, the incoming data already reset
			// the timeout counter
			LOG(dout_con<<m_connection->getDesc()
					<<"DISCO: Removing peer "<<(peer_id)<<std::endl);

			if(m_connection->deletePeer(peer_id, false) == false)
			{
				derr_con<<m_connection->getDesc()
						<<"DISCO: Peer not found"<<std::endl;
			}

			throw ProcessedSilentlyException("Got a DISCO");
		}
		else if((controltype == CONTROLTYPE_ENABLE_BIG_SEND_WINDOW)
				&& (peer_id <= MAX_UDP_PEERS))
		{
			dynamic_cast<UDPPeer*>(&peer)->setNonLegacyPeer();
			throw ProcessedSilentlyException("Got non legacy control");
		}
		else{
			LOG(derr_con<<m_connection->getDesc()
					<<"INVALID TYPE_CONTROL: invalid controltype="
					<<((int)controltype&0xff)<<std::endl);
			throw InvalidIncomingDataException("Invalid control type");
		}
	}
	else if(type == TYPE_ORIGINAL)
	{
		if(packetdata.getSize() <= ORIGINAL_HEADER_SIZE)
			throw InvalidIncomingDataException
					("packetdata.getSize() <= ORIGINAL_HEADER_SIZE");
		LOG(dout_con<<m_connection->getDesc()
				<<"RETURNING TYPE_ORIGINAL to user"
				<<std::endl);
		// Get the inside packet out and return it
		SharedBuffer<u8> payload(packetdata.getSize() - ORIGINAL_HEADER_SIZE);
		memcpy(*payload, &(packetdata[ORIGINAL_HEADER_SIZE]), payload.getSize());
		return payload;
	}
	else if(type == TYPE_SPLIT)
	{
		Address peer_address;

		if (peer->getAddress(MTP_UDP, peer_address)) {

			// We have to create a packet again for buffering
			// This isn't actually too bad an idea.
			BufferedPacket packet = makePacket(
					peer_address,
					packetdata,
					m_connection->GetProtocolID(),
					peer_id,
					channelnum);

			// Buffer the packet
			SharedBuffer<u8> data =
					peer->addSpiltPacket(channelnum,packet,reliable);

			if(data.getSize() != 0)
			{
				LOG(dout_con<<m_connection->getDesc()
						<<"RETURNING TYPE_SPLIT: Constructed full data, "
						<<"size="<<data.getSize()<<std::endl);
				return data;
			}
			LOG(dout_con<<m_connection->getDesc()<<"BUFFERED TYPE_SPLIT"<<std::endl);
			throw ProcessedSilentlyException("Buffered a split packet chunk");
		}
		else {
			//TODO throw some error
		}
	}
	else if((peer_id <= MAX_UDP_PEERS) && (type == TYPE_RELIABLE))
	{
		assert(channel != 0);
		// Recursive reliable packets not allowed
		if(reliable)
			throw InvalidIncomingDataException("Found nested reliable packets");

		if(packetdata.getSize() < RELIABLE_HEADER_SIZE)
			throw InvalidIncomingDataException
					("packetdata.getSize() < RELIABLE_HEADER_SIZE");

		u16 seqnum = readU16(&packetdata[1]);
		bool is_future_packet = false;
		bool is_old_packet = false;

		/* packet is within our receive window send ack */
		if (seqnum_in_window(seqnum, channel->readNextIncomingSeqNum(),MAX_RELIABLE_WINDOW_SIZE))
		{
			m_connection->sendAck(peer_id,channelnum,seqnum);
		}
		else {
			is_future_packet = seqnum_higher(seqnum, channel->readNextIncomingSeqNum());
			is_old_packet    = seqnum_higher(channel->readNextIncomingSeqNum(), seqnum);


			/* packet is not within receive window, don't send ack.           *
			 * if this was a valid packet it's gonna be retransmitted         */
			if (is_future_packet)
			{
				throw ProcessedSilentlyException("Received packet newer then expected, not sending ack");
			}

			/* seems like our ack was lost, send another one for a old packet */
			if (is_old_packet)
			{
				LOG(dout_con<<m_connection->getDesc()
						<< "RE-SENDING ACK: peer_id: " << peer_id
						<< ", channel: " << (channelnum&0xFF)
						<< ", seqnum: " << seqnum << std::endl;)
				m_connection->sendAck(peer_id,channelnum,seqnum);

				// we already have this packet so this one was on wire at least
				// the current timeout
				// we don't know how long this packet was on wire don't do silly guessing
				// dynamic_cast<UDPPeer*>(&peer)->reportRTT(dynamic_cast<UDPPeer*>(&peer)->getResendTimeout());

				throw ProcessedSilentlyException("Retransmitting ack for old packet");
			}
		}

		if (seqnum != channel->readNextIncomingSeqNum())
		{
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
					peer_id,
					channelnum);
			try{
				channel->incoming_reliables.insert(packet,channel->readNextIncomingSeqNum());

				LOG(dout_con<<m_connection->getDesc()
						<< "BUFFERING, TYPE_RELIABLE peer_id: " << peer_id
						<< ", channel: " << (channelnum&0xFF)
						<< ", seqnum: " << seqnum << std::endl;)

				throw ProcessedQueued("Buffered future reliable packet");
			}
			catch(AlreadyExistsException &e)
			{
			}
			catch(IncomingDataCorruption &e)
			{
				ConnectionCommand discon;
				discon.disconnect_peer(peer_id);
				m_connection->putCommand(discon);

				LOG(derr_con<<m_connection->getDesc()
						<< "INVALID, TYPE_RELIABLE peer_id: " << peer_id
						<< ", channel: " << (channelnum&0xFF)
						<< ", seqnum: " << seqnum
						<< "DROPPING CLIENT!" << std::endl;)
			}
		}

		/* we got a packet to process right now */
		LOG(dout_con<<m_connection->getDesc()
				<< "RECURSIVE, TYPE_RELIABLE peer_id: " << peer_id
				<< ", channel: " << (channelnum&0xFF)
				<< ", seqnum: " << seqnum << std::endl;)


		/* check for resend case */
		u16 queued_seqnum = 0;
		if (channel->incoming_reliables.getFirstSeqnum(queued_seqnum))
		{
			if (queued_seqnum == seqnum)
			{
				BufferedPacket queued_packet = channel->incoming_reliables.popFirst();
				/** TODO find a way to verify the new against the old packet */
			}
		}

		channel->incNextIncomingSeqNum();

		// Get out the inside packet and re-process it
		SharedBuffer<u8> payload(packetdata.getSize() - RELIABLE_HEADER_SIZE);
		memcpy(*payload, &packetdata[RELIABLE_HEADER_SIZE], payload.getSize());

		return processPacket(channel, payload, peer_id, channelnum, true);
	}
	else
	{
		derr_con<<m_connection->getDesc()
				<<"Got invalid type="<<((int)type&0xff)<<std::endl;
		throw InvalidIncomingDataException("Invalid packet type");
	}

	// We should never get here.
	// If you get here, add an exception or a return to some of the
	// above conditionals.
	assert(0);
	throw BaseException("Error in Channel::ProcessPacket()");
}

/*
	Connection
*/

Connection::Connection(u32 protocol_id, u32 max_packet_size, float timeout,
		bool ipv6) :
	m_udpSocket(ipv6),
	m_command_queue(),
	m_event_queue(),
	m_peer_id(0),
	m_protocol_id(protocol_id),
	m_sendThread(this, max_packet_size, timeout),
	m_receiveThread(this, max_packet_size),
	m_info_mutex(),
	m_bc_peerhandler(0),
	m_bc_receive_timeout(0),
	m_shutting_down(false),
	m_next_remote_peer_id(2)
{
	m_udpSocket.setTimeoutMs(5);

	m_sendThread.Start();
	m_receiveThread.Start();
}

Connection::Connection(u32 protocol_id, u32 max_packet_size, float timeout,
		bool ipv6, PeerHandler *peerhandler) :
	m_udpSocket(ipv6),
	m_command_queue(),
	m_event_queue(),
	m_peer_id(0),
	m_protocol_id(protocol_id),
	m_sendThread(this, max_packet_size, timeout),
	m_receiveThread(this, max_packet_size),
	m_info_mutex(),
	m_bc_peerhandler(peerhandler),
	m_bc_receive_timeout(0),
	m_shutting_down(false),
	m_next_remote_peer_id(2)

{
	m_udpSocket.setTimeoutMs(5);

	m_sendThread.Start();
	m_receiveThread.Start();

}


Connection::~Connection()
{
	m_shutting_down = true;
	// request threads to stop
	m_sendThread.Stop();
	m_receiveThread.Stop();

	//TODO for some unkonwn reason send/receive threads do not exit as they're
	// supposed to be but wait on peer timeout. To speed up shutdown we reduce
	// timeout to half a second.
	m_sendThread.setPeerTimeout(0.5);

	// wait for threads to finish
	m_sendThread.Wait();
	m_receiveThread.Wait();

	// Delete peers
	for(std::map<u16, Peer*>::iterator
			j = m_peers.begin();
			j != m_peers.end(); ++j)
	{
		delete j->second;
	}
}

/* Internal stuff */
void Connection::putEvent(ConnectionEvent &e)
{
	assert(e.type != CONNEVENT_NONE);
	m_event_queue.push_back(e);
}

PeerHelper Connection::getPeer(u16 peer_id)
{
	JMutexAutoLock peerlock(m_peers_mutex);
	std::map<u16, Peer*>::iterator node = m_peers.find(peer_id);

	if(node == m_peers.end()){
		throw PeerNotFoundException("GetPeer: Peer not found (possible timeout)");
	}

	// Error checking
	assert(node->second->id == peer_id);

	return PeerHelper(node->second);
}

PeerHelper Connection::getPeerNoEx(u16 peer_id)
{
	JMutexAutoLock peerlock(m_peers_mutex);
	std::map<u16, Peer*>::iterator node = m_peers.find(peer_id);

	if(node == m_peers.end()){
		return PeerHelper(NULL);
	}

	// Error checking
	assert(node->second->id == peer_id);

	return PeerHelper(node->second);
}

/* find peer_id for address */
u16 Connection::lookupPeer(Address& sender)
{
	JMutexAutoLock peerlock(m_peers_mutex);
	std::map<u16, Peer*>::iterator j;
	j = m_peers.begin();
	for(; j != m_peers.end(); ++j)
	{
		Peer *peer = j->second;
		if(peer->isActive())
			continue;

		Address tocheck;

		if ((peer->getAddress(MTP_MINETEST_RELIABLE_UDP, tocheck)) && (tocheck == sender))
			return peer->id;

		if ((peer->getAddress(MTP_UDP, tocheck)) && (tocheck == sender))
			return peer->id;
	}

	return PEER_ID_INEXISTENT;
}

std::list<Peer*> Connection::getPeers()
{
	std::list<Peer*> list;
	for(std::map<u16, Peer*>::iterator j = m_peers.begin();
		j != m_peers.end(); ++j)
	{
		Peer *peer = j->second;
		list.push_back(peer);
	}
	return list;
}

bool Connection::deletePeer(u16 peer_id, bool timeout)
{
	Peer *peer = 0;

	/* lock list as short as possible */
	{
		JMutexAutoLock peerlock(m_peers_mutex);
		if(m_peers.find(peer_id) == m_peers.end())
			return false;
		peer = m_peers[peer_id];
		m_peers.erase(peer_id);
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

ConnectionEvent Connection::getEvent()
{
	if(m_event_queue.empty()){
		ConnectionEvent e;
		e.type = CONNEVENT_NONE;
		return e;
	}
	return m_event_queue.pop_frontNoEx();
}

ConnectionEvent Connection::waitEvent(u32 timeout_ms)
{
	try{
		return m_event_queue.pop_front(timeout_ms);
	} catch(ItemNotFoundException &ex){
		ConnectionEvent e;
		e.type = CONNEVENT_NONE;
		return e;
	}
}

void Connection::putCommand(ConnectionCommand &c)
{
	if (!m_shutting_down)
	{
		m_command_queue.push_back(c);
		m_sendThread.Trigger();
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
	JMutexAutoLock peerlock(m_peers_mutex);

	if(m_peers.size() != 1)
		return false;

	std::map<u16, Peer*>::iterator node = m_peers.find(PEER_ID_SERVER);
	if(node == m_peers.end())
		return false;

	if(m_peer_id == PEER_ID_INEXISTENT)
		return false;

	return true;
}

void Connection::Disconnect()
{
	ConnectionCommand c;
	c.disconnect();
	putCommand(c);
}

u32 Connection::Receive(u16 &peer_id, SharedBuffer<u8> &data)
{
	for(;;){
		ConnectionEvent e = waitEvent(m_bc_receive_timeout);
		if(e.type != CONNEVENT_NONE)
			LOG(dout_con<<getDesc()<<": Receive: got event: "
					<<e.describe()<<std::endl);
		switch(e.type){
		case CONNEVENT_NONE:
			throw NoIncomingDataException("No incoming data");
		case CONNEVENT_DATA_RECEIVED:
			peer_id = e.peer_id;
			data = SharedBuffer<u8>(e.data);
			return e.data.getSize();
		case CONNEVENT_PEER_ADDED: {
			UDPPeer tmp(e.peer_id, e.address, this);
			if(m_bc_peerhandler)
				m_bc_peerhandler->peerAdded(&tmp);
			continue; }
		case CONNEVENT_PEER_REMOVED: {
			UDPPeer tmp(e.peer_id, e.address, this);
			if(m_bc_peerhandler)
				m_bc_peerhandler->deletingPeer(&tmp, e.timeout);
			continue; }
		case CONNEVENT_BIND_FAILED:
			throw ConnectionBindFailed("Failed to bind socket "
					"(port already in use?)");
		}
	}
	throw NoIncomingDataException("No incoming data");
}

void Connection::SendToAll(u8 channelnum, SharedBuffer<u8> data, bool reliable)
{
	assert(channelnum < CHANNEL_COUNT);

	ConnectionCommand c;
	c.sendToAll(channelnum, data, reliable);
	putCommand(c);
}

void Connection::Send(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data, bool reliable)
{
	assert(channelnum < CHANNEL_COUNT);

	ConnectionCommand c;
	c.send(peer_id, channelnum, data, reliable);
	putCommand(c);
}

Address Connection::GetPeerAddress(u16 peer_id)
{
	PeerHelper peer = getPeerNoEx(peer_id);

	if (!peer)
		throw PeerNotFoundException("No address for peer found!");
	Address peer_address;
	peer->getAddress(MTP_PRIMARY, peer_address);
	return peer_address;
}

float Connection::getPeerStat(u16 peer_id, rtt_stat_type type)
{
	PeerHelper peer = getPeerNoEx(peer_id);
	if (!peer) return -1;
	return peer->getStat(type);
}

float Connection::getLocalStat(rate_stat_type type)
{
	PeerHelper peer = getPeerNoEx(PEER_ID_SERVER);

	if (!peer) {
		assert("Connection::getLocalStat we couldn't get our own peer? are you serious???" == 0);
	}

	float retval = 0.0;

	for (u16 j=0; j<CHANNEL_COUNT; j++) {
		switch(type) {
			case CUR_DL_RATE:
				retval += dynamic_cast<UDPPeer*>(&peer)->channels[j].getCurrentDownloadRateKB();
				break;
			case AVG_DL_RATE:
				retval += dynamic_cast<UDPPeer*>(&peer)->channels[j].getAvgDownloadRateKB();
				break;
			case CUR_INC_RATE:
				retval += dynamic_cast<UDPPeer*>(&peer)->channels[j].getCurrentIncomingRateKB();
				break;
			case AVG_INC_RATE:
				retval += dynamic_cast<UDPPeer*>(&peer)->channels[j].getAvgIncomingRateKB();
				break;
			case AVG_LOSS_RATE:
				retval += dynamic_cast<UDPPeer*>(&peer)->channels[j].getAvgLossRateKB();
				break;
			case CUR_LOSS_RATE:
				retval += dynamic_cast<UDPPeer*>(&peer)->channels[j].getCurrentLossRateKB();
				break;
		default:
			assert("Connection::getLocalStat Invalid stat type" == 0);
		}
	}
	return retval;
}

u16 Connection::createPeer(Address& sender, MTProtocols protocol, int fd)
{
	// Somebody wants to make a new connection

	// Get a unique peer id (2 or higher)
	u16 peer_id_new = m_next_remote_peer_id;
	u16 overflow =  MAX_UDP_PEERS;

	/*
		Find an unused peer id
	*/
	{
	JMutexAutoLock lock(m_peers_mutex);
		bool out_of_ids = false;
		for(;;)
		{
			// Check if exists
			if(m_peers.find(peer_id_new) == m_peers.end())
				break;
			// Check for overflow
			if(peer_id_new == overflow){
				out_of_ids = true;
				break;
			}
			peer_id_new++;
		}
		if(out_of_ids){
			errorstream<<getDesc()<<" ran out of peer ids"<<std::endl;
			return PEER_ID_INEXISTENT;
		}

		// Create a peer
		Peer *peer = 0;
		peer = new UDPPeer(peer_id_new, sender, this);

		m_peers[peer->id] = peer;
	}

	m_next_remote_peer_id = (peer_id_new +1) % MAX_UDP_PEERS;

	LOG(dout_con<<getDesc()
			<<"createPeer(): giving peer_id="<<peer_id_new<<std::endl);

	ConnectionCommand cmd;
	SharedBuffer<u8> reply(4);
	writeU8(&reply[0], TYPE_CONTROL);
	writeU8(&reply[1], CONTROLTYPE_SET_PEER_ID);
	writeU16(&reply[2], peer_id_new);
	cmd.createPeer(peer_id_new,reply);
	this->putCommand(cmd);

	// Create peer addition event
	ConnectionEvent e;
	e.peerAdded(peer_id_new, sender);
	putEvent(e);

	// We're now talking to a valid peer_id
	return peer_id_new;
}

void Connection::PrintInfo(std::ostream &out)
{
	m_info_mutex.Lock();
	out<<getDesc()<<": ";
	m_info_mutex.Unlock();
}

void Connection::PrintInfo()
{
	PrintInfo(dout_con);
}

const std::string Connection::getDesc()
{
	return std::string("con(")+
			itos(m_udpSocket.GetHandle())+"/"+itos(m_peer_id)+")";
}

void Connection::DisconnectPeer(u16 peer_id)
{
	ConnectionCommand discon;
	discon.disconnect_peer(peer_id);
	putCommand(discon);
}

void Connection::sendAck(u16 peer_id, u8 channelnum, u16 seqnum)
{
	assert(channelnum < CHANNEL_COUNT);

	LOG(dout_con<<getDesc()
			<<" Queuing ACK command to peer_id: " << peer_id <<
			" channel: " << (channelnum & 0xFF) <<
			" seqnum: " << seqnum << std::endl);

	ConnectionCommand c;
	SharedBuffer<u8> ack(4);
	writeU8(&ack[0], TYPE_CONTROL);
	writeU8(&ack[1], CONTROLTYPE_ACK);
	writeU16(&ack[2], seqnum);

	c.ack(peer_id, channelnum, ack);
	putCommand(c);
	m_sendThread.Trigger();
}

UDPPeer* Connection::createServerPeer(Address& address)
{
	if (getPeerNoEx(PEER_ID_SERVER) != 0)
	{
		throw ConnectionException("Already connected to a server");
	}

	UDPPeer *peer = new UDPPeer(PEER_ID_SERVER, address, this);

	{
		JMutexAutoLock lock(m_peers_mutex);
		m_peers[peer->id] = peer;
	}

	return peer;
}

std::list<u16> Connection::getPeerIDs()
{
	std::list<u16> retval;

	JMutexAutoLock lock(m_peers_mutex);
	for(std::map<u16, Peer*>::iterator j = m_peers.begin();
		j != m_peers.end(); ++j)
	{
		retval.push_back(j->first);
	}
	return retval;
}

} // namespace
