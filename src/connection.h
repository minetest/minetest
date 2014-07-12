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

#ifndef CONNECTION_HEADER
#define CONNECTION_HEADER

#include "irrlichttypes_bloated.h"
#include "socket.h"
#include "exceptions.h"
#include "constants.h"
#include "util/pointer.h"
#include "util/container.h"
#include "util/thread.h"
#include "util/numeric.h"
#include <iostream>
#include <fstream>
#include <list>
#include <map>

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

class ConnectionBindFailed : public BaseException
{
public:
	ConnectionBindFailed(const char *s):
		BaseException(s)
	{}
};

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

class ProcessedQueued : public BaseException
{
public:
	ProcessedQueued(const char *s):
		BaseException(s)
	{}
};

class IncomingDataCorruption : public BaseException
{
public:
	IncomingDataCorruption(const char *s):
		BaseException(s)
	{}
};

typedef enum MTProtocols {
	MTP_PRIMARY,
	MTP_UDP,
	MTP_MINETEST_RELIABLE_UDP
} MTProtocols;

#define SEQNUM_MAX 65535
inline bool seqnum_higher(u16 totest, u16 base)
{
	if (totest > base)
	{
		if((totest - base) > (SEQNUM_MAX/2))
			return false;
		else
			return true;
	}
	else
	{
		if((base - totest) > (SEQNUM_MAX/2))
			return true;
		else
			return false;
	}
}

inline bool seqnum_in_window(u16 seqnum, u16 next,u16 window_size)
{
	u16 window_start = next;
	u16 window_end   = ( next + window_size ) % (SEQNUM_MAX+1);

	if (window_start < window_end)
	{
		return ((seqnum >= window_start) && (seqnum < window_end));
	}
	else
	{
		return ((seqnum < window_end) || (seqnum >= window_start));
	}
}

