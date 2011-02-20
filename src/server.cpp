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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "server.h"
#include "utility.h"
#include <iostream>
#include "clientserver.h"
#include "map.h"
#include "jmutexautolock.h"
#include "main.h"
#include "constants.h"
#include "voxel.h"
#include "materials.h"
#include "mineral.h"

#define BLOCK_EMERGE_FLAG_FROMDISK (1<<0)

void * ServerThread::Thread()
{
	ThreadStarted();

	DSTACK(__FUNCTION_NAME);

	BEGIN_DEBUG_EXCEPTION_HANDLER

	while(getRun())
	{
		try{
			//TimeTaker timer("AsyncRunStep() + Receive()");

			{
				//TimeTaker timer("AsyncRunStep()");
				m_server->AsyncRunStep();
			}
		
			//dout_server<<"Running m_server->Receive()"<<std::endl;
			m_server->Receive();
		}
		catch(con::NoIncomingDataException &e)
		{
		}
		catch(con::PeerNotFoundException &e)
		{
			dout_server<<"Server: PeerNotFoundException"<<std::endl;
		}
	}
	
	END_DEBUG_EXCEPTION_HANDLER

	return NULL;
}

void * EmergeThread::Thread()
{
	ThreadStarted();

	DSTACK(__FUNCTION_NAME);

	bool debug=false;
	
	BEGIN_DEBUG_EXCEPTION_HANDLER

	/*
		Get block info from queue, emerge them and send them
		to clients.

		After queue is empty, exit.
	*/
	while(getRun())
	{
		QueuedBlockEmerge *qptr = m_server->m_emerge_queue.pop();
		if(qptr == NULL)
			break;
		
		SharedPtr<QueuedBlockEmerge> q(qptr);

		v3s16 &p = q->pos;
		
		//derr_server<<"EmergeThread::Thread(): running"<<std::endl;

		//TimeTaker timer("block emerge");
		
		/*
			Try to emerge it from somewhere.

			If it is only wanted as optional, only loading from disk
			will be allowed.
		*/
		
		/*
			Check if any peer wants it as non-optional. In that case it
			will be generated.

			Also decrement the emerge queue count in clients.
		*/

		bool optional = true;

		{
			core::map<u16, u8>::Iterator i;
			for(i=q->peer_ids.getIterator(); i.atEnd()==false; i++)
			{
				//u16 peer_id = i.getNode()->getKey();

				// Check flags
				u8 flags = i.getNode()->getValue();
				if((flags & BLOCK_EMERGE_FLAG_FROMDISK) == false)
					optional = false;
				
			}
		}

		/*dstream<<"EmergeThread: p="
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<") "
				<<"optional="<<optional<<std::endl;*/
		
		ServerMap &map = ((ServerMap&)m_server->m_env.getMap());
			
		core::map<v3s16, MapBlock*> changed_blocks;
		core::map<v3s16, MapBlock*> lighting_invalidated_blocks;

		MapBlock *block = NULL;
		bool got_block = true;
		core::map<v3s16, MapBlock*> modified_blocks;
		
		{//envlock

		//TimeTaker envlockwaittimer("block emerge envlock wait time");
		
		// 0-50ms
		JMutexAutoLock envlock(m_server->m_env_mutex);

		//envlockwaittimer.stop();

		//TimeTaker timer("block emerge (while env locked)");
			
		try{
			bool only_from_disk = false;
			
			if(optional)
				only_from_disk = true;
			
			// First check if the block already exists
			//block = map.getBlockNoCreate(p);

			if(block == NULL)
			{
				//dstream<<"Calling emergeBlock"<<std::endl;
				block = map.emergeBlock(
						p,
						only_from_disk,
						changed_blocks,
						lighting_invalidated_blocks);
			}

			// If it is a dummy, block was not found on disk
			if(block->isDummy())
			{
				//dstream<<"EmergeThread: Got a dummy block"<<std::endl;
				got_block = false;

				if(only_from_disk == false)
				{
					dstream<<"EmergeThread: wanted to generate a block but got a dummy"<<std::endl;
					assert(0);
				}
			}
		}
		catch(InvalidPositionException &e)
		{
			// Block not found.
			// This happens when position is over limit.
			got_block = false;
		}
		
		if(got_block)
		{
			if(debug && changed_blocks.size() > 0)
			{
				dout_server<<DTIME<<"Got changed_blocks: ";
				for(core::map<v3s16, MapBlock*>::Iterator i = changed_blocks.getIterator();
						i.atEnd() == false; i++)
				{
					MapBlock *block = i.getNode()->getValue();
					v3s16 p = block->getPos();
					dout_server<<"("<<p.X<<","<<p.Y<<","<<p.Z<<") ";
				}
				dout_server<<std::endl;
			}

			/*
				Collect a list of blocks that have been modified in
				addition to the fetched one.
			*/

			// Add all the "changed blocks" to modified_blocks
			for(core::map<v3s16, MapBlock*>::Iterator i = changed_blocks.getIterator();
					i.atEnd() == false; i++)
			{
				MapBlock *block = i.getNode()->getValue();
				modified_blocks.insert(block->getPos(), block);
			}
			
			/*dstream<<"lighting "<<lighting_invalidated_blocks.size()
					<<" blocks"<<std::endl;*/
			
			//TimeTaker timer("** updateLighting");
			
			// Update lighting without locking the environment mutex,
			// add modified blocks to changed blocks
			map.updateLighting(lighting_invalidated_blocks, modified_blocks);
		}
		// If we got no block, there should be no invalidated blocks
		else
		{
			assert(lighting_invalidated_blocks.size() == 0);
		}

		}//envlock

		/*
			Set sent status of modified blocks on clients
		*/
	
		// NOTE: Server's clients are also behind the connection mutex
		JMutexAutoLock lock(m_server->m_con_mutex);

		/*
			Add the originally fetched block to the modified list
		*/
		if(got_block)
		{
			modified_blocks.insert(p, block);
		}
		
		/*
			Set the modified blocks unsent for all the clients
		*/
		
		for(core::map<u16, RemoteClient*>::Iterator
				i = m_server->m_clients.getIterator();
				i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			
			if(modified_blocks.size() > 0)
			{
				// Remove block from sent history
				client->SetBlocksNotSent(modified_blocks);
			}
		}
		
	}

	END_DEBUG_EXCEPTION_HANDLER

	return NULL;
}

void RemoteClient::GetNextBlocks(Server *server, float dtime,
		core::array<PrioritySortedBlockTransfer> &dest)
{
	DSTACK(__FUNCTION_NAME);
	
	// Increment timers
	m_nearest_unsent_reset_timer += dtime;

	// Won't send anything if already sending
	if(m_blocks_sending.size() >= g_settings.getU16
			("max_simultaneous_block_sends_per_client"))
	{
		//dstream<<"Not sending any blocks, Queue full."<<std::endl;
		return;
	}

	Player *player = server->m_env.getPlayer(peer_id);

	assert(player != NULL);

	v3f playerpos = player->getPosition();
	v3f playerspeed = player->getSpeed();

	v3s16 center_nodepos = floatToInt(playerpos, BS);

	v3s16 center = getNodeBlockPos(center_nodepos);
	
	// Camera position and direction
	v3f camera_pos =
			playerpos + v3f(0, BS+BS/2, 0);
	v3f camera_dir = v3f(0,0,1);
	camera_dir.rotateYZBy(player->getPitch());
	camera_dir.rotateXZBy(player->getYaw());

	/*
		Get the starting value of the block finder radius.
	*/
	s16 last_nearest_unsent_d;
	s16 d_start;
		
	if(m_last_center != center)
	{
		m_nearest_unsent_d = 0;
		m_last_center = center;
	}

	/*dstream<<"m_nearest_unsent_reset_timer="
			<<m_nearest_unsent_reset_timer<<std::endl;*/
	if(m_nearest_unsent_reset_timer > 5.0)
	{
		m_nearest_unsent_reset_timer = 0;
		m_nearest_unsent_d = 0;
		//dstream<<"Resetting m_nearest_unsent_d"<<std::endl;
	}

	last_nearest_unsent_d = m_nearest_unsent_d;
	
	d_start = m_nearest_unsent_d;

	u16 maximum_simultaneous_block_sends_setting = g_settings.getU16
			("max_simultaneous_block_sends_per_client");
	u16 maximum_simultaneous_block_sends = 
			maximum_simultaneous_block_sends_setting;

	/*
		Check the time from last addNode/removeNode.
		
		Decrease send rate if player is building stuff.
	*/
	m_time_from_building += dtime;
	if(m_time_from_building < g_settings.getFloat(
				"full_block_send_enable_min_time_from_building"))
	{
		maximum_simultaneous_block_sends
			= LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS;
	}
	
	u32 num_blocks_selected = m_blocks_sending.size();
	
	/*
		next time d will be continued from the d from which the nearest
		unsent block was found this time.

		This is because not necessarily any of the blocks found this
		time are actually sent.
	*/
	s32 new_nearest_unsent_d = -1;

	s16 d_max = g_settings.getS16("max_block_send_distance");
	s16 d_max_gen = g_settings.getS16("max_block_generate_distance");
	
	//dstream<<"Starting from "<<d_start<<std::endl;

	for(s16 d = d_start; d <= d_max; d++)
	{
		//dstream<<"RemoteClient::SendBlocks(): d="<<d<<std::endl;
		
		/*
			If m_nearest_unsent_d was changed by the EmergeThread
			(it can change it to 0 through SetBlockNotSent),
			update our d to it.
			Else update m_nearest_unsent_d
		*/
		if(m_nearest_unsent_d != last_nearest_unsent_d)
		{
			d = m_nearest_unsent_d;
			last_nearest_unsent_d = m_nearest_unsent_d;
		}

		/*
			Get the border/face dot coordinates of a "d-radiused"
			box
		*/
		core::list<v3s16> list;
		getFacePositions(list, d);
		
		core::list<v3s16>::Iterator li;
		for(li=list.begin(); li!=list.end(); li++)
		{
			v3s16 p = *li + center;
			
			/*
				Send throttling
				- Don't allow too many simultaneous transfers
				- EXCEPT when the blocks are very close

				Also, don't send blocks that are already flying.
			*/
			
			u16 maximum_simultaneous_block_sends_now =
					maximum_simultaneous_block_sends;
			
			if(d <= BLOCK_SEND_DISABLE_LIMITS_MAX_D)
			{
				maximum_simultaneous_block_sends_now =
						maximum_simultaneous_block_sends_setting;
			}

			// Limit is dynamically lowered when building
			if(num_blocks_selected
					>= maximum_simultaneous_block_sends_now)
			{
				/*dstream<<"Not sending more blocks. Queue full. "
						<<m_blocks_sending.size()
						<<std::endl;*/
				goto queue_full;
			}

			if(m_blocks_sending.find(p) != NULL)
				continue;
		
			/*
				Do not go over-limit
			*/
			if(p.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
			|| p.X > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
			|| p.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
			|| p.Y > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
			|| p.Z < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
			|| p.Z > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE)
				continue;
		
			// If this is true, inexistent block will be made from scratch
			bool generate = d <= d_max_gen;
			
			{
				/*// Limit the generating area vertically to 2/3
				if(abs(p.Y - center.Y) > d_max_gen - d_max_gen / 3)
					generate = false;*/

				// Limit the send area vertically to 2/3
				if(abs(p.Y - center.Y) > d_max_gen - d_max_gen / 3)
					continue;
			}

#if 0
			/*
				If block is far away, don't generate it unless it is
				near ground level

				NOTE: We can't know the ground level this way with the
				new generator.
			*/
			if(d > 4)
			{
				v2s16 p2d(p.X, p.Z);
				MapSector *sector = NULL;
				try
				{
					sector = server->m_env.getMap().getSectorNoGenerate(p2d);
				}
				catch(InvalidPositionException &e)
				{
				}

				if(sector != NULL)
				{
					// Get center ground height in nodes
					f32 gh = sector->getGroundHeight(
							v2s16(MAP_BLOCKSIZE/2, MAP_BLOCKSIZE/2));
					// Block center y in nodes
					f32 y = (f32)(p.Y * MAP_BLOCKSIZE + MAP_BLOCKSIZE/2);
					// If differs a lot, don't generate
					if(fabs(gh - y) > MAP_BLOCKSIZE*2)
						generate = false;
				}
			}
#endif

			/*
				Don't generate or send if not in sight
			*/

			if(isBlockInSight(p, camera_pos, camera_dir, 10000*BS) == false)
			{
				continue;
			}
			
			/*
				Don't send already sent blocks
			*/
			{
				if(m_blocks_sent.find(p) != NULL)
					continue;
			}

			/*
				Check if map has this block
			*/
			MapBlock *block = server->m_env.getMap().getBlockNoCreateNoEx(p);
			
			bool surely_not_found_on_disk = false;
			bool block_is_invalid = false;
			if(block != NULL)
			{
				if(block->isDummy())
				{
					surely_not_found_on_disk = true;
				}

				if(block->isValid() == false)
				{
					block_is_invalid = true;
				}
				
				v2s16 p2d(p.X, p.Z);
				ServerMap *map = (ServerMap*)(&server->m_env.getMap());
				v2s16 chunkpos = map->sector_to_chunk(p2d);
				if(map->chunkNonVolatile(chunkpos) == false)
					block_is_invalid = true;
			}

			/*
				If block has been marked to not exist on disk (dummy)
				and generating new ones is not wanted, skip block.
			*/
			if(generate == false && surely_not_found_on_disk == true)
			{
				// get next one.
				continue;
			}

			/*
				Record the lowest d from which a a block has been
				found being not sent and possibly to exist
			*/
			if(new_nearest_unsent_d == -1 || d < new_nearest_unsent_d)
			{
				new_nearest_unsent_d = d;
			}
					
			/*
				Add inexistent block to emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || block_is_invalid)
			{
				//TODO: Get value from somewhere
				// Allow only one block in emerge queue
				if(server->m_emerge_queue.peerItemCount(peer_id) < 1)
				{
					//dstream<<"Adding block to emerge queue"<<std::endl;
					
					// Add it to the emerge queue and trigger the thread
					
					u8 flags = 0;
					if(generate == false)
						flags |= BLOCK_EMERGE_FLAG_FROMDISK;
					
					server->m_emerge_queue.addBlock(peer_id, p, flags);
					server->m_emergethread.trigger();
				}
				
				// get next one.
				continue;
			}

			/*
				Add block to send queue
			*/

			PrioritySortedBlockTransfer q((float)d, p, peer_id);

			dest.push_back(q);

			num_blocks_selected += 1;
		}
	}
queue_full:

	if(new_nearest_unsent_d != -1)
	{
		m_nearest_unsent_d = new_nearest_unsent_d;
	}
}

