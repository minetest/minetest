/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
MeshUpdateQueue::(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "client.h"
#include "utility.h"
#include <iostream>
#include "clientserver.h"
#include "jmutexautolock.h"
#include "main.h"
#include <sstream>
#include "porting.h"
#include "mapsector.h"
#include "mapblock_mesh.h"
#include "mapblock.h"

/*
	QueuedMeshUpdate
*/

QueuedMeshUpdate::QueuedMeshUpdate():
	p(-1337,-1337,-1337),
	data(NULL),
	ack_block_to_server(false)
{
}

QueuedMeshUpdate::~QueuedMeshUpdate()
{
	if(data)
		delete data;
}

/*
	MeshUpdateQueue
*/
	
MeshUpdateQueue::MeshUpdateQueue()
{
	m_mutex.Init();
}

MeshUpdateQueue::~MeshUpdateQueue()
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
void MeshUpdateQueue::addBlock(v3s16 p, MeshMakeData *data, bool ack_block_to_server)
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
QueuedMeshUpdate * MeshUpdateQueue::pop()
{
	JMutexAutoLock lock(m_mutex);

	core::list<QueuedMeshUpdate*>::Iterator i = m_queue.begin();
	if(i == m_queue.end())
		return NULL;
	QueuedMeshUpdate *q = *i;
	m_queue.erase(i);
	return q;
}

/*
	MeshUpdateThread
*/

void * MeshUpdateThread::Thread()
{
	ThreadStarted();

	DSTACK(__FUNCTION_NAME);
	
	BEGIN_DEBUG_EXCEPTION_HANDLER

	while(getRun())
	{
		/*// Wait for output queue to flush.
		// Allow 2 in queue, this makes less frametime jitter.
		// Umm actually, there is no much difference
		if(m_queue_out.size() >= 2)
		{
			sleep_ms(3);
			continue;
		}*/

		QueuedMeshUpdate *q = m_queue_in.pop();
		if(q == NULL)
		{
			sleep_ms(3);
			continue;
		}

		ScopeProfiler sp(&g_profiler, "mesh make");

		scene::SMesh *mesh_new = NULL;
		mesh_new = makeMapBlockMesh(q->data);

		MeshUpdateResult r;
		r.p = q->p;
		r.mesh = mesh_new;
		r.ack_block_to_server = q->ack_block_to_server;

		/*dstream<<"MeshUpdateThread: Processed "
				<<"("<<q->p.X<<","<<q->p.Y<<","<<q->p.Z<<")"
				<<std::endl;*/

		m_queue_out.push_back(r);

		delete q;
	}

	END_DEBUG_EXCEPTION_HANDLER

	return NULL;
}

Client::Client(
		IrrlichtDevice *device,
		const char *playername,
		std::string password,
		MapDrawControl &control):
	m_mesh_update_thread(),
	m_env(
		new ClientMap(this, control,
			device->getSceneManager()->getRootSceneNode(),
			device->getSceneManager(), 666),
		device->getSceneManager()
	),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this),
	m_device(device),
	camera_position(0,0,0),
	camera_direction(0,0,1),
	m_server_ser_ver(SER_FMT_VER_INVALID),
	m_inventory_updated(false),
	m_time_of_day(0),
	m_map_seed(0),
	m_password(password),
	m_access_denied(false)
{
	m_packetcounter_timer = 0.0;
	m_delete_unused_sectors_timer = 0.0;
	m_connection_reinit_timer = 0.0;
	m_avg_rtt_timer = 0.0;
	m_playerpos_send_timer = 0.0;
	m_ignore_damage_timer = 0.0;

	//m_env_mutex.Init();
	//m_con_mutex.Init();

	m_mesh_update_thread.Start();

	/*
		Add local player
	*/
	{
		//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out

		Player *player = new LocalPlayer();

		player->updateName(playername);

		m_env.addPlayer(player);
		
		// Initialize player in the inventory context
		m_inventory_context.current_player = player;
	}
}

Client::~Client()
{
	{
		//JMutexAutoLock conlock(m_con_mutex); //bulk comment-out
		m_con.Disconnect();
	}

	m_mesh_update_thread.setRun(false);
	while(m_mesh_update_thread.IsRunning())
		sleep_ms(100);
}

void Client::connect(Address address)
{
	DSTACK(__FUNCTION_NAME);
	//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
	m_con.setTimeoutMs(0);
	m_con.Connect(address);
}

bool Client::connectedAndInitialized()
{
	//JMutexAutoLock lock(m_con_mutex); //bulk comment-out

	if(m_con.Connected() == false)
		return false;
	
	if(m_server_ser_ver == SER_FMT_VER_INVALID)
		return false;
	
	return true;
}

