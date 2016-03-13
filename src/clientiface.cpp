/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

// These two lines should surely get us INT32_MAX
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <sstream>

#include "clientiface.h"
#include "util/numeric.h"
#include "util/mathconstants.h"
#include "player.h"
#include "settings.h"
#include "mapblock.h"
#include "network/connection.h"
#include "environment.h"
#include "map.h"
#include "emerge.h"
#include "serverobject.h"              // TODO this is used for cleanup of only
#include "log.h"
#include "far_map_server.h"
#include "network/wms_priority.h"
#include "util/srp.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

struct WMSSPriority: WMSPriority
{
	WMSSPriority(v3s16 player_p, float far_weight):
		WMSPriority(player_p, far_weight)
	{}

	// Lower is more important
	float getPriority(const WMSSuggestion &wmss) {
		return WMSPriority::getPriority(wmss.wms);
	}

	// Sorts WMSSuggestions in descending order of priority
	bool operator() (const WMSSuggestion& wms1, const WMSSuggestion& wms2) {
		return (getPriority(wms1) < getPriority(wms2));
	}
};

/*
	AutosendAlgorithm
*/

u16 figure_out_max_simultaneous_block_sends(u16 base_setting,
		float time_from_building, float time_from_building_limit_setting,
		s16 block_distance_in_blocks)
{
	// If block is very close, always send the configured amount
	if (block_distance_in_blocks <= BLOCK_SEND_DISABLE_LIMITS_MAX_D)
		return base_setting;

	// If the time from last addNode/removeNode is small, don't send as much
	// stuff in order to reduce lag
	if (time_from_building < time_from_building_limit_setting)
		return LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS;

	// Send the configured amount if nothing special is happening
	return base_setting;
}

struct AutosendCycle
{
	bool disabled;
	AutosendAlgorithm *alg;
	RemoteClient *client;
	ServerEnvironment *env;

	v3f camera_p;
	v3f camera_dir;
	v3s16 focus_point;

	u16 max_simul_sends_setting;
	float time_from_building_limit_s;
	s16 max_mapblock_send_distance_setting;
	s16 max_mapblock_generate_distance;
	bool far_map_allow_generate;

	struct Search {
		// All of these are reset by start()
		s16 max_send_distance;
		s16 fov_limit_activation_distance;
		s16 d_start;
		s16 d_max;
		s16 d;
		size_t i;
		s32 nearest_emergequeued_d;
		s32 nearest_emergefull_d;
		s32 nearest_sendqueued_d;

		// All of these are persistent
		s16 nearest_unsent_d;
		bool fov_limit_enabled; // Automatically turned off to transfer the rest
		float nothing_to_send_timer;
		float nothing_to_send_pause_timer; // CPU usage optimization

		Search():
			nearest_unsent_d(0),
			fov_limit_enabled(true),
			nothing_to_send_timer(0), // Counts up from 0
			nothing_to_send_pause_timer(0) // Pause if >=0; decrements
		{}

		void runTimers(float dtime);

		void start(s16 max_send_distance_, s16 fov_limit_activation_distance_);

		void finish(bool enable_fov_toggle);
	};
	
	Search mapblock;
	Search farblock;

	AutosendCycle():
		disabled(true)
	{}

	void start(AutosendAlgorithm *alg_,
			RemoteClient *client_, ServerEnvironment *env_);

	WMSSuggestion getNextBlock(EmergeManager *emerge, ServerFarMap *far_map);

	void finish();

private:
	WMSSuggestion suggestNextMapBlock(EmergeManager *emerge);
	WMSSuggestion suggestNextFarBlock(EmergeManager *emerge, ServerFarMap *far_map);
};

void AutosendCycle::start(AutosendAlgorithm *alg_,
		RemoteClient *client_, ServerEnvironment *env_)
{
	//dstream<<"AutosendCycle::start()"<<std::endl;

	alg = alg_;
	client = client_;
	env = env_;

	disabled = false;

	if (alg->m_radius_map == 0 && alg->m_radius_far == 0) {
		//dstream<<"radius=0"<<std::endl;
		disabled = true;
		return;
	}

	// Disable if MapBlock and FarBlock pauses are active
	if (mapblock.nothing_to_send_pause_timer >= 0 &&
			farblock.nothing_to_send_pause_timer >= 0) {
		//dstream<<"nothing to send pause (map+far)"<<std::endl;
		disabled = true;
		return;
	}

	Player *player = env->getPlayer(client->peer_id);
	if (player == NULL) {
		//dstream<<"player==NULL"<<std::endl;
		// This can happen sometimes; clients and players are not in perfect sync.
		disabled = true;
		return;
	}

	// Won't send anything if already sending
	if (client->m_blocks_sending.size() >= g_settings->getU16
			("max_simultaneous_block_sends_per_client")) {
		//dstream<<"max_simul_sends"<<std::endl;
		//infostream<<"Not sending any blocks, Queue full."<<std::endl;
		disabled = true;
		return;
	}

	camera_p = player->getEyePosition();
	v3f player_speed = player->getSpeed();

	// Get player position and figure out a good focus point for block selection
	v3f player_speed_dir(0,0,0);
	if(player_speed.getLength() > 1.0*BS)
		player_speed_dir = player_speed / player_speed.getLength();
	// Predict to next block
	v3f camera_p_predicted = camera_p + player_speed_dir*MAP_BLOCKSIZE*BS;
	v3s16 focus_point_nodepos = floatToInt(camera_p_predicted, BS);
	focus_point = getNodeBlockPos(focus_point_nodepos);

	// Camera position and direction
	camera_dir = v3f(0,0,1);
	camera_dir.rotateYZBy(player->getPitch());
	camera_dir.rotateXZBy(player->getYaw());

	// If focus point has changed to a different MapBlock, reset radius value
	// for iterating from zero again
	if (alg->m_last_focus_point != focus_point) {
		alg->m_cycle->mapblock.nearest_unsent_d = 0;
		alg->m_cycle->farblock.nearest_unsent_d = 0;
		alg->m_last_focus_point = focus_point;
	}

	// Get some settings
	max_simul_sends_setting =
			g_settings->getU16("max_simultaneous_block_sends_per_client");
	time_from_building_limit_s =
			g_settings->getFloat("full_block_send_enable_min_time_from_building");
	max_mapblock_send_distance_setting =
			g_settings->getS16("max_block_send_distance");
	max_mapblock_generate_distance =
			g_settings->getS16("max_block_generate_distance");
	far_map_allow_generate =
			g_settings->getBool("far_map_allow_generate");

	// Derived settings
	s16 max_mapblock_send_distance =
			alg->m_radius_map < max_mapblock_send_distance_setting ?
			alg->m_radius_map : max_mapblock_send_distance_setting;

	// Reset periodically to workaround possible glitches due to whatever
	// reasons (this is somewhat guided by just heuristics, after all)
	if (alg->m_nearest_unsent_reset_timer > 20.0) {
		alg->m_nearest_unsent_reset_timer = 0;
		alg->m_cycle->mapblock.nearest_unsent_d = 0;
		alg->m_cycle->farblock.nearest_unsent_d = 0;
	}

	// MapBlock search iteration
	// Out-of-FOV distance limit is halved when limit is enabled
	s16 mb_fov_limit_activation_distance = mapblock.fov_limit_enabled ?
			max_mapblock_send_distance / 2 : max_mapblock_send_distance;
	mapblock.start(max_mapblock_send_distance, mb_fov_limit_activation_distance);

	// FarBlock search iteration
	// Out-of-FOV distance limit is always disabled
	s16 fb_fov_limit_activation_distance = alg->m_radius_far;
	farblock.start(alg->m_radius_far, fb_fov_limit_activation_distance);
}

