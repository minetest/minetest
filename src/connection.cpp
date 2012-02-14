/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "connection.h"
#include "main.h"
#include "serialization.h"
#include "log.h"
#include "porting.h"

namespace con
{

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

	writeU8(&b[0], TYPE_ORIGINAL);

	memcpy(&b[header_size], *data, data.getSize());

	return b;
}

core::list<SharedBuffer<u8> > makeSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 seqnum)
{
	// Chunk packets, containing the TYPE_SPLIT header
	core::list<SharedBuffer<u8> > chunks;
	
	u32 chunk_header_size = 7;
	u32 maximum_data_size = chunksize_max - chunk_header_size;
	u32 start = 0;
	u32 end = 0;
	u32 chunk_num = 0;
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
		
		start = end + 1;
		chunk_num++;
	}
	while(end != data.getSize() - 1);

	u16 chunk_count = chunks.getSize();

	core::list<SharedBuffer<u8> >::Iterator i = chunks.begin();
	for(; i != chunks.end(); i++)
	{
		// Write chunk_count
		writeU16(&((*i)[3]), chunk_count);
	}

	return chunks;
}

core::list<SharedBuffer<u8> > makeAutoSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 &split_seqnum)
{
	u32 original_header_size = 1;
	core::list<SharedBuffer<u8> > list;
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
	/*dstream<<"BEGIN SharedBuffer<u8> makeReliablePacket()"<<std::endl;
	dstream<<"data.getSize()="<<data.getSize()<<", data[0]="
			<<((unsigned int)data[0]&0xff)<<std::endl;*/
	u32 header_size = 3;
	u32 packet_size = data.getSize() + header_size;
	SharedBuffer<u8> b(packet_size);

	writeU8(&b[0], TYPE_RELIABLE);
	writeU16(&b[1], seqnum);

	memcpy(&b[header_size], *data, data.getSize());

	/*dstream<<"data.getSize()="<<data.getSize()<<", data[0]="
			<<((unsigned int)data[0]&0xff)<<std::endl;*/
	//dstream<<"END SharedBuffer<u8> makeReliablePacket()"<<std::endl;
	return b;
}

/*
	ReliablePacketBuffer
*/

void ReliablePacketBuffer::print()
{
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	for(; i != m_list.end(); i++)
	{
		u16 s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		dout_con<<s<<" ";
	}
}
bool ReliablePacketBuffer::empty()
{
	return m_list.empty();
}
u32 ReliablePacketBuffer::size()
{
	return m_list.getSize();
}
RPBSearchResult ReliablePacketBuffer::findPacket(u16 seqnum)
{
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	for(; i != m_list.end(); i++)
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
u16 ReliablePacketBuffer::getFirstSeqnum()
{
	if(empty())
		throw NotFoundException("Buffer is empty");
	BufferedPacket p = *m_list.begin();
	return readU16(&p.data[BASE_HEADER_SIZE+1]);
}
BufferedPacket ReliablePacketBuffer::popFirst()
{
	if(empty())
		throw NotFoundException("Buffer is empty");
	BufferedPacket p = *m_list.begin();
	core::list<BufferedPacket>::Iterator i = m_list.begin();
	m_list.erase(i);
	return p;
}
BufferedPacket ReliablePacketBuffer::popSeqnum(u16 seqnum)
{
	RPBSearchResult r = findPacket(seqnum);
	if(r == notFound()){
		dout_con<<"Not found"<<std::endl;
		throw NotFoundException("seqnum not found in buffer");
	}
	BufferedPacket p = *r;
	m_list.erase(r);
	return p;
}
void ReliablePacketBuffer::insert(BufferedPacket &p)
{
	assert(p.data.getSize() >= BASE_HEADER_SIZE+3);
	u8 type = readU8(&p.data[BASE_HEADER_SIZE+0]);
	assert(type == TYPE_RELIABLE);
	u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);

	// Find the right place for the packet and insert it there

	// If list is empty, just add it
	if(m_list.empty())
	{
		m_list.push_back(p);
		// Done.
		return;
	}
	// Otherwise find the right place
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	// Find the first packet in the list which has a higher seqnum
	for(; i != m_list.end(); i++){
		u16 s = readU16(&(i->data[BASE_HEADER_SIZE+1]));
		if(s == seqnum){
			throw AlreadyExistsException("Same seqnum in list");
		}
		if(seqnum_higher(s, seqnum)){
			break;
		}
	}
	// If we're at the end of the list, add the packet to the
	// end of the list
	if(i == m_list.end())
	{
		m_list.push_back(p);
		// Done.
		return;
	}
	// Insert before i
	m_list.insert_before(i, p);
}

void ReliablePacketBuffer::incrementTimeouts(float dtime)
{
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	for(; i != m_list.end(); i++){
		i->time += dtime;
		i->totaltime += dtime;
	}
}

void ReliablePacketBuffer::resetTimedOuts(float timeout)
{
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	for(; i != m_list.end(); i++){
		if(i->time >= timeout)
			i->time = 0.0;
	}
}

bool ReliablePacketBuffer::anyTotaltimeReached(float timeout)
{
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	for(; i != m_list.end(); i++){
		if(i->totaltime >= timeout)
			return true;
	}
	return false;
}

core::list<BufferedPacket> ReliablePacketBuffer::getTimedOuts(float timeout)
{
	core::list<BufferedPacket> timed_outs;
	core::list<BufferedPacket>::Iterator i;
	i = m_list.begin();
	for(; i != m_list.end(); i++)
	{
		if(i->time >= timeout)
			timed_outs.push_back(*i);
	}
	return timed_outs;
}