void Client::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);
	
	// Limit a bit
	if(dtime > 2.0)
		dtime = 2.0;
	
	if(m_ignore_damage_timer > dtime)
		m_ignore_damage_timer -= dtime;
	else
		m_ignore_damage_timer = 0.0;
	
	//dstream<<"Client steps "<<dtime<<std::endl;

	{
		//TimeTaker timer("ReceiveAll()", m_device);
		// 0ms
		ReceiveAll();
	}
	
	{
		//TimeTaker timer("m_con_mutex + m_con.RunTimeouts()", m_device);
		// 0ms
		//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
		m_con.RunTimeouts(dtime);
	}

	/*
		Packet counter
	*/
	{
		float &counter = m_packetcounter_timer;
		counter -= dtime;
		if(counter <= 0.0)
		{
			counter = 20.0;
			
			dout_client<<"Client packetcounter (20s):"<<std::endl;
			m_packetcounter.print(dout_client);
			m_packetcounter.clear();
		}
	}

	{
		/*
			Delete unused sectors

			NOTE: This jams the game for a while because deleting sectors
			      clear caches
		*/
		
		float &counter = m_delete_unused_sectors_timer;
		counter -= dtime;
		if(counter <= 0.0)
		{
			// 3 minute interval
			//counter = 180.0;
			counter = 60.0;

			//JMutexAutoLock lock(m_env_mutex); //bulk comment-out

			core::list<v3s16> deleted_blocks;

			float delete_unused_sectors_timeout = 
				g_settings.getFloat("client_delete_unused_sectors_timeout");
	
			// Delete sector blocks
			/*u32 num = m_env.getMap().unloadUnusedData
					(delete_unused_sectors_timeout,
					true, &deleted_blocks);*/
			
			// Delete whole sectors
			u32 num = m_env.getMap().unloadUnusedData
					(delete_unused_sectors_timeout,
					false, &deleted_blocks);

			if(num > 0)
			{
				/*dstream<<DTIME<<"Client: Deleted blocks of "<<num
						<<" unused sectors"<<std::endl;*/
				dstream<<DTIME<<"Client: Deleted "<<num
						<<" unused sectors"<<std::endl;
				
				/*
					Send info to server
				*/

				// Env is locked so con can be locked.
				//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
				
				core::list<v3s16>::Iterator i = deleted_blocks.begin();
				core::list<v3s16> sendlist;
				for(;;)
				{
					if(sendlist.size() == 255 || i == deleted_blocks.end())
					{
						if(sendlist.size() == 0)
							break;
						/*
							[0] u16 command
							[2] u8 count
							[3] v3s16 pos_0
							[3+6] v3s16 pos_1
							...
						*/
						u32 replysize = 2+1+6*sendlist.size();
						SharedBuffer<u8> reply(replysize);
						writeU16(&reply[0], TOSERVER_DELETEDBLOCKS);
						reply[2] = sendlist.size();
						u32 k = 0;
						for(core::list<v3s16>::Iterator
								j = sendlist.begin();
								j != sendlist.end(); j++)
						{
							writeV3S16(&reply[2+1+6*k], *j);
							k++;
						}
						m_con.Send(PEER_ID_SERVER, 1, reply, true);

						if(i == deleted_blocks.end())
							break;

						sendlist.clear();
					}

					sendlist.push_back(*i);
					i++;
				}
			}
		}
	}

	bool connected = connectedAndInitialized();

	if(connected == false)
	{
		float &counter = m_connection_reinit_timer;
		counter -= dtime;
		if(counter <= 0.0)
		{
			counter = 2.0;

			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			
			Player *myplayer = m_env.getLocalPlayer();
			assert(myplayer != NULL);
	
			// Send TOSERVER_INIT
			// [0] u16 TOSERVER_INIT
			// [2] u8 SER_FMT_VER_HIGHEST
			// [3] u8[20] player_name
			// [23] u8[28] password
			SharedBuffer<u8> data(2+1+PLAYERNAME_SIZE+PASSWORD_SIZE);
			writeU16(&data[0], TOSERVER_INIT);
			writeU8(&data[2], SER_FMT_VER_HIGHEST);

			memset((char*)&data[3], 0, PLAYERNAME_SIZE);
			snprintf((char*)&data[3], PLAYERNAME_SIZE, "%s", myplayer->getName());

			/*dstream<<"Client: password hash is \""<<m_password<<"\""
					<<std::endl;*/

			memset((char*)&data[23], 0, PASSWORD_SIZE);
			snprintf((char*)&data[23], PASSWORD_SIZE, "%s", m_password.c_str());

			// Send as unreliable
			Send(0, data, false);
		}

		// Not connected, return
		return;
	}

	/*
		Do stuff if connected
	*/
	
	/*
		Handle environment
	*/
	{
		// 0ms
		//JMutexAutoLock lock(m_env_mutex); //bulk comment-out

		// Control local player (0ms)
		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);
		player->applyControl(dtime);

		//TimeTaker envtimer("env step", m_device);
		// Step environment
		m_env.step(dtime);

		// Step active blocks
		for(core::map<v3s16, bool>::Iterator
				i = m_active_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			v3s16 p = i.getNode()->getKey();

			MapBlock *block = NULL;
			try
			{
				block = m_env.getMap().getBlockNoCreate(p);
				block->stepObjects(dtime, false, m_env.getDayNightRatio());
			}
			catch(InvalidPositionException &e)
			{
			}
		}

		/*
			Get events
		*/
		for(;;)
		{
			ClientEnvEvent event = m_env.getClientEvent();
			if(event.type == CEE_NONE)
			{
				break;
			}
			else if(event.type == CEE_PLAYER_DAMAGE)
			{
				if(m_ignore_damage_timer <= 0)
				{
					u8 damage = event.player_damage.amount;
					sendDamage(damage);

					// Add to ClientEvent queue
					ClientEvent event;
					event.type = CE_PLAYER_DAMAGE;
					event.player_damage.amount = damage;
					m_client_event_queue.push_back(event);
				}
			}
		}
	}
	
	/*
		Print some info
	*/
	{
		float &counter = m_avg_rtt_timer;
		counter += dtime;
		if(counter >= 10)
		{
			counter = 0.0;
			//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
			// connectedAndInitialized() is true, peer exists.
			con::Peer *peer = m_con.GetPeer(PEER_ID_SERVER);
			dstream<<DTIME<<"Client: avg_rtt="<<peer->avg_rtt<<std::endl;
		}
	}

	/*
		Send player position to server
	*/
	{
		float &counter = m_playerpos_send_timer;
		counter += dtime;
		if(counter >= 0.2)
		{
			counter = 0.0;
			sendPlayerPos();
		}
	}

	/*
		Replace updated meshes
	*/
	{
		//JMutexAutoLock lock(m_env_mutex); //bulk comment-out

		//TimeTaker timer("** Processing mesh update result queue");
		// 0ms
		
		/*dstream<<"Mesh update result queue size is "
				<<m_mesh_update_thread.m_queue_out.size()
				<<std::endl;*/

		while(m_mesh_update_thread.m_queue_out.size() > 0)
		{
			MeshUpdateResult r = m_mesh_update_thread.m_queue_out.pop_front();
			MapBlock *block = m_env.getMap().getBlockNoCreateNoEx(r.p);
			if(block)
			{
				block->replaceMesh(r.mesh);
			}
			if(r.ack_block_to_server)
			{
				/*dstream<<"Client: ACK block ("<<r.p.X<<","<<r.p.Y
						<<","<<r.p.Z<<")"<<std::endl;*/
				/*
					Acknowledge block
				*/
				/*
					[0] u16 command
					[2] u8 count
					[3] v3s16 pos_0
					[3+6] v3s16 pos_1
					...
				*/
				u32 replysize = 2+1+6;
				SharedBuffer<u8> reply(replysize);
				writeU16(&reply[0], TOSERVER_GOTBLOCKS);
				reply[2] = 1;
				writeV3S16(&reply[3], r.p);
				// Send as reliable
				m_con.Send(PEER_ID_SERVER, 1, reply, true);
			}
		}
	}
}

// Virtual methods from con::PeerHandler
void Client::peerAdded(con::Peer *peer)
{
	derr_client<<"Client::peerAdded(): peer->id="
			<<peer->id<<std::endl;
}
void Client::deletingPeer(con::Peer *peer, bool timeout)
{
	derr_client<<"Client::deletingPeer(): "
			"Server Peer is getting deleted "
			<<"(timeout="<<timeout<<")"<<std::endl;
}

void Client::ReceiveAll()
{
	DSTACK(__FUNCTION_NAME);
	for(;;)
	{
		try{
			Receive();
		}
		catch(con::NoIncomingDataException &e)
		{
			break;
		}
		catch(con::InvalidIncomingDataException &e)
		{
			dout_client<<DTIME<<"Client::ReceiveAll(): "
					"InvalidIncomingDataException: what()="
					<<e.what()<<std::endl;
		}
	}
}

