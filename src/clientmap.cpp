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

#include "clientmap.h"
#include "client.h"
#include "mapblock_mesh.h"
#include <IMaterialRenderer.h>
#include <matrix4.h>
#include "log.h"
#include "mapsector.h"
#include "nodedef.h"
#include "mapblock.h"
#include "profiler.h"
#include "settings.h"
#include "camera.h"               // CameraModes
#include "far_map.h"              // For reporting what is being rendered by us
#include "util/mathconstants.h"
#include <algorithm>

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

static bool isOccluded(Map *map, v3s16 p0, v3s16 p1, float step, float stepfac,
		float start_off, float end_off, u32 needed_count, INodeDefManager *nodemgr);

struct DrawListUpdate
{
	ClientMap *cmap;
	video::IVideoDriver* driver;

	std::vector<v2s16> sector_positions;
	size_t next_sector_i;

	v3f camera_position;
	v3f camera_direction;
	f32 camera_fov;

	v3s16 cam_pos_nodes;
	v3s16 p_blocks_min;
	v3s16 p_blocks_max;

	// BlockAreaBitmap<bool> for reporting to FarMap
	VoxelArea nrb_area;
	BlockAreaBitmap<bool> normally_rendered_blocks;

	// Number of blocks in rendering range
	u32 blocks_in_range;
	// Number of blocks occlusion culled
	u32 blocks_occlusion_culled;
	// Number of blocks in rendering range but don't have a mesh
	u32 blocks_in_range_without_mesh;
	// Blocks that had mesh that would have been drawn according to
	// rendering range (if max blocks limit didn't kick in)
	u32 blocks_would_have_drawn;
	// Blocks that were drawn and had a mesh
	u32 blocks_drawn;
	// Blocks which had a corresponding meshbuffer for this pass
	//u32 blocks_had_pass_meshbuf;
	// Blocks from which stuff was actually drawn
	//u32 blocks_without_stuff;
	// Distance to farthest drawn block
	float farthest_drawn;

	// Result is collected into here
	std::map<v3s16, MapBlock*> drawlist;
	bool is_finished;

	DrawListUpdate(ClientMap *cmap, video::IVideoDriver* driver);
	void process(u32 max_us, u32 max_frames_per_update);
};

DrawListUpdate::DrawListUpdate(ClientMap *cmap, video::IVideoDriver* driver):
	cmap(cmap),
	driver(driver),
	next_sector_i(0),
	blocks_in_range(0),
	blocks_occlusion_culled(0),
	blocks_in_range_without_mesh(0),
	blocks_would_have_drawn(0),
	blocks_drawn(0),
	farthest_drawn(0),
	is_finished(false)
{
	g_profiler->add("CM: DrawListUpdate count", 1);
	ScopeProfiler sp(g_profiler, "CM: DrawListUpdate processing", SPT_AVG);

	camera_position = cmap->m_camera_position;
	camera_direction = cmap->m_camera_direction;
	camera_fov = cmap->m_camera_fov;

	// Use a higher fov to accomodate faster camera movements.
	// Blocks are cropped better when they are drawn.
	// Or maybe they aren't? Well whatever.
	camera_fov *= 1.2;

	cam_pos_nodes = floatToInt(camera_position, BS);
	cmap->getBlocksInViewRange(cam_pos_nodes, &p_blocks_min, &p_blocks_max);

	// Set up a BlockAreaBitmap<bool> for reporting to Map
	nrb_area = VoxelArea(p_blocks_min, p_blocks_max);
	normally_rendered_blocks.reset(nrb_area);

	// Sectors might be deleted and created while creating the new drawlist so
	// collect the keys of the current ones into this vector
	sector_positions.reserve(cmap->m_sectors.size());
	for (std::map<v2s16, MapSector*>::iterator si = cmap->m_sectors.begin();
			si != cmap->m_sectors.end(); ++si) {
		sector_positions.push_back(si->first);
	}
}