WMSSuggestion AutosendCycle::suggestNextMapBlock(EmergeManager *emerge)
{
	for (; mapblock.d <= mapblock.d_max; mapblock.d++) {
		//dstream<<"AutosendMap: mapblock.d="<<mapblock.d<<std::endl;

		// Get the border/face dot coordinates of a mapblock.d-"radiused" box
		std::vector<v3s16> face_ps = FacePositionCache::getFacePositions(mapblock.d);
		// Continue from the last mapblock.i unless it was reset by something
		for (; mapblock.i < face_ps.size(); mapblock.i++) {
			v3s16 p = face_ps[mapblock.i] + focus_point;
			WantedMapSend wms(WMST_MAPBLOCK, p);

			// Limit fetched MapBlocks to a ball radius instead of a square
			// because that is how they are limited when drawing too
			v3s16 blockpos_nodes = p * MAP_BLOCKSIZE;
			v3f blockpos_center(
					blockpos_nodes.X * BS + MAP_BLOCKSIZE/2 * BS,
					blockpos_nodes.Y * BS + MAP_BLOCKSIZE/2 * BS,
					blockpos_nodes.Z * BS + MAP_BLOCKSIZE/2 * BS
			);
			v3f blockpos_relative = blockpos_center - camera_p;
			f32 distance = blockpos_relative.getLength();
			if (distance > mapblock.max_send_distance * MAP_BLOCKSIZE * BS) {
				/*dstream<<"AutosendMap: "<<wms.describe()
						<<": continue: distance: "<<distance<<std::endl;*/
				continue; // Not in range
			}

			// Calculate this thing
			u16 max_simultaneous_block_sends =
					figure_out_max_simultaneous_block_sends(
							max_simul_sends_setting,
							client->m_time_from_building,
							time_from_building_limit_s,
							mapblock.d);

			// Don't select too many blocks for sending
			if (client->SendingCount() >= max_simultaneous_block_sends) {
				/*dstream<<"AutosendMap: "<<wms.describe()
						<<": return: num_selected"<<std::endl;*/
				// Nothing to suggest
				return WMSSuggestion();
			}

			// Don't send blocks that are currently being transferred
			if (client->m_blocks_sending.count(wms)) {
				/*dstream<<"AutosendMap: "<<wms.describe()
						<<": continue: num sending"<<std::endl;*/
				continue;
			}

			// Don't go over hard map limits
			if (blockpos_over_limit(p)) {
				/*dstream<<"AutosendMap: "<<wms.describe()
						<<": continue: over limit"<<std::endl;*/
				continue;
			}

			// If this is true, inexistent blocks will be made from scratch
			bool allow_generate = mapblock.d <= max_mapblock_generate_distance;

			/*// Limit the generating area vertically to 2/3
			if(abs(p.Y - focus_point.Y) >
					max_mapblock_generate_distance - max_mapblock_generate_distance / 3)
				allow_generate = false;*/

			/*// Limit the send area vertically to 1/2
			if (abs(p.Y - focus_point.Y) > max_block_send_distance / 2)
				continue;*/

			if (mapblock.d >= mapblock.fov_limit_activation_distance) {
				// Don't generate or send if not in sight
				if(isBlockInSight(p, camera_p, camera_dir, alg->m_fov,
						10000*BS) == false)
				{
					/*dstream<<"AutosendMap: "<<wms.describe()
							<<": continue: not in sight"<<std::endl;*/
					continue;
				}
			}

			// Don't send blocks that have already been sent
			std::map<WantedMapSend, time_t>::const_iterator
					blocks_sent_i = client->m_blocks_sent.find(wms);
			if (blocks_sent_i != client->m_blocks_sent.end()){
				if (client->m_blocks_updated_since_last_send.count(wms) == 0) {
					/*dstream<<"AutosendMap: "<<wms.describe()
							<<": Already sent and not updated"<<std::endl;*/
					continue;
				}
				// NOTE: Don't do rate limiting for MapBlocks
				//time_t sent_time = blocks_sent_i->second;
				/*dstream<<"AutosendMap: "<<wms.describe()
						<<": Already sent but updated"<<std::endl;*/
			}

			/*
				Check if map has this block
			*/
			MapBlock *block = env->getMap().getBlockNoCreateNoEx(p);

			bool surely_not_found_on_disk = false;
			bool emerge_required = false;
			if(block != NULL)
			{
				// Reset usage timer, this block will be of use in the future.
				block->resetUsageTimer();

				// Block is dummy if data doesn't exist.
				// It means it has been not found from disk and not generated
				if(block->isDummy())
					surely_not_found_on_disk = true;

				// Block is valid if lighting is up-to-date and data exists
				if(block->isValid() == false)
					emerge_required = true;

				// If a block hasn't been generated but we would ask it to be
				// generated, it's invalid.
				if(block->isGenerated() == false && allow_generate)
					emerge_required = true;

				// This check is disabled because it mis-guesses sea floors to
				// not be worth transferring to the client, while they are.
				/*if (mapblock.d >= 4 && block->getDayNightDiff() == false)
					continue;*/
			}

			/*
				If block has been marked to not exist on disk (dummy)
				and generating new ones is not wanted, skip block.
			*/
			if(allow_generate == false && surely_not_found_on_disk == true)
			{
				//dstream<<"continue: not on disk, no generate"<<std::endl;
				// get next one.
				continue;
			}

			if(!block || surely_not_found_on_disk || emerge_required)
			{
				// Queue the block for loading or generating
				bool queue_full = !emerge->enqueueBlockEmerge(
						client->peer_id, p, allow_generate);
				if (!queue_full) {
					if (mapblock.nearest_emergequeued_d == INT32_MAX)
						mapblock.nearest_emergequeued_d = mapblock.d;
					// Queue is not full; continue searching
					continue;
				} else {
					if (mapblock.nearest_emergefull_d == INT32_MAX)
						mapblock.nearest_emergefull_d = mapblock.d;
					// Queue is full; don't bother searching futher
					return WMSSuggestion();
				}
			}

			// Suggest this block (is already loaded)
			return WMSSuggestion(wms);
		}

		// Now reset i as we go to next d
		mapblock.i = 0;
	}

	// Nothing to suggest
	return WMSSuggestion();
}