void Client::Receive()
{
	DSTACK(__FUNCTION_NAME);
	u32 data_maxsize = 200000;
	Buffer<u8> data(data_maxsize);
	u16 sender_peer_id;
	u32 datasize;
	{
		//TimeTaker t1("con mutex and receive", m_device);
		//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
		datasize = m_con.Receive(sender_peer_id, *data, data_maxsize);
	}
	//TimeTaker t1("ProcessData", m_device);
	ProcessData(*data, datasize, sender_peer_id);
}

/*
	sender_peer_id given to this shall be quaranteed to be a valid peer
*/
void Client::ProcessData(u8 *data, u32 datasize, u16 sender_peer_id)
{
	DSTACK(__FUNCTION_NAME);

	// Ignore packets that don't even fit a command
	if(datasize < 2)
	{
		m_packetcounter.add(60000);
		return;
	}

	ToClientCommand command = (ToClientCommand)readU16(&data[0]);

	//dstream<<"Client: received command="<<command<<std::endl;
	m_packetcounter.add((u16)command);
	
	/*
		If this check is removed, be sure to change the queue
		system to know the ids
	*/
	if(sender_peer_id != PEER_ID_SERVER)
	{
		dout_client<<DTIME<<"Client::ProcessData(): Discarding data not "
				"coming from server: peer_id="<<sender_peer_id
				<<std::endl;
		return;
	}

	con::Peer *peer;
	{
		//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
		// All data is coming from the server
		// PeerNotFoundException is handled by caller.
		peer = m_con.GetPeer(PEER_ID_SERVER);
	}

	u8 ser_version = m_server_ser_ver;

	//dstream<<"Client received command="<<(int)command<<std::endl;

	if(command == TOCLIENT_INIT)
	{
		if(datasize < 3)
			return;

		u8 deployed = data[2];

		dout_client<<DTIME<<"Client: TOCLIENT_INIT received with "
				"deployed="<<((int)deployed&0xff)<<std::endl;

		if(deployed < SER_FMT_VER_LOWEST
				|| deployed > SER_FMT_VER_HIGHEST)
		{
			derr_client<<DTIME<<"Client: TOCLIENT_INIT: Server sent "
					<<"unsupported ser_fmt_ver"<<std::endl;
			return;
		}
		
		m_server_ser_ver = deployed;

		// Get player position
		v3s16 playerpos_s16(0, BS*2+BS*20, 0);
		if(datasize >= 2+1+6)
			playerpos_s16 = readV3S16(&data[2+1]);
		v3f playerpos_f = intToFloat(playerpos_s16, BS) - v3f(0, BS/2, 0);

		{ //envlock
			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			
			// Set player position
			Player *player = m_env.getLocalPlayer();
			assert(player != NULL);
			player->setPosition(playerpos_f);
		}
		
		if(datasize >= 2+1+6+8)
		{
			// Get map seed
			m_map_seed = readU64(&data[2+1+6]);
			dstream<<"Client: received map seed: "<<m_map_seed<<std::endl;
		}
		
		// Reply to server
		u32 replysize = 2;
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOSERVER_INIT2);
		// Send as reliable
		m_con.Send(PEER_ID_SERVER, 1, reply, true);

		return;
	}

	if(command == TOCLIENT_ACCESS_DENIED)
	{
		// The server didn't like our password. Note, this needs
		// to be processed even if the serialisation format has
		// not been agreed yet, the same as TOCLIENT_INIT.
		m_access_denied = true;
		m_access_denied_reason = L"Unknown";
		if(datasize >= 4)
		{
			std::string datastring((char*)&data[2], datasize-2);
			std::istringstream is(datastring, std::ios_base::binary);
			m_access_denied_reason = deSerializeWideString(is);
		}
		return;
	}

	if(ser_version == SER_FMT_VER_INVALID)
	{
		dout_client<<DTIME<<"WARNING: Client: Server serialization"
				" format invalid or not initialized."
				" Skipping incoming command="<<command<<std::endl;
		return;
	}
	
	// Just here to avoid putting the two if's together when
	// making some copypasta
	{}

	if(command == TOCLIENT_REMOVENODE)
	{
		if(datasize < 8)
			return;
		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);
		
		//TimeTaker t1("TOCLIENT_REMOVENODE");
		
		// This will clear the cracking animation after digging
		((ClientMap&)m_env.getMap()).clearTempMod(p);

		removeNode(p);
	}
	else if(command == TOCLIENT_ADDNODE)
	{
		if(datasize < 8 + MapNode::serializedLength(ser_version))
			return;

		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);
		
		//TimeTaker t1("TOCLIENT_ADDNODE");

		MapNode n;
		n.deSerialize(&data[8], ser_version);
		
		addNode(p, n);
	}
	else if(command == TOCLIENT_BLOCKDATA)
	{
		// Ignore too small packet
		if(datasize < 8)
			return;
			
		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);
		
		/*dout_client<<DTIME<<"Client: Thread: BLOCKDATA for ("
				<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		/*dstream<<DTIME<<"Client: Thread: BLOCKDATA for ("
				<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		
		std::string datastring((char*)&data[8], datasize-8);
		std::istringstream istr(datastring, std::ios_base::binary);
		
		MapSector *sector;
		MapBlock *block;
		
		v2s16 p2d(p.X, p.Z);
		sector = m_env.getMap().emergeSector(p2d);
		
		assert(sector->getPos() == p2d);

		//TimeTaker timer("MapBlock deSerialize");
		// 0ms
		
		block = sector->getBlockNoCreateNoEx(p.Y);
		if(block)
		{
			/*
				Update an existing block
			*/
			//dstream<<"Updating"<<std::endl;
			block->deSerialize(istr, ser_version);
		}
		else
		{
			/*
				Create a new block
			*/
			//dstream<<"Creating new"<<std::endl;
			block = new MapBlock(&m_env.getMap(), p);
			block->deSerialize(istr, ser_version);
			sector->insertBlock(block);

			//DEBUG
			/*NodeMod mod;
			mod.type = NODEMOD_CHANGECONTENT;
			mod.param = CONTENT_MESE;
			block->setTempMod(v3s16(8,10,8), mod);
			block->setTempMod(v3s16(8,9,8), mod);
			block->setTempMod(v3s16(8,8,8), mod);
			block->setTempMod(v3s16(8,7,8), mod);
			block->setTempMod(v3s16(8,6,8), mod);*/
		}

#if 0
		/*
			Acknowledge block
		*/
		/*
			[0] u16 command
			[2] u8 count
			[3] v3s16 pos_0
			[3+6] v3s16 pos_1
			...
		*/
		u32 replysize = 2+1+6;
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOSERVER_GOTBLOCKS);
		reply[2] = 1;
		writeV3S16(&reply[3], p);
		// Send as reliable
		m_con.Send(PEER_ID_SERVER, 1, reply, true);
