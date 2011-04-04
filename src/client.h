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

#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#ifndef SERVER

#include "connection.h"
#include "environment.h"
#include "common_irrlicht.h"
#include "jmutex.h"
#include <ostream>
#include "clientobject.h"

class ClientNotReadyException : public BaseException
{
public:
	ClientNotReadyException(const char *s):
		BaseException(s)
	{}
};

struct QueuedMeshUpdate
{
	v3s16 p;
	MeshMakeData *data;
	bool ack_block_to_server;

	QueuedMeshUpdate():
		p(-1337,-1337,-1337),
		data(NULL),
		ack_block_to_server(false)
	{
	}
	
	~QueuedMeshUpdate()
	{
		if(data)
			delete data;
	}
};

/*
	A thread-safe queue of mesh update tasks
*/
class MeshUpdateQueue
{
public:
	MeshUpdateQueue()
	{
		m_mutex.Init();
	}

	~MeshUpdateQueue()
	{
		JMutexAutoLock lock(m_mutex);

		core::list<QueuedMeshUpdate*>::Iterator i;
		for(i=m_queue.begin(); i!=m_queue.end(); i++)
		{
			QueuedMeshUpdate *q = *i;
			delete q;
		}
	}
	
	/*
		peer_id=0 adds with nobody to send to
	*/
	void addBlock(v3s16 p, MeshMakeData *data, bool ack_block_to_server)
	{
		DSTACK(__FUNCTION_NAME);

		assert(data);
	
		JMutexAutoLock lock(m_mutex);

		/*
			Find if block is already in queue.
			If it is, update the data and quit.
		*/
		core::list<QueuedMeshUpdate*>::Iterator i;
		for(i=m_queue.begin(); i!=m_queue.end(); i++)
		{
			QueuedMeshUpdate *q = *i;
			if(q->p == p)
			{
				if(q->data)
					delete q->data;
				q->data = data;
				if(ack_block_to_server)
					q->ack_block_to_server = true;
				return;
			}
		}
		
		/*
			Add the block
		*/
		QueuedMeshUpdate *q = new QueuedMeshUpdate;
		q->p = p;
		q->data = data;
		q->ack_block_to_server = ack_block_to_server;
		m_queue.push_back(q);
	}

	// Returned pointer must be deleted
	// Returns NULL if queue is empty
	QueuedMeshUpdate * pop()
	{
		JMutexAutoLock lock(m_mutex);

		core::list<QueuedMeshUpdate*>::Iterator i = m_queue.begin();
		if(i == m_queue.end())
			return NULL;
		QueuedMeshUpdate *q = *i;
		m_queue.erase(i);
		return q;
	}

	u32 size()
	{
		JMutexAutoLock lock(m_mutex);
		return m_queue.size();
	}
	
private:
	core::list<QueuedMeshUpdate*> m_queue;
	JMutex m_mutex;
};

struct MeshUpdateResult
{
	v3s16 p;
	scene::SMesh *mesh;
	bool ack_block_to_server;

	MeshUpdateResult():
		p(-1338,-1338,-1338),
		mesh(NULL),
		ack_block_to_server(false)
	{
	}
};

class MeshUpdateThread : public SimpleThread
{
public:

	MeshUpdateThread()
	{
	}

	void * Thread();

	MeshUpdateQueue m_queue_in;

	MutexedQueue<MeshUpdateResult> m_queue_out;
};

#if 0
struct IncomingPacket
{
	IncomingPacket()
	{
		m_data = NULL;
		m_datalen = 0;
		m_refcount = NULL;
	}
	IncomingPacket(const IncomingPacket &a)
	{
		m_data = a.m_data;
		m_datalen = a.m_datalen;
		m_refcount = a.m_refcount;
		if(m_refcount != NULL)
			(*m_refcount)++;
	}
	IncomingPacket(u8 *data, u32 datalen)
	{
		m_data = new u8[datalen];
		memcpy(m_data, data, datalen);
		m_datalen = datalen;
		m_refcount = new s32(1);
	}
	~IncomingPacket()
	{
		if(m_refcount != NULL){
			assert(*m_refcount > 0);
			(*m_refcount)--;
			if(*m_refcount == 0){
				if(m_data != NULL)
					delete[] m_data;
				delete m_refcount;
			}
		}
	}
	/*IncomingPacket & operator=(IncomingPacket a)
	{
		m_data = a.m_data;
		m_datalen = a.m_datalen;
		m_refcount = a.m_refcount;
		(*m_refcount)++;
		return *this;
	}*/
	u8 *m_data;
	u32 m_datalen;
	s32 *m_refcount;
};
#endif

class Client : public con::PeerHandler, public InventoryManager
{
public:
	/*
		NOTE: Nothing is thread-safe here.
	*/

	Client(
			IrrlichtDevice *device,
			const char *playername,
			MapDrawControl &control
			);
	
	~Client();
	/*
		The name of the local player should already be set when
		calling this, as it is sent in the initialization.
	*/
	void connect(Address address);
	/*
		returns true when
			m_con.Connected() == true
			AND m_server_ser_ver != SER_FMT_VER_INVALID
		throws con::PeerNotFoundException if connection has been deleted,
		eg. timed out.
	*/
	bool connectedAndInitialized();
	/*
		Stuff that references the environment is valid only as
		long as this is not called. (eg. Players)
		If this throws a PeerNotFoundException, the connection has
		timed out.
	*/
	void step(float dtime);