WMSSuggestion AutosendCycle::suggestNextFarBlock(
		EmergeManager *emerge, ServerFarMap *far_map)
{
	v3s16 focus_point_fb = getContainerPos(focus_point, FMP_SCALE);

	const float fb_size_bs = FMP_SCALE * MAP_BLOCKSIZE * BS;
	const float max_distance_f = fb_size_bs * farblock.max_send_distance;

	const size_t max_blind_generate_requests = 1;
	size_t num_blind_generate_requests = 0;

	for (; farblock.d <= farblock.d_max; farblock.d++) {
		//dstream<<"AutosendFar: farblock.d="<<farblock.d<<std::endl;

		// Get the border/face dot coordinates of a farblock.d-"radiused" box
		std::vector<v3s16> face_ps = FacePositionCache::getFacePositions(farblock.d);
		// Continue from the last farblock.i unless it was reset by something
		for (; farblock.i < face_ps.size(); farblock.i++) {
			v3s16 p = focus_point_fb + face_ps[farblock.i];

			WantedMapSend wms(WMST_FARBLOCK, p);

			// Calculate this thing
			u16 max_simultaneous_block_sends =
					figure_out_max_simultaneous_block_sends(
							max_simul_sends_setting,
							client->m_time_from_building,
							time_from_building_limit_s,
							farblock.d * FMP_SCALE);

			// Don't select too many blocks for sending
			if (client->SendingCount() >= max_simultaneous_block_sends) {
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": return: num_selected"<<std::endl;*/
				// Nothing to suggest
				return WMSSuggestion();
			}

			// Limit distance to a circular radius
			v3f farblock_pf(
				((float)p.X + 0.5) * fb_size_bs,
				((float)p.Y + 0.5) * fb_size_bs,
				((float)p.Z + 0.5) * fb_size_bs
			);
			float df = (camera_p - farblock_pf).getLength();
			if (df > max_distance_f) {
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": continue: distance"
						<<" (df="<<df<<", max_distance_f="<<max_distance_f<<")"
						<<std::endl;*/
				continue;
			}

			// Don't send blocks that are currently being transferred
			if (client->m_blocks_sending.count(wms)) {
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": continue: num sending"<<std::endl;*/
				continue;
			}

			// Don't go over hard map limits
			if (blockpos_over_limit(p * FMP_SCALE)) {
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": continue: over limit"<<std::endl;*/
				continue;
			}

			// We really can't generate everything; it would bloat up
			// the world database way too much.
			// TODO: Figure out how to do this in a good way
			bool allow_generate = wms.p.Y >= -1 && wms.p.Y <= 1;
			bool allow_load = wms.p.Y >= -1 && wms.p.Y <= 1;

			if (!far_map_allow_generate)
				allow_generate = false;

			ServerFarBlock *fb = far_map->getBlock(wms.p);

			// Check load states of MapBlocks
			const size_t mbs_total = FMP_SCALE * FMP_SCALE * FMP_SCALE;
			size_t mbs_unknown = mbs_total;
			size_t mbs_not_loaded = 0;
			size_t mbs_not_generated = 0;
			size_t mbs_generated = 0;
			if (fb) {
				const BlockAreaBitmap<ServerFarBlock::LoadState> &lm =
						fb->loaded_mapblocks;
				mbs_unknown = lm.count(ServerFarBlock::LS_UNKNOWN);
				mbs_not_loaded = lm.count(ServerFarBlock::LS_NOT_LOADED);
				mbs_not_generated = lm.count(ServerFarBlock::LS_NOT_GENERATED);
				mbs_generated = lm.count(ServerFarBlock::LS_GENERATED);
			}
			bool fully_loaded = allow_generate ? (mbs_generated == mbs_total) :
					(mbs_unknown + mbs_not_loaded == 0);

			// If none of this FarBlock is loaded (it may be if a player has
			// been very close to it) and we are generally not allowing loading
			// of this area for FarBlock purposes, completely skip this
			// FarBlock. This allows much faster loading of relevant FarBlocks
			// as we neither try to load nor send these FarBlocks at all.
			if (mbs_unknown + mbs_not_loaded == mbs_total && !allow_load) {
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": continue: not loaded and load disabled"<<std::endl;*/
				continue;
			}

			// FarBlock area in divisions (FarNodes)
			static const v3s16 divs_per_mb(
					SERVER_FB_MB_DIV, SERVER_FB_MB_DIV, SERVER_FB_MB_DIV);

			v3s16 area_offset_mb(
				FMP_SCALE * wms.p.X,
				FMP_SCALE * wms.p.Y,
				FMP_SCALE * wms.p.Z
			);

			v3s16 area_size_mb(FMP_SCALE, FMP_SCALE, FMP_SCALE);

			// Queue emergement of missing MapBlocks
			v3s16 bp;
			bool emerge_queue_full = false;
			for (bp.Z=area_offset_mb.Z; bp.Z<area_offset_mb.Z+area_size_mb.Z; bp.Z++)
			// From top towards bottom; loading looks better this way
			for (bp.Y=area_offset_mb.Y+area_size_mb.Y-1; bp.Y>=area_offset_mb.Y; bp.Y--)
			for (bp.X=area_offset_mb.X; bp.X<area_offset_mb.X+area_size_mb.X; bp.X++)
			{
				ServerFarBlock::LoadState load_state = fb ?
						fb->loaded_mapblocks.get(bp) : ServerFarBlock::LS_UNKNOWN;

				// There's nothing we can or need to do to generated ones
				if (load_state == ServerFarBlock::LS_GENERATED) {
					continue;
				}

				// Limit blind geneartions; i.e. generation-allowing emerge
				// requests of unloaded MapBlocks
				bool would_be_blind_generate =
						load_state < ServerFarBlock::LS_NOT_GENERATED;
				bool allow_generate_now = allow_generate;
				if (would_be_blind_generate &&
						num_blind_generate_requests >= max_blind_generate_requests)
					allow_generate_now = false;

				// If it's already loaded and we don't want to generate it, skip
				// it
				if (!allow_generate_now &&
						load_state == ServerFarBlock::LS_NOT_GENERATED) {
					/*dstream<<"AutosendFar: "<<wms.describe()
							<<": Not generating "<<PP(bp)<<std::endl;*/
					continue;
				}

				// Load or generate it
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": Adding "<<PP(bp)<<" to emerge queue"
						<<" (allow_generate_now="<<allow_generate_now<<")"
						<<std::endl;*/
				emerge_queue_full = !emerge->enqueueBlockEmerge(
						client->peer_id, bp, allow_generate_now);

				if (emerge_queue_full) {
					if (farblock.nearest_emergefull_d == INT32_MAX)
						farblock.nearest_emergefull_d = farblock.d;
					// Emerge queue is full; don't bother attempting to add more
					// stuff to it from this FarBlock and don't increment our
					// counters
					goto farblock_mapblocks_iterate_break;
				}

				if (farblock.nearest_emergequeued_d == INT32_MAX)
					farblock.nearest_emergequeued_d = farblock.d;

				if (allow_generate_now) {
					if (would_be_blind_generate) {
						num_blind_generate_requests++;
					}
				}
			}
farblock_mapblocks_iterate_break:

			if (mbs_unknown + mbs_not_loaded == mbs_total) {
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": continue: not loaded"<<std::endl;*/

				// None MapBlocks inside this FarBlock have not been loaded.
				// Many of them are being queued for emerging, so just wait for
				// them and don't go poking other FarBlocks.
				return WMSSuggestion();
			}

			// Don't send blocks that have already been sent and have not been
			// updated. Check this after the emerge queuing so that they have a
			// chance of being loaded so that they can pass this check later.
			std::map<WantedMapSend, time_t>::const_iterator
					blocks_sent_i = client->m_blocks_sent.find(wms);
			if (blocks_sent_i != client->m_blocks_sent.end()){
				if (client->m_blocks_updated_since_last_send.count(wms) == 0) {
					/*dstream<<"AutosendFar: "<<wms.describe()
							<<": Already sent and not updated"<<std::endl;*/
					continue;
				}
				time_t sent_time = blocks_sent_i->second;
				if (sent_time + 2 > time(NULL)) {
					/*dstream<<"AutosendFar: "<<wms.describe()
							<<": Already sent; rate-limiting"<<std::endl;*/
					continue;
				}
				/*dstream<<"AutosendFar: "<<wms.describe()
						<<": Already sent but updated; allowing"<<std::endl;*/
			}

			// Suggest this block
			WMSSuggestion wmss(wms);
			wmss.is_fully_loaded = fully_loaded;
			return wmss;
		}

		// Now reset i as we go to next d
		farblock.i = 0;
	}

	// Nothing to suggest
	return WMSSuggestion();
}

