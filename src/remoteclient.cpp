/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "player.h"
#include "main.h"
#include "settings.h"
#include "log.h"
#include "emerge.h"

void RemoteClient::GetNextBlocks(Server *server, float dtime,
		std::vector<PrioritySortedBlockTransfer> &dest)
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
		std::list<v3s16> list;
		getFacePositions(list, d);

		std::list<v3s16>::iterator li;
		for(li=list.begin(); li!=list.end(); ++li)
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
			if(m_blocks_sending.find(p) != m_blocks_sending.end())
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

			/*
				Don't send already sent blocks
			*/
			{
				if(m_blocks_sent.find(p) != m_blocks_sent.end())
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

				if(block->isGenerated() == false)
					block_is_invalid = true;

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
			/*	//TODO: Get value from somewhere
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
					goto queue_full_break;
				}
			*/

				if (server->m_emerge->enqueueBlockEmerge(peer_id, p, generate)) {
					if (nearest_emerged_d == -1)
						nearest_emerged_d = d;
				} else {
					if (nearest_emergefull_d == -1)
						nearest_emergefull_d = d;
					goto queue_full_break;
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
	if(m_blocks_sending.find(p) != m_blocks_sending.end())
		m_blocks_sending.erase(p);
	else
	{
		/*infostream<<"RemoteClient::GotBlock(): Didn't find in"
				" m_blocks_sending"<<std::endl;*/
		m_excess_gotblocks++;
	}
	m_blocks_sent.insert(p);
}

void RemoteClient::SentBlock(v3s16 p)
{
	if(m_blocks_sending.find(p) == m_blocks_sending.end())
		m_blocks_sending[p] = 0.0;
	else
		infostream<<"RemoteClient::SentBlock(): Sent block"
				" already in m_blocks_sending"<<std::endl;
}

void RemoteClient::SetBlockNotSent(v3s16 p)
{
	m_nearest_unsent_d = 0;

	if(m_blocks_sending.find(p) != m_blocks_sending.end())
		m_blocks_sending.erase(p);
	if(m_blocks_sent.find(p) != m_blocks_sent.end())
		m_blocks_sent.erase(p);
}

void RemoteClient::SetBlocksNotSent(std::map<v3s16, MapBlock*> &blocks)
{
	m_nearest_unsent_d = 0;

	for(std::map<v3s16, MapBlock*>::iterator
			i = blocks.begin();
			i != blocks.end(); ++i)
	{
		v3s16 p = i->first;

		if(m_blocks_sending.find(p) != m_blocks_sending.end())
			m_blocks_sending.erase(p);
		if(m_blocks_sent.find(p) != m_blocks_sent.end())
			m_blocks_sent.erase(p);
	}
}