void DrawListUpdate::process(u32 max_us, u32 max_frames_per_update)
{
	if (is_finished)
		return;

	ScopeProfiler sp(g_profiler, "CM: DrawListUpdate processing", SPT_AVG);

	INodeDefManager *nodemgr = cmap->m_gamedef->ndef();

	// Always get the most recent camera position and direction; this way we
	// halve the amount of occlusion culling glitches when the camera is moving
	// fast
	camera_position = cmap->m_camera_position;
	camera_direction = cmap->m_camera_direction;

	const u32 t0_us = getTime(PRECISION_MICRO);
	const u32 t1_us_max = t0_us + max_us; // Limit amount of processing per call

	for (u32 num_sectors_processed = 0; ; num_sectors_processed++) {
		//dstream<<"DrawListUpdate next_sector_i="<<next_sector_i<<std::endl;

		if (next_sector_i >= sector_positions.size()) {
			is_finished = true;
			break;
		}

		// Allow processing limits when we have managed to process a reasonable
		// amount of sectors. This ensures that the draw list update does not
		// take too many frames to complete.
		if (num_sectors_processed >= sector_positions.size() / max_frames_per_update) {
			// Limit amount of processing per call
			const u32 t1_us = getTime(PRECISION_MICRO);
			if (t1_us >= t1_us_max)
				break;
		}

		v2s16 sp = sector_positions[next_sector_i++];
		std::map<v2s16, MapSector*>::iterator si = cmap->m_sectors.find(sp);
		if (si == cmap->m_sectors.end())
			continue;
		MapSector *sector = si->second;

		if (cmap->m_control.range_all == false) {
			if (sp.X < p_blocks_min.X || sp.X > p_blocks_max.X ||
					sp.Y < p_blocks_min.Z || sp.Y > p_blocks_max.Z)
				continue;
		}

		MapBlockVect sectorblocks;
		sector->getBlocks(sectorblocks);

		/*
			Loop through blocks in sector
		*/

		u32 sector_blocks_drawn = 0;

		for (MapBlockVect::iterator i = sectorblocks.begin();
				i != sectorblocks.end(); ++i) {
			MapBlock *block = *i;

			/*
				Compare block position to camera position, skip
				if not seen on display
			*/

			if (block->mesh != NULL)
				block->mesh->updateCameraOffset(cmap->m_camera_offset);

			float range = 100000 * BS;
			if (cmap->m_control.range_all == false)
				range = cmap->m_control.wanted_range * BS;

			// TODO: On large view ranges don't use occlusion culling or even frustum
			//       culling on nearby MapBlocks because drawlist updates are slower

			float d = 0.0;
			if (!isBlockInSight(block->getPos(), camera_position,
					camera_direction, camera_fov, range, &d))
				continue;

			// This is ugly (spherical distance limit?)
			/*if(cmap->m_control.range_all == false &&
					d - 0.5*BS*MAP_BLOCKSIZE > range)
				continue;*/

			blocks_in_range++;

			/*
				Ignore if mesh doesn't exist
			*/
			{
				//MutexAutoLock lock(block->mesh_mutex);

				if (block->mesh == NULL) {
					blocks_in_range_without_mesh++;
					normally_rendered_blocks.set(block->getPos(), true);
					continue;
				}
			}

			/*
				Occlusion culling
			*/

			// No occlusion culling when free_move is on and camera is
			// inside ground
			bool occlusion_culling_enabled = true;
			if (g_settings->getBool("free_move")) {
				MapNode n = cmap->getNodeNoEx(cam_pos_nodes);
				if (n.getContent() == CONTENT_IGNORE ||
						nodemgr->get(n).solidness == 2)
					occlusion_culling_enabled = false;
			}

			v3s16 cpn = block->getPos() * MAP_BLOCKSIZE;
			cpn += v3s16(MAP_BLOCKSIZE / 2, MAP_BLOCKSIZE / 2, MAP_BLOCKSIZE / 2);
			float step = BS * 1;
			float stepfac = 1.1;
			float startoff = BS * 1;
			float endoff = -BS*MAP_BLOCKSIZE * 1.42 * 1.42;
			v3s16 spn = cam_pos_nodes + v3s16(0, 0, 0);
			s16 bs2 = MAP_BLOCKSIZE / 2 + 1;
			u32 needed_count = 1;
			if (occlusion_culling_enabled &&
					isOccluded(cmap, spn, cpn + v3s16(0, 0, 0),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(bs2,bs2,bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(bs2,bs2,-bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(bs2,-bs2,bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(bs2,-bs2,-bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(-bs2,bs2,bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(-bs2,bs2,-bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(-bs2,-bs2,bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr) &&
					isOccluded(cmap, spn, cpn + v3s16(-bs2,-bs2,-bs2),
						step, stepfac, startoff, endoff, needed_count, nodemgr)) {
				blocks_occlusion_culled++;
				continue;
			}

			// This block is in range. Reset usage timer.
			block->resetUsageTimer();

			// Limit block count in case of a sudden increase
			blocks_would_have_drawn++;
			if (blocks_drawn >= cmap->m_control.wanted_max_blocks &&
					!cmap->m_control.range_all &&
					d > cmap->m_control.wanted_range * BS)
				continue;

			// Add to set
			block->refGrab();
			drawlist[block->getPos()] = block;

			sector_blocks_drawn++;
			blocks_drawn++;
			if (d / BS > farthest_drawn)
				farthest_drawn = d / BS;

			normally_rendered_blocks.set(block->getPos(), true);

		} // foreach sectorblocks
	}

	if (!is_finished)
		return;

	//dstream<<"DrawListUpdate finished"<<std::endl;

	// Dereference MapBlocks on old drawlist
	for (std::map<v3s16, MapBlock*>::iterator i = cmap->m_drawlist.begin();
			i != cmap->m_drawlist.end(); ++i) {
		MapBlock *block = i->second;
		block->refDrop();
	}

	// Set new drawlist
	cmap->m_drawlist = drawlist;

	FarMap *fm = cmap->m_client->getFarMap();
	if (fm) {
		fm->reportNormallyRenderedBlocks(normally_rendered_blocks);
	}

	cmap->m_control.blocks_would_have_drawn = blocks_would_have_drawn;
	cmap->m_control.blocks_drawn = blocks_drawn;
	cmap->m_control.farthest_drawn = farthest_drawn;

	g_profiler->avg("CM: blocks in range", blocks_in_range);
	g_profiler->avg("CM: blocks occlusion culled", blocks_occlusion_culled);
	if (blocks_in_range != 0)
		g_profiler->avg("CM: blocks in range without mesh (frac)",
				(float)blocks_in_range_without_mesh / blocks_in_range);
	g_profiler->avg("CM: blocks drawn", blocks_drawn);
	g_profiler->avg("CM: farthest drawn", farthest_drawn);
	g_profiler->avg("CM: wanted max blocks", cmap->m_control.wanted_max_blocks);
}

ClientMap::ClientMap(
		Client *client,
		IGameDef *gamedef,
		MapDrawControl &control,
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id
):
	Map(dout_client, gamedef),
	scene::ISceneNode(parent, mgr, id),
	m_client(client),
	m_control(control),
	m_camera_position(0,0,0),
	m_camera_direction(0,0,1),
	m_camera_fov(M_PI),
	m_update(NULL),
	m_mapblocks_exist_up_to_d(0),
	m_mapblocks_exist_up_to_d_reset_counter(0)
{
	m_box = aabb3f(-BS*1000000,-BS*1000000,-BS*1000000,
			BS*1000000,BS*1000000,BS*1000000);

	/* TODO: Add a callback function so these can be updated when a setting
	 *       changes.  At this point in time it doesn't matter (e.g. /set
	 *       is documented to change server settings only)
	 *
	 * TODO: Local caching of settings is not optimal and should at some stage
	 *       be updated to use a global settings object for getting thse values
	 *       (as opposed to the this local caching). This can be addressed in
	 *       a later release.
	 */
	m_cache_trilinear_filter  = g_settings->getBool("trilinear_filter");
	m_cache_bilinear_filter   = g_settings->getBool("bilinear_filter");
	m_cache_anistropic_filter = g_settings->getBool("anisotropic_filter");
}

ClientMap::~ClientMap()
{
	/*MutexAutoLock lock(mesh_mutex);

	if(mesh != NULL)
	{
		mesh->drop();
		mesh = NULL;
	}*/

	delete m_update;
}

MapSector * ClientMap::emergeSector(v2s16 p2d)
{
	DSTACK(FUNCTION_NAME);
	// Check that it doesn't exist already
	try{
		return getSectorNoGenerate(p2d);
	}
	catch(InvalidPositionException &e)
	{
	}

	// Create a sector
	ClientMapSector *sector = new ClientMapSector(this, p2d, m_gamedef);

	{
		//MutexAutoLock lock(m_sector_mutex); // Bulk comment-out
		m_sectors[p2d] = sector;
	}

	return sector;
}

void ClientMap::OnRegisterSceneNode()
{
	if(IsVisible)
	{
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
	}

	ISceneNode::OnRegisterSceneNode();
}

static bool isOccluded(Map *map, v3s16 p0, v3s16 p1, float step, float stepfac,
		float start_off, float end_off, u32 needed_count, INodeDefManager *nodemgr)
{
	float d0 = (float)BS * p0.getDistanceFrom(p1);
	v3s16 u0 = p1 - p0;
	v3f uf = v3f(u0.X, u0.Y, u0.Z) * BS;
	uf.normalize();
	v3f p0f = v3f(p0.X, p0.Y, p0.Z) * BS;
	u32 count = 0;
	for(float s=start_off; s<d0+end_off; s+=step){
		v3f pf = p0f + uf * s;
		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNodeNoEx(p);
		bool is_transparent = false;
		const ContentFeatures &f = nodemgr->get(n);
		if(f.solidness == 0)
			is_transparent = (f.visual_solidness != 2);
		else
			is_transparent = (f.solidness != 2);
		if(!is_transparent){
			if(count == needed_count)
				return true;
			count++;
		}
		step *= stepfac;
	}
	return false;
}

void ClientMap::getBlocksInViewRange(v3s16 cam_pos_nodes, 
		v3s16 *p_blocks_min, v3s16 *p_blocks_max)
{
	v3s16 box_nodes_d = m_control.wanted_range * v3s16(1, 1, 1);
	// Define p_nodes_min/max as v3s32 because 'cam_pos_nodes -/+ box_nodes_d'
	// can exceed the range of v3s16 when a large view range is used near the
	// world edges.
	v3s32 p_nodes_min(
		cam_pos_nodes.X - box_nodes_d.X,
		cam_pos_nodes.Y - box_nodes_d.Y,
		cam_pos_nodes.Z - box_nodes_d.Z);
	v3s32 p_nodes_max(
		cam_pos_nodes.X + box_nodes_d.X,
		cam_pos_nodes.Y + box_nodes_d.Y,
		cam_pos_nodes.Z + box_nodes_d.Z);
	// Take a fair amount as we will be dropping more out later
	// Umm... these additions are a bit strange but they are needed.
	*p_blocks_min = v3s16(
			p_nodes_min.X / MAP_BLOCKSIZE - 3,
			p_nodes_min.Y / MAP_BLOCKSIZE - 3,
			p_nodes_min.Z / MAP_BLOCKSIZE - 3);
	*p_blocks_max = v3s16(
			p_nodes_max.X / MAP_BLOCKSIZE + 1,
			p_nodes_max.Y / MAP_BLOCKSIZE + 1,
			p_nodes_max.Z / MAP_BLOCKSIZE + 1);
}

void ClientMap::updateDrawList(video::IVideoDriver* driver)
{
	if (m_update == NULL) {
		m_update = new DrawListUpdate(this, driver);
	}

	m_update->process(1000, 6);

	if (m_update->is_finished) {
		delete m_update;
		m_update = NULL;
	}
}

void ClientMap::updateDrawListImmediately(video::IVideoDriver* driver)
{
	// This can be called when the camera offset changes, in which case we have
	// to start over and do a full drawlist update right now; otherwise
	// completely wrong things will be rendered.

	// Cancel and delete old update
	if (m_update) {
		// Dereference MapBlocks collected by old update
		for (std::map<v3s16, MapBlock*>::iterator i = m_update->drawlist.begin();
				i != m_update->drawlist.end(); ++i) {
			MapBlock *block = i->second;
			block->refDrop();
		}
		delete m_update;
		m_update = NULL;
	}

	// Dereference MapBlocks on old drawlist
	for (std::map<v3s16, MapBlock*>::iterator i = m_drawlist.begin();
			i != m_drawlist.end(); ++i) {
		MapBlock *block = i->second;
		block->refDrop();
	}

	// Do full new update
	m_update = new DrawListUpdate(this, driver);
	while (!m_update->is_finished) {
		m_update->process(0, 1);
	}
}

struct MeshBufList
{
	video::SMaterial m;
	std::vector<scene::IMeshBuffer*> bufs;
};

struct MeshBufListList
{
	std::vector<MeshBufList> lists;

	void clear()
	{
		lists.clear();
	}

	void add(scene::IMeshBuffer *buf)
	{
		for(std::vector<MeshBufList>::iterator i = lists.begin();
				i != lists.end(); ++i){
			MeshBufList &l = *i;
			video::SMaterial &m = buf->getMaterial();

			// comparing a full material is quite expensive so we don't do it if
			// not even first texture is equal
			if (l.m.TextureLayer[0].Texture != m.TextureLayer[0].Texture)
				continue;

			if (l.m == m) {
				l.bufs.push_back(buf);
				return;
			}
		}
		MeshBufList l;
		l.m = buf->getMaterial();
		l.bufs.push_back(buf);
		lists.push_back(l);
	}
};

void ClientMap::renderMap(video::IVideoDriver* driver, s32 pass)
{
	DSTACK(FUNCTION_NAME);

	bool is_transparent_pass = pass == scene::ESNRP_TRANSPARENT;

	std::string prefix;
	if (pass == scene::ESNRP_SOLID)
		prefix = "CM: solid: ";
	else
		prefix = "CM: transparent: ";

	/*
		Get time for measuring timeout.

		Measuring time is very useful for long delays when the
		machine is swapping a lot.
	*/
	int time1 = time(0);

	/*
		Get animation parameters
	*/
	float animation_time = m_client->getAnimationTime();
	int crack = m_client->getCrackLevel();
	u32 daynight_ratio = m_client->getEnv().getDayNightRatio();

	v3f camera_position = m_camera_position;
	v3f camera_direction = m_camera_direction;
	f32 camera_fov = m_camera_fov;

	/*
		Get all blocks and draw all visible ones
	*/

	v3s16 cam_pos_nodes = floatToInt(camera_position, BS);
	v3s16 p_blocks_min;
	v3s16 p_blocks_max;
	getBlocksInViewRange(cam_pos_nodes, &p_blocks_min, &p_blocks_max);

	u32 vertex_count = 0;
	u32 meshbuffer_count = 0;

	// For limiting number of mesh animations per frame
	u32 mesh_animate_count = 0;
	u32 mesh_animate_count_far = 0;

	// Blocks that were drawn and had a mesh
	u32 blocks_drawn = 0;
	// Blocks which had a corresponding meshbuffer for this pass
	u32 blocks_had_pass_meshbuf = 0;
	// Blocks from which stuff was actually drawn
	u32 blocks_without_stuff = 0;

	/*
		Draw the selected MapBlocks
	*/

	{
	ScopeProfiler sp(g_profiler, prefix + "drawing blocks", SPT_AVG);

	MeshBufListList drawbufs;

	for (std::map<v3s16, MapBlock*>::iterator i = m_drawlist.begin();
			i != m_drawlist.end(); ++i) {
		MapBlock *block = i->second;

		// If the mesh of the block happened to get deleted, ignore it
		if (block->mesh == NULL)
			continue;

		float d = 0.0;
		if (!isBlockInSight(block->getPos(), camera_position,
				camera_direction, camera_fov, 100000 * BS, &d))
			continue;

		// Mesh animation
		{
			//MutexAutoLock lock(block->mesh_mutex);
			MapBlockMesh *mapBlockMesh = block->mesh;
			assert(mapBlockMesh);
			// Pretty random but this should work somewhat nicely
			bool faraway = d >= BS * 50;
			//bool faraway = d >= m_control.wanted_range * BS;
			if (mapBlockMesh->isAnimationForced() || !faraway ||
					mesh_animate_count_far < (m_control.range_all ? 200 : 50)) {
				bool animated = mapBlockMesh->animate(faraway, animation_time,
					crack, daynight_ratio);
				if (animated)
					mesh_animate_count++;
				if (animated && faraway)
					mesh_animate_count_far++;
			} else {
				mapBlockMesh->decreaseAnimationForceTimer();
			}
		}

		/*
			Get the meshbuffers of the block
		*/
		{
			//MutexAutoLock lock(block->mesh_mutex);

			MapBlockMesh *mapBlockMesh = block->mesh;
			assert(mapBlockMesh);

			scene::IMesh *mesh = mapBlockMesh->getMesh();
			assert(mesh);

			u32 c = mesh->getMeshBufferCount();
			for (u32 i = 0; i < c; i++)
			{
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);

				buf->getMaterial().setFlag(video::EMF_TRILINEAR_FILTER, m_cache_trilinear_filter);
				buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, m_cache_bilinear_filter);
				buf->getMaterial().setFlag(video::EMF_ANISOTROPIC_FILTER, m_cache_anistropic_filter);

				const video::SMaterial& material = buf->getMaterial();
				video::IMaterialRenderer* rnd =
						driver->getMaterialRenderer(material.MaterialType);
				bool transparent = (rnd && rnd->isTransparent());
				if (transparent == is_transparent_pass) {
					if (buf->getVertexCount() == 0)
						errorstream << "Block [" << analyze_block(block)
							 << "] contains an empty meshbuf" << std::endl;
					drawbufs.add(buf);
				}
			}
		}
	}

	std::vector<MeshBufList> &lists = drawbufs.lists;

	int timecheck_counter = 0;
	for (std::vector<MeshBufList>::iterator i = lists.begin();
			i != lists.end(); ++i) {
		timecheck_counter++;
		if (timecheck_counter > 50) {
			timecheck_counter = 0;
			int time2 = time(0);
			if (time2 > time1 + 4) {
				infostream << "ClientMap::renderMap(): "
					"Rendering takes ages, returning."
					<< std::endl;
				return;
			}
		}

		MeshBufList &list = *i;

		driver->setMaterial(list.m);

		for (std::vector<scene::IMeshBuffer*>::iterator j = list.bufs.begin();
				j != list.bufs.end(); ++j) {
			scene::IMeshBuffer *buf = *j;
			driver->drawMeshBuffer(buf);
			vertex_count += buf->getVertexCount();
			meshbuffer_count++;
		}

	}
	} // ScopeProfiler

	// Log only on solid pass because values are the same
	if (pass == scene::ESNRP_SOLID) {
		g_profiler->avg("CM: animated meshes", mesh_animate_count);
		g_profiler->avg("CM: animated meshes (far)", mesh_animate_count_far);
	}

	g_profiler->avg(prefix + "vertices drawn", vertex_count);
	if (blocks_had_pass_meshbuf != 0)
		g_profiler->avg(prefix + "meshbuffers per block",
			(float)meshbuffer_count / (float)blocks_had_pass_meshbuf);
	if (blocks_drawn != 0)
		g_profiler->avg(prefix + "empty blocks (frac)",
			(float)blocks_without_stuff / blocks_drawn);

	/*infostream<<"renderMap(): is_transparent_pass="<<is_transparent_pass
			<<", rendered "<<vertex_count<<" vertices."<<std::endl;*/
}

static bool getVisibleBrightness(Map *map, v3f p0, v3f dir, float step,
		float step_multiplier, float start_distance, float end_distance,
		INodeDefManager *ndef, u32 daylight_factor, float sunlight_min_d,
		int *result, bool *sunlight_seen)
{
	int brightness_sum = 0;
	int brightness_count = 0;
	float distance = start_distance;
	dir.normalize();
	v3f pf = p0;
	pf += dir * distance;
	int noncount = 0;
	bool nonlight_seen = false;
	bool allow_allowing_non_sunlight_propagates = false;
	bool allow_non_sunlight_propagates = false;
	// Check content nearly at camera position
	{
		v3s16 p = floatToInt(p0 /*+ dir * 3*BS*/, BS);
		MapNode n = map->getNodeNoEx(p);
		if(ndef->get(n).param_type == CPT_LIGHT &&
				!ndef->get(n).sunlight_propagates)
			allow_allowing_non_sunlight_propagates = true;
	}
	// If would start at CONTENT_IGNORE, start closer
	{
		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNodeNoEx(p);
		if(n.getContent() == CONTENT_IGNORE){
			float newd = 2*BS;
			pf = p0 + dir * 2*newd;
			distance = newd;
			sunlight_min_d = 0;
		}
	}
	for(int i=0; distance < end_distance; i++){
		pf += dir * step;
		distance += step;
		step *= step_multiplier;

		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNodeNoEx(p);
		if(allow_allowing_non_sunlight_propagates && i == 0 &&
				ndef->get(n).param_type == CPT_LIGHT &&
				!ndef->get(n).sunlight_propagates){
			allow_non_sunlight_propagates = true;
		}
		if(ndef->get(n).param_type != CPT_LIGHT ||
				(!ndef->get(n).sunlight_propagates &&
					!allow_non_sunlight_propagates)){
			nonlight_seen = true;
			noncount++;
			if(noncount >= 4)
				break;
			continue;
		}
		if(distance >= sunlight_min_d && *sunlight_seen == false
				&& nonlight_seen == false)
			if(n.getLight(LIGHTBANK_DAY, ndef) == LIGHT_SUN)
				*sunlight_seen = true;
		noncount = 0;
		brightness_sum += decode_light(n.getLightBlend(daylight_factor, ndef));
		brightness_count++;
	}
	*result = 0;
	if(brightness_count == 0)
		return false;
	*result = brightness_sum / brightness_count;
	/*std::cerr<<"Sampled "<<brightness_count<<" points; result="
			<<(*result)<<std::endl;*/
	return true;
}

int ClientMap::getBackgroundBrightness(float max_d, u32 daylight_factor,
		int oldvalue, bool *sunlight_seen_result)
{
	const bool debugprint = false;
	INodeDefManager *ndef = m_gamedef->ndef();
	static v3f z_directions[50] = {
		v3f(-100, 0, 0)
	};
	static f32 z_offsets[sizeof(z_directions)/sizeof(*z_directions)] = {
		-1000,
	};
	if(z_directions[0].X < -99){
		for(u32 i=0; i<sizeof(z_directions)/sizeof(*z_directions); i++){
			z_directions[i] = v3f(
				0.01 * myrand_range(-100, 100),
				1.0,
				0.01 * myrand_range(-100, 100)
			);
			z_offsets[i] = 0.01 * myrand_range(0,100);
		}
	}
	if(debugprint)
		std::cerr<<"In goes "<<PP(m_camera_direction)<<", out comes ";
	int sunlight_seen_count = 0;
	float sunlight_min_d = max_d*0.8;
	if(sunlight_min_d > 35*BS)
		sunlight_min_d = 35*BS;
	std::vector<int> values;
	for(u32 i=0; i<sizeof(z_directions)/sizeof(*z_directions); i++){
		v3f z_dir = z_directions[i];
		z_dir.normalize();
		core::CMatrix4<f32> a;
		a.buildRotateFromTo(v3f(0,1,0), z_dir);
		v3f dir = m_camera_direction;
		a.rotateVect(dir);
		int br = 0;
		float step = BS*1.5;
		if(max_d > 35*BS)
			step = max_d / 35 * 1.5;
		float off = step * z_offsets[i];
		bool sunlight_seen_now = false;
		bool ok = getVisibleBrightness(this, m_camera_position, dir,
				step, 1.0, max_d*0.6+off, max_d, ndef, daylight_factor,
				sunlight_min_d,
				&br, &sunlight_seen_now);
		if(sunlight_seen_now)
			sunlight_seen_count++;
		if(!ok)
			continue;
		values.push_back(br);
		// Don't try too much if being in the sun is clear
		if(sunlight_seen_count >= 20)
			break;
	}
	int brightness_sum = 0;
	int brightness_count = 0;
	std::sort(values.begin(), values.end());
	u32 num_values_to_use = values.size();
	if(num_values_to_use >= 10)
		num_values_to_use -= num_values_to_use/2;
	else if(num_values_to_use >= 7)
		num_values_to_use -= num_values_to_use/3;
	u32 first_value_i = (values.size() - num_values_to_use) / 2;
	if(debugprint){
		for(u32 i=0; i < first_value_i; i++)
			std::cerr<<values[i]<<" ";
		std::cerr<<"[";
	}
	for(u32 i=first_value_i; i < first_value_i+num_values_to_use; i++){
		if(debugprint)
			std::cerr<<values[i]<<" ";
		brightness_sum += values[i];
		brightness_count++;
	}
	if(debugprint){
		std::cerr<<"]";
		for(u32 i=first_value_i+num_values_to_use; i < values.size(); i++)
			std::cerr<<values[i]<<" ";
	}
	int ret = 0;
	if(brightness_count == 0){
		MapNode n = getNodeNoEx(floatToInt(m_camera_position, BS));
		if(ndef->get(n).param_type == CPT_LIGHT){
			ret = decode_light(n.getLightBlend(daylight_factor, ndef));
		} else {
			ret = oldvalue;
		}
	} else {
		/*float pre = (float)brightness_sum / (float)brightness_count;
		float tmp = pre;
		const float d = 0.2;
		pre *= 1.0 + d*2;
		pre -= tmp * d;
		int preint = pre;
		ret = MYMAX(0, MYMIN(255, preint));*/
		ret = brightness_sum / brightness_count;
	}
	if(debugprint)
		std::cerr<<"Result: "<<ret<<" sunlight_seen_count="
				<<sunlight_seen_count<<std::endl;
	*sunlight_seen_result = (sunlight_seen_count > 0);
	return ret;
}

void ClientMap::renderPostFx(CameraMode cam_mode)
{
	INodeDefManager *nodemgr = m_gamedef->ndef();

	// Sadly ISceneManager has no "post effects" render pass, in that case we
	// could just register for that and handle it in renderMap().

	MapNode n = getNodeNoEx(floatToInt(m_camera_position, BS));

	// - If the player is in a solid node, make everything black.
	// - If the player is in liquid, draw a semi-transparent overlay.
	// - Do not if player is in third person mode
	const ContentFeatures& features = nodemgr->get(n);
	video::SColor post_effect_color = features.post_effect_color;
	if(features.solidness == 2 && !(g_settings->getBool("noclip") &&
			m_gamedef->checkLocalPrivilege("noclip")) &&
			cam_mode == CAMERA_MODE_FIRST)
	{
		post_effect_color = video::SColor(255, 0, 0, 0);
	}
	if (post_effect_color.getAlpha() != 0)
	{
		// Draw a full-screen rectangle
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		v2u32 ss = driver->getScreenSize();
		core::rect<s32> rect(0,0, ss.X, ss.Y);
		driver->draw2DRectangle(post_effect_color, rect);
	}
}

void ClientMap::PrintInfo(std::ostream &out)
{
	out<<"ClientMap: ";
}


std::vector<v3s16> ClientMap::suggestMapBlocksToFetch(v3s16 camera_p,
		size_t wanted_num_results)
{
	std::vector<v3s16> suggested_mbs;

	// TODO: Add some prediction based on player's current speed so that we are
	//       getting stuff from the correct location compared to where the
	//       player will be after a while?

	v3s16 center_mb = getContainerPos(camera_p, MAP_BLOCKSIZE);

	s16 fetch_distance_nodes = m_control.wanted_range;
	s16 fetch_distance_mapblocks =
			roundf((float)fetch_distance_nodes / MAP_BLOCKSIZE);

	// Avoid running the algorithm through all the close MapBlocks that probably
	// have already been fetched, except once in a while to catch up with
	// possible missed MapBlocks due to player movement or whatever.
	// TODO: Lower this according to the distance the player has moved since
	//       last time this was called
	s16 start_d = m_mapblocks_exist_up_to_d; // Start one lower than have to
	if (++m_mapblocks_exist_up_to_d_reset_counter >= 10) {
		m_mapblocks_exist_up_to_d_reset_counter = 0;
		start_d = 0;
	}
	m_mapblocks_exist_up_to_d = -1; // Reset and recalculate
	if (start_d < 0)
		start_d = 0;

	for (s16 d = start_d; d <= fetch_distance_mapblocks; d++) {
		std::vector<v3s16> ps = FacePositionCache::getFacePositions(d);
		for (size_t i=0; i<ps.size(); i++) {
			v3s16 p = center_mb + ps[i];

			v3s16 blockpos_nodes = p * MAP_BLOCKSIZE;
			v3s16 blockpos_center(
					blockpos_nodes.X + MAP_BLOCKSIZE/2,
					blockpos_nodes.Y + MAP_BLOCKSIZE/2,
					blockpos_nodes.Z + MAP_BLOCKSIZE/2
			);
			v3s16 blockpos_relative = blockpos_center - camera_p;
			f32 distance = blockpos_relative.getLength();
			// Limit fetched MapBlocks to a ball radius instead of a square
			// because that is how they are limited when drawing too
			if (distance > fetch_distance_nodes)
				continue; // Not in range

			MapBlock *b = getBlockNoCreateNoEx(p);

			// TODO: Not sure if this conditional is exactly correct
			if (b != NULL && !b->isDummy())
				continue; // Exists

			if (m_mapblocks_exist_up_to_d == -1)
				m_mapblocks_exist_up_to_d = d - 1;

			// TODO: Frustum culling?
			// TODO: Occlusion culling?

			suggested_mbs.push_back(p);
			if (suggested_mbs.size() >= wanted_num_results)
				goto done;
		}
	}
done:

	infostream << "suggested_mbs.size()=" << suggested_mbs.size() << std::endl;
	return suggested_mbs;
}

s16 ClientMap::suggestAutosendMapblocksRadius()
{
	s16 radius_nodes = m_control.wanted_range;
	s16 radius_mapblocks = roundf((float)radius_nodes / MAP_BLOCKSIZE);
	return radius_mapblocks;
}

float ClientMap::suggestAutosendFov()
{
	return m_camera_fov;
}