WMSSuggestion AutosendCycle::getNextBlock(
		EmergeManager *emerge, ServerFarMap *far_map)
{
	DSTACK(FUNCTION_NAME);

	//dstream<<"AutosendCycle::getNextBlock()"<<std::endl;

	if (disabled)
		return WMSSuggestion();

	// Get MapBlock and FarBlock suggestions

	WMSSuggestion suggested_mb = suggestNextMapBlock(emerge);
	WMSSuggestion suggested_fb = suggestNextFarBlock(emerge, far_map);

	//dstream<<"suggested_mb = "<<suggested_mb.describe()<<std::endl;
	//dstream<<"suggested_fb = "<<suggested_fb.describe()<<std::endl;

	// Prioritize suggestions

	std::vector<WMSSuggestion> suggestions;
	if (suggested_mb.wms.type != WMST_INVALID)
		suggestions.push_back(suggested_mb);
	if (suggested_fb.wms.type != WMST_INVALID)
		suggestions.push_back(suggested_fb);

	if (suggestions.empty()) {
		// Nothing to send or prioritize
		return WMSSuggestion();
	}

	// Prioritize if there is more than one option
	if (suggestions.size() > 1) {
		v3s16 camera_np = floatToInt(camera_p, BS);
		std::sort(suggestions.begin(), suggestions.end(),
				WMSSPriority(camera_np, alg->m_far_weight));
	}

	// Take the most important one
	WMSSuggestion wmss = suggestions[0];

	//dstream<<"wmss = "<<wmss.describe()<<std::endl;

	// Keep track of the distance of the closest block being sent (separetely
	// for MapBlocks and FarBlocks)

	if (wmss.wms.type == WMST_MAPBLOCK) {
		if(mapblock.nearest_sendqueued_d == INT32_MAX)
			mapblock.nearest_sendqueued_d = mapblock.d;
		mapblock.nothing_to_send_timer = 0.0f;
	}

	if (wmss.wms.type == WMST_FARBLOCK) {
		if(farblock.nearest_sendqueued_d == INT32_MAX)
			farblock.nearest_sendqueued_d = farblock.d;
		farblock.nothing_to_send_timer = 0.0f;
	}

	return wmss;
}

void AutosendCycle::Search::runTimers(float dtime)
{
	nothing_to_send_timer += dtime;
	nothing_to_send_pause_timer -= dtime;
}