/*
	IncomingSplitBuffer
*/

IncomingSplitBuffer::~IncomingSplitBuffer()
{
	core::map<u16, IncomingSplitPacket*>::Iterator i;
	i = m_buf.getIterator();
	for(; i.atEnd() == false; i++)
	{
		delete i.getNode()->getValue();
	}
}
/*
	This will throw a GotSplitPacketException when a full
	split packet is constructed.
*/
SharedBuffer<u8> IncomingSplitBuffer::insert(BufferedPacket &p, bool reliable)
{
	u32 headersize = BASE_HEADER_SIZE + 7;
	assert(p.data.getSize() >= headersize);
	u8 type = readU8(&p.data[BASE_HEADER_SIZE+0]);
	assert(type == TYPE_SPLIT);
	u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);
	u16 chunk_count = readU16(&p.data[BASE_HEADER_SIZE+3]);
	u16 chunk_num = readU16(&p.data[BASE_HEADER_SIZE+5]);

	// Add if doesn't exist
	if(m_buf.find(seqnum) == NULL)
	{
		IncomingSplitPacket *sp = new IncomingSplitPacket();
		sp->chunk_count = chunk_count;
		sp->reliable = reliable;
		m_buf[seqnum] = sp;
	}
	
	IncomingSplitPacket *sp = m_buf[seqnum];
	
	// TODO: These errors should be thrown or something? Dunno.
	if(chunk_count != sp->chunk_count)
		derr_con<<"Connection: WARNING: chunk_count="<<chunk_count
				<<" != sp->chunk_count="<<sp->chunk_count
				<<std::endl;
	if(reliable != sp->reliable)
		derr_con<<"Connection: WARNING: reliable="<<reliable
				<<" != sp->reliable="<<sp->reliable
				<<std::endl;

	// If chunk already exists, cancel
	if(sp->chunks.find(chunk_num) != NULL)
		throw AlreadyExistsException("Chunk already in buffer");
	
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
	core::map<u16, SharedBuffer<u8> >::Iterator i;
	i = sp->chunks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		totalsize += i.getNode()->getValue().getSize();
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
	m_buf.remove(seqnum);
	delete sp;

	return fulldata;
}
void IncomingSplitBuffer::removeUnreliableTimedOuts(float dtime, float timeout)
{
	core::list<u16> remove_queue;
	core::map<u16, IncomingSplitPacket*>::Iterator i;
	i = m_buf.getIterator();
	for(; i.atEnd() == false; i++)
	{
		IncomingSplitPacket *p = i.getNode()->getValue();
		// Reliable ones are not removed by timeout
		if(p->reliable == true)
			continue;
		p->time += dtime;
		if(p->time >= timeout)
			remove_queue.push_back(i.getNode()->getKey());
	}
	core::list<u16>::Iterator j;
	j = remove_queue.begin();
	for(; j != remove_queue.end(); j++)
	{
		dout_con<<"NOTE: Removing timed out unreliable split packet"
				<<std::endl;
		delete m_buf[*j];
		m_buf.remove(*j);
	}
}

/*
	Channel
*/

Channel::Channel()
{
	next_outgoing_seqnum = SEQNUM_INITIAL;
	next_incoming_seqnum = SEQNUM_INITIAL;
	next_outgoing_split_seqnum = SEQNUM_INITIAL;
}
Channel::~Channel()
{
}

/*
	Peer
*/

Peer::Peer(u16 a_id, Address a_address):
	address(a_address),
	id(a_id),
	timeout_counter(0.0),
	ping_timer(0.0),
	resend_timeout(0.5),
	avg_rtt(-1.0),
	has_sent_with_id(false),
	m_sendtime_accu(0),
	m_max_packets_per_second(10),
	m_num_sent(0),
	m_max_num_sent(0)
{
}
Peer::~Peer()
{
}

void Peer::reportRTT(float rtt)
{
	if(rtt >= 0.0){
		if(rtt < 0.01){
			if(m_max_packets_per_second < 100)
				m_max_packets_per_second += 10;
		} else if(rtt < 0.2){
			if(m_max_packets_per_second < 100)
				m_max_packets_per_second += 2;
		} else {
			m_max_packets_per_second *= 0.8;
			if(m_max_packets_per_second < 10)
				m_max_packets_per_second = 10;
		}
	}

	if(rtt < -0.999)
	{}
	else if(avg_rtt < 0.0)
		avg_rtt = rtt;
	else
		avg_rtt = rtt * 0.1 + avg_rtt * 0.9;
	
	// Calculate resend_timeout

	/*int reliable_count = 0;
	for(int i=0; i<CHANNEL_COUNT; i++)
	{
		reliable_count += channels[i].outgoing_reliables.size();
	}
	float timeout = avg_rtt * RESEND_TIMEOUT_FACTOR
			* ((float)reliable_count * 1);*/
	
	float timeout = avg_rtt * RESEND_TIMEOUT_FACTOR;
	if(timeout < RESEND_TIMEOUT_MIN)
		timeout = RESEND_TIMEOUT_MIN;
	if(timeout > RESEND_TIMEOUT_MAX)
		timeout = RESEND_TIMEOUT_MAX;
	resend_timeout = timeout;
}
				
/*
	Connection
*/

