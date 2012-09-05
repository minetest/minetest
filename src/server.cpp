/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "server.h"
#include <iostream>
#include <queue>
#include "clientserver.h"
#include "map.h"
#include "jmutexautolock.h"
#include "main.h"
#include "constants.h"
#include "voxel.h"
#include "config.h"
#include "filesys.h"
#include "mapblock.h"
#include "serverobject.h"
#include "settings.h"
#include "profiler.h"
#include "log.h"
#include "script.h"
#include "scriptapi.h"
#include "nodedef.h"
#include "itemdef.h"
#include "craftdef.h"
#include "mapgen.h"
#include "content_mapnode.h"
#include "content_nodemeta.h"
#include "content_abm.h"
#include "content_sao.h"
#include "mods.h"
#include "sha1.h"
#include "base64.h"
#include "tool.h"
#include "sound.h" // dummySoundManager
#include "event_manager.h"
#include "hex.h"
#include "util/string.h"
#include "util/pointedthing.h"
#include "util/mathconstants.h"
#include "rollback.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

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

class MapEditEventAreaIgnorer
{
public:
	MapEditEventAreaIgnorer(VoxelArea *ignorevariable, const VoxelArea &a):
		m_ignorevariable(ignorevariable)
	{
		if(m_ignorevariable->getVolume() == 0)
			*m_ignorevariable = a;
		else
			m_ignorevariable = NULL;
	}

	~MapEditEventAreaIgnorer()
	{
		if(m_ignorevariable)
		{
			assert(m_ignorevariable->getVolume() != 0);
			*m_ignorevariable = VoxelArea();
		}
	}
	
private:
	VoxelArea *m_ignorevariable;
};

void * ServerThread::Thread()
{
	ThreadStarted();

	log_register_thread("ServerThread");

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
		
			//infostream<<"Running m_server->Receive()"<<std::endl;
			m_server->Receive();
		}
		catch(con::NoIncomingDataException &e)
		{
		}
		catch(con::PeerNotFoundException &e)
		{
			infostream<<"Server: PeerNotFoundException"<<std::endl;
		}
		catch(con::ConnectionBindFailed &e)
		{
			m_server->setAsyncFatalError(e.what());
		}
		catch(LuaError &e)
		{
			m_server->setAsyncFatalError(e.what());
		}
	}
	
	END_DEBUG_EXCEPTION_HANDLER(errorstream)

	return NULL;
}

void * EmergeThread::Thread()
{
	ThreadStarted();

	log_register_thread("EmergeThread");

	DSTACK(__FUNCTION_NAME);

	BEGIN_DEBUG_EXCEPTION_HANDLER

	bool enable_mapgen_debug_info = g_settings->getBool("enable_mapgen_debug_info");

	v3s16 last_tried_pos(-32768,-32768,-32768); // For error output
	
	/*
		Get block info from queue, emerge them and send them
		to clients.

		After queue is empty, exit.
	*/
	while(getRun())
	try{
		QueuedBlockEmerge *qptr = m_server->m_emerge_queue.pop();
		if(qptr == NULL)
			break;
		
		SharedPtr<QueuedBlockEmerge> q(qptr);

		v3s16 &p = q->pos;
		v2s16 p2d(p.X,p.Z);
		
		last_tried_pos = p;

		/*
			Do not generate over-limit
		*/
		if(blockpos_over_limit(p))
			continue;
			
		//infostream<<"EmergeThread::Thread(): running"<<std::endl;

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
			infostream<<"EmergeThread: p="
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<") "
					<<"only_from_disk="<<only_from_disk<<std::endl;
		
		ServerMap &map = ((ServerMap&)m_server->m_env->getMap());
			
		MapBlock *block = NULL;
		bool got_block = true;
		core::map<v3s16, MapBlock*> modified_blocks;

		/*
			Try to fetch block from memory or disk.
			If not found and asked to generate, initialize generator.
		*/
		
		bool started_generate = false;
		mapgen::BlockMakeData data;

		{
			JMutexAutoLock envlock(m_server->m_env_mutex);
			
			// Load sector if it isn't loaded
			if(map.getSectorNoGenerateNoEx(p2d) == NULL)
				map.loadSectorMeta(p2d);
			
			// Attempt to load block
			block = map.getBlockNoCreateNoEx(p);
			if(!block || block->isDummy() || !block->isGenerated())
			{
				if(enable_mapgen_debug_info)
					infostream<<"EmergeThread: not in memory, "
							<<"attempting to load from disk"<<std::endl;

				block = map.loadBlock(p);
			}
			
			// If could not load and allowed to generate, start generation
			// inside this same envlock
			if(only_from_disk == false &&
					(block == NULL || block->isGenerated() == false)){
				if(enable_mapgen_debug_info)
					infostream<<"EmergeThread: generating"<<std::endl;
				started_generate = true;

				map.initBlockMake(&data, p);
			}
		}

		/*
			If generator was initialized, generate now when envlock is free.
		*/
		if(started_generate)
		{
			{
				ScopeProfiler sp(g_profiler, "EmergeThread: mapgen::make_block",
						SPT_AVG);
				TimeTaker t("mapgen::make_block()");

				mapgen::make_block(&data);

				if(enable_mapgen_debug_info == false)
					t.stop(true); // Hide output
			}
			
			do{ // enable break
				// Lock environment again to access the map
				JMutexAutoLock envlock(m_server->m_env_mutex);
				
				ScopeProfiler sp(g_profiler, "EmergeThread: after "
						"mapgen::make_block (envlock)", SPT_AVG);

				// Blit data back on map, update lighting, add mobs and
				// whatever this does
				map.finishBlockMake(&data, modified_blocks);

				// Get central block
				block = map.getBlockNoCreateNoEx(p);
				
				// If block doesn't exist, don't try doing anything with it
				// This happens if the block is not in generation boundaries
				if(!block)
					break;

				/*
					Do some post-generate stuff
				*/

				v3s16 minp = data.blockpos_min*MAP_BLOCKSIZE;
				v3s16 maxp = data.blockpos_max*MAP_BLOCKSIZE +
						v3s16(1,1,1)*(MAP_BLOCKSIZE-1);
				
				/*
					Ignore map edit events, they will not need to be
					sent to anybody because the block hasn't been sent
					to anybody
				*/
				//MapEditEventIgnorer ign(&m_server->m_ignore_map_edit_events);
				MapEditEventAreaIgnorer ign(
						&m_server->m_ignore_map_edit_events_area,
						VoxelArea(minp, maxp));
				{
					TimeTaker timer("on_generated");
					scriptapi_environment_on_generated(m_server->m_lua,
							minp, maxp, mapgen::get_blockseed(data.seed, minp));
					/*int t = timer.stop(true);
					dstream<<"on_generated took "<<t<<"ms"<<std::endl;*/
				}
				
				if(enable_mapgen_debug_info)
					infostream<<"EmergeThread: ended up with: "
							<<analyze_block(block)<<std::endl;

				// Activate objects and stuff
				m_server->m_env->activateBlock(block, 0);
			}while(false);
		}

		if(block == NULL)
			got_block = false;
			
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
	catch(VersionMismatchException &e)
	{
		std::ostringstream err;
		err<<"World data version mismatch in MapBlock "<<PP(last_tried_pos)<<std::endl;
		err<<"----"<<std::endl;
		err<<"\""<<e.what()<<"\""<<std::endl;
		err<<"See debug.txt."<<std::endl;
		err<<"World probably saved by a newer version of Minetest."<<std::endl;
		m_server->setAsyncFatalError(err.str());
	}
	catch(SerializationError &e)
	{
		std::ostringstream err;
		err<<"Invalid data in MapBlock "<<PP(last_tried_pos)<<std::endl;
		err<<"----"<<std::endl;
		err<<"\""<<e.what()<<"\""<<std::endl;
		err<<"See debug.txt."<<std::endl;
		err<<"You can ignore this using [ignore_world_load_errors = true]."<<std::endl;
		m_server->setAsyncFatalError(err.str());
	}

	END_DEBUG_EXCEPTION_HANDLER(errorstream)

	log_deregister_thread();

	return NULL;
}

v3f ServerSoundParams::getPos(ServerEnvironment *env, bool *pos_exists) const
{
	if(pos_exists) *pos_exists = false;
	switch(type){
	case SSP_LOCAL:
		return v3f(0,0,0);
	case SSP_POSITIONAL:
		if(pos_exists) *pos_exists = true;
		return pos;
	case SSP_OBJECT: {
		if(object == 0)
			return v3f(0,0,0);
		ServerActiveObject *sao = env->getActiveObject(object);
		if(!sao)
			return v3f(0,0,0);
		if(pos_exists) *pos_exists = true;
		return sao->getBasePosition(); }
	}
	return v3f(0,0,0);
}

void RemoteClient::GetNextBlocks(Server *server, float dtime,
		core::array<PrioritySortedBlockTransfer> &dest)
{
	DSTACK(__FUNCTION_NAME);
	
	/*u32 timer_result;
	TimeTaker timer("RemoteClient::GetNextBlocks", &timer_result);*/
	
	// Increment timers
	m_nothing_to_send_pause_timer -= dtime;
	m_nearest_unsent_reset_timer += dtime;
	
	if(m_nothing_to_send_pause_timer >= 0)
		return;

	Player *player = server->m_env->getPlayer(peer_id);
	// This can happen sometimes; clients and players are not in perfect sync.
	if(player == NULL)
		return;

	// Won't send anything if already sending
	if(m_blocks_sending.size() >= g_settings->getU16
			("max_simultaneous_block_sends_per_client"))
	{
		//infostream<<"Not sending any blocks, Queue full."<<std::endl;
		return;
	}

	//TimeTaker timer("RemoteClient::GetNextBlocks");
	
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
	v3f camera_pos = player->getEyePosition();
	v3f camera_dir = v3f(0,0,1);
	camera_dir.rotateYZBy(player->getPitch());
	camera_dir.rotateXZBy(player->getYaw());

	/*infostream<<"camera_dir=("<<camera_dir.X<<","<<camera_dir.Y<<","
			<<camera_dir.Z<<")"<<std::endl;*/

	/*
		Get the starting value of the block finder radius.
	*/
		
	if(m_last_center != center)
	{
		m_nearest_unsent_d = 0;
		m_last_center = center;
	}

	/*infostream<<"m_nearest_unsent_reset_timer="
			<<m_nearest_unsent_reset_timer<<std::endl;*/
			
	// Reset periodically to workaround for some bugs or stuff
	if(m_nearest_unsent_reset_timer > 20.0)
	{
		m_nearest_unsent_reset_timer = 0;
		m_nearest_unsent_d = 0;
		//infostream<<"Resetting m_nearest_unsent_d for "
		//		<<server->getPlayerName(peer_id)<<std::endl;
	}

	//s16 last_nearest_unsent_d = m_nearest_unsent_d;
	s16 d_start = m_nearest_unsent_d;

	//infostream<<"d_start="<<d_start<<std::endl;

	u16 max_simul_sends_setting = g_settings->getU16
			("max_simultaneous_block_sends_per_client");
	u16 max_simul_sends_usually = max_simul_sends_setting;

	/*
		Check the time from last addNode/removeNode.
		
		Decrease send rate if player is building stuff.
	*/
	m_time_from_building += dtime;
	if(m_time_from_building < g_settings->getFloat(
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

	s16 d_max = g_settings->getS16("max_block_send_distance");
	s16 d_max_gen = g_settings->getS16("max_block_generate_distance");
	
	// Don't loop very much at a time
	s16 max_d_increment_at_time = 2;
	if(d_max > d_start + max_d_increment_at_time)
		d_max = d_start + max_d_increment_at_time;
	/*if(d_max_gen > d_start+2)
		d_max_gen = d_start+2;*/
	
	//infostream<<"Starting from "<<d_start<<std::endl;

	s32 nearest_emerged_d = -1;
	s32 nearest_emergefull_d = -1;
	s32 nearest_sent_d = -1;
	bool queue_is_full = false;
	
	s16 d;
	for(d = d_start; d <= d_max; d++)
	{
		/*errorstream<<"checking d="<<d<<" for "
				<<server->getPlayerName(peer_id)<<std::endl;*/
		//infostream<<"RemoteClient::SendBlocks(): d="<<d<<std::endl;
		
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

				// Limit the send area vertically to 1/2
				if(abs(p.Y - center.Y) > d_max / 2)
					continue;
			}

#if 0
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
				s16 gh = server->m_env->getServerMap().findGroundLevel(
						p2d_nodes_center);

				// If differs a lot, don't generate
				if(fabs(gh - y) > MAP_BLOCKSIZE*2)
					generate = false;
					// Actually, don't even send it
					//continue;
	#endif
			}
#endif

			//infostream<<"d="<<d<<std::endl;
#if 1
			/*
				Don't generate or send if not in sight
				FIXME This only works if the client uses a small enough
				FOV setting. The default of 72 degrees is fine.
			*/

			float camera_fov = (72.0*M_PI/180) * 4./3.;
			if(isBlockInSight(p, camera_pos, camera_dir, camera_fov, 10000*BS) == false)
			{
				continue;
			}
#endif
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
			MapBlock *block = server->m_env->getMap().getBlockNoCreateNoEx(p);
			
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
				ServerMap *map = (ServerMap*)(&server->m_env->getMap());
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
				if(d >= 4)
				{
					if(block->getDayNightDiff() == false)
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
				Add inexistent block to emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || block_is_invalid)
			{
				//TODO: Get value from somewhere
				// Allow only one block in emerge queue
				//if(server->m_emerge_queue.peerItemCount(peer_id) < 1)
				// Allow two blocks in queue per client
				//if(server->m_emerge_queue.peerItemCount(peer_id) < 2)
				u32 max_emerge = 5;
				// Make it more responsive when needing to generate stuff
				if(surely_not_found_on_disk)
					max_emerge = 1;
				if(server->m_emerge_queue.peerItemCount(peer_id) < max_emerge)
				{
					//infostream<<"Adding block to emerge queue"<<std::endl;
					
					// Add it to the emerge queue and trigger the thread
					
					u8 flags = 0;
					if(generate == false)
						flags |= BLOCK_EMERGE_FLAG_FROMDISK;
					
					server->m_emerge_queue.addBlock(peer_id, p, flags);
					server->m_emergethread.trigger();

					if(nearest_emerged_d == -1)
						nearest_emerged_d = d;
				} else {
					if(nearest_emergefull_d == -1)
						nearest_emergefull_d = d;
				}
				
				// get next one.
				continue;
			}

			if(nearest_sent_d == -1)
				nearest_sent_d = d;

			/*
				Add block to send queue
			*/

			/*errorstream<<"sending from d="<<d<<" to "
					<<server->getPlayerName(peer_id)<<std::endl;*/

			PrioritySortedBlockTransfer q((float)d, p, peer_id);

			dest.push_back(q);

			num_blocks_selected += 1;
		}
	}
queue_full_break:

	//infostream<<"Stopped at "<<d<<std::endl;
	
	// If nothing was found for sending and nothing was queued for
	// emerging, continue next time browsing from here
	if(nearest_emerged_d != -1){
		new_nearest_unsent_d = nearest_emerged_d;
	} else if(nearest_emergefull_d != -1){
		new_nearest_unsent_d = nearest_emergefull_d;
	} else {
		if(d > g_settings->getS16("max_block_send_distance")){
			new_nearest_unsent_d = 0;
			m_nothing_to_send_pause_timer = 2.0;
			/*infostream<<"GetNextBlocks(): d wrapped around for "
					<<server->getPlayerName(peer_id)
					<<"; setting to 0 and pausing"<<std::endl;*/
		} else {
			if(nearest_sent_d != -1)
				new_nearest_unsent_d = nearest_sent_d;
			else
				new_nearest_unsent_d = d;
		}
	}

	if(new_nearest_unsent_d != -1)
		m_nearest_unsent_d = new_nearest_unsent_d;

	/*timer_result = timer.stop(true);
	if(timer_result != 0)
		infostream<<"GetNextBlocks timeout: "<<timer_result<<" (!=0)"<<std::endl;*/
}