void RemoteClient::SendObjectData(
		Server *server,
		float dtime,
		core::map<v3s16, bool> &stepped_blocks
	)
{
	DSTACK(__FUNCTION_NAME);

	// Can't send anything without knowing version
	if(serialization_version == SER_FMT_VER_INVALID)
	{
		dstream<<"RemoteClient::SendObjectData(): Not sending, no version."
				<<std::endl;
		return;
	}

	/*
		Send a TOCLIENT_OBJECTDATA packet.
		Sent as unreliable.

		u16 command
		u16 number of player positions
		for each player:
			v3s32 position*100
			v3s32 speed*100
			s32 pitch*100
			s32 yaw*100
		u16 count of blocks
		for each block:
			block objects
	*/

	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOCLIENT_OBJECTDATA);
	os.write((char*)buf, 2);
	
	/*
		Get and write player data
	*/
	
	// Get connected players
	core::list<Player*> players = server->m_env.getPlayers(true);

	// Write player count
	u16 playercount = players.size();
	writeU16(buf, playercount);
	os.write((char*)buf, 2);

	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		Player *player = *i;

		v3f pf = player->getPosition();
		v3f sf = player->getSpeed();

		v3s32 position_i(pf.X*100, pf.Y*100, pf.Z*100);
		v3s32 speed_i   (sf.X*100, sf.Y*100, sf.Z*100);
		s32   pitch_i   (player->getPitch() * 100);
		s32   yaw_i     (player->getYaw() * 100);
		
		writeU16(buf, player->peer_id);
		os.write((char*)buf, 2);
		writeV3S32(buf, position_i);
		os.write((char*)buf, 12);
		writeV3S32(buf, speed_i);
		os.write((char*)buf, 12);
		writeS32(buf, pitch_i);
		os.write((char*)buf, 4);
		writeS32(buf, yaw_i);
		os.write((char*)buf, 4);
	}
	
	/*
		Get and write object data
	*/

	/*
		Get nearby blocks.
		
		For making players to be able to build to their nearby
		environment (building is not possible on blocks that are not
		in memory):
		- Set blocks changed
		- Add blocks to emerge queue if they are not found

		SUGGESTION: These could be ignored from the backside of the player
	*/

	Player *player = server->m_env.getPlayer(peer_id);

	assert(player);

	v3f playerpos = player->getPosition();
	v3f playerspeed = player->getSpeed();

	v3s16 center_nodepos = floatToInt(playerpos, BS);
	v3s16 center = getNodeBlockPos(center_nodepos);

	s16 d_max = g_settings.getS16("active_object_range");
	
	// Number of blocks whose objects were written to bos
	u16 blockcount = 0;

	std::ostringstream bos(std::ios_base::binary);

	for(s16 d = 0; d <= d_max; d++)
	{
		core::list<v3s16> list;
		getFacePositions(list, d);
		
		core::list<v3s16>::Iterator li;
		for(li=list.begin(); li!=list.end(); li++)
		{
			v3s16 p = *li + center;

			/*
				Ignore blocks that haven't been sent to the client
			*/
			{
				if(m_blocks_sent.find(p) == NULL)
					continue;
			}
			
			// Try stepping block and add it to a send queue
			try
			{

			// Get block
			MapBlock *block = server->m_env.getMap().getBlockNoCreate(p);

			/*
				Step block if not in stepped_blocks and add to stepped_blocks.
			*/
			if(stepped_blocks.find(p) == NULL)
			{
				block->stepObjects(dtime, true, server->getDayNightRatio());
				stepped_blocks.insert(p, true);
				block->setChangedFlag();
			}

			// Skip block if there are no objects
			if(block->getObjectCount() == 0)
				continue;
			
			/*
				Write objects
			*/

			// Write blockpos
			writeV3S16(buf, p);
			bos.write((char*)buf, 6);

			// Write objects
			block->serializeObjects(bos, serialization_version);

			blockcount++;

			/*
				Stop collecting objects if data is already too big
			*/
			// Sum of player and object data sizes
			s32 sum = (s32)os.tellp() + 2 + (s32)bos.tellp();
			// break out if data too big
			if(sum > MAX_OBJECTDATA_SIZE)
			{
				goto skip_subsequent;
			}
			
			} //try
			catch(InvalidPositionException &e)
			{
				// Not in memory
				// Add it to the emerge queue and trigger the thread.
				// Fetch the block only if it is on disk.
				
				// Grab and increment counter
				/*SharedPtr<JMutexAutoLock> lock
						(m_num_blocks_in_emerge_queue.getLock());
				m_num_blocks_in_emerge_queue.m_value++;*/
				
				// Add to queue as an anonymous fetch from disk
				u8 flags = BLOCK_EMERGE_FLAG_FROMDISK;
				server->m_emerge_queue.addBlock(0, p, flags);
				server->m_emergethread.trigger();
			}
		}
	}

skip_subsequent:

	// Write block count
	writeU16(buf, blockcount);
	os.write((char*)buf, 2);

	// Write block objects
	os<<bos.str();

	/*
		Send data
	*/
	
	//dstream<<"Server: Sending object data to "<<peer_id<<std::endl;

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as unreliable
	server->m_con.Send(peer_id, 0, data, false);
}

void RemoteClient::GotBlock(v3s16 p)
{
	if(m_blocks_sending.find(p) != NULL)
		m_blocks_sending.remove(p);
	else
	{
		/*dstream<<"RemoteClient::GotBlock(): Didn't find in"
				" m_blocks_sending"<<std::endl;*/
		m_excess_gotblocks++;
	}
	m_blocks_sent.insert(p, true);
}

void RemoteClient::SentBlock(v3s16 p)
{
	if(m_blocks_sending.find(p) == NULL)
		m_blocks_sending.insert(p, 0.0);
	else
		dstream<<"RemoteClient::SentBlock(): Sent block"
				" already in m_blocks_sending"<<std::endl;
}

void RemoteClient::SetBlockNotSent(v3s16 p)
{
	m_nearest_unsent_d = 0;
	
	if(m_blocks_sending.find(p) != NULL)
		m_blocks_sending.remove(p);
	if(m_blocks_sent.find(p) != NULL)
		m_blocks_sent.remove(p);
}

void RemoteClient::SetBlocksNotSent(core::map<v3s16, MapBlock*> &blocks)
{
	m_nearest_unsent_d = 0;
	
	for(core::map<v3s16, MapBlock*>::Iterator
			i = blocks.getIterator();
			i.atEnd()==false; i++)
	{
		v3s16 p = i.getNode()->getKey();

		if(m_blocks_sending.find(p) != NULL)
			m_blocks_sending.remove(p);
		if(m_blocks_sent.find(p) != NULL)
			m_blocks_sent.remove(p);
	}
}

/*
	PlayerInfo
*/

PlayerInfo::PlayerInfo()
{
	name[0] = 0;
}

void PlayerInfo::PrintLine(std::ostream *s)
{
	(*s)<<id<<": ";
	(*s)<<"\""<<name<<"\" ("
			<<(position.X/10)<<","<<(position.Y/10)
			<<","<<(position.Z/10)<<") ";
	address.print(s);
	(*s)<<" avg_rtt="<<avg_rtt;
	(*s)<<std::endl;
}

u32 PIChecksum(core::list<PlayerInfo> &l)
{
	core::list<PlayerInfo>::Iterator i;
	u32 checksum = 1;
	u32 a = 10;
	for(i=l.begin(); i!=l.end(); i++)
	{
		checksum += a * (i->id+1);
		checksum ^= 0x435aafcd;
		a *= 10;
	}
	return checksum;
}

/*
	Server
*/

