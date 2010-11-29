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

#ifndef CONNECTION_HEADER
#define CONNECTION_HEADER

#include <iostream>
#include <fstream>
#include "debug.h"
#include "common_irrlicht.h"
#include "socket.h"
#include "utility.h"
#include "exceptions.h"
#include "constants.h"

namespace con
{

/*
	Exceptions
*/
class NotFoundException : public BaseException
{
public:
	NotFoundException(const char *s):
		BaseException(s)
	{}
};

class PeerNotFoundException : public BaseException
{
public:
	PeerNotFoundException(const char *s):
		BaseException(s)
	{}
};

class ConnectionException : public BaseException
{
public:
	ConnectionException(const char *s):
		BaseException(s)
	{}
};

/*class ThrottlingException : public BaseException
{
public:
	ThrottlingException(const char *s):
		BaseException(s)
	{}
};*/

class InvalidIncomingDataException : public BaseException
{
public:
	InvalidIncomingDataException(const char *s):
		BaseException(s)
	{}
};

class InvalidOutgoingDataException : public BaseException
{
public:
	InvalidOutgoingDataException(const char *s):
		BaseException(s)
	{}
};

class NoIncomingDataException : public BaseException
{
public:
	NoIncomingDataException(const char *s):
		BaseException(s)
	{}
};

class ProcessedSilentlyException : public BaseException
{
public:
	ProcessedSilentlyException(const char *s):
		BaseException(s)
	{}
};

class GotSplitPacketException
{
	SharedBuffer<u8> m_data;
public:
	GotSplitPacketException(SharedBuffer<u8> data):
		m_data(data)
	{}
	SharedBuffer<u8> getData()
	{
		return m_data;
	}
};

inline u16 readPeerId(u8 *packetdata)
{
	return readU16(&packetdata[4]);
}
inline u8 readChannel(u8 *packetdata)
{
	return readU8(&packetdata[6]);
}

#define SEQNUM_MAX 65535
inline bool seqnum_higher(u16 higher, u16 lower)
{
	if(lower > higher && lower - higher > SEQNUM_MAX/2){
		return true;
	}
	return (higher > lower);
}

struct BufferedPacket
{
	BufferedPacket(u8 *a_data, u32 a_size):
		data(a_data, a_size), time(0.0), totaltime(0.0)
	{}
	BufferedPacket(u32 a_size):
		data(a_size), time(0.0), totaltime(0.0)
	{}
	SharedBuffer<u8> data; // Data of the packet, including headers
	float time; // Seconds from buffering the packet or re-sending
	float totaltime; // Seconds from buffering the packet
	Address address; // Sender or destination
};

// This adds the base headers to the data and makes a packet out of it
BufferedPacket makePacket(Address &address, u8 *data, u32 datasize,
		u32 protocol_id, u16 sender_peer_id, u8 channel);
BufferedPacket makePacket(Address &address, SharedBuffer<u8> &data,
		u32 protocol_id, u16 sender_peer_id, u8 channel);

// Add the TYPE_ORIGINAL header to the data
SharedBuffer<u8> makeOriginalPacket(
		SharedBuffer<u8> data);

// Split data in chunks and add TYPE_SPLIT headers to them
core::list<SharedBuffer<u8> > makeSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 seqnum);

// Depending on size, make a TYPE_ORIGINAL or TYPE_SPLIT packet
// Increments split_seqnum if a split packet is made
core::list<SharedBuffer<u8> > makeAutoSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 &split_seqnum);

// Add the TYPE_RELIABLE header to the data
SharedBuffer<u8> makeReliablePacket(
		SharedBuffer<u8> data,
		u16 seqnum);

struct IncomingSplitPacket
{
	IncomingSplitPacket()
	{
		time = 0.0;
		reliable = false;
	}
	// Key is chunk number, value is data without headers
	core::map<u16, SharedBuffer<u8> > chunks;
	u32 chunk_count;
	float time; // Seconds from adding
	bool reliable; // If true, isn't deleted on timeout

	bool allReceived()
	{
		return (chunks.size() == chunk_count);
	}
};