void RemoteClient::GotBlock(v3s16 p)
{
	if(m_blocks_sending.find(p) != NULL)
		m_blocks_sending.remove(p);
	else
	{
		/*infostream<<"RemoteClient::GotBlock(): Didn't find in"
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
		infostream<<"RemoteClient::SentBlock(): Sent block"
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

/*
	Server
*/

Server::Server(
		const std::string &path_world,
		const std::string &path_config,
		const SubgameSpec &gamespec,
		bool simple_singleplayer_mode
	):
	m_path_world(path_world),
	m_path_config(path_config),
	m_gamespec(gamespec),
	m_simple_singleplayer_mode(simple_singleplayer_mode),
	m_async_fatal_error(""),
	m_env(NULL),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this),
	m_banmanager(path_world+DIR_DELIM+"ipban.txt"),
	m_rollback(NULL),
	m_rollback_sink_enabled(true),
	m_enable_rollback_recording(false),
	m_lua(NULL),
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_craftdef(createCraftDefManager()),
	m_event(new EventManager()),
	m_thread(this),
	m_emergethread(this),
	m_time_of_day_send_timer(0),
	m_uptime(0),
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

	if(path_world == "")
		throw ServerError("Supplied empty world path");
	
	if(!gamespec.isValid())
		throw ServerError("Supplied invalid gamespec");
	
	infostream<<"Server created for gameid \""<<m_gamespec.id<<"\"";
	if(m_simple_singleplayer_mode)
		infostream<<" in simple singleplayer mode"<<std::endl;
	else
		infostream<<std::endl;
	infostream<<"- world:  "<<m_path_world<<std::endl;
	infostream<<"- config: "<<m_path_config<<std::endl;
	infostream<<"- game:   "<<m_gamespec.path<<std::endl;

	// Create rollback manager
	std::string rollback_path = m_path_world+DIR_DELIM+"rollback.txt";
	m_rollback = createRollbackManager(rollback_path, this);

	// Add world mod search path
	m_modspaths.push_front(m_path_world + DIR_DELIM + "worldmods");
	// Add addon mod search path
	for(std::set<std::string>::const_iterator i = m_gamespec.mods_paths.begin();
			i != m_gamespec.mods_paths.end(); i++)
		m_modspaths.push_front((*i));

	// Print out mod search paths
	for(core::list<std::string>::Iterator i = m_modspaths.begin();
			i != m_modspaths.end(); i++){
		std::string modspath = *i;
		infostream<<"- mods:   "<<modspath<<std::endl;
	}
	
	// Path to builtin.lua
	std::string builtinpath = getBuiltinLuaPath() + DIR_DELIM + "builtin.lua";

	// Create world if it doesn't exist
	if(!initializeWorld(m_path_world, m_gamespec.id))
		throw ServerError("Failed to initialize world");

	// Lock environment
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	// Initialize scripting
	
	infostream<<"Server: Initializing Lua"<<std::endl;
	m_lua = script_init();
	assert(m_lua);
	// Export API
	scriptapi_export(m_lua, this);
	// Load and run builtin.lua
	infostream<<"Server: Loading builtin.lua [\""
			<<builtinpath<<"\"]"<<std::endl;
	bool success = scriptapi_loadmod(m_lua, builtinpath, "__builtin");
	if(!success){
		errorstream<<"Server: Failed to load and run "
				<<builtinpath<<std::endl;
		throw ModError("Failed to load and run "+builtinpath);
	}
	// Find mods in mod search paths
	m_mods = getMods(m_modspaths);
	// Print 'em
	infostream<<"Server: Loading mods: ";
	for(core::list<ModSpec>::Iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		infostream<<mod.name<<" ";
	}
	infostream<<std::endl;
	// Load and run "mod" scripts
	for(core::list<ModSpec>::Iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		std::string scriptpath = mod.path + DIR_DELIM + "init.lua";
		infostream<<"  ["<<padStringRight(mod.name, 12)<<"] [\""
				<<scriptpath<<"\"]"<<std::endl;
		bool success = scriptapi_loadmod(m_lua, scriptpath, mod.name);
		if(!success){
			errorstream<<"Server: Failed to load and run "
					<<scriptpath<<std::endl;
			throw ModError("Failed to load and run "+scriptpath);
		}
	}
	
	// Read Textures and calculate sha1 sums
	fillMediaCache();

	// Apply item aliases in the node definition manager
	m_nodedef->updateAliases(m_itemdef);

	// Initialize Environment
	
	m_env = new ServerEnvironment(new ServerMap(path_world, this), m_lua,
			this, this);
	
	// Give environment reference to scripting api
	scriptapi_add_environment(m_lua, m_env);
	
	// Register us to receive map edit events
	m_env->getMap().addEventReceiver(this);

	// If file exists, load environment metadata
	if(fs::PathExists(m_path_world+DIR_DELIM+"env_meta.txt"))
	{
		infostream<<"Server: Loading environment metadata"<<std::endl;
		m_env->loadMeta(m_path_world);
	}

	// Load players
	infostream<<"Server: Loading players"<<std::endl;
	m_env->deSerializePlayers(m_path_world);

	/*
		Add some test ActiveBlockModifiers to environment
	*/
	add_legacy_abms(m_env, m_nodedef);
}

Server::~Server()
{
	infostream<<"Server destructing"<<std::endl;

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
	
	{
		JMutexAutoLock envlock(m_env_mutex);

		/*
			Save players
		*/
		infostream<<"Server: Saving players"<<std::endl;
		m_env->serializePlayers(m_path_world);

		/*
			Save environment metadata
		*/
		infostream<<"Server: Saving environment metadata"<<std::endl;
		m_env->saveMeta(m_path_world);
	}
		
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
				m_env->removePlayer(peer_id);
			}*/
			
			// Delete client
			delete i.getNode()->getValue();
		}
	}
	
	// Delete things in the reverse order of creation
	delete m_env;
	delete m_rollback;
	delete m_event;
	delete m_itemdef;
	delete m_nodedef;
	delete m_craftdef;
	
	// Deinitialize scripting
	infostream<<"Server: Deinitializing scripting"<<std::endl;
	script_deinit(m_lua);

	// Delete detached inventories
	{
		for(std::map<std::string, Inventory*>::iterator
				i = m_detached_inventories.begin();
				i != m_detached_inventories.end(); i++){
			delete i->second;
		}
	}
}

void Server::start(unsigned short port)
{
	DSTACK(__FUNCTION_NAME);
	infostream<<"Starting server on port "<<port<<"..."<<std::endl;

	// Stop thread if already running
	m_thread.stop();
	
	// Initialize connection
	m_con.SetTimeoutMs(30);
	m_con.Serve(port);

	// Start thread
	m_thread.setRun(true);
	m_thread.Start();
	
	// ASCII art for the win!
	actionstream
	<<"        .__               __                   __   "<<std::endl
	<<"  _____ |__| ____   _____/  |_  ____   _______/  |_ "<<std::endl
	<<" /     \\|  |/    \\_/ __ \\   __\\/ __ \\ /  ___/\\   __\\"<<std::endl
	<<"|  Y Y  \\  |   |  \\  ___/|  | \\  ___/ \\___ \\  |  |  "<<std::endl
	<<"|__|_|  /__|___|  /\\___  >__|  \\___  >____  > |__|  "<<std::endl
	<<"      \\/        \\/     \\/          \\/     \\/        "<<std::endl;
	actionstream<<"World at ["<<m_path_world<<"]"<<std::endl;
	actionstream<<"Server for gameid=\""<<m_gamespec.id
			<<"\" listening on port "<<port<<"."<<std::endl;
}

void Server::stop()
{
	DSTACK(__FUNCTION_NAME);
	
	infostream<<"Server: Stopping and waiting threads"<<std::endl;

	// Stop threads (set run=false first so both start stopping)
	m_thread.setRun(false);
	m_emergethread.setRun(false);
	m_thread.stop();
	m_emergethread.stop();
	
	infostream<<"Server: Threads stopped"<<std::endl;
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
	// Throw if fatal error occurred in thread
	std::string async_err = m_async_fatal_error.get();
	if(async_err != ""){
		throw ServerError(async_err);
	}
}

