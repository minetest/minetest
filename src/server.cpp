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
#include <queue>
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
#include "content_nodemeta.h"
#include "mapblock.h"
#include "serverobject.h"
#include "settings.h"
#include "profiler.h"
#include "log.h"
#include "script.h"
#include "scriptapi.h"
#include "nodedef.h"
#include "tooldef.h"
#include "craftdef.h"
#include "craftitemdef.h"
#include "mapgen.h"
#include "content_abm.h"
#include "mods.h"

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
			
			{
				// Lock environment again to access the map
				JMutexAutoLock envlock(m_server->m_env_mutex);
				
				ScopeProfiler sp(g_profiler, "EmergeThread: after "
						"mapgen::make_block (envlock)", SPT_AVG);

				// Blit data back on map, update lighting, add mobs and
				// whatever this does
				map.finishBlockMake(&data, modified_blocks);

				// Get central block
				block = map.getBlockNoCreateNoEx(p);

				/*
					Do some post-generate stuff
				*/
				
				v3s16 minp = block->getPos()*MAP_BLOCKSIZE;
				v3s16 maxp = minp + v3s16(1,1,1)*(MAP_BLOCKSIZE-1);
				scriptapi_environment_on_generated(m_server->m_lua,
						minp, maxp);
				
				if(enable_mapgen_debug_info)
					infostream<<"EmergeThread: ended up with: "
							<<analyze_block(block)<<std::endl;

				/*
					Ignore map edit events, they will not need to be
					sent to anybody because the block hasn't been sent
					to anybody
				*/
				MapEditEventIgnorer ign(&m_server->m_ignore_map_edit_events);
				
				// Activate objects and stuff
				m_server->m_env->activateBlock(block, 0);
			}
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

	END_DEBUG_EXCEPTION_HANDLER(errorstream)

	log_deregister_thread();

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
	m_nearest_unsent_reset_timer += dtime;
	
	if(m_nothing_to_send_pause_timer >= 0)
	{
		return;
	}

	// Won't send anything if already sending
	if(m_blocks_sending.size() >= g_settings->getU16
			("max_simultaneous_block_sends_per_client"))
	{
		//infostream<<"Not sending any blocks, Queue full."<<std::endl;
		return;
	}

	//TimeTaker timer("RemoteClient::GetNextBlocks");
	
	Player *player = server->m_env->getPlayer(peer_id);

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

			float camera_fov = (72.0*PI/180) * 4./3.;
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
				Add inexistent block to emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || block_is_invalid)
			{
				//TODO: Get value from somewhere
				// Allow only one block in emerge queue
				//if(server->m_emerge_queue.peerItemCount(peer_id) < 1)
				// Allow two blocks in queue per client
				//if(server->m_emerge_queue.peerItemCount(peer_id) < 2)
				u32 max_emerge = 25;
				// Make it more responsive when needing to generate stuff
				if(surely_not_found_on_disk)
					max_emerge = 5;
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
		infostream<<"GetNextBlocks duration: "<<timer_result<<" (!=0)"<<std::endl;*/
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
	m_env(NULL),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this),
	m_authmanager(mapsavedir+DIR_DELIM+"auth.txt"),
	m_banmanager(mapsavedir+DIR_DELIM+"ipban.txt"),
	m_lua(NULL),
	m_toolmgr(createToolDefManager()),
	m_nodedef(createNodeDefManager()),
	m_craftdef(createCraftDefManager()),
	m_craftitemdef(createCraftItemDefManager()),
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

	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	// Path to builtin.lua
	std::string builtinpath = porting::path_data + DIR_DELIM + "builtin.lua";

	// Add default global mod search path
	m_modspaths.push_front(porting::path_data + DIR_DELIM + "mods");
	// Add world mod search path
	m_modspaths.push_front(mapsavedir + DIR_DELIM + "worldmods");
	// Add user mod search path
	m_modspaths.push_front(porting::path_userdata + DIR_DELIM + "usermods");
	
	// Print out mod search paths
	infostream<<"Mod search paths:"<<std::endl;
	for(core::list<std::string>::Iterator i = m_modspaths.begin();
			i != m_modspaths.end(); i++){
		std::string modspath = *i;
		infostream<<"    "<<modspath<<std::endl;
	}
	
	// Initialize scripting
	
	infostream<<"Server: Initializing scripting"<<std::endl;
	m_lua = script_init();
	assert(m_lua);
	// Export API
	scriptapi_export(m_lua, this);
	// Load and run builtin.lua
	infostream<<"Server: Loading builtin Lua stuff from \""<<builtinpath
			<<"\""<<std::endl;
	bool success = scriptapi_loadmod(m_lua, builtinpath, "__builtin");
	if(!success){
		errorstream<<"Server: Failed to load and run "
				<<builtinpath<<std::endl;
		throw ModError("Failed to load and run "+builtinpath);
	}
	// Load and run "mod" scripts
	m_mods = getMods(m_modspaths);
	for(core::list<ModSpec>::Iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		infostream<<"Server: Loading mod \""<<mod.name<<"\""<<std::endl;
		std::string scriptpath = mod.path + DIR_DELIM + "init.lua";
		bool success = scriptapi_loadmod(m_lua, scriptpath, mod.name);
		if(!success){
			errorstream<<"Server: Failed to load and run "
					<<scriptpath<<std::endl;
			throw ModError("Failed to load and run "+scriptpath);
		}
	}
	
	// Initialize Environment
	
	m_env = new ServerEnvironment(new ServerMap(mapsavedir, this), m_lua,
			this, this);

	// Give environment reference to scripting api
	scriptapi_add_environment(m_lua, m_env);
	
	// Register us to receive map edit events
	m_env->getMap().addEventReceiver(this);

	// If file exists, load environment metadata
	if(fs::PathExists(m_mapsavedir+DIR_DELIM+"env_meta.txt"))
	{
		infostream<<"Server: Loading environment metadata"<<std::endl;
		m_env->loadMeta(m_mapsavedir);
	}

	// Load players
	infostream<<"Server: Loading players"<<std::endl;
	m_env->deSerializePlayers(m_mapsavedir);

	/*
		Add some test ActiveBlockModifiers to environment
	*/
	add_legacy_abms(m_env, m_nodedef);
}

Server::~Server()
{
	infostream<<"Server::~Server()"<<std::endl;

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
		m_env->serializePlayers(m_mapsavedir);

		/*
			Save environment metadata
		*/
		infostream<<"Server: Saving environment metadata"<<std::endl;
		m_env->saveMeta(m_mapsavedir);
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

	// Delete Environment
	delete m_env;

	delete m_toolmgr;
	delete m_nodedef;
	delete m_craftdef;
	delete m_craftitemdef;
	
	// Deinitialize scripting
	infostream<<"Server: Deinitializing scripting"<<std::endl;
	script_deinit(m_lua);
}