Connection::Connection(u32 protocol_id, u32 max_packet_size, float timeout):
	m_protocol_id(protocol_id),
	m_max_packet_size(max_packet_size),
	m_timeout(timeout),
	m_peer_id(0),
	m_bc_peerhandler(NULL),
	m_bc_receive_timeout(0),
	m_indentation(0)
{
	m_socket.setTimeoutMs(5);

	Start();
}

Connection::Connection(u32 protocol_id, u32 max_packet_size, float timeout,
		PeerHandler *peerhandler):
	m_protocol_id(protocol_id),
	m_max_packet_size(max_packet_size),
	m_timeout(timeout),
	m_peer_id(0),
	m_bc_peerhandler(peerhandler),
	m_bc_receive_timeout(0),
	m_indentation(0)
{
	m_socket.setTimeoutMs(5);

	Start();
}


Connection::~Connection()
{
	stop();
}

/* Internal stuff */

void * Connection::Thread()
{
	ThreadStarted();
	log_register_thread("Connection");

	dout_con<<"Connection thread started"<<std::endl;
	
	u32 curtime = porting::getTimeMs();
	u32 lasttime = curtime;

	while(getRun())
	{
		BEGIN_DEBUG_EXCEPTION_HANDLER
		
		lasttime = curtime;
		curtime = porting::getTimeMs();
		float dtime = (float)(curtime - lasttime) / 1000.;
		if(dtime > 0.1)
			dtime = 0.1;
		if(dtime < 0.0)
			dtime = 0.0;
		
		runTimeouts(dtime);

		while(m_command_queue.size() != 0){
			ConnectionCommand c = m_command_queue.pop_front();
			processCommand(c);
		}

		send(dtime);

		receive();
		
		END_DEBUG_EXCEPTION_HANDLER(derr_con);
	}

	return NULL;
}

void Connection::putEvent(ConnectionEvent &e)
{
	assert(e.type != CONNEVENT_NONE);
	m_event_queue.push_back(e);
}

void Connection::processCommand(ConnectionCommand &c)
{
	switch(c.type){
	case CONNCMD_NONE:
		dout_con<<getDesc()<<" processing CONNCMD_NONE"<<std::endl;
		return;
	case CONNCMD_SERVE:
		dout_con<<getDesc()<<" processing CONNCMD_SERVE port="
				<<c.port<<std::endl;
		serve(c.port);
		return;
	case CONNCMD_CONNECT:
		dout_con<<getDesc()<<" processing CONNCMD_CONNECT"<<std::endl;
		connect(c.address);
		return;
	case CONNCMD_DISCONNECT:
		dout_con<<getDesc()<<" processing CONNCMD_DISCONNECT"<<std::endl;
		disconnect();
		return;
	case CONNCMD_SEND:
		dout_con<<getDesc()<<" processing CONNCMD_SEND"<<std::endl;
		send(c.peer_id, c.channelnum, c.data, c.reliable);
		return;
	case CONNCMD_SEND_TO_ALL:
		dout_con<<getDesc()<<" processing CONNCMD_SEND_TO_ALL"<<std::endl;
		sendToAll(c.channelnum, c.data, c.reliable);
		return;
	case CONNCMD_DELETE_PEER:
		dout_con<<getDesc()<<" processing CONNCMD_DELETE_PEER"<<std::endl;
		deletePeer(c.peer_id, false);
		return;
	}
}

void Connection::send(float dtime)
{
	for(core::map<u16, Peer*>::Iterator
			j = m_peers.getIterator();
			j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		peer->m_sendtime_accu += dtime;
		peer->m_num_sent = 0;
		peer->m_max_num_sent = peer->m_sendtime_accu *
				peer->m_max_packets_per_second;
	}
	Queue<OutgoingPacket> postponed_packets;
	while(m_outgoing_queue.size() != 0){
		OutgoingPacket packet = m_outgoing_queue.pop_front();
		Peer *peer = getPeerNoEx(packet.peer_id);
		if(!peer)
			continue;
		if(peer->channels[packet.channelnum].outgoing_reliables.size() >= 5){
			postponed_packets.push_back(packet);
		} else if(peer->m_num_sent < peer->m_max_num_sent){
			rawSendAsPacket(packet.peer_id, packet.channelnum,
					packet.data, packet.reliable);
			peer->m_num_sent++;
		} else {
			postponed_packets.push_back(packet);
		}
	}
	while(postponed_packets.size() != 0){
		m_outgoing_queue.push_back(postponed_packets.pop_front());
	}
	for(core::map<u16, Peer*>::Iterator
			j = m_peers.getIterator();
			j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		peer->m_sendtime_accu -= (float)peer->m_num_sent /
				peer->m_max_packets_per_second;
		if(peer->m_sendtime_accu > 10. / peer->m_max_packets_per_second)
			peer->m_sendtime_accu = 10. / peer->m_max_packets_per_second;
	}
}