void Server::AsyncRunStep()
{
	DSTACK(__FUNCTION_NAME);
	
	g_profiler->add("Server::AsyncRunStep (num)", 1);
	
	float dtime;
	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		dtime = m_step_dtime;
	}
	
	{
		// Send blocks to clients
		SendBlocks(dtime);
	}
	
	if(dtime < 0.001)
		return;
	
	g_profiler->add("Server::AsyncRunStep with dtime (num)", 1);

	//infostream<<"Server steps "<<dtime<<std::endl;
	//infostream<<"Server::AsyncRunStep(): dtime="<<dtime<<std::endl;
	
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
	
	{
		// Process connection's timeouts
		JMutexAutoLock lock2(m_con_mutex);
		ScopeProfiler sp(g_profiler, "Server: connection timeout processing");
		m_con.RunTimeouts(dtime);
	}
	
	{
		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();
	}

	/*
		Update time of day and overall game time
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);

		m_env->setTimeOfDaySpeed(g_settings->getFloat("time_speed"));

		/*
			Send to clients at constant intervals
		*/

		m_time_of_day_send_timer -= dtime;
		if(m_time_of_day_send_timer < 0.0)
		{
			m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");

			//JMutexAutoLock envlock(m_env_mutex);
			JMutexAutoLock conlock(m_con_mutex);

			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				RemoteClient *client = i.getNode()->getValue();
				SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
						m_env->getTimeOfDay(), g_settings->getFloat("time_speed"));
				// Send as reliable
				m_con.Send(client->peer_id, 0, data, true);
			}
		}
	}

	{
		JMutexAutoLock lock(m_env_mutex);
		// Step environment
		ScopeProfiler sp(g_profiler, "SEnv step");
		ScopeProfiler sp2(g_profiler, "SEnv step avg", SPT_AVG);
		m_env->step(dtime);
	}
		
	const float map_timer_and_unload_dtime = 2.92;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime))
	{
		JMutexAutoLock lock(m_env_mutex);
		// Run Map's timers and unload unused data
		ScopeProfiler sp(g_profiler, "Server: map timer and unload");
		m_env->getMap().timerUpdate(map_timer_and_unload_dtime,
				g_settings->getFloat("server_unload_unused_data_timeout"));
	}
	
	/*
		Do background stuff
	*/

	/*
		Handle players
	*/
	{
		JMutexAutoLock lock(m_env_mutex);
		JMutexAutoLock lock2(m_con_mutex);

		ScopeProfiler sp(g_profiler, "Server: handle players");

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			PlayerSAO *playersao = getPlayerSAO(client->peer_id);
			if(playersao == NULL)
				continue;

			/*
				Handle player HPs (die if hp=0)
			*/
			if(playersao->getHP() == 0 && playersao->m_hp_not_sent)
				DiePlayer(client->peer_id);

			/*
				Send player inventories and HPs if necessary
			*/
			if(playersao->m_teleported){
				SendMovePlayer(client->peer_id);
				playersao->m_teleported = false;
			}
			if(playersao->m_inventory_not_sent){
				UpdateCrafting(client->peer_id);
				SendInventory(client->peer_id);
			}
			if(playersao->m_hp_not_sent){
				SendPlayerHP(client->peer_id);
			}
		}
	}
	
	/* Transform liquids */
	m_liquid_transform_timer += dtime;
	if(m_liquid_transform_timer >= 1.00)
	{
		m_liquid_transform_timer -= 1.00;
		
		JMutexAutoLock lock(m_env_mutex);

		ScopeProfiler sp(g_profiler, "Server: liquid transform");

		core::map<v3s16, MapBlock*> modified_blocks;
		m_env->getMap().transformLiquids(modified_blocks);
#if 0		
		/*
			Update lighting
		*/
		core::map<v3s16, MapBlock*> lighting_modified_blocks;
		ServerMap &map = ((ServerMap&)m_env->getMap());
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
			
			if(m_clients.size() != 0)
				infostream<<"Players:"<<std::endl;
			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				//u16 peer_id = i.getNode()->getKey();
				RemoteClient *client = i.getNode()->getValue();
				Player *player = m_env->getPlayer(client->peer_id);
				if(player==NULL)
					continue;
				infostream<<"* "<<player->getName()<<"\t";
				client->PrintInfo(infostream);
			}
		}
	}

	//if(g_settings->getBool("enable_experimental"))
	{

	/*
		Check added and deleted active objects
	*/
	{
		//infostream<<"Server: Checking added and deleted active objects"<<std::endl;
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		ScopeProfiler sp(g_profiler, "Server: checking added and deleted objs");

		// Radius inside which objects are active
		s16 radius = g_settings->getS16("active_object_send_range_blocks");
		radius *= MAP_BLOCKSIZE;

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();

			// If definitions and textures have not been sent, don't
			// send objects either
			if(!client->definitions_sent)
				continue;

			Player *player = m_env->getPlayer(client->peer_id);
			if(player==NULL)
			{
				// This can happen if the client timeouts somehow
				/*infostream<<"WARNING: "<<__FUNCTION_NAME<<": Client "
						<<client->peer_id
						<<" has no associated player"<<std::endl;*/
				continue;
			}
			v3s16 pos = floatToInt(player->getPosition(), BS);

			core::map<u16, bool> removed_objects;
			core::map<u16, bool> added_objects;
			m_env->getRemovedActiveObjects(pos, radius,
					client->m_known_objects, removed_objects);
			m_env->getAddedActiveObjects(pos, radius,
					client->m_known_objects, added_objects);
			
			// Ignore if nothing happened
			if(removed_objects.size() == 0 && added_objects.size() == 0)
			{
				//infostream<<"active objects: none changed"<<std::endl;
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
				ServerActiveObject* obj = m_env->getActiveObject(id);

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
				ServerActiveObject* obj = m_env->getActiveObject(id);
				
				// Get object type
				u8 type = ACTIVEOBJECT_TYPE_INVALID;
				if(obj == NULL)
					infostream<<"WARNING: "<<__FUNCTION_NAME
							<<": NULL object"<<std::endl;
				else
					type = obj->getSendType();

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

			verbosestream<<"Server: Sent object remove/add: "
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
		
		m_env->setKnownActiveObjects(whatever);
#endif

	}

	/*
		Send object messages
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		ScopeProfiler sp(g_profiler, "Server: sending object messages");

		// Key = object id
		// Value = data sent by object
		core::map<u16, core::list<ActiveObjectMessage>* > buffered_messages;

		// Get active object messages from environment
		for(;;)
		{
			ActiveObjectMessage aom = m_env->getActiveObjectMessage();
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
				infostream<<"Server: Size of object message data: "
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
		// We will be accessing the environment and the connection
		JMutexAutoLock lock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		// Don't send too many at a time
		//u32 count = 0;

		// Single change sending is disabled if queue size is not small
		bool disable_single_change_sending = false;
		if(m_unsent_map_edit_queue.size() >= 4)
			disable_single_change_sending = true;

		int event_count = m_unsent_map_edit_queue.size();

		// We'll log the amount of each
		Profiler prof;

		while(m_unsent_map_edit_queue.size() != 0)
		{
			MapEditEvent* event = m_unsent_map_edit_queue.pop_front();
			
			// Players far away from the change are stored here.
			// Instead of sending the changes, MapBlocks are set not sent
			// for them.
			core::list<u16> far_players;

			if(event->type == MEET_ADDNODE)
			{
				//infostream<<"Server: MEET_ADDNODE"<<std::endl;
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
				//infostream<<"Server: MEET_REMOVENODE"<<std::endl;
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
				infostream<<"Server: MEET_BLOCK_NODE_METADATA_CHANGED"<<std::endl;
				prof.add("MEET_BLOCK_NODE_METADATA_CHANGED", 1);
				setBlockNotSent(event->p);
			}
			else if(event->type == MEET_OTHER)
			{
				infostream<<"Server: MEET_OTHER"<<std::endl;
				prof.add("MEET_OTHER", 1);
				for(core::map<v3s16, bool>::Iterator
						i = event->modified_blocks.getIterator();
						i.atEnd()==false; i++)
				{
					v3s16 p = i.getNode()->getKey();
					setBlockNotSent(p);
				}
			}
			else
			{
				prof.add("unknown", 1);
				infostream<<"WARNING: Server: Unknown MapEditEvent "
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
							m_env->getMap().getBlockNoCreateNoEx(p));
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

		if(event_count >= 5){
			infostream<<"Server: MapEditEvents:"<<std::endl;
			prof.print(infostream);
		} else if(event_count != 0){
			verbosestream<<"Server: MapEditEvents:"<<std::endl;
			prof.print(verbosestream);
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

			// Update m_enable_rollback_recording here too
			m_enable_rollback_recording =
					g_settings->getBool("enable_rollback_recording");
		}
	}

	// Save map, players and auth stuff
	{
		float &counter = m_savemap_timer;
		counter += dtime;
		if(counter >= g_settings->getFloat("server_map_save_interval"))
		{
			counter = 0.0;
			JMutexAutoLock lock(m_env_mutex);

			ScopeProfiler sp(g_profiler, "Server: saving stuff");

			//Ban stuff
			if(m_banmanager.isModified())
				m_banmanager.save();
			
			// Save changed parts of map
			m_env->getMap().save(MOD_STATE_WRITE_NEEDED);

			// Save players
			m_env->serializePlayers(m_path_world);
			
			// Save environment metadata
			m_env->saveMeta(m_path_world);
		}
	}
}

void Server::Receive()
{
	DSTACK(__FUNCTION_NAME);
	SharedBuffer<u8> data;
	u16 peer_id;
	u32 datasize;
	try{
		{
			JMutexAutoLock conlock(m_con_mutex);
			datasize = m_con.Receive(peer_id, data);
		}

		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();

		ProcessData(*data, datasize, peer_id);
	}
	catch(con::InvalidIncomingDataException &e)
	{
		infostream<<"Server::Receive(): "
				"InvalidIncomingDataException: what()="
				<<e.what()<<std::endl;
	}
	catch(con::PeerNotFoundException &e)
	{
		//NOTE: This is not needed anymore
		
		// The peer has been disconnected.
		// Find the associated player and remove it.

		/*JMutexAutoLock envlock(m_env_mutex);

		infostream<<"ServerThread: peer_id="<<peer_id
				<<" has apparently closed connection. "
				<<"Removing player."<<std::endl;

		m_env->removePlayer(peer_id);*/
	}
}

