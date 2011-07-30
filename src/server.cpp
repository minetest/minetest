/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "config.h"
#include "servercommand.h"
#include "filesys.h"
#include "content_mapnode.h"
#include "content_craft.h"
#include "content_nodemeta.h"
#include "mapblock.h"
#include "serverobject.h"

#define BLOCK_EMERGE_FLAG_FROMDISK (1<<0)

class MapEditEventIgnorer
{
public:
	MapEditEventIgnorer(bool *flag):
		m_flag(flag)
	{
		if(*m_flag == false)
			*m_flag = true;
		else
			m_flag = NULL;
	}

	~MapEditEventIgnorer()
	{
		if(m_flag)
		{
			assert(*m_flag);
			*m_flag = false;
		}
	}
	
private:
	bool *m_flag;
};

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

	BEGIN_DEBUG_EXCEPTION_HANDLER

	bool enable_mapgen_debug_info = g_settings.getBool("enable_mapgen_debug_info");
	
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
		v2s16 p2d(p.X,p.Z);

		/*
			Do not generate over-limit
		*/
		if(p.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
		|| p.X > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
		|| p.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
		|| p.Y > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
		|| p.Z < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
		|| p.Z > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE)
			continue;
			
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

		bool only_from_disk = true;

		{
			core::map<u16, u8>::Iterator i;
			for(i=q->peer_ids.getIterator(); i.atEnd()==false; i++)
			{
				//u16 peer_id = i.getNode()->getKey();

				// Check flags
				u8 flags = i.getNode()->getValue();
				if((flags & BLOCK_EMERGE_FLAG_FROMDISK) == false)
					only_from_disk = false;
				
			}
		}
		
		if(enable_mapgen_debug_info)
			dstream<<"EmergeThread: p="
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<") "
					<<"only_from_disk="<<only_from_disk<<std::endl;
		
		ServerMap &map = ((ServerMap&)m_server->m_env.getMap());
			
		//core::map<v3s16, MapBlock*> changed_blocks;
		//core::map<v3s16, MapBlock*> lighting_invalidated_blocks;

		MapBlock *block = NULL;
		bool got_block = true;
		core::map<v3s16, MapBlock*> modified_blocks;
		
		/*
			Fetch block from map or generate a single block
		*/
		{
			JMutexAutoLock envlock(m_server->m_env_mutex);
			
			// Load sector if it isn't loaded
			if(map.getSectorNoGenerateNoEx(p2d) == NULL)
				//map.loadSectorFull(p2d);
				map.loadSectorMeta(p2d);

			block = map.getBlockNoCreateNoEx(p);
			if(!block || block->isDummy() || !block->isGenerated())
			{
				if(enable_mapgen_debug_info)
					dstream<<"EmergeThread: not in memory, loading"<<std::endl;

				// Get, load or create sector
				/*ServerMapSector *sector =
						(ServerMapSector*)map.createSector(p2d);*/

				// Load/generate block

				/*block = map.emergeBlock(p, sector, changed_blocks,
						lighting_invalidated_blocks);*/

				block = map.loadBlock(p);
				
				if(only_from_disk == false)
				{
					if(block == NULL || block->isGenerated() == false)
					{
						if(enable_mapgen_debug_info)
							dstream<<"EmergeThread: generating"<<std::endl;
						block = map.generateBlock(p, modified_blocks);
					}
				}

				if(enable_mapgen_debug_info)
					dstream<<"EmergeThread: ended up with: "
							<<analyze_block(block)<<std::endl;

				if(block == NULL)
				{
					got_block = false;
				}
				else
				{
					/*
						Ignore map edit events, they will not need to be
						sent to anybody because the block hasn't been sent
						to anybody
					*/
					MapEditEventIgnorer ign(&m_server->m_ignore_map_edit_events);
					
					// Activate objects and stuff
					m_server->m_env.activateBlock(block, 3600);
				}
			}
			else
			{
				/*if(block->getLightingExpired()){
					lighting_invalidated_blocks[block->getPos()] = block;
				}*/
			}

			// TODO: Some additional checking and lighting updating,
			//       see emergeBlock
		}

		{//envlock
		JMutexAutoLock envlock(m_server->m_env_mutex);
		
		if(got_block)
		{
			/*
				Collect a list of blocks that have been modified in
				addition to the fetched one.
			*/

#if 0
			if(lighting_invalidated_blocks.size() > 0)
			{
				/*dstream<<"lighting "<<lighting_invalidated_blocks.size()
						<<" blocks"<<std::endl;*/
			
				// 50-100ms for single block generation
				//TimeTaker timer("** EmergeThread updateLighting");
				
				// Update lighting without locking the environment mutex,
				// add modified blocks to changed blocks
				map.updateLighting(lighting_invalidated_blocks, modified_blocks);
			}
				
			// Add all from changed_blocks to modified_blocks
			for(core::map<v3s16, MapBlock*>::Iterator i = changed_blocks.getIterator();
					i.atEnd() == false; i++)
			{
				MapBlock *block = i.getNode()->getValue();
				modified_blocks.insert(block->getPos(), block);
			}
#endif
		}
		// If we got no block, there should be no invalidated blocks
		else
		{
			//assert(lighting_invalidated_blocks.size() == 0);
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
	
	/*u32 timer_result;
	TimeTaker timer("RemoteClient::GetNextBlocks", &timer_result);*/
	
	// Increment timers
	m_nothing_to_send_pause_timer -= dtime;
	
	if(m_nothing_to_send_pause_timer >= 0)
	{
		// Keep this reset
		m_nearest_unsent_reset_timer = 0;
		return;
	}

	// Won't send anything if already sending
	if(m_blocks_sending.size() >= g_settings.getU16
			("max_simultaneous_block_sends_per_client"))
	{
		//dstream<<"Not sending any blocks, Queue full."<<std::endl;
		return;
	}

	//TimeTaker timer("RemoteClient::GetNextBlocks");
	
	Player *player = server->m_env.getPlayer(peer_id);

	assert(player != NULL);

	v3f playerpos = player->getPosition();
	v3f playerspeed = player->getSpeed();
	v3f playerspeeddir(0,0,0);
	if(playerspeed.getLength() > 1.0*BS)
		playerspeeddir = playerspeed / playerspeed.getLength();
	// Predict to next block
	v3f playerpos_predicted = playerpos + playerspeeddir*MAP_BLOCKSIZE*BS;

	v3s16 center_nodepos = floatToInt(playerpos_predicted, BS);

	v3s16 center = getNodeBlockPos(center_nodepos);
	
	// Camera position and direction
	v3f camera_pos =
			playerpos + v3f(0, BS+BS/2, 0);
	v3f camera_dir = v3f(0,0,1);
	camera_dir.rotateYZBy(player->getPitch());
	camera_dir.rotateXZBy(player->getYaw());

	/*dstream<<"camera_dir=("<<camera_dir.X<<","<<camera_dir.Y<<","
			<<camera_dir.Z<<")"<<std::endl;*/

	/*
		Get the starting value of the block finder radius.
	*/
		
	if(m_last_center != center)
	{
		m_nearest_unsent_d = 0;
		m_last_center = center;
	}

	/*dstream<<"m_nearest_unsent_reset_timer="
			<<m_nearest_unsent_reset_timer<<std::endl;*/
			
	// This has to be incremented only when the nothing to send pause
	// is not active
	m_nearest_unsent_reset_timer += dtime;
	
	// Reset periodically to avoid possible bugs or other mishaps
	if(m_nearest_unsent_reset_timer > 10.0)
	{
		m_nearest_unsent_reset_timer = 0;
		m_nearest_unsent_d = 0;
		/*dstream<<"Resetting m_nearest_unsent_d for "
				<<server->getPlayerName(peer_id)<<std::endl;*/
	}

	//s16 last_nearest_unsent_d = m_nearest_unsent_d;
	s16 d_start = m_nearest_unsent_d;

	//dstream<<"d_start="<<d_start<<std::endl;

	u16 max_simul_sends_setting = g_settings.getU16
			("max_simultaneous_block_sends_per_client");
	u16 max_simul_sends_usually = max_simul_sends_setting;

	/*
		Check the time from last addNode/removeNode.
		
		Decrease send rate if player is building stuff.
	*/
	m_time_from_building += dtime;
	if(m_time_from_building < g_settings.getFloat(
				"full_block_send_enable_min_time_from_building"))
	{
		max_simul_sends_usually
			= LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS;
	}
	
	/*
		Number of blocks sending + number of blocks selected for sending
	*/
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
	
	// Don't loop very much at a time
	if(d_max > d_start+1)
		d_max = d_start+1;
	/*if(d_max_gen > d_start+2)
		d_max_gen = d_start+2;*/
	
	//dstream<<"Starting from "<<d_start<<std::endl;

	bool sending_something = false;

	bool no_blocks_found_for_sending = true;

	bool queue_is_full = false;
	
	s16 d;
	for(d = d_start; d <= d_max; d++)
	{
		//dstream<<"RemoteClient::SendBlocks(): d="<<d<<std::endl;
		
		/*
			If m_nearest_unsent_d was changed by the EmergeThread
			(it can change it to 0 through SetBlockNotSent),
			update our d to it.
			Else update m_nearest_unsent_d
		*/
		/*if(m_nearest_unsent_d != last_nearest_unsent_d)
		{
			d = m_nearest_unsent_d;
			last_nearest_unsent_d = m_nearest_unsent_d;
		}*/

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
			
			// Start with the usual maximum
			u16 max_simul_dynamic = max_simul_sends_usually;
			
			// If block is very close, allow full maximum
			if(d <= BLOCK_SEND_DISABLE_LIMITS_MAX_D)
				max_simul_dynamic = max_simul_sends_setting;

			// Don't select too many blocks for sending
			if(num_blocks_selected >= max_simul_dynamic)
			{
				queue_is_full = true;
				goto queue_full_break;
			}
			
			// Don't send blocks that are currently being transferred
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

#if 1
			/*
				If block is far away, don't generate it unless it is
				near ground level.
			*/
			if(d >= 4)
			{
	#if 1
				// Block center y in nodes
				f32 y = (f32)(p.Y * MAP_BLOCKSIZE + MAP_BLOCKSIZE/2);
				// Don't generate if it's very high or very low
				if(y < -64 || y > 64)
					generate = false;
	#endif
	#if 0
				v2s16 p2d_nodes_center(
					MAP_BLOCKSIZE*p.X,
					MAP_BLOCKSIZE*p.Z);
				
				// Get ground height in nodes
				s16 gh = server->m_env.getServerMap().findGroundLevel(
						p2d_nodes_center);

				// If differs a lot, don't generate
				if(fabs(gh - y) > MAP_BLOCKSIZE*2)
					generate = false;
					// Actually, don't even send it
					//continue;
	#endif
			}
#endif

			//dstream<<"d="<<d<<std::endl;
			
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
				{
					continue;
				}
			}

			/*
				Check if map has this block
			*/
			MapBlock *block = server->m_env.getMap().getBlockNoCreateNoEx(p);
			
			bool surely_not_found_on_disk = false;
			bool block_is_invalid = false;
			if(block != NULL)
			{
				// Reset usage timer, this block will be of use in the future.
				block->resetUsageTimer();

				// Block is dummy if data doesn't exist.
				// It means it has been not found from disk and not generated
				if(block->isDummy())
				{
					surely_not_found_on_disk = true;
				}
				
				// Block is valid if lighting is up-to-date and data exists
				if(block->isValid() == false)
				{
					block_is_invalid = true;
				}
				
				/*if(block->isFullyGenerated() == false)
				{
					block_is_invalid = true;
				}*/

#if 0
				v2s16 p2d(p.X, p.Z);
				ServerMap *map = (ServerMap*)(&server->m_env.getMap());
				v2s16 chunkpos = map->sector_to_chunk(p2d);
				if(map->chunkNonVolatile(chunkpos) == false)
					block_is_invalid = true;
#endif
				if(block->isGenerated() == false)
					block_is_invalid = true;
#if 1
				/*
					If block is not close, don't send it unless it is near
					ground level.

					Block is near ground level if night-time mesh
					differs from day-time mesh.
				*/
				if(d > 3)
				{
					if(block->dayNightDiffed() == false)
						continue;
				}
#endif
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
				Record the lowest d from which a block has been
				found being not sent and possibly to exist
			*/
			if(no_blocks_found_for_sending)
			{
				if(generate == true)
					new_nearest_unsent_d = d;
			}

			no_blocks_found_for_sending = false;
					
			/*
				Add inexistent block to emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || block_is_invalid)
			{
				//TODO: Get value from somewhere
				// Allow only one block in emerge queue
				//if(server->m_emerge_queue.peerItemCount(peer_id) < 1)
				// Allow two blocks in queue per client
				if(server->m_emerge_queue.peerItemCount(peer_id) < 2)
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
			sending_something = true;
		}
	}
queue_full_break:

	//dstream<<"Stopped at "<<d<<std::endl;
	
	if(no_blocks_found_for_sending)
	{
		if(queue_is_full == false)
			new_nearest_unsent_d = d;
	}

	if(new_nearest_unsent_d != -1)
		m_nearest_unsent_d = new_nearest_unsent_d;

	if(sending_something == false)
	{
		m_nothing_to_send_counter++;
		if((s16)m_nothing_to_send_counter >=
				g_settings.getS16("max_block_send_distance"))
		{
			// Pause time in seconds
			m_nothing_to_send_pause_timer = 1.0;
			/*dstream<<"nothing to send to "
					<<server->getPlayerName(peer_id)
					<<" (d="<<d<<")"<<std::endl;*/
		}
	}
	else
	{
		m_nothing_to_send_counter = 0;
	}

	/*timer_result = timer.stop(true);
	if(timer_result != 0)
		dstream<<"GetNextBlocks duration: "<<timer_result<<" (!=0)"<<std::endl;*/
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
				block->stepObjects(dtime, true, server->m_env.getDayNightRatio());
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
			//block->serializeObjects(bos, serialization_version); // DEPRECATED
			// count=0
			writeU16(bos, 0);

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
	avg_rtt = 0;
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
		std::string mapsavedir,
		std::string configpath
	):
	m_env(new ServerMap(mapsavedir), this),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this),
	m_authmanager(mapsavedir+"/auth.txt"),
	m_thread(this),
	m_emergethread(this),
	m_time_counter(0),
	m_time_of_day_send_timer(0),
	m_uptime(0),
	m_mapsavedir(mapsavedir),
	m_configpath(configpath),
	m_shutdown_requested(false),
	m_ignore_map_edit_events(false),
	m_ignore_map_edit_events_peer_id(0)
{
	m_liquid_transform_timer = 0.0;
	m_print_info_timer = 0.0;
	m_objectdata_timer = 0.0;
	m_emergethread_trigger_timer = 0.0;
	m_savemap_timer = 0.0;
	
	m_env_mutex.Init();
	m_con_mutex.Init();
	m_step_dtime_mutex.Init();
	m_step_dtime = 0.0;
	
	// Register us to receive map edit events
	m_env.getMap().addEventReceiver(this);

	// If file exists, load environment metadata
	if(fs::PathExists(m_mapsavedir+"/env_meta.txt"))
	{
		dstream<<"Server: Loading environment metadata"<<std::endl;
		m_env.loadMeta(m_mapsavedir);
	}

	// Load players
	dstream<<"Server: Loading players"<<std::endl;
	m_env.deSerializePlayers(m_mapsavedir);
}