// Receive packets from the network and buffers and create ConnectionEvents
void Connection::receive()
{
	u32 datasize = m_max_packet_size * 2;  // Double it just to be safe
	// TODO: We can not know how many layers of header there are.
	// For now, just assume there are no other than the base headers.
	u32 packet_maxsize = datasize + BASE_HEADER_SIZE;
	SharedBuffer<u8> packetdata(packet_maxsize);

	bool single_wait_done = false;
	
	for(;;)
	{
	try{
		/* Check if some buffer has relevant data */
		{
			u16 peer_id;
			SharedBuffer<u8> resultdata;
			bool got = getFromBuffers(peer_id, resultdata);
			if(got){
				ConnectionEvent e;
				e.dataReceived(peer_id, resultdata);
				putEvent(e);
				continue;
			}
		}
		
		if(single_wait_done){
			if(m_socket.WaitData(0) == false)
				break;
		}
		
		single_wait_done = true;

		Address sender;
		s32 received_size = m_socket.Receive(sender, *packetdata, packet_maxsize);

		if(received_size < 0)
			break;
		if(received_size < BASE_HEADER_SIZE)
			continue;
		if(readU32(&packetdata[0]) != m_protocol_id)
			continue;
		
		u16 peer_id = readPeerId(*packetdata);
		u8 channelnum = readChannel(*packetdata);
		if(channelnum > CHANNEL_COUNT-1){
			PrintInfo(derr_con);
			derr_con<<"Receive(): Invalid channel "<<channelnum<<std::endl;
			throw InvalidIncomingDataException("Channel doesn't exist");
		}

		if(peer_id == PEER_ID_INEXISTENT)
		{
			/*
				Somebody is trying to send stuff to us with no peer id.
				
				Check if the same address and port was added to our peer
				list before.
				Allow only entries that have has_sent_with_id==false.
			*/

			core::map<u16, Peer*>::Iterator j;
			j = m_peers.getIterator();
			for(; j.atEnd() == false; j++)
			{
				Peer *peer = j.getNode()->getValue();
				if(peer->has_sent_with_id)
					continue;
				if(peer->address == sender)
					break;
			}
			
			/*
				If no peer was found with the same address and port,
				we shall assume it is a new peer and create an entry.
			*/
			if(j.atEnd())
			{
				// Pass on to adding the peer
			}
			// Else: A peer was found.
			else
			{
				Peer *peer = j.getNode()->getValue();
				peer_id = peer->id;
				PrintInfo(derr_con);
				derr_con<<"WARNING: Assuming unknown peer to be "
						<<"peer_id="<<peer_id<<std::endl;
			}
		}
		
		/*
			The peer was not found in our lists. Add it.
		*/
		if(peer_id == PEER_ID_INEXISTENT)
		{
			// Somebody wants to make a new connection

			// Get a unique peer id (2 or higher)
			u16 peer_id_new = 2;
			/*
				Find an unused peer id
			*/
			bool out_of_ids = false;
			for(;;)
			{
				// Check if exists
				if(m_peers.find(peer_id_new) == NULL)
					break;
				// Check for overflow
				if(peer_id_new == 65535){
					out_of_ids = true;
					break;
				}
				peer_id_new++;
			}
			if(out_of_ids){
				errorstream<<getDesc()<<" ran out of peer ids"<<std::endl;
				continue;
			}

			PrintInfo();
			dout_con<<"Receive(): Got a packet with peer_id=PEER_ID_INEXISTENT,"
					" giving peer_id="<<peer_id_new<<std::endl;

			// Create a peer
			Peer *peer = new Peer(peer_id_new, sender);
			m_peers.insert(peer->id, peer);
			
			// Create peer addition event
			ConnectionEvent e;
			e.peerAdded(peer_id_new, sender);
			putEvent(e);
			
			// Create CONTROL packet to tell the peer id to the new peer.
			SharedBuffer<u8> reply(4);
			writeU8(&reply[0], TYPE_CONTROL);
			writeU8(&reply[1], CONTROLTYPE_SET_PEER_ID);
			writeU16(&reply[2], peer_id_new);
			sendAsPacket(peer_id_new, 0, reply, true);
			
			// We're now talking to a valid peer_id
			peer_id = peer_id_new;

			// Go on and process whatever it sent
		}

		core::map<u16, Peer*>::Node *node = m_peers.find(peer_id);

		if(node == NULL)
		{
			// Peer not found
			// This means that the peer id of the sender is not PEER_ID_INEXISTENT
			// and it is invalid.
			PrintInfo(derr_con);
			derr_con<<"Receive(): Peer not found"<<std::endl;
			throw InvalidIncomingDataException("Peer not found (possible timeout)");
		}

		Peer *peer = node->getValue();

		// Validate peer address
		if(peer->address != sender)
		{
			PrintInfo(derr_con);
			derr_con<<"Peer "<<peer_id<<" sending from different address."
					" Ignoring."<<std::endl;
			continue;
		}
		
		peer->timeout_counter = 0.0;

		Channel *channel = &(peer->channels[channelnum]);
		
		// Throw the received packet to channel->processPacket()

		// Make a new SharedBuffer from the data without the base headers
		SharedBuffer<u8> strippeddata(received_size - BASE_HEADER_SIZE);
		memcpy(*strippeddata, &packetdata[BASE_HEADER_SIZE],
				strippeddata.getSize());
		
		try{
			// Process it (the result is some data with no headers made by us)
			SharedBuffer<u8> resultdata = processPacket
					(channel, strippeddata, peer_id, channelnum, false);
			
			PrintInfo();
			dout_con<<"ProcessPacket returned data of size "
					<<resultdata.getSize()<<std::endl;
			
			ConnectionEvent e;
			e.dataReceived(peer_id, resultdata);
			putEvent(e);
			continue;
		}catch(ProcessedSilentlyException &e){
		}
	}catch(InvalidIncomingDataException &e){
	}
	catch(ProcessedSilentlyException &e){
	}
	} // for
}