struct BufferedPacket
{
	BufferedPacket(u8 *a_data, u32 a_size):
		data(a_data, a_size), time(0.0), totaltime(0.0), absolute_send_time(-1),
		resend_count(0)
	{}
	BufferedPacket(u32 a_size):
		data(a_size), time(0.0), totaltime(0.0), absolute_send_time(-1),
		resend_count(0)
	{}
	SharedBuffer<u8> data; // Data of the packet, including headers
	float time; // Seconds from buffering the packet or re-sending
	float totaltime; // Seconds from buffering the packet
	unsigned int absolute_send_time;
	Address address; // Sender or destination
	unsigned int resend_count;
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
std::list<SharedBuffer<u8> > makeSplitPacket(
		SharedBuffer<u8> data,
		u32 chunksize_max,
		u16 seqnum);

// Depending on size, make a TYPE_ORIGINAL or TYPE_SPLIT packet
// Increments split_seqnum if a split packet is made
std::list<SharedBuffer<u8> > makeAutoSplitPacket(
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
	std::map<u16, SharedBuffer<u8> > chunks;
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
	value 0 (PEER_ID_INEXISTENT) is reserved for making new connections
	value 1 (PEER_ID_SERVER) is reserved for server
	these constants are defined in constants.h
channel:
	The lower the number, the higher the priority is.
	Only channels 0, 1 and 2 exist.
*/
#define BASE_HEADER_SIZE 7
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
	- There is no actual reply, but this can be sent in a reliable
	  packet to get a reply
	CONTROLTYPE_DISCO
*/
#define TYPE_CONTROL 0
#define CONTROLTYPE_ACK 0
#define CONTROLTYPE_SET_PEER_ID 1
#define CONTROLTYPE_PING 2
#define CONTROLTYPE_DISCO 3
#define CONTROLTYPE_ENABLE_BIG_SEND_WINDOW 4

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
#define SEQNUM_INITIAL 65500

/*
	A buffer which stores reliable packets and sorts them internally
	for fast access to the smallest one.
*/

typedef std::list<BufferedPacket>::iterator RPBSearchResult;

class ReliablePacketBuffer
{
public:
	ReliablePacketBuffer();

	bool getFirstSeqnum(u16& result);

	BufferedPacket popFirst();
	BufferedPacket popSeqnum(u16 seqnum);
	void insert(BufferedPacket &p,u16 next_expected);

	void incrementTimeouts(float dtime);
	std::list<BufferedPacket> getTimedOuts(float timeout,
			unsigned int max_packets);

	void print();
	bool empty();
	bool containsPacket(u16 seqnum);
	RPBSearchResult notFound();
	u32 size();


private:
	RPBSearchResult findPacket(u16 seqnum);

	std::list<BufferedPacket> m_list;
	u32 m_list_size;

	u16 m_oldest_non_answered_ack;

	JMutex m_list_mutex;
};

/*
	A buffer for reconstructing split packets
*/

class IncomingSplitBuffer
{
public:
	~IncomingSplitBuffer();
	/*
		Returns a reference counted buffer of length != 0 when a full split
		packet is constructed. If not, returns one of length 0.
	*/
	SharedBuffer<u8> insert(BufferedPacket &p, bool reliable);
	
	void removeUnreliableTimedOuts(float dtime, float timeout);
	
private:
	// Key is seqnum
	std::map<u16, IncomingSplitPacket*> m_buf;

	JMutex m_map_mutex;
};

struct OutgoingPacket
{
	u16 peer_id;
	u8 channelnum;
	SharedBuffer<u8> data;
	bool reliable;
	bool ack;

	OutgoingPacket(u16 peer_id_, u8 channelnum_, SharedBuffer<u8> data_,
			bool reliable_,bool ack_=false):
		peer_id(peer_id_),
		channelnum(channelnum_),
		data(data_),
		reliable(reliable_),
		ack(ack_)
	{
	}
};

enum ConnectionCommandType{
	CONNCMD_NONE,
	CONNCMD_SERVE,
	CONNCMD_CONNECT,
	CONNCMD_DISCONNECT,
	CONNCMD_DISCONNECT_PEER,
	CONNCMD_SEND,
	CONNCMD_SEND_TO_ALL,
	CONCMD_ACK,
	CONCMD_CREATE_PEER,
	CONCMD_DISABLE_LEGACY
};

struct ConnectionCommand
{
	enum ConnectionCommandType type;
	Address address;
	u16 peer_id;
	u8 channelnum;
	Buffer<u8> data;
	bool reliable;
	bool raw;

	ConnectionCommand(): type(CONNCMD_NONE), peer_id(PEER_ID_INEXISTENT), reliable(false), raw(false) {}

	void serve(Address address_)
	{
		type = CONNCMD_SERVE;
		address = address_;
	}
	void connect(Address address_)
	{
		type = CONNCMD_CONNECT;
		address = address_;
	}
	void disconnect()
	{
		type = CONNCMD_DISCONNECT;
	}
	void disconnect_peer(u16 peer_id_)
	{
		type = CONNCMD_DISCONNECT_PEER;
		peer_id = peer_id_;
	}
	void send(u16 peer_id_, u8 channelnum_,
			SharedBuffer<u8> data_, bool reliable_)
	{
		type = CONNCMD_SEND;
		peer_id = peer_id_;
		channelnum = channelnum_;
		data = data_;
		reliable = reliable_;
	}
	void sendToAll(u8 channelnum_, SharedBuffer<u8> data_, bool reliable_)
	{
		type = CONNCMD_SEND_TO_ALL;
		channelnum = channelnum_;
		data = data_;
		reliable = reliable_;
	}

	void ack(u16 peer_id_, u8 channelnum_, SharedBuffer<u8> data_)
	{
		type = CONCMD_ACK;
		peer_id = peer_id_;
		channelnum = channelnum_;
		data = data_;
		reliable = false;
	}

	void createPeer(u16 peer_id_, SharedBuffer<u8> data_)
	{
		type = CONCMD_CREATE_PEER;
		peer_id = peer_id_;
		data = data_;
		channelnum = 0;
		reliable = true;
		raw = true;
	}

	void disableLegacy(u16 peer_id_, SharedBuffer<u8> data_)
	{
		type = CONCMD_DISABLE_LEGACY;
		peer_id = peer_id_;
		data = data_;
		channelnum = 0;
		reliable = true;
		raw = true;
	}
};

class Channel
{

public:
	u16 readNextIncomingSeqNum();
	u16 incNextIncomingSeqNum();

	u16 getOutgoingSequenceNumber(bool& successfull);
	u16 readOutgoingSequenceNumber();
	bool putBackSequenceNumber(u16);

	u16 readNextSplitSeqNum();
	void setNextSplitSeqNum(u16 seqnum);
	
	// This is for buffering the incoming packets that are coming in
	// the wrong order
	ReliablePacketBuffer incoming_reliables;
	// This is for buffering the sent packets so that the sender can
	// re-send them if no ACK is received
	ReliablePacketBuffer outgoing_reliables_sent;

	//queued reliable packets
	Queue<BufferedPacket> queued_reliables;

	//queue commands prior splitting to packets
	Queue<ConnectionCommand> queued_commands;

	IncomingSplitBuffer incoming_splits;

	Channel();
	~Channel();

	void UpdatePacketLossCounter(unsigned int count);
	void UpdatePacketTooLateCounter();
	void UpdateBytesSent(unsigned int bytes,unsigned int packages=1);
	void UpdateBytesLost(unsigned int bytes);
	void UpdateBytesReceived(unsigned int bytes);

	void UpdateTimers(float dtime, bool legacy_peer);

	const float getCurrentDownloadRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return cur_kbps; };
	const float getMaxDownloadRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return max_kbps; };