#endif

		/*
			Update Mesh of this block and blocks at x-, y- and z-.
			Environment should not be locked as it interlocks with the
			main thread, from which is will want to retrieve textures.
		*/

		//m_env.getClientMap().updateMeshes(block->getPos(), getDayNightRatio());
		
		/*
			Add it to mesh update queue and set it to be acknowledged after update.
		*/
		//std::cerr<<"Adding mesh update task for received block"<<std::endl;
		addUpdateMeshTaskWithEdge(p, true);
	}
	else if(command == TOCLIENT_PLAYERPOS)
	{
		dstream<<"WARNING: Received deprecated TOCLIENT_PLAYERPOS"
				<<std::endl;
		/*u16 our_peer_id;
		{
			//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
			our_peer_id = m_con.GetPeerID();
		}
		// Cancel if we don't have a peer id
		if(our_peer_id == PEER_ID_INEXISTENT){
			dout_client<<DTIME<<"TOCLIENT_PLAYERPOS cancelled: "
					"we have no peer id"
					<<std::endl;
			return;
		}*/

		{ //envlock
			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			
			u32 player_size = 2+12+12+4+4;
				
			u32 player_count = (datasize-2) / player_size;
			u32 start = 2;
			for(u32 i=0; i<player_count; i++)
			{
				u16 peer_id = readU16(&data[start]);

				Player *player = m_env.getPlayer(peer_id);

				// Skip if player doesn't exist
				if(player == NULL)
				{
					start += player_size;
					continue;
				}

				// Skip if player is local player
				if(player->isLocal())
				{
					start += player_size;
					continue;
				}

				v3s32 ps = readV3S32(&data[start+2]);
				v3s32 ss = readV3S32(&data[start+2+12]);
				s32 pitch_i = readS32(&data[start+2+12+12]);
				s32 yaw_i = readS32(&data[start+2+12+12+4]);
				/*dstream<<"Client: got "
						<<"pitch_i="<<pitch_i
						<<" yaw_i="<<yaw_i<<std::endl;*/
				f32 pitch = (f32)pitch_i / 100.0;
				f32 yaw = (f32)yaw_i / 100.0;
				v3f position((f32)ps.X/100., (f32)ps.Y/100., (f32)ps.Z/100.);
				v3f speed((f32)ss.X/100., (f32)ss.Y/100., (f32)ss.Z/100.);
				player->setPosition(position);
				player->setSpeed(speed);
				player->setPitch(pitch);
				player->setYaw(yaw);

				/*dstream<<"Client: player "<<peer_id
						<<" pitch="<<pitch
						<<" yaw="<<yaw<<std::endl;*/

				start += player_size;
			}
		} //envlock
	}
	else if(command == TOCLIENT_PLAYERINFO)
	{
		u16 our_peer_id;
		{
			//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
			our_peer_id = m_con.GetPeerID();
		}
		// Cancel if we don't have a peer id
		if(our_peer_id == PEER_ID_INEXISTENT){
			dout_client<<DTIME<<"TOCLIENT_PLAYERINFO cancelled: "
					"we have no peer id"
					<<std::endl;
			return;
		}
		
		//dstream<<DTIME<<"Client: Server reports players:"<<std::endl;

		{ //envlock
			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			
			u32 item_size = 2+PLAYERNAME_SIZE;
			u32 player_count = (datasize-2) / item_size;
			u32 start = 2;
			// peer_ids
			core::list<u16> players_alive;
			for(u32 i=0; i<player_count; i++)
			{
				// Make sure the name ends in '\0'
				data[start+2+20-1] = 0;

				u16 peer_id = readU16(&data[start]);

				players_alive.push_back(peer_id);
				
				/*dstream<<DTIME<<"peer_id="<<peer_id
						<<" name="<<((char*)&data[start+2])<<std::endl;*/

				// Don't update the info of the local player
				if(peer_id == our_peer_id)
				{
					start += item_size;
					continue;
				}

				Player *player = m_env.getPlayer(peer_id);

				// Create a player if it doesn't exist
				if(player == NULL)
				{
					player = new RemotePlayer(
							m_device->getSceneManager()->getRootSceneNode(),
							m_device,
							-1);
					player->peer_id = peer_id;
					m_env.addPlayer(player);
					dout_client<<DTIME<<"Client: Adding new player "
							<<peer_id<<std::endl;
				}
				
				player->updateName((char*)&data[start+2]);

				start += item_size;
			}
			
			/*
				Remove those players from the environment that
				weren't listed by the server.
			*/
			//dstream<<DTIME<<"Removing dead players"<<std::endl;
			core::list<Player*> players = m_env.getPlayers();
			core::list<Player*>::Iterator ip;
			for(ip=players.begin(); ip!=players.end(); ip++)
			{
				// Ingore local player
				if((*ip)->isLocal())
					continue;
				
				// Warn about a special case
				if((*ip)->peer_id == 0)
				{
					dstream<<DTIME<<"WARNING: Client: Removing "
							"dead player with id=0"<<std::endl;
				}

				bool is_alive = false;
				core::list<u16>::Iterator i;
				for(i=players_alive.begin(); i!=players_alive.end(); i++)
				{
					if((*ip)->peer_id == *i)
					{
						is_alive = true;
						break;
					}
				}
				/*dstream<<DTIME<<"peer_id="<<((*ip)->peer_id)
						<<" is_alive="<<is_alive<<std::endl;*/
				if(is_alive)
					continue;
				dstream<<DTIME<<"Removing dead player "<<(*ip)->peer_id
						<<std::endl;
				m_env.removePlayer((*ip)->peer_id);
			}
		} //envlock
	}
	else if(command == TOCLIENT_SECTORMETA)
	{
		dstream<<"Client received DEPRECATED TOCLIENT_SECTORMETA"<<std::endl;
#if 0
		/*
			[0] u16 command
			[2] u8 sector count
			[3...] v2s16 pos + sector metadata
		*/
		if(datasize < 3)
			return;

		//dstream<<"Client received TOCLIENT_SECTORMETA"<<std::endl;

		{ //envlock
			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			
			std::string datastring((char*)&data[2], datasize-2);
			std::istringstream is(datastring, std::ios_base::binary);

			u8 buf[4];

			is.read((char*)buf, 1);
			u16 sector_count = readU8(buf);
			
			//dstream<<"sector_count="<<sector_count<<std::endl;

			for(u16 i=0; i<sector_count; i++)
			{
				// Read position
				is.read((char*)buf, 4);
				v2s16 pos = readV2S16(buf);
				/*dstream<<"Client: deserializing sector at "
						<<"("<<pos.X<<","<<pos.Y<<")"<<std::endl;*/
				// Create sector
				assert(m_env.getMap().mapType() == MAPTYPE_CLIENT);
				((ClientMap&)m_env.getMap()).deSerializeSector(pos, is);
			}
		} //envlock
#endif
	}
	else if(command == TOCLIENT_INVENTORY)
	{
		if(datasize < 3)
			return;

		//TimeTaker t1("Parsing TOCLIENT_INVENTORY", m_device);

		{ //envlock
			//TimeTaker t2("mutex locking", m_device);
			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			//t2.stop();
			
			//TimeTaker t3("istringstream init", m_device);
			std::string datastring((char*)&data[2], datasize-2);
			std::istringstream is(datastring, std::ios_base::binary);
			//t3.stop();
			
			//m_env.printPlayers(dstream);

			//TimeTaker t4("player get", m_device);
			Player *player = m_env.getLocalPlayer();
			assert(player != NULL);
			//t4.stop();

			//TimeTaker t1("inventory.deSerialize()", m_device);
			player->inventory.deSerialize(is);
			//t1.stop();

			m_inventory_updated = true;

			//dstream<<"Client got player inventory:"<<std::endl;
			//player->inventory.print(dstream);
		}
	}
	//DEBUG
	else if(command == TOCLIENT_OBJECTDATA)
	//else if(0)
	{
		// Strip command word and create a stringstream
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		{ //envlock
		
		//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out

		u8 buf[12];

		/*
			Read players
		*/

		is.read((char*)buf, 2);
		u16 playercount = readU16(buf);
		
		for(u16 i=0; i<playercount; i++)
		{
			is.read((char*)buf, 2);
			u16 peer_id = readU16(buf);
			is.read((char*)buf, 12);
			v3s32 p_i = readV3S32(buf);
			is.read((char*)buf, 12);
			v3s32 s_i = readV3S32(buf);
			is.read((char*)buf, 4);
			s32 pitch_i = readS32(buf);
			is.read((char*)buf, 4);
			s32 yaw_i = readS32(buf);
			
			Player *player = m_env.getPlayer(peer_id);

			// Skip if player doesn't exist
			if(player == NULL)
			{
				continue;
			}

			// Skip if player is local player
			if(player->isLocal())
			{
				continue;
			}
	
			f32 pitch = (f32)pitch_i / 100.0;
			f32 yaw = (f32)yaw_i / 100.0;
			v3f position((f32)p_i.X/100., (f32)p_i.Y/100., (f32)p_i.Z/100.);
			v3f speed((f32)s_i.X/100., (f32)s_i.Y/100., (f32)s_i.Z/100.);
			
			player->setPosition(position);
			player->setSpeed(speed);
			player->setPitch(pitch);
			player->setYaw(yaw);
		}

		/*
			Read block objects
		*/

		// Read active block count
		is.read((char*)buf, 2);
		u16 blockcount = readU16(buf);
		
		// Initialize delete queue with all active blocks
		core::map<v3s16, bool> abs_to_delete;
		for(core::map<v3s16, bool>::Iterator
				i = m_active_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			/*dstream<<"adding "
					<<"("<<p.x<<","<<p.y<<","<<p.z<<") "
					<<" to abs_to_delete"
					<<std::endl;*/
			abs_to_delete.insert(p, true);
		}

		/*dstream<<"Initial delete queue size: "<<abs_to_delete.size()
				<<std::endl;*/
		
		for(u16 i=0; i<blockcount; i++)
		{
			// Read blockpos
			is.read((char*)buf, 6);
			v3s16 p = readV3S16(buf);
			// Get block from somewhere
			MapBlock *block = NULL;
			try{
				block = m_env.getMap().getBlockNoCreate(p);
			}
			catch(InvalidPositionException &e)
			{
				//TODO: Create a dummy block?
			}
			if(block == NULL)
			{
				dstream<<"WARNING: "
						<<"Could not get block at blockpos "
						<<"("<<p.X<<","<<p.Y<<","<<p.Z<<") "
						<<"in TOCLIENT_OBJECTDATA. Ignoring "
						<<"following block object data."
						<<std::endl;
				return;
			}

			/*dstream<<"Client updating objects for block "
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<std::endl;*/

			// Insert to active block list
			m_active_blocks.insert(p, true);

			// Remove from deletion queue
			if(abs_to_delete.find(p) != NULL)
				abs_to_delete.remove(p);

			/*
				Update objects of block
				
				NOTE: Be sure this is done in the main thread.
			*/
			block->updateObjects(is, m_server_ser_ver,
					m_device->getSceneManager(), m_env.getDayNightRatio());
		}
		
		/*dstream<<"Final delete queue size: "<<abs_to_delete.size()
				<<std::endl;*/
		
		// Delete objects of blocks in delete queue
		for(core::map<v3s16, bool>::Iterator
				i = abs_to_delete.getIterator();
				i.atEnd() == false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			try
			{
				MapBlock *block = m_env.getMap().getBlockNoCreate(p);
				
				// Clear objects
				block->clearObjects();
				// Remove from active blocks list
				m_active_blocks.remove(p);
			}
			catch(InvalidPositionException &e)
			{
				dstream<<"WARNAING: Client: "
						<<"Couldn't clear objects of active->inactive"
						<<" block "
						<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
						<<" because block was not found"
						<<std::endl;
				// Ignore
			}
		}

		} //envlock
	}
	else if(command == TOCLIENT_TIME_OF_DAY)
	{
		if(datasize < 4)
			return;
		
		u16 time_of_day = readU16(&data[2]);
		time_of_day = time_of_day % 24000;
		//dstream<<"Client: time_of_day="<<time_of_day<<std::endl;
		
		/*
			time_of_day:
			0 = midnight
			12000 = midday
		*/
		{
			m_env.setTimeOfDay(time_of_day);

			u32 dr = m_env.getDayNightRatio();

			dstream<<"Client: time_of_day="<<time_of_day
					<<", dr="<<dr
					<<std::endl;
		}

	}
	else if(command == TOCLIENT_CHAT_MESSAGE)
	{
		/*
			u16 command
			u16 length
			wstring message
		*/
		u8 buf[6];
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		// Read stuff
		is.read((char*)buf, 2);
		u16 len = readU16(buf);
		
		std::wstring message;
		for(u16 i=0; i<len; i++)
		{
			is.read((char*)buf, 2);
			message += (wchar_t)readU16(buf);
		}

		/*dstream<<"Client received chat message: "
				<<wide_to_narrow(message)<<std::endl;*/
		
		m_chat_queue.push_back(message);
	}
	else if(command == TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD)
	{
		//if(g_settings.getBool("enable_experimental"))
		{
			/*
				u16 command
				u16 count of removed objects
				for all removed objects {
					u16 id
				}
				u16 count of added objects
				for all added objects {
					u16 id
					u8 type
					u16 initialization data length
					string initialization data
				}
			*/

			char buf[6];
			// Get all data except the command number
			std::string datastring((char*)&data[2], datasize-2);
			// Throw them in an istringstream
			std::istringstream is(datastring, std::ios_base::binary);

			// Read stuff
			
			// Read removed objects
			is.read(buf, 2);
			u16 removed_count = readU16((u8*)buf);
			for(u16 i=0; i<removed_count; i++)
			{
				is.read(buf, 2);
				u16 id = readU16((u8*)buf);
				// Remove it
				{
					//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
					m_env.removeActiveObject(id);
				}
			}
			
			// Read added objects
			is.read(buf, 2);
			u16 added_count = readU16((u8*)buf);
			for(u16 i=0; i<added_count; i++)
			{
				is.read(buf, 2);
				u16 id = readU16((u8*)buf);
				is.read(buf, 1);
				u8 type = readU8((u8*)buf);
				std::string data = deSerializeLongString(is);
				// Add it
				{
					//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
					m_env.addActiveObject(id, type, data);
				}
			}
		}
	}
	else if(command == TOCLIENT_ACTIVE_OBJECT_MESSAGES)
	{
		//if(g_settings.getBool("enable_experimental"))
		{
			/*
				u16 command
				for all objects
				{
					u16 id
					u16 message length
					string message
				}
			*/
			char buf[6];
			// Get all data except the command number
			std::string datastring((char*)&data[2], datasize-2);
			// Throw them in an istringstream
			std::istringstream is(datastring, std::ios_base::binary);
			
			while(is.eof() == false)
			{
				// Read stuff
				is.read(buf, 2);
				u16 id = readU16((u8*)buf);
				if(is.eof())
					break;
				is.read(buf, 2);
				u16 message_size = readU16((u8*)buf);
				std::string message;
				message.reserve(message_size);
				for(u16 i=0; i<message_size; i++)
				{
					is.read(buf, 1);
					message.append(buf, 1);
				}
				// Pass on to the environment
				{
					//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
					m_env.processActiveObjectMessage(id, message);
				}
			}
		}
	}
	else if(command == TOCLIENT_HP)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		Player *player = m_env.getLocalPlayer();
		assert(player != NULL);
		u8 hp = readU8(is);
		player->hp = hp;
	}
	else if(command == TOCLIENT_MOVE_PLAYER)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		Player *player = m_env.getLocalPlayer();
		assert(player != NULL);
		v3f pos = readV3F1000(is);
		f32 pitch = readF1000(is);
		f32 yaw = readF1000(is);
		player->setPosition(pos);
		/*player->setPitch(pitch);
		player->setYaw(yaw);*/

		dstream<<"Client got TOCLIENT_MOVE_PLAYER"
				<<" pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"
				<<" pitch="<<pitch
				<<" yaw="<<yaw
				<<std::endl;

		/*
			Add to ClientEvent queue.
			This has to be sent to the main program because otherwise
			it would just force the pitch and yaw values to whatever
			the camera points to.
		*/
		ClientEvent event;
		event.type = CE_PLAYER_FORCE_MOVE;
		event.player_force_move.pitch = pitch;
		event.player_force_move.yaw = yaw;
		m_client_event_queue.push_back(event);

		// Ignore damage for a few seconds, so that the player doesn't
		// get damage from falling on ground
		m_ignore_damage_timer = 3.0;
	}
	else
	{
		dout_client<<DTIME<<"WARNING: Client: Ignoring unknown command "
				<<command<<std::endl;
	}
}