void Server::ProcessData(u8 *data, u32 datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	// Environment is locked first.
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);
	
	ScopeProfiler sp(g_profiler, "Server::ProcessData");
	
	try{
		Address address = m_con.GetPeerAddress(peer_id);
		std::string addr_s = address.serializeString();

		// drop player if is ip is banned
		if(m_banmanager.isIpBanned(addr_s)){
			infostream<<"Server: A banned client tried to connect from "
					<<addr_s<<"; banned name was "
					<<m_banmanager.getBanName(addr_s)<<std::endl;
			// This actually doesn't seem to transfer to the client
			SendAccessDenied(m_con, peer_id,
					L"Your ip is banned. Banned name was "
					+narrow_to_wide(m_banmanager.getBanName(addr_s)));
			m_con.DeletePeer(peer_id);
			return;
		}
	}
	catch(con::PeerNotFoundException &e)
	{
		infostream<<"Server::ProcessData(): Cancelling: peer "
				<<peer_id<<" not found"<<std::endl;
		return;
	}
	
	std::string addr_s = m_con.GetPeerAddress(peer_id).serializeString();

	u8 peer_ser_ver = getClient(peer_id)->serialization_version;

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

		verbosestream<<"Server: Got TOSERVER_INIT from "
				<<peer_id<<std::endl;

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
		getClient(peer_id)->pending_serialization_version = deployed;
		
		if(deployed == SER_FMT_VER_INVALID)
		{
			actionstream<<"Server: A mismatched client tried to connect from "
					<<addr_s<<std::endl;
			infostream<<"Server: Cannot negotiate "
					"serialization version with peer "
					<<peer_id<<std::endl;
			SendAccessDenied(m_con, peer_id, std::wstring(
					L"Your client's version is not supported.\n"
					L"Server version is ")
					+ narrow_to_wide(VERSION_STRING) + L"."
			);
			return;
		}
		
		/*
			Read and check network protocol version
		*/

		u16 net_proto_version = 0;
		if(datasize >= 2+1+PLAYERNAME_SIZE+PASSWORD_SIZE+2)
		{
			net_proto_version = readU16(&data[2+1+PLAYERNAME_SIZE+PASSWORD_SIZE]);
		}

		getClient(peer_id)->net_proto_version = net_proto_version;

		if(net_proto_version == 0)
		{
			actionstream<<"Server: An old tried to connect from "<<addr_s
					<<std::endl;
			SendAccessDenied(m_con, peer_id, std::wstring(
					L"Your client's version is not supported.\n"
					L"Server version is ")
					+ narrow_to_wide(VERSION_STRING) + L"."
			);
			return;
		}
		
		if(g_settings->getBool("strict_protocol_version_checking"))
		{
			if(net_proto_version != PROTOCOL_VERSION)
			{
				actionstream<<"Server: A mismatched client tried to connect"
						<<" from "<<addr_s<<std::endl;
				SendAccessDenied(m_con, peer_id, std::wstring(
						L"Your client's version is not supported.\n"
						L"Server version is ")
						+ narrow_to_wide(VERSION_STRING) + L",\n"
						+ L"server's PROTOCOL_VERSION is "
						+ narrow_to_wide(itos(PROTOCOL_VERSION))
						+ L", client's PROTOCOL_VERSION is "
						+ narrow_to_wide(itos(net_proto_version))
				);
				return;
			}
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
			actionstream<<"Server: Player with an empty name "
					<<"tried to connect from "<<addr_s<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Empty name");
			return;
		}

		if(string_allowed(playername, PLAYERNAME_ALLOWED_CHARS)==false)
		{
			actionstream<<"Server: Player with an invalid name "
					<<"tried to connect from "<<addr_s<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Name contains unallowed characters");
			return;
		}

		infostream<<"Server: New connection: \""<<playername<<"\" from "
				<<m_con.GetPeerAddress(peer_id).serializeString()<<std::endl;

		// Get password
		char given_password[PASSWORD_SIZE];
		if(datasize < 2+1+PLAYERNAME_SIZE+PASSWORD_SIZE)
		{
			// old version - assume blank password
			given_password[0] = 0;
		}
		else
		{
			for(u32 i=0; i<PASSWORD_SIZE-1; i++)
			{
				given_password[i] = data[23+i];
			}
			given_password[PASSWORD_SIZE-1] = 0;
		}

		if(!base64_is_valid(given_password)){
			infostream<<"Server: "<<playername
					<<" supplied invalid password hash"<<std::endl;
			SendAccessDenied(m_con, peer_id, L"Invalid password hash");
			return;
		}
		
		std::string checkpwd; // Password hash to check against
		bool has_auth = scriptapi_get_auth(m_lua, playername, &checkpwd, NULL);
		
		// If no authentication info exists for user, create it
		if(!has_auth){
			if(!isSingleplayer() &&
					g_settings->getBool("disallow_empty_password") &&
					std::string(given_password) == ""){
				SendAccessDenied(m_con, peer_id, L"Empty passwords are "
						L"disallowed. Set a password and try again.");
				return;
			}
			std::wstring raw_default_password =
				narrow_to_wide(g_settings->get("default_password"));
			std::string initial_password =
				translatePassword(playername, raw_default_password);

			// If default_password is empty, allow any initial password
			if (raw_default_password.length() == 0)
				initial_password = given_password;

			scriptapi_create_auth(m_lua, playername, initial_password);
		}
		
		has_auth = scriptapi_get_auth(m_lua, playername, &checkpwd, NULL);

		if(!has_auth){
			SendAccessDenied(m_con, peer_id, L"Not allowed to login");
			return;
		}

		if(given_password != checkpwd){
			infostream<<"Server: peer_id="<<peer_id
					<<": supplied invalid password for "
					<<playername<<std::endl;
			SendAccessDenied(m_con, peer_id, L"Invalid password");
			return;
		}

		// Do not allow multiple players in simple singleplayer mode.
		// This isn't a perfect way to do it, but will suffice for now.
		if(m_simple_singleplayer_mode && m_clients.size() > 1){
			infostream<<"Server: Not allowing another client to connect in"
					<<" simple singleplayer mode"<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Running in simple singleplayer mode.");
			return;
		}
		
		// Enforce user limit.
		// Don't enforce for users that have some admin right
		if(m_clients.size() >= g_settings->getU16("max_users") &&
				!checkPriv(playername, "server") &&
				!checkPriv(playername, "ban") &&
				!checkPriv(playername, "privs") &&
				!checkPriv(playername, "password") &&
				playername != g_settings->get("name"))
		{
			actionstream<<"Server: "<<playername<<" tried to join, but there"
					<<" are already max_users="
					<<g_settings->getU16("max_users")<<" players."<<std::endl;
			SendAccessDenied(m_con, peer_id, L"Too many users.");
			return;
		}

		// Get player
		PlayerSAO *playersao = emergePlayer(playername, peer_id);

		// If failed, cancel
		if(playersao == NULL)
		{
			errorstream<<"Server: peer_id="<<peer_id
					<<": failed to emerge player"<<std::endl;
			return;
		}

		/*
			Answer with a TOCLIENT_INIT
		*/
		{
			SharedBuffer<u8> reply(2+1+6+8);
			writeU16(&reply[0], TOCLIENT_INIT);
			writeU8(&reply[2], deployed);
			writeV3S16(&reply[2+1], floatToInt(playersao->getPlayer()->getPosition()+v3f(0,BS/2,0), BS));
			writeU64(&reply[2+1+6], m_env->getServerMap().getSeed());
			
			// Send as reliable
			m_con.Send(peer_id, 0, reply, true);
		}

		/*
			Send complete position information
		*/
		SendMovePlayer(peer_id);

		return;
	}

	if(command == TOSERVER_INIT2)
	{
		verbosestream<<"Server: Got TOSERVER_INIT2 from "
				<<peer_id<<std::endl;

		Player *player = m_env->getPlayer(peer_id);
		if(!player){
			verbosestream<<"Server: TOSERVER_INIT2: "
					<<"Player not found; ignoring."<<std::endl;
			return;
		}

		getClient(peer_id)->serialization_version
				= getClient(peer_id)->pending_serialization_version;

		/*
			Send some initialization data
		*/

		infostream<<"Server: Sending content to "
				<<getPlayerName(peer_id)<<std::endl;

		// Send item definitions
		SendItemDef(m_con, peer_id, m_itemdef);
		
		// Send node definitions
		SendNodeDef(m_con, peer_id, m_nodedef);
		
		// Send media announcement
		sendMediaAnnouncement(peer_id);
		
		// Send privileges
		SendPlayerPrivileges(peer_id);
		
		// Send inventory formspec
		SendPlayerInventoryFormspec(peer_id);

		// Send inventory
		UpdateCrafting(peer_id);
		SendInventory(peer_id);

		// Send HP
		SendPlayerHP(peer_id);
		
		// Send detached inventories
		sendDetachedInventories(peer_id);
		
		// Show death screen if necessary
		if(player->hp == 0)
			SendDeathscreen(m_con, peer_id, false, v3f(0,0,0));

		// Send time of day
		{
			SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
					m_env->getTimeOfDay(), g_settings->getFloat("time_speed"));
			m_con.Send(peer_id, 0, data, true);
		}
		
		// Note things in chat if not in simple singleplayer mode
		if(!m_simple_singleplayer_mode)
		{
			// Send information about server to player in chat
			SendChatMessage(peer_id, getStatusString());
			
			// Send information about joining in chat
			{
				std::wstring name = L"unknown";
				Player *player = m_env->getPlayer(peer_id);
				if(player != NULL)
					name = narrow_to_wide(player->getName());
				
				std::wstring message;
				message += L"*** ";
				message += name;
				message += L" joined the game.";
				BroadcastChatMessage(message);
			}
		}
		
		// Warnings about protocol version can be issued here
		if(getClient(peer_id)->net_proto_version < PROTOCOL_VERSION)
		{
			SendChatMessage(peer_id, L"# Server: WARNING: YOUR CLIENT IS OLD AND MAY WORK PROPERLY WITH THIS SERVER!");
		}

		/*
			Print out action
		*/
		{
			std::ostringstream os(std::ios_base::binary);
			for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++)
			{
				RemoteClient *client = i.getNode()->getValue();
				assert(client->peer_id == i.getNode()->getKey());
				if(client->serialization_version == SER_FMT_VER_INVALID)
					continue;
				// Get player
				Player *player = m_env->getPlayer(client->peer_id);
				if(!player)
					continue;
				// Get name of player
				os<<player->getName()<<" ";
			}

			actionstream<<player->getName()<<" joins game. List of players: "
					<<os.str()<<std::endl;
		}

		return;
	}

	if(peer_ser_ver == SER_FMT_VER_INVALID)
	{
		infostream<<"Server::ProcessData(): Cancelling: Peer"
				" serialization format invalid or not initialized."
				" Skipping incoming command="<<command<<std::endl;
		return;
	}
	
	Player *player = m_env->getPlayer(peer_id);
	if(player == NULL){
		infostream<<"Server::ProcessData(): Cancelling: "
				"No player for peer_id="<<peer_id
				<<std::endl;
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if(playersao == NULL){
		infostream<<"Server::ProcessData(): Cancelling: "
				"No player object for peer_id="<<peer_id
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
		
		/*infostream<<"Server::ProcessData(): Moved player "<<peer_id<<" to "
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
			/*infostream<<"Server: GOTBLOCKS ("
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
			/*infostream<<"Server: DELETEDBLOCKS ("
					<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
			RemoteClient *client = getClient(peer_id);
			client->SetBlockNotSent(p);
		}
	}
	else if(command == TOSERVER_CLICK_OBJECT)
	{
		infostream<<"Server: CLICK_OBJECT not supported anymore"<<std::endl;
		return;
	}
	else if(command == TOSERVER_CLICK_ACTIVEOBJECT)
	{
		infostream<<"Server: CLICK_ACTIVEOBJECT not supported anymore"<<std::endl;
		return;
	}
	else if(command == TOSERVER_GROUND_ACTION)
	{
		infostream<<"Server: GROUND_ACTION not supported anymore"<<std::endl;
		return;

	}
	else if(command == TOSERVER_RELEASE)
	{
		infostream<<"Server: RELEASE not supported anymore"<<std::endl;
		return;
	}
	else if(command == TOSERVER_SIGNTEXT)
	{
		infostream<<"Server: SIGNTEXT not supported anymore"
				<<std::endl;
		return;
	}
	else if(command == TOSERVER_SIGNNODETEXT)
	{
		infostream<<"Server: SIGNNODETEXT not supported anymore"
				<<std::endl;
		return;
	}
	else if(command == TOSERVER_INVENTORY_ACTION)
	{
		// Strip command and create a stream
		std::string datastring((char*)&data[2], datasize-2);
		verbosestream<<"TOSERVER_INVENTORY_ACTION: data="<<datastring<<std::endl;
		std::istringstream is(datastring, std::ios_base::binary);
		// Create an action
		InventoryAction *a = InventoryAction::deSerialize(is);
		if(a == NULL)
		{
			infostream<<"TOSERVER_INVENTORY_ACTION: "
					<<"InventoryAction::deSerialize() returned NULL"
					<<std::endl;
			return;
		}

		// If something goes wrong, this player is to blame
		RollbackScopeActor rollback_scope(m_rollback,
				std::string("player:")+player->getName());

		/*
			Note: Always set inventory not sent, to repair cases
			where the client made a bad prediction.
		*/

		/*
			Handle restrictions and special cases of the move action
		*/
		if(a->getType() == IACTION_MOVE)
		{
			IMoveAction *ma = (IMoveAction*)a;

			ma->from_inv.applyCurrentPlayer(player->getName());
			ma->to_inv.applyCurrentPlayer(player->getName());

			setInventoryModified(ma->from_inv);
			setInventoryModified(ma->to_inv);

			bool from_inv_is_current_player =
				(ma->from_inv.type == InventoryLocation::PLAYER) &&
				(ma->from_inv.name == player->getName());

			bool to_inv_is_current_player =
				(ma->to_inv.type == InventoryLocation::PLAYER) &&
				(ma->to_inv.name == player->getName());

			/*
				Disable moving items out of craftpreview
			*/
			if(ma->from_list == "craftpreview")
			{
				infostream<<"Ignoring IMoveAction from "
						<<(ma->from_inv.dump())<<":"<<ma->from_list
						<<" to "<<(ma->to_inv.dump())<<":"<<ma->to_list
						<<" because src is "<<ma->from_list<<std::endl;
				delete a;
				return;
			}

			/*
				Disable moving items into craftresult and craftpreview
			*/
			if(ma->to_list == "craftpreview" || ma->to_list == "craftresult")
			{
				infostream<<"Ignoring IMoveAction from "
						<<(ma->from_inv.dump())<<":"<<ma->from_list
						<<" to "<<(ma->to_inv.dump())<<":"<<ma->to_list
						<<" because dst is "<<ma->to_list<<std::endl;
				delete a;
				return;
			}

			// Disallow moving items in elsewhere than player's inventory
			// if not allowed to interact
			if(!checkPriv(player->getName(), "interact") &&
					(!from_inv_is_current_player ||
					!to_inv_is_current_player))
			{
				infostream<<"Cannot move outside of player's inventory: "
						<<"No interact privilege"<<std::endl;
				delete a;
				return;
			}
		}
		/*
			Handle restrictions and special cases of the drop action
		*/
		else if(a->getType() == IACTION_DROP)
		{
			IDropAction *da = (IDropAction*)a;

			da->from_inv.applyCurrentPlayer(player->getName());

			setInventoryModified(da->from_inv);

			// Disallow dropping items if not allowed to interact
			if(!checkPriv(player->getName(), "interact"))
			{
				delete a;
				return;
			}
		}
		/*
			Handle restrictions and special cases of the craft action
		*/
		else if(a->getType() == IACTION_CRAFT)
		{
			ICraftAction *ca = (ICraftAction*)a;

			ca->craft_inv.applyCurrentPlayer(player->getName());

			setInventoryModified(ca->craft_inv);

			//bool craft_inv_is_current_player =
			//	(ca->craft_inv.type == InventoryLocation::PLAYER) &&
			//	(ca->craft_inv.name == player->getName());

			// Disallow crafting if not allowed to interact
			if(!checkPriv(player->getName(), "interact"))
			{
				infostream<<"Cannot craft: "
						<<"No interact privilege"<<std::endl;
				delete a;
				return;
			}
		}
		
		// Do the action
		a->apply(this, playersao, this);
		// Eat the action
		delete a;
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

		// If something goes wrong, this player is to blame
		RollbackScopeActor rollback_scope(m_rollback,
				std::string("player:")+player->getName());

		// Get player name of this client
		std::wstring name = narrow_to_wide(player->getName());
		
		// Run script hook
		bool ate = scriptapi_on_chat_message(m_lua, player->getName(),
				wide_to_narrow(message));
		// If script ate the message, don't proceed
		if(ate)
			return;
		
		// Line to send to players
		std::wstring line;
		// Whether to send to the player that sent the line
		bool send_to_sender = false;
		// Whether to send to other players
		bool send_to_others = false;
		
		// Commands are implemented in Lua, so only catch invalid
		// commands that were not "eaten" and send an error back
		if(message[0] == L'/')
		{
			message = message.substr(1);
			send_to_sender = true;
			if(message.length() == 0)
				line += L"-!- Empty command";
			else
				line += L"-!- Invalid command: " + str_split(message, L' ')[0];
		}
		else
		{
			if(checkPriv(player->getName(), "shout")){
				line += L"<";
				line += name;
				line += L"> ";
				line += message;
				send_to_others = true;
			} else {
				line += L"-!- You don't have permission to shout.";
				send_to_sender = true;
			}
		}
		
		if(line != L"")
		{
			if(send_to_others)
				actionstream<<"CHAT: "<<wide_to_narrow(line)<<std::endl;

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
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		u8 damage = readU8(is);

		actionstream<<player->getName()<<" damaged by "
				<<(int)damage<<" hp at "<<PP(player->getPosition()/BS)
				<<std::endl;

		playersao->setHP(playersao->getHP() - damage);

		if(playersao->getHP() == 0 && playersao->m_hp_not_sent)
			DiePlayer(peer_id);

		if(playersao->m_hp_not_sent)
			SendPlayerHP(peer_id);
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

		if(!base64_is_valid(newpwd)){
			infostream<<"Server: "<<player->getName()<<" supplied invalid password hash"<<std::endl;
			// Wrong old password supplied!!
			SendChatMessage(peer_id, L"Invalid new password hash supplied. Password NOT changed.");
			return;
		}

		infostream<<"Server: Client requests a password change from "
				<<"'"<<oldpwd<<"' to '"<<newpwd<<"'"<<std::endl;

		std::string playername = player->getName();

		std::string checkpwd;
		scriptapi_get_auth(m_lua, playername, &checkpwd, NULL);

		if(oldpwd != checkpwd)
		{
			infostream<<"Server: invalid old password"<<std::endl;
			// Wrong old password supplied!!
			SendChatMessage(peer_id, L"Invalid old password supplied. Password NOT changed.");
			return;
		}

		bool success = scriptapi_set_password(m_lua, playername, newpwd);
		if(success){
			actionstream<<player->getName()<<" changes password"<<std::endl;
			SendChatMessage(peer_id, L"Password change successful.");
		} else {
			actionstream<<player->getName()<<" tries to change password but "
					<<"it fails"<<std::endl;
			SendChatMessage(peer_id, L"Password change failed or inavailable.");
		}
	}
	else if(command == TOSERVER_PLAYERITEM)
	{
		if (datasize < 2+2)
			return;

		u16 item = readU16(&data[2]);
		playersao->setWieldIndex(item);
	}
	else if(command == TOSERVER_RESPAWN)
	{
		if(player->hp != 0)
			return;
		
		RespawnPlayer(peer_id);
		
		actionstream<<player->getName()<<" respawns at "
				<<PP(player->getPosition()/BS)<<std::endl;

		// ActiveObject is added to environment in AsyncRunStep after
		// the previous addition has been succesfully removed
	}
	else if(command == TOSERVER_REQUEST_MEDIA) {
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		core::list<MediaRequest> tosend;
		u16 numfiles = readU16(is);

		infostream<<"Sending "<<numfiles<<" files to "
				<<getPlayerName(peer_id)<<std::endl;
		verbosestream<<"TOSERVER_REQUEST_MEDIA: "<<std::endl;

		for(int i = 0; i < numfiles; i++) {
			std::string name = deSerializeString(is);
			tosend.push_back(MediaRequest(name));
			verbosestream<<"TOSERVER_REQUEST_MEDIA: requested file "
					<<name<<std::endl;
		}

		sendRequestedMedia(peer_id, tosend);

		// Now the client should know about everything
		// (definitions and files)
		getClient(peer_id)->definitions_sent = true;
	}
	else if(command == TOSERVER_INTERACT)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		/*
			[0] u16 command
			[2] u8 action
			[3] u16 item
			[5] u32 length of the next item
			[9] serialized PointedThing
			actions:
			0: start digging (from undersurface) or use
			1: stop digging (all parameters ignored)
			2: digging completed
			3: place block or item (to abovesurface)
			4: use item
		*/
		u8 action = readU8(is);
		u16 item_i = readU16(is);
		std::istringstream tmp_is(deSerializeLongString(is), std::ios::binary);
		PointedThing pointed;
		pointed.deSerialize(tmp_is);

		verbosestream<<"TOSERVER_INTERACT: action="<<(int)action<<", item="
				<<item_i<<", pointed="<<pointed.dump()<<std::endl;

		if(player->hp == 0)
		{
			verbosestream<<"TOSERVER_INTERACT: "<<player->getName()
				<<" tried to interact, but is dead!"<<std::endl;
			return;
		}

		v3f player_pos = playersao->getLastGoodPosition();

		// Update wielded item
		playersao->setWieldIndex(item_i);

		// Get pointed to node (undefined if not POINTEDTYPE_NODE)
		v3s16 p_under = pointed.node_undersurface;
		v3s16 p_above = pointed.node_abovesurface;

		// Get pointed to object (NULL if not POINTEDTYPE_OBJECT)
		ServerActiveObject *pointed_object = NULL;
		if(pointed.type == POINTEDTHING_OBJECT)
		{
			pointed_object = m_env->getActiveObject(pointed.object_id);
			if(pointed_object == NULL)
			{
				verbosestream<<"TOSERVER_INTERACT: "
					"pointed object is NULL"<<std::endl;
				return;
			}

		}

		v3f pointed_pos_under = player_pos;
		v3f pointed_pos_above = player_pos;
		if(pointed.type == POINTEDTHING_NODE)
		{
			pointed_pos_under = intToFloat(p_under, BS);
			pointed_pos_above = intToFloat(p_above, BS);
		}
		else if(pointed.type == POINTEDTHING_OBJECT)
		{
			pointed_pos_under = pointed_object->getBasePosition();
			pointed_pos_above = pointed_pos_under;
		}

		/*
			Check that target is reasonably close
			(only when digging or placing things)
		*/
		if(action == 0 || action == 2 || action == 3)
		{
			float d = player_pos.getDistanceFrom(pointed_pos_under);
			float max_d = BS * 14; // Just some large enough value
			if(d > max_d){
				actionstream<<"Player "<<player->getName()
						<<" tried to access "<<pointed.dump()
						<<" from too far: "
						<<"d="<<d<<", max_d="<<max_d
						<<". ignoring."<<std::endl;
				// Re-send block to revert change on client-side
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
				client->SetBlockNotSent(blockpos);
				// Do nothing else
				return;
			}
		}

		/*
			Make sure the player is allowed to do it
		*/
		if(!checkPriv(player->getName(), "interact"))
		{
			actionstream<<player->getName()<<" attempted to interact with "
					<<pointed.dump()<<" without 'interact' privilege"
					<<std::endl;
			// Re-send block to revert change on client-side
			RemoteClient *client = getClient(peer_id);
			// Digging completed -> under
			if(action == 2){
				v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
				client->SetBlockNotSent(blockpos);
			}
			// Placement -> above
			if(action == 3){
				v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_above, BS));
				client->SetBlockNotSent(blockpos);
			}
			return;
		}

		/*
			If something goes wrong, this player is to blame
		*/
		RollbackScopeActor rollback_scope(m_rollback,
				std::string("player:")+player->getName());

		/*
			0: start digging or punch object
		*/
		if(action == 0)
		{
			if(pointed.type == POINTEDTHING_NODE)
			{
				/*
					NOTE: This can be used in the future to check if
					somebody is cheating, by checking the timing.
				*/
				MapNode n(CONTENT_IGNORE);
				try
				{
					n = m_env->getMap().getNode(p_under);
				}
				catch(InvalidPositionException &e)
				{
					infostream<<"Server: Not punching: Node not found."
							<<" Adding block to emerge queue."
							<<std::endl;
					m_emerge_queue.addBlock(peer_id,
							getNodeBlockPos(p_above), BLOCK_EMERGE_FLAG_FROMDISK);
				}
				if(n.getContent() != CONTENT_IGNORE)
					scriptapi_node_on_punch(m_lua, p_under, n, playersao);
				// Cheat prevention
				playersao->noCheatDigStart(p_under);
			}
			else if(pointed.type == POINTEDTHING_OBJECT)
			{
				// Skip if object has been removed
				if(pointed_object->m_removed)
					return;

				actionstream<<player->getName()<<" punches object "
						<<pointed.object_id<<": "
						<<pointed_object->getDescription()<<std::endl;

				ItemStack punchitem = playersao->getWieldedItem();
				ToolCapabilities toolcap =
						punchitem.getToolCapabilities(m_itemdef);
				v3f dir = (pointed_object->getBasePosition() -
						(player->getPosition() + player->getEyeOffset())
							).normalize();
				float time_from_last_punch =
					playersao->resetTimeFromLastPunch();
				pointed_object->punch(dir, &toolcap, playersao,
						time_from_last_punch);
			}

		} // action == 0

		/*
			1: stop digging
		*/
		else if(action == 1)
		{
		} // action == 1

		/*
			2: Digging completed
		*/
		else if(action == 2)
		{
			// Only digging of nodes
			if(pointed.type == POINTEDTHING_NODE)
			{
				MapNode n(CONTENT_IGNORE);
				try
				{
					n = m_env->getMap().getNode(p_under);
				}
				catch(InvalidPositionException &e)
				{
					infostream<<"Server: Not finishing digging: Node not found."
							<<" Adding block to emerge queue."
							<<std::endl;
					m_emerge_queue.addBlock(peer_id,
							getNodeBlockPos(p_above), BLOCK_EMERGE_FLAG_FROMDISK);
				}

				/* Cheat prevention */
				bool is_valid_dig = true;
				if(!isSingleplayer() && !g_settings->getBool("disable_anticheat"))
				{
					v3s16 nocheat_p = playersao->getNoCheatDigPos();
					float nocheat_t = playersao->getNoCheatDigTime();
					playersao->noCheatDigEnd();
					// If player didn't start digging this, ignore dig
					if(nocheat_p != p_under){
						infostream<<"Server: NoCheat: "<<player->getName()
								<<" started digging "
								<<PP(nocheat_p)<<" and completed digging "
								<<PP(p_under)<<"; not digging."<<std::endl;
						is_valid_dig = false;
					}
					// Get player's wielded item
					ItemStack playeritem;
					InventoryList *mlist = playersao->getInventory()->getList("main");
					if(mlist != NULL)
						playeritem = mlist->getItem(playersao->getWieldIndex());
					ToolCapabilities playeritem_toolcap =
							playeritem.getToolCapabilities(m_itemdef);
					// Get diggability and expected digging time
					DigParams params = getDigParams(m_nodedef->get(n).groups,
							&playeritem_toolcap);
					// If can't dig, try hand
					if(!params.diggable){
						const ItemDefinition &hand = m_itemdef->get("");
						const ToolCapabilities *tp = hand.tool_capabilities;
						if(tp)
							params = getDigParams(m_nodedef->get(n).groups, tp);
					}
					// If can't dig, ignore dig
					if(!params.diggable){
						infostream<<"Server: NoCheat: "<<player->getName()
								<<" completed digging "<<PP(p_under)
								<<", which is not diggable with tool. not digging."
								<<std::endl;
						is_valid_dig = false;
					}
					// If time is considerably too short, ignore dig
					// Check time only for medium and slow timed digs
					if(params.diggable && params.time > 0.3 && nocheat_t < 0.5 * params.time){
						infostream<<"Server: NoCheat: "<<player->getName()
								<<" completed digging "
								<<PP(p_under)<<" in "<<nocheat_t<<"s; expected "
								<<params.time<<"s; not digging."<<std::endl;
						is_valid_dig = false;
					}
				}

				/* Actually dig node */

				if(is_valid_dig && n.getContent() != CONTENT_IGNORE)
					scriptapi_node_on_dig(m_lua, p_under, n, playersao);

				// Send unusual result (that is, node not being removed)
				if(m_env->getMap().getNodeNoEx(p_under).getContent() != CONTENT_AIR)
				{
					// Re-send block to revert change on client-side
					RemoteClient *client = getClient(peer_id);
					v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
					client->SetBlockNotSent(blockpos);
				}
			}
		} // action == 2
		
		/*
			3: place block or right-click object
		*/
		else if(action == 3)
		{
			ItemStack item = playersao->getWieldedItem();

			// Reset build time counter
			if(pointed.type == POINTEDTHING_NODE &&
					item.getDefinition(m_itemdef).type == ITEM_NODE)
				getClient(peer_id)->m_time_from_building = 0.0;

			if(pointed.type == POINTEDTHING_OBJECT)
			{
				// Right click object

				// Skip if object has been removed
				if(pointed_object->m_removed)
					return;

				actionstream<<player->getName()<<" right-clicks object "
						<<pointed.object_id<<": "
						<<pointed_object->getDescription()<<std::endl;

				// Do stuff
				pointed_object->rightClick(playersao);
			}
			else if(scriptapi_item_on_place(m_lua,
					item, playersao, pointed))
			{
				// Placement was handled in lua

				// Apply returned ItemStack
				playersao->setWieldedItem(item);
			}
			
			// If item has node placement prediction, always send the above
			// node to make sure the client knows what exactly happened
			if(item.getDefinition(m_itemdef).node_placement_prediction != ""){
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_above, BS));
				client->SetBlockNotSent(blockpos);
			}
		} // action == 3

		/*
			4: use
		*/
		else if(action == 4)
		{
			ItemStack item = playersao->getWieldedItem();

			actionstream<<player->getName()<<" uses "<<item.name
					<<", pointing at "<<pointed.dump()<<std::endl;

			if(scriptapi_item_on_use(m_lua,
					item, playersao, pointed))
			{
				// Apply returned ItemStack
				playersao->setWieldedItem(item);
			}

		} // action == 4

		/*
			Catch invalid actions
		*/
		else
		{
			infostream<<"WARNING: Server: Invalid action "
					<<action<<std::endl;
		}
	}
	else if(command == TOSERVER_REMOVED_SOUNDS)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		int num = readU16(is);
		for(int k=0; k<num; k++){
			s32 id = readS32(is);
			std::map<s32, ServerPlayingSound>::iterator i =
					m_playing_sounds.find(id);
			if(i == m_playing_sounds.end())
				continue;
			ServerPlayingSound &psound = i->second;
			psound.clients.erase(peer_id);
			if(psound.clients.size() == 0)
				m_playing_sounds.erase(i++);
		}
	}
	else if(command == TOSERVER_NODEMETA_FIELDS)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		v3s16 p = readV3S16(is);
		std::string formname = deSerializeString(is);
		int num = readU16(is);
		std::map<std::string, std::string> fields;
		for(int k=0; k<num; k++){
			std::string fieldname = deSerializeString(is);
			std::string fieldvalue = deSerializeLongString(is);
			fields[fieldname] = fieldvalue;
		}

		// If something goes wrong, this player is to blame
		RollbackScopeActor rollback_scope(m_rollback,
				std::string("player:")+player->getName());

		// Check the target node for rollback data; leave others unnoticed
		RollbackNode rn_old(&m_env->getMap(), p, this);

		scriptapi_node_on_receive_fields(m_lua, p, formname, fields,
				playersao);

		// Report rollback data
		RollbackNode rn_new(&m_env->getMap(), p, this);
		if(rollback() && rn_new != rn_old){
			RollbackAction action;
			action.setSetNode(p, rn_old, rn_new);
			rollback()->reportAction(action);
		}
	}
	else if(command == TOSERVER_INVENTORY_FIELDS)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		std::string formname = deSerializeString(is);
		int num = readU16(is);
		std::map<std::string, std::string> fields;
		for(int k=0; k<num; k++){
			std::string fieldname = deSerializeString(is);
			std::string fieldvalue = deSerializeLongString(is);
			fields[fieldname] = fieldvalue;
		}

		scriptapi_on_player_receive_fields(m_lua, playersao, formname, fields);
	}
	else
	{
		infostream<<"Server::ProcessData(): Ignoring "
				"unknown command "<<command<<std::endl;
	}
	
	} //try
	catch(SendFailedException &e)
	{
		errorstream<<"Server::ProcessData(): SendFailedException: "
				<<"what="<<e.what()
				<<std::endl;
	}
}