	const float getCurrentLossRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return cur_kbps_lost; };
	const float getMaxLossRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return max_kbps_lost; };

	const float getCurrentIncomingRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return cur_incoming_kbps; };
	const float getMaxIncomingRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return max_incoming_kbps; };

	const float getAvgDownloadRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return avg_kbps; };
	const float getAvgLossRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return avg_kbps_lost; };
	const float getAvgIncomingRateKB()
		{ JMutexAutoLock lock(m_internal_mutex); return avg_incoming_kbps; };

	const unsigned int getWindowSize() const { return window_size; };

	void setWindowSize(unsigned int size) { window_size = size; };
private:
	JMutex m_internal_mutex;
	int window_size;

	u16 next_incoming_seqnum;

	u16 next_outgoing_seqnum;
	u16 next_outgoing_split_seqnum;

	unsigned int current_packet_loss;
	unsigned int current_packet_too_late;
	unsigned int current_packet_successfull;
	float packet_loss_counter;

	unsigned int current_bytes_transfered;
	unsigned int current_bytes_received;
	unsigned int current_bytes_lost;
	float max_kbps;
	float cur_kbps;
	float avg_kbps;
	float max_incoming_kbps;
	float cur_incoming_kbps;
	float avg_incoming_kbps;
	float max_kbps_lost;
	float cur_kbps_lost;
	float avg_kbps_lost;
	float bpm_counter;

	unsigned int rate_samples;
};

class Peer;

enum PeerChangeType
{
	PEER_ADDED,
	PEER_REMOVED
};
struct PeerChange
{
	PeerChangeType type;
	u16 peer_id;
	bool timeout;
};

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

class PeerHelper
{
public:
	PeerHelper();
	PeerHelper(Peer* peer);
	~PeerHelper();

	PeerHelper&   operator=(Peer* peer);
	Peer*         operator->() const;
	bool          operator!();
	Peer*         operator&() const;
	bool          operator!=(void* ptr);

private:
	Peer* m_peer;
};

class Connection;

typedef enum {
	MIN_RTT,
	MAX_RTT,
	AVG_RTT,
	MIN_JITTER,
	MAX_JITTER,
	AVG_JITTER
} rtt_stat_type;

typedef enum {
	CUR_DL_RATE,
	AVG_DL_RATE,
	CUR_INC_RATE,
	AVG_INC_RATE,
	CUR_LOSS_RATE,
	AVG_LOSS_RATE,
} rate_stat_type;

class Peer {
	public:
		friend class PeerHelper;

		Peer(Address address_,u16 id_,Connection* connection) :
			id(id_),
			m_increment_packets_remaining(9),
			m_increment_bytes_remaining(0),
			m_pending_deletion(false),
			m_connection(connection),
			address(address_),
			m_ping_timer(0.0),
			m_last_rtt(-1.0),
			m_usage(0),
			m_timeout_counter(0.0),
			m_last_timeout_check(porting::getTimeMs()),
			m_has_sent_with_id(false)
		{
			m_rtt.avg_rtt = -1.0;
			m_rtt.jitter_avg = -1.0;
			m_rtt.jitter_max = 0.0;
			m_rtt.max_rtt = 0.0;
			m_rtt.jitter_min = FLT_MAX;
			m_rtt.min_rtt = FLT_MAX;
		};