void Client::Send(u16 channelnum, SharedBuffer<u8> data, bool reliable)
{
	//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
	m_con.Send(PEER_ID_SERVER, channelnum, data, reliable);
}

void Client::groundAction(u8 action, v3s16 nodepos_undersurface,
		v3s16 nodepos_oversurface, u16 item)
{
	if(connectedAndInitialized() == false){
		dout_client<<DTIME<<"Client::groundAction() "
				"cancelled (not connected)"
				<<std::endl;
		return;
	}
	
	/*
		length: 17
		[0] u16 command
		[2] u8 action
		[3] v3s16 nodepos_undersurface
		[9] v3s16 nodepos_abovesurface
		[15] u16 item
		actions:
		0: start digging
		1: place block
		2: stop digging (all parameters ignored)
		3: digging completed
	*/
	u8 datasize = 2 + 1 + 6 + 6 + 2;
	SharedBuffer<u8> data(datasize);
	writeU16(&data[0], TOSERVER_GROUND_ACTION);
	writeU8(&data[2], action);
	writeV3S16(&data[3], nodepos_undersurface);
	writeV3S16(&data[9], nodepos_oversurface);
	writeU16(&data[15], item);
	Send(0, data, true);
}

void Client::clickObject(u8 button, v3s16 blockpos, s16 id, u16 item)
{
	if(connectedAndInitialized() == false){
		dout_client<<DTIME<<"Client::clickObject() "
				"cancelled (not connected)"
				<<std::endl;
		return;
	}
	
	/*
		[0] u16 command=TOSERVER_CLICK_OBJECT
		[2] u8 button (0=left, 1=right)
		[3] v3s16 block
		[9] s16 id
		[11] u16 item
	*/
	u8 datasize = 2 + 1 + 6 + 2 + 2;
	SharedBuffer<u8> data(datasize);
	writeU16(&data[0], TOSERVER_CLICK_OBJECT);
	writeU8(&data[2], button);
	writeV3S16(&data[3], blockpos);
	writeS16(&data[9], id);
	writeU16(&data[11], item);
	Send(0, data, true);
}