	// Called from updater thread
	// Returns dtime
	//float asyncStep();

	void ProcessData(u8 *data, u32 datasize, u16 sender_peer_id);
	// Returns true if something was received
	bool AsyncProcessPacket();
	bool AsyncProcessData();
	void Send(u16 channelnum, SharedBuffer<u8> data, bool reliable);

	// Pops out a packet from the packet queue
	//IncomingPacket getPacket();

	void groundAction(u8 action, v3s16 nodepos_undersurface,
			v3s16 nodepos_oversurface, u16 item);
	void clickObject(u8 button, v3s16 blockpos, s16 id, u16 item);

	void sendSignText(v3s16 blockpos, s16 id, std::string text);
	void sendSignNodeText(v3s16 p, std::string text);
	void sendInventoryAction(InventoryAction *a);
	void sendChatMessage(const std::wstring &message);
	
	// locks envlock
	void removeNode(v3s16 p);
	// locks envlock
	void addNode(v3s16 p, MapNode n);
	
	void updateCamera(v3f pos, v3f dir);
	
	// Returns InvalidPositionException if not found
	MapNode getNode(v3s16 p);
	// Wrapper to Map
	NodeMetadata* getNodeMetadata(v3s16 p);

	v3f getPlayerPosition();

	void setPlayerControl(PlayerControl &control);
	
	// Returns true if the inventory of the local player has been
	// updated from the server. If it is true, it is set to false.
	bool getLocalInventoryUpdated();
	// Copies the inventory of the local player to parameter
	void getLocalInventory(Inventory &dst);
	
	InventoryContext *getInventoryContext();

	Inventory* getInventory(InventoryContext *c, std::string id);
	void inventoryAction(InventoryAction *a);

	// Gets closest object pointed by the shootline
	// Returns NULL if not found
	MapBlockObject * getSelectedObject(
			f32 max_d,
			v3f from_pos_f_on_map,
			core::line3d<f32> shootline_on_map
	);

	// Prints a line or two of info
	void printDebugInfo(std::ostream &os);

	u32 getDayNightRatio();

	//void updateSomeExpiredMeshes();
	
	void setTempMod(v3s16 p, NodeMod mod)
	{
		//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
		assert(m_env.getMap().mapType() == MAPTYPE_CLIENT);

		core::map<v3s16, MapBlock*> affected_blocks;
		((ClientMap&)m_env.getMap()).setTempMod(p, mod,
				&affected_blocks);

		for(core::map<v3s16, MapBlock*>::Iterator
				i = affected_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			i.getNode()->getValue()->updateMesh(m_env.getDayNightRatio());
		}
	}
	void clearTempMod(v3s16 p)
	{
		//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
		assert(m_env.getMap().mapType() == MAPTYPE_CLIENT);

		core::map<v3s16, MapBlock*> affected_blocks;
		((ClientMap&)m_env.getMap()).clearTempMod(p,
				&affected_blocks);

		for(core::map<v3s16, MapBlock*>::Iterator
				i = affected_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			i.getNode()->getValue()->updateMesh(m_env.getDayNightRatio());
		}
	}

	float getAvgRtt()
	{
		//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
		con::Peer *peer = m_con.GetPeerNoEx(PEER_ID_SERVER);
		if(peer == NULL)
			return 0.0;
		return peer->avg_rtt;
	}

	bool getChatMessage(std::wstring &message)
	{
		if(m_chat_queue.size() == 0)
			return false;
		message = m_chat_queue.pop_front();
		return true;
	}

	void addChatMessage(const std::wstring &message)
	{
		//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);
		std::wstring name = narrow_to_wide(player->getName());
		m_chat_queue.push_back(
				(std::wstring)L"<"+name+L"> "+message);
	}

	u64 getMapSeed(){ return m_map_seed; }

	/*
		These are not thread-safe
	*/
	void addUpdateMeshTask(v3s16 blockpos, bool ack_to_server=false);
	// Including blocks at appropriate edges
	void addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server=false);

private:
	
	// Virtual methods from con::PeerHandler
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);
	
	void ReceiveAll();
	void Receive();
	
	void sendPlayerPos();
	// This sends the player's current name etc to the server
	void sendPlayerInfo();
	
	float m_packetcounter_timer;
	float m_delete_unused_sectors_timer;
	float m_connection_reinit_timer;
	float m_avg_rtt_timer;
	float m_playerpos_send_timer;

	MeshUpdateThread m_mesh_update_thread;
	
	ClientEnvironment m_env;
	
	con::Connection m_con;

	IrrlichtDevice *m_device;

	v3f camera_position;
	v3f camera_direction;
	
	// Server serialization version
	u8 m_server_ser_ver;

	// This is behind m_env_mutex.
	bool m_inventory_updated;

	core::map<v3s16, bool> m_active_blocks;

	PacketCounter m_packetcounter;
	
	// Received from the server. 0-23999
	u32 m_time_of_day;
	
	// 0 <= m_daynight_i < DAYNIGHT_CACHE_COUNT
	//s32 m_daynight_i;
	//u32 m_daynight_ratio;

	Queue<std::wstring> m_chat_queue;
	
	// The seed returned by the server in TOCLIENT_INIT is stored here
	u64 m_map_seed;
	
	InventoryContext m_inventory_context;
};

#endif // !SERVER

#endif // !CLIENT_HEADER