Server::~Server()
{
	dstream<<"Server::~Server()"<<std::endl;

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

			try{
				SendChatMessage(client->peer_id, line);
			}
			catch(con::PeerNotFoundException &e)
			{}
		}
	}

	/*
		Save players
	*/
	dstream<<"Server: Saving players"<<std::endl;
	m_env.serializePlayers(m_mapsavedir);

	/*
		Save environment metadata
	*/
	dstream<<"Server: Saving environment metadata"<<std::endl;
	m_env.saveMeta(m_mapsavedir);
	
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
	
	dout_server<<"Server: Started on port "<<port<<std::endl;
}

void Server::stop()
{
	DSTACK(__FUNCTION_NAME);

	// Stop threads (set run=false first so both start stopping)
	m_thread.setRun(false);
	m_emergethread.setRun(false);
	m_thread.stop();
	m_emergethread.stop();
	
	dout_server<<"Server: Threads stopped"<<std::endl;
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
	
	{
		ScopeProfiler sp(&g_profiler, "Server: selecting and sending "
				"blocks to clients");
		// Send blocks to clients
		SendBlocks(dtime);
	}
	
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
		Update m_time_of_day and overall game time
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);

		m_time_counter += dtime;
		f32 speed = g_settings.getFloat("time_speed") * 24000./(24.*3600);
		u32 units = (u32)(m_time_counter*speed);
		m_time_counter -= (f32)units / speed;
		
		m_env.setTimeOfDay((m_env.getTimeOfDay() + units) % 24000);
		
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
						m_env.getTimeOfDay());
				// Send as reliable
				m_con.Send(client->peer_id, 0, data, true);
			}
		}
	}

	{
		// Process connection's timeouts
		JMutexAutoLock lock2(m_con_mutex);
		ScopeProfiler sp(&g_profiler, "Server: connection timeout processing");
		m_con.RunTimeouts(dtime);
	}
	
	{
		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		ScopeProfiler sp(&g_profiler, "Server: peer change handling");
		handlePeerChanges();
	}

	{
		JMutexAutoLock lock(m_env_mutex);
		// Step environment
		ScopeProfiler sp(&g_profiler, "Server: environment step");
		m_env.step(dtime);
	}
		
	const float map_timer_and_unload_dtime = 5.15;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime))
	{
		JMutexAutoLock lock(m_env_mutex);
		// Run Map's timers and unload unused data
		ScopeProfiler sp(&g_profiler, "Server: map timer and unload");
		m_env.getMap().timerUpdate(map_timer_and_unload_dtime,
				g_settings.getFloat("server_unload_unused_data_timeout"));
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

		ScopeProfiler sp(&g_profiler, "Server: liquid transform");

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
				Player *player = m_env.getPlayer(client->peer_id);
				if(player==NULL)
					continue;
				std::cout<<player->getName()<<"\t";
				client->PrintInfo(std::cout);
			}
		}
	}

	//if(g_settings.getBool("enable_experimental"))
	{

	/*
		Check added and deleted active objects
	*/
	{
		//dstream<<"Server: Checking added and deleted active objects"<<std::endl;
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		ScopeProfiler sp(&g_profiler, "Server: checking added and deleted objects");

		// Radius inside which objects are active
		s16 radius = 32;

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			Player *player = m_env.getPlayer(client->peer_id);
			if(player==NULL)
			{
				// This can happen if the client timeouts somehow
				/*dstream<<"WARNING: "<<__FUNCTION_NAME<<": Client "
						<<client->peer_id
						<<" has no associated player"<<std::endl;*/
				continue;
			}
			v3s16 pos = floatToInt(player->getPosition(), BS);

			core::map<u16, bool> removed_objects;
			core::map<u16, bool> added_objects;
			m_env.getRemovedActiveObjects(pos, radius,
					client->m_known_objects, removed_objects);
			m_env.getAddedActiveObjects(pos, radius,
					client->m_known_objects, added_objects);
			
			// Ignore if nothing happened
			if(removed_objects.size() == 0 && added_objects.size() == 0)
			{
				//dstream<<"INFO: active objects: none changed"<<std::endl;
				continue;
			}
			
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
				
				if(obj)
					data_buffer.append(serializeLongString(
							obj->getClientInitializationData()));
				else
					data_buffer.append(serializeLongString(""));

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

#if 0
		/*
			Collect a list of all the objects known by the clients
			and report it back to the environment.
		*/

		core::map<u16, bool> all_known_objects;

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			// Go through all known objects of client
			for(core::map<u16, bool>::Iterator
					i = client->m_known_objects.getIterator();
					i.atEnd()==false; i++)
			{
				u16 id = i.getNode()->getKey();
				all_known_objects[id] = true;
			}
		}
		
		m_env.setKnownActiveObjects(whatever);
#endif

	}

	/*
		Send object messages
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		ScopeProfiler sp(&g_profiler, "Server: sending object messages");

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
					// Add object id
					char buf[2];
					writeU16((u8*)&buf[0], aom.id);
					new_data.append(buf, 2);
					// Add data
					new_data += serializeString(aom.datastring);
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

			/*if(reliable_data.size() > 0 || unreliable_data.size() > 0)
			{
				dstream<<"INFO: Server: Size of object message data: "
						<<"reliable: "<<reliable_data.size()
						<<", unreliable: "<<unreliable_data.size()
						<<std::endl;
			}*/
		}

		// Clear buffered_messages
		for(core::map<u16, core::list<ActiveObjectMessage>* >::Iterator
				i = buffered_messages.getIterator();
				i.atEnd()==false; i++)
		{
			delete i.getNode()->getValue();
		}
	}

	} // enable_experimental

	/*
		Send queued-for-sending map edit events.
	*/
	{
		// Don't send too many at a time
		//u32 count = 0;

		// Single change sending is disabled if queue size is not small
		bool disable_single_change_sending = false;
		if(m_unsent_map_edit_queue.size() >= 4)
			disable_single_change_sending = true;

		bool got_any_events = false;

		// We'll log the amount of each
		Profiler prof;

		while(m_unsent_map_edit_queue.size() != 0)
		{
			got_any_events = true;

			MapEditEvent* event = m_unsent_map_edit_queue.pop_front();
			
			// Players far away from the change are stored here.
			// Instead of sending the changes, MapBlocks are set not sent
			// for them.
			core::list<u16> far_players;

			if(event->type == MEET_ADDNODE)
			{
				//dstream<<"Server: MEET_ADDNODE"<<std::endl;
				prof.add("MEET_ADDNODE", 1);
				if(disable_single_change_sending)
					sendAddNode(event->p, event->n, event->already_known_by_peer,
							&far_players, 5);
				else
					sendAddNode(event->p, event->n, event->already_known_by_peer,
							&far_players, 30);
			}
			else if(event->type == MEET_REMOVENODE)
			{
				//dstream<<"Server: MEET_REMOVENODE"<<std::endl;
				prof.add("MEET_REMOVENODE", 1);
				if(disable_single_change_sending)
					sendRemoveNode(event->p, event->already_known_by_peer,
							&far_players, 5);
				else
					sendRemoveNode(event->p, event->already_known_by_peer,
							&far_players, 30);
			}
			else if(event->type == MEET_BLOCK_NODE_METADATA_CHANGED)
			{
				dstream<<"Server: MEET_BLOCK_NODE_METADATA_CHANGED"<<std::endl;
				prof.add("MEET_BLOCK_NODE_METADATA_CHANGED", 1);
				setBlockNotSent(event->p);
			}
			else if(event->type == MEET_OTHER)
			{
				prof.add("MEET_OTHER", 1);
				dstream<<"WARNING: Server: MEET_OTHER not implemented"
						<<std::endl;
			}
			else
			{
				prof.add("unknown", 1);
				dstream<<"WARNING: Server: Unknown MapEditEvent "
						<<((u32)event->type)<<std::endl;
			}
			
			/*
				Set blocks not sent to far players
			*/
			if(far_players.size() > 0)
			{
				// Convert list format to that wanted by SetBlocksNotSent
				core::map<v3s16, MapBlock*> modified_blocks2;
				for(core::map<v3s16, bool>::Iterator
						i = event->modified_blocks.getIterator();
						i.atEnd()==false; i++)
				{
					v3s16 p = i.getNode()->getKey();
					modified_blocks2.insert(p,
							m_env.getMap().getBlockNoCreateNoEx(p));
				}
				// Set blocks not sent
				for(core::list<u16>::Iterator
						i = far_players.begin();
						i != far_players.end(); i++)
				{
					u16 peer_id = *i;
					RemoteClient *client = getClient(peer_id);
					if(client==NULL)
						continue;
					client->SetBlocksNotSent(modified_blocks2);
				}
			}

			delete event;

			/*// Don't send too many at a time
			count++;
			if(count >= 1 && m_unsent_map_edit_queue.size() < 100)
				break;*/
		}

		if(got_any_events)
		{
			dstream<<"Server: MapEditEvents:"<<std::endl;
			prof.print(dstream);
		}
		
	}

	/*
		Send object positions
		TODO: Get rid of MapBlockObjects
	*/
	{
		float &counter = m_objectdata_timer;
		counter += dtime;
		if(counter >= g_settings.getFloat("objectdata_interval"))
		{
			JMutexAutoLock lock1(m_env_mutex);
			JMutexAutoLock lock2(m_con_mutex);

			ScopeProfiler sp(&g_profiler, "Server: sending mbo positions");

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

	// Save map, players and auth stuff
	{
		float &counter = m_savemap_timer;
		counter += dtime;
		if(counter >= g_settings.getFloat("server_map_save_interval"))
		{
			counter = 0.0;

			ScopeProfiler sp(&g_profiler, "Server: saving stuff");

			// Auth stuff
			if(m_authmanager.isModified())
				m_authmanager.save();
			
			// Map
			JMutexAutoLock lock(m_env_mutex);

			/*// Unload unused data (delete from memory)
			m_env.getMap().unloadUnusedData(
					g_settings.getFloat("server_unload_unused_sectors_timeout"));
					*/
			/*u32 deleted_count = m_env.getMap().unloadUnusedData(
					g_settings.getFloat("server_unload_unused_sectors_timeout"));
					*/

			// Save only changed parts
			m_env.getMap().save(true);

			/*if(deleted_count > 0)
			{
				dout_server<<"Server: Unloaded "<<deleted_count
						<<" blocks from memory"<<std::endl;
			}*/

			// Save players
			m_env.serializePlayers(m_mapsavedir);
			
			// Save environment metadata
			m_env.saveMeta(m_mapsavedir);
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
		// [23] u8[28] password <--- can be sent without this, from old versions

		if(datasize < 2+1+PLAYERNAME_SIZE)
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
		char playername[PLAYERNAME_SIZE];
		for(u32 i=0; i<PLAYERNAME_SIZE-1; i++)
		{
			playername[i] = data[3+i];
		}
		playername[PLAYERNAME_SIZE-1] = 0;
		
		if(playername[0]=='\0')
		{
			derr_server<<DTIME<<"Server: Player has empty name"<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Empty name");
			return;
		}

		if(string_allowed(playername, PLAYERNAME_ALLOWED_CHARS)==false)
		{
			derr_server<<DTIME<<"Server: Player has invalid name"<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Name contains unallowed characters");
			return;
		}

		// Get password
		char password[PASSWORD_SIZE];
		if(datasize == 2+1+PLAYERNAME_SIZE)
		{
			// old version - assume blank password
			password[0] = 0;
		}
		else
		{
				for(u32 i=0; i<PASSWORD_SIZE-1; i++)
				{
					password[i] = data[23+i];
				}
				password[PASSWORD_SIZE-1] = 0;
		}
		
		std::string checkpwd;
		if(m_authmanager.exists(playername))
		{
			checkpwd = m_authmanager.getPassword(playername);
		}
		else
		{
			checkpwd = g_settings.get("default_password");
		}
		
		if(password != checkpwd && checkpwd != "")
		{
			derr_server<<DTIME<<"Server: peer_id="<<peer_id
					<<": supplied invalid password for "
					<<playername<<std::endl;
			SendAccessDenied(m_con, peer_id, L"Invalid password");
			return;
		}
		
		// Add player to auth manager
		if(m_authmanager.exists(playername) == false)
		{
			derr_server<<DTIME<<"Server: adding player "<<playername
					<<" to auth manager"<<std::endl;
			m_authmanager.add(playername);
			m_authmanager.setPassword(playername, checkpwd);
			m_authmanager.setPrivs(playername,
					stringToPrivs(g_settings.get("default_privs")));
			m_authmanager.save();
		}

		// Get player
		Player *player = emergePlayer(playername, password, peer_id);


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
		
		/*
			Answer with a TOCLIENT_INIT
		*/
		{
			SharedBuffer<u8> reply(2+1+6+8);
			writeU16(&reply[0], TOCLIENT_INIT);
			writeU8(&reply[2], deployed);
			writeV3S16(&reply[2+1], floatToInt(player->getPosition()+v3f(0,BS/2,0), BS));
			writeU64(&reply[2+1+6], m_env.getServerMap().getSeed());
			
			// Send as reliable
			m_con.Send(peer_id, 0, reply, true);
		}

		/*
			Send complete position information
		*/
		SendMovePlayer(player);

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
		UpdateCrafting(peer->id);
		SendInventory(peer->id);

		// Send HP
		{
			Player *player = m_env.getPlayer(peer_id);
			SendPlayerHP(player);
		}
		
		// Send time of day
		{
			SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
					m_env.getTimeOfDay());
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

		if((getPlayerPrivs(player) & PRIV_BUILD) == 0)
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
				UpdateCrafting(player->peer_id);
				SendInventory(player->peer_id);
			}

			// Remove from block
			block->removeObject(id);
		}
	}
	else if(command == TOSERVER_CLICK_ACTIVEOBJECT)
	{
		if(datasize < 7)
			return;

		if((getPlayerPrivs(player) & PRIV_BUILD) == 0)
			return;

		/*
			length: 7
			[0] u16 command
			[2] u8 button (0=left, 1=right)
			[3] u16 id
			[5] u16 item
		*/
		u8 button = readU8(&data[2]);
		u16 id = readS16(&data[3]);
		u16 item_i = readU16(&data[11]);
	
		ServerActiveObject *obj = m_env.getActiveObject(id);

		if(obj == NULL)
		{
			derr_server<<"Server: CLICK_ACTIVEOBJECT: object not found"
					<<std::endl;
			return;
		}

		// Skip if object has been removed
		if(obj->m_removed)
			return;
		
		//TODO: Check that object is reasonably close
		
		// Left click, pick object up (usually)
		if(button == 0)
		{
			/*
				Try creating inventory item
			*/
			InventoryItem *item = obj->createPickedUpItem();
			
			if(item)
			{
				InventoryList *ilist = player->inventory.getList("main");
				if(ilist != NULL)
				{
					if(g_settings.getBool("creative_mode") == false)
					{
						// Skip if inventory has no free space
						if(ilist->getUsedSlots() == ilist->getSize())
						{
							dout_server<<"Player inventory has no free space"<<std::endl;
							return;
						}

						// Add to inventory and send inventory
						ilist->addItem(item);
						UpdateCrafting(player->peer_id);
						SendInventory(player->peer_id);
					}

					// Remove object from environment
					obj->m_removed = true;
				}
			}
			else
			{
				/*
					Item cannot be picked up. Punch it instead.
				*/

				ToolItem *titem = NULL;
				std::string toolname = "";

				InventoryList *mlist = player->inventory.getList("main");
				if(mlist != NULL)
				{
					InventoryItem *item = mlist->getItem(item_i);
					if(item && (std::string)item->getName() == "ToolItem")
					{
						titem = (ToolItem*)item;
						toolname = titem->getToolName();
					}
				}

				v3f playerpos = player->getPosition();
				v3f objpos = obj->getBasePosition();
				v3f dir = (objpos - playerpos).normalize();
				
				u16 wear = obj->punch(toolname, dir);
				
				if(titem)
				{
					bool weared_out = titem->addWear(wear);
					if(weared_out)
						mlist->deleteItem(item_i);
					SendInventory(player->peer_id);
				}
			}
		}
		// Right click, do something with object
		if(button == 1)
		{
			// Track hp changes super-crappily
			u16 oldhp = player->hp;
			
			// Do stuff
			obj->rightClick(player);
			
			// Send back stuff
			if(player->hp != oldhp)
			{
				SendPlayerHP(player);
			}
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

			content_t material = CONTENT_IGNORE;
			u8 mineral = MINERAL_NONE;

			bool cannot_remove_node = false;

			try
			{
				MapNode n = m_env.getMap().getNode(p_under);
				// Get mineral
				mineral = n.getMineral();
				// Get material at position
				material = n.getContent();
				// If not yet cancelled
				if(cannot_remove_node == false)
				{
					// If it's not diggable, do nothing
					if(content_diggable(material) == false)
					{
						derr_server<<"Server: Not finishing digging: "
								<<"Node not diggable"
								<<std::endl;
						cannot_remove_node = true;
					}
				}
				// If not yet cancelled
				if(cannot_remove_node == false)
				{
					// Get node metadata
					NodeMetadata *meta = m_env.getMap().getNodeMetadata(p_under);
					if(meta && meta->nodeRemovalDisabled() == true)
					{
						derr_server<<"Server: Not finishing digging: "
								<<"Node metadata disables removal"
								<<std::endl;
						cannot_remove_node = true;
					}
				}
			}
			catch(InvalidPositionException &e)
			{
				derr_server<<"Server: Not finishing digging: Node not found."
						<<" Adding block to emerge queue."
						<<std::endl;
				m_emerge_queue.addBlock(peer_id,
						getNodeBlockPos(p_over), BLOCK_EMERGE_FLAG_FROMDISK);
				cannot_remove_node = true;
			}

			// Make sure the player is allowed to do it
			if((getPlayerPrivs(player) & PRIV_BUILD) == 0)
			{
				dstream<<"Player "<<player->getName()<<" cannot remove node"
						<<" because privileges are "<<getPlayerPrivs(player)
						<<std::endl;
				cannot_remove_node = true;
			}

			/*
				If node can't be removed, set block to be re-sent to
				client and quit.
			*/
			if(cannot_remove_node)
			{
				derr_server<<"Server: Not finishing digging."<<std::endl;

				// Client probably has wrong data.
				// Set block not sent, so that client will get
				// a valid one.
				dstream<<"Client "<<peer_id<<" tried to dig "
						<<"node; but node cannot be removed."
						<<" setting MapBlock not sent."<<std::endl;
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(p_under);
				client->SetBlockNotSent(blockpos);
					
				return;
			}
			
			/*
				Send the removal to all close-by players.
				- If other player is close, send REMOVENODE
				- Otherwise set blocks not sent
			*/
			core::list<u16> far_players;
			sendRemoveNode(p_under, peer_id, &far_players, 30);
			
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
					UpdateCrafting(player->peer_id);
					SendInventory(player->peer_id);
				}
			}

			/*
				Remove the node
				(this takes some time so it is done after the quick stuff)
			*/
			{
				MapEditEventIgnorer ign(&m_ignore_map_edit_events);

				m_env.getMap().removeNodeAndUpdate(p_under, modified_blocks);
			}
			/*
				Set blocks not sent to far players
			*/
			for(core::list<u16>::Iterator
					i = far_players.begin();
					i != far_players.end(); i++)
			{
				u16 peer_id = *i;
				RemoteClient *client = getClient(peer_id);
				if(client==NULL)
					continue;
				client->SetBlocksNotSent(modified_blocks);
			}
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
					bool no_enough_privs =
							((getPlayerPrivs(player) & PRIV_BUILD)==0);
					if(no_enough_privs)
						dstream<<"Player "<<player->getName()<<" cannot add node"
							<<" because privileges are "<<getPlayerPrivs(player)
							<<std::endl;

					if(content_features(n2).buildable_to == false
						|| no_enough_privs)
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
				n.setContent(mitem->getMaterial());

				// Calculate direction for wall mounted stuff
				if(content_features(n).wall_mounted)
					n.param2 = packDir(p_under - p_over);

				// Calculate the direction for furnaces and chests and stuff
				if(content_features(n).param_type == CPT_FACEDIR_SIMPLE)
				{
					v3f playerpos = player->getPosition();
					v3f blockpos = intToFloat(p_over, BS) - playerpos;
					blockpos = blockpos.normalize();
					n.param1 = 0;
					if (fabs(blockpos.X) > fabs(blockpos.Z)) {
						if (blockpos.X < 0)
							n.param1 = 3;
						else
							n.param1 = 1;
					} else {
						if (blockpos.Z < 0)
							n.param1 = 2;
						else
							n.param1 = 0;
					}
				}

				/*
					Send to all close-by players
				*/
				core::list<u16> far_players;
				sendAddNode(p_over, n, 0, &far_players, 30);
				
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
					UpdateCrafting(peer_id);
					SendInventory(peer_id);
				}
				
				/*
					Add node.

					This takes some time so it is done after the quick stuff
				*/
				core::map<v3s16, MapBlock*> modified_blocks;
				{
					MapEditEventIgnorer ign(&m_ignore_map_edit_events);

					m_env.getMap().addNodeAndUpdate(p_over, n, modified_blocks);
				}
				/*
					Set blocks not sent to far players
				*/
				for(core::list<u16>::Iterator
						i = far_players.begin();
						i != far_players.end(); i++)
				{
					u16 peer_id = *i;
					RemoteClient *client = getClient(peer_id);
					if(client==NULL)
						continue;
					client->SetBlocksNotSent(modified_blocks);
				}

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
				Place other item (not a block)
			*/
			else
			{
				v3s16 blockpos = getNodeBlockPos(p_over);
				
				/*
					Check that the block is loaded so that the item
					can properly be added to the static list too
				*/
				MapBlock *block = m_env.getMap().getBlockNoCreateNoEx(blockpos);
				if(block==NULL)
				{
					derr_server<<"Error while placing object: "
							"block not found"<<std::endl;
					return;
				}

				dout_server<<"Placing a miscellaneous item on map"
						<<std::endl;
				
				// Calculate a position for it
				v3f pos = intToFloat(p_over, BS);
				//pos.Y -= BS*0.45;
				pos.Y -= BS*0.25; // let it drop a bit
				// Randomize a bit
				pos.X += BS*0.2*(float)myrand_range(-1000,1000)/1000.0;
				pos.Z += BS*0.2*(float)myrand_range(-1000,1000)/1000.0;

				/*
					Create the object
				*/
				ServerActiveObject *obj = item->createSAO(&m_env, 0, pos);

				if(obj == NULL)
				{
					derr_server<<"WARNING: item resulted in NULL object, "
							<<"not placing onto map"
							<<std::endl;
				}
				else
				{
					// Add the object to the environment
					m_env.addActiveObject(obj);
					
					dout_server<<"Placed object"<<std::endl;

					if(g_settings.getBool("creative_mode") == false)
					{
						// Delete the right amount of items from the slot
						u16 dropcount = item->getDropCount();
						
						// Delete item if all gone
						if(item->getCount() <= dropcount)
						{
							if(item->getCount() < dropcount)
								dstream<<"WARNING: Server: dropped more items"
										<<" than the slot contains"<<std::endl;
							
							InventoryList *ilist = player->inventory.getList("main");
							if(ilist)
								// Remove from inventory and send inventory
								ilist->deleteItem(item_i);
						}
						// Else decrement it
						else
							item->remove(dropcount);
						
						// Send inventory
						UpdateCrafting(peer_id);
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
		if((getPlayerPrivs(player) & PRIV_BUILD) == 0)
			return;
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
	else if(command == TOSERVER_SIGNNODETEXT)
	{
		if((getPlayerPrivs(player) & PRIV_BUILD) == 0)
			return;
		/*
			u16 command
			v3s16 p
			u16 textlen
			textdata
		*/
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		u8 buf[6];
		// Read stuff
		is.read((char*)buf, 6);
		v3s16 p = readV3S16(buf);
		is.read((char*)buf, 2);
		u16 textlen = readU16(buf);
		std::string text;
		for(u16 i=0; i<textlen; i++)
		{
			is.read((char*)buf, 1);
			text += (char)buf[0];
		}

		NodeMetadata *meta = m_env.getMap().getNodeMetadata(p);
		if(!meta)
			return;
		if(meta->typeId() != CONTENT_SIGN_WALL)
			return;
		SignNodeMetadata *signmeta = (SignNodeMetadata*)meta;
		signmeta->setText(text);
		
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock *block = m_env.getMap().getBlockNoCreateNoEx(blockpos);
		if(block)
		{
			block->setChangedFlag();
		}

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd()==false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			client->SetBlockNotSent(blockpos);
		}
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
			// Create context
			InventoryContext c;
			c.current_player = player;

			/*
				Handle craftresult specially if not in creative mode
			*/
			bool disable_action = false;
			if(a->getType() == IACTION_MOVE
					&& g_settings.getBool("creative_mode") == false)
			{
				IMoveAction *ma = (IMoveAction*)a;
				if(ma->to_inv == "current_player" &&
						ma->from_inv == "current_player")
				{
					InventoryList *rlist = player->inventory.getList("craftresult");
					assert(rlist);
					InventoryList *clist = player->inventory.getList("craft");
					assert(clist);
					InventoryList *mlist = player->inventory.getList("main");
					assert(mlist);
					/*
						Craftresult is no longer preview if something
						is moved into it
					*/
					if(ma->to_list == "craftresult"
							&& ma->from_list != "craftresult")
					{
						// If it currently is a preview, remove
						// its contents
						if(player->craftresult_is_preview)
						{
							rlist->deleteItem(0);
						}
						player->craftresult_is_preview = false;
					}
					/*
						Crafting takes place if this condition is true.
					*/
					if(player->craftresult_is_preview &&
							ma->from_list == "craftresult")
					{
						player->craftresult_is_preview = false;
						clist->decrementMaterials(1);
					}
					/*
						If the craftresult is placed on itself, move it to
						main inventory instead of doing the action
					*/
					if(ma->to_list == "craftresult"
							&& ma->from_list == "craftresult")
					{
						disable_action = true;
						
						InventoryItem *item1 = rlist->changeItem(0, NULL);
						mlist->addItem(item1);
					}
				}
			}
			
			if(disable_action == false)
			{
				// Feed action to player inventory
				a->apply(&c, this);
				// Eat the action
				delete a;
			}
			else
			{
				// Send inventory
				UpdateCrafting(player->peer_id);
				SendInventory(player->peer_id);
			}
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
		
		// Local player gets all privileges regardless of
		// what's set on their account.
		u64 privs = getPlayerPrivs(player);

		// Parse commands
		std::wstring commandprefix = L"/#";
		if(message.substr(0, commandprefix.size()) == commandprefix)
		{
			line += L"Server: ";

			message = message.substr(commandprefix.size());
			
			WStrfnd f1(message);
			f1.next(L" "); // Skip over /#whatever
			std::wstring paramstring = f1.next(L"");

			ServerCommandContext *ctx = new ServerCommandContext(
				str_split(message, L' '),
				paramstring,
				this,
				&m_env,
				player,
				privs);

			line += processServerCommand(ctx);
			send_to_sender = ctx->flags & 1;
			send_to_others = ctx->flags & 2;
			delete ctx;

		}
		else
		{
			if(privs & PRIV_SHOUT)
			{
				line += L"<";
				line += name;
				line += L"> ";
				line += message;
				send_to_others = true;
			}
			else
			{
				line += L"Server: You are not allowed to shout";
				send_to_sender = true;
			}
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
	else if(command == TOSERVER_DAMAGE)
	{
		if(g_settings.getBool("enable_damage"))
		{
			std::string datastring((char*)&data[2], datasize-2);
			std::istringstream is(datastring, std::ios_base::binary);
			u8 damage = readU8(is);
			if(player->hp > damage)
			{
				player->hp -= damage;
			}
			else
			{
				player->hp = 0;

				dstream<<"TODO: Server: TOSERVER_HP_DECREMENT: Player dies"
						<<std::endl;
				
				v3f pos = findSpawnPos(m_env.getServerMap());
				player->setPosition(pos);
				player->hp = 20;
				SendMovePlayer(player);
				SendPlayerHP(player);
				
				//TODO: Throw items around
			}
		}

		SendPlayerHP(player);
	}
	else if(command == TOSERVER_PASSWORD)
	{
		/*
			[0] u16 TOSERVER_PASSWORD
			[2] u8[28] old password
			[30] u8[28] new password
		*/

		if(datasize != 2+PASSWORD_SIZE*2)
			return;
		/*char password[PASSWORD_SIZE];
		for(u32 i=0; i<PASSWORD_SIZE-1; i++)
			password[i] = data[2+i];
		password[PASSWORD_SIZE-1] = 0;*/
		std::string oldpwd;
		for(u32 i=0; i<PASSWORD_SIZE-1; i++)
		{
			char c = data[2+i];
			if(c == 0)
				break;
			oldpwd += c;
		}
		std::string newpwd;
		for(u32 i=0; i<PASSWORD_SIZE-1; i++)
		{
			char c = data[2+PASSWORD_SIZE+i];
			if(c == 0)
				break;
			newpwd += c;
		}

		std::string playername = player->getName();

		if(m_authmanager.exists(playername) == false)
		{
			dstream<<"Server: playername not found in authmanager"<<std::endl;
			// Wrong old password supplied!!
			SendChatMessage(peer_id, L"playername not found in authmanager");
			return;
		}

		std::string checkpwd = m_authmanager.getPassword(playername);
		
		if(oldpwd != checkpwd)
		{
			dstream<<"Server: invalid old password"<<std::endl;
			// Wrong old password supplied!!
			SendChatMessage(peer_id, L"Invalid old password supplied. Password NOT changed.");
			return;
		}

		m_authmanager.setPassword(playername, newpwd);
		
		dstream<<"Server: password change successful for "<<playername
				<<std::endl;
		SendChatMessage(peer_id, L"Password change successful");
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

void Server::onMapEditEvent(MapEditEvent *event)
{
	//dstream<<"Server::onMapEditEvent()"<<std::endl;
	if(m_ignore_map_edit_events)
		return;
	MapEditEvent *e = event->clone();
	m_unsent_map_edit_queue.push_back(e);
}

Inventory* Server::getInventory(InventoryContext *c, std::string id)
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
		NodeMetadata *meta = m_env.getMap().getNodeMetadata(p);
		if(meta)
			return meta->getInventory();
		dstream<<"nodemeta at ("<<p.X<<","<<p.Y<<","<<p.Z<<"): "
				<<"no metadata found"<<std::endl;
		return NULL;
	}

	dstream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
	return NULL;
}
void Server::inventoryModified(InventoryContext *c, std::string id)
{
	if(id == "current_player")
	{
		assert(c->current_player);
		// Send inventory
		UpdateCrafting(c->current_player->peer_id);
		SendInventory(c->current_player->peer_id);
		return;
	}
	
	Strfnd fn(id);
	std::string id0 = fn.next(":");

	if(id0 == "nodemeta")
	{
		v3s16 p;
		p.X = stoi(fn.next(","));
		p.Y = stoi(fn.next(","));
		p.Z = stoi(fn.next(","));
		v3s16 blockpos = getNodeBlockPos(p);

		NodeMetadata *meta = m_env.getMap().getNodeMetadata(p);
		if(meta)
			meta->inventoryModified();

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd()==false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			client->SetBlockNotSent(blockpos);
		}

		return;
	}

	dstream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
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

/*
	Static send methods
*/

void Server::SendHP(con::Connection &con, u16 peer_id, u8 hp)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_HP);
	writeU8(os, hp);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendAccessDenied(con::Connection &con, u16 peer_id,
		const std::wstring &reason)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_ACCESS_DENIED);
	os<<serializeWideString(reason);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

/*
	Non-static send methods
*/

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

void Server::SendInventory(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	Player* player = m_env.getPlayer(peer_id);
	assert(player);

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

void Server::SendPlayerHP(Player *player)
{
	SendHP(m_con, player->peer_id, player->hp);
}

void Server::SendMovePlayer(Player *player)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_MOVE_PLAYER);
	writeV3F1000(os, player->getPosition());
	writeF1000(os, player->getPitch());
	writeF1000(os, player->getYaw());
	
	{
		v3f pos = player->getPosition();
		f32 pitch = player->getPitch();
		f32 yaw = player->getYaw();
		dstream<<"Server sending TOCLIENT_MOVE_PLAYER"
				<<" pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"
				<<" pitch="<<pitch
				<<" yaw="<<yaw
				<<std::endl;
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(player->peer_id, 0, data, true);
}

void Server::sendRemoveNode(v3s16 p, u16 ignore_id,
	core::list<u16> *far_players, float far_d_nodes)
{
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

	// Create packet
	u32 replysize = 8;
	SharedBuffer<u8> reply(replysize);
	writeU16(&reply[0], TOCLIENT_REMOVENODE);
	writeS16(&reply[2], p.X);
	writeS16(&reply[4], p.Y);
	writeS16(&reply[6], p.Z);

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
		if(client->peer_id == ignore_id)
			continue;
		
		if(far_players)
		{
			// Get player
			Player *player = m_env.getPlayer(client->peer_id);
			if(player)
			{
				// If player is far away, only set modified blocks not sent
				v3f player_pos = player->getPosition();
				if(player_pos.getDistanceFrom(p_f) > maxd)
				{
					far_players->push_back(client->peer_id);
					continue;
				}
			}
		}

		// Send as reliable
		m_con.Send(client->peer_id, 0, reply, true);
	}
}

