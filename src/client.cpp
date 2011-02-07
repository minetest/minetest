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

#include "client.h"
#include "utility.h"
#include <iostream>
#include "clientserver.h"
#include "jmutexautolock.h"
#include "main.h"
#include <sstream>
#include "porting.h"

void * ClientUpdateThread::Thread()
{
	ThreadStarted();

	DSTACK(__FUNCTION_NAME);
	
	BEGIN_DEBUG_EXCEPTION_HANDLER
	
	while(getRun())
	{
		m_client->asyncStep();

		//m_client->updateSomeExpiredMeshes();

		bool was = m_client->AsyncProcessData();

		if(was == false)
			sleep_ms(10);
	}

	END_DEBUG_EXCEPTION_HANDLER

	return NULL;
}

Client::Client(
		IrrlichtDevice *device,
		const char *playername,
		MapDrawControl &control):
	m_thread(this),
	m_env(new ClientMap(this, control,
			device->getSceneManager()->getRootSceneNode(),
			device->getSceneManager(), 666),
			dout_client),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this),
	m_device(device),
	camera_position(0,0,0),
	camera_direction(0,0,1),
	m_server_ser_ver(SER_FMT_VER_INVALID),
	m_step_dtime(0.0),
	m_inventory_updated(false),
	m_time_of_day(0)
{
	m_packetcounter_timer = 0.0;
	m_delete_unused_sectors_timer = 0.0;
	m_connection_reinit_timer = 0.0;
	m_avg_rtt_timer = 0.0;
	m_playerpos_send_timer = 0.0;

	//m_fetchblock_mutex.Init();
	m_incoming_queue_mutex.Init();
	m_env_mutex.Init();
	m_con_mutex.Init();
	m_step_dtime_mutex.Init();

	m_thread.Start();
	
	{
		JMutexAutoLock envlock(m_env_mutex);
		//m_env.getMap().StartUpdater();

		Player *player = new LocalPlayer();

		player->updateName(playername);

		/*f32 y = BS*2 + BS*20;
		player->setPosition(v3f(0, y, 0));*/
		//player->setPosition(v3f(0, y, 30900*BS)); // DEBUG
		m_env.addPlayer(player);
	}
}

Client::~Client()
{
	{
		JMutexAutoLock conlock(m_con_mutex);
		m_con.Disconnect();
	}

	m_thread.setRun(false);
	while(m_thread.IsRunning())
		sleep_ms(100);
}

void Client::connect(Address address)
{
	DSTACK(__FUNCTION_NAME);
	JMutexAutoLock lock(m_con_mutex);
	m_con.setTimeoutMs(0);
	m_con.Connect(address);
}

bool Client::connectedAndInitialized()
{
	JMutexAutoLock lock(m_con_mutex);

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
	
	
	//dstream<<"Client steps "<<dtime<<std::endl;

	{
		//TimeTaker timer("ReceiveAll()", m_device);
		// 0ms
		ReceiveAll();
	}
	
	{
		//TimeTaker timer("m_con_mutex + m_con.RunTimeouts()", m_device);
		// 0ms
		JMutexAutoLock lock(m_con_mutex);
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

			JMutexAutoLock lock(m_env_mutex);

			core::list<v3s16> deleted_blocks;

			float delete_unused_sectors_timeout = 
				g_settings.getFloat("client_delete_unused_sectors_timeout");
	
			// Delete sector blocks
			/*u32 num = m_env.getMap().deleteUnusedSectors
					(delete_unused_sectors_timeout,
					true, &deleted_blocks);*/
			
			// Delete whole sectors
			u32 num = m_env.getMap().deleteUnusedSectors
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
				JMutexAutoLock lock(m_con_mutex);
				
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

			JMutexAutoLock envlock(m_env_mutex);
			
			Player *myplayer = m_env.getLocalPlayer();
			assert(myplayer != NULL);
	
			// Send TOSERVER_INIT
			// [0] u16 TOSERVER_INIT
			// [2] u8 SER_FMT_VER_HIGHEST
			// [3] u8[20] player_name
			SharedBuffer<u8> data(2+1+PLAYERNAME_SIZE);
			writeU16(&data[0], TOSERVER_INIT);
			writeU8(&data[2], SER_FMT_VER_HIGHEST);
			memset((char*)&data[3], 0, PLAYERNAME_SIZE);
			snprintf((char*)&data[3], PLAYERNAME_SIZE, "%s", myplayer->getName());
			// Send as unreliable
			Send(0, data, false);
		}

		// Not connected, return
		return;
	}

	/*
		Do stuff if connected
	*/
	
	{
		// 0ms
		JMutexAutoLock lock(m_env_mutex);

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
	}

	{
		float &counter = m_avg_rtt_timer;
		counter += dtime;
		if(counter >= 10)
		{
			counter = 0.0;
			JMutexAutoLock lock(m_con_mutex);
			// connectedAndInitialized() is true, peer exists.
			con::Peer *peer = m_con.GetPeer(PEER_ID_SERVER);
			dstream<<DTIME<<"Client: avg_rtt="<<peer->avg_rtt<<std::endl;
		}
	}
	{
		float &counter = m_playerpos_send_timer;
		counter += dtime;
		if(counter >= 0.2)
		{
			counter = 0.0;
			sendPlayerPos();
		}
	}

	/*{
		JMutexAutoLock lock(m_step_dtime_mutex);
		m_step_dtime += dtime;
	}*/
}