void Server::start(unsigned short port)
{
	DSTACK(__FUNCTION_NAME);
	// Stop thread if already running
	m_thread.stop();
	
	// Initialize connection
	m_con.SetTimeoutMs(30);
	m_con.Serve(port);

	// Start thread
	m_thread.setRun(true);
	m_thread.Start();
	
	infostream<<"Server: Started on port "<<port<<std::endl;
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
		ScopeProfiler sp(g_profiler, "Server: sel and send blocks to clients");
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
		Update m_time_of_day and overall game time
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);

		m_time_counter += dtime;
		f32 speed = g_settings->getFloat("time_speed") * 24000./(24.*3600);
		u32 units = (u32)(m_time_counter*speed);
		m_time_counter -= (f32)units / speed;
		
		m_env->setTimeOfDay((m_env->getTimeOfDay() + units) % 24000);
		
		//infostream<<"Server: m_time_of_day = "<<m_time_of_day.get()<<std::endl;

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
				//Player *player = m_env->getPlayer(client->peer_id);
				
				SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
						m_env->getTimeOfDay());
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

		//float player_max_speed = BS * 4.0; // Normal speed
		float player_max_speed = BS * 20; // Fast speed
		float player_max_speed_up = BS * 20;
		
		player_max_speed *= 2.5; // Tolerance
		player_max_speed_up *= 2.5;

		for(core::map<u16, RemoteClient*>::Iterator
			i = m_clients.getIterator();
			i.atEnd() == false; i++)
		{
			RemoteClient *client = i.getNode()->getValue();
			ServerRemotePlayer *player =
					static_cast<ServerRemotePlayer*>
					(m_env->getPlayer(client->peer_id));
			if(player==NULL)
				continue;
			
			/*
				Check player movements

				NOTE: Actually the server should handle player physics like the
				client does and compare player's position to what is calculated
				on our side. This is required when eg. players fly due to an
				explosion.
			*/
			player->m_last_good_position_age += dtime;
			if(player->m_last_good_position_age >= 2.0){
				float age = player->m_last_good_position_age;
				v3f diff = (player->getPosition() - player->m_last_good_position);
				float d_vert = diff.Y;
				diff.Y = 0;
				float d_horiz = diff.getLength();
				/*infostream<<player->getName()<<"'s horizontal speed is "
						<<(d_horiz/age)<<std::endl;*/
				if(d_horiz <= age * player_max_speed &&
						(d_vert < 0 || d_vert < age * player_max_speed_up)){
					player->m_last_good_position = player->getPosition();
				} else {
					actionstream<<"Player "<<player->getName()
							<<" moved too fast; resetting position"
							<<std::endl;
					player->setPosition(player->m_last_good_position);
					SendMovePlayer(player);
				}
				player->m_last_good_position_age = 0;
			}

			/*
				Handle player HPs (die if hp=0)
			*/
			HandlePlayerHP(player, 0);

			/*
				Send player inventories and HPs if necessary
			*/
			if(player->m_inventory_not_sent){
				UpdateCrafting(player->peer_id);
				SendInventory(player->peer_id);
			}
			if(player->m_hp_not_sent){
				SendPlayerHP(player);
			}

			/*
				Add to environment if is not in respawn screen
			*/
			if(!player->m_is_in_environment && !player->m_respawn_active){
				player->m_removed = false;
				player->setId(0);
				m_env->addActiveObject(player);
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

			infostream<<"Server: Sent object remove/add: "
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

		if(got_any_events)
		{
			infostream<<"Server: MapEditEvents:"<<std::endl;
			prof.print(infostream);
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
		if(counter >= g_settings->getFloat("server_map_save_interval"))
		{
			counter = 0.0;

			ScopeProfiler sp(g_profiler, "Server: saving stuff");

			// Auth stuff
			if(m_authmanager.isModified())
				m_authmanager.save();

			//Bann stuff
			if(m_banmanager.isModified())
				m_banmanager.save();
			
			// Map
			JMutexAutoLock lock(m_env_mutex);

			// Save changed parts of map
			m_env->getMap().save(MOD_STATE_WRITE_NEEDED);

			// Save players
			m_env->serializePlayers(m_mapsavedir);
			
			// Save environment metadata
			m_env->saveMeta(m_mapsavedir);
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
	
	try{
		Address address = m_con.GetPeerAddress(peer_id);

		// drop player if is ip is banned
		if(m_banmanager.isIpBanned(address.serializeString())){
			SendAccessDenied(m_con, peer_id,
					L"Your ip is banned. Banned name was "
					+narrow_to_wide(m_banmanager.getBanName(
						address.serializeString())));
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

		infostream<<"Server: Got TOSERVER_INIT from "
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
			infostream<<"Server: Player has empty name"<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Empty name");
			return;
		}

		if(string_allowed(playername, PLAYERNAME_ALLOWED_CHARS)==false)
		{
			infostream<<"Server: Player has invalid name"<<std::endl;
			SendAccessDenied(m_con, peer_id,
					L"Name contains unallowed characters");
			return;
		}

		// Get password
		char password[PASSWORD_SIZE];
		if(datasize < 2+1+PLAYERNAME_SIZE+PASSWORD_SIZE)
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
		
		// Add player to auth manager
		if(m_authmanager.exists(playername) == false)
		{
			std::wstring default_password =
				narrow_to_wide(g_settings->get("default_password"));
			std::string translated_default_password =
				translatePassword(playername, default_password);

			// If default_password is empty, allow any initial password
			if (default_password.length() == 0)
				translated_default_password = password;

			infostream<<"Server: adding player "<<playername
					<<" to auth manager"<<std::endl;
			m_authmanager.add(playername);
			m_authmanager.setPassword(playername, translated_default_password);
			m_authmanager.setPrivs(playername,
					stringToPrivs(g_settings->get("default_privs")));
			m_authmanager.save();
		}

		std::string checkpwd = m_authmanager.getPassword(playername);

		/*infostream<<"Server: Client gave password '"<<password
				<<"', the correct one is '"<<checkpwd<<"'"<<std::endl;*/

		if(password != checkpwd)
		{
			infostream<<"Server: peer_id="<<peer_id
					<<": supplied invalid password for "
					<<playername<<std::endl;
			SendAccessDenied(m_con, peer_id, L"Invalid password");
			return;
		}
		
		// Enforce user limit.
		// Don't enforce for users that have some admin right
		if(m_clients.size() >= g_settings->getU16("max_users") &&
				(m_authmanager.getPrivs(playername)
					& (PRIV_SERVER|PRIV_BAN|PRIV_PRIVS|PRIV_PASSWORD)) == 0 &&
				playername != g_settings->get("name"))
		{
			SendAccessDenied(m_con, peer_id, L"Too many users.");
			return;
		}

		// Get player
		ServerRemotePlayer *player = emergePlayer(playername, peer_id);

		// If failed, cancel
		if(player == NULL)
		{
			infostream<<"Server: peer_id="<<peer_id
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
			writeV3S16(&reply[2+1], floatToInt(player->getPosition()+v3f(0,BS/2,0), BS));
			writeU64(&reply[2+1+6], m_env->getServerMap().getSeed());
			
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
		infostream<<"Server: Got TOSERVER_INIT2 from "
				<<peer_id<<std::endl;


		getClient(peer_id)->serialization_version
				= getClient(peer_id)->pending_serialization_version;

		/*
			Send some initialization data
		*/

		// Send tool definitions
		SendToolDef(m_con, peer_id, m_toolmgr);
		
		// Send node definitions
		SendNodeDef(m_con, peer_id, m_nodedef);
		
		// Send CraftItem definitions
		SendCraftItemDef(m_con, peer_id, m_craftitemdef);
		
		// Send textures
		SendTextures(peer_id);
		
		// Send player info to all players
		//SendPlayerInfos();

		// Send inventory to player
		UpdateCrafting(peer_id);
		SendInventory(peer_id);
		
		// Send player items to all players
		SendPlayerItems();

		Player *player = m_env->getPlayer(peer_id);

		// Send HP
		SendPlayerHP(player);
		
		// Send time of day
		{
			SharedBuffer<u8> data = makePacket_TOCLIENT_TIME_OF_DAY(
					m_env->getTimeOfDay());
			m_con.Send(peer_id, 0, data, true);
		}
		
		// Now the client should know about everything
		getClient(peer_id)->definitions_sent = true;
		
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
			message += L" joined game";
			BroadcastChatMessage(message);
		}
		
		// Warnings about protocol version can be issued here
		if(getClient(peer_id)->net_proto_version < PROTOCOL_VERSION)
		{
			SendChatMessage(peer_id, L"# Server: WARNING: YOUR CLIENT IS OLD AND MAY WORK PROPERLY WITH THIS SERVER");
		}

		/*
			Check HP, respawn if necessary
		*/
		HandlePlayerHP(player, 0);

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
	ServerRemotePlayer *srp = static_cast<ServerRemotePlayer*>(player);

	if(player == NULL){
		infostream<<"Server::ProcessData(): Cancelling: "
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
		if((getPlayerPrivs(player) & PRIV_INTERACT) == 0)
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

		NodeMetadata *meta = m_env->getMap().getNodeMetadata(p);
		if(!meta)
			return;

		meta->setText(text);
		
		actionstream<<player->getName()<<" writes \""<<text<<"\" to sign"
				<<" at "<<PP(p)<<std::endl;
				
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
		if(block)
		{
			block->raiseModified(MOD_STATE_WRITE_NEEDED,
					"sign node text");
		}

		setBlockNotSent(blockpos);
	}
	else if(command == TOSERVER_INVENTORY_ACTION)
	{
		/*// Ignore inventory changes if in creative mode
		if(g_settings->getBool("creative_mode") == true)
		{
			infostream<<"TOSERVER_INVENTORY_ACTION: ignoring in creative mode"
					<<std::endl;
			return;
		}*/
		// Strip command and create a stream
		std::string datastring((char*)&data[2], datasize-2);
		infostream<<"TOSERVER_INVENTORY_ACTION: data="<<datastring<<std::endl;
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
		// Create context
		InventoryContext c;
		c.current_player = player;

		/*
			Handle restrictions and special cases of the move action
		*/
		if(a->getType() == IACTION_MOVE
				&& g_settings->getBool("creative_mode") == false)
		{
			InventoryList *rlist = player->inventory.getList("craftresult");
			assert(rlist);
			InventoryList *clist = player->inventory.getList("craft");
			assert(clist);
			InventoryList *mlist = player->inventory.getList("main");
			assert(mlist);

			IMoveAction *ma = (IMoveAction*)a;

			/*
				Disable moving items into craftresult from elsewhere
			*/
			if(ma->to_inv == "current_player"
					&& ma->to_list == "craftresult"
					&& (ma->from_inv != "current_player"
					|| ma->from_list != "craftresult"))
			{
				infostream<<"Ignoring IMoveAction from "
						<<ma->from_inv<<":"<<ma->from_list
						<<" to "<<ma->to_inv<<":"<<ma->to_list
						<<" because dst is craftresult"
						<<" and src isn't craftresult"<<std::endl;
				delete a;
				return;
			}

			/*
				Handle crafting (source is craftresult, which is preview)
			*/
			if(ma->from_inv == "current_player"
					&& ma->from_list == "craftresult"
					&& player->craftresult_is_preview)
			{
				/*
					If the craftresult is placed on itself, crafting takes
					place and result is moved into main list
				*/
				if(ma->to_inv == "current_player"
						&& ma->to_list == "craftresult")
				{
					// Except if main list doesn't have free slots
					if(mlist->getFreeSlots() == 0){
						infostream<<"Cannot craft: Main list doesn't have"
								<<" free slots"<<std::endl;
						delete a;
						return;
					}
					
					player->craftresult_is_preview = false;
					clist->decrementMaterials(1);

					InventoryItem *item1 = rlist->changeItem(0, NULL);
					mlist->addItem(item1);

					srp->m_inventory_not_sent = true;

					delete a;
					return;
				}
				/*
					Disable action if there are no free slots in
					destination
					
					If the item is placed on an item that is not of the
					same kind, the existing item will be first moved to
					craftresult and immediately moved to the free slot.
				*/
				do{
					Inventory *inv_to = getInventory(&c, ma->to_inv);
					if(!inv_to) break;
					InventoryList *list_to = inv_to->getList(ma->to_list);
					if(!list_to) break;
					if(list_to->getFreeSlots() == 0){
						infostream<<"Cannot craft: Destination doesn't have"
								<<" free slots"<<std::endl;
						delete a;
						return;
					}
				}while(0); // Allow break

				/*
					Ok, craft normally.
				*/
				player->craftresult_is_preview = false;
				clist->decrementMaterials(1);
				
				/* Print out action */
				InventoryItem *item = rlist->getItem(0);
				std::string itemstring = "NULL";
				if(item)
					itemstring = item->getItemString();
				actionstream<<player->getName()<<" crafts "
						<<itemstring<<std::endl;

				// Do the action
				a->apply(&c, this, m_env);
				
				delete a;
				return;
			}

			/*
				Non-crafting move
			*/
			
			// Disallow moving items in elsewhere than player's inventory
			// if not allowed to build
			if((getPlayerPrivs(player) & PRIV_INTERACT) == 0
					&& (ma->from_inv != "current_player"
					|| ma->to_inv != "current_player"))
			{
				infostream<<"Cannot move outside of player's inventory: "
						<<"No build privilege"<<std::endl;
				delete a;
				return;
			}

			// If player is not an admin, check for ownership of src
			if(ma->from_inv != "current_player"
					&& (getPlayerPrivs(player) & PRIV_SERVER) == 0)
			{
				Strfnd fn(ma->from_inv);
				std::string id0 = fn.next(":");
				if(id0 == "nodemeta")
				{
					v3s16 p;
					p.X = stoi(fn.next(","));
					p.Y = stoi(fn.next(","));
					p.Z = stoi(fn.next(","));
					NodeMetadata *meta = m_env->getMap().getNodeMetadata(p);
					if(meta->getOwner() != "" &&
							meta->getOwner() != player->getName())
					{
						infostream<<"Cannot move item: "
								"not owner of metadata"
								<<std::endl;
						delete a;
						return;
					}
				}
			}
			// If player is not an admin, check for ownership of dst
			if(ma->to_inv != "current_player"
					&& (getPlayerPrivs(player) & PRIV_SERVER) == 0)
			{
				Strfnd fn(ma->to_inv);
				std::string id0 = fn.next(":");
				if(id0 == "nodemeta")
				{
					v3s16 p;
					p.X = stoi(fn.next(","));
					p.Y = stoi(fn.next(","));
					p.Z = stoi(fn.next(","));
					NodeMetadata *meta = m_env->getMap().getNodeMetadata(p);
					if(meta->getOwner() != "" &&
							meta->getOwner() != player->getName())
					{
						infostream<<"Cannot move item: "
								"not owner of metadata"
								<<std::endl;
						delete a;
						return;
					}
				}
			}
		}
		/*
			Handle restrictions and special cases of the drop action
		*/
		else if(a->getType() == IACTION_DROP)
		{
			IDropAction *da = (IDropAction*)a;
			// Disallow dropping items if not allowed to build
			if((getPlayerPrivs(player) & PRIV_INTERACT) == 0)
			{
				delete a;
				return;
			}
			// If player is not an admin, check for ownership
			else if (da->from_inv != "current_player"
					&& (getPlayerPrivs(player) & PRIV_SERVER) == 0)
			{
				Strfnd fn(da->from_inv);
				std::string id0 = fn.next(":");
				if(id0 == "nodemeta")
				{
					v3s16 p;
					p.X = stoi(fn.next(","));
					p.Y = stoi(fn.next(","));
					p.Z = stoi(fn.next(","));
					NodeMetadata *meta = m_env->getMap().getNodeMetadata(p);
					if(meta->getOwner() != "" &&
							meta->getOwner() != player->getName())
					{
						infostream<<"Cannot move item: "
								"not owner of metadata"
								<<std::endl;
						delete a;
						return;
					}
				}
			}
		}
		
		// Do the action
		a->apply(&c, this, m_env);
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
		
		// Local player gets all privileges regardless of
		// what's set on their account.
		u64 privs = getPlayerPrivs(player);

		// Parse commands
		if(message[0] == L'/')
		{
			size_t strip_size = 1;
			if (message[1] == L'#') // support old-style commans
				++strip_size;
			message = message.substr(strip_size);

			WStrfnd f1(message);
			f1.next(L" "); // Skip over /#whatever
			std::wstring paramstring = f1.next(L"");

			ServerCommandContext *ctx = new ServerCommandContext(
				str_split(message, L' '),
				paramstring,
				this,
				m_env,
				player,
				privs);

			std::wstring reply(processServerCommand(ctx));
			send_to_sender = ctx->flags & SEND_TO_SENDER;
			send_to_others = ctx->flags & SEND_TO_OTHERS;

			if (ctx->flags & SEND_NO_PREFIX)
				line += reply;
			else
				line += L"Server: " + reply;

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

		if(g_settings->getBool("enable_damage"))
		{
			actionstream<<player->getName()<<" damaged by "
					<<(int)damage<<" hp at "<<PP(player->getPosition()/BS)
					<<std::endl;
				
			HandlePlayerHP(player, damage);
		}
		else
		{
			SendPlayerHP(player);
		}
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

		infostream<<"Server: Client requests a password change from "
				<<"'"<<oldpwd<<"' to '"<<newpwd<<"'"<<std::endl;

		std::string playername = player->getName();

		if(m_authmanager.exists(playername) == false)
		{
			infostream<<"Server: playername not found in authmanager"<<std::endl;
			// Wrong old password supplied!!
			SendChatMessage(peer_id, L"playername not found in authmanager");
			return;
		}

		std::string checkpwd = m_authmanager.getPassword(playername);

		if(oldpwd != checkpwd)
		{
			infostream<<"Server: invalid old password"<<std::endl;
			// Wrong old password supplied!!
			SendChatMessage(peer_id, L"Invalid old password supplied. Password NOT changed.");
			return;
		}

		actionstream<<player->getName()<<" changes password"<<std::endl;

		m_authmanager.setPassword(playername, newpwd);
		
		infostream<<"Server: password change successful for "<<playername
				<<std::endl;
		SendChatMessage(peer_id, L"Password change successful");
	}
	else if(command == TOSERVER_PLAYERITEM)
	{
		if (datasize < 2+2)
			return;

		u16 item = readU16(&data[2]);
		player->wieldItem(item);
		SendWieldedItem(player);
	}
	else if(command == TOSERVER_RESPAWN)
	{
		if(player->hp != 0)
			return;
		
		srp->m_respawn_active = false;

		RespawnPlayer(player);
		
		actionstream<<player->getName()<<" respawns at "
				<<PP(player->getPosition()/BS)<<std::endl;

		// ActiveObject is added to environment in AsyncRunStep after
		// the previous addition has been succesfully removed
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

		infostream<<"TOSERVER_INTERACT: action="<<(int)action<<", item="<<item_i<<", pointed="<<pointed.dump()<<std::endl;

		v3f player_pos = srp->m_last_good_position;

		// Update wielded item
		srp->wieldItem(item_i);

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
				infostream<<"TOSERVER_INTERACT: "
					"pointed object is NULL"<<std::endl;
				return;
			}

		}

		/*
			Check that target is reasonably close
			(only when digging or placing things)
		*/
		if(action == 0 || action == 2 || action == 3)
		{
			v3f pointed_pos = player_pos;
			if(pointed.type == POINTEDTHING_NODE)
			{
				pointed_pos = intToFloat(p_under, BS);
			}
			else if(pointed.type == POINTEDTHING_OBJECT)
			{
				pointed_pos = pointed_object->getBasePosition();
			}

			float d = player_pos.getDistanceFrom(pointed_pos);
			float max_d = BS * 10; // Just some large enough value
			if(d > max_d){
				actionstream<<"Player "<<player->getName()
						<<" tried to access "<<pointed.dump()
						<<" from too far: "
						<<"d="<<d<<", max_d="<<max_d
						<<". ignoring."<<std::endl;
				// Re-send block to revert change on client-side
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos, BS));
				client->SetBlockNotSent(blockpos);
				// Do nothing else
				return;
			}
		}

		/*
			Make sure the player is allowed to do it
		*/
		bool build_priv = (getPlayerPrivs(player) & PRIV_INTERACT) != 0;
		if(!build_priv)
		{
			infostream<<"Ignoring interaction from player "<<player->getName()
					<<" because privileges are "<<getPlayerPrivs(player)
					<<std::endl;
			// NOTE: no return; here, fall through
		}

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
				bool cannot_punch_node = !build_priv;

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
					cannot_punch_node = true;
				}

				if(cannot_punch_node)
					return;

				/*
					Run script hook
				*/
				scriptapi_environment_on_punchnode(m_lua, p_under, n, srp);
			}
			else if(pointed.type == POINTEDTHING_OBJECT)
			{
				if(!build_priv)
					return;

				// Skip if object has been removed
				if(pointed_object->m_removed)
					return;

				actionstream<<player->getName()<<" punches object "
					<<pointed.object_id<<std::endl;

				// Do stuff
				pointed_object->punch(srp, srp->m_time_from_last_punch);
				srp->m_time_from_last_punch = 0;
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
			// Only complete digging of nodes
			if(pointed.type != POINTEDTHING_NODE)
				return;

			// Mandatory parameter; actually used for nothing
			core::map<v3s16, MapBlock*> modified_blocks;

			content_t material = CONTENT_IGNORE;
			u8 mineral = MINERAL_NONE;

			bool cannot_remove_node = !build_priv;
			
			MapNode n(CONTENT_IGNORE);
			try
			{
				n = m_env->getMap().getNode(p_under);
				// Get mineral
				mineral = n.getMineral(m_nodedef);
				// Get material at position
				material = n.getContent();
				// If not yet cancelled
				if(cannot_remove_node == false)
				{
					// If it's not diggable, do nothing
					if(m_nodedef->get(material).diggable == false)
					{
						infostream<<"Server: Not finishing digging: "
								<<"Node not diggable"
								<<std::endl;
						cannot_remove_node = true;
					}
				}
				// If not yet cancelled
				if(cannot_remove_node == false)
				{
					// Get node metadata
					NodeMetadata *meta = m_env->getMap().getNodeMetadata(p_under);
					if(meta && meta->nodeRemovalDisabled() == true)
					{
						infostream<<"Server: Not finishing digging: "
								<<"Node metadata disables removal"
								<<std::endl;
						cannot_remove_node = true;
					}
				}
			}
			catch(InvalidPositionException &e)
			{
				infostream<<"Server: Not finishing digging: Node not found."
						<<" Adding block to emerge queue."
						<<std::endl;
				m_emerge_queue.addBlock(peer_id,
						getNodeBlockPos(p_above), BLOCK_EMERGE_FLAG_FROMDISK);
				cannot_remove_node = true;
			}

			/*
				If node can't be removed, set block to be re-sent to
				client and quit.
			*/
			if(cannot_remove_node)
			{
				infostream<<"Server: Not finishing digging."<<std::endl;

				// Client probably has wrong data.
				// Set block not sent, so that client will get
				// a valid one.
				infostream<<"Client "<<peer_id<<" tried to dig "
						<<"node; but node cannot be removed."
						<<" setting MapBlock not sent."<<std::endl;
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(p_under);
				client->SetBlockNotSent(blockpos);
					
				return;
			}
			
			actionstream<<player->getName()<<" digs "<<PP(p_under)
					<<", gets material "<<(int)material<<", mineral "
					<<(int)mineral<<std::endl;
			
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

			if(g_settings->getBool("creative_mode") == false)
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
						ToolDiggingProperties tp =
								m_toolmgr->getDiggingProperties(toolname);
						DiggingProperties prop =
								getDiggingProperties(material, &tp, m_nodedef);

						if(prop.diggable == false)
						{
							infostream<<"Server: WARNING: Player digged"
									<<" with impossible material + tool"
									<<" combination"<<std::endl;
						}
						
						bool weared_out = titem->addWear(prop.wear);

						if(weared_out)
						{
							mlist->deleteItem(item_i);
						}

						srp->m_inventory_not_sent = true;
					}
				}

				/*
					Add dug item to inventory
				*/

				InventoryItem *item = NULL;

				if(mineral != MINERAL_NONE)
					item = getDiggedMineralItem(mineral, this);
				
				// If not mineral
				if(item == NULL)
				{
					const std::string &dug_s = m_nodedef->get(material).dug_item;
					if(dug_s != "")
					{
						std::istringstream is(dug_s, std::ios::binary);
						item = InventoryItem::deSerialize(is, this);
					}
				}
				
				if(item != NULL)
				{
					// Add a item to inventory
					player->inventory.addItem("main", item);
					srp->m_inventory_not_sent = true;
				}

				item = NULL;
				
				{
					const std::string &extra_dug_s = m_nodedef->get(material).extra_dug_item;
					s32 extra_rarity = m_nodedef->get(material).extra_dug_item_rarity;
					if(extra_dug_s != "" && extra_rarity != 0
					   && myrand() % extra_rarity == 0)
					{
						std::istringstream is(extra_dug_s, std::ios::binary);
						item = InventoryItem::deSerialize(is, this);
					}
				}
			
				if(item != NULL)
				{
					// Add a item to inventory
					player->inventory.addItem("main", item);
					srp->m_inventory_not_sent = true;
				}
			}

			/*
				Remove the node
				(this takes some time so it is done after the quick stuff)
			*/
			{
				MapEditEventIgnorer ign(&m_ignore_map_edit_events);

				m_env->getMap().removeNodeAndUpdate(p_under, modified_blocks);
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
				Run script hook
			*/
			scriptapi_environment_on_dignode(m_lua, p_under, n, srp);
		} // action == 2
		
		/*
			3: place block or right-click object
		*/
		else if(action == 3)
		{
			if(pointed.type == POINTEDTHING_NODE)
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
					bool cannot_place_node = !build_priv;

					try{
						// Don't add a node if this is not a free space
						MapNode n2 = m_env->getMap().getNode(p_above);
						if(m_nodedef->get(n2).buildable_to == false)
						{
							infostream<<"Client "<<peer_id<<" tried to place"
									<<" node in invalid position."<<std::endl;
							cannot_place_node = true;
						}
					}
					catch(InvalidPositionException &e)
					{
						infostream<<"Server: Ignoring ADDNODE: Node not found"
								<<" Adding block to emerge queue."
								<<std::endl;
						m_emerge_queue.addBlock(peer_id,
								getNodeBlockPos(p_above), BLOCK_EMERGE_FLAG_FROMDISK);
						cannot_place_node = true;
					}

					if(cannot_place_node)
					{
						// Client probably has wrong data.
						// Set block not sent, so that client will get
						// a valid one.
						RemoteClient *client = getClient(peer_id);
						v3s16 blockpos = getNodeBlockPos(p_above);
						client->SetBlockNotSent(blockpos);
						return;
					}

					// Reset build time counter
					getClient(peer_id)->m_time_from_building = 0.0;
					
					// Create node data
					MaterialItem *mitem = (MaterialItem*)item;
					MapNode n;
					n.setContent(mitem->getMaterial());

					actionstream<<player->getName()<<" places material "
							<<(int)mitem->getMaterial()
							<<" at "<<PP(p_under)<<std::endl;
				
					// Calculate direction for wall mounted stuff
					if(m_nodedef->get(n).wall_mounted)
						n.param2 = packDir(p_under - p_above);

					// Calculate the direction for furnaces and chests and stuff
					if(m_nodedef->get(n).param_type == CPT_FACEDIR_SIMPLE)
					{
						v3f playerpos = player->getPosition();
						v3f blockpos = intToFloat(p_above, BS) - playerpos;
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
					sendAddNode(p_above, n, 0, &far_players, 30);
					
					/*
						Handle inventory
					*/
					InventoryList *ilist = player->inventory.getList("main");
					if(g_settings->getBool("creative_mode") == false && ilist)
					{
						// Remove from inventory and send inventory
						if(mitem->getCount() <= 1)
							ilist->deleteItem(item_i);
						else
							mitem->remove(1);
						srp->m_inventory_not_sent = true;
					}
					
					/*
						Add node.

						This takes some time so it is done after the quick stuff
					*/
					core::map<v3s16, MapBlock*> modified_blocks;
					{
						MapEditEventIgnorer ign(&m_ignore_map_edit_events);

						std::string p_name = std::string(player->getName());
						m_env->getMap().addNodeAndUpdate(p_above, n, modified_blocks, p_name);
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
						Run script hook
					*/
					scriptapi_environment_on_placenode(m_lua, p_above, n, srp);

					/*
						Calculate special events
					*/
					
					/*if(n.d == LEGN(m_nodedef, "CONTENT_MESE"))
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
					if(!build_priv)
					{
						infostream<<"Not allowing player to place item: "
								"no build privileges"<<std::endl;
						return;
					}

					// Calculate a position for it
					v3f pos = player_pos;
					if(pointed.type == POINTEDTHING_NOTHING)
					{
						infostream<<"Not allowing player to place item: "
								"pointing to nothing"<<std::endl;
						return;
					}
					else if(pointed.type == POINTEDTHING_NODE)
					{
						pos = intToFloat(p_above, BS);
					}
					else if(pointed.type == POINTEDTHING_OBJECT)
					{
						pos = pointed_object->getBasePosition();

						// Randomize a bit
						pos.X += BS*0.2*(float)myrand_range(-1000,1000)/1000.0;
						pos.Z += BS*0.2*(float)myrand_range(-1000,1000)/1000.0;
					}

					//pos.Y -= BS*0.45;
					//pos.Y -= BS*0.25; // let it drop a bit

					/*
						Check that the block is loaded so that the item
						can properly be added to the static list too
					*/
					v3s16 blockpos = getNodeBlockPos(floatToInt(pos, BS));
					MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
					if(block==NULL)
					{
						infostream<<"Error while placing item: "
								"block not found"<<std::endl;
						return;
					}

					actionstream<<player->getName()<<" places "<<item->getName()
							<<" at "<<PP(pos)<<std::endl;

					/*
						Place the item
					*/
					bool remove = item->dropOrPlace(m_env, srp, pos, true, -1);
					if(remove && g_settings->getBool("creative_mode") == false)
					{
						InventoryList *ilist = player->inventory.getList("main");
						if(ilist){
							// Remove from inventory and send inventory
							ilist->deleteItem(item_i);
							srp->m_inventory_not_sent = true;
						}
					}
				}
			}
			else if(pointed.type == POINTEDTHING_OBJECT)
			{
				// Right click object

				if(!build_priv)
					return;

				// Skip if object has been removed
				if(pointed_object->m_removed)
					return;

				actionstream<<player->getName()<<" right-clicks object "
					<<pointed.object_id<<std::endl;

				// Do stuff
				pointed_object->rightClick(srp);
			}

		} // action == 3

		/*
			4: use
		*/
		else if(action == 4)
		{
			InventoryList *ilist = player->inventory.getList("main");
			if(ilist == NULL)
				return;

			// Get item
			InventoryItem *item = ilist->getItem(item_i);
			
			// If there is no item, it is not possible to add it anywhere
			if(item == NULL)
				return;

			// Requires build privs
			if(!build_priv)
			{
				infostream<<"Not allowing player to use item: "
						"no build privileges"<<std::endl;
				return;
			}

			actionstream<<player->getName()<<" uses "<<item->getName()
					<<", pointing at "<<pointed.dump()<<std::endl;

			bool remove = item->use(m_env, srp, pointed);
			
			if(remove && g_settings->getBool("creative_mode") == false)
			{
				InventoryList *ilist = player->inventory.getList("main");
				if(ilist){
					// Remove from inventory and send inventory
					ilist->deleteItem(item_i);
					srp->m_inventory_not_sent = true;
				}
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

		// Complete add_to_inventory_later
		srp->completeAddToInventoryLater(item_i);
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
		NodeMetadata *meta = m_env->getMap().getNodeMetadata(p);
		if(meta)
			return meta->getInventory();
		infostream<<"nodemeta at ("<<p.X<<","<<p.Y<<","<<p.Z<<"): "
				<<"no metadata found"<<std::endl;
		return NULL;
	}

	infostream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
	return NULL;
}
void Server::inventoryModified(InventoryContext *c, std::string id)
{
	if(id == "current_player")
	{
		assert(c->current_player);
		ServerRemotePlayer *srp =
				static_cast<ServerRemotePlayer*>(c->current_player);
		srp->m_inventory_not_sent = true;
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

		NodeMetadata *meta = m_env->getMap().getNodeMetadata(p);
		if(meta)
			meta->inventoryModified();
		
		MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
		if(block)
			block->raiseModified(MOD_STATE_WRITE_NEEDED);
		
		setBlockNotSent(blockpos);

		return;
	}

	infostream<<__FUNCTION_NAME<<": unknown id "<<id<<std::endl;
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
	infostream<<"Server::peerAdded(): peer->id="
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
	infostream<<"Server::deletingPeer(): peer->id="
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

void Server::SendToolDef(con::Connection &con, u16 peer_id,
		IToolDefManager *tooldef)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 length of the next item
		serialized ToolDefManager
	*/
	writeU16(os, TOCLIENT_TOOLDEF);
	std::ostringstream tmp_os(std::ios::binary);
	tooldef->serialize(tmp_os);
	os<<serializeLongString(tmp_os.str());

	// Make data buffer
	std::string s = os.str();
	infostream<<"Server::SendToolDef(): Sending tool definitions: size="
			<<s.size()<<std::endl;
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
		serialized NodeDefManager
	*/
	writeU16(os, TOCLIENT_NODEDEF);
	std::ostringstream tmp_os(std::ios::binary);
	nodedef->serialize(tmp_os);
	os<<serializeLongString(tmp_os.str());

	// Make data buffer
	std::string s = os.str();
	infostream<<"Server::SendNodeDef(): Sending node definitions: size="
			<<s.size()<<std::endl;
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendCraftItemDef(con::Connection &con, u16 peer_id,
		ICraftItemDefManager *craftitemdef)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 length of the next item
		serialized CraftItemDefManager
	*/
	writeU16(os, TOCLIENT_CRAFTITEMDEF);
	std::ostringstream tmp_os(std::ios::binary);
	craftitemdef->serialize(tmp_os);
	os<<serializeLongString(tmp_os.str());

	// Make data buffer
	std::string s = os.str();
	infostream<<"Server::SendCraftItemDef(): Sending craft item definitions: size="
			<<s.size()<<std::endl;
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
	
	ServerRemotePlayer* player =
			static_cast<ServerRemotePlayer*>(m_env->getPlayer(peer_id));
	assert(player);

	player->m_inventory_not_sent = false;

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

std::string getWieldedItemString(const Player *player)
{
	const InventoryItem *item = player->getWieldItem();
	if (item == NULL)
		return std::string("");
	std::ostringstream os(std::ios_base::binary);
	item->serialize(os);
	return os.str();
}

void Server::SendWieldedItem(const Player* player)
{
	DSTACK(__FUNCTION_NAME);

	assert(player);

	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_PLAYERITEM);
	writeU16(os, 1);
	writeU16(os, player->peer_id);
	os<<serializeString(getWieldedItemString(player));

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());

	m_con.SendToAll(0, data, true);
}