Server::Server(
		std::string mapsavedir
	):
	m_env(new ServerMap(mapsavedir)),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this),
	m_thread(this),
	m_emergethread(this),
	m_time_of_day(9000),
	m_time_counter(0),
	m_time_of_day_send_timer(0),
	m_uptime(0),
	m_mapsavedir(mapsavedir),
	m_shutdown_requested(false)
{
	//m_flowwater_timer = 0.0;
	m_liquid_transform_timer = 0.0;
	m_print_info_timer = 0.0;
	m_objectdata_timer = 0.0;
	m_emergethread_trigger_timer = 0.0;
	m_savemap_timer = 0.0;
	
	m_env_mutex.Init();
	m_con_mutex.Init();
	m_step_dtime_mutex.Init();
	m_step_dtime = 0.0;

	// Load players
	m_env.deSerializePlayers(m_mapsavedir);
}

Server::~Server()
{
	/*
		Send shutdown message
	*/
	{
		JMutexAutoLock conlock(m_con_mutex);
		
		std::wstring line = L"*** Server shutting down";

		/*
			Send the message to clients
		*/
		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			// Get client and check that it is valid
			RemoteClient *client = i.getNode()->getValue();
			assert(client->peer_id == i.getNode()->getKey());
			if(client->serialization_version == SER_FMT_VER_INVALID)
				continue;

			SendChatMessage(client->peer_id, line);
		}
	}

	/*
		Save players
	*/
	m_env.serializePlayers(m_mapsavedir);
	
	/*
		Stop threads
	*/
	stop();
	
	/*
		Delete clients
	*/
	{
		JMutexAutoLock clientslock(m_con_mutex);

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			/*// Delete player
			// NOTE: These are removed by env destructor
			{
				u16 peer_id = i.getNode()->getKey();
				JMutexAutoLock envlock(m_env_mutex);
				m_env.removePlayer(peer_id);
			}*/
			
			// Delete client
			delete i.getNode()->getValue();
		}
	}
}

void Server::start(unsigned short port)
{
	DSTACK(__FUNCTION_NAME);
	// Stop thread if already running
	m_thread.stop();
	
	// Initialize connection
	m_con.setTimeoutMs(30);
	m_con.Serve(port);

	// Start thread
	m_thread.setRun(true);
	m_thread.Start();
	
	dout_server<<"Server started on port "<<port<<std::endl;
}

void Server::stop()
{
	DSTACK(__FUNCTION_NAME);
	// Stop threads (set run=false first so both start stopping)
	m_thread.setRun(false);
	m_emergethread.setRun(false);
	m_thread.stop();
	m_emergethread.stop();
	
	dout_server<<"Server threads stopped"<<std::endl;
}

void Server::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);
	// Limit a bit
	if(dtime > 2.0)
		dtime = 2.0;
	{
		JMutexAutoLock lock(m_step_dtime_mutex);
		m_step_dtime += dtime;
	}
}

void Server::AsyncRunStep()
{
	DSTACK(__FUNCTION_NAME);
	
	float dtime;
	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		dtime = m_step_dtime;
	}
	
	// Send blocks to clients
	SendBlocks(dtime);
	
	if(dtime < 0.001)
		return;
	
	//dstream<<"Server steps "<<dtime<<std::endl;
	//dstream<<"Server::AsyncRunStep(): dtime="<<dtime<<std::endl;
	
	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		m_step_dtime -= dtime;
	}

	/*
		Update uptime
	*/
	{
		m_uptime.set(m_uptime.get() + dtime);
	}
	
	/*
		Update m_time_of_day
	*/
	{
		m_time_counter += dtime;
		f32 speed = g_settings.getFloat("time_speed") * 24000./(24.*3600);
		u32 units = (u32)(m_time_counter*speed);
		m_time_counter -= (f32)units / speed;
		m_time_of_day.set((m_time_of_day.get() + units) % 24000);
		
		//dstream<<"Server: m_time_of_day = "<<m_time_of_day.get()<<std::endl;

		/*
			Send to clients at constant intervals
		*/

		m_time_of_day_send_timer -= dtime;
		if(m_time_of_day_send_timer < 0.0)
		{
			m_time_of_day_send_timer = g_settings.getFloat("time_send_interval");

			//JMutexAutoLock envlock(m_env_mutex);
			JMutexAutoLock conlock(m_con_mutex);

			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				RemoteClient *client = i.getNode()->getValue();
				//Player *player = m_env.getPlayer(client->peer_id);
				
				SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
						m_time_of_day.get());
				// Send as reliable
				m_con.Send(client->peer_id, 0, data, true);
			}
		}
	}

	{
		// Process connection's timeouts
		JMutexAutoLock lock2(m_con_mutex);
		m_con.RunTimeouts(dtime);
	}
	
	{
		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();
	}

	{
		// Step environment
		// This also runs Map's timers
		JMutexAutoLock lock(m_env_mutex);
		m_env.step(dtime);
	}
	
	/*
		Do background stuff
	*/
	
	/*
		Transform liquids
	*/
	m_liquid_transform_timer += dtime;
	if(m_liquid_transform_timer >= 1.00)
	{
		m_liquid_transform_timer -= 1.00;
		
		JMutexAutoLock lock(m_env_mutex);
		
		core::map<v3s16, MapBlock*> modified_blocks;
		m_env.getMap().transformLiquids(modified_blocks);
#if 0		
		/*
			Update lighting
		*/
		core::map<v3s16, MapBlock*> lighting_modified_blocks;
		ServerMap &map = ((ServerMap&)m_env.getMap());
		map.updateLighting(modified_blocks, lighting_modified_blocks);
		
		// Add blocks modified by lighting to modified_blocks
		for(core::map<v3s16, MapBlock*>::Iterator
				i = lighting_modified_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			MapBlock *block = i.getNode()->getValue();
			modified_blocks.insert(block->getPos(), block);
		}
#endif
		/*
			Set the modified blocks unsent for all the clients
		*/
		
		JMutexAutoLock lock2(m_con_mutex);

		for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			
			if(modified_blocks.size() > 0)
			{
				// Remove block from sent history
				client->SetBlocksNotSent(modified_blocks);
			}
		}
	}

	// Periodically print some info
	{
		float &counter = m_print_info_timer;
		counter += dtime;
		if(counter >= 30.0)
		{
			counter = 0.0;

			JMutexAutoLock lock2(m_con_mutex);

			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				//u16 peer_id = i.getNode()->getKey();
				RemoteClient *client = i.getNode()->getValue();
				client->PrintInfo(std::cout);
			}
		}
	}

	/*
		Check added and deleted active objects
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			Player *player = m_env.getPlayer(client->peer_id);
			v3s16 pos = floatToInt(player->getPosition(), BS);
			s16 radius = 32;

			core::map<u16, bool> removed_objects;
			core::map<u16, bool> added_objects;
			m_env.getRemovedActiveObjects(pos, radius,
					client->m_known_objects, removed_objects);
			m_env.getAddedActiveObjects(pos, radius,
					client->m_known_objects, added_objects);
			
			// Ignore if nothing happened
			if(removed_objects.size() == 0 && added_objects.size() == 0)
				continue;
			
			std::string data_buffer;

			char buf[4];
			
			// Handle removed objects
			writeU16((u8*)buf, removed_objects.size());
			data_buffer.append(buf, 2);
			for(core::map<u16, bool>::Iterator
					i = removed_objects.getIterator();
					i.atEnd()==false; i++)
			{
				// Get object
				u16 id = i.getNode()->getKey();
				ServerActiveObject* obj = m_env.getActiveObject(id);

				// Add to data buffer for sending
				writeU16((u8*)buf, i.getNode()->getKey());
				data_buffer.append(buf, 2);
				
				// Remove from known objects
				client->m_known_objects.remove(i.getNode()->getKey());

				if(obj && obj->m_known_by_count > 0)
					obj->m_known_by_count--;
			}

			// Handle added objects
			writeU16((u8*)buf, added_objects.size());
			data_buffer.append(buf, 2);
			for(core::map<u16, bool>::Iterator
					i = added_objects.getIterator();
					i.atEnd()==false; i++)
			{
				// Get object
				u16 id = i.getNode()->getKey();
				ServerActiveObject* obj = m_env.getActiveObject(id);
				
				// Get object type
				u8 type = ACTIVEOBJECT_TYPE_INVALID;
				if(obj == NULL)
					dstream<<"WARNING: "<<__FUNCTION_NAME
							<<": NULL object"<<std::endl;
				else
					type = obj->getType();

				// Add to data buffer for sending
				writeU16((u8*)buf, id);
				data_buffer.append(buf, 2);
				writeU8((u8*)buf, type);
				data_buffer.append(buf, 1);

				// Add to known objects
				client->m_known_objects.insert(i.getNode()->getKey(), false);

				if(obj)
					obj->m_known_by_count++;
			}

			// Send packet
			SharedBuffer<u8> reply(2 + data_buffer.size());
			writeU16(&reply[0], TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD);
			memcpy((char*)&reply[2], data_buffer.c_str(),
					data_buffer.size());
			// Send as reliable
			m_con.Send(client->peer_id, 0, reply, true);

			dstream<<"INFO: Server: Sent object remove/add: "
					<<removed_objects.size()<<" removed, "
					<<added_objects.size()<<" added, "
					<<"packet size is "<<reply.getSize()<<std::endl;
		}
	}

	/*
		Send object messages
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		// Key = object id
		// Value = data sent by object
		core::map<u16, core::list<ActiveObjectMessage>* > buffered_messages;

		// Get active object messages from environment
		for(;;)
		{
			ActiveObjectMessage aom = m_env.getActiveObjectMessage();
			if(aom.id == 0)
				break;
			
			core::list<ActiveObjectMessage>* message_list = NULL;
			core::map<u16, core::list<ActiveObjectMessage>* >::Node *n;
			n = buffered_messages.find(aom.id);
			if(n == NULL)
			{
				message_list = new core::list<ActiveObjectMessage>;
				buffered_messages.insert(aom.id, message_list);
			}
			else
			{
				message_list = n->getValue();
			}
			message_list->push_back(aom);
		}
		
		// Route data to every client
		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd()==false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			std::string reliable_data;
			std::string unreliable_data;
			// Go through all objects in message buffer
			for(core::map<u16, core::list<ActiveObjectMessage>* >::Iterator
					j = buffered_messages.getIterator();
					j.atEnd()==false; j++)
			{
				// If object is not known by client, skip it
				u16 id = j.getNode()->getKey();
				if(client->m_known_objects.find(id) == NULL)
					continue;
				// Get message list of object
				core::list<ActiveObjectMessage>* list = j.getNode()->getValue();
				// Go through every message
				for(core::list<ActiveObjectMessage>::Iterator
						k = list->begin(); k != list->end(); k++)
				{
					// Compose the full new data with header
					ActiveObjectMessage aom = *k;
					std::string new_data;
					// Add header (object id + length)
					char header[4];
					writeU16((u8*)&header[0], aom.id);
					writeU16((u8*)&header[2], aom.datastring.size());
					new_data.append(header, 4);
					// Add data
					new_data += aom.datastring;
					// Add data to buffer
					if(aom.reliable)
						reliable_data += new_data;
					else
						unreliable_data += new_data;
				}
			}
			/*
				reliable_data and unreliable_data are now ready.
				Send them.
			*/
			if(reliable_data.size() > 0)
			{
				SharedBuffer<u8> reply(2 + reliable_data.size());
				writeU16(&reply[0], TOCLIENT_ACTIVE_OBJECT_MESSAGES);
				memcpy((char*)&reply[2], reliable_data.c_str(),
						reliable_data.size());
				// Send as reliable
				m_con.Send(client->peer_id, 0, reply, true);
			}
			if(unreliable_data.size() > 0)
			{
				SharedBuffer<u8> reply(2 + unreliable_data.size());
				writeU16(&reply[0], TOCLIENT_ACTIVE_OBJECT_MESSAGES);
				memcpy((char*)&reply[2], unreliable_data.c_str(),
						unreliable_data.size());
				// Send as unreliable
				m_con.Send(client->peer_id, 0, reply, false);
			}
			if(reliable_data.size() > 0 || unreliable_data.size() > 0)
			{
				dstream<<"INFO: Server: Size of object message data: "
						<<"reliable: "<<reliable_data.size()
						<<", unreliable: "<<unreliable_data.size()
						<<std::endl;
			}
		}

		// Clear buffered_messages
		for(core::map<u16, core::list<ActiveObjectMessage>* >::Iterator
				i = buffered_messages.getIterator();
				i.atEnd()==false; i++)
		{
			delete i.getNode()->getValue();
		}
	}

	/*
		Send object positions
	*/
	{
		float &counter = m_objectdata_timer;
		counter += dtime;
		if(counter >= g_settings.getFloat("objectdata_interval"))
		{
			JMutexAutoLock lock1(m_env_mutex);
			JMutexAutoLock lock2(m_con_mutex);
			SendObjectData(counter);

			counter = 0.0;
		}
	}
	
	/*
		Trigger emergethread (it somehow gets to a non-triggered but
		bysy state sometimes)
	*/
	{
		float &counter = m_emergethread_trigger_timer;
		counter += dtime;
		if(counter >= 2.0)
		{
			counter = 0.0;
			
			m_emergethread.trigger();
		}
	}

	// Save map
	{
		float &counter = m_savemap_timer;
		counter += dtime;
		if(counter >= g_settings.getFloat("server_map_save_interval"))
		{
			counter = 0.0;

			JMutexAutoLock lock(m_env_mutex);

			if(((ServerMap*)(&m_env.getMap()))->isSavingEnabled() == true)
			{
				// Save only changed parts
				m_env.getMap().save(true);

				// Delete unused sectors
				u32 deleted_count = m_env.getMap().deleteUnusedSectors(
						g_settings.getFloat("server_unload_unused_sectors_timeout"));
				if(deleted_count > 0)
				{
					dout_server<<"Server: Unloaded "<<deleted_count
							<<" sectors from memory"<<std::endl;
				}

				// Save players
				m_env.serializePlayers(m_mapsavedir);
			}
		}
	}
}