void Connection::runTimeouts(float dtime)
{
	core::list<u16> timeouted_peers;
	core::map<u16, Peer*>::Iterator j;
	j = m_peers.getIterator();
	for(; j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		
		/*
			Check peer timeout
		*/
		peer->timeout_counter += dtime;
		if(peer->timeout_counter > m_timeout)
		{
			PrintInfo(derr_con);
			derr_con<<"RunTimeouts(): Peer "<<peer->id
					<<" has timed out."
					<<" (source=peer->timeout_counter)"
					<<std::endl;
			// Add peer to the list
			timeouted_peers.push_back(peer->id);
			// Don't bother going through the buffers of this one
			continue;
		}

		float resend_timeout = peer->resend_timeout;
		for(u16 i=0; i<CHANNEL_COUNT; i++)
		{
			core::list<BufferedPacket> timed_outs;
			core::list<BufferedPacket>::Iterator j;
			
			Channel *channel = &peer->channels[i];

			// Remove timed out incomplete unreliable split packets
			channel->incoming_splits.removeUnreliableTimedOuts(dtime, m_timeout);
			
			// Increment reliable packet times
			channel->outgoing_reliables.incrementTimeouts(dtime);

			// Check reliable packet total times, remove peer if
			// over timeout.
			if(channel->outgoing_reliables.anyTotaltimeReached(m_timeout))
			{
				PrintInfo(derr_con);
				derr_con<<"RunTimeouts(): Peer "<<peer->id
						<<" has timed out."
						<<" (source=reliable packet totaltime)"
						<<std::endl;
				// Add peer to the to-be-removed list
				timeouted_peers.push_back(peer->id);
				goto nextpeer;
			}

			// Re-send timed out outgoing reliables
			
			timed_outs = channel->
					outgoing_reliables.getTimedOuts(resend_timeout);

			channel->outgoing_reliables.resetTimedOuts(resend_timeout);

			j = timed_outs.begin();
			for(; j != timed_outs.end(); j++)
			{
				u16 peer_id = readPeerId(*(j->data));
				u8 channel = readChannel(*(j->data));
				u16 seqnum = readU16(&(j->data[BASE_HEADER_SIZE+1]));

				PrintInfo(derr_con);
				derr_con<<"RE-SENDING timed-out RELIABLE to ";
				j->address.print(&derr_con);
				derr_con<<"(t/o="<<resend_timeout<<"): "
						<<"from_peer_id="<<peer_id
						<<", channel="<<((int)channel&0xff)
						<<", seqnum="<<seqnum
						<<std::endl;

				rawSend(*j);

				// Enlarge avg_rtt and resend_timeout:
				// The rtt will be at least the timeout.
				// NOTE: This won't affect the timeout of the next
				// checked channel because it was cached.
				peer->reportRTT(resend_timeout);
			}
		}
		
		/*
			Send pings
		*/
		peer->ping_timer += dtime;
		if(peer->ping_timer >= 5.0)
		{
			// Create and send PING packet
			SharedBuffer<u8> data(2);
			writeU8(&data[0], TYPE_CONTROL);
			writeU8(&data[1], CONTROLTYPE_PING);
			rawSendAsPacket(peer->id, 0, data, true);

			peer->ping_timer = 0.0;
		}
		
nextpeer:
		continue;
	}

	// Remove timed out peers
	core::list<u16>::Iterator i = timeouted_peers.begin();
	for(; i != timeouted_peers.end(); i++)
	{
		PrintInfo(derr_con);
		derr_con<<"RunTimeouts(): Removing peer "<<(*i)<<std::endl;
		deletePeer(*i, true);
	}
}

void Connection::serve(u16 port)
{
	dout_con<<getDesc()<<" serving at port "<<port<<std::endl;
	m_socket.Bind(port);
	m_peer_id = PEER_ID_SERVER;
}

void Connection::connect(Address address)
{
	dout_con<<getDesc()<<" connecting to "<<address.serializeString()
			<<":"<<address.getPort()<<std::endl;

	core::map<u16, Peer*>::Node *node = m_peers.find(PEER_ID_SERVER);
	if(node != NULL){
		throw ConnectionException("Already connected to a server");
	}

	Peer *peer = new Peer(PEER_ID_SERVER, address);
	m_peers.insert(peer->id, peer);

	// Create event
	ConnectionEvent e;
	e.peerAdded(peer->id, peer->address);
	putEvent(e);
	
	m_socket.Bind(0);
	
	// Send a dummy packet to server with peer_id = PEER_ID_INEXISTENT
	m_peer_id = PEER_ID_INEXISTENT;
	SharedBuffer<u8> data(0);
	Send(PEER_ID_SERVER, 0, data, true);
}

void Connection::disconnect()
{
	dout_con<<getDesc()<<" disconnecting"<<std::endl;

	// Create and send DISCO packet
	SharedBuffer<u8> data(2);
	writeU8(&data[0], TYPE_CONTROL);
	writeU8(&data[1], CONTROLTYPE_DISCO);
	
	// Send to all
	core::map<u16, Peer*>::Iterator j;
	j = m_peers.getIterator();
	for(; j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		rawSendAsPacket(peer->id, 0, data, false);
	}
}

void Connection::sendToAll(u8 channelnum, SharedBuffer<u8> data, bool reliable)
{
	core::map<u16, Peer*>::Iterator j;
	j = m_peers.getIterator();
	for(; j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		send(peer->id, channelnum, data, reliable);
	}
}