void AutosendCycle::Search::start(s16 max_send_distance_,
		s16 fov_limit_activation_distance_)
{
	/*dstream<<"AutosendCycle::Search::start()"
			<<": max_send_distance="<<max_send_distance_
			<<", fov_limit_activation_distance="<<fov_limit_activation_distance_
			<<std::endl;*/

	max_send_distance = max_send_distance_; // Needed by finish()
	fov_limit_activation_distance = fov_limit_activation_distance_;
	i = 0;

	// We will start from a radius that still has unsent MapBlocks
	d_start = nearest_unsent_d >= 0 ? nearest_unsent_d : 0;
	d = d_start;

	//dstream<<"d_start="<<d_start<<std::endl;
	//infostream<<"d_start="<<d_start<<std::endl;

	// Don't loop very much at a time. This function is called each server tick
	// so just a few steps will work fine (+2 is 3 steps per call).
	d_max = 0;
	if (d_start < 5)
		d_max = d_start + 2;
	else if (d_max < 8)
		d_max = d_start + 1;
	else
		d_max = d_start; // These iterations start to be rather heavy

	if (d_max > max_send_distance)
		d_max = max_send_distance;

	//dstream<<"d_max="<<d_max<<std::endl;

	// Keep track of... things. We need these in order to figure out where to
	// continue iterating on the next call to this function.
	// TODO: Put these in the class
	nearest_emergequeued_d = INT32_MAX;
	nearest_emergefull_d = INT32_MAX;
	nearest_sendqueued_d = INT32_MAX;
}

void AutosendCycle::Search::finish(bool enable_fov_toggle)
{
	/*infostream << "nearest_emergequeued_d = "<<nearest_emergequeued_d
			<<", nearest_emergefull_d = "<<nearest_emergefull_d
			<<", nearest_sendqueued_d = "<<nearest_sendqueued_d
			<<std::endl;*/

	// Because none of the things we queue for sending or emerging or anything
	// will necessarily be sent or anything, next time we need to continue
	// iterating from the closest radius of any of that happening so that we can
	// check whether they were sent or emerged or otherwise handled.
	s32 closest_required_re_check = INT32_MAX;
	if (nearest_emergequeued_d < closest_required_re_check)
		closest_required_re_check = nearest_emergequeued_d;
	if (nearest_emergefull_d < closest_required_re_check)
		closest_required_re_check = nearest_emergefull_d;
	if (nearest_sendqueued_d < closest_required_re_check)
		closest_required_re_check = nearest_sendqueued_d;

	bool searched_full_range = false;
	bool result_may_be_available_later_at_this_d = false;

	if (closest_required_re_check != INT32_MAX) {
		// We did something that requires a result to be checked later. Continue
		// from that on the next time.
		nearest_unsent_d = closest_required_re_check;
		// If nothing has been sent in a moment, indicating that the emerge
		// thread is not finding anything on disk anymore, caller should start a
		// fresh pass without the FOV limit.
		result_may_be_available_later_at_this_d = true;
	} else if (d > max_send_distance) {
		// We iterated all the way through to the end of the send radius, if you
		// can believe that.
		nearest_unsent_d = 0;
		// Caller should do a second pass with FOV limiting disabled or start
		// from the beginning after a short idle delay. (with FOV limiting
		// enabled because nobody knows what the future holds.)
		searched_full_range = true;
	} else {
		// Absolutely nothing interesting happened. Next time we will continue
		// iterating from the next radius.
		nearest_unsent_d = d;
	}

	// Trigger FOV limit removal in certain situations
	if (result_may_be_available_later_at_this_d) {
		/*dstream<<"CycleResult: Result may be available later at "
				<<nearest_unsent_d<<std::endl;*/

		// If nothing has been sent in a moment, indicating that the emerge
		// thread is not finding anything on disk anymore, start a fresh
		// pass without the FOV limit.
		if (enable_fov_toggle && fov_limit_enabled &&
				nothing_to_send_timer >= 3.0f) {
			/*dstream<<"CycleResult: Nothing to send"
					<<"; Disabling FOV limit"<<std::endl;*/

			fov_limit_enabled = false;
			// Have to be reset in order to not trigger this immediately again
			nothing_to_send_timer = 0.0f;
		}
	} else if(searched_full_range) {
		//dstream<<"CycleResult: Searched full range"<<std::endl;

		// We iterated all the way through to the end of the send radius, if you
		// can believe that.
		if (enable_fov_toggle && fov_limit_enabled) {
			/*dstream<<"CycleResult: Iterated all the way through"
					<<"; Disabling FOV limit"<<std::endl;*/

			// Do a second pass with FOV limiting disabled
			fov_limit_enabled = false;
		} else {
			/*dstream<<"CycleResult: Initiating idle sleep"<<std::endl;*/

			// Start from the beginning after a short idle delay, with FOV
			// limiting enabled because nobody knows what the future holds.
			fov_limit_enabled = true;
			nothing_to_send_pause_timer = 2.0;
		}
	} else {
		/*dstream<<"CycleResult: Continuing at "
				<<nearest_unsent_d<<std::endl;*/
	}
}

void AutosendCycle::finish()
{
	//dstream<<"AutosendCycle::finish()"<<std::endl;

	if (disabled) {
		// If disabled, none of our variables are even initialized
		return;
	}

	bool enable_fov_toggle = (alg->m_fov != 0.0f);
	mapblock.finish(enable_fov_toggle);
	farblock.finish(enable_fov_toggle);
}

AutosendAlgorithm::AutosendAlgorithm(RemoteClient *client):
	m_client(client),
	m_cycle(new AutosendCycle()),
	m_radius_map(0),
	m_radius_far(0),
	m_far_weight(8.0f),
	m_fov(1.72f), // In radians
	m_nearest_unsent_reset_timer(0.0)
{}

AutosendAlgorithm::~AutosendAlgorithm()
{
	delete m_cycle;
}

void AutosendAlgorithm::cycle(float dtime, ServerEnvironment *env)
{
	m_cycle->finish();

	// Increment timers
	m_cycle->mapblock.runTimers(dtime);
	m_cycle->farblock.runTimers(dtime);
	m_nearest_unsent_reset_timer += dtime;

	m_cycle->start(this, m_client, env);
}

WMSSuggestion AutosendAlgorithm::getNextBlock(
		EmergeManager *emerge, ServerFarMap *far_map)
{
	return m_cycle->getNextBlock(emerge, far_map);
}