void Server::Receive()
{
	DSTACK(__FUNCTION_NAME);
	u32 data_maxsize = 10000;
	Buffer<u8> data(data_maxsize);
	u16 peer_id;
	u32 datasize;
	try{
		{
			JMutexAutoLock conlock(m_con_mutex);
			datasize = m_con.Receive(peer_id, *data, data_maxsize);
		}

		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();

		ProcessData(*data, datasize, peer_id);
	}
	catch(con::InvalidIncomingDataException &e)
	{
		derr_server<<"Server::Receive(): "
				"InvalidIncomingDataException: what()="
				<<e.what()<<std::endl;
	}
	catch(con::PeerNotFoundException &e)
	{
		//NOTE: This is not needed anymore
		
		// The peer has been disconnected.
		// Find the associated player and remove it.

		/*JMutexAutoLock envlock(m_env_mutex);

		dout_server<<"ServerThread: peer_id="<<peer_id
				<<" has apparently closed connection. "
				<<"Removing player."<<std::endl;

		m_env.removePlayer(peer_id);*/
	}
}

void Server::ProcessData(u8 *data, u32 datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	// Environment is locked first.
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);
	
	con::Peer *peer;
	try{
		peer = m_con.GetPeer(peer_id);
	}
	catch(con::PeerNotFoundException &e)
	{
		derr_server<<DTIME<<"Server::ProcessData(): Cancelling: peer "
				<<peer_id<<" not found"<<std::endl;
		return;
	}
	
	u8 peer_ser_ver = getClient(peer->id)->serialization_version;

	try
	{

	if(datasize < 2)
		return;

	ToServerCommand command = (ToServerCommand)readU16(&data[0]);
	
	if(command == TOSERVER_INIT)
	{
		// [0] u16 TOSERVER_INIT
		// [2] u8 SER_FMT_VER_HIGHEST
		// [3] u8[20] player_name

		if(datasize < 3)
			return;

		derr_server<<DTIME<<"Server: Got TOSERVER_INIT from "
				<<peer->id<<std::endl;

		// First byte after command is maximum supported
		// serialization version
		u8 client_max = data[2];
		u8 our_max = SER_FMT_VER_HIGHEST;
		// Use the highest version supported by both
		u8 deployed = core::min_(client_max, our_max);
		// If it's lower than the lowest supported, give up.
		if(deployed < SER_FMT_VER_LOWEST)
			deployed = SER_FMT_VER_INVALID;

		//peer->serialization_version = deployed;
		getClient(peer->id)->pending_serialization_version = deployed;

		if(deployed == SER_FMT_VER_INVALID)
		{
			derr_server<<DTIME<<"Server: Cannot negotiate "
					"serialization version with peer "
					<<peer_id<<std::endl;
			return;
		}

		/*
			Set up player
		*/
		
		// Get player name
		const u32 playername_size = 20;
		char playername[playername_size];
		for(u32 i=0; i<playername_size-1; i++)
		{
			playername[i] = data[3+i];
		}
		playername[playername_size-1] = 0;
		
		// Get player
		Player *player = emergePlayer(playername, "", peer_id);
		//Player *player = m_env.getPlayer(peer_id);

		/*{
			// DEBUG: Test serialization
			std::ostringstream test_os;
			player->serialize(test_os);
			dstream<<"Player serialization test: \""<<test_os.str()
					<<"\""<<std::endl;
			std::istringstream test_is(test_os.str());
			player->deSerialize(test_is);
		}*/

		// If failed, cancel
		if(player == NULL)
		{
			derr_server<<DTIME<<"Server: peer_id="<<peer_id
					<<": failed to emerge player"<<std::endl;
			return;
		}

		/*
		// If a client is already connected to the player, cancel
		if(player->peer_id != 0)
		{
			derr_server<<DTIME<<"Server: peer_id="<<peer_id
					<<" tried to connect to "
					"an already connected player (peer_id="
					<<player->peer_id<<")"<<std::endl;
			return;
		}
		// Set client of player
		player->peer_id = peer_id;
		*/

		// Check if player doesn't exist
		if(player == NULL)
			throw con::InvalidIncomingDataException
				("Server::ProcessData(): INIT: Player doesn't exist");

		/*// update name if it was supplied
		if(datasize >= 20+3)
		{
			data[20+3-1] = 0;
			player->updateName((const char*)&data[3]);
		}*/

		// Now answer with a TOCLIENT_INIT
		
		SharedBuffer<u8> reply(2+1+6);
		writeU16(&reply[0], TOCLIENT_INIT);
		writeU8(&reply[2], deployed);
		writeV3S16(&reply[3], floatToInt(player->getPosition()+v3f(0,BS/2,0), BS));
		// Send as reliable
		m_con.Send(peer_id, 0, reply, true);

		return;
	}
	if(command == TOSERVER_INIT2)
	{
		derr_server<<DTIME<<"Server: Got TOSERVER_INIT2 from "
				<<peer->id<<std::endl;


		getClient(peer->id)->serialization_version
				= getClient(peer->id)->pending_serialization_version;

		/*
			Send some initialization data
		*/
		
		// Send player info to all players
		SendPlayerInfos();

		// Send inventory to player
		SendInventory(peer->id);
		
		// Send time of day
		{
			SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
					m_time_of_day.get());
			m_con.Send(peer->id, 0, data, true);
		}
		
		// Send information about server to player in chat
		SendChatMessage(peer_id, getStatusString());
		
		// Send information about joining in chat
		{
			std::wstring name = L"unknown";
			Player *player = m_env.getPlayer(peer_id);
			if(player != NULL)
				name = narrow_to_wide(player->getName());
			
			std::wstring message;
			message += L"*** ";
			message += name;
			message += L" joined game";
			BroadcastChatMessage(message);
		}

		return;
	}

	if(peer_ser_ver == SER_FMT_VER_INVALID)
	{
		derr_server<<DTIME<<"Server::ProcessData(): Cancelling: Peer"
				" serialization format invalid or not initialized."
				" Skipping incoming command="<<command<<std::endl;
		return;
	}
	
	Player *player = m_env.getPlayer(peer_id);

	if(player == NULL){
		derr_server<<"Server::ProcessData(): Cancelling: "
				"No player for peer_id="<<peer_id
				<<std::endl;
		return;
	}
	if(command == TOSERVER_PLAYERPOS)
	{
		if(datasize < 2+12+12+4+4)
			return;
	
		u32 start = 0;
		v3s32 ps = readV3S32(&data[start+2]);
		v3s32 ss = readV3S32(&data[start+2+12]);
		f32 pitch = (f32)readS32(&data[2+12+12]) / 100.0;
		f32 yaw = (f32)readS32(&data[2+12+12+4]) / 100.0;
		v3f position((f32)ps.X/100., (f32)ps.Y/100., (f32)ps.Z/100.);
		v3f speed((f32)ss.X/100., (f32)ss.Y/100., (f32)ss.Z/100.);
		pitch = wrapDegrees(pitch);
		yaw = wrapDegrees(yaw);
		player->setPosition(position);
		player->setSpeed(speed);
		player->setPitch(pitch);
		player->setYaw(yaw);
		
		/*dout_server<<"Server::ProcessData(): Moved player "<<peer_id<<" to "
				<<"("<<position.X<<","<<position.Y<<","<<position.Z<<")"
				<<" pitch="<<pitch<<" yaw="<<yaw<<std::endl;*/
	}
	else if(command == TOSERVER_GOTBLOCKS)
	{
		if(datasize < 2+1)
			return;
		
		/*
			[0] u16 command
			[2] u8 count
			[3] v3s16 pos_0
			[3+6] v3s16 pos_1
			...
		*/

		u16 count = data[2];
		for(u16 i=0; i<count; i++)
		{
			if((s16)datasize < 2+1+(i+1)*6)
				throw con::InvalidIncomingDataException
					("GOTBLOCKS length is too short");
			v3s16 p = readV3S16(&data[2+1+i*6]);
			/*dstream<<"Server: GOTBLOCKS ("
					<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
			RemoteClient *client = getClient(peer_id);
			client->GotBlock(p);
		}
	}
	else if(command == TOSERVER_DELETEDBLOCKS)
	{
		if(datasize < 2+1)
			return;
		
		/*
			[0] u16 command
			[2] u8 count
			[3] v3s16 pos_0
			[3+6] v3s16 pos_1
			...
		*/

		u16 count = data[2];
		for(u16 i=0; i<count; i++)
		{
			if((s16)datasize < 2+1+(i+1)*6)
				throw con::InvalidIncomingDataException
					("DELETEDBLOCKS length is too short");
			v3s16 p = readV3S16(&data[2+1+i*6]);
			/*dstream<<"Server: DELETEDBLOCKS ("
					<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
			RemoteClient *client = getClient(peer_id);
			client->SetBlockNotSent(p);
		}
	}
	else if(command == TOSERVER_CLICK_OBJECT)
	{
		if(datasize < 13)
			return;

		/*
			[0] u16 command
			[2] u8 button (0=left, 1=right)
			[3] v3s16 block
			[9] s16 id
			[11] u16 item
		*/
		u8 button = readU8(&data[2]);
		v3s16 p;
		p.X = readS16(&data[3]);
		p.Y = readS16(&data[5]);
		p.Z = readS16(&data[7]);
		s16 id = readS16(&data[9]);
		//u16 item_i = readU16(&data[11]);

		MapBlock *block = NULL;
		try
		{
			block = m_env.getMap().getBlockNoCreate(p);
		}
		catch(InvalidPositionException &e)
		{
			derr_server<<"CLICK_OBJECT block not found"<<std::endl;
			return;
		}

		MapBlockObject *obj = block->getObject(id);

		if(obj == NULL)
		{
			derr_server<<"CLICK_OBJECT object not found"<<std::endl;
			return;
		}

		//TODO: Check that object is reasonably close
		
		// Left click
		if(button == 0)
		{
			InventoryList *ilist = player->inventory.getList("main");
			if(g_settings.getBool("creative_mode") == false && ilist != NULL)
			{
			
				// Skip if inventory has no free space
				if(ilist->getUsedSlots() == ilist->getSize())
				{
					dout_server<<"Player inventory has no free space"<<std::endl;
					return;
				}
				
				/*
					Create the inventory item
				*/
				InventoryItem *item = NULL;
				// If it is an item-object, take the item from it
				if(obj->getTypeId() == MAPBLOCKOBJECT_TYPE_ITEM)
				{
					item = ((ItemObject*)obj)->createInventoryItem();
				}
				// Else create an item of the object
				else
				{
					item = new MapBlockObjectItem
							(obj->getInventoryString());
				}
				
				// Add to inventory and send inventory
				ilist->addItem(item);
				SendInventory(player->peer_id);
			}

			// Remove from block
			block->removeObject(id);
		}
	}
	else if(command == TOSERVER_GROUND_ACTION)
	{
		if(datasize < 17)
			return;
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
		u8 action = readU8(&data[2]);
		v3s16 p_under;
		p_under.X = readS16(&data[3]);
		p_under.Y = readS16(&data[5]);
		p_under.Z = readS16(&data[7]);
		v3s16 p_over;
		p_over.X = readS16(&data[9]);
		p_over.Y = readS16(&data[11]);
		p_over.Z = readS16(&data[13]);
		u16 item_i = readU16(&data[15]);

		//TODO: Check that target is reasonably close
		
		/*
			0: start digging
		*/
		if(action == 0)
		{
			/*
				NOTE: This can be used in the future to check if
				somebody is cheating, by checking the timing.
			*/
		} // action == 0

		/*
			2: stop digging
		*/
		else if(action == 2)
		{
#if 0
			RemoteClient *client = getClient(peer->id);
			JMutexAutoLock digmutex(client->m_dig_mutex);
			client->m_dig_tool_item = -1;
#endif
		}

		/*
			3: Digging completed
		*/
		else if(action == 3)
		{
			// Mandatory parameter; actually used for nothing
			core::map<v3s16, MapBlock*> modified_blocks;

			u8 material;
			u8 mineral = MINERAL_NONE;

			try
			{
				MapNode n = m_env.getMap().getNode(p_under);
				// Get material at position
				material = n.d;
				// If it's not diggable, do nothing
				if(content_diggable(material) == false)
				{
					derr_server<<"Server: Not finishing digging: Node not diggable"
							<<std::endl;

					// Client probably has wrong data.
					// Set block not sent, so that client will get
					// a valid one.
					dstream<<"Client "<<peer_id<<" tried to dig "
							<<"node from invalid position; setting"
							<<" MapBlock not sent."<<std::endl;
					RemoteClient *client = getClient(peer_id);
					v3s16 blockpos = getNodeBlockPos(p_under);
					client->SetBlockNotSent(blockpos);
						
					return;
				}
				// Get mineral
				mineral = n.getMineral();
			}
			catch(InvalidPositionException &e)
			{
				derr_server<<"Server: Not finishing digging: Node not found."
						<<" Adding block to emerge queue."
						<<std::endl;
				m_emerge_queue.addBlock(peer_id,
						getNodeBlockPos(p_over), BLOCK_EMERGE_FLAG_FROMDISK);
				return;
			}
			
			/*
				Send the removal to all other clients
			*/

			// Create packet
			u32 replysize = 8;
			SharedBuffer<u8> reply(replysize);
			writeU16(&reply[0], TOCLIENT_REMOVENODE);
			writeS16(&reply[2], p_under.X);
			writeS16(&reply[4], p_under.Y);
			writeS16(&reply[6], p_under.Z);

			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				// Get client and check that it is valid
				RemoteClient *client = i.getNode()->getValue();
				assert(client->peer_id == i.getNode()->getKey());
				if(client->serialization_version == SER_FMT_VER_INVALID)
					continue;

				// Don't send if it's the same one
				if(peer_id == client->peer_id)
					continue;

				// Send as reliable
				m_con.Send(client->peer_id, 0, reply, true);
			}
			
			/*
				Update and send inventory
			*/

			if(g_settings.getBool("creative_mode") == false)
			{
				/*
					Wear out tool
				*/
				InventoryList *mlist = player->inventory.getList("main");
				if(mlist != NULL)
				{
					InventoryItem *item = mlist->getItem(item_i);
					if(item && (std::string)item->getName() == "ToolItem")
					{
						ToolItem *titem = (ToolItem*)item;
						std::string toolname = titem->getToolName();

						// Get digging properties for material and tool
						DiggingProperties prop =
								getDiggingProperties(material, toolname);

						if(prop.diggable == false)
						{
							derr_server<<"Server: WARNING: Player digged"
									<<" with impossible material + tool"
									<<" combination"<<std::endl;
						}
						
						bool weared_out = titem->addWear(prop.wear);

						if(weared_out)
						{
							mlist->deleteItem(item_i);
						}
					}
				}

				/*
					Add dug item to inventory
				*/

				InventoryItem *item = NULL;

				if(mineral != MINERAL_NONE)
					item = getDiggedMineralItem(mineral);
				
				// If not mineral
				if(item == NULL)
				{
					std::string &dug_s = content_features(material).dug_item;
					if(dug_s != "")
					{
						std::istringstream is(dug_s, std::ios::binary);
						item = InventoryItem::deSerialize(is);
					}
				}
				
				if(item != NULL)
				{
					// Add a item to inventory
					player->inventory.addItem("main", item);

					// Send inventory
					SendInventory(player->peer_id);
				}
			}

			/*
				Remove the node
				(this takes some time so it is done after the quick stuff)
			*/
			m_env.getMap().removeNodeAndUpdate(p_under, modified_blocks);

#if 0
			/*
				Update water
			*/
			
			// Update water pressure around modification
			// This also adds it to m_flow_active_nodes if appropriate

			MapVoxelManipulator v(&m_env.getMap());
			v.m_disable_water_climb =
					g_settings.getBool("disable_water_climb");
			
			VoxelArea area(p_under-v3s16(1,1,1), p_under+v3s16(1,1,1));

			try
			{
				v.updateAreaWaterPressure(area, m_flow_active_nodes);
			}
			catch(ProcessingLimitException &e)
			{
				dstream<<"Processing limit reached (1)"<<std::endl;
			}
			
			v.blitBack(modified_blocks);
#endif
		}
		
		/*
			1: place block
		*/
		else if(action == 1)
		{

			InventoryList *ilist = player->inventory.getList("main");
			if(ilist == NULL)
				return;

			// Get item
			InventoryItem *item = ilist->getItem(item_i);
			
			// If there is no item, it is not possible to add it anywhere
			if(item == NULL)
				return;
			
			/*
				Handle material items
			*/
			if(std::string("MaterialItem") == item->getName())
			{
				try{
					// Don't add a node if this is not a free space
					MapNode n2 = m_env.getMap().getNode(p_over);
					if(content_buildable_to(n2.d) == false)
					{
						// Client probably has wrong data.
						// Set block not sent, so that client will get
						// a valid one.
						dstream<<"Client "<<peer_id<<" tried to place"
								<<" node in invalid position; setting"
								<<" MapBlock not sent."<<std::endl;
						RemoteClient *client = getClient(peer_id);
						v3s16 blockpos = getNodeBlockPos(p_over);
						client->SetBlockNotSent(blockpos);
						return;
					}
				}
				catch(InvalidPositionException &e)
				{
					derr_server<<"Server: Ignoring ADDNODE: Node not found"
							<<" Adding block to emerge queue."
							<<std::endl;
					m_emerge_queue.addBlock(peer_id,
							getNodeBlockPos(p_over), BLOCK_EMERGE_FLAG_FROMDISK);
					return;
				}

				// Reset build time counter
				getClient(peer->id)->m_time_from_building = 0.0;
				
				// Create node data
				MaterialItem *mitem = (MaterialItem*)item;
				MapNode n;
				n.d = mitem->getMaterial();
				if(content_features(n.d).wall_mounted)
					n.dir = packDir(p_under - p_over);

				// Create packet
				u32 replysize = 8 + MapNode::serializedLength(peer_ser_ver);
				SharedBuffer<u8> reply(replysize);
				writeU16(&reply[0], TOCLIENT_ADDNODE);
				writeS16(&reply[2], p_over.X);
				writeS16(&reply[4], p_over.Y);
				writeS16(&reply[6], p_over.Z);
				n.serialize(&reply[8], peer_ser_ver);
				// Send as reliable
				m_con.SendToAll(0, reply, true);
				
				/*
					Handle inventory
				*/
				InventoryList *ilist = player->inventory.getList("main");
				if(g_settings.getBool("creative_mode") == false && ilist)
				{
					// Remove from inventory and send inventory
					if(mitem->getCount() == 1)
						ilist->deleteItem(item_i);
					else
						mitem->remove(1);
					// Send inventory
					SendInventory(peer_id);
				}
				
				/*
					Add node.

					This takes some time so it is done after the quick stuff
				*/
				core::map<v3s16, MapBlock*> modified_blocks;
				m_env.getMap().addNodeAndUpdate(p_over, n, modified_blocks);
				
				/*
					Calculate special events
				*/
				
				/*if(n.d == CONTENT_MESE)
				{
					u32 count = 0;
					for(s16 z=-1; z<=1; z++)
					for(s16 y=-1; y<=1; y++)
					for(s16 x=-1; x<=1; x++)
					{
						
					}
				}*/
			}
			/*
				Handle other items
			*/
			else
			{
				v3s16 blockpos = getNodeBlockPos(p_over);

				MapBlock *block = NULL;
				try
				{
					block = m_env.getMap().getBlockNoCreate(blockpos);
				}
				catch(InvalidPositionException &e)
				{
					derr_server<<"Error while placing object: "
							"block not found"<<std::endl;
					return;
				}

				v3s16 block_pos_i_on_map = block->getPosRelative();
				v3f block_pos_f_on_map = intToFloat(block_pos_i_on_map, BS);

				v3f pos = intToFloat(p_over, BS);
				pos -= block_pos_f_on_map;
				
				/*dout_server<<"pos="
						<<"("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"
						<<std::endl;*/

				MapBlockObject *obj = NULL;

				/*
					Handle block object items
				*/
				if(std::string("MBOItem") == item->getName())
				{
					MapBlockObjectItem *oitem = (MapBlockObjectItem*)item;

					/*dout_server<<"Trying to place a MapBlockObjectItem: "
							"inventorystring=\""
							<<oitem->getInventoryString()
							<<"\""<<std::endl;*/
							
					obj = oitem->createObject
							(pos, player->getYaw(), player->getPitch());
				}
				/*
					Handle other items
				*/
				else
				{
					dout_server<<"Placing a miscellaneous item on map"
							<<std::endl;
					/*
						Create an ItemObject that contains the item.
					*/
					ItemObject *iobj = new ItemObject(NULL, -1, pos);
					std::ostringstream os(std::ios_base::binary);
					item->serialize(os);
					dout_server<<"Item string is \""<<os.str()<<"\""<<std::endl;
					iobj->setItemString(os.str());
					obj = iobj;
				}

				if(obj == NULL)
				{
					derr_server<<"WARNING: item resulted in NULL object, "
							<<"not placing onto map"
							<<std::endl;
				}
				else
				{
					block->addObject(obj);

					dout_server<<"Placed object"<<std::endl;

					InventoryList *ilist = player->inventory.getList("main");
					if(g_settings.getBool("creative_mode") == false && ilist)
					{
						// Remove from inventory and send inventory
						ilist->deleteItem(item_i);
						// Send inventory
						SendInventory(peer_id);
					}
				}
			}

		} // action == 1

		/*
			Catch invalid actions
		*/
		else
		{
			derr_server<<"WARNING: Server: Invalid action "
					<<action<<std::endl;
		}
	}
#if 0
	else if(command == TOSERVER_RELEASE)
	{
		if(datasize < 3)
			return;
		/*
			length: 3
			[0] u16 command
			[2] u8 button
		*/
		dstream<<"TOSERVER_RELEASE ignored"<<std::endl;
	}
#endif
	else if(command == TOSERVER_SIGNTEXT)
	{
		/*
			u16 command
			v3s16 blockpos
			s16 id
			u16 textlen
			textdata
		*/
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		u8 buf[6];
		// Read stuff
		is.read((char*)buf, 6);
		v3s16 blockpos = readV3S16(buf);
		is.read((char*)buf, 2);
		s16 id = readS16(buf);
		is.read((char*)buf, 2);
		u16 textlen = readU16(buf);
		std::string text;
		for(u16 i=0; i<textlen; i++)
		{
			is.read((char*)buf, 1);
			text += (char)buf[0];
		}

		MapBlock *block = NULL;
		try
		{
			block = m_env.getMap().getBlockNoCreate(blockpos);
		}
		catch(InvalidPositionException &e)
		{
			derr_server<<"Error while setting sign text: "
					"block not found"<<std::endl;
			return;
		}

		MapBlockObject *obj = block->getObject(id);
		if(obj == NULL)
		{
			derr_server<<"Error while setting sign text: "
					"object not found"<<std::endl;
			return;
		}
		
		if(obj->getTypeId() != MAPBLOCKOBJECT_TYPE_SIGN)
		{
			derr_server<<"Error while setting sign text: "
					"object is not a sign"<<std::endl;
			return;
		}

		((SignObject*)obj)->setText(text);

		obj->getBlock()->setChangedFlag();
	}
	else if(command == TOSERVER_INVENTORY_ACTION)
	{
		/*// Ignore inventory changes if in creative mode
		if(g_settings.getBool("creative_mode") == true)
		{
			dstream<<"TOSERVER_INVENTORY_ACTION: ignoring in creative mode"
					<<std::endl;
			return;
		}*/
		// Strip command and create a stream
		std::string datastring((char*)&data[2], datasize-2);
		dstream<<"TOSERVER_INVENTORY_ACTION: data="<<datastring<<std::endl;
		std::istringstream is(datastring, std::ios_base::binary);
		// Create an action
		InventoryAction *a = InventoryAction::deSerialize(is);
		if(a != NULL)
		{
			/*
				Handle craftresult specially if not in creative mode
			*/
			bool disable_action = false;
			if(a->getType() == IACTION_MOVE
					&& g_settings.getBool("creative_mode") == false)
			{
				IMoveAction *ma = (IMoveAction*)a;
				// Don't allow moving anything to craftresult
				if(ma->to_name == "craftresult")
				{
					// Do nothing
					disable_action = true;
				}
				// When something is removed from craftresult
				if(ma->from_name == "craftresult")
				{
					disable_action = true;
					// Remove stuff from craft
					InventoryList *clist = player->inventory.getList("craft");
					if(clist)
					{
						u16 count = ma->count;
						if(count == 0)
							count = 1;
						clist->decrementMaterials(count);
					}
					// Do action
					// Feed action to player inventory
					a->apply(&player->inventory);
					// Eat it
					delete a;
					// If something appeared in craftresult, throw it
					// in the main list
					InventoryList *rlist = player->inventory.getList("craftresult");
					InventoryList *mlist = player->inventory.getList("main");
					if(rlist && mlist && rlist->getUsedSlots() == 1)
					{
						InventoryItem *item1 = rlist->changeItem(0, NULL);
						mlist->addItem(item1);
					}
				}
			}
			if(disable_action == false)
			{
				// Feed action to player inventory
				a->apply(&player->inventory);
				// Eat it
				delete a;
			}
			// Send inventory
			SendInventory(player->peer_id);
		}
		else
		{
			dstream<<"TOSERVER_INVENTORY_ACTION: "
					<<"InventoryAction::deSerialize() returned NULL"
					<<std::endl;
		}
	}
	else if(command == TOSERVER_CHAT_MESSAGE)
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

		// Get player name of this client
		std::wstring name = narrow_to_wide(player->getName());
		
		// Line to send to players
		std::wstring line;
		// Whether to send to the player that sent the line
		bool send_to_sender = false;
		// Whether to send to other players
		bool send_to_others = false;
		
		// Parse commands
		std::wstring commandprefix = L"/#";
		if(message.substr(0, commandprefix.size()) == commandprefix)
		{
			line += L"Server: ";

			message = message.substr(commandprefix.size());
			// Get player name as narrow string
			std::string name_s = player->getName();
			// Convert message to narrow string
			std::string message_s = wide_to_narrow(message);
			// Operator is the single name defined in config.
			std::string operator_name = g_settings.get("name");
			bool is_operator = (operator_name != "" &&
					wide_to_narrow(name) == operator_name);
			bool valid_command = false;
			if(message_s == "help")
			{
				line += L"-!- Available commands: ";
				line += L"status ";
				if(is_operator)
				{
					line += L"shutdown setting ";
				}
				else
				{
				}
				send_to_sender = true;
				valid_command = true;
			}
			else if(message_s == "status")
			{
				line = getStatusString();
				send_to_sender = true;
				valid_command = true;
			}
			else if(is_operator)
			{
				if(message_s == "shutdown")
				{
					dstream<<DTIME<<" Server: Operator requested shutdown."
							<<std::endl;
					m_shutdown_requested.set(true);
					
					line += L"*** Server shutting down (operator request)";
					send_to_sender = true;
					valid_command = true;
				}
				else if(message_s.substr(0,8) == "setting ")
				{
					std::string confline = message_s.substr(8);
					g_settings.parseConfigLine(confline);
					line += L"-!- Setting changed.";
					send_to_sender = true;
					valid_command = true;
				}
			}
			
			if(valid_command == false)
			{
				line += L"-!- Invalid command: " + message;
				send_to_sender = true;
			}
		}
		else
		{
			line += L"<";
			/*if(is_operator)
				line += L"@";*/
			line += name;
			line += L"> ";
			line += message;
			send_to_others = true;
		}
		
		if(line != L"")
		{
			dstream<<"CHAT: "<<wide_to_narrow(line)<<std::endl;

			/*
				Send the message to clients
			*/
			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				// Get client and check that it is valid
				RemoteClient *client = i.getNode()->getValue();
				assert(client->peer_id == i.getNode()->getKey());
				if(client->serialization_version == SER_FMT_VER_INVALID)
					continue;

				// Filter recipient
				bool sender_selected = (peer_id == client->peer_id);
				if(sender_selected == true && send_to_sender == false)
					continue;
				if(sender_selected == false && send_to_others == false)
					continue;

				SendChatMessage(client->peer_id, line);
			}
		}
	}
	else
	{
		derr_server<<"WARNING: Server::ProcessData(): Ignoring "
				"unknown command "<<command<<std::endl;
	}
	
	} //try
	catch(SendFailedException &e)
	{
		derr_server<<"Server::ProcessData(): SendFailedException: "
				<<"what="<<e.what()
				<<std::endl;
	}
}