/*
=== NOTES ===

A packet is sent through a channel to a peer with a basic header:
TODO: Should we have a receiver_peer_id also?
	Header (7 bytes):
	[0] u32 protocol_id
	[4] u16 sender_peer_id
	[6] u8 channel
sender_peer_id:
	Unique to each peer.
	value 0 is reserved for making new connections
	value 1 is reserved for server
channel:
	The lower the number, the higher the priority is.
	Only channels 0, 1 and 2 exist.
*/
#define BASE_HEADER_SIZE 7
#define PEER_ID_NEW 0
#define PEER_ID_SERVER 1
#define CHANNEL_COUNT 3
/*
Packet types:

CONTROL: This is a packet used by the protocol.
- When this is processed, nothing is handed to the user.
	Header (2 byte):
	[0] u8 type
	[1] u8 controltype
controltype and data description:
	CONTROLTYPE_ACK
		[2] u16 seqnum
	CONTROLTYPE_SET_PEER_ID
		[2] u16 peer_id_new
	CONTROLTYPE_PING
	- This can be sent in a reliable packet to get a reply
*/
#define TYPE_CONTROL 0
#define CONTROLTYPE_ACK 0
#define CONTROLTYPE_SET_PEER_ID 1
#define CONTROLTYPE_PING 2
/*
ORIGINAL: This is a plain packet with no control and no error
checking at all.
- When this is processed, it is directly handed to the user.
	Header (1 byte):
	[0] u8 type
*/
#define TYPE_ORIGINAL 1
#define ORIGINAL_HEADER_SIZE 1
/*
SPLIT: These are sequences of packets forming one bigger piece of
data.
- When processed and all the packet_nums 0...packet_count-1 are
  present (this should be buffered), the resulting data shall be
  directly handed to the user.
- If the data fails to come up in a reasonable time, the buffer shall
  be silently discarded.
- These can be sent as-is or atop of a RELIABLE packet stream.
	Header (7 bytes):
	[0] u8 type
	[1] u16 seqnum
	[3] u16 chunk_count
	[5] u16 chunk_num
*/
#define TYPE_SPLIT 2
/*
RELIABLE: Delivery of all RELIABLE packets shall be forced by ACKs,
and they shall be delivered in the same order as sent. This is done
with a buffer in the receiving and transmitting end.
- When this is processed, the contents of each packet is recursively
  processed as packets.
	Header (3 bytes):
	[0] u8 type
	[1] u16 seqnum

*/
#define TYPE_RELIABLE 3
#define RELIABLE_HEADER_SIZE 3
//#define SEQNUM_INITIAL 0x10
#define SEQNUM_INITIAL 65500

/*
	A buffer which stores reliable packets and sorts them internally
	for fast access to the smallest one.
*/

typedef core::list<BufferedPacket>::Iterator RPBSearchResult;

class ReliablePacketBuffer
{
public:
	
	void print();
	bool empty();
	u32 size();
	RPBSearchResult findPacket(u16 seqnum);
	RPBSearchResult notFound();
	u16 getFirstSeqnum();
	BufferedPacket popFirst();
	BufferedPacket popSeqnum(u16 seqnum);
	void insert(BufferedPacket &p);
	void incrementTimeouts(float dtime);
	void resetTimedOuts(float timeout);
	bool anyTotaltimeReached(float timeout);
	core::list<BufferedPacket> getTimedOuts(float timeout);

private:
	core::list<BufferedPacket> m_list;
};

/*
	A buffer for reconstructing split packets
*/

class IncomingSplitBuffer
{
public:
	~IncomingSplitBuffer();
	/*
		This will throw a GotSplitPacketException when a full
		split packet is constructed.
	*/
	void insert(BufferedPacket &p, bool reliable);
	
	void removeUnreliableTimedOuts(float dtime, float timeout);
	
private:
	// Key is seqnum
	core::map<u16, IncomingSplitPacket*> m_buf;
};

class Connection;

struct Channel
{
	Channel();
	~Channel();
	/*
		Processes a packet with the basic header stripped out.
		Parameters:
			packetdata: Data in packet (with no base headers)
			con: The connection to which the channel is associated
			     (used for sending back stuff (ACKs))
			peer_id: peer id of the sender of the packet in question
			channelnum: channel on which the packet was sent
			reliable: true if recursing into a reliable packet
	*/
	SharedBuffer<u8> ProcessPacket(
			SharedBuffer<u8> packetdata,
			Connection *con,
			u16 peer_id,
			u8 channelnum,
			bool reliable=false);
	