		virtual ~Peer() {
			JMutexAutoLock usage_lock(m_exclusive_access_mutex);
			assert(m_usage == 0);
		};

		// Unique id of the peer
		u16 id;

		void Drop();

		virtual void PutReliableSendCommand(ConnectionCommand &c,
						unsigned int max_packet_size) {};

		virtual bool isActive() { return false; };

		virtual bool getAddress(MTProtocols type, Address& toset) = 0;

		void ResetTimeout()
			{JMutexAutoLock lock(m_exclusive_access_mutex); m_timeout_counter=0.0; };

		bool isTimedOut(float timeout);

		void setSentWithID()
		{ JMutexAutoLock lock(m_exclusive_access_mutex); m_has_sent_with_id = true; };

		bool hasSentWithID()
		{ JMutexAutoLock lock(m_exclusive_access_mutex); return m_has_sent_with_id; };

		unsigned int m_increment_packets_remaining;
		unsigned int m_increment_bytes_remaining;

		virtual u16 getNextSplitSequenceNumber(u8 channel) { return 0; };
		virtual void setNextSplitSequenceNumber(u8 channel, u16 seqnum) {};
		virtual SharedBuffer<u8> addSpiltPacket(u8 channel,
												BufferedPacket toadd,
												bool reliable)
				{
					fprintf(stderr,"Peer: addSplitPacket called, this is supposed to be never called!\n");
					return SharedBuffer<u8>(0);
				};

		virtual bool Ping(float dtime, SharedBuffer<u8>& data) { return false; };

		virtual float getStat(rtt_stat_type type) const {
			switch (type) {
				case MIN_RTT:
					return m_rtt.min_rtt;
				case MAX_RTT:
					return m_rtt.max_rtt;
				case AVG_RTT:
					return m_rtt.avg_rtt;
				case MIN_JITTER:
					return m_rtt.jitter_min;
				case MAX_JITTER:
					return m_rtt.jitter_max;
				case AVG_JITTER:
					return m_rtt.jitter_avg;
			}
			return -1;
		}
	protected:
		virtual void reportRTT(float rtt) {};

		void RTTStatistics(float rtt,
							std::string profiler_id="",
							unsigned int num_samples=1000);

		bool IncUseCount();
		void DecUseCount();

		JMutex m_exclusive_access_mutex;

		bool m_pending_deletion;

		Connection* m_connection;

		// Address of the peer
		Address address;

		// Ping timer
		float m_ping_timer;
	private:

		struct rttstats {
			float jitter_min;
			float jitter_max;
			float jitter_avg;
			float min_rtt;
			float max_rtt;
			float avg_rtt;
		};

		rttstats m_rtt;
		float    m_last_rtt;

		// current usage count
		unsigned int m_usage;

		// Seconds from last receive
		float m_timeout_counter;

		u32 m_last_timeout_check;

		bool m_has_sent_with_id;
};

class UDPPeer : public Peer
{
public:

	friend class PeerHelper;
	friend class ConnectionReceiveThread;
	friend class ConnectionSendThread;
	friend class Connection;

	UDPPeer(u16 a_id, Address a_address, Connection* connection);
	virtual ~UDPPeer() {};

	void PutReliableSendCommand(ConnectionCommand &c,
							unsigned int max_packet_size);

	bool isActive()
	{ return ((hasSentWithID()) && (!m_pending_deletion)); };

	bool getAddress(MTProtocols type, Address& toset);

	void setNonLegacyPeer();

	bool getLegacyPeer()
	{ return m_legacy_peer; }

	u16 getNextSplitSequenceNumber(u8 channel);
	void setNextSplitSequenceNumber(u8 channel, u16 seqnum);