/*void Server::Send(u16 peer_id, u16 channelnum,
		SharedBuffer<u8> data, bool reliable)
{
	JMutexAutoLock lock(m_con_mutex);
	m_con.Send(peer_id, channelnum, data, reliable);
}*/

void Server::SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Create a packet with the block in the right format
	*/
	
	std::ostringstream os(std::ios_base::binary);
	block->serialize(os, ver);
	std::string s = os.str();
	SharedBuffer<u8> blockdata((u8*)s.c_str(), s.size());

	u32 replysize = 8 + blockdata.getSize();
	SharedBuffer<u8> reply(replysize);
	v3s16 p = block->getPos();
	writeU16(&reply[0], TOCLIENT_BLOCKDATA);
	writeS16(&reply[2], p.X);
	writeS16(&reply[4], p.Y);
	writeS16(&reply[6], p.Z);
	memcpy(&reply[8], *blockdata, blockdata.getSize());

	/*dstream<<"Sending block ("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<":  \tpacket size: "<<replysize<<std::endl;*/
	
	/*
		Send packet
	*/
	m_con.Send(peer_id, 1, reply, true);
}

core::list<PlayerInfo> Server::getPlayerInfo()
{
	DSTACK(__FUNCTION_NAME);
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);
	
	core::list<PlayerInfo> list;

	core::list<Player*> players = m_env.getPlayers();
	
	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		PlayerInfo info;

		Player *player = *i;

		try{
			con::Peer *peer = m_con.GetPeer(player->peer_id);
			// Copy info from peer to info struct
			info.id = peer->id;
			info.address = peer->address;
			info.avg_rtt = peer->avg_rtt;
		}
		catch(con::PeerNotFoundException &e)
		{
			// Set dummy peer info
			info.id = 0;
			info.address = Address(0,0,0,0,0);
			info.avg_rtt = 0.0;
		}

		snprintf(info.name, PLAYERNAME_SIZE, "%s", player->getName());
		info.position = player->getPosition();

		list.push_back(info);
	}

	return list;
}


