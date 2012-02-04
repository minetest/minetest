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
#include "utility.h" // For IntervalLimiter
#include "gamedef.h"
#include "inventorymanager.h"
#include "filesys.h"

struct MeshMakeData;
class IGameDef;
class IWritableTextureSource;
class IWritableItemDefManager;
class IWritableNodeDefManager;
//class IWritableCraftDefManager;

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

	QueuedMeshUpdate();
	~QueuedMeshUpdate();
};

/*
	A thread-safe queue of mesh update tasks
*/
class MeshUpdateQueue
{
public:
	MeshUpdateQueue();

	~MeshUpdateQueue();
	
	/*
		peer_id=0 adds with nobody to send to
	*/
	void addBlock(v3s16 p, MeshMakeData *data, bool ack_block_to_server);

	// Returned pointer must be deleted
	// Returns NULL if queue is empty
	QueuedMeshUpdate * pop();

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

	MeshUpdateThread(IGameDef *gamedef):
		m_gamedef(gamedef)
	{
	}

	void * Thread();

	MeshUpdateQueue m_queue_in;

	MutexedQueue<MeshUpdateResult> m_queue_out;

	IGameDef *m_gamedef;
};

enum ClientEventType
{
	CE_NONE,
	CE_PLAYER_DAMAGE,
	CE_PLAYER_FORCE_MOVE,
	CE_DEATHSCREEN,
	CE_TEXTURES_UPDATED
};

struct ClientEvent
{
	ClientEventType type;
	union{
		struct{
		} none;
		struct{
			u8 amount;
		} player_damage;
		struct{
			f32 pitch;
			f32 yaw;
		} player_force_move;
		struct{
			bool set_camera_point_target;
			f32 camera_point_target_x;
			f32 camera_point_target_y;
			f32 camera_point_target_z;
		} deathscreen;
		struct{
		} textures_updated;
	};
};

class Client : public con::PeerHandler, public InventoryManager, public IGameDef
{
public:
	/*
		NOTE: Nothing is thread-safe here.
	*/

	Client(
			IrrlichtDevice *device,
			const char *playername,
			std::string password,
			MapDrawControl &control,
			IWritableTextureSource *tsrc,
			IWritableItemDefManager *itemdef,
			IWritableNodeDefManager *nodedef
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

	void interact(u8 action, const PointedThing& pointed);

	void sendSignNodeText(v3s16 p, std::string text);
	void sendInventoryAction(InventoryAction *a);
	void sendChatMessage(const std::wstring &message);
	void sendChangePassword(const std::wstring oldpassword,
		const std::wstring newpassword);
	void sendDamage(u8 damage);
	void sendRespawn();
	
	// locks envlock
	void removeNode(v3s16 p);
	// locks envlock
	void addNode(v3s16 p, MapNode n);
	
	void updateCamera(v3f pos, v3f dir, f32 fov);
	
	void renderPostFx();
	
	// Returns InvalidPositionException if not found
	MapNode getNode(v3s16 p);
	// Wrapper to Map
	NodeMetadata* getNodeMetadata(v3s16 p);

	LocalPlayer* getLocalPlayer();

	void setPlayerControl(PlayerControl &control);

	void selectPlayerItem(u16 item);
	u16 getPlayerItem() const
	{ return m_playeritem; }

	// Returns true if the inventory of the local player has been
	// updated from the server. If it is true, it is set to false.
	bool getLocalInventoryUpdated();
	// Copies the inventory of the local player to parameter
	void getLocalInventory(Inventory &dst);
	
	/* InventoryManager interface */
	Inventory* getInventory(const InventoryLocation &loc);
	void inventoryAction(InventoryAction *a);

	// Gets closest object pointed by the shootline
	// Returns NULL if not found
	ClientActiveObject * getSelectedActiveObject(
			f32 max_d,
			v3f from_pos_f_on_map,
			core::line3d<f32> shootline_on_map
	);

	// Prints a line or two of info
	void printDebugInfo(std::ostream &os);

	u32 getDayNightRatio();

	u16 getHP();

	void setTempMod(v3s16 p, NodeMod mod);
	void clearTempMod(v3s16 p);

	float getAvgRtt()
	{
		try{
			return m_con.GetPeerAvgRTT(PEER_ID_SERVER);
		} catch(con::PeerNotFoundException){
			return 1337;
		}
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
		if (message[0] == L'/') {
			m_chat_queue.push_back(
				(std::wstring)L"issued command: "+message);
			return;
		}

		//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);
		std::wstring name = narrow_to_wide(player->getName());
		m_chat_queue.push_back(
				(std::wstring)L"<"+name+L"> "+message);
	}

	u64 getMapSeed(){ return m_map_seed; }

	void addUpdateMeshTask(v3s16 blockpos, bool ack_to_server=false);
	// Including blocks at appropriate edges
	void addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server=false);

	// Get event from queue. CE_NONE is returned if queue is empty.
	ClientEvent getClientEvent();
	
	bool accessDenied()
	{ return m_access_denied; }

	std::wstring accessDeniedReason()
	{ return m_access_denied_reason; }

	float textureReceiveProgress()
	{ return m_texture_receive_progress; }

	bool texturesReceived()
	{ return m_textures_received; }
	bool itemdefReceived()
	{ return m_itemdef_received; }
	bool nodedefReceived()
	{ return m_nodedef_received; }
	
	void afterContentReceived();

	float getRTT(void);

	// IGameDef interface
	virtual IItemDefManager* getItemDefManager();
	virtual INodeDefManager* getNodeDefManager();
	virtual ICraftDefManager* getCraftDefManager();
	virtual ITextureSource* getTextureSource();
	virtual u16 allocateUnknownNodeId(const std::string &name);

private:
	
	// Virtual methods from con::PeerHandler
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);
	
	void ReceiveAll();
	void Receive();
	
	void sendPlayerPos();
	// This sends the player's current name etc to the server
	void sendPlayerInfo();
	// Send the item number 'item' as player item to the server
	void sendPlayerItem(u16 item);
	
	float m_packetcounter_timer;
	float m_connection_reinit_timer;
	float m_avg_rtt_timer;
	float m_playerpos_send_timer;
	float m_ignore_damage_timer; // Used after server moves player
	IntervalLimiter m_map_timer_and_unload_interval;

	IWritableTextureSource *m_tsrc;
	IWritableItemDefManager *m_itemdef;
	IWritableNodeDefManager *m_nodedef;
	MeshUpdateThread m_mesh_update_thread;
	ClientEnvironment m_env;
	con::Connection m_con;
	IrrlichtDevice *m_device;
	// Server serialization version
	u8 m_server_ser_ver;
	u16 m_playeritem;
	bool m_inventory_updated;
	Inventory *m_inventory_from_server;
	float m_inventory_from_server_age;
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
	std::string m_password;
	bool m_access_denied;
	std::wstring m_access_denied_reason;
	Queue<ClientEvent> m_client_event_queue;
	float m_texture_receive_progress;
	bool m_textures_received;
	bool m_itemdef_received;
	bool m_nodedef_received;
	friend class FarMesh;
};

#endif // !SERVER

#endif // !CLIENT_HEADER

