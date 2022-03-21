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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include "map.h"
#include "camera.h"
#include <set>
#include <map>

struct MapDrawControl
{
	// Overrides limits by drawing everything
	bool range_all = false;
	// Wanted drawing range
	float wanted_range = 0.0f;
	// show a wire frame for debugging
	bool show_wireframe = false;
};

struct MeshBufList
{
	video::SMaterial m;
	std::vector<std::pair<v3s16,scene::IMeshBuffer*>> bufs;
};

struct MeshBufListList
{
	/*!
	 * Stores the mesh buffers of the world.
	 * The array index is the material's layer.
	 * The vector part groups vertices by material.
	 */
	std::vector<MeshBufList> lists[MAX_TILE_LAYERS];

	void clear();
	void add(scene::IMeshBuffer *buf, v3s16 position, u8 layer);
};

class Client;
class ITextureSource;

/*
	ClientMap

	This is the only map class that is able to render itself on screen.
*/

class ClientMap : public Map, public scene::ISceneNode
{
public:
	ClientMap(
			Client *client,
			RenderingEngine *rendering_engine,
			MapDrawControl &control,
			s32 id
	);

	virtual ~ClientMap() = default;

	bool maySaveBlocks() override
	{
		return false;
	}

	void drop()
	{
		ISceneNode::drop();
	}

	void updateCamera(const v3f &pos, const v3f &dir, f32 fov, const v3s16 &offset)
	{
		v3s16 previous_block = getContainerPos(floatToInt(m_camera_position, BS) + m_camera_offset, MAP_BLOCKSIZE);

		m_camera_position = pos;
		m_camera_direction = dir;
		m_camera_fov = fov;
		m_camera_offset = offset;

		v3s16 current_block = getContainerPos(floatToInt(m_camera_position, BS) + m_camera_offset, MAP_BLOCKSIZE);

		// reorder the blocks when camera crosses block boundary
		if (previous_block != current_block)
			m_needs_update_drawlist = true;
	}

	/*
		Forcefully get a sector from somewhere
	*/
	MapSector * emergeSector(v2s16 p);

	//void deSerializeSector(v2s16 p2d, std::istream &is);

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode();

	virtual void render()
	{
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		renderMap(driver, SceneManager->getSceneNodeRenderPass());
	}

	virtual const aabb3f &getBoundingBox() const
	{
		return m_box;
	}

	void getBlocksInViewRange(v3s16 cam_pos_nodes,
		v3s16 *p_blocks_min, v3s16 *p_blocks_max, float range=-1.0f);
	void updateDrawList();
	void updateDrawListShadow(const v3f &shadow_light_pos, const v3f &shadow_light_dir, float shadow_range);
	// Returns true if draw list needs updating before drawing the next frame.
	bool needsUpdateDrawList() { return m_needs_update_drawlist; }
	void renderMap(video::IVideoDriver* driver, s32 pass);

	void renderMapShadows(video::IVideoDriver *driver,
			const video::SMaterial &material, s32 pass, int frame, int total_frames);

	int getBackgroundBrightness(float max_d, u32 daylight_factor,
			int oldvalue, bool *sunlight_seen_result);

	void renderPostFx(CameraMode cam_mode);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);

	const MapDrawControl & getControl() const { return m_control; }
	f32 getWantedRange() const { return m_control.wanted_range; }
	f32 getCameraFov() const { return m_camera_fov; }

private:
	// Orders blocks by distance to the camera
	class MapBlockComparer
	{
	public:
		MapBlockComparer(const v3s16 &camera_block) : m_camera_block(camera_block) {}

		bool operator() (const v3s16 &left, const v3s16 &right) const
		{
			auto distance_left = left.getDistanceFromSQ(m_camera_block);
			auto distance_right = right.getDistanceFromSQ(m_camera_block);
			return distance_left > distance_right || (distance_left == distance_right && left > right);
		}

	private:
		v3s16 m_camera_block;
	};

	Client *m_client;
	RenderingEngine *m_rendering_engine;

	aabb3f m_box = aabb3f(-BS * 1000000, -BS * 1000000, -BS * 1000000,
		BS * 1000000, BS * 1000000, BS * 1000000);

	MapDrawControl &m_control;

	v3f m_camera_position = v3f(0,0,0);
	v3f m_camera_direction = v3f(0,0,1);
	f32 m_camera_fov = M_PI;
	v3s16 m_camera_offset;

	std::map<v3s16, MapBlock*, MapBlockComparer> m_drawlist;
	std::map<v3s16, MapBlock*> m_drawlist_shadow;
	bool m_needs_update_drawlist;

	std::set<v2s16> m_last_drawn_sectors;

	bool m_cache_trilinear_filter;
	bool m_cache_bilinear_filter;
	bool m_cache_anistropic_filter;
	bool m_added_to_shadow_renderer{false};
};