	SharedBuffer<u8> addSpiltPacket(u8 channel,
									BufferedPacket toadd,
									bool reliable);


protected:
	/*
		Calculates avg_rtt and resend_timeout.
		rtt=-1 only recalculates resend_timeout
	*/
	void reportRTT(float rtt);

	void RunCommandQueues(
					unsigned int max_packet_size,
					unsigned int maxcommands,
					unsigned int maxtransfer);

	float getResendTimeout()
		{ JMutexAutoLock lock(m_exclusive_access_mutex); return resend_timeout; }

	void setResendTimeout(float timeout)
		{ JMutexAutoLock lock(m_exclusive_access_mutex); resend_timeout = timeout; }
	bool Ping(float dtime,SharedBuffer<u8>& data);

	Channel channels[CHANNEL_COUNT];
	bool m_pending_disconnect;
private:
	// This is changed dynamically
	float resend_timeout;

	bool processReliableSendCommand(
					ConnectionCommand &c,
					unsigned int max_packet_size);

	bool m_legacy_peer;
};

/*
	Connection
*/

enum ConnectionEventType{
	CONNEVENT_NONE,
	CONNEVENT_DATA_RECEIVED,
	CONNEVENT_PEER_ADDED,
	CONNEVENT_PEER_REMOVED,
	CONNEVENT_BIND_FAILED,
};

struct ConnectionEvent
{
	enum ConnectionEventType type;
	u16 peer_id;
	Buffer<u8> data;
	bool timeout;
	Address address;

	ConnectionEvent(): type(CONNEVENT_NONE) {}

	std::string describe()
	{
		switch(type){
		case CONNEVENT_NONE:
			return "CONNEVENT_NONE";
		case CONNEVENT_DATA_RECEIVED:
			return "CONNEVENT_DATA_RECEIVED";
		case CONNEVENT_PEER_ADDED:
			return "CONNEVENT_PEER_ADDED";
		case CONNEVENT_PEER_REMOVED:
			return "CONNEVENT_PEER_REMOVED";
		case CONNEVENT_BIND_FAILED:
			return "CONNEVENT_BIND_FAILED";
		}
		return "Invalid ConnectionEvent";
	}
	
	void dataReceived(u16 peer_id_, SharedBuffer<u8> data_)
	{
		type = CONNEVENT_DATA_RECEIVED;
		peer_id = peer_id_;
		data = data_;
	}
	void peerAdded(u16 peer_id_, Address address_)
	{
		type = CONNEVENT_PEER_ADDED;
		peer_id = peer_id_;
		address = address_;
	}
	void peerRemoved(u16 peer_id_, bool timeout_, Address address_)
	{
		type = CONNEVENT_PEER_REMOVED;
		peer_id = peer_id_;
		timeout = timeout_;
		address = address_;
	}
	void bindFailed()
	{
		type = CONNEVENT_BIND_FAILED;
	}
};

class ConnectionSendThread : public JThread {

public:
	friend class UDPPeer;

	ConnectionSendThread(Connection* parent,
							unsigned int max_packet_size, float timeout);

	void * Thread       ();

	void Trigger();

	void setPeerTimeout(float peer_timeout)
		{ m_timeout = peer_timeout; }

private:
	void runTimeouts    (float dtime);
	void rawSend        (const BufferedPacket &packet);
	bool rawSendAsPacket(u16 peer_id, u8 channelnum,
							SharedBuffer<u8> data, bool reliable);

	void processReliableCommand (ConnectionCommand &c);
	void processNonReliableCommand (ConnectionCommand &c);
	void serve          (Address bind_address);
	void connect        (Address address);
	void disconnect     ();
	void disconnect_peer(u16 peer_id);
	void send           (u16 peer_id, u8 channelnum,
							SharedBuffer<u8> data);
	void sendReliable   (ConnectionCommand &c);
	void sendToAll      (u8 channelnum,
							SharedBuffer<u8> data);
	void sendToAllReliable(ConnectionCommand &c);

	void sendPackets    (float dtime);

	void sendAsPacket   (u16 peer_id, u8 channelnum,
							SharedBuffer<u8> data,bool ack=false);

	void sendAsPacketReliable(BufferedPacket& p, Channel* channel);

	bool packetsQueued();