void Server::sendAddNode(v3s16 p, MapNode n, u16 ignore_id,
		core::list<u16> *far_players, float far_d_nodes)
{
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

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
		if(client->peer_id == ignore_id)
			continue;

		if(far_players)
		{
			// Get player
			Player *player = m_env.getPlayer(client->peer_id);
			if(player)
			{
				// If player is far away, only set modified blocks not sent
				v3f player_pos = player->getPosition();
				if(player_pos.getDistanceFrom(p_f) > maxd)
				{
					far_players->push_back(client->peer_id);
					continue;
				}
			}
		}

		// Create packet
		u32 replysize = 8 + MapNode::serializedLength(client->serialization_version);
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOCLIENT_ADDNODE);
		writeS16(&reply[2], p.X);
		writeS16(&reply[4], p.Y);
		writeS16(&reply[6], p.Z);
		n.serialize(&reply[8], client->serialization_version);

		// Send as reliable
		m_con.Send(client->peer_id, 0, reply, true);
	}
}

void Server::setBlockNotSent(v3s16 p)
{
	for(core::map<u16, RemoteClient*>::Iterator
		i = m_clients.getIterator();
		i.atEnd()==false; i++)
	{
		RemoteClient *client = i.getNode()->getValue();
		client->SetBlockNotSent(p);
	}
}