void Client::clickActiveObject(u8 button, u16 id, u16 item)
{
	if(connectedAndInitialized() == false){
		dout_client<<DTIME<<"Client::clickActiveObject() "
				"cancelled (not connected)"
				<<std::endl;
		return;
	}
	
	/*
		length: 7
		[0] u16 command
		[2] u8 button (0=left, 1=right)
		[3] u16 id
		[5] u16 item
	*/
	u8 datasize = 2 + 1 + 6 + 2 + 2;
	SharedBuffer<u8> data(datasize);
	writeU16(&data[0], TOSERVER_CLICK_ACTIVEOBJECT);
	writeU8(&data[2], button);
	writeU16(&data[3], id);
	writeU16(&data[5], item);
	Send(0, data, true);
}

void Client::sendSignText(v3s16 blockpos, s16 id, std::string text)
{
	/*
		u16 command
		v3s16 blockpos
		s16 id
		u16 textlen
		textdata
	*/
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOSERVER_SIGNTEXT);
	os.write((char*)buf, 2);
	
	// Write blockpos
	writeV3S16(buf, blockpos);
	os.write((char*)buf, 6);

	// Write id
	writeS16(buf, id);
	os.write((char*)buf, 2);

	u16 textlen = text.size();
	// Write text length
	writeS16(buf, textlen);
	os.write((char*)buf, 2);

	// Write text
	os.write((char*)text.c_str(), textlen);
	
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}
	