void Server::onMapEditEvent(MapEditEvent *event)
{
	//infostream<<"Server::onMapEditEvent()"<<std::endl;
	if(m_ignore_map_edit_events)
		return;
	if(m_ignore_map_edit_events_area.contains(event->getArea()))
		return;
	MapEditEvent *e = event->clone();
	m_unsent_map_edit_queue.push_back(e);
}

Inventory* Server::getInventory(const InventoryLocation &loc)
{
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
	{}
	break;
	case InventoryLocation::CURRENT_PLAYER:
	{}
	break;
	case InventoryLocation::PLAYER:
	{
		Player *player = m_env->getPlayer(loc.name.c_str());
		if(!player)
			return NULL;
		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return NULL;
		return playersao->getInventory();
	}
	break;
	case InventoryLocation::NODEMETA:
	{
		NodeMetadata *meta = m_env->getMap().getNodeMetadata(loc.p);
		if(!meta)
			return NULL;
		return meta->getInventory();
	}
	break;
	case InventoryLocation::DETACHED:
	{
		if(m_detached_inventories.count(loc.name) == 0)
			return NULL;
		return m_detached_inventories[loc.name];
	}
	break;
	default:
		assert(0);
	}
	return NULL;
}
void Server::setInventoryModified(const InventoryLocation &loc)
{
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
	{}
	break;
	case InventoryLocation::PLAYER:
	{
		Player *player = m_env->getPlayer(loc.name.c_str());
		if(!player)
			return;
		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return;
		playersao->m_inventory_not_sent = true;
		playersao->m_wielded_item_not_sent = true;
	}
	break;
	case InventoryLocation::NODEMETA:
	{
		v3s16 blockpos = getNodeBlockPos(loc.p);

		MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
		if(block)
			block->raiseModified(MOD_STATE_WRITE_NEEDED);
		
		setBlockNotSent(blockpos);
	}
	break;
	case InventoryLocation::DETACHED:
	{
		sendDetachedInventoryToAll(loc.name);
	}
	break;
	default:
		assert(0);
	}
}