void Server::peerAdded(con::Peer *peer)
{
	DSTACK(__FUNCTION_NAME);
	dout_server<<"Server::peerAdded(): peer->id="
			<<peer->id<<std::endl;
	
	PeerChange c;
	c.type = PEER_ADDED;
	c.peer_id = peer->id;
	c.timeout = false;
	m_peer_change_queue.push_back(c);
}

void Server::deletingPeer(con::Peer *peer, bool timeout)
{
	DSTACK(__FUNCTION_NAME);
	dout_server<<"Server::deletingPeer(): peer->id="
			<<peer->id<<", timeout="<<timeout<<std::endl;
	
	PeerChange c;
	c.type = PEER_REMOVED;
	c.peer_id = peer->id;
	c.timeout = timeout;
	m_peer_change_queue.push_back(c);
}

void Server::SendObjectData(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	core::map<v3s16, bool> stepped_blocks;
	
	for(core::map<u16, RemoteClient*>::Iterator
		i = m_clients.getIterator();
		i.atEnd() == false; i++)
	{
		u16 peer_id = i.getNode()->getKey();
		RemoteClient *client = i.getNode()->getValue();
		assert(client->peer_id == peer_id);
		
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;
		
		client->SendObjectData(this, dtime, stepped_blocks);
	}
}