void Server::SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver)
{
	DSTACK(__FUNCTION_NAME);

	v3s16 p = block->getPos();
	
#if 0
	// Analyze it a bit
	bool completely_air = true;
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
	{
		if(block->getNodeNoEx(v3s16(x0,y0,z0)).d != CONTENT_AIR)
		{
			completely_air = false;
			x0 = y0 = z0 = MAP_BLOCKSIZE; // Break out
		}
	}

	// Print result
	dstream<<"Server: Sending block ("<<p.X<<","<<p.Y<<","<<p.Z<<"): ";
	if(completely_air)
		dstream<<"[completely air] ";
	dstream<<std::endl;
#endif

	/*
		Create a packet with the block in the right format
	*/
	
	std::ostringstream os(std::ios_base::binary);
	block->serialize(os, ver);
	std::string s = os.str();
	SharedBuffer<u8> blockdata((u8*)s.c_str(), s.size());

	u32 replysize = 8 + blockdata.getSize();
	SharedBuffer<u8> reply(replysize);
	writeU16(&reply[0], TOCLIENT_BLOCKDATA);
	writeS16(&reply[2], p.X);
	writeS16(&reply[4], p.Y);
	writeS16(&reply[6], p.Z);
	memcpy(&reply[8], *blockdata, blockdata.getSize());

	/*dstream<<"Server: Sending block ("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<":  \tpacket size: "<<replysize<<std::endl;*/
	
	/*
		Send packet
	*/
	m_con.Send(peer_id, 1, reply, true);
}

