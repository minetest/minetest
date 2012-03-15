/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "clientmap.h"
#include "client.h"
#include "mapblock_mesh.h"
#include <IMaterialRenderer.h>
#include "log.h"
#include "mapsector.h"
#include "main.h" // dout_client, g_settings
#include "nodedef.h"
#include "mapblock.h"
#include "profiler.h"
#include "settings.h"

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
	m_camera_fov(PI)
{
	m_camera_mutex.Init();
	assert(m_camera_mutex.IsInitialized());
	
	m_box = core::aabbox3d<f32>(-BS*1000000,-BS*1000000,-BS*1000000,
			BS*1000000,BS*1000000,BS*1000000);
}

ClientMap::~ClientMap()
{
	/*JMutexAutoLock lock(mesh_mutex);
	
	if(mesh != NULL)
	{
		mesh->drop();
		mesh = NULL;
	}*/
}

MapSector * ClientMap::emergeSector(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);
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
		//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
		m_sectors.insert(p2d, sector);
	}
	
	return sector;
}

#if 0
void ClientMap::deSerializeSector(v2s16 p2d, std::istream &is)
{
	DSTACK(__FUNCTION_NAME);
	ClientMapSector *sector = NULL;

	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
	
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p2d);

	if(n != NULL)
	{
		sector = (ClientMapSector*)n->getValue();
		assert(sector->getId() == MAPSECTOR_CLIENT);
	}
	else
	{
		sector = new ClientMapSector(this, p2d);
		{
			//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
			m_sectors.insert(p2d, sector);
		}
	}

	sector->deSerialize(is);
}
#endif

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
			count++;
			if(count >= needed_count)
				return true;
		}
		step *= stepfac;
	}
	return false;
}