void Server::SendPlayerInfos()
{
	DSTACK(__FUNCTION_NAME);

	//JMutexAutoLock envlock(m_env_mutex);
	
	// Get connected players
	core::list<Player*> players = m_env.getPlayers(true);
	
	u32 player_count = players.getSize();
	u32 datasize = 2+(2+PLAYERNAME_SIZE)*player_count;

	SharedBuffer<u8> data(datasize);
	writeU16(&data[0], TOCLIENT_PLAYERINFO);
	
	u32 start = 2;
	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		Player *player = *i;

		/*dstream<<"Server sending player info for player with "
				"peer_id="<<player->peer_id<<std::endl;*/
		
		writeU16(&data[start], player->peer_id);
		memset((char*)&data[start+2], 0, PLAYERNAME_SIZE);
		snprintf((char*)&data[start+2], PLAYERNAME_SIZE, "%s", player->getName());
		start += 2+PLAYERNAME_SIZE;
	}

	//JMutexAutoLock conlock(m_con_mutex);

	// Send as reliable
	m_con.SendToAll(0, data, true);
}

enum ItemSpecType
{
	ITEM_NONE,
	ITEM_MATERIAL,
	ITEM_CRAFT,
	ITEM_TOOL,
	ITEM_MBO
};

struct ItemSpec
{
	ItemSpec():
		type(ITEM_NONE)
	{
	}
	ItemSpec(enum ItemSpecType a_type, std::string a_name):
		type(a_type),
		name(a_name),
		num(65535)
	{
	}
	ItemSpec(enum ItemSpecType a_type, u16 a_num):
		type(a_type),
		name(""),
		num(a_num)
	{
	}
	enum ItemSpecType type;
	// Only other one of these is used
	std::string name;
	u16 num;
};

/*
	items: a pointer to an array of 9 pointers to items
	specs: a pointer to an array of 9 ItemSpecs
*/
bool checkItemCombination(InventoryItem **items, ItemSpec *specs)
{
	u16 items_min_x = 100;
	u16 items_max_x = 100;
	u16 items_min_y = 100;
	u16 items_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(items[y*3 + x] == NULL)
			continue;
		if(items_min_x == 100 || x < items_min_x)
			items_min_x = x;
		if(items_min_y == 100 || y < items_min_y)
			items_min_y = y;
		if(items_max_x == 100 || x > items_max_x)
			items_max_x = x;
		if(items_max_y == 100 || y > items_max_y)
			items_max_y = y;
	}
	// No items at all, just return false
	if(items_min_x == 100)
		return false;
	
	u16 items_w = items_max_x - items_min_x + 1;
	u16 items_h = items_max_y - items_min_y + 1;

	u16 specs_min_x = 100;
	u16 specs_max_x = 100;
	u16 specs_min_y = 100;
	u16 specs_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(specs[y*3 + x].type == ITEM_NONE)
			continue;
		if(specs_min_x == 100 || x < specs_min_x)
			specs_min_x = x;
		if(specs_min_y == 100 || y < specs_min_y)
			specs_min_y = y;
		if(specs_max_x == 100 || x > specs_max_x)
			specs_max_x = x;
		if(specs_max_y == 100 || y > specs_max_y)
			specs_max_y = y;
	}
	// No specs at all, just return false
	if(specs_min_x == 100)
		return false;

	u16 specs_w = specs_max_x - specs_min_x + 1;
	u16 specs_h = specs_max_y - specs_min_y + 1;

	// Different sizes
	if(items_w != specs_w || items_h != specs_h)
		return false;

	for(u16 y=0; y<specs_h; y++)
	for(u16 x=0; x<specs_w; x++)
	{
		u16 items_x = items_min_x + x;
		u16 items_y = items_min_y + y;
		u16 specs_x = specs_min_x + x;
		u16 specs_y = specs_min_y + y;
		InventoryItem *item = items[items_y * 3 + items_x];
		ItemSpec &spec = specs[specs_y * 3 + specs_x];
		
		if(spec.type == ITEM_NONE)
		{
			// Has to be no item
			if(item != NULL)
				return false;
			continue;
		}
		
		// There should be an item
		if(item == NULL)
			return false;

		std::string itemname = item->getName();

		if(spec.type == ITEM_MATERIAL)
		{
			if(itemname != "MaterialItem")
				return false;
			MaterialItem *mitem = (MaterialItem*)item;
			if(mitem->getMaterial() != spec.num)
				return false;
		}
		else if(spec.type == ITEM_CRAFT)
		{
			if(itemname != "CraftItem")
				return false;
			CraftItem *mitem = (CraftItem*)item;
			if(mitem->getSubName() != spec.name)
				return false;
		}
		else if(spec.type == ITEM_TOOL)
		{
			// Not supported yet
			assert(0);
		}
		else if(spec.type == ITEM_MBO)
		{
			// Not supported yet
			assert(0);
		}
		else
		{
			// Not supported yet
			assert(0);
		}
	}

	return true;
}

void Server::SendInventory(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	Player* player = m_env.getPlayer(peer_id);

	/*
		Calculate crafting stuff
	*/
	if(g_settings.getBool("creative_mode") == false)
	{
		InventoryList *clist = player->inventory.getList("craft");
		InventoryList *rlist = player->inventory.getList("craftresult");
		if(rlist)
		{
			rlist->clearItems();
		}
		if(clist && rlist)
		{
			InventoryItem *items[9];
			for(u16 i=0; i<9; i++)
			{
				items[i] = clist->getItem(i);
			}
			
			bool found = false;

			// Wood
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_MATERIAL, CONTENT_TREE);
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new MaterialItem(CONTENT_WOOD, 4));
					found = true;
				}
			}

			// Stick
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new CraftItem("Stick", 4));
					found = true;
				}
			}

			// Sign
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[1] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[2] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[3] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[4] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[5] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new MapBlockObjectItem("Sign"));
					found = true;
				}
			}

			// Torch
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_CRAFT, "lump_of_coal");
				specs[3] = ItemSpec(ITEM_CRAFT, "Stick");
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new MaterialItem(CONTENT_TORCH, 4));
					found = true;
				}
			}

			// Wooden pick
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[1] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[2] = ItemSpec(ITEM_MATERIAL, CONTENT_WOOD);
				specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
				specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new ToolItem("WPick", 0));
					found = true;
				}
			}

			// Stone pick
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_MATERIAL, CONTENT_STONE);
				specs[1] = ItemSpec(ITEM_MATERIAL, CONTENT_STONE);
				specs[2] = ItemSpec(ITEM_MATERIAL, CONTENT_STONE);
				specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
				specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new ToolItem("STPick", 0));
					found = true;
				}
			}

			// Mese pick
			if(!found)
			{
				ItemSpec specs[9];
				specs[0] = ItemSpec(ITEM_MATERIAL, CONTENT_MESE);
				specs[1] = ItemSpec(ITEM_MATERIAL, CONTENT_MESE);
				specs[2] = ItemSpec(ITEM_MATERIAL, CONTENT_MESE);
				specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
				specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
				if(checkItemCombination(items, specs))
				{
					rlist->addItem(new ToolItem("MesePick", 0));
					found = true;
				}
			}
		}
	} // if creative_mode == false

	/*
		Serialize it
	*/

	std::ostringstream os;
	//os.imbue(std::locale("C"));

	player->inventory.serialize(os);

	std::string s = os.str();
	
	SharedBuffer<u8> data(s.size()+2);
	writeU16(&data[0], TOCLIENT_INVENTORY);
	memcpy(&data[2], s.c_str(), s.size());
	
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendChatMessage(u16 peer_id, const std::wstring &message)
{
	DSTACK(__FUNCTION_NAME);
	
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOCLIENT_CHAT_MESSAGE);
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
	m_con.Send(peer_id, 0, data, true);
}

void Server::BroadcastChatMessage(const std::wstring &message)
{
	for(core::map<u16, RemoteClient*>::Iterator
		i = m_clients.getIterator();
		i.atEnd() == false; i++)
	{
		// Get client and check that it is valid
		RemoteClient *client = i.getNode()->getValue();
		assert(client->peer_id == i.getNode()->getKey());
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		SendChatMessage(client->peer_id, message);
	}
}