	Connection*           m_connection;
	unsigned int          m_max_packet_size;
	float                 m_timeout;
	Queue<OutgoingPacket> m_outgoing_queue;
	JSemaphore            m_send_sleep_semaphore;

	unsigned int          m_iteration_packets_avaialble;
	unsigned int          m_max_commands_per_iteration;
	unsigned int          m_max_data_packets_per_iteration;
	unsigned int          m_max_packets_requeued;
};

class ConnectionReceiveThread : public JThread {
public:
	ConnectionReceiveThread(Connection* parent,
							unsigned int max_packet_size);

	void * Thread       ();

private:
	void receive        ();

	// Returns next data from a buffer if possible
	// If found, returns true; if not, false.
	// If found, sets peer_id and dst
	bool getFromBuffers (u16 &peer_id, SharedBuffer<u8> &dst);

	bool checkIncomingBuffers(Channel *channel, u16 &peer_id,
							SharedBuffer<u8> &dst);

	/*
		Processes a packet with the basic header stripped out.
		Parameters:
			packetdata: Data in packet (with no base headers)
			peer_id: peer id of the sender of the packet in question
			channelnum: channel on which the packet was sent
			reliable: true if recursing into a reliable packet
	*/
	SharedBuffer<u8> processPacket(Channel *channel,
							SharedBuffer<u8> packetdata, u16 peer_id,
							u8 channelnum, bool reliable);


	Connection*           m_connection;
};

class Connection
{
public:
	friend class ConnectionSendThread;
	friend class ConnectionReceiveThread;

	Connection(u32 protocol_id, u32 max_packet_size, float timeout, bool ipv6);
	Connection(u32 protocol_id, u32 max_packet_size, float timeout, bool ipv6,
			PeerHandler *peerhandler);
	~Connection();

	/* Interface */
	ConnectionEvent getEvent();
	ConnectionEvent waitEvent(u32 timeout_ms);
	void putCommand(ConnectionCommand &c);
	
	void SetTimeoutMs(int timeout){ m_bc_receive_timeout = timeout; }
	void Serve(Address bind_addr);
	void Connect(Address address);
	bool Connected();
	void Disconnect();
	u32 Receive(u16 &peer_id, SharedBuffer<u8> &data);
	void SendToAll(u8 channelnum, SharedBuffer<u8> data, bool reliable);
	void Send(u16 peer_id, u8 channelnum, SharedBuffer<u8> data, bool reliable);
	u16 GetPeerID(){ return m_peer_id; }
	Address GetPeerAddress(u16 peer_id);
	float getPeerStat(u16 peer_id, rtt_stat_type type);
	float getLocalStat(rate_stat_type type);
	const u32 GetProtocolID() const { return m_protocol_id; };
	const std::string getDesc();
	void DisconnectPeer(u16 peer_id);

protected:
	PeerHelper getPeer(u16 peer_id);
	PeerHelper getPeerNoEx(u16 peer_id);
	u16   lookupPeer(Address& sender);

	u16 createPeer(Address& sender, MTProtocols protocol, int fd);
	UDPPeer*  createServerPeer(Address& sender);
	bool deletePeer(u16 peer_id, bool timeout);

	void SetPeerID(u16 id){ m_peer_id = id; }

	void sendAck(u16 peer_id, u8 channelnum, u16 seqnum);

	void PrintInfo(std::ostream &out);
	void PrintInfo();

	std::list<u16> getPeerIDs();

	UDPSocket m_udpSocket;
	MutexedQueue<ConnectionCommand> m_command_queue;

	void putEvent(ConnectionEvent &e);

	void TriggerSend()
		{ m_sendThread.Trigger(); }
private:
	std::list<Peer*> getPeers();

	MutexedQueue<ConnectionEvent> m_event_queue;

	u16 m_peer_id;
	u32 m_protocol_id;
	
	std::map<u16, Peer*> m_peers;
	JMutex m_peers_mutex;

	ConnectionSendThread m_sendThread;
	ConnectionReceiveThread m_receiveThread;

	JMutex m_info_mutex;

	// Backwards compatibility
	PeerHandler *m_bc_peerhandler;
	int m_bc_receive_timeout;

	bool m_shutting_down;

	u16 m_next_remote_peer_id;
};

} // namespace

#endif