void AutosendAlgorithm::resetMapblockSearchRadius(const v3s16 &mb_p)
{
	s16 reset_to_d = 0; // Radius value in MapBlocks to reset the search to

	// Get distance to player and use it as the reset value in order to not scan
	// unnecessarily close
	Player *player = m_client->m_env->getPlayer(m_client->peer_id);
	if (player) {
		v3f camera_pf_nodes = player->getEyePosition() / BS;
		v3s16 mb_nodes = mb_p * MAP_BLOCKSIZE;
		v3f mb_pf_nodes(mb_nodes.X, mb_nodes.Y, mb_nodes.Z);
		float distance_nodes = (mb_pf_nodes - camera_pf_nodes).getLength();
		reset_to_d = distance_nodes / MAP_BLOCKSIZE;
	}

	/*dstream<<"AutosendAlgorithm::resetMapblockSearchRadius(): reset_to_d="
			<<reset_to_d<<std::endl;*/

	// Reset the index for looping through blocks at the given radius
	m_cycle->mapblock.i = 0;

	// Reset this, which maybe should already do the job
	if (m_cycle->mapblock.nearest_unsent_d < reset_to_d){
		m_cycle->mapblock.nearest_unsent_d = reset_to_d;
	}

	// Apparently all of these have to be reset in order to have the search
	// start at 0 because of what finish() does (it's maybe a bug, but as long
	// as this works, we really don't need to care)
	if (m_cycle->mapblock.nearest_emergequeued_d > reset_to_d)
		m_cycle->mapblock.nearest_emergequeued_d = reset_to_d;
	if (m_cycle->mapblock.nearest_emergefull_d > reset_to_d)
		m_cycle->mapblock.nearest_emergefull_d = reset_to_d;
	if (m_cycle->mapblock.nearest_sendqueued_d > reset_to_d)
		m_cycle->mapblock.nearest_sendqueued_d = reset_to_d;

	// Start searching for MapBlocks to be sent soon enough (don't set this to
	// zero because it would cause unnecessary CPU usage)
	if (m_cycle->mapblock.nothing_to_send_pause_timer > 0.1f)
		m_cycle->mapblock.nothing_to_send_pause_timer = 0.1f;
}

std::string AutosendAlgorithm::describeStatus()
{
	return "(mapblock.nearest_unsent_d="+itos(m_cycle->mapblock.nearest_unsent_d)+", "
			"farblock.nearest_unsent_d="+itos(m_cycle->farblock.nearest_unsent_d)+")";
}

/*
	ClientInterface
*/

const char *ClientInterface::statenames[] = {
	"Invalid",
	"Disconnecting",
	"Denied",
	"Created",
	"AwaitingInit2",
	"HelloSent",
	"InitDone",
	"DefinitionsSent",
	"Active",
	"SudoMode",
};

std::string ClientInterface::state2Name(ClientState state)
{
	return statenames[state];
}

WMSSuggestion RemoteClient::getNextBlock(
		EmergeManager *emerge, ServerFarMap *far_map)
{
	// If the client has not indicated it supports the new algorithm, fill in
	// autosend parameters and things should work fine
	if (m_fallback_autosend_active)
	{
		s16 radius_map = g_settings->getS16("max_block_send_distance");
		s16 radius_far = 0; // Old client does not understand FarBlocks
		float far_weight = 8.0f; // Whatever non-zero
		float fov = 1.72f; // Assume something (radians)
		m_autosend.setParameters(radius_map, radius_far, far_weight, fov);

		// Continue normally.
	}

	/*
		Autosend

		NOTE: All auto-sent stuff is considered higher priority than custom
		transfers. If the client wants to get custom stuff quickly, it has to
		disable autosend.
	*/
	WMSSuggestion wmss = m_autosend.getNextBlock(emerge, far_map);
	WantedMapSend wms = wmss.wms;
	if (wms.type != WMST_INVALID)
		return wmss;

	/*
		Handle map send queue as set by the client for custom map transfers

		// TODO: This needs rework
	*/
	for (size_t i=0; i<m_map_send_queue.size(); i++) {
		const WantedMapSend &wms = m_map_send_queue[i];
		if (wms.type == WMST_MAPBLOCK) {
			/*verbosestream << "Server: Client "<<peer_id<<" wants MapBlock ("
					<<wms.p.X<<","<<wms.p.Y<<","<<wms.p.Z<<")"
					<< std::endl;*/

			// Do not go over-limit
			if (blockpos_over_limit(wms.p))
				continue;

			// Don't send blocks that are currently being transferred
			if (m_blocks_sending.find(wms) != m_blocks_sending.end())
				continue;

			// Don't send blocks that have already been sent, unless it has been
			// updated
			if (m_blocks_sent.find(wms) != m_blocks_sent.end()) {
				if (m_blocks_updated_since_last_send.count(wms) == 0)
					continue;
			}

			// If the MapBlock is not loaded, it will be queued to be loaded or
			// generated. Otherwise it will be added to 'dest'.

			const bool allow_generate = true;

			MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(wms.p);

			bool surely_not_found_on_disk = false;
			bool emerge_required = false;
			if(block != NULL)
			{
				// Reset usage timer, this block will be of use in the future.
				block->resetUsageTimer();

				// Block is dummy if data doesn't exist.
				// It means it has been not found from disk and not generated
				if(block->isDummy())
					surely_not_found_on_disk = true;

				// Block is valid if lighting is up-to-date and data exists
				if(block->isValid() == false)
					emerge_required = true;

				if(block->isGenerated() == false && allow_generate)
					emerge_required = true;

				// This check is disabled because it mis-guesses sea floors to
				// not be worth transferring to the client, while they are.
				/*if (d >= 4 && block->getDayNightDiff() == false)
					continue;*/
			}

			if(allow_generate == false && surely_not_found_on_disk == true) {
				// NOTE: If we wanted to avoid generating new blocks based on
				// some criterion, that check woould go here and we would call
				// 'continue' if the block should not be generated.

				// TODO: There needs to be some new way or limiting which
				// positions are allowed to be generated, because we aren't
				// going to look up the player's position and compare it with
				// max_mapblock_generate_distance anymore. Maybe a configured set
				// of allowed areas, or maybe a callback to Lua.

				// NOTE: There may need to be a way to tell the client that this
				// block will not be transferred to it no matter how nicely it
				// asks. Otherwise the requested send queue is going to get
				// filled up by these and less important blocks further away
				// that happen to have been already generated will not transfer.
			}

			/*
				If block does not exist, add it to the emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || emerge_required)
			{
				bool allow_generate = true;
				if (!emerge->enqueueBlockEmerge(peer_id, wms.p, allow_generate)) {
					// EmergeThread's queue is full; maybe it's not full on the
					// next time this is called.
				}

				// This block is not available now; hopefully it appears on some
				// next call to this function.
				continue;
			}

			// The block is available
			return wms;
		}
		if (wms.type == WMST_FARBLOCK) {
			/*verbosestream << "Server: Client "<<peer_id<<" wants FarBlock ("
					<<wms.p.X<<","<<wms.p.Y<<","<<wms.p.Z<<")"
					<< std::endl;*/

			// Do not go over-limit
			if (blockpos_over_limit(wms.p))
				continue;

			// Don't send blocks that are currently being transferred
			if (m_blocks_sending.find(wms) != m_blocks_sending.end())
				continue;

			// Don't send blocks that have already been sent
			std::map<WantedMapSend, time_t>::const_iterator
					blocks_sent_i = m_blocks_sent.find(wms);
			if (blocks_sent_i != m_blocks_sent.end()){
				if (m_blocks_updated_since_last_send.count(wms) == 0) {
					/*dstream<<"RemoteClient: Already sent and not updated: "
							<<"("<<wms.p.X<<","<<wms.p.Y<<","<<wms.p.Z<<")"
							<<std::endl;*/
					continue;
				}
				time_t sent_time = blocks_sent_i->second;
				if (sent_time + 5 > time(NULL)) {
					/*dstream<<"RemoteClient: Already sent; rate-limiting: "
							<<"("<<wms.p.X<<","<<wms.p.Y<<","<<wms.p.Z<<")"
							<<std::endl;*/
					continue;
				}
				dstream<<"RemoteClient: Already sent but updated; allowing re-send: "
						<<"("<<wms.p.X<<","<<wms.p.Y<<","<<wms.p.Z<<")"
						<<std::endl;
			}

			// TODO: Stay determined at which FarBlock to load so that we don't
			//       load them all at the same time in weird stripes

			// TODO: Use modification counter?

			// TODO: Check if data for this is available and if not, possibly
			//       request an emerge of the required area

			return wms;
		}
	}

	return WantedMapSend();
}