void Connection::send(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data, bool reliable)
{
	dout_con<<getDesc()<<" sending to peer_id="<<peer_id<<std::endl;

	assert(channelnum < CHANNEL_COUNT);
	
	Peer *peer = getPeerNoEx(peer_id);
	if(peer == NULL)
		return;
	Channel *channel = &(peer->channels[channelnum]);

	u32 chunksize_max = m_max_packet_size - BASE_HEADER_SIZE;
	if(reliable)
		chunksize_max -= RELIABLE_HEADER_SIZE;

	core::list<SharedBuffer<u8> > originals;
	originals = makeAutoSplitPacket(data, chunksize_max,
			channel->next_outgoing_split_seqnum);
	
	core::list<SharedBuffer<u8> >::Iterator i;
	i = originals.begin();
	for(; i != originals.end(); i++)
	{
		SharedBuffer<u8> original = *i;
		
		sendAsPacket(peer_id, channelnum, original, reliable);
	}
}

void Connection::sendAsPacket(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data, bool reliable)
{
	OutgoingPacket packet(peer_id, channelnum, data, reliable);
	m_outgoing_queue.push_back(packet);
}

void Connection::rawSendAsPacket(u16 peer_id, u8 channelnum,
		SharedBuffer<u8> data, bool reliable)
{
	Peer *peer = getPeerNoEx(peer_id);
	if(!peer)
		return;
	Channel *channel = &(peer->channels[channelnum]);

	if(reliable)
	{
		u16 seqnum = channel->next_outgoing_seqnum;
		channel->next_outgoing_seqnum++;

		SharedBuffer<u8> reliable = makeReliablePacket(data, seqnum);

		// Add base headers and make a packet
		BufferedPacket p = makePacket(peer->address, reliable,
				m_protocol_id, m_peer_id, channelnum);
		
		try{
			// Buffer the packet
			channel->outgoing_reliables.insert(p);
		}
		catch(AlreadyExistsException &e)
		{
			PrintInfo(derr_con);
			derr_con<<"WARNING: Going to send a reliable packet "
					"seqnum="<<seqnum<<" that is already "
					"in outgoing buffer"<<std::endl;
			//assert(0);
		}
		
		// Send the packet
		rawSend(p);
	}
	else
	{
		// Add base headers and make a packet
		BufferedPacket p = makePacket(peer->address, data,
				m_protocol_id, m_peer_id, channelnum);

		// Send the packet
		rawSend(p);
	}
}

void Connection::rawSend(const BufferedPacket &packet)
{
	try{
		m_socket.Send(packet.address, *packet.data, packet.data.getSize());
	} catch(SendFailedException &e){
		derr_con<<"Connection::rawSend(): SendFailedException: "
				<<packet.address.serializeString()<<std::endl;
	}
}

Peer* Connection::getPeer(u16 peer_id)
{
	core::map<u16, Peer*>::Node *node = m_peers.find(peer_id);

	if(node == NULL){
		throw PeerNotFoundException("GetPeer: Peer not found (possible timeout)");
	}

	// Error checking
	assert(node->getValue()->id == peer_id);

	return node->getValue();
}

Peer* Connection::getPeerNoEx(u16 peer_id)
{
	core::map<u16, Peer*>::Node *node = m_peers.find(peer_id);

	if(node == NULL){
		return NULL;
	}

	// Error checking
	assert(node->getValue()->id == peer_id);

	return node->getValue();
}

core::list<Peer*> Connection::getPeers()
{
	core::list<Peer*> list;
	core::map<u16, Peer*>::Iterator j;
	j = m_peers.getIterator();
	for(; j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		list.push_back(peer);
	}
	return list;
}