void Server::SendBlocks(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	//TimeTaker timer("Server::SendBlocks");

	core::array<PrioritySortedBlockTransfer> queue;

	s32 total_sending = 0;

	for(core::map<u16, RemoteClient*>::Iterator
		i = m_clients.getIterator();
		i.atEnd() == false; i++)
	{
		RemoteClient *client = i.getNode()->getValue();
		assert(client->peer_id == i.getNode()->getKey());

		total_sending += client->SendingCount();
		
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;
		
		client->GetNextBlocks(this, dtime, queue);
	}

	// Sort.
	// Lowest priority number comes first.
	// Lowest is most important.
	queue.sort();

	for(u32 i=0; i<queue.size(); i++)
	{
		//TODO: Calculate limit dynamically
		if(total_sending >= g_settings.getS32
				("max_simultaneous_block_sends_server_total"))
			break;
		
		PrioritySortedBlockTransfer q = queue[i];

		MapBlock *block = NULL;
		try
		{
			block = m_env.getMap().getBlockNoCreate(q.pos);
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		RemoteClient *client = getClient(q.peer_id);

		SendBlockNoLock(q.peer_id, block, client->serialization_version);

		client->SentBlock(q.pos);

		total_sending++;
	}
}


RemoteClient* Server::getClient(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	//JMutexAutoLock lock(m_con_mutex);
	core::map<u16, RemoteClient*>::Node *n;
	n = m_clients.find(peer_id);
	// A client should exist for all peers
	assert(n != NULL);
	return n->getValue();
}

std::wstring Server::getStatusString()
{
	std::wostringstream os(std::ios_base::binary);
	os<<L"# Server: ";
	// Uptime
	os<<L"uptime="<<m_uptime.get();
	// Information about clients
	os<<L", clients={";
	for(core::map<u16, RemoteClient*>::Iterator
		i = m_clients.getIterator();
		i.atEnd() == false; i++)
	{
		// Get client and check that it is valid
		RemoteClient *client = i.getNode()->getValue();
		assert(client->peer_id == i.getNode()->getKey());
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;
		// Get player
		Player *player = m_env.getPlayer(client->peer_id);
		// Get name of player
		std::wstring name = L"unknown";
		if(player != NULL)
			name = narrow_to_wide(player->getName());
		// Add name to information string
		os<<name<<L",";
	}
	os<<L"}";
	if(((ServerMap*)(&m_env.getMap()))->isSavingEnabled() == false)
		os<<" WARNING: Map saving is disabled."<<std::endl;
	return os.str();
}


void setCreativeInventory(Player *player)
{
	player->resetInventory();
	
	// Give some good picks
	{
		InventoryItem *item = new ToolItem("STPick", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem("MesePick", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}

	/*
		Give materials
	*/
	assert(USEFUL_CONTENT_COUNT <= PLAYER_INVENTORY_SIZE);
	
	// add torch first
	InventoryItem *item = new MaterialItem(CONTENT_TORCH, 1);
	player->inventory.addItem("main", item);
	
	// Then others
	for(u16 i=0; i<USEFUL_CONTENT_COUNT; i++)
	{
		// Skip some materials
		if(i == CONTENT_WATER || i == CONTENT_TORCH
			|| i == CONTENT_COALSTONE)
			continue;

		InventoryItem *item = new MaterialItem(i, 1);
		player->inventory.addItem("main", item);
	}
	// Sign
	{
		InventoryItem *item = new MapBlockObjectItem("Sign Example text");
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
}

Player *Server::emergePlayer(const char *name, const char *password,
		u16 peer_id)
{
	/*
		Try to get an existing player
	*/
	Player *player = m_env.getPlayer(name);
	if(player != NULL)
	{
		// If player is already connected, cancel
		if(player->peer_id != 0)
		{
			dstream<<"emergePlayer(): Player already connected"<<std::endl;
			return NULL;
		}

		// Got one.
		player->peer_id = peer_id;
		
		// Reset inventory to creative if in creative mode
		if(g_settings.getBool("creative_mode"))
		{
			setCreativeInventory(player);
		}

		return player;
	}

	/*
		If player with the wanted peer_id already exists, cancel.
	*/
	if(m_env.getPlayer(peer_id) != NULL)
	{
		dstream<<"emergePlayer(): Player with wrong name but same"
				" peer_id already exists"<<std::endl;
		return NULL;
	}
	
	/*
		Create a new player
	*/
	{
		player = new ServerRemotePlayer();
		//player->peer_id = c.peer_id;
		//player->peer_id = PEER_ID_INEXISTENT;
		player->peer_id = peer_id;
		player->updateName(name);

		/*
			Set player position
		*/
		
		dstream<<"Server: Finding spawn place for player \""
				<<player->getName()<<"\""<<std::endl;

		v2s16 nodepos;
#if 1
		player->setPosition(intToFloat(v3s16(
				0,
				45, //64,
				0
		), BS));
#endif
#if 0
		f32 groundheight = 0;
#if 0
		// Try to find a good place a few times
		for(s32 i=0; i<500; i++)
		{
			s32 range = 1 + i;
			// We're going to try to throw the player to this position
			nodepos = v2s16(-range + (myrand()%(range*2)),
					-range + (myrand()%(range*2)));
			v2s16 sectorpos = getNodeSectorPos(nodepos);
			// Get sector
			m_env.getMap().emergeSector(sectorpos);
			// Get ground height at point
			groundheight = m_env.getMap().getGroundHeight(nodepos, true);
			// The sector should have been generated -> groundheight exists
			assert(groundheight > GROUNDHEIGHT_VALID_MINVALUE);
			// Don't go underwater
			if(groundheight < WATER_LEVEL)
			{
				//dstream<<"-> Underwater"<<std::endl;
				continue;
			}
#if 0 // Doesn't work, generating blocks is a bit too complicated for doing here
			// Get block at point
			v3s16 nodepos3d;
			nodepos3d = v3s16(nodepos.X, groundheight+1, nodepos.Y);
			v3s16 blockpos = getNodeBlockPos(nodepos3d);
			((ServerMap*)(&m_env.getMap()))->emergeBlock(blockpos);
			// Don't go inside ground
			try{
				/*v3s16 footpos(nodepos.X, groundheight+1, nodepos.Y);
				v3s16 headpos(nodepos.X, groundheight+2, nodepos.Y);*/
				v3s16 footpos = nodepos3d + v3s16(0,0,0);
				v3s16 headpos = nodepos3d + v3s16(0,1,0);
				if(m_env.getMap().getNode(footpos).d != CONTENT_AIR
					|| m_env.getMap().getNode(headpos).d != CONTENT_AIR)
				{
					dstream<<"-> Inside ground"<<std::endl;
					// In ground
					continue;
				}
			}catch(InvalidPositionException &e)
			{
				dstream<<"-> Invalid position"<<std::endl;
				// Ignore invalid position
				continue;
			}
#endif
			// Found a good place
			dstream<<"Searched through "<<i<<" places."<<std::endl;
			break;
		}
#endif
		
		// If no suitable place was not found, go above water at least.
		if(groundheight < WATER_LEVEL)
			groundheight = WATER_LEVEL;

		player->setPosition(intToFloat(v3s16(
				nodepos.X,
				//groundheight + 1,
				groundheight + 15,
				nodepos.Y
		)));
#endif

		/*
			Add player to environment
		*/

		m_env.addPlayer(player);

		/*
			Add stuff to inventory
		*/
		
		if(g_settings.getBool("creative_mode"))
		{
			setCreativeInventory(player);
		}
		else
		{
			/*{
				InventoryItem *item = new ToolItem("WPick", 32000);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}*/
			/*{
				InventoryItem *item = new MaterialItem(CONTENT_MESE, 6);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}
			{
				InventoryItem *item = new MaterialItem(CONTENT_COALSTONE, 6);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}
			{
				InventoryItem *item = new MaterialItem(CONTENT_WOOD, 6);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}
			{
				InventoryItem *item = new CraftItem("Stick", 4);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}
			{
				InventoryItem *item = new ToolItem("WPick", 32000);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}
			{
				InventoryItem *item = new ToolItem("STPick", 32000);
				void* r = player->inventory.addItem("main", item);
				assert(r == NULL);
			}*/
			/*// Give some lights
			{
				InventoryItem *item = new MaterialItem(CONTENT_TORCH, 999);
				bool r = player->inventory.addItem("main", item);
				assert(r == true);
			}
			// and some signs
			for(u16 i=0; i<4; i++)
			{
				InventoryItem *item = new MapBlockObjectItem("Sign Example text");
				bool r = player->inventory.addItem("main", item);
				assert(r == true);
			}*/
			/*// Give some other stuff
			{
				InventoryItem *item = new MaterialItem(CONTENT_TREE, 999);
				bool r = player->inventory.addItem("main", item);
				assert(r == true);
			}*/
		}

		return player;
		
	} // create new player
}

#if 0
void Server::UpdateBlockWaterPressure(MapBlock *block,
			core::map<v3s16, MapBlock*> &modified_blocks)
{
	MapVoxelManipulator v(&m_env.getMap());
	v.m_disable_water_climb =
			g_settings.getBool("disable_water_climb");
	
	VoxelArea area(block->getPosRelative(),
			block->getPosRelative() + v3s16(1,1,1)*(MAP_BLOCKSIZE-1));

	try
	{
		v.updateAreaWaterPressure(area, m_flow_active_nodes);
	}
	catch(ProcessingLimitException &e)
	{
		dstream<<"Processing limit reached (1)"<<std::endl;
	}
	
	v.blitBack(modified_blocks);
}
#endif

void Server::handlePeerChange(PeerChange &c)
{
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);
	
	if(c.type == PEER_ADDED)
	{
		/*
			Add
		*/

		// Error check
		core::map<u16, RemoteClient*>::Node *n;
		n = m_clients.find(c.peer_id);
		// The client shouldn't already exist
		assert(n == NULL);

		// Create client
		RemoteClient *client = new RemoteClient();
		client->peer_id = c.peer_id;
		m_clients.insert(client->peer_id, client);

	} // PEER_ADDED
	else if(c.type == PEER_REMOVED)
	{
		/*
			Delete
		*/

		// Error check
		core::map<u16, RemoteClient*>::Node *n;
		n = m_clients.find(c.peer_id);
		// The client should exist
		assert(n != NULL);
		
		// Collect information about leaving in chat
		std::wstring message;
		{
			std::wstring name = L"unknown";
			Player *player = m_env.getPlayer(c.peer_id);
			if(player != NULL)
				name = narrow_to_wide(player->getName());
			
			message += L"*** ";
			message += name;
			message += L" left game";
			if(c.timeout)
				message += L" (timed out)";
		}

		/*// Delete player
		{
			m_env.removePlayer(c.peer_id);
		}*/

		// Set player client disconnected
		{
			Player *player = m_env.getPlayer(c.peer_id);
			if(player != NULL)
				player->peer_id = 0;
		}
		
		// Delete client
		delete m_clients[c.peer_id];
		m_clients.remove(c.peer_id);

		// Send player info to all remaining clients
		SendPlayerInfos();
		
		// Send leave chat message to all remaining clients
		BroadcastChatMessage(message);
		
	} // PEER_REMOVED
	else
	{
		assert(0);
	}
}

void Server::handlePeerChanges()
{
	while(m_peer_change_queue.size() > 0)
	{
		PeerChange c = m_peer_change_queue.pop_front();

		dout_server<<"Server: Handling peer change: "
				<<"id="<<c.peer_id<<", timeout="<<c.timeout
				<<std::endl;

		handlePeerChange(c);
	}
}

void dedicated_server_loop(Server &server, bool &kill)
{
	DSTACK(__FUNCTION_NAME);
	
	std::cout<<DTIME<<std::endl;
	std::cout<<"========================"<<std::endl;
	std::cout<<"Running dedicated server"<<std::endl;
	std::cout<<"========================"<<std::endl;
	std::cout<<std::endl;

	for(;;)
	{
		// This is kind of a hack but can be done like this
		// because server.step() is very light
		sleep_ms(30);
		server.step(0.030);

		if(server.getShutdownRequested() || kill)
		{
			std::cout<<DTIME<<" dedicated_server_loop(): Quitting."<<std::endl;
			break;
		}

		static int counter = 0;
		counter--;
		if(counter <= 0)
		{
			counter = 10;

			core::list<PlayerInfo> list = server.getPlayerInfo();
			core::list<PlayerInfo>::Iterator i;
			static u32 sum_old = 0;
			u32 sum = PIChecksum(list);
			if(sum != sum_old)
			{
				std::cout<<DTIME<<"Player info:"<<std::endl;
				for(i=list.begin(); i!=list.end(); i++)
				{
					i->PrintLine(&std::cout);
				}
			}
			sum_old = sum;
		}
	}
}