core::list<PlayerInfo> Server::getPlayerInfo()
{
	DSTACK(__FUNCTION_NAME);
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);
	
	core::list<PlayerInfo> list;

	core::list<Player*> players = m_env->getPlayers();
	
	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		PlayerInfo info;

		Player *player = *i;

		try{
			// Copy info from connection to info struct
			info.id = player->peer_id;
			info.address = m_con.GetPeerAddress(player->peer_id);
			info.avg_rtt = m_con.GetPeerAvgRTT(player->peer_id);
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
	verbosestream<<"Server::peerAdded(): peer->id="
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
	verbosestream<<"Server::deletingPeer(): peer->id="
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

void Server::SendDeathscreen(con::Connection &con, u16 peer_id,
		bool set_camera_point_target, v3f camera_point_target)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_DEATHSCREEN);
	writeU8(os, set_camera_point_target);
	writeV3F1000(os, camera_point_target);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendItemDef(con::Connection &con, u16 peer_id,
		IItemDefManager *itemdef)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized ItemDefManager
	*/
	writeU16(os, TOCLIENT_ITEMDEF);
	std::ostringstream tmp_os(std::ios::binary);
	itemdef->serialize(tmp_os);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);
	os<<serializeLongString(tmp_os2.str());

	// Make data buffer
	std::string s = os.str();
	verbosestream<<"Server: Sending item definitions to id("<<peer_id
			<<"): size="<<s.size()<<std::endl;
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendNodeDef(con::Connection &con, u16 peer_id,
		INodeDefManager *nodedef)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized NodeDefManager
	*/
	writeU16(os, TOCLIENT_NODEDEF);
	std::ostringstream tmp_os(std::ios::binary);
	nodedef->serialize(tmp_os);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);
	os<<serializeLongString(tmp_os2.str());

	// Make data buffer
	std::string s = os.str();
	verbosestream<<"Server: Sending node definitions to id("<<peer_id
			<<"): size="<<s.size()<<std::endl;
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

/*
	Non-static send methods
*/

void Server::SendInventory(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	playersao->m_inventory_not_sent = false;

	/*
		Serialize it
	*/

	std::ostringstream os;
	playersao->getInventory()->serialize(os);

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

void Server::SendPlayerHP(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);
	playersao->m_hp_not_sent = false;
	SendHP(m_con, peer_id, playersao->getHP());
}

void Server::SendMovePlayer(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(peer_id);
	assert(player);

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_MOVE_PLAYER);
	writeV3F1000(os, player->getPosition());
	writeF1000(os, player->getPitch());
	writeF1000(os, player->getYaw());
	
	{
		v3f pos = player->getPosition();
		f32 pitch = player->getPitch();
		f32 yaw = player->getYaw();
		verbosestream<<"Server: Sending TOCLIENT_MOVE_PLAYER"
				<<" pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"
				<<" pitch="<<pitch
				<<" yaw="<<yaw
				<<std::endl;
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendPlayerPrivileges(u16 peer_id)
{
	Player *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->peer_id == PEER_ID_INEXISTENT)
		return;

	std::set<std::string> privs;
	scriptapi_get_auth(m_lua, player->getName(), NULL, &privs);
	
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_PRIVILEGES);
	writeU16(os, privs.size());
	for(std::set<std::string>::const_iterator i = privs.begin();
			i != privs.end(); i++){
		os<<serializeString(*i);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendPlayerInventoryFormspec(u16 peer_id)
{
	Player *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->peer_id == PEER_ID_INEXISTENT)
		return;

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_INVENTORY_FORMSPEC);
	os<<serializeLongString(player->inventory_formspec);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

s32 Server::playSound(const SimpleSoundSpec &spec,
		const ServerSoundParams &params)
{
	// Find out initial position of sound
	bool pos_exists = false;
	v3f pos = params.getPos(m_env, &pos_exists);
	// If position is not found while it should be, cancel sound
	if(pos_exists != (params.type != ServerSoundParams::SSP_LOCAL))
		return -1;
	// Filter destination clients
	std::set<RemoteClient*> dst_clients;
	if(params.to_player != "")
	{
		Player *player = m_env->getPlayer(params.to_player.c_str());
		if(!player){
			infostream<<"Server::playSound: Player \""<<params.to_player
					<<"\" not found"<<std::endl;
			return -1;
		}
		if(player->peer_id == PEER_ID_INEXISTENT){
			infostream<<"Server::playSound: Player \""<<params.to_player
					<<"\" not connected"<<std::endl;
			return -1;
		}
		RemoteClient *client = getClient(player->peer_id);
		dst_clients.insert(client);
	}
	else
	{
		for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator(); i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			Player *player = m_env->getPlayer(client->peer_id);
			if(!player)
				continue;
			if(pos_exists){
				if(player->getPosition().getDistanceFrom(pos) >
						params.max_hear_distance)
					continue;
			}
			dst_clients.insert(client);
		}
	}
	if(dst_clients.size() == 0)
		return -1;
	// Create the sound
	s32 id = m_next_sound_id++;
	// The sound will exist as a reference in m_playing_sounds
	m_playing_sounds[id] = ServerPlayingSound();
	ServerPlayingSound &psound = m_playing_sounds[id];
	psound.params = params;
	for(std::set<RemoteClient*>::iterator i = dst_clients.begin();
			i != dst_clients.end(); i++)
		psound.clients.insert((*i)->peer_id);
	// Create packet
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_PLAY_SOUND);
	writeS32(os, id);
	os<<serializeString(spec.name);
	writeF1000(os, spec.gain * params.gain);
	writeU8(os, params.type);
	writeV3F1000(os, pos);
	writeU16(os, params.object);
	writeU8(os, params.loop);
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send
	for(std::set<RemoteClient*>::iterator i = dst_clients.begin();
			i != dst_clients.end(); i++){
		// Send as reliable
		m_con.Send((*i)->peer_id, 0, data, true);
	}
	return id;
}
void Server::stopSound(s32 handle)
{
	// Get sound reference
	std::map<s32, ServerPlayingSound>::iterator i =
			m_playing_sounds.find(handle);
	if(i == m_playing_sounds.end())
		return;
	ServerPlayingSound &psound = i->second;
	// Create packet
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_STOP_SOUND);
	writeS32(os, handle);
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send
	for(std::set<u16>::iterator i = psound.clients.begin();
			i != psound.clients.end(); i++){
		// Send as reliable
		m_con.Send(*i, 0, data, true);
	}
	// Remove sound reference
	m_playing_sounds.erase(i);
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
			Player *player = m_env->getPlayer(client->peer_id);
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
			Player *player = m_env->getPlayer(client->peer_id);
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
	infostream<<"Server: Sending block ("<<p.X<<","<<p.Y<<","<<p.Z<<"): ";
	if(completely_air)
		infostream<<"[completely air] ";
	infostream<<std::endl;