void ClientMap::renderMap(video::IVideoDriver* driver, s32 pass)
{
	INodeDefManager *nodemgr = m_gamedef->ndef();

	//m_dout<<DTIME<<"Rendering map..."<<std::endl;
	DSTACK(__FUNCTION_NAME);

	bool is_transparent_pass = pass == scene::ESNRP_TRANSPARENT;
	
	std::string prefix;
	if(pass == scene::ESNRP_SOLID)
		prefix = "CM: solid: ";
	else
		prefix = "CM: transparent: ";

	/*
		This is called two times per frame, reset on the non-transparent one
	*/
	if(pass == scene::ESNRP_SOLID)
	{
		m_last_drawn_sectors.clear();
	}

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
	u32 daynight_ratio = m_client->getDayNightRatio();

	m_camera_mutex.Lock();
	v3f camera_position = m_camera_position;
	v3f camera_direction = m_camera_direction;
	f32 camera_fov = m_camera_fov;
	m_camera_mutex.Unlock();

	/*
		Get all blocks and draw all visible ones
	*/

	v3s16 cam_pos_nodes = floatToInt(camera_position, BS);
	
	v3s16 box_nodes_d = m_control.wanted_range * v3s16(1,1,1);

	v3s16 p_nodes_min = cam_pos_nodes - box_nodes_d;
	v3s16 p_nodes_max = cam_pos_nodes + box_nodes_d;

	// Take a fair amount as we will be dropping more out later
	// Umm... these additions are a bit strange but they are needed.
	v3s16 p_blocks_min(
			p_nodes_min.X / MAP_BLOCKSIZE - 3,
			p_nodes_min.Y / MAP_BLOCKSIZE - 3,
			p_nodes_min.Z / MAP_BLOCKSIZE - 3);
	v3s16 p_blocks_max(
			p_nodes_max.X / MAP_BLOCKSIZE + 1,
			p_nodes_max.Y / MAP_BLOCKSIZE + 1,
			p_nodes_max.Z / MAP_BLOCKSIZE + 1);
	
	u32 vertex_count = 0;
	u32 meshbuffer_count = 0;
	
	// For limiting number of mesh animations per frame
	u32 mesh_animate_count = 0;
	u32 mesh_animate_count_far = 0;
	
	// Number of blocks in rendering range
	u32 blocks_in_range = 0;
	// Number of blocks occlusion culled
	u32 blocks_occlusion_culled = 0;
	// Number of blocks in rendering range but don't have a mesh
	u32 blocks_in_range_without_mesh = 0;
	// Blocks that had mesh that would have been drawn according to
	// rendering range (if max blocks limit didn't kick in)
	u32 blocks_would_have_drawn = 0;
	// Blocks that were drawn and had a mesh
	u32 blocks_drawn = 0;
	// Blocks which had a corresponding meshbuffer for this pass
	u32 blocks_had_pass_meshbuf = 0;
	// Blocks from which stuff was actually drawn
	u32 blocks_without_stuff = 0;

	/*
		Collect a set of blocks for drawing
	*/
	
	core::map<v3s16, MapBlock*> drawset;

	{
	ScopeProfiler sp(g_profiler, prefix+"collecting blocks for drawing", SPT_AVG);

	for(core::map<v2s16, MapSector*>::Iterator
			si = m_sectors.getIterator();
			si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();
		v2s16 sp = sector->getPos();
		
		if(m_control.range_all == false)
		{
			if(sp.X < p_blocks_min.X
			|| sp.X > p_blocks_max.X
			|| sp.Y < p_blocks_min.Z
			|| sp.Y > p_blocks_max.Z)
				continue;
		}

		core::list< MapBlock * > sectorblocks;
		sector->getBlocks(sectorblocks);
		
		/*
			Loop through blocks in sector
		*/

		u32 sector_blocks_drawn = 0;
		
		core::list< MapBlock * >::Iterator i;
		for(i=sectorblocks.begin(); i!=sectorblocks.end(); i++)
		{
			MapBlock *block = *i;

			/*
				Compare block position to camera position, skip
				if not seen on display
			*/
			
			float range = 100000 * BS;
			if(m_control.range_all == false)
				range = m_control.wanted_range * BS;

			float d = 0.0;
			if(isBlockInSight(block->getPos(), camera_position,
					camera_direction, camera_fov,
					range, &d) == false)
			{
				continue;
			}

			// This is ugly (spherical distance limit?)
			/*if(m_control.range_all == false &&
					d - 0.5*BS*MAP_BLOCKSIZE > range)
				continue;*/

			blocks_in_range++;
			
			/*
				Ignore if mesh doesn't exist
			*/
			{
				//JMutexAutoLock lock(block->mesh_mutex);

				if(block->mesh == NULL){
					blocks_in_range_without_mesh++;
					continue;
				}
			}

			/*
				Occlusion culling
			*/

			v3s16 cpn = block->getPos() * MAP_BLOCKSIZE;
			cpn += v3s16(MAP_BLOCKSIZE/2, MAP_BLOCKSIZE/2, MAP_BLOCKSIZE/2);
			float step = BS*1;
			float stepfac = 1.1;
			float startoff = BS*1;
			float endoff = -BS*MAP_BLOCKSIZE*1.42*1.42;
			v3s16 spn = cam_pos_nodes + v3s16(0,0,0);
			s16 bs2 = MAP_BLOCKSIZE/2 + 1;
			u32 needed_count = 1;
			if(
				isOccluded(this, spn, cpn + v3s16(0,0,0),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(bs2,bs2,bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(bs2,bs2,-bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(bs2,-bs2,bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(bs2,-bs2,-bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(-bs2,bs2,bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(-bs2,bs2,-bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(-bs2,-bs2,bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr) &&
				isOccluded(this, spn, cpn + v3s16(-bs2,-bs2,-bs2),
					step, stepfac, startoff, endoff, needed_count, nodemgr)
			)
			{
				blocks_occlusion_culled++;
				continue;
			}
			
			// This block is in range. Reset usage timer.
			block->resetUsageTimer();

			// Limit block count in case of a sudden increase
			blocks_would_have_drawn++;
			if(blocks_drawn >= m_control.wanted_max_blocks
					&& m_control.range_all == false
					&& d > m_control.wanted_min_range * BS)
				continue;

			// Mesh animation
			{
				//JMutexAutoLock lock(block->mesh_mutex);
				MapBlockMesh *mapBlockMesh = block->mesh;
				// Pretty random but this should work somewhat nicely
				bool faraway = d >= BS*50;
				//bool faraway = d >= m_control.wanted_range * BS;
				if(mapBlockMesh->isAnimationForced() ||
						!faraway ||
						mesh_animate_count_far < (m_control.range_all ? 200 : 50))
				{
					bool animated = mapBlockMesh->animate(
							faraway,
							animation_time,
							crack,
							daynight_ratio);
					if(animated)
						mesh_animate_count++;
					if(animated && faraway)
						mesh_animate_count_far++;
				}
				else
				{
					mapBlockMesh->decreaseAnimationForceTimer();
				}
			}

			// Add to set
			drawset[block->getPos()] = block;
			
			sector_blocks_drawn++;
			blocks_drawn++;

		} // foreach sectorblocks

		if(sector_blocks_drawn != 0)
			m_last_drawn_sectors[sp] = true;
	}
	} // ScopeProfiler
	
	/*
		Draw the selected MapBlocks
	*/

	{
	ScopeProfiler sp(g_profiler, prefix+"drawing blocks", SPT_AVG);

	int timecheck_counter = 0;
	for(core::map<v3s16, MapBlock*>::Iterator
			i = drawset.getIterator();
			i.atEnd() == false; i++)
	{
		{
			timecheck_counter++;
			if(timecheck_counter > 50)
			{
				timecheck_counter = 0;
				int time2 = time(0);
				if(time2 > time1 + 4)
				{
					infostream<<"ClientMap::renderMap(): "
						"Rendering takes ages, returning."
						<<std::endl;
					return;
				}
			}
		}
		
		MapBlock *block = i.getNode()->getValue();

		/*
			Draw the faces of the block
		*/
		{
			//JMutexAutoLock lock(block->mesh_mutex);

			MapBlockMesh *mapBlockMesh = block->mesh;
			assert(mapBlockMesh);

			scene::SMesh *mesh = mapBlockMesh->getMesh();
			assert(mesh);

			u32 c = mesh->getMeshBufferCount();
			bool stuff_actually_drawn = false;
			for(u32 i=0; i<c; i++)
			{
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
				const video::SMaterial& material = buf->getMaterial();
				video::IMaterialRenderer* rnd =
						driver->getMaterialRenderer(material.MaterialType);
				bool transparent = (rnd && rnd->isTransparent());
				// Render transparent on transparent pass and likewise.
				if(transparent == is_transparent_pass)
				{
					if(buf->getVertexCount() == 0)
						errorstream<<"Block ["<<analyze_block(block)
								<<"] contains an empty meshbuf"<<std::endl;
					/*
						This *shouldn't* hurt too much because Irrlicht
						doesn't change opengl textures if the old
						material has the same texture.
					*/
					driver->setMaterial(buf->getMaterial());
					driver->drawMeshBuffer(buf);
					vertex_count += buf->getVertexCount();
					meshbuffer_count++;
					stuff_actually_drawn = true;
				}
			}
			if(stuff_actually_drawn)
				blocks_had_pass_meshbuf++;
			else
				blocks_without_stuff++;
		}
	}
	} // ScopeProfiler
	
	// Log only on solid pass because values are the same
	if(pass == scene::ESNRP_SOLID){
		g_profiler->avg("CM: blocks in range", blocks_in_range);
		g_profiler->avg("CM: blocks occlusion culled", blocks_occlusion_culled);
		if(blocks_in_range != 0)
			g_profiler->avg("CM: blocks in range without mesh (frac)",
					(float)blocks_in_range_without_mesh/blocks_in_range);
		g_profiler->avg("CM: blocks drawn", blocks_drawn);
		g_profiler->avg("CM: animated meshes", mesh_animate_count);
		g_profiler->avg("CM: animated meshes (far)", mesh_animate_count_far);
	}
	
	g_profiler->avg(prefix+"vertices drawn", vertex_count);
	if(blocks_had_pass_meshbuf != 0)
		g_profiler->avg(prefix+"meshbuffers per block",
				(float)meshbuffer_count / (float)blocks_had_pass_meshbuf);
	if(blocks_drawn != 0)
		g_profiler->avg(prefix+"empty blocks (frac)",
				(float)blocks_without_stuff / blocks_drawn);

	m_control.blocks_drawn = blocks_drawn;
	m_control.blocks_would_have_drawn = blocks_would_have_drawn;

	/*infostream<<"renderMap(): is_transparent_pass="<<is_transparent_pass
			<<", rendered "<<vertex_count<<" vertices."<<std::endl;*/
}

void ClientMap::renderPostFx()
{
	INodeDefManager *nodemgr = m_gamedef->ndef();

	// Sadly ISceneManager has no "post effects" render pass, in that case we
	// could just register for that and handle it in renderMap().

	m_camera_mutex.Lock();
	v3f camera_position = m_camera_position;
	m_camera_mutex.Unlock();

	MapNode n = getNodeNoEx(floatToInt(camera_position, BS));

	// - If the player is in a solid node, make everything black.
	// - If the player is in liquid, draw a semi-transparent overlay.
	const ContentFeatures& features = nodemgr->get(n);
	video::SColor post_effect_color = features.post_effect_color;
	if(features.solidness == 2 && g_settings->getBool("free_move") == false)
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