void Client::sendSignNodeText(v3s16 p, std::string text)
{
	/*
		u16 command
		v3s16 p
		u16 textlen
		textdata
	*/
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOSERVER_SIGNNODETEXT);
	os.write((char*)buf, 2);
	
	// Write p
	writeV3S16(buf, p);
	os.write((char*)buf, 6);

	u16 textlen = text.size();
	// Write text length
	writeS16(buf, textlen);
	os.write((char*)buf, 2);

	// Write text
	os.write((char*)text.c_str(), textlen);
	
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}
	
void Client::sendInventoryAction(InventoryAction *a)
{
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOSERVER_INVENTORY_ACTION);
	os.write((char*)buf, 2);

	a->serialize(os);
	
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendChatMessage(const std::wstring &message)
{
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOSERVER_CHAT_MESSAGE);
	os.write((char*)buf, 2);
	
	// Write length
	writeU16(buf, message.size());
	os.write((char*)buf, 2);
	
	// Write string
	for(u32 i=0; i<message.size(); i++)
	{
		u16 w = message[i];
		writeU16(buf, w);
		os.write((char*)buf, 2);
	}
	
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendChangePassword(const std::wstring oldpassword,
		const std::wstring newpassword)
{
	Player *player = m_env.getLocalPlayer();
	if(player == NULL)
		return;

	std::string playername = player->getName();
	std::string oldpwd = translatePassword(playername, oldpassword);
	std::string newpwd = translatePassword(playername, newpassword);

	std::ostringstream os(std::ios_base::binary);
	u8 buf[2+PASSWORD_SIZE*2];
	/*
		[0] u16 TOSERVER_PASSWORD
		[2] u8[28] old password
		[30] u8[28] new password
	*/

	writeU16(buf, TOSERVER_PASSWORD);
	for(u32 i=0;i<PASSWORD_SIZE-1;i++)
	{
		buf[2+i] = i<oldpwd.length()?oldpwd[i]:0;
		buf[30+i] = i<newpwd.length()?newpwd[i]:0;
	}
	buf[2+PASSWORD_SIZE-1] = 0;
	buf[30+PASSWORD_SIZE-1] = 0;
	os.write((char*)buf, 2+PASSWORD_SIZE*2);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}


void Client::sendDamage(u8 damage)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_DAMAGE);
	writeU8(os, damage);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendPlayerPos()
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	
	Player *myplayer = m_env.getLocalPlayer();
	if(myplayer == NULL)
		return;
	
	u16 our_peer_id;
	{
		//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
		our_peer_id = m_con.GetPeerID();
	}
	
	// Set peer id if not set already
	if(myplayer->peer_id == PEER_ID_INEXISTENT)
		myplayer->peer_id = our_peer_id;
	// Check that an existing peer_id is the same as the connection's
	assert(myplayer->peer_id == our_peer_id);
	
	v3f pf = myplayer->getPosition();
	v3s32 position(pf.X*100, pf.Y*100, pf.Z*100);
	v3f sf = myplayer->getSpeed();
	v3s32 speed(sf.X*100, sf.Y*100, sf.Z*100);
	s32 pitch = myplayer->getPitch() * 100;
	s32 yaw = myplayer->getYaw() * 100;

	/*
		Format:
		[0] u16 command
		[2] v3s32 position*100
		[2+12] v3s32 speed*100
		[2+12+12] s32 pitch*100
		[2+12+12+4] s32 yaw*100
	*/

	SharedBuffer<u8> data(2+12+12+4+4);
	writeU16(&data[0], TOSERVER_PLAYERPOS);
	writeV3S32(&data[2], position);
	writeV3S32(&data[2+12], speed);
	writeS32(&data[2+12+12], pitch);
	writeS32(&data[2+12+12+4], yaw);

	// Send as unreliable
	Send(0, data, false);
}

void Client::removeNode(v3s16 p)
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	
	core::map<v3s16, MapBlock*> modified_blocks;

	try
	{
		//TimeTaker t("removeNodeAndUpdate", m_device);
		m_env.getMap().removeNodeAndUpdate(p, modified_blocks);
	}
	catch(InvalidPositionException &e)
	{
	}
	
	for(core::map<v3s16, MapBlock * >::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		//m_env.getClientMap().updateMeshes(p, m_env.getDayNightRatio());
		addUpdateMeshTaskWithEdge(p);
	}
}

void Client::addNode(v3s16 p, MapNode n)
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out

	TimeTaker timer1("Client::addNode()");

	core::map<v3s16, MapBlock*> modified_blocks;

	try
	{
		//TimeTaker timer3("Client::addNode(): addNodeAndUpdate");
		m_env.getMap().addNodeAndUpdate(p, n, modified_blocks);
	}
	catch(InvalidPositionException &e)
	{}
	
	//TimeTaker timer2("Client::addNode(): updateMeshes");

	for(core::map<v3s16, MapBlock * >::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		//m_env.getClientMap().updateMeshes(p, m_env.getDayNightRatio());
		addUpdateMeshTaskWithEdge(p);
	}
}
	
void Client::updateCamera(v3f pos, v3f dir)
{
	m_env.getClientMap().updateCamera(pos, dir);
	camera_position = pos;
	camera_direction = dir;
}

MapNode Client::getNode(v3s16 p)
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	return m_env.getMap().getNode(p);
}

NodeMetadata* Client::getNodeMetadata(v3s16 p)
{
	return m_env.getMap().getNodeMetadata(p);
}

v3f Client::getPlayerPosition()
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player != NULL);
	return player->getPosition();
}

void Client::setPlayerControl(PlayerControl &control)
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player != NULL);
	player->control = control;
}

// Returns true if the inventory of the local player has been
// updated from the server. If it is true, it is set to false.
bool Client::getLocalInventoryUpdated()
{
	// m_inventory_updated is behind envlock
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	bool updated = m_inventory_updated;
	m_inventory_updated = false;
	return updated;
}

// Copies the inventory of the local player to parameter
void Client::getLocalInventory(Inventory &dst)
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);
	dst = player->inventory;
}

InventoryContext *Client::getInventoryContext()
{
	return &m_inventory_context;
}

