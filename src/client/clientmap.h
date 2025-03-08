// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "irrlichttypes_bloated.h"
#include "map.h"
#include "camera.h"
#include <set>
#include <map>

struct ConvexPolygon;

struct MapDrawControl
{
	// Wanted drawing range
	float wanted_range = 0.0f;
	// Overrides limits by drawing everything
	bool range_all = false;
	// Allow rendering out of bounds
	bool allow_noclip = false;
	// show a wire frame for debugging
	bool show_wireframe = false;
};

struct LiquidWaveParams {
	f32 height;
	f32 length;
	f32 speed;
	f32 animation_timer;
};

class Client;
class ITextureSource;
class PartialMeshBuffer;

namespace irr::scene
{
	class IMeshBuffer;
}

namespace irr::video
{
	class IVideoDriver;
}

struct CachedMeshBuffer {
	std::vector<scene::IMeshBuffer*> buf;
	u8 age = 0;

	void drop();
};

using CachedMeshBuffers = std::unordered_map<std::string, CachedMeshBuffer>;


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

	bool maySaveBlocks() override
	{
		return false;
	}

	void updateCamera(v3f pos, v3f dir, f32 fov, v3s16 offset, video::SColor light_color);

	/*
		Forcefully get a sector from somewhere
	*/
	MapSector * emergeSector(v2s16 p) override;

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode() override;

	virtual void render() override;

	virtual const aabb3f &getBoundingBox() const override
	{
		return m_box;
	}

	void getBlocksInViewRange(v3s16 cam_pos_nodes,
		v3s16 *p_blocks_min, v3s16 *p_blocks_max, float range=-1.0f);
	void updateDrawList();
	// @brief Calculate statistics about the map and keep the blocks alive
	void touchMapBlocks();
	void updateDrawListShadow(v3f shadow_light_pos, v3f shadow_light_dir, float radius, float length);
	// Returns true if draw list needs updating before drawing the next frame.
	bool needsUpdateDrawList() { return m_needs_update_drawlist; }
	void renderMap(video::IVideoDriver* driver, s32 pass);

	void renderMapShadows(video::IVideoDriver *driver,
			const video::SMaterial &material, s32 pass, int frame, int total_frames);

	int getBackgroundBrightness(float max_d, u32 daylight_factor,
			int oldvalue, bool *sunlight_seen_result);

	// Returns the post effect color for the wield hand
	video::SColorf renderPostFx();

	void invalidateMapBlockMesh(MapBlockMesh *mesh);

	// For debug printing
	void PrintInfo(std::ostream &out) override;

	const MapDrawControl & getControl() const { return m_control; }
	f32 getWantedRange() const { return m_control.wanted_range; }
	f32 getCameraFov() const { return m_camera_fov; }

	/**
	 * Finds the heights of the corners of the top face of a liquid node at the
	 * given pos.
	 *
	 * Note: Liquid top faces are not quads, but two triangles, split by a
	 * diagonal.
	 *
	 * @param pos The position of the node.
	 * @param wave_params If given, the corner heights are waved like in the node
	 *                    shader.
	 * @return The heights of the corners in local node space coordinates, in the
	 *         following order: {-x-z, +x-z, -x+z, +x+z}
	 *         Or nullopt if there is no lquid top face.
	 */
	std::optional<std::array<f32, 4>>
	getLiquidTopFaceHeights(v3s16 pos,
			const std::optional<LiquidWaveParams> &wave_params);

	void onSettingChanged(std::string_view name, bool all);

protected:
	// use drop() instead
	virtual ~ClientMap();

	void reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks) override;

private:
	bool isMeshOccluded(MapBlock *mesh_block, u16 mesh_size, v3s16 cam_pos_nodes);

	// update the vertex order in transparent mesh buffers
	void updateTransparentMeshBuffers();

	// helper for renderPostFx
	std::vector<std::pair<ConvexPolygon, video::SColor>> getPostFxPolygons();

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
	video::SColor m_camera_light_color = video::SColor(0xFFFFFFFF);
	bool m_needs_update_transparent_meshes = true;

	std::map<v3s16, MapBlock*, MapBlockComparer> m_drawlist;
	std::vector<MapBlock*> m_keeplist;
	std::map<v3s16, MapBlock*> m_drawlist_shadow;
	bool m_needs_update_drawlist;
	CachedMeshBuffers m_dynamic_buffers;

	bool m_cache_trilinear_filter;
	bool m_cache_bilinear_filter;
	bool m_cache_anistropic_filter;
	bool m_cache_transparency_sorting_group_by_buffers;
	u16 m_cache_transparency_sorting_distance;

	bool m_loops_occlusion_culler;
	bool m_enable_raytraced_culling;
};