void RemoteClient::cycleAutosendAlgorithm(float dtime)
{
	m_autosend.cycle(dtime, m_env);
}

void RemoteClient::GotBlock(const WantedMapSend &wms)
{
	if (m_blocks_sending.find(wms) != m_blocks_sending.end()) {
		m_blocks_sent[wms] = m_blocks_sending[wms];
		m_blocks_sending.erase(wms);
	} else {
		m_excess_gotblocks++;
	}
}

void RemoteClient::SendingBlock(const WantedMapSend &wms)
{
	assert(!m_blocks_sending.count(wms) ||
			m_blocks_updated_while_sending.count(wms) &&
			"Block shouldn't be re-sent if it is being sent and was not"
			" updated while being sent");

	m_blocks_sending[wms] = time(NULL);
	m_blocks_updated_since_last_send.erase(wms);
	m_blocks_updated_while_sending.erase(wms);
}

void RemoteClient::SetBlockUpdated(const WantedMapSend &wms)
{
	//dstream<<"SetBlockUpdated: "<<wms.describe()<<std::endl;

	// Reset autosend's search radius if it's a MapBlock
	if (wms.type == WMST_MAPBLOCK) {
		m_autosend.resetMapblockSearchRadius(wms.p);
	}

	m_blocks_updated_since_last_send.insert(wms);

	if (m_blocks_sending.count(wms)) {
		m_blocks_updated_while_sending.insert(wms);
	}
}

void RemoteClient::SetMapBlockUpdated(v3s16 p)
{
	SetBlockUpdated(WantedMapSend(WMST_MAPBLOCK, p));

	// Also set the corresponding FarBlock not sent
	v3s16 farblock_p = getContainerPos(p, FMP_SCALE);
	SetBlockUpdated(WantedMapSend(WMST_FARBLOCK, farblock_p));

	/*dstream<<"RemoteClient: now not sent: MB"<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<" FB"<<"("<<farblock_p.X<<","<<farblock_p.Y<<","<<farblock_p.Z<<")"
			<<std::endl;*/
}

void RemoteClient::SetMapBlocksUpdated(std::map<v3s16, MapBlock*> &blocks)
{
	for(std::map<v3s16, MapBlock*>::iterator
			i = blocks.begin();
			i != blocks.end(); ++i)
	{
		v3s16 p = i->first;
		SetMapBlockUpdated(p);
	}
}

void RemoteClient::ResendBlockIfOnWire(const WantedMapSend &wms)
{
	// if this block is on wire, mark it for sending again as soon as possible
	if (m_blocks_sending.find(wms) != m_blocks_sending.end()) {
		SetBlockUpdated(wms);
	}
}