float Client::asyncStep()
{
	DSTACK(__FUNCTION_NAME);
	//dstream<<"Client::asyncStep()"<<std::endl;
	
	/*float dtime;
	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		if(m_step_dtime < 0.001)
			return 0.0;
		dtime = m_step_dtime;
		m_step_dtime = 0.0;
	}

	return dtime;*/
	return 0.0;
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
	u32 data_maxsize = 10000;
	Buffer<u8> data(data_maxsize);
	u16 sender_peer_id;
	u32 datasize;
	{
		//TimeTaker t1("con mutex and receive", m_device);
		JMutexAutoLock lock(m_con_mutex);
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
		JMutexAutoLock lock(m_con_mutex);
		// All data is coming from the server
		// PeerNotFoundException is handled by caller.
		peer = m_con.GetPeer(PEER_ID_SERVER);
	}

	u8 ser_version = m_server_ser_ver;

	//dstream<<"Client received command="<<(int)command<<std::endl;

	// Execute fast commands straight away

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
		v3f playerpos_f = intToFloat(playerpos_s16) - v3f(0, BS/2, 0);

		{ //envlock
			JMutexAutoLock envlock(m_env_mutex);
			
			// Set player position
			Player *player = m_env.getLocalPlayer();
			assert(player != NULL);
			player->setPosition(playerpos_f);
		}
		
		// Reply to server
		u32 replysize = 2;
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOSERVER_INIT2);
		// Send as reliable
		m_con.Send(PEER_ID_SERVER, 1, reply, true);

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
		
		//TimeTaker t1("TOCLIENT_REMOVENODE", g_device);
		
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
		
		//TimeTaker t1("TOCLIENT_ADDNODE", g_device);

		MapNode n;
		n.deSerialize(&data[8], ser_version);
		
		addNode(p, n);
	}
	else if(command == TOCLIENT_PLAYERPOS)
	{
		dstream<<"WARNING: Received deprecated TOCLIENT_PLAYERPOS"
				<<std::endl;
		/*u16 our_peer_id;
		{
			JMutexAutoLock lock(m_con_mutex);
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
			JMutexAutoLock envlock(m_env_mutex);
			
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
			JMutexAutoLock lock(m_con_mutex);
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
			JMutexAutoLock envlock(m_env_mutex);
			
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
		/*
			[0] u16 command
			[2] u8 sector count
			[3...] v2s16 pos + sector metadata
		*/
		if(datasize < 3)
			return;

		//dstream<<"Client received TOCLIENT_SECTORMETA"<<std::endl;

		{ //envlock
			JMutexAutoLock envlock(m_env_mutex);
			
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
	}
	else if(command == TOCLIENT_INVENTORY)
	{
		if(datasize < 3)
			return;

		//TimeTaker t1("Parsing TOCLIENT_INVENTORY", m_device);

		{ //envlock
			//TimeTaker t2("mutex locking", m_device);
			JMutexAutoLock envlock(m_env_mutex);
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
		
		JMutexAutoLock envlock(m_env_mutex);

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
		
		u16 time = readU16(&data[2]);
		time = time % 24000;
		m_time_of_day.set(time);
		//dstream<<"Client: time="<<time<<std::endl;
		
		/*
			Day/night

			time_of_day:
			0 = midnight
			12000 = midday
		*/
		{
			const s32 daylength = 16;
			const s32 nightlength = 6;
			const s32 daytimelength = 8;
			s32 d = daylength;
			s32 t = (((m_time_of_day.get())%24000)/(24000/d));
			u32 dr;
			if(t < nightlength/2 || t >= d - nightlength/2)
				dr = 400;
			else if(t >= d/2 - daytimelength/2 && t < d/2 + daytimelength/2)
				dr = 1000;
			else
				dr = 750;

			dstream<<"time_of_day="<<m_time_of_day.get()
					<<", t="<<t
					<<", dr="<<dr
					<<std::endl;
			
			if(dr != m_env.getDayNightRatio())
			{
				//dstream<<"dr="<<dr<<std::endl;
				dout_client<<DTIME<<"Client: changing day-night ratio"<<std::endl;
				m_env.setDayNightRatio(dr);
				m_env.expireMeshes(true);
			}
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
	// Default to queueing it (for slow commands)
	else
	{
		JMutexAutoLock lock(m_incoming_queue_mutex);
		
		IncomingPacket packet(data, datasize);
		m_incoming_queue.push_back(packet);
	}
}

/*
	Returns true if there was something in queue
*/
bool Client::AsyncProcessPacket()
{
	DSTACK(__FUNCTION_NAME);
	
	try //for catching con::PeerNotFoundException
	{

	con::Peer *peer;
	{
		JMutexAutoLock lock(m_con_mutex);
		// All data is coming from the server
		peer = m_con.GetPeer(PEER_ID_SERVER);
	}
	
	u8 ser_version = m_server_ser_ver;

	IncomingPacket packet = getPacket();
	u8 *data = packet.m_data;
	u32 datasize = packet.m_datalen;
	
	// An empty packet means queue is empty
	if(data == NULL){
		return false;
	}
	
	if(datasize < 2)
		return true;
	
	ToClientCommand command = (ToClientCommand)readU16(&data[0]);

	if(command == TOCLIENT_BLOCKDATA)
	{
		// Ignore too small packet
		if(datasize < 8)
			return true;
		/*if(datasize < 8 + MapBlock::serializedLength(ser_version))
			goto getdata;*/
			
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
		
		{ //envlock
			JMutexAutoLock envlock(m_env_mutex);
			
			v2s16 p2d(p.X, p.Z);
			sector = m_env.getMap().emergeSector(p2d);
			
			v2s16 sp = sector->getPos();
			if(sp != p2d)
			{
				dstream<<"ERROR: Got sector with getPos()="
						<<"("<<sp.X<<","<<sp.Y<<"), tried to get"
						<<"("<<p2d.X<<","<<p2d.Y<<")"<<std::endl;
			}

			assert(sp == p2d);
			//assert(sector->getPos() == p2d);
			
			try{
				block = sector->getBlockNoCreate(p.Y);
				/*
					Update an existing block
				*/
				//dstream<<"Updating"<<std::endl;
				block->deSerialize(istr, ser_version);
				//block->setChangedFlag();
			}
			catch(InvalidPositionException &e)
			{
				/*
					Create a new block
				*/
				//dstream<<"Creating new"<<std::endl;
				block = new MapBlock(&m_env.getMap(), p);
				block->deSerialize(istr, ser_version);
				sector->insertBlock(block);
				//block->setChangedFlag();

				//DEBUG
				/*NodeMod mod;
				mod.type = NODEMOD_CHANGECONTENT;
				mod.param = CONTENT_MESE;
				block->setTempMod(v3s16(8,10,8), mod);
				block->setTempMod(v3s16(8,9,8), mod);
				block->setTempMod(v3s16(8,8,8), mod);
				block->setTempMod(v3s16(8,7,8), mod);
				block->setTempMod(v3s16(8,6,8), mod);*/
				
				/*
					Add some coulds
					Well, this is a dumb way to do it, they should just
					be drawn as separate objects.
				*/
				/*if(p.Y == 3)
				{
					NodeMod mod;
					mod.type = NODEMOD_CHANGECONTENT;
					mod.param = CONTENT_CLOUD;
					v3s16 p2;
					p2.Y = 8;
					for(p2.X=3; p2.X<=13; p2.X++)
					for(p2.Z=3; p2.Z<=13; p2.Z++)
					{
						block->setTempMod(p2, mod);
					}
				}*/
			}
		} //envlock
		
		/*
			Acknowledge block.
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

		/*
			Update Mesh of this block and blocks at x-, y- and z-.
			Environment should not be locked as it interlocks with the
			main thread, from which is will want to retrieve textures.
		*/

		m_env.getMap().updateMeshes(block->getPos(), getDayNightRatio());
	}
	else
	{
		dout_client<<DTIME<<"WARNING: Client: Ignoring unknown command "
				<<command<<std::endl;
	}

	return true;

	} //try
	catch(con::PeerNotFoundException &e)
	{
		/*dout_client<<DTIME<<"Client::AsyncProcessData(): Cancelling: The server"
				" connection doesn't exist (a timeout or not yet connected?)"<<std::endl;*/
		return false;
	}
}

bool Client::AsyncProcessData()
{
	for(;;)
	{
		bool r = AsyncProcessPacket();
		if(r == false)
			break;
	}
	return false;
}

void Client::Send(u16 channelnum, SharedBuffer<u8> data, bool reliable)
{
	JMutexAutoLock lock(m_con_mutex);
	m_con.Send(PEER_ID_SERVER, channelnum, data, reliable);
}

IncomingPacket Client::getPacket()
{
	JMutexAutoLock lock(m_incoming_queue_mutex);
	
	core::list<IncomingPacket>::Iterator i;
	// Refer to first one
	i = m_incoming_queue.begin();

	// If queue is empty, return empty packet
	if(i == m_incoming_queue.end()){
		IncomingPacket packet;
		return packet;
	}
	
	// Pop out first packet and return it
	IncomingPacket packet = *i;
	m_incoming_queue.erase(i);
	return packet;
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

void Client::sendPlayerPos()
{
	JMutexAutoLock envlock(m_env_mutex);
	
	Player *myplayer = m_env.getLocalPlayer();
	if(myplayer == NULL)
		return;
	
	u16 our_peer_id;
	{
		JMutexAutoLock lock(m_con_mutex);
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
	JMutexAutoLock envlock(m_env_mutex);
	
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
		m_env.getMap().updateMeshes(p, m_env.getDayNightRatio());
	}
}

void Client::addNode(v3s16 p, MapNode n)
{
	JMutexAutoLock envlock(m_env_mutex);

	core::map<v3s16, MapBlock*> modified_blocks;

	try
	{
		m_env.getMap().addNodeAndUpdate(p, n, modified_blocks);
	}
	catch(InvalidPositionException &e)
	{}
	
	for(core::map<v3s16, MapBlock * >::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		m_env.getMap().updateMeshes(p, m_env.getDayNightRatio());
	}
}
	
void Client::updateCamera(v3f pos, v3f dir)
{
	m_env.getMap().updateCamera(pos, dir);
	camera_position = pos;
	camera_direction = dir;
}

MapNode Client::getNode(v3s16 p)
{
	JMutexAutoLock envlock(m_env_mutex);
	return m_env.getMap().getNode(p);
}

/*void Client::getNode(v3s16 p, MapNode n)
{
	JMutexAutoLock envlock(m_env_mutex);
	m_env.getMap().setNode(p, n);
}*/

/*f32 Client::getGroundHeight(v2s16 p)
{
	JMutexAutoLock envlock(m_env_mutex);
	return m_env.getMap().getGroundHeight(p);
}*/

/*bool Client::isNodeUnderground(v3s16 p)
{
	JMutexAutoLock envlock(m_env_mutex);
	return m_env.getMap().isNodeUnderground(p);
}*/

/*Player * Client::getLocalPlayer()
{
	JMutexAutoLock envlock(m_env_mutex);
	return m_env.getLocalPlayer();
}*/

/*core::list<Player*> Client::getPlayers()
{
	JMutexAutoLock envlock(m_env_mutex);
	return m_env.getPlayers();
}*/

v3f Client::getPlayerPosition()
{
	JMutexAutoLock envlock(m_env_mutex);
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player != NULL);
	return player->getPosition();
}

void Client::setPlayerControl(PlayerControl &control)
{
	JMutexAutoLock envlock(m_env_mutex);
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player != NULL);
	player->control = control;
}

// Returns true if the inventory of the local player has been
// updated from the server. If it is true, it is set to false.
bool Client::getLocalInventoryUpdated()
{
	// m_inventory_updated is behind envlock
	JMutexAutoLock envlock(m_env_mutex);
	bool updated = m_inventory_updated;
	m_inventory_updated = false;
	return updated;
}

// Copies the inventory of the local player to parameter
void Client::getLocalInventory(Inventory &dst)
{
	JMutexAutoLock envlock(m_env_mutex);
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);
	dst = player->inventory;
}

MapBlockObject * Client::getSelectedObject(
		f32 max_d,
		v3f from_pos_f_on_map,
		core::line3d<f32> shootline_on_map
	)
{
	JMutexAutoLock envlock(m_env_mutex);

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
		v3f block_pos_f_on_map = intToFloat(block_pos_i_on_map);
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
		v3f block_pos_f_on_map = intToFloat(block_pos_i_on_map);
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

void Client::printDebugInfo(std::ostream &os)
{
	//JMutexAutoLock lock1(m_fetchblock_mutex);
	JMutexAutoLock lock2(m_incoming_queue_mutex);

	os<<"m_incoming_queue.getSize()="<<m_incoming_queue.getSize()
		//<<", m_fetchblock_history.size()="<<m_fetchblock_history.size()
		//<<", m_opt_not_found_history.size()="<<m_opt_not_found_history.size()
		<<std::endl;
}
	
/*s32 Client::getDayNightIndex()
{
	assert(m_daynight_i >= 0 && m_daynight_i < DAYNIGHT_CACHE_COUNT);
	return m_daynight_i;
}*/

u32 Client::getDayNightRatio()
{
	JMutexAutoLock envlock(m_env_mutex);
	return m_env.getDayNightRatio();
}

/*void Client::updateSomeExpiredMeshes()
{
	TimeTaker timer("updateSomeExpiredMeshes()", g_device);
	
	Player *player;
	{
		JMutexAutoLock envlock(m_env_mutex);
		player = m_env.getLocalPlayer();
	}

	u32 daynight_ratio = getDayNightRatio();

	v3f playerpos = player->getPosition();
	v3f playerspeed = player->getSpeed();

	v3s16 center_nodepos = floatToInt(playerpos);
	v3s16 center = getNodeBlockPos(center_nodepos);

	u32 counter = 0;

	s16 d_max = 5;
	
	for(s16 d = 0; d <= d_max; d++)
	{
		core::list<v3s16> list;
		getFacePositions(list, d);
		
		core::list<v3s16>::Iterator li;
		for(li=list.begin(); li!=list.end(); li++)
		{
			v3s16 p = *li + center;
			MapBlock *block = NULL;
			try
			{
				//JMutexAutoLock envlock(m_env_mutex);
				block = m_env.getMap().getBlockNoCreate(p);
			}
			catch(InvalidPositionException &e)
			{
			}

			if(block == NULL)
				continue;

			if(block->getMeshExpired() == false)
				continue;

			block->updateMesh(daynight_ratio);

			counter++;
			if(counter >= 5)
				return;
		}
	}
}*/