void Server::SendBlocks(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	//TimeTaker timer("Server::SendBlocks");

	core::array<PrioritySortedBlockTransfer> queue;

	s32 total_sending = 0;
	
	{
		ScopeProfiler sp(&g_profiler, "Server: selecting blocks for sending");

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

/*
	Something random
*/

void Server::UpdateCrafting(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	Player* player = m_env.getPlayer(peer_id);
	assert(player);

	/*
		Calculate crafting stuff
	*/
	if(g_settings.getBool("creative_mode") == false)
	{
		InventoryList *clist = player->inventory.getList("craft");
		InventoryList *rlist = player->inventory.getList("craftresult");

		if(rlist->getUsedSlots() == 0)
			player->craftresult_is_preview = true;

		if(rlist && player->craftresult_is_preview)
		{
			rlist->clearItems();
		}
		if(clist && rlist && player->craftresult_is_preview)
		{
			InventoryItem *items[9];
			for(u16 i=0; i<9; i++)
			{
				items[i] = clist->getItem(i);
			}
			
			// Get result of crafting grid
			InventoryItem *result = craft_get_result(items);
			if(result)
				rlist->addItem(result);
		}
	
	} // if creative_mode == false
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
	// Version
	os<<L"version="<<narrow_to_wide(VERSION_STRING);
	// Uptime
	os<<L", uptime="<<m_uptime.get();
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
		os<<std::endl<<L"# Server: "<<" WARNING: Map saving is disabled.";
	if(g_settings.get("motd") != "")
		os<<std::endl<<L"# Server: "<<narrow_to_wide(g_settings.get("motd"));
	return os.str();
}

v3f findSpawnPos(ServerMap &map)
{
	//return v3f(50,50,50)*BS;

	v2s16 nodepos;
	s16 groundheight = 0;
	
#if 0
	nodepos = v2s16(0,0);
	groundheight = 20;
#endif

#if 1
	// Try to find a good place a few times
	for(s32 i=0; i<1000; i++)
	{
		s32 range = 1 + i;
		// We're going to try to throw the player to this position
		nodepos = v2s16(-range + (myrand()%(range*2)),
				-range + (myrand()%(range*2)));
		v2s16 sectorpos = getNodeSectorPos(nodepos);
		// Get sector (NOTE: Don't get because it's slow)
		//m_env.getMap().emergeSector(sectorpos);
		// Get ground height at point (fallbacks to heightmap function)
		groundheight = map.findGroundLevel(nodepos);
		// Don't go underwater
		if(groundheight < WATER_LEVEL)
		{
			//dstream<<"-> Underwater"<<std::endl;
			continue;
		}
		// Don't go to high places
		if(groundheight > WATER_LEVEL + 4)
		{
			//dstream<<"-> Underwater"<<std::endl;
			continue;
		}

		// Found a good place
		//dstream<<"Searched through "<<i<<" places."<<std::endl;
		break;
	}
#endif
	
	// If no suitable place was not found, go above water at least.
	if(groundheight < WATER_LEVEL)
		groundheight = WATER_LEVEL;

	return intToFloat(v3s16(
			nodepos.X,
			groundheight + 3,
			nodepos.Y
			), BS);
}

Player *Server::emergePlayer(const char *name, const char *password, u16 peer_id)
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
			craft_set_creative_inventory(player);
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
		m_authmanager.add(name);
		m_authmanager.setPassword(name, password);
		m_authmanager.setPrivs(name,
				stringToPrivs(g_settings.get("default_privs")));

		/*
			Set player position
		*/
		
		dstream<<"Server: Finding spawn place for player \""
				<<player->getName()<<"\""<<std::endl;

		v3f pos = findSpawnPos(m_env.getServerMap());

		player->setPosition(pos);

		/*
			Add player to environment
		*/

		m_env.addPlayer(player);

		/*
			Add stuff to inventory
		*/
		
		if(g_settings.getBool("creative_mode"))
		{
			craft_set_creative_inventory(player);
		}
		else if(g_settings.getBool("give_initial_stuff"))
		{
			craft_give_initial_stuff(player);
		}

		return player;
		
	} // create new player
}

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
		
		/*
			Mark objects to be not known by the client
		*/
		RemoteClient *client = n->getValue();
		// Handle objects
		for(core::map<u16, bool>::Iterator
				i = client->m_known_objects.getIterator();
				i.atEnd()==false; i++)
		{
			// Get object
			u16 id = i.getNode()->getKey();
			ServerActiveObject* obj = m_env.getActiveObject(id);
			
			if(obj && obj->m_known_by_count > 0)
				obj->m_known_by_count--;
		}

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

