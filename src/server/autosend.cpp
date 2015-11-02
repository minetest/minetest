/*
Minetest
Copyright (C) 2010-2022 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "autosend.h"
#include "clientiface.h" // RemoteClient
#include "mapblock.h"
#include "serverenvironment.h"
#include "network/wms_priority.h"
#include "server/player_sao.h"
#include "server/luaentity_sao.h"
#include "remoteplayer.h"
#include "face_position_cache.h"
#include "util/basic_macros.h"

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

RemoteClient::RemoteClient(ServerEnvironment *env):
	m_env(env),
	m_autosend(this)
{
}

struct AutosendCycle
{
	bool disabled = false;
	AutosendAlgorithm *alg = nullptr;
	RemoteClient *client = nullptr;
	ServerEnvironment *env = nullptr;

	v3f camera_p;
	v3f camera_dir;
	v3s16 focus_point_nodepos;
	v3s16 focus_point;
	float fov = 1.72f; // Modified FOV based on client's FOV

	u16 max_simul_sends_setting = 0;
	float time_from_building_limit_s = 0;
	s16 max_mapblock_send_distance_setting = 0; // Setting as-is
	s16 max_mapblock_generate_distance_setting = 0; // Setting as-is
	s16 max_mapblock_send_distance = 0; // Modified value during cycle
	s16 max_mapblock_generate_distance = 0; // Modified value during cycle
	bool far_map_allow_generate = false;
	bool server_side_occlusion_culling_setting = true;
	s16 block_send_optimize_distance_setting = 0; // TODO: Remove?
	float max_send_distance_bs = 0;

	std::list<v3s16> finished_mapblock_emerges;

	struct Search {
		s32 log_id; // Id used for logging and debugging (peer id)
		const char *log_name = "?"; // Name used for logging and debugging (mapblock/farblock)

		// All of these are reset by start()
		s16 max_send_distance = 0;
		s16 fov_limit_activation_distance = 0;
		s16 d_start = 0;
		s16 d_max = 0;
		s16 d = 0;
		size_t i = 0;
		s32 nearest_emergequeued_d = 0;
		s32 nearest_emergefull_d = 0;
		s32 nearest_sendqueued_d = 0;

		// All of these are persistent
		s16 nearest_unsent_d = 0;
		bool fov_limit_enabled = true; // Automatically turned off to transfer the rest
		float nothing_to_send_timer = 0; // // Counts up from 0
		float nothing_to_send_pause_timer = 0; // CPU usage optimization. Pause if >=0; decrements

		Search(){}

		void runTimers(float dtime);

		void start(s16 max_send_distance_, s16 fov_limit_activation_distance_);

		void finish(bool enable_fov_toggle);
	};

	Search mapblock;
	Search farblock;

	AutosendCycle():
		disabled(true)
	{
		mapblock.log_name = "map";
		farblock.log_name = "far";
	}

	void start(AutosendAlgorithm *alg_,
			RemoteClient *client_, ServerEnvironment *env_);

	//WMSSuggestion getNextBlock(EmergeManager *emerge, ServerFarMap *far_map);
	WMSSuggestion getNextBlock(EmergeManager *emerge);

	void finish();

private:
	enum class MapBlockAnalysis {
		IN_VIEW_AND_LOADED,
		IN_VIEW_BUT_NOT_LOADED, // Disk not checked
		IN_VIEW_BUT_NOT_GENERATED, // Disk checked
		NOT_IN_VIEW,
		ALREADY_SENT,
	};
	MapBlockAnalysis checkMapBlock(const v3s16 &p);
	WMSSuggestion suggestNextMapBlock(EmergeManager *emerge);
	//WMSSuggestion suggestNextFarBlock(EmergeManager *emerge, ServerFarMap *far_map);
};

LuaEntitySAO *getAttachedObject(PlayerSAO *sao, ServerEnvironment *env)
{
	if (!sao->isAttached())
		return nullptr;

	int id;
	std::string bone;
	v3f dummy;
	bool force_visible;
	sao->getAttachment(&id, &bone, &dummy, &dummy, &force_visible);
	ServerActiveObject *ao = env->getActiveObject(id);
	while (id && ao) {
		ao->getAttachment(&id, &bone, &dummy, &dummy, &force_visible);
		if (id)
			ao = env->getActiveObject(id);
	}
	return dynamic_cast<LuaEntitySAO *>(ao);
}

void AutosendCycle::start(AutosendAlgorithm *alg_,
		RemoteClient *client_, ServerEnvironment *env_)
{
	alg = alg_;
	client = client_;
	env = env_;

	mapblock.log_id = client->peer_id;
	farblock.log_id = client->peer_id;

	disabled = false;

	if (alg->m_radius_map == 0 && alg->m_radius_far == 0) {
		tracestream<<"AutosendCycle::start(): peer_id="<<client->peer_id
				<<": radius=0"<<std::endl;
		disabled = true;
		return;
	}

	// Disable if MapBlock and FarBlock pauses are active
	if (mapblock.nothing_to_send_pause_timer >= 0 &&
			farblock.nothing_to_send_pause_timer >= 0) {
		tracestream<<"AutosendCycle::start(): peer_id="<<client->peer_id
				<<": nothing to send pause (map+far)"<<std::endl;
		disabled = true;
		return;
	}

	RemotePlayer *player = env->getPlayer(client->peer_id);
	if (player == NULL) {
		tracestream<<"AutosendCycle::start(): peer_id="<<client->peer_id
				<<": player==NULL"<<std::endl;
		// This can happen sometimes; clients and players are not in perfect sync.
		disabled = true;
		return;
	}

	PlayerSAO *sao = player->getPlayerSAO();
	if (sao == NULL) {
		tracestream<<"AutosendCycle::start(): peer_id="<<client->peer_id
				<<": sao==NULL"<<std::endl;
		disabled = true;
		return;
	}

	// Get some settings. We need the first one for the next check so get them
	// all.
	max_simul_sends_setting =
			g_settings->getU16("max_simultaneous_block_sends_per_client");
	time_from_building_limit_s =
			g_settings->getFloat("full_block_send_enable_min_time_from_building");
	max_mapblock_send_distance_setting =
			g_settings->getS16("max_block_send_distance");
	max_mapblock_generate_distance_setting =
			g_settings->getS16("max_block_generate_distance");
	server_side_occlusion_culling_setting =
			g_settings->getBool("server_side_occlusion_culling");
	block_send_optimize_distance_setting =
			g_settings->getS16("block_send_optimize_distance");
	far_map_allow_generate = false;

	// Won't send anything if already sending enough
	if (client->m_blocks_sending.size() >= max_simul_sends_setting) {
		tracestream<<"AutosendCycle::start(): peer_id="<<client->peer_id
				<<": client->m_blocks_sending.size()="<<client->m_blocks_sending.size()
				<<" > max_simul_sends_setting="<<max_simul_sends_setting<<std::endl;
		disabled = true;
		return;
	}

	tracestream<<"AutosendCycle::start(): peer_id="<<client_->peer_id
			<<": Starting normally"<<std::endl;

	// Use camera position as the center for searching blocks
	// Camera position and direction
	camera_p = sao->getEyePosition();
	camera_dir = v3f(0,0,1);
	camera_dir.rotateYZBy(sao->getLookPitch());
	camera_dir.rotateXZBy(sao->getRotation().Y);

	// Get the player's speed
	// If the player is attached, get the velocity from the attached object
	// (In practice the player walks at around BS*4/second and fast speed is at
	// around BS*16/second, but these of course depend on the game and mods.)
	LuaEntitySAO *lsao = getAttachedObject(sao, env);
	const v3f &player_velocity = lsao? lsao->getVelocity() : player->getSpeed();

	// Get player position and figure out a good focus point for block selection
	v3f player_velocity_dir(0,0,0);
	if(player_velocity.getLength() > 1.0f * BS)
		player_velocity_dir = player_velocity / player_velocity.getLength();

	// Predict one block into player's movement direction
	v3f camera_p_predicted = camera_p + player_velocity_dir * MAP_BLOCKSIZE * BS;

	// Focus point is at predicted camera position
	focus_point_nodepos = floatToInt(camera_p_predicted, BS);
	focus_point = getNodeBlockPos(focus_point_nodepos);

	// If focus point has changed to a different MapBlock, reset radius value
	// for iterating from zero again
	if (alg->m_last_focus_point != focus_point) {
		alg->m_cycle->mapblock.nearest_unsent_d = 0;
		alg->m_cycle->farblock.nearest_unsent_d = 0;
		alg->m_last_focus_point = focus_point;
	}
	// If camera has turned considerably, reset radius value for iterating from
	// zero again (has to be done because of FOV dependence of some things)
	if (alg->m_last_camera_dir.dotProduct(camera_dir) < 0.8f) {
		alg->m_cycle->mapblock.nearest_unsent_d = 0;
		alg->m_cycle->farblock.nearest_unsent_d = 0;
		alg->m_last_camera_dir = camera_dir;
	}

	// Derived settings
	max_mapblock_send_distance = std::min(adjustDist(
			max_mapblock_send_distance_setting, alg->m_fov), alg->m_radius_map);
	max_mapblock_generate_distance = std::min(adjustDist(
			max_mapblock_generate_distance_setting, alg->m_fov), alg->m_radius_map);
	max_send_distance_bs = (float)alg->m_radius_map * BS * MAP_BLOCKSIZE;

	// FOV
	fov = alg->m_fov; // Begin with client's reported FOV

	// If the player not using the zoom function and is moving fast, replace
	// camera direction with the player's movement direction to make sure the
	// right stuff is being sent. Also replace the FOV value with a static one.
	// When moving fast, the real camera direction and FOV are worthless.
	if(sao->getZoomFOV() < 0.001f && player_velocity.getLength() > 10.0f * BS){
		fov = 1.72f;
		camera_dir = player_velocity;
	}

	// Reduce the field of view when a player moves and looks forward.
	// limit max fov effect to 50%, 60% at 20n/s fly speed

	// cos(angle between velocity and camera) * |velocity|
	// Limit to 0.0f in case player moves backwards.
	f32 dot = rangelim(camera_dir.dotProduct(player_velocity), 0.0f, 300.0f);

	fov = fov / (1 + dot / 300.0f);

	// Reset periodically to workaround possible glitches due to whatever
	// reasons (this is somewhat guided by just heuristics, after all)
	if (alg->m_nearest_unsent_reset_timer > 20.0) {
		alg->m_nearest_unsent_reset_timer = 0;
		alg->m_cycle->mapblock.nearest_unsent_d = 0;
		alg->m_cycle->farblock.nearest_unsent_d = 0;
	}

	// MapBlock search iteration
	// Out-of-FOV distance limit is decreased when limit is enabled
	//s16 mb_fov_limit_activation_distance = 1; // This is the classic behavior
	s16 mb_fov_limit_activation_distance = mapblock.fov_limit_enabled ?
			max_mapblock_send_distance / 5 : max_mapblock_send_distance;
	mapblock.start(max_mapblock_send_distance, mb_fov_limit_activation_distance);

	// FarBlock search iteration
	// Out-of-FOV distance limit is always disabled
	// TODO: Enable later
	//s16 fb_fov_limit_activation_distance = alg->m_radius_far;
	//farblock.start(alg->m_radius_far, fb_fov_limit_activation_distance);
	// TODO: Remove once farblocks are enabled. This allows the system to sleep
	// when no mapblocks were found.
	farblock.nothing_to_send_pause_timer = 5.0;
}

static void AutosendCycleEmergeCallback(v3s16 bp, EmergeAction action, void *param)
{
	AutosendCycle *cycle = (AutosendCycle*)param;
	tracestream<<"AutosendCycleEmergeCallback(): peer_id="
				<<cycle->client->peer_id<<": bp="<<PP(bp)
				<<", action="<<(int)action<<std::endl;

	if(action == EMERGE_FROM_MEMORY || action == EMERGE_FROM_DISK ||
			action == EMERGE_GENERATED){
		// Add to shortcut queue
		cycle->finished_mapblock_emerges.push_back(bp);
	}
}

AutosendCycle::MapBlockAnalysis AutosendCycle::checkMapBlock(const v3s16 &p)
{
	// Don't go over hard map limits
	if (blockpos_over_max_limit(p)) {
		/*dstream<<"AutosendMap: "<<wms.describe()
				<<": continue: over limit"<<std::endl;*/
		return AutosendCycle::MapBlockAnalysis::NOT_IN_VIEW;
	}

	WantedMapSend wms(WMST_MAPBLOCK, p);

	// Don't send blocks that are currently being transferred
	if (client->m_blocks_sending.count(wms)) {
		/*dstream<<"AutosendMap: "<<wms.describe()
				<<": continue: num sending"<<std::endl;*/
		return AutosendCycle::MapBlockAnalysis::ALREADY_SENT;
	}

	// TODO: Remove
	/*// Limit the generating area vertically to 2/3
	if(abs(p.Y - focus_point.Y) >
			max_mapblock_generate_distance - max_mapblock_generate_distance / 3)
		allow_generate = false;*/

	// TODO: Remove
	/*// Limit the send area vertically to 1/2
	if (abs(p.Y - focus_point.Y) > max_block_send_distance / 2)
		continue;*/

	f32 distance_bs;
	if (mapblock.d >= mapblock.fov_limit_activation_distance) {
		// Don't generate or send if not in sight
		if(isBlockInSight(p, camera_p, camera_dir, fov,
				max_send_distance_bs, &distance_bs) == false)
		{
			/*dstream<<"AutosendMap: "<<wms.describe()
					<<": continue: not in sight"<<std::endl;*/
			return AutosendCycle::MapBlockAnalysis::NOT_IN_VIEW;
		}
	} else {
		// Calculate block distance without FOV check
		v3s16 blockpos_nodes = p * MAP_BLOCKSIZE;
		v3f blockpos_center(
				blockpos_nodes.X * BS + MAP_BLOCKSIZE/2 * BS,
				blockpos_nodes.Y * BS + MAP_BLOCKSIZE/2 * BS,
				blockpos_nodes.Z * BS + MAP_BLOCKSIZE/2 * BS
		);
		v3f blockpos_relative = blockpos_center - camera_p;
		distance_bs = blockpos_relative.getLength();
		// Don't generate or send if not in sight
		if (distance_bs > max_send_distance_bs) {
			/*dstream<<"AutosendMap: "<<wms.describe()
					<<": continue: distance_bs: "<<distance_bs<<std::endl;*/
			return AutosendCycle::MapBlockAnalysis::NOT_IN_VIEW;
		}
	}

	// Don't send blocks that have already been sent
	std::map<v3s16, time_t>::const_iterator
			blocks_sent_i = client->m_mapblocks_sent.find(wms.p);
	if (blocks_sent_i != client->m_mapblocks_sent.end()){
		if (client->m_blocks_updated_since_last_send.count(wms) == 0) {
			/*dstream<<"AutosendMap: "<<wms.describe()
					<<": Already sent and not updated"<<std::endl;*/
			return AutosendCycle::MapBlockAnalysis::ALREADY_SENT;
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
	bool surely_not_generated = false;
	if(block != NULL)
	{
		// Reset usage timer, this block will be of use in the future.
		block->resetUsageTimer();

		// Block is dummy if data doesn't exist. A dummy block has been
		// confirmed to be not found on disk and to not having been
		// generated.
		if(block->isDummy())
			surely_not_found_on_disk = true;

		if(!block->isGenerated())
			surely_not_generated = true;

		/*
			If block is not close, don't send it unless it is near
			ground level.

			Block is near ground level if night-time mesh
			differs from day-time mesh.
		*/
		if (mapblock.d >= block_send_optimize_distance_setting) {
			if (!block->getIsUnderground() && !block->getDayNightDiff())
				return AutosendCycle::MapBlockAnalysis::NOT_IN_VIEW;
		}
	}

	// If doing occlusion culling on the server:
	// If a block isn't found on disk, don't proceed to generate or load
	// it if it's occluded
	if (server_side_occlusion_culling_setting
			&& mapblock.d >= block_send_optimize_distance_setting) {
		// Make a dummy block detached from the Map so that we can check
		// occlusion of any block regardless of whether it exists or not
		// TODO: Make version of Map::isBlockOccluded that doesn't need
		//       an actual MapBlock
		MapBlock dummyblock(NULL, p, NULL, true);
		bool occ = env->getMap().isBlockOccluded(&dummyblock, focus_point_nodepos);
		tracestream<<"AutosendCycle::suggestNextMapBlock(): peer_id="
				<<client->peer_id<<": "<<PP(p)
				<<": isBlockOccluded() = "<<occ<<std::endl;
		if (occ)
			return AutosendCycle::MapBlockAnalysis::NOT_IN_VIEW;
	}

	if(surely_not_generated)
		return AutosendCycle::MapBlockAnalysis::IN_VIEW_BUT_NOT_GENERATED;

	if(!block || surely_not_found_on_disk || surely_not_generated)
		return AutosendCycle::MapBlockAnalysis::IN_VIEW_BUT_NOT_LOADED;

	return AutosendCycle::MapBlockAnalysis::IN_VIEW_AND_LOADED;
}

WMSSuggestion AutosendCycle::suggestNextMapBlock(EmergeManager *emerge)
{
	// Use shortcut queue first
	// TODO: Somehow keep track and check that client's position hasn't changed
	//       by a huge amount between making the queue and now reading from it
	//       (generally it should be a short time). Maybe just drop queue
	//       elements based on their age, eg. by a 3 second timeout or so.
	while(!finished_mapblock_emerges.empty()){
		// Pop position from queue
		v3s16 p = finished_mapblock_emerges.front();
		finished_mapblock_emerges.pop_front();

		// Check if this MapBlock is relevant for sending to the client and
		// return it if so. Otherwise just ignore it.
		AutosendCycle::MapBlockAnalysis a = checkMapBlock(p);
		if(a == AutosendCycle::MapBlockAnalysis::IN_VIEW_AND_LOADED){
			tracestream<<"AutosendCycle::suggestNextMapBlock(): peer_id="
					<<client->peer_id<<": Suggesting finished emerge: "
					<<PP(p)<<std::endl;
			WantedMapSend wms(WMST_MAPBLOCK, p);
			return WMSSuggestion(wms);
		}
	}

	// Then do actual algorithm
	for (; mapblock.d <= mapblock.d_max; mapblock.d++) {
		tracestream<<"AutosendCycle::suggestNextMapBlock(): peer_id="
				<<client->peer_id<<": mapblock.d="<<mapblock.d<<std::endl;

		// Get the border/face dot coordinates of a mapblock.d-"radiused" box
		std::vector<v3s16> face_ps = FacePositionCache::getFacePositions(mapblock.d);
		// Continue from the last mapblock.i unless it was reset by something
		for (; mapblock.i < face_ps.size(); mapblock.i++) {

			// Calculate this thing
			u16 max_simultaneous_block_sends =
					figure_out_max_simultaneous_block_sends(
							max_simul_sends_setting,
							client->m_time_from_building,
							time_from_building_limit_s,
							mapblock.d);

			// Don't select too many blocks for sending
			if (client->getSendingCount() >= max_simultaneous_block_sends) {
				tracestream<<"AutosendCycle::suggestNextMapBlock(): peer_id="
						<<client->peer_id<<": "
						<<"(client->getSendingCount()="<<client->getSendingCount()
						<<" >= max_simultaneous_block_sends="<<max_simultaneous_block_sends
						<<") (client->m_time_from_building="<<client->m_time_from_building
						<<")-> Returning nothing"<<std::endl;
				// Shouldn't suggest anything (why is the server even asking us?)
				return WMSSuggestion();
			}

			v3s16 p = face_ps[mapblock.i] + focus_point;

			// Analyze block

			AutosendCycle::MapBlockAnalysis analysis = checkMapBlock(p);

			// Return this block if it's good for sending
			if(analysis == AutosendCycle::MapBlockAnalysis::IN_VIEW_AND_LOADED){
				tracestream<<"AutosendCycle::suggestNextMapBlock(): peer_id="
						<<client->peer_id<<": Suggesting: "
						<<PP(p)<<std::endl;
				WantedMapSend wms(WMST_MAPBLOCK, p);
				return WMSSuggestion(wms);
			}

			// If this block shouldn't be sent, continue searching
			if(analysis == AutosendCycle::MapBlockAnalysis::ALREADY_SENT ||
					analysis == AutosendCycle::MapBlockAnalysis::NOT_IN_VIEW){
				continue;
			}

			// If block has been marked to not exist on disk (dummy) or is
			// not generated and generating new ones is not wanted, skip block.
			bool allow_generate = mapblock.d <= max_mapblock_generate_distance;
			if(analysis == AutosendCycle::MapBlockAnalysis::IN_VIEW_BUT_NOT_GENERATED &&
					!allow_generate){
				//dstream<<"continue: not on disk, no generate"<<std::endl;
				// get next one.
				continue;
			}

			// Generating is wanted. Generate or load it if it's not loaded
			if(analysis == AutosendCycle::MapBlockAnalysis::IN_VIEW_BUT_NOT_LOADED)
			{
				// Queue the block for loading or generating
				u16 flags = 0;
				if (allow_generate)
					flags |= BLOCK_EMERGE_ALLOW_GEN;
				bool queue_full = !emerge->enqueueBlockEmergeEx(
						p, client->peer_id, flags, AutosendCycleEmergeCallback, this);
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

			// Continue searching
		}

		// Now reset i as we go to next d
		mapblock.i = 0;
	}

	tracestream<<"AutosendCycle::suggestNextMapBlock(): peer_id="
			<<client->peer_id<<": Nothing to suggest"<<std::endl;

	// Nothing to suggest
	return WMSSuggestion();
}

/*WMSSuggestion AutosendCycle::getNextBlock(
		EmergeManager *emerge, ServerFarMap *far_map)*/
WMSSuggestion AutosendCycle::getNextBlock(EmergeManager *emerge)
{
	//dstream<<"AutosendCycle::getNextBlock()"<<std::endl;

	if (disabled)
		return WMSSuggestion();

	// Get MapBlock and FarBlock suggestions

	WMSSuggestion suggested_mb;
	if (alg->m_client->m_mapblocks_sent.size() < alg->m_max_total_mapblocks) {
		suggested_mb = suggestNextMapBlock(emerge);
	}

	WMSSuggestion suggested_fb;
	// Not suggesting FarBlocks (not supported by this version)

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
	max_send_distance = max_send_distance_; // Needed by finish()
	fov_limit_activation_distance = fov_limit_activation_distance_;
	i = 0;

	// We will start from a radius that still has unsent MapBlocks
	d_start = nearest_unsent_d >= 0 ? nearest_unsent_d : 0;
	d = d_start;

	// Don't loop very much at a time. This function is called each server tick
	// so just a few steps will work fine (+2 is 3 steps per call).
	d_max = 0;
	if (d_start < 5)
		d_max = d_start + 3;
	else if (d_max < 8)
		d_max = d_start + 2;
	else
		d_max = d_start + 1; // These iterations start to be rather heavy

	if (d_max > max_send_distance)
		d_max = max_send_distance;

	tracestream<<"AutosendCycle::Search::start(): "
			<<"id="<<log_id<<"/"<<log_name<<": "
			<<"d_start="<<d_start
			<<", d_max="<<d_max
			<<", max_send_distance="<<max_send_distance
			<<", fov_limit_activation_distance="<<fov_limit_activation_distance
			<<std::endl;

	// Keep track of... things. We need these in order to figure out where to
	// continue iterating on the next call to this function.
	// TODO: Put these in the class
	nearest_emergequeued_d = INT32_MAX;
	nearest_emergefull_d = INT32_MAX;
	nearest_sendqueued_d = INT32_MAX;
}

void AutosendCycle::Search::finish(bool enable_fov_toggle)
{
	if(nearest_emergequeued_d == INT32_MAX &&
			nearest_emergefull_d == INT32_MAX &&
			nearest_sendqueued_d == INT32_MAX){
		tracestream << "AutosendCycle::Search::finish(): "
				<<"id="<<log_id<<"/"<<log_name<<":"
				<<" Nothing queued"<<tracestream<<std::endl;
	} else {
		tracestream << "AutosendCycle::Search::finish(): "
				<<"id="<<log_id<<"/"<<log_name<<":";
		tracestream<<" Stuff queued:";
		if(nearest_emergequeued_d != INT32_MAX)
			tracestream<<" nearest_emergequeued_d="<<nearest_emergequeued_d;
		if(nearest_emergefull_d != INT32_MAX)
			tracestream<<" nearest_emergefull_d="<<nearest_emergefull_d;
		if(nearest_sendqueued_d != INT32_MAX)
			tracestream<<" nearest_sendqueued_d="<<nearest_sendqueued_d;
		tracestream<<std::endl;
	}

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
		tracestream << "AutosendCycle::Search::finish(): "
				<<"id="<<log_id<<"/"<<log_name<<": "
				<<"-> Processed result may be found later at d="<<nearest_unsent_d
				<<std::endl;

		// If nothing has been sent in a moment, indicating that the emerge
		// thread is not finding anything on disk anymore, start a fresh
		// pass without the FOV limit.
		if (enable_fov_toggle && nothing_to_send_timer >= 3.0f) {
			if(fov_limit_enabled){
				verbosestream << "AutosendCycle::Search::finish(): "
						<<"id="<<log_id<<"/"<<log_name<<": "
						<<"-> Nothing to send; Disabling FOV limit"<<std::endl;

				fov_limit_enabled = false;
			} else {
				verbosestream << "AutosendCycle::Search::finish(): "
						<<"id="<<log_id<<"/"<<log_name<<": "
						<<"-> Nothing to send (no FOV limit); starting short sleep"
						<<std::endl;

				// Start from the beginning after a very short idle delay.
				// During this delay, some map data may be loaded or generated.
				// Start is done with FOV limiting enabled again.
				fov_limit_enabled = true;
				nothing_to_send_pause_timer = 0.2;
			}
			// Have to be reset in order to not trigger this immediately again
			nothing_to_send_timer = 0.0f;
		}
	} else if(searched_full_range) {
		// We iterated all the way through to the end of the send radius, if you
		// can believe that.
		if (enable_fov_toggle && fov_limit_enabled) {
			tracestream << "AutosendCycle::Search::finish(): "
					<<"id="<<log_id<<"/"<<log_name<<": "
					<<"-> Iterated all the way through"
					<<"; Disabling FOV limit"<<std::endl;

			// Do a second pass with FOV limiting disabled
			fov_limit_enabled = false;
		} else {
			tracestream << "AutosendCycle::Search::finish(): "
					<<"id="<<log_id<<"/"<<log_name<<": "
					<<"-> Initiating idle sleep"<<std::endl;

			// Start from the beginning after a short idle delay. During
			// this delay, some map data may be loaded or generated.
			// Start is done with FOV limiting enabled again.
			// TODO: The pause timer should be reset also when the emergethread
			// has generated or loaded something within the area the client is
			// looking for. This hasn't been implemented yet, so the pause will
			// always be fully waited before anything is checked again. For this
			// reason the pause is relatively short.
			fov_limit_enabled = true;
			nothing_to_send_pause_timer = 1.0;
		}
	} else {
		tracestream << "AutosendCycle::Search::finish(): "
				<<"id="<<log_id<<"/"<<log_name<<": "
				<<"-> Continuing at d="<<nearest_unsent_d<<std::endl;
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
	//farblock.finish(enable_fov_toggle); // TODO: Enable later
}

AutosendAlgorithm::AutosendAlgorithm(RemoteClient *client):
	m_client(client),
	m_cycle(new AutosendCycle()),
	m_radius_map(0),
	m_radius_far(0),
	m_far_weight(3.0f),
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

/*WMSSuggestion AutosendAlgorithm::getNextBlock(
		EmergeManager *emerge, ServerFarMap *far_map)*/
WMSSuggestion AutosendAlgorithm::getNextBlock(EmergeManager *emerge)
{
	return m_cycle->getNextBlock(emerge);
}

void AutosendAlgorithm::resetMapblockSearchRadius(const v3s16 &mb_p)
{
	s16 reset_to_d = 0; // Radius value in MapBlocks to reset the search to

	// Get distance to player and use it as the reset value in order to not scan
	// unnecessarily close
	RemotePlayer *player = m_client->m_env->getPlayer(m_client->peer_id);
	PlayerSAO *sao = player ? player->getPlayerSAO() : NULL;
	if (sao) {
		v3f camera_pf_nodes = sao->getEyePosition() / BS;
		v3s16 mb_nodes = mb_p * MAP_BLOCKSIZE;
		v3f mb_pf_nodes(mb_nodes.X, mb_nodes.Y, mb_nodes.Z);
		float distance_nodes = (mb_pf_nodes - camera_pf_nodes).getLength();
		reset_to_d = distance_nodes / MAP_BLOCKSIZE;
	}

	/*dstream<<"AutosendAlgorithm::resetMapblockSearchRadius(): peer_id="
			<<m_client->peer_id<<", reset_to_d="<<reset_to_d<<std::endl;*/

	if (m_cycle->mapblock.d == reset_to_d){
		m_cycle->mapblock.i = 0;
	}

	if (m_cycle->mapblock.d > reset_to_d){
		m_cycle->mapblock.d = reset_to_d;
		m_cycle->mapblock.fov_limit_enabled = true;
	}

	if (m_cycle->mapblock.nearest_unsent_d > reset_to_d){
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