Inventory* Client::getInventory(InventoryContext *c, std::string id)
{
	if(id == "current_player")
	{
		assert(c->current_player);
		return &(c->current_player->inventory);
	}
	
	Strfnd fn(id);
	std::string id0 = fn.next(":");

	if(id0 == "nodemeta")
	{
		v3s16 p;
		p.X = stoi(fn.next(","));
		p.Y = stoi(fn.next(","));
		p.Z = stoi(fn.next(","));
		NodeMetadata* meta = getNodeMetadata(p);
		if(meta)
			return meta->getInventory();
		dstream<<"nodemeta at ("<<p.X<<","<<p.Y<<","<<p.Z<<"): "
				<<"no metadata found"<<std::endl;
		return NULL;
	}

	dstream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
	return NULL;
}
void Client::inventoryAction(InventoryAction *a)
{
	sendInventoryAction(a);
}

MapBlockObject * Client::getSelectedObject(
		f32 max_d,
		v3f from_pos_f_on_map,
		core::line3d<f32> shootline_on_map
	)
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out

	core::array<DistanceSortedObject> objects;

	for(core::map<v3s16, bool>::Iterator
			i = m_active_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();

		MapBlock *block = NULL;
		try
		{
			block = m_env.getMap().getBlockNoCreate(p);
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		// Calculate from_pos relative to block
		v3s16 block_pos_i_on_map = block->getPosRelative();
		v3f block_pos_f_on_map = intToFloat(block_pos_i_on_map, BS);
		v3f from_pos_f_on_block = from_pos_f_on_map - block_pos_f_on_map;

		block->getObjects(from_pos_f_on_block, max_d, objects);
		//block->getPseudoObjects(from_pos_f_on_block, max_d, objects);
	}

	//dstream<<"Collected "<<objects.size()<<" nearby objects"<<std::endl;
	
	// Sort them.
	// After this, the closest object is the first in the array.
	objects.sort();

	for(u32 i=0; i<objects.size(); i++)
	{
		MapBlockObject *obj = objects[i].obj;
		MapBlock *block = obj->getBlock();

		// Calculate shootline relative to block
		v3s16 block_pos_i_on_map = block->getPosRelative();
		v3f block_pos_f_on_map = intToFloat(block_pos_i_on_map, BS);
		core::line3d<f32> shootline_on_block(
				shootline_on_map.start - block_pos_f_on_map,
				shootline_on_map.end - block_pos_f_on_map
		);

		if(obj->isSelected(shootline_on_block))
		{
			//dstream<<"Returning selected object"<<std::endl;
			return obj;
		}
	}

	//dstream<<"No object selected; returning NULL."<<std::endl;
	return NULL;
}

ClientActiveObject * Client::getSelectedActiveObject(
		f32 max_d,
		v3f from_pos_f_on_map,
		core::line3d<f32> shootline_on_map
	)
{
	core::array<DistanceSortedActiveObject> objects;

	m_env.getActiveObjects(from_pos_f_on_map, max_d, objects);

	//dstream<<"Collected "<<objects.size()<<" nearby objects"<<std::endl;
	
	// Sort them.
	// After this, the closest object is the first in the array.
	objects.sort();

	for(u32 i=0; i<objects.size(); i++)
	{
		ClientActiveObject *obj = objects[i].obj;
		
		core::aabbox3d<f32> *selection_box = obj->getSelectionBox();
		if(selection_box == NULL)
			continue;

		v3f pos = obj->getPosition();

		core::aabbox3d<f32> offsetted_box(
				selection_box->MinEdge + pos,
				selection_box->MaxEdge + pos
		);

		if(offsetted_box.intersectsWithLine(shootline_on_map))
		{
			//dstream<<"Returning selected object"<<std::endl;
			return obj;
		}
	}

	//dstream<<"No object selected; returning NULL."<<std::endl;
	return NULL;
}

void Client::printDebugInfo(std::ostream &os)
{
	//JMutexAutoLock lock1(m_fetchblock_mutex);
	/*JMutexAutoLock lock2(m_incoming_queue_mutex);

	os<<"m_incoming_queue.getSize()="<<m_incoming_queue.getSize()
		//<<", m_fetchblock_history.size()="<<m_fetchblock_history.size()
		//<<", m_opt_not_found_history.size()="<<m_opt_not_found_history.size()
		<<std::endl;*/
}
	
u32 Client::getDayNightRatio()
{
	//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
	return m_env.getDayNightRatio();
}

u16 Client::getHP()
{
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);
	return player->hp;
}

void Client::setTempMod(v3s16 p, NodeMod mod)
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

void Client::clearTempMod(v3s16 p)
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

void Client::addUpdateMeshTask(v3s16 p, bool ack_to_server)
{
	/*dstream<<"Client::addUpdateMeshTask(): "
			<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<std::endl;*/

	MapBlock *b = m_env.getMap().getBlockNoCreateNoEx(p);
	if(b == NULL)
		return;
	
	/*
		Create a task to update the mesh of the block
	*/
	
	MeshMakeData *data = new MeshMakeData;
	
	{
		//TimeTaker timer("data fill");
		// Release: ~0ms
		// Debug: 1-6ms, avg=2ms
		data->fill(getDayNightRatio(), b);
	}

	// Debug wait
	//while(m_mesh_update_thread.m_queue_in.size() > 0) sleep_ms(10);
	
	// Add task to queue
	m_mesh_update_thread.m_queue_in.addBlock(p, data, ack_to_server);

	/*dstream<<"Mesh update input queue size is "
			<<m_mesh_update_thread.m_queue_in.size()
			<<std::endl;*/
	
#if 0
	// Temporary test: make mesh directly in here
	{
		//TimeTaker timer("make mesh");
		// 10ms
		scene::SMesh *mesh_new = NULL;
		mesh_new = makeMapBlockMesh(data);
		b->replaceMesh(mesh_new);
		delete data;
	}
#endif

	/*
		Mark mesh as non-expired at this point so that it can already
		be marked as expired again if the data changes
	*/
	b->setMeshExpired(false);
}

void Client::addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server)
{
	/*{
		v3s16 p = blockpos;
		dstream<<"Client::addUpdateMeshTaskWithEdge(): "
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
				<<std::endl;
	}*/

	try{
		v3s16 p = blockpos + v3s16(0,0,0);
		//MapBlock *b = m_env.getMap().getBlockNoCreate(p);
		addUpdateMeshTask(p, ack_to_server);
	}
	catch(InvalidPositionException &e){}
	// Leading edge
	try{
		v3s16 p = blockpos + v3s16(-1,0,0);
		addUpdateMeshTask(p);
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,-1,0);
		addUpdateMeshTask(p);
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,0,-1);
		addUpdateMeshTask(p);
	}
	catch(InvalidPositionException &e){}
}

ClientEvent Client::getClientEvent()
{
	if(m_client_event_queue.size() == 0)
	{
		ClientEvent event;
		event.type = CE_NONE;
		return event;
	}
	return m_client_event_queue.pop_front();
}