void Server::SendPlayerItems()
{
	DSTACK(__FUNCTION_NAME);

	std::ostringstream os(std::ios_base::binary);
	core::list<Player *> players = m_env->getPlayers(true);

	writeU16(os, TOCLIENT_PLAYERITEM);
	writeU16(os, players.size());
	core::list<Player *>::Iterator i;
	for(i = players.begin(); i != players.end(); ++i)
	{
		Player *p = *i;
		writeU16(os, p->peer_id);
		os<<serializeString(getWieldedItemString(p));
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());

	m_con.SendToAll(0, data, true);
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
	static_cast<ServerRemotePlayer*>(player)->m_hp_not_sent = false;
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
		infostream<<"Server sending TOCLIENT_MOVE_PLAYER"
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

	//TimeTaker timer("Server::SendBlocks");

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

struct SendableTexture
{
	std::string name;
	std::string path;
	std::string data;

	SendableTexture(const std::string &name_="", const std::string path_="",
			const std::string &data_=""):
		name(name_),
		path(path_),
		data(data_)
	{}
};

void Server::SendTextures(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	infostream<<"Server::SendTextures(): Sending textures to client"<<std::endl;
	
	/* Read textures */
	
	// Put 5kB in one bunch (this is not accurate)
	u32 bytes_per_bunch = 5000;
	
	core::array< core::list<SendableTexture> > texture_bunches;
	texture_bunches.push_back(core::list<SendableTexture>());
	
	u32 texture_size_bunch_total = 0;
	for(core::list<ModSpec>::Iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		std::string texturepath = mod.path + DIR_DELIM + "textures";
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(texturepath);
		for(u32 j=0; j<dirlist.size(); j++){
			if(dirlist[j].dir) // Ignode dirs
				continue;
			std::string tname = dirlist[j].name;
			std::string tpath = texturepath + DIR_DELIM + tname;
			// Read data
			std::ifstream fis(tpath.c_str(), std::ios_base::binary);
			if(fis.good() == false){
				errorstream<<"Server::SendTextures(): Could not open \""
						<<tname<<"\" for reading"<<std::endl;
				continue;
			}
			std::ostringstream tmp_os(std::ios_base::binary);
			bool bad = false;
			for(;;){
				char buf[1024];
				fis.read(buf, 1024);
				std::streamsize len = fis.gcount();
				tmp_os.write(buf, len);
				texture_size_bunch_total += len;
				if(fis.eof())
					break;
				if(!fis.good()){
					bad = true;
					break;
				}
			}
			if(bad){
				errorstream<<"Server::SendTextures(): Failed to read \""
						<<tname<<"\""<<std::endl;
				continue;
			}
			/*infostream<<"Server::SendTextures(): Loaded \""
					<<tname<<"\""<<std::endl;*/
			// Put in list
			texture_bunches[texture_bunches.size()-1].push_back(
					SendableTexture(tname, tpath, tmp_os.str()));
			
			// Start next bunch if got enough data
			if(texture_size_bunch_total >= bytes_per_bunch){
				texture_bunches.push_back(core::list<SendableTexture>());
				texture_size_bunch_total = 0;
			}
		}
	}

	/* Create and send packets */
	
	u32 num_bunches = texture_bunches.size();
	for(u32 i=0; i<num_bunches; i++)
	{
		/*
			u16 command
			u16 total number of texture bunches
			u16 index of this bunch
			u32 number of textures in this bunch
			for each texture {
				u16 length of name
				string name
				u32 length of data
				data
			}
		*/
		std::ostringstream os(std::ios_base::binary);

		writeU16(os, TOCLIENT_TEXTURES);
		writeU16(os, num_bunches);
		writeU16(os, i);
		writeU32(os, texture_bunches[i].size());
		
		for(core::list<SendableTexture>::Iterator
				j = texture_bunches[i].begin();
				j != texture_bunches[i].end(); j++){
			os<<serializeString(j->name);
			os<<serializeLongString(j->data);
		}
		
		// Make data buffer
		std::string s = os.str();
		infostream<<"Server::SendTextures(): bunch "<<i<<"/"<<num_bunches
				<<" textures="<<texture_bunches[i].size()
				<<" size=" <<s.size()<<std::endl;
		SharedBuffer<u8> data((u8*)s.c_str(), s.size());
		// Send as reliable
		m_con.Send(peer_id, 0, data, true);
	}
}

/*
	Something random
*/

void Server::HandlePlayerHP(Player *player, s16 damage)
{
	ServerRemotePlayer *srp = static_cast<ServerRemotePlayer*>(player);

	if(srp->m_respawn_active)
		return;
	
	if(player->hp > damage)
	{
		if(damage != 0){
			player->hp -= damage;
			SendPlayerHP(player);
		}
		return;
	}

	infostream<<"Server::HandlePlayerHP(): Player "
			<<player->getName()<<" dies"<<std::endl;
	
	player->hp = 0;
	
	//TODO: Throw items around
	
	// Handle players that are not connected
	if(player->peer_id == PEER_ID_INEXISTENT){
		RespawnPlayer(player);
		return;
	}

	SendPlayerHP(player);
	
	RemoteClient *client = getClient(player->peer_id);
	if(client->net_proto_version >= 3)
	{
		SendDeathscreen(m_con, player->peer_id, false, v3f(0,0,0));
		srp->m_removed = true;
		srp->m_respawn_active = true;
	}
	else
	{
		RespawnPlayer(player);
	}
}

void Server::RespawnPlayer(Player *player)
{
	player->hp = 20;
	ServerRemotePlayer *srp = static_cast<ServerRemotePlayer*>(player);
	bool repositioned = scriptapi_on_respawnplayer(m_lua, srp);
	if(!repositioned){
		v3f pos = findSpawnPos(m_env->getServerMap());
		player->setPosition(pos);
		srp->m_last_good_position = pos;
		srp->m_last_good_position_age = 0;
	}
	SendMovePlayer(player);
	SendPlayerHP(player);
}

void Server::UpdateCrafting(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	
	Player* player = m_env->getPlayer(peer_id);
	assert(player);
	ServerRemotePlayer *srp = static_cast<ServerRemotePlayer*>(player);

	// No crafting in creative mode
	if(g_settings->getBool("creative_mode"))
		return;
	
	// Get the InventoryLists of the player in which we will operate
	InventoryList *clist = player->inventory.getList("craft");
	assert(clist);
	InventoryList *rlist = player->inventory.getList("craftresult");
	assert(rlist);
	InventoryList *mlist = player->inventory.getList("main");
	assert(mlist);

	// If the result list is not a preview and is not empty, try to
	// throw the item into main list
	if(!player->craftresult_is_preview && rlist->getUsedSlots() != 0)
	{
		// Grab item out of craftresult
		InventoryItem *item = rlist->changeItem(0, NULL);
		// Try to put in main
		InventoryItem *leftover = mlist->addItem(item);
		// If there are leftovers, put them back to craftresult and
		// delete leftovers
		delete rlist->addItem(leftover);
		// Inventory was modified
		srp->m_inventory_not_sent = true;
	}
	
	// If result list is empty, we will make it preview what would be
	// crafted
	if(rlist->getUsedSlots() == 0)
		player->craftresult_is_preview = true;
	
	// If it is a preview, clear the possible old preview in it
	if(player->craftresult_is_preview)
		rlist->clearItems();

	// If it is a preview, find out what is the crafting result
	// and put it in
	if(player->craftresult_is_preview)
	{
		// Mangle crafting grid to an another format
		std::vector<InventoryItem*> items;
		for(u16 i=0; i<9; i++){
			if(clist->getItem(i) == NULL)
				items.push_back(NULL);
			else
				items.push_back(clist->getItem(i)->clone());
		}
		CraftPointerInput cpi(3, items);

		// Find out what is crafted and add it to result item slot
		InventoryItem *result = m_craftdef->getCraftResult(cpi, this);
		if(result)
			rlist->addItem(result);
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
		Player *player = m_env->getPlayer(client->peer_id);
		// Get name of player
		std::wstring name = L"unknown";
		if(player != NULL)
			name = narrow_to_wide(player->getName());
		// Add name to information string
		os<<name<<L",";
	}
	os<<L"}";
	if(((ServerMap*)(&m_env->getMap()))->isSavingEnabled() == false)
		os<<std::endl<<L"# Server: "<<" WARNING: Map saving is disabled.";
	if(g_settings->get("motd") != "")
		os<<std::endl<<L"# Server: "<<narrow_to_wide(g_settings->get("motd"));
	return os.str();
}

void Server::setPlayerPassword(const std::string &name, const std::wstring &password)
{
	// Add player to auth manager
	if(m_authmanager.exists(name) == false)
	{
		infostream<<"Server: adding player "<<name
				<<" to auth manager"<<std::endl;
		m_authmanager.add(name);
		m_authmanager.setPrivs(name,
			stringToPrivs(g_settings->get("default_privs")));
	}
	// Change password and save
	m_authmanager.setPassword(name, translatePassword(name, password));
	m_authmanager.save();
}

// Saves g_settings to configpath given at initialization
void Server::saveConfig()
{
	if(m_configpath != "")
		g_settings->updateConfigFile(m_configpath.c_str());
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

// IGameDef interface
// Under envlock
IToolDefManager* Server::getToolDefManager()
{
	return m_toolmgr;
}
INodeDefManager* Server::getNodeDefManager()
{
	return m_nodedef;
}
ICraftDefManager* Server::getCraftDefManager()
{
	return m_craftdef;
}
ICraftItemDefManager* Server::getCraftItemDefManager()
{
	return m_craftitemdef;
}
ITextureSource* Server::getTextureSource()
{
	return NULL;
}
u16 Server::allocateUnknownNodeId(const std::string &name)
{
	return m_nodedef->allocateDummy(name);
}

IWritableToolDefManager* Server::getWritableToolDefManager()
{
	return m_toolmgr;
}
IWritableNodeDefManager* Server::getWritableNodeDefManager()
{
	return m_nodedef;
}
IWritableCraftDefManager* Server::getWritableCraftDefManager()
{
	return m_craftdef;
}
IWritableCraftItemDefManager* Server::getWritableCraftItemDefManager()
{
	return m_craftitemdef;
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

ServerRemotePlayer *Server::emergePlayer(const char *name, u16 peer_id)
{
	/*
		Try to get an existing player
	*/
	ServerRemotePlayer *player =
			static_cast<ServerRemotePlayer*>(m_env->getPlayer(name));
	if(player != NULL)
	{
		// If player is already connected, cancel
		if(player->peer_id != 0)
		{
			infostream<<"emergePlayer(): Player already connected"<<std::endl;
			return NULL;
		}

		// Got one.
		player->peer_id = peer_id;
		
		// Reset inventory to creative if in creative mode
		if(g_settings->getBool("creative_mode"))
		{
			// Warning: double code below
			// Backup actual inventory
			player->inventory_backup = new Inventory();
			*(player->inventory_backup) = player->inventory;
			// Set creative inventory
			player->resetInventory();
			scriptapi_get_creative_inventory(m_lua, player);
		}

		return player;
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
		Create a new player
	*/
	{
		/* Set player position */
		
		infostream<<"Server: Finding spawn place for player \""
				<<name<<"\""<<std::endl;

		v3f pos = findSpawnPos(m_env->getServerMap());

		player = new ServerRemotePlayer(m_env, pos, peer_id, name);

		/* Add player to environment */
		m_env->addPlayer(player);

		/* Run scripts */
		ServerRemotePlayer *srp = static_cast<ServerRemotePlayer*>(player);
		scriptapi_on_newplayer(m_lua, srp);

		/* Add stuff to inventory */
		if(g_settings->getBool("creative_mode"))
		{
			// Warning: double code above
			// Backup actual inventory
			player->inventory_backup = new Inventory();
			*(player->inventory_backup) = player->inventory;
			// Set creative inventory
			player->resetInventory();
			scriptapi_get_creative_inventory(m_lua, player);
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
			ServerActiveObject* obj = m_env->getActiveObject(id);
			
			if(obj && obj->m_known_by_count > 0)
				obj->m_known_by_count--;
		}

		ServerRemotePlayer* player =
				static_cast<ServerRemotePlayer*>(m_env->getPlayer(c.peer_id));

		// Collect information about leaving in chat
		std::wstring message;
		{
			if(player != NULL)
			{
				std::wstring name = narrow_to_wide(player->getName());
				message += L"*** ";
				message += name;
				message += L" left game";
				if(c.timeout)
					message += L" (timed out)";
			}
		}
		
		// Remove from environment
		if(player != NULL)
			player->m_removed = true;
		
		// Set player client disconnected
		if(player != NULL)
			player->peer_id = 0;
	
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

		infostream<<"Server: Handling peer change: "
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
	if(g_settings->get("name") == playername)
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
	
	infostream<<DTIME<<std::endl;
	infostream<<"========================"<<std::endl;
	infostream<<"Running dedicated server"<<std::endl;
	infostream<<"========================"<<std::endl;
	infostream<<std::endl;

	IntervalLimiter m_profiler_interval;

	for(;;)
	{
		// This is kind of a hack but can be done like this
		// because server.step() is very light
		{
			ScopeProfiler sp(g_profiler, "dedicated server sleep");
			sleep_ms(30);
		}
		server.step(0.030);

		if(server.getShutdownRequested() || kill)
		{
			infostream<<DTIME<<" dedicated_server_loop(): Quitting."<<std::endl;
			break;
		}

		/*
			Profiler
		*/
		float profiler_print_interval =
				g_settings->getFloat("profiler_print_interval");
		if(profiler_print_interval != 0)
		{
			if(m_profiler_interval.step(0.030, profiler_print_interval))
			{
				infostream<<"Profiler:"<<std::endl;
				g_profiler->print(infostream);
				g_profiler->clear();
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
				infostream<<DTIME<<"Player info:"<<std::endl;
				for(i=list.begin(); i!=list.end(); i++)
				{
					i->PrintLine(&infostream);
				}
			}
			sum_old = sum;
		}
	}
}