#endif

	/*
		Create a packet with the block in the right format
	*/
	
	std::ostringstream os(std::ios_base::binary);
	block->serialize(os, ver, false);
	std::string s = os.str();
	SharedBuffer<u8> blockdata((u8*)s.c_str(), s.size());

	u32 replysize = 8 + blockdata.getSize();
	SharedBuffer<u8> reply(replysize);
	writeU16(&reply[0], TOCLIENT_BLOCKDATA);
	writeS16(&reply[2], p.X);
	writeS16(&reply[4], p.Y);
	writeS16(&reply[6], p.Z);
	memcpy(&reply[8], *blockdata, blockdata.getSize());

	/*infostream<<"Server: Sending block ("<<p.X<<","<<p.Y<<","<<p.Z<<")"
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

	ScopeProfiler sp(g_profiler, "Server: sel and send blocks to clients");

	core::array<PrioritySortedBlockTransfer> queue;

	s32 total_sending = 0;
	
	{
		ScopeProfiler sp(g_profiler, "Server: selecting blocks for sending");

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			assert(client->peer_id == i.getNode()->getKey());

			// If definitions and textures have not been sent, don't
			// send MapBlocks either
			if(!client->definitions_sent)
				continue;

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
		if(total_sending >= g_settings->getS32
				("max_simultaneous_block_sends_server_total"))
			break;
		
		PrioritySortedBlockTransfer q = queue[i];

		MapBlock *block = NULL;
		try
		{
			block = m_env->getMap().getBlockNoCreate(q.pos);
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

void Server::fillMediaCache()
{
	DSTACK(__FUNCTION_NAME);

	infostream<<"Server: Calculating media file checksums"<<std::endl;
	
	// Collect all media file paths
	std::list<std::string> paths;
	for(core::list<ModSpec>::Iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		paths.push_back(mod.path + DIR_DELIM + "textures");
		paths.push_back(mod.path + DIR_DELIM + "sounds");
		paths.push_back(mod.path + DIR_DELIM + "media");
	}
	std::string path_all = "textures";
	paths.push_back(path_all + DIR_DELIM + "all");
	
	// Collect media file information from paths into cache
	for(std::list<std::string>::iterator i = paths.begin();
			i != paths.end(); i++)
	{
		std::string mediapath = *i;
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(mediapath);
		for(u32 j=0; j<dirlist.size(); j++){
			if(dirlist[j].dir) // Ignode dirs
				continue;
			std::string filename = dirlist[j].name;
			// If name contains illegal characters, ignore the file
			if(!string_allowed(filename, TEXTURENAME_ALLOWED_CHARS)){
				infostream<<"Server: ignoring illegal file name: \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			// If name is not in a supported format, ignore it
			const char *supported_ext[] = {
				".png", ".jpg", ".bmp", ".tga",
				".pcx", ".ppm", ".psd", ".wal", ".rgb",
				".ogg",
				NULL
			};
			if(removeStringEnd(filename, supported_ext) == ""){
				infostream<<"Server: ignoring unsupported file extension: \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			// Ok, attempt to load the file and add to cache
			std::string filepath = mediapath + DIR_DELIM + filename;
			// Read data
			std::ifstream fis(filepath.c_str(), std::ios_base::binary);
			if(fis.good() == false){
				errorstream<<"Server::fillMediaCache(): Could not open \""
						<<filename<<"\" for reading"<<std::endl;
				continue;
			}
			std::ostringstream tmp_os(std::ios_base::binary);
			bool bad = false;
			for(;;){
				char buf[1024];
				fis.read(buf, 1024);
				std::streamsize len = fis.gcount();
				tmp_os.write(buf, len);
				if(fis.eof())
					break;
				if(!fis.good()){
					bad = true;
					break;
				}
			}
			if(bad){
				errorstream<<"Server::fillMediaCache(): Failed to read \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			if(tmp_os.str().length() == 0){
				errorstream<<"Server::fillMediaCache(): Empty file \""
						<<filepath<<"\""<<std::endl;
				continue;
			}

			SHA1 sha1;
			sha1.addBytes(tmp_os.str().c_str(), tmp_os.str().length());

			unsigned char *digest = sha1.getDigest();
			std::string sha1_base64 = base64_encode(digest, 20);
			std::string sha1_hex = hex_encode((char*)digest, 20);
			free(digest);

			// Put in list
			this->m_media[filename] = MediaInfo(filepath, sha1_base64);
			verbosestream<<"Server: "<<sha1_hex<<" is "<<filename<<std::endl;
		}
	}
}

struct SendableMediaAnnouncement
{
	std::string name;
	std::string sha1_digest;

	SendableMediaAnnouncement(const std::string name_="",
			const std::string sha1_digest_=""):
		name(name_),
		sha1_digest(sha1_digest_)
	{}
};

void Server::sendMediaAnnouncement(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	verbosestream<<"Server: Announcing files to id("<<peer_id<<")"
			<<std::endl;

	core::list<SendableMediaAnnouncement> file_announcements;

	for(std::map<std::string, MediaInfo>::iterator i = m_media.begin();
			i != m_media.end(); i++){
		// Put in list
		file_announcements.push_back(
				SendableMediaAnnouncement(i->first, i->second.sha1_digest));
	}

	// Make packet
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 number of files
		for each texture {
			u16 length of name
			string name
			u16 length of sha1_digest
			string sha1_digest
		}
	*/
	
	writeU16(os, TOCLIENT_ANNOUNCE_MEDIA);
	writeU16(os, file_announcements.size());

	for(core::list<SendableMediaAnnouncement>::Iterator
			j = file_announcements.begin();
			j != file_announcements.end(); j++){
		os<<serializeString(j->name);
		os<<serializeString(j->sha1_digest);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());

	// Send as reliable
	m_con.Send(peer_id, 0, data, true);

}

struct SendableMedia
{
	std::string name;
	std::string path;
	std::string data;

	SendableMedia(const std::string &name_="", const std::string path_="",
			const std::string &data_=""):
		name(name_),
		path(path_),
		data(data_)
	{}
};

void Server::sendRequestedMedia(u16 peer_id,
		const core::list<MediaRequest> &tosend)
{
	DSTACK(__FUNCTION_NAME);

	verbosestream<<"Server::sendRequestedMedia(): "
			<<"Sending files to client"<<std::endl;

	/* Read files */

	// Put 5kB in one bunch (this is not accurate)
	u32 bytes_per_bunch = 5000;

	core::array< core::list<SendableMedia> > file_bunches;
	file_bunches.push_back(core::list<SendableMedia>());

	u32 file_size_bunch_total = 0;

	for(core::list<MediaRequest>::ConstIterator i = tosend.begin();
			i != tosend.end(); i++)
	{
		if(m_media.find(i->name) == m_media.end()){
			errorstream<<"Server::sendRequestedMedia(): Client asked for "
					<<"unknown file \""<<(i->name)<<"\""<<std::endl;
			continue;
		}

		//TODO get path + name
		std::string tpath = m_media[(*i).name].path;

		// Read data
		std::ifstream fis(tpath.c_str(), std::ios_base::binary);
		if(fis.good() == false){
			errorstream<<"Server::sendRequestedMedia(): Could not open \""
					<<tpath<<"\" for reading"<<std::endl;
			continue;
		}
		std::ostringstream tmp_os(std::ios_base::binary);
		bool bad = false;
		for(;;){
			char buf[1024];
			fis.read(buf, 1024);
			std::streamsize len = fis.gcount();
			tmp_os.write(buf, len);
			file_size_bunch_total += len;
			if(fis.eof())
				break;
			if(!fis.good()){
				bad = true;
				break;
			}
		}
		if(bad){
			errorstream<<"Server::sendRequestedMedia(): Failed to read \""
					<<(*i).name<<"\""<<std::endl;
			continue;
		}
		/*infostream<<"Server::sendRequestedMedia(): Loaded \""
				<<tname<<"\""<<std::endl;*/
		// Put in list
		file_bunches[file_bunches.size()-1].push_back(
				SendableMedia((*i).name, tpath, tmp_os.str()));

		// Start next bunch if got enough data
		if(file_size_bunch_total >= bytes_per_bunch){
			file_bunches.push_back(core::list<SendableMedia>());
			file_size_bunch_total = 0;
		}

	}

	/* Create and send packets */

	u32 num_bunches = file_bunches.size();
	for(u32 i=0; i<num_bunches; i++)
	{
		std::ostringstream os(std::ios_base::binary);

		/*
			u16 command
			u16 total number of texture bunches
			u16 index of this bunch
			u32 number of files in this bunch
			for each file {
				u16 length of name
				string name
				u32 length of data
				data
			}
		*/

		writeU16(os, TOCLIENT_MEDIA);
		writeU16(os, num_bunches);
		writeU16(os, i);
		writeU32(os, file_bunches[i].size());

		for(core::list<SendableMedia>::Iterator
				j = file_bunches[i].begin();
				j != file_bunches[i].end(); j++){
			os<<serializeString(j->name);
			os<<serializeLongString(j->data);
		}

		// Make data buffer
		std::string s = os.str();
		verbosestream<<"Server::sendRequestedMedia(): bunch "
				<<i<<"/"<<num_bunches
				<<" files="<<file_bunches[i].size()
				<<" size=" <<s.size()<<std::endl;
		SharedBuffer<u8> data((u8*)s.c_str(), s.size());
		// Send as reliable
		m_con.Send(peer_id, 0, data, true);
	}
}

void Server::sendDetachedInventory(const std::string &name, u16 peer_id)
{
	if(m_detached_inventories.count(name) == 0){
		errorstream<<__FUNCTION_NAME<<": \""<<name<<"\" not found"<<std::endl;
		return;
	}
	Inventory *inv = m_detached_inventories[name];

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_DETACHED_INVENTORY);
	os<<serializeString(name);
	inv->serialize(os);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::sendDetachedInventoryToAll(const std::string &name)
{
	DSTACK(__FUNCTION_NAME);

	for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++){
		RemoteClient *client = i.getNode()->getValue();
		sendDetachedInventory(name, client->peer_id);
	}
}

void Server::sendDetachedInventories(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	for(std::map<std::string, Inventory*>::iterator
			i = m_detached_inventories.begin();
			i != m_detached_inventories.end(); i++){
		const std::string &name = i->first;
		//Inventory *inv = i->second;
		sendDetachedInventory(name, peer_id);
	}
}

/*
	Something random
*/

void Server::DiePlayer(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	infostream<<"Server::DiePlayer(): Player "
			<<playersao->getPlayer()->getName()
			<<" dies"<<std::endl;

	playersao->setHP(0);

	// Trigger scripted stuff
	scriptapi_on_dieplayer(m_lua, playersao);

	SendPlayerHP(peer_id);
	SendDeathscreen(m_con, peer_id, false, v3f(0,0,0));
}

void Server::RespawnPlayer(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	infostream<<"Server::RespawnPlayer(): Player "
			<<playersao->getPlayer()->getName()
			<<" respawns"<<std::endl;

	playersao->setHP(PLAYER_MAX_HP);

	bool repositioned = scriptapi_on_respawnplayer(m_lua, playersao);
	if(!repositioned){
		v3f pos = findSpawnPos(m_env->getServerMap());
		playersao->setPos(pos);
	}
}

void Server::UpdateCrafting(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	Player* player = m_env->getPlayer(peer_id);
	assert(player);

	// Get a preview for crafting
	ItemStack preview;
	getCraftingResult(&player->inventory, preview, false, this);

	// Put the new preview in
	InventoryList *plist = player->inventory.getList("craftpreview");
	assert(plist);
	assert(plist->getSize() >= 1);
	plist->changeItem(0, preview);
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
	core::map<u16, RemoteClient*>::Iterator i;
	bool first;
	os<<L", clients={";
	for(i = m_clients.getIterator(), first = true;
		i.atEnd() == false; i++)
	{
		// Get client and check that it is valid
		RemoteClient *client = i.getNode()->getValue();
		assert(client->peer_id == i.getNode()->getKey());
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;
		// Get player
		Player *player = m_env->getPlayer(client->peer_id);
		// Get name of player
		std::wstring name = L"unknown";
		if(player != NULL)
			name = narrow_to_wide(player->getName());
		// Add name to information string
		if(!first)
			os<<L",";
		else
			first = false;
		os<<name;
	}
	os<<L"}";
	if(((ServerMap*)(&m_env->getMap()))->isSavingEnabled() == false)
		os<<std::endl<<L"# Server: "<<" WARNING: Map saving is disabled.";
	if(g_settings->get("motd") != "")
		os<<std::endl<<L"# Server: "<<narrow_to_wide(g_settings->get("motd"));
	return os.str();
}

std::set<std::string> Server::getPlayerEffectivePrivs(const std::string &name)
{
	std::set<std::string> privs;
	scriptapi_get_auth(m_lua, name, NULL, &privs);
	return privs;
}