bool Connection::getFromBuffers(u16 &peer_id, SharedBuffer<u8> &dst)
{
	core::map<u16, Peer*>::Iterator j;
	j = m_peers.getIterator();
	for(; j.atEnd() == false; j++)
	{
		Peer *peer = j.getNode()->getValue();
		for(u16 i=0; i<CHANNEL_COUNT; i++)
		{
			Channel *channel = &peer->channels[i];
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

bool Connection::checkIncomingBuffers(Channel *channel, u16 &peer_id,
		SharedBuffer<u8> &dst)
{
	u16 firstseqnum = 0;
	// Clear old packets from start of buffer
	try{
	for(;;){
		firstseqnum = channel->incoming_reliables.getFirstSeqnum();
		if(seqnum_higher(channel->next_incoming_seqnum, firstseqnum))
			channel->incoming_reliables.popFirst();
		else
			break;
	}
	// This happens if all packets are old
	}catch(con::NotFoundException)
	{}
	
	if(channel->incoming_reliables.empty() == false)
	{
		if(firstseqnum == channel->next_incoming_seqnum)
		{
			BufferedPacket p = channel->incoming_reliables.popFirst();
			
			peer_id = readPeerId(*p.data);
			u8 channelnum = readChannel(*p.data);
			u16 seqnum = readU16(&p.data[BASE_HEADER_SIZE+1]);

			PrintInfo();
			dout_con<<"UNBUFFERING TYPE_RELIABLE"
					<<" seqnum="<<seqnum
					<<" peer_id="<<peer_id
					<<" channel="<<((int)channelnum&0xff)
					<<std::endl;

			channel->next_incoming_seqnum++;
			
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

SharedBuffer<u8> Connection::processPacket(Channel *channel,
		SharedBuffer<u8> packetdata, u16 peer_id,
		u8 channelnum, bool reliable)
{
	IndentationRaiser iraiser(&(m_indentation));

	if(packetdata.getSize() < 1)
		throw InvalidIncomingDataException("packetdata.getSize() < 1");

	u8 type = readU8(&packetdata[0]);
	
	if(type == TYPE_CONTROL)
	{
		if(packetdata.getSize() < 2)
			throw InvalidIncomingDataException("packetdata.getSize() < 2");

		u8 controltype = readU8(&packetdata[1]);

		if(controltype == CONTROLTYPE_ACK)
		{
			if(packetdata.getSize() < 4)
				throw InvalidIncomingDataException
						("packetdata.getSize() < 4 (ACK header size)");

			u16 seqnum = readU16(&packetdata[2]);
			PrintInfo();
			dout_con<<"Got CONTROLTYPE_ACK: channelnum="
					<<((int)channelnum&0xff)<<", peer_id="<<peer_id
					<<", seqnum="<<seqnum<<std::endl;

			try{
				BufferedPacket p = channel->outgoing_reliables.popSeqnum(seqnum);
				// Get round trip time
				float rtt = p.totaltime;

				// Let peer calculate stuff according to it
				// (avg_rtt and resend_timeout)
				Peer *peer = getPeer(peer_id);
				peer->reportRTT(rtt);

				//PrintInfo(dout_con);
				//dout_con<<"RTT = "<<rtt<<std::endl;

				/*dout_con<<"OUTGOING: ";
				PrintInfo();
				channel->outgoing_reliables.print();
				dout_con<<std::endl;*/
			}
			catch(NotFoundException &e){
				PrintInfo(derr_con);
				derr_con<<"WARNING: ACKed packet not "
						"in outgoing queue"
						<<std::endl;
			}

			throw ProcessedSilentlyException("Got an ACK");
		}
		else if(controltype == CONTROLTYPE_SET_PEER_ID)
		{
			if(packetdata.getSize() < 4)
				throw InvalidIncomingDataException
						("packetdata.getSize() < 4 (SET_PEER_ID header size)");
			u16 peer_id_new = readU16(&packetdata[2]);
			PrintInfo();
			dout_con<<"Got new peer id: "<<peer_id_new<<"... "<<std::endl;

			if(GetPeerID() != PEER_ID_INEXISTENT)
			{
				PrintInfo(derr_con);
				derr_con<<"WARNING: Not changing"
						" existing peer id."<<std::endl;
			}
			else
			{
				dout_con<<"changing."<<std::endl;
				SetPeerID(peer_id_new);
			}
			throw ProcessedSilentlyException("Got a SET_PEER_ID");
		}
		else if(controltype == CONTROLTYPE_PING)
		{
			// Just ignore it, the incoming data already reset
			// the timeout counter
			PrintInfo();
			dout_con<<"PING"<<std::endl;
			throw ProcessedSilentlyException("Got a PING");
		}
		else if(controltype == CONTROLTYPE_DISCO)
		{
			// Just ignore it, the incoming data already reset
			// the timeout counter
			PrintInfo();
			dout_con<<"DISCO: Removing peer "<<(peer_id)<<std::endl;
			
			if(deletePeer(peer_id, false) == false)
			{
				PrintInfo(derr_con);
				derr_con<<"DISCO: Peer not found"<<std::endl;
			}

			throw ProcessedSilentlyException("Got a DISCO");
		}
		else{
			PrintInfo(derr_con);
			derr_con<<"INVALID TYPE_CONTROL: invalid controltype="
					<<((int)controltype&0xff)<<std::endl;
			throw InvalidIncomingDataException("Invalid control type");
		}
	}
	else if(type == TYPE_ORIGINAL)
	{
		if(packetdata.getSize() < ORIGINAL_HEADER_SIZE)
			throw InvalidIncomingDataException
					("packetdata.getSize() < ORIGINAL_HEADER_SIZE");
		PrintInfo();
		dout_con<<"RETURNING TYPE_ORIGINAL to user"
				<<std::endl;
		// Get the inside packet out and return it
		SharedBuffer<u8> payload(packetdata.getSize() - ORIGINAL_HEADER_SIZE);
		memcpy(*payload, &packetdata[ORIGINAL_HEADER_SIZE], payload.getSize());
		return payload;
	}
	else if(type == TYPE_SPLIT)
	{
		// We have to create a packet again for buffering
		// This isn't actually too bad an idea.
		BufferedPacket packet = makePacket(
				getPeer(peer_id)->address,
				packetdata,
				GetProtocolID(),
				peer_id,
				channelnum);
		// Buffer the packet
		SharedBuffer<u8> data = channel->incoming_splits.insert(packet, reliable);
		if(data.getSize() != 0)
		{
			PrintInfo();
			dout_con<<"RETURNING TYPE_SPLIT: Constructed full data, "
					<<"size="<<data.getSize()<<std::endl;
			return data;
		}
		PrintInfo();
		dout_con<<"BUFFERED TYPE_SPLIT"<<std::endl;
		throw ProcessedSilentlyException("Buffered a split packet chunk");
	}
	else if(type == TYPE_RELIABLE)
	{
		// Recursive reliable packets not allowed
		assert(reliable == false);

		if(packetdata.getSize() < RELIABLE_HEADER_SIZE)
			throw InvalidIncomingDataException
					("packetdata.getSize() < RELIABLE_HEADER_SIZE");

		u16 seqnum = readU16(&packetdata[1]);

		bool is_future_packet = seqnum_higher(seqnum, channel->next_incoming_seqnum);
		bool is_old_packet = seqnum_higher(channel->next_incoming_seqnum, seqnum);
		
		PrintInfo();
		if(is_future_packet)
			dout_con<<"BUFFERING";
		else if(is_old_packet)
			dout_con<<"OLD";
		else
			dout_con<<"RECUR";
		dout_con<<" TYPE_RELIABLE seqnum="<<seqnum
				<<" next="<<channel->next_incoming_seqnum;
		dout_con<<" [sending CONTROLTYPE_ACK"
				" to peer_id="<<peer_id<<"]";
		dout_con<<std::endl;
		
		//DEBUG
		//assert(channel->incoming_reliables.size() < 100);

		// Send a CONTROLTYPE_ACK
		SharedBuffer<u8> reply(4);
		writeU8(&reply[0], TYPE_CONTROL);
		writeU8(&reply[1], CONTROLTYPE_ACK);
		writeU16(&reply[2], seqnum);
		rawSendAsPacket(peer_id, channelnum, reply, false);

		//if(seqnum_higher(seqnum, channel->next_incoming_seqnum))
		if(is_future_packet)
		{
			/*PrintInfo();
			dout_con<<"Buffering reliable packet (seqnum="
					<<seqnum<<")"<<std::endl;*/
			
			// This one comes later, buffer it.
			// Actually we have to make a packet to buffer one.
			// Well, we have all the ingredients, so just do it.
			BufferedPacket packet = makePacket(
					getPeer(peer_id)->address,
					packetdata,
					GetProtocolID(),
					peer_id,
					channelnum);
			try{
				channel->incoming_reliables.insert(packet);
				
				/*PrintInfo();
				dout_con<<"INCOMING: ";
				channel->incoming_reliables.print();
				dout_con<<std::endl;*/
			}
			catch(AlreadyExistsException &e)
			{
			}

			throw ProcessedSilentlyException("Buffered future reliable packet");
		}
		//else if(seqnum_higher(channel->next_incoming_seqnum, seqnum))
		else if(is_old_packet)
		{
			// An old packet, dump it
			throw InvalidIncomingDataException("Got an old reliable packet");
		}

		channel->next_incoming_seqnum++;

		// Get out the inside packet and re-process it
		SharedBuffer<u8> payload(packetdata.getSize() - RELIABLE_HEADER_SIZE);
		memcpy(*payload, &packetdata[RELIABLE_HEADER_SIZE], payload.getSize());

		return processPacket(channel, payload, peer_id, channelnum, true);
	}
	else
	{
		PrintInfo(derr_con);
		derr_con<<"Got invalid type="<<((int)type&0xff)<<std::endl;
		throw InvalidIncomingDataException("Invalid packet type");
	}
	
	// We should never get here.
	// If you get here, add an exception or a return to some of the
	// above conditionals.
	assert(0);
	throw BaseException("Error in Channel::ProcessPacket()");
}

bool Connection::deletePeer(u16 peer_id, bool timeout)
{
	if(m_peers.find(peer_id) == NULL)
		return false;
	
	Peer *peer = m_peers[peer_id];

	// Create event
	ConnectionEvent e;
	e.peerRemoved(peer_id, timeout, peer->address);
	putEvent(e);

	delete m_peers[peer_id];
	m_peers.remove(peer_id);
	return true;
}

/* Interface */

ConnectionEvent Connection::getEvent()
{
	if(m_event_queue.size() == 0){
		ConnectionEvent e;
		e.type = CONNEVENT_NONE;
		return e;
	}
	return m_event_queue.pop_front();
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
	m_command_queue.push_back(c);
}

void Connection::Serve(unsigned short port)
{
	ConnectionCommand c;
	c.serve(port);
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
		
	core::map<u16, Peer*>::Node *node = m_peers.find(PEER_ID_SERVER);
	if(node == NULL)
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
			dout_con<<getDesc()<<": Receive: got event: "
					<<e.describe()<<std::endl;
		switch(e.type){
		case CONNEVENT_NONE:
			throw NoIncomingDataException("No incoming data");
		case CONNEVENT_DATA_RECEIVED:
			peer_id = e.peer_id;
			data = SharedBuffer<u8>(e.data);
			return e.data.getSize();
		case CONNEVENT_PEER_ADDED: {
			Peer tmp(e.peer_id, e.address);
			if(m_bc_peerhandler)
				m_bc_peerhandler->peerAdded(&tmp);
			continue; }
		case CONNEVENT_PEER_REMOVED: {
			Peer tmp(e.peer_id, e.address);
			if(m_bc_peerhandler)
				m_bc_peerhandler->deletingPeer(&tmp, e.timeout);
			continue; }
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

void Connection::RunTimeouts(float dtime)
{
	// No-op
}

Address Connection::GetPeerAddress(u16 peer_id)
{
	JMutexAutoLock peerlock(m_peers_mutex);
	return getPeer(peer_id)->address;
}

float Connection::GetPeerAvgRTT(u16 peer_id)
{
	JMutexAutoLock peerlock(m_peers_mutex);
	return getPeer(peer_id)->avg_rtt;
}

void Connection::DeletePeer(u16 peer_id)
{
	ConnectionCommand c;
	c.deletePeer(peer_id);
	putCommand(c);
}

void Connection::PrintInfo(std::ostream &out)
{
	out<<getDesc()<<": ";
}

void Connection::PrintInfo()
{
	PrintInfo(dout_con);
}

std::string Connection::getDesc()
{
	return std::string("con(")+itos(m_socket.GetHandle())+"/"+itos(m_peer_id)+")";
}

} // namespace