	// Returns next data from a buffer if possible
	// throws a NoIncomingDataException if no data is available
	// If found, sets peer_id
	SharedBuffer<u8> CheckIncomingBuffers(Connection *con,
			u16 &peer_id);

	u16 next_outgoing_seqnum;
	u16 next_incoming_seqnum;
	u16 next_outgoing_split_seqnum;
	
	// This is for buffering the incoming packets that are coming in
	// the wrong order
	ReliablePacketBuffer incoming_reliables;
	// This is for buffering the sent packets so that the sender can
	// re-send them if no ACK is received
	ReliablePacketBuffer outgoing_reliables;

	IncomingSplitBuffer incoming_splits;
};

class Peer;

class PeerHandler
{
public:
	PeerHandler()
	{
	}
	virtual ~PeerHandler()
	{
	}
	
	/*
		This is called after the Peer has been inserted into the
		Connection's peer container.
	*/
	virtual void peerAdded(Peer *peer) = 0;
	/*
		This is called before the Peer has been removed from the
		Connection's peer container.
	*/
	virtual void deletingPeer(Peer *peer, bool timeout) = 0;
};

class Peer
{
public:

	Peer(u16 a_id, Address a_address);
	virtual ~Peer();
	
	/*
		Calculates avg_rtt and resend_timeout.

		rtt=-1 only recalculates resend_timeout
	*/
	void reportRTT(float rtt);

	Channel channels[CHANNEL_COUNT];

	// Address of the peer
	Address address;
	// Unique id of the peer
	u16 id;
	// Seconds from last receive
	float timeout_counter;
	// Ping timer
	float ping_timer;
	// This is changed dynamically
	float resend_timeout;
	// Updated when an ACK is received
	float avg_rtt;
	// This is set to true when the peer has actually sent something
	// with the id we have given to it
	bool has_sent_with_id;
	
private:
};

class Connection
{
public:
	Connection(
		u32 protocol_id,
		u32 max_packet_size,
		float timeout,
		PeerHandler *peerhandler
	);
	~Connection();
	void setTimeoutMs(int timeout){ m_socket.setTimeoutMs(timeout); }
	// Start being a server
	void Serve(unsigned short port);
	// Connect to a server
	void Connect(Address address);
	bool Connected();

	// Sets peer_id
	SharedBuffer<u8> GetFromBuffers(u16 &peer_id);

	// The peer_id of sender is stored in peer_id
	// Return value: I guess this always throws an exception or
	//               actually gets data
	u32 Receive(u16 &peer_id, u8 *data, u32 datasize);
	
	// These will automatically package the data as an original or split
	void SendToAll(u8 channelnum, SharedBuffer<u8> data, bool reliable);
	void Send(u16 peer_id, u8 channelnum, SharedBuffer<u8> data, bool reliable);
	// Send data as a packet; it will be wrapped in base header and
	// optionally to a reliable packet.
	void SendAsPacket(u16 peer_id, u8 channelnum,
			SharedBuffer<u8> data, bool reliable);
	// Sends a raw packet
	void RawSend(const BufferedPacket &packet);
	
	void RunTimeouts(float dtime);
	// Can throw a PeerNotFoundException
	Peer* GetPeer(u16 peer_id);
	// returns NULL if failed
	Peer* GetPeerNoEx(u16 peer_id);
	core::list<Peer*> GetPeers();

	void SetPeerID(u16 id){ m_peer_id = id; }
	u16 GetPeerID(){ return m_peer_id; }
	u32 GetProtocolID(){ return m_protocol_id; }

	// For debug printing
	void PrintInfo(std::ostream &out);
	void PrintInfo();
	u16 m_indentation;

private:
	u32 m_protocol_id;
	float m_timeout;
	PeerHandler *m_peerhandler;
	core::map<u16, Peer*> m_peers;
	u16 m_peer_id;
	//bool m_waiting_new_peer_id;
	u32 m_max_packet_size;
	UDPSocket m_socket;
};

} // namespace

#endif