void RemoteClient::notifyEvent(ClientStateEvent event)
{
	std::ostringstream myerror;
	switch (m_state)
	{
	case CS_Invalid:
		//intentionally do nothing
		break;
	case CS_Created:
		switch (event) {
		case CSE_Hello:
			m_state = CS_HelloSent;
			break;
		case CSE_InitLegacy:
			m_state = CS_AwaitingInit2;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		/* GotInit2 SetDefinitionsSent SetMediaSent */
		default:
			myerror << "Created: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_Denied:
		/* don't do anything if in denied state */
		break;
	case CS_HelloSent:
		switch(event)
		{
		case CSE_AuthAccept:
			m_state = CS_AwaitingInit2;
			if ((chosen_mech == AUTH_MECHANISM_SRP)
					|| (chosen_mech == AUTH_MECHANISM_LEGACY_PASSWORD))
				srp_verifier_delete((SRPVerifier *) auth_data);
			chosen_mech = AUTH_MECHANISM_NONE;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			if ((chosen_mech == AUTH_MECHANISM_SRP)
					|| (chosen_mech == AUTH_MECHANISM_LEGACY_PASSWORD))
				srp_verifier_delete((SRPVerifier *) auth_data);
			chosen_mech = AUTH_MECHANISM_NONE;
			break;
		default:
			myerror << "HelloSent: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_AwaitingInit2:
		switch(event)
		{
		case CSE_GotInit2:
			confirmSerializationVersion();
			m_state = CS_InitDone;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;

		/* Init SetDefinitionsSent SetMediaSent */
		default:
			myerror << "InitSent: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;

	case CS_InitDone:
		switch(event)
		{
		case CSE_SetDefinitionsSent:
			m_state = CS_DefinitionsSent;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;

		/* Init GotInit2 SetMediaSent */
		default:
			myerror << "InitDone: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_DefinitionsSent:
		switch(event)
		{
		case CSE_SetClientReady:
			m_state = CS_Active;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		/* Init GotInit2 SetDefinitionsSent */
		default:
			myerror << "DefinitionsSent: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_Active:
		switch(event)
		{
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SudoSuccess:
			m_state = CS_SudoMode;
			if ((chosen_mech == AUTH_MECHANISM_SRP)
					|| (chosen_mech == AUTH_MECHANISM_LEGACY_PASSWORD))
				srp_verifier_delete((SRPVerifier *) auth_data);
			chosen_mech = AUTH_MECHANISM_NONE;
			break;
		/* Init GotInit2 SetDefinitionsSent SetMediaSent SetDenied */
		default:
			myerror << "Active: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
			break;
		}
		break;
	case CS_SudoMode:
		switch(event)
		{
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SudoLeave:
			m_state = CS_Active;
			break;
		default:
			myerror << "Active: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
			break;
		}
		break;
	case CS_Disconnecting:
		/* we are already disconnecting */
		break;
	}
}

u32 RemoteClient::uptime()
{
	return getTime(PRECISION_SECONDS) - m_connection_time;
}

ClientInterface::ClientInterface(con::Connection* con)
:
	m_con(con),
	m_env(NULL),
	m_print_info_timer(0.0)
{

}
ClientInterface::~ClientInterface()
{
	/*
		Delete clients
	*/
	{
		MutexAutoLock clientslock(m_clients_mutex);

		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{

			// Delete client
			delete i->second;
		}
	}
}

std::vector<u16> ClientInterface::getClientIDs(ClientState min_state)
{
	std::vector<u16> reply;
	MutexAutoLock clientslock(m_clients_mutex);

	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); ++i)
	{
		if (i->second->getState() >= min_state)
			reply.push_back(i->second->peer_id);
	}

	return reply;
}

std::vector<std::string> ClientInterface::getPlayerNames()
{
	return m_clients_names;
}


void ClientInterface::step(float dtime)
{
	m_print_info_timer += dtime;
	if(m_print_info_timer >= 30.0)
	{
		m_print_info_timer = 0.0;
		UpdatePlayerList();
	}
}

void ClientInterface::UpdatePlayerList()
{
	if (m_env != NULL)
		{
		std::vector<u16> clients = getClientIDs();
		m_clients_names.clear();


		if(!clients.empty())
			infostream<<"Players:"<<std::endl;

		for(std::vector<u16>::iterator
			i = clients.begin();
			i != clients.end(); ++i) {
			Player *player = m_env->getPlayer(*i);

			if (player == NULL)
				continue;

			infostream << "* " << player->getName() << "\t";

			{
				MutexAutoLock clientslock(m_clients_mutex);
				RemoteClient* client = lockedGetClientNoEx(*i);
				if(client != NULL)
					client->PrintInfo(infostream);
			}

			m_clients_names.push_back(player->getName());
		}
	}
}

void ClientInterface::send(u16 peer_id, u8 channelnum,
		NetworkPacket* pkt, bool reliable)
{
	m_con->Send(peer_id, channelnum, pkt, reliable);
}

void ClientInterface::sendToAll(u16 channelnum,
		NetworkPacket* pkt, bool reliable)
{
	MutexAutoLock clientslock(m_clients_mutex);
	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); ++i) {
		RemoteClient *client = i->second;

		if (client->net_proto_version != 0) {
			m_con->Send(client->peer_id, channelnum, pkt, reliable);
		}
	}
}

RemoteClient* ClientInterface::getClientNoEx(u16 peer_id, ClientState state_min)
{
	MutexAutoLock clientslock(m_clients_mutex);
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n == m_clients.end())
		return NULL;

	if (n->second->getState() >= state_min)
		return n->second;
	else
		return NULL;
}

RemoteClient* ClientInterface::lockedGetClientNoEx(u16 peer_id, ClientState state_min)
{
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n == m_clients.end())
		return NULL;

	if (n->second->getState() >= state_min)
		return n->second;
	else
		return NULL;
}

ClientState ClientInterface::getClientState(u16 peer_id)
{
	MutexAutoLock clientslock(m_clients_mutex);
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n == m_clients.end())
		return CS_Invalid;

	return n->second->getState();
}

void ClientInterface::setPlayerName(u16 peer_id,std::string name)
{
	MutexAutoLock clientslock(m_clients_mutex);
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n != m_clients.end())
		n->second->setName(name);
}

void ClientInterface::DeleteClient(u16 peer_id)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n == m_clients.end())
		return;

	/*
		Mark objects to be not known by the client
	*/
	//TODO this should be done by client destructor!!!
	RemoteClient *client = n->second;
	// Handle objects
	for(std::set<u16>::iterator
			i = client->m_known_objects.begin();
			i != client->m_known_objects.end(); ++i)
	{
		// Get object
		u16 id = *i;
		ServerActiveObject* obj = m_env->getActiveObject(id);

		if(obj && obj->m_known_by_count > 0)
			obj->m_known_by_count--;
	}

	// Delete client
	delete m_clients[peer_id];
	m_clients.erase(peer_id);
}

void ClientInterface::CreateClient(u16 peer_id)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client shouldn't already exist
	if(n != m_clients.end()) return;

	// Create client
	RemoteClient *client = new RemoteClient(m_env);
	client->peer_id = peer_id;
	m_clients[client->peer_id] = client;
}

void ClientInterface::event(u16 peer_id, ClientStateEvent event)
{
	{
		MutexAutoLock clientlock(m_clients_mutex);

		// Error check
		std::map<u16, RemoteClient*>::iterator n;
		n = m_clients.find(peer_id);

		// No client to deliver event
		if (n == m_clients.end())
			return;
		n->second->notifyEvent(event);
	}

	if ((event == CSE_SetClientReady) ||
		(event == CSE_Disconnect)     ||
		(event == CSE_SetDenied))
	{
		UpdatePlayerList();
	}
}

u16 ClientInterface::getProtocolVersion(u16 peer_id)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);

	// No client to get version
	if (n == m_clients.end())
		return 0;

	return n->second->net_proto_version;
}

void ClientInterface::setClientVersion(u16 peer_id, u8 major, u8 minor, u8 patch, std::string full)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);

	// No client to set versions
	if (n == m_clients.end())
		return;

	n->second->setVersionInfo(major,minor,patch,full);
}