u64 Server::getPlayerPrivs(Player *player)
{
	if(player==NULL)
		return 0;
	std::string playername = player->getName();
	// Local player gets all privileges regardless of
	// what's set on their account.
	if(g_settings.get("name") == playername)
	{
		return PRIV_ALL;
	}
	else
	{
		return getPlayerAuthPrivs(playername);
	}
}

void dedicated_server_loop(Server &server, bool &kill)
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<DTIME<<std::endl;
	dstream<<"========================"<<std::endl;
	dstream<<"Running dedicated server"<<std::endl;
	dstream<<"========================"<<std::endl;
	dstream<<std::endl;

	IntervalLimiter m_profiler_interval;

	for(;;)
	{
		// This is kind of a hack but can be done like this
		// because server.step() is very light
		{
			ScopeProfiler sp(&g_profiler, "dedicated server sleep");
			sleep_ms(30);
		}
		server.step(0.030);

		if(server.getShutdownRequested() || kill)
		{
			dstream<<DTIME<<" dedicated_server_loop(): Quitting."<<std::endl;
			break;
		}

		/*
			Profiler
		*/
		float profiler_print_interval =
				g_settings.getFloat("profiler_print_interval");
		if(profiler_print_interval != 0)
		{
			if(m_profiler_interval.step(0.030, profiler_print_interval))
			{
				dstream<<"Profiler:"<<std::endl;
				g_profiler.print(dstream);
				g_profiler.clear();
			}
		}
		
		/*
			Player info
		*/
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
				dstream<<DTIME<<"Player info:"<<std::endl;
				for(i=list.begin(); i!=list.end(); i++)
				{
					i->PrintLine(&dstream);
				}
			}
			sum_old = sum;
		}
	}
}