bool Server::checkPriv(const std::string &name, const std::string &priv)
{
	std::set<std::string> privs = getPlayerEffectivePrivs(name);
	return (privs.count(priv) != 0);
}

void Server::reportPrivsModified(const std::string &name)
{
	if(name == ""){
		for(core::map<u16, RemoteClient*>::Iterator
				i = m_clients.getIterator();
				i.atEnd() == false; i++){
			RemoteClient *client = i.getNode()->getValue();
			Player *player = m_env->getPlayer(client->peer_id);
			reportPrivsModified(player->getName());
		}
	} else {
		Player *player = m_env->getPlayer(name.c_str());
		if(!player)
			return;
		SendPlayerPrivileges(player->peer_id);
		PlayerSAO *sao = player->getPlayerSAO();
		if(!sao)
			return;
		sao->updatePrivileges(
				getPlayerEffectivePrivs(name),
				isSingleplayer());
	}
}

void Server::reportInventoryFormspecModified(const std::string &name)
{
	Player *player = m_env->getPlayer(name.c_str());
	if(!player)
		return;
	SendPlayerInventoryFormspec(player->peer_id);
}

// Saves g_settings to configpath given at initialization
void Server::saveConfig()
{
	if(m_path_config != "")
		g_settings->updateConfigFile(m_path_config.c_str());
}

void Server::notifyPlayer(const char *name, const std::wstring msg)
{
	Player *player = m_env->getPlayer(name);
	if(!player)
		return;
	SendChatMessage(player->peer_id, std::wstring(L"Server: -!- ")+msg);
}

void Server::notifyPlayers(const std::wstring msg)
{
	BroadcastChatMessage(msg);
}

void Server::queueBlockEmerge(v3s16 blockpos, bool allow_generate)
{
	u8 flags = 0;
	if(!allow_generate)
		flags |= BLOCK_EMERGE_FLAG_FROMDISK;
	m_emerge_queue.addBlock(PEER_ID_INEXISTENT, blockpos, flags);
}

Inventory* Server::createDetachedInventory(const std::string &name)
{
	if(m_detached_inventories.count(name) > 0){
		infostream<<"Server clearing detached inventory \""<<name<<"\""<<std::endl;
		delete m_detached_inventories[name];
	} else {
		infostream<<"Server creating detached inventory \""<<name<<"\""<<std::endl;
	}
	Inventory *inv = new Inventory(m_itemdef);
	assert(inv);
	m_detached_inventories[name] = inv;
	sendDetachedInventoryToAll(name);
	return inv;
}

class BoolScopeSet
{
public:
	BoolScopeSet(bool *dst, bool val):
		m_dst(dst)
	{
		m_orig_state = *m_dst;
		*m_dst = val;
	}
	~BoolScopeSet()
	{
		*m_dst = m_orig_state;
	}
private:
	bool *m_dst;
	bool m_orig_state;
};

// actions: time-reversed list
// Return value: success/failure
bool Server::rollbackRevertActions(const std::list<RollbackAction> &actions,
		std::list<std::string> *log)
{
	infostream<<"Server::rollbackRevertActions(len="<<actions.size()<<")"<<std::endl;
	ServerMap *map = (ServerMap*)(&m_env->getMap());
	// Disable rollback report sink while reverting
	BoolScopeSet rollback_scope_disable(&m_rollback_sink_enabled, false);
	
	// Fail if no actions to handle
	if(actions.empty()){
		log->push_back("Nothing to do.");
		return false;
	}

	int num_tried = 0;
	int num_failed = 0;
	
	for(std::list<RollbackAction>::const_iterator
			i = actions.begin();
			i != actions.end(); i++)
	{
		const RollbackAction &action = *i;
		num_tried++;
		bool success = action.applyRevert(map, this, this);
		if(!success){
			num_failed++;
			std::ostringstream os;
			os<<"Revert of step ("<<num_tried<<") "<<action.toString()<<" failed";
			infostream<<"Map::rollbackRevertActions(): "<<os.str()<<std::endl;
			if(log)
				log->push_back(os.str());
		}else{
			std::ostringstream os;
			os<<"Succesfully reverted step ("<<num_tried<<") "<<action.toString();
			infostream<<"Map::rollbackRevertActions(): "<<os.str()<<std::endl;
			if(log)
				log->push_back(os.str());
		}
	}
	
	infostream<<"Map::rollbackRevertActions(): "<<num_failed<<"/"<<num_tried
			<<" failed"<<std::endl;

	// Call it done if less than half failed
	return num_failed <= num_tried/2;
}

// IGameDef interface
// Under envlock
IItemDefManager* Server::getItemDefManager()
{
	return m_itemdef;
}
INodeDefManager* Server::getNodeDefManager()
{
	return m_nodedef;
}
ICraftDefManager* Server::getCraftDefManager()
{
	return m_craftdef;
}
ITextureSource* Server::getTextureSource()
{
	return NULL;
}
u16 Server::allocateUnknownNodeId(const std::string &name)
{
	return m_nodedef->allocateDummy(name);
}
ISoundManager* Server::getSoundManager()
{
	return &dummySoundManager;
}
MtEventManager* Server::getEventManager()
{
	return m_event;
}
IRollbackReportSink* Server::getRollbackReportSink()
{
	if(!m_enable_rollback_recording)
		return NULL;
	if(!m_rollback_sink_enabled)
		return NULL;
	return m_rollback;
}

IWritableItemDefManager* Server::getWritableItemDefManager()
{
	return m_itemdef;
}
IWritableNodeDefManager* Server::getWritableNodeDefManager()
{
	return m_nodedef;
}
IWritableCraftDefManager* Server::getWritableCraftDefManager()
{
	return m_craftdef;
}

const ModSpec* Server::getModSpec(const std::string &modname)
{
	for(core::list<ModSpec>::Iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		if(mod.name == modname)
			return &mod;
	}
	return NULL;
}
void Server::getModNames(core::list<std::string> &modlist)
{
	for(core::list<ModSpec>::Iterator i = m_mods.begin(); i != m_mods.end(); i++)
	{
		modlist.push_back((*i).name);
	}
}
std::string Server::getBuiltinLuaPath()
{
	return porting::path_share + DIR_DELIM + "builtin";
}

v3f findSpawnPos(ServerMap &map)
{
	//return v3f(50,50,50)*BS;

	v3s16 nodepos;
	
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
		v2s16 nodepos2d = v2s16(-range + (myrand()%(range*2)),
				-range + (myrand()%(range*2)));
		//v2s16 sectorpos = getNodeSectorPos(nodepos2d);
		// Get ground height at point (fallbacks to heightmap function)
		s16 groundheight = map.findGroundLevel(nodepos2d);
		// Don't go underwater
		if(groundheight < WATER_LEVEL)
		{
			//infostream<<"-> Underwater"<<std::endl;
			continue;
		}
		// Don't go to high places
		if(groundheight > WATER_LEVEL + 4)
		{
			//infostream<<"-> Underwater"<<std::endl;
			continue;
		}
		
		nodepos = v3s16(nodepos2d.X, groundheight-2, nodepos2d.Y);
		bool is_good = false;
		s32 air_count = 0;
		for(s32 i=0; i<10; i++){
			v3s16 blockpos = getNodeBlockPos(nodepos);
			map.emergeBlock(blockpos, true);
			MapNode n = map.getNodeNoEx(nodepos);
			if(n.getContent() == CONTENT_AIR){
				air_count++;
				if(air_count >= 2){
					is_good = true;
					nodepos.Y -= 1;
					break;
				}
			}
			nodepos.Y++;
		}
		if(is_good){
			// Found a good place
			//infostream<<"Searched through "<<i<<" places."<<std::endl;
			break;
		}
	}
#endif
	
	return intToFloat(nodepos, BS);
}

PlayerSAO* Server::emergePlayer(const char *name, u16 peer_id)
{
	RemotePlayer *player = NULL;
	bool newplayer = false;

	/*
		Try to get an existing player
	*/
	player = static_cast<RemotePlayer*>(m_env->getPlayer(name));

	// If player is already connected, cancel
	if(player != NULL && player->peer_id != 0)
	{
		infostream<<"emergePlayer(): Player already connected"<<std::endl;
		return NULL;
	}

	/*
		If player with the wanted peer_id already exists, cancel.
	*/
	if(m_env->getPlayer(peer_id) != NULL)
	{
		infostream<<"emergePlayer(): Player with wrong name but same"
				" peer_id already exists"<<std::endl;
		return NULL;
	}

	/*
		Create a new player if it doesn't exist yet
	*/
	if(player == NULL)
	{
		newplayer = true;
		player = new RemotePlayer(this);
		player->updateName(name);

		/* Set player position */
		infostream<<"Server: Finding spawn place for player \""
				<<name<<"\""<<std::endl;
		v3f pos = findSpawnPos(m_env->getServerMap());
		player->setPosition(pos);

		/* Add player to environment */
		m_env->addPlayer(player);
	}

	/*
		Create a new player active object
	*/
	PlayerSAO *playersao = new PlayerSAO(m_env, player, peer_id,
			getPlayerEffectivePrivs(player->getName()),
			isSingleplayer());

	/* Add object to environment */
	m_env->addActiveObject(playersao);

	/* Run scripts */
	if(newplayer)
		scriptapi_on_newplayer(m_lua, playersao);

	scriptapi_on_joinplayer(m_lua, playersao);

	return playersao;
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
			ServerActiveObject* obj = m_env->getActiveObject(id);
			
			if(obj && obj->m_known_by_count > 0)
				obj->m_known_by_count--;
		}

		/*
			Clear references to playing sounds
		*/
		for(std::map<s32, ServerPlayingSound>::iterator
				i = m_playing_sounds.begin();
				i != m_playing_sounds.end();)
		{
			ServerPlayingSound &psound = i->second;
			psound.clients.erase(c.peer_id);
			if(psound.clients.size() == 0)
				m_playing_sounds.erase(i++);
			else
				i++;
		}

		Player *player = m_env->getPlayer(c.peer_id);

		// Collect information about leaving in chat
		std::wstring message;
		{
			if(player != NULL)
			{
				std::wstring name = narrow_to_wide(player->getName());
				message += L"*** ";
				message += name;
				message += L" left the game.";
				if(c.timeout)
					message += L" (timed out)";
			}
		}
		
		/* Run scripts and remove from environment */
		{
			if(player != NULL)
			{
				PlayerSAO *playersao = player->getPlayerSAO();
				assert(playersao);

				scriptapi_on_leaveplayer(m_lua, playersao);

				playersao->disconnected();
			}
		}

		/*
			Print out action
		*/
		{
			if(player != NULL)
			{
				std::ostringstream os(std::ios_base::binary);
				for(core::map<u16, RemoteClient*>::Iterator
					i = m_clients.getIterator();
					i.atEnd() == false; i++)
				{
					RemoteClient *client = i.getNode()->getValue();
					assert(client->peer_id == i.getNode()->getKey());
					if(client->serialization_version == SER_FMT_VER_INVALID)
						continue;
					// Get player
					Player *player = m_env->getPlayer(client->peer_id);
					if(!player)
						continue;
					// Get name of player
					os<<player->getName()<<" ";
				}

				actionstream<<player->getName()<<" "
						<<(c.timeout?"times out.":"leaves game.")
						<<" List of players: "
						<<os.str()<<std::endl;
			}
		}
		
		// Delete client
		delete m_clients[c.peer_id];
		m_clients.remove(c.peer_id);

		// Send player info to all remaining clients
		//SendPlayerInfos();
		
		// Send leave chat message to all remaining clients
		if(message.length() != 0)
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

		verbosestream<<"Server: Handling peer change: "
				<<"id="<<c.peer_id<<", timeout="<<c.timeout
				<<std::endl;

		handlePeerChange(c);
	}
}

void dedicated_server_loop(Server &server, bool &kill)
{
	DSTACK(__FUNCTION_NAME);
	
	verbosestream<<"dedicated_server_loop()"<<std::endl;

	IntervalLimiter m_profiler_interval;

	for(;;)
	{
		float steplen = g_settings->getFloat("dedicated_server_step");
		// This is kind of a hack but can be done like this
		// because server.step() is very light
		{
			ScopeProfiler sp(g_profiler, "dedicated server sleep");
			sleep_ms((int)(steplen*1000.0));
		}
		server.step(steplen);

		if(server.getShutdownRequested() || kill)
		{
			infostream<<"Dedicated server quitting"<<std::endl;
			break;
		}

		/*
			Profiler
		*/
		float profiler_print_interval =
				g_settings->getFloat("profiler_print_interval");
		if(profiler_print_interval != 0)
		{
			if(m_profiler_interval.step(steplen, profiler_print_interval))
			{
				infostream<<"Profiler:"<<std::endl;
				g_profiler->print(infostream);
				g_profiler->clear();
			}
		}
	}
}


