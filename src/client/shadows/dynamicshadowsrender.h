/*
Minetest
Copyright (C) 2021 Liso <anlismon@gmail.com>

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

#include <string>
#include <vector>
#include "irrlichttypes_extrabloated.h"
#include "client/shadows/dynamicshadows.h"

class ShadowDepthShaderCB;
class shadowScreenQuad;
class shadowScreenQuadCB;

enum E_SHADOW_MODE : u8
{
	ESM_RECEIVE = 0,
	ESM_BOTH,
};

struct NodeToApply
{
	NodeToApply(scene::ISceneNode *n,
			E_SHADOW_MODE m = E_SHADOW_MODE::ESM_BOTH) :
			node(n),
			shadowMode(m){};
	bool operator<(const NodeToApply &other) const { return node < other.node; };

	scene::ISceneNode *node;

	E_SHADOW_MODE shadowMode{E_SHADOW_MODE::ESM_BOTH};
	bool dirty{false};
};

// Enum class of cube map faces

enum class CubeMapFace : u16
{
	POSITIVE_X = 0,
	NEGATIVE_X,
	POSITIVE_Y,
	NEGATIVE_Y,
	POSITIVE_Z,
	NEGATIVE_Z,
	COUNT
};

// Cube map texture

/*class CubeMapTexture
{
public:
	// Default constructor
	CubeMapTexture() = default;

	// Constructor of 6 2d textures (+X, -X, +Y, -Y, +Z, -Z) with the same data (name, size and format)
	CubeMapTexture(std::string name, f32 size, video::E_COLOR_FORMAT format);

	~CubeMapTexture();

	// Getters

	std::array<video::ITexture*, 6> getAllData() const;

	video::ITexture* getTexture(CubeMapFace face) const;

private:
	std::array<video::ITexture*, 6> m_data;

	std::string m_name;
	core::dimension2du m_size;
	video::ECOLOR_FORMAT m_format;
};*/

class ShadowRenderer
{
public:
	ShadowRenderer(IrrlichtDevice *device, Client *client);

	~ShadowRenderer();

	void initialize();

	/// Adds a directional light shadow map (Usually just one (the sun) except in
	/// Tattoine ).
	size_t addDirectionalLight();
	DirectionalLight &getDirectionalLight(u32 index = 0);
	size_t getDirectionalLightCount() const;
	f32 getMaxShadowFar() const;

	// Add a point light
	bool addPointLight(v3s16 light_node_pos, MapNode* light_node=nullptr);

	// Gets a point light with index 'index'
	PointLight &getPointLight(u32 index) const;

	// Gets a total count of point lights
	size_t getPointLightCount() const;

	// Clears m_pointlight_list
	void clearPointLightList();

	float getUpdateDelta() const;
	/// Adds a shadow to the scene node.
	/// The shadow mode can be ESM_BOTH, or ESM_RECEIVE.
	/// ESM_BOTH casts and receives shadows
	/// ESM_RECEIVE only receives but does not cast shadows.
	///
	void addNodeToShadowList(scene::ISceneNode *node,
			E_SHADOW_MODE shadowMode = ESM_BOTH);
	void removeNodeFromShadowList(scene::ISceneNode *node);

	void setClearColor(video::SColor ClearColor);

	void update();

	video::ITexture *get_texture()
	{
		return shadowMapTextureFinal;
	}


	bool is_active() const { return m_shadows_enabled; }
	void setTimeOfDay(float isDay) { m_time_day = isDay; };

	s32 getShadowSamples() const { return m_shadow_samples; }
	float getShadowStrength() const { return m_shadow_strength; }
	float getTimeOfDay() const { return m_time_day; }

private:
	video::ITexture *getSMTexture(const std::string &shadow_map_name,
			video::ECOLOR_FORMAT texture_format,
			bool force_creation = false);

	void genShadowMaps(std::string clientmap_name, std::string dynamic_objects_name, std::string colormap_name, std::string final_name
			video::ITexture* clientmap, video::ITexture* dynamic_objects_map, video::ITexture* colormap, video::ITexture* finalmap);
	void renderShadowMap(video::ITexture *target, m4f view_mat, m4f proj_mat
			scene::E_SCENE_NODE_RENDER_PASS pass =
					scene::ESNRP_SOLID);
	void renderShadowObjects(video::ITexture *target, m4f view_mat, m4f_proj_mat);
	void mixShadowsQuad();

	// a bunch of variables
	IrrlichtDevice *m_device{nullptr};
	scene::ISceneManager *m_smgr{nullptr};
	video::IVideoDriver *m_driver{nullptr};
	Client *m_client{nullptr};
	video::ITexture *shadowMapClientMap{nullptr};
	video::ITexture *shadowMapTextureFinal{nullptr};
	video::ITexture *shadowMapTextureDynamicObjects{nullptr};
	video::ITexture *shadowMapTextureColors{nullptr};
	video::SColor m_clear_color{0x0};
	std::array<video::ITexture*, 6> shadowCubeMapClientMap;
	std::array<video::ITexture*, 6> shadowCubeMapDynamicObjects;
	std::array<video::ITexture*, 6> shadowCubeMapColors;
	std::array<video::ITexture*, 6> shadowCubeMapFinal;

	std::vector<DirectionalLight> m_light_list;
	std::vector<NodeToApply> m_shadow_node_array;

	// List of all point lights within a viewing range
	std::vector<PointLight> m_pointlight_list;

	float m_shadow_strength;
	float m_shadow_map_max_distance;
	float m_shadow_map_texture_size;
	float m_time_day{0.0f};
	float m_update_delta;
	int m_shadow_samples;
	bool m_shadow_map_texture_32bit;
	bool m_shadows_enabled;
	bool m_shadow_map_colored;

	video::ECOLOR_FORMAT m_texture_format{video::ECOLOR_FORMAT::ECF_R16F};
	video::ECOLOR_FORMAT m_texture_format_color{video::ECOLOR_FORMAT::ECF_R16G16};

	// Shadow Shader stuff

	void createShaders();
	std::string readShaderFile(const std::string &path);

	s32 depth_shader{-1};
	s32 depth_shader_trans{-1};
	s32 mixcsm_shader{-1};

	ShadowDepthShaderCB *m_shadow_depth_cb{nullptr};
	ShadowDepthShaderCB *m_shadow_depth_trans_cb{nullptr};

	shadowScreenQuad *m_screen_quad{nullptr};
	shadowScreenQuadCB *m_shadow_mix_cb{nullptr};
};
