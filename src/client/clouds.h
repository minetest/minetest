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

#include "irrlichttypes_bloated.h"
#include "constants.h"
#include "irr_ptr.h"
#include "skyparams.h"
#include <iostream>
#include <ISceneNode.h>
#include <SMaterial.h>
#include <SMeshBuffer.h>

class IShaderSource;

namespace irr::scene
{
	class ISceneManager;
}

// Menu clouds
// The mainmenu and the loading screen use the same Clouds object so that the
// clouds don't jump when switching between the two.
class Clouds;
extern scene::ISceneManager *g_menucloudsmgr;
extern Clouds *g_menuclouds;

class Clouds : public scene::ISceneNode
{
public:
	Clouds(scene::ISceneManager* mgr, IShaderSource *ssrc,
			s32 id,
			u32 seed
	);

	~Clouds();

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode();

	virtual void render();

	void renderDepth();

	virtual const aabb3f &getBoundingBox() const
	{
		return m_box;
	}

	virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual video::SMaterial& getMaterial(u32 i)
	{
		return m_material;
	}

	/*
		Other stuff
	*/

	void step(float dtime);

	void update(const v3f &camera_p, const video::SColorf &color);

	void updateCameraOffset(v3s16 camera_offset)
	{
		m_camera_offset = camera_offset;
		updateBox();
	}

	void readSettings();

	void setDensity(float density)
	{
		if (m_params.density == density)
			return;
		m_params.density = density;
		invalidateMesh();
	}

	float getDensity() const {
		return m_params.density;
	}

	void setColorBright(video::SColor color_bright)
	{
		m_params.color_bright = color_bright;
	}

	void setColorAmbient(video::SColor color_ambient)
	{
		m_params.color_ambient = color_ambient;
	}

	void setColorShadow(video::SColor color_shadow)
	{
		if (m_params.color_shadow == color_shadow)
			return;
		m_params.color_shadow = color_shadow;
		invalidateMesh();
	}

	void setHeight(float height)
	{
		if (m_params.height == height)
			return;
		m_params.height = height;
		updateBox();
		invalidateMesh();
	}

	float getHeight() const {
		return m_params.height;
	}

	void setSpeed(v2f speed)
	{
		m_params.speed = speed;
	}

	void setThickness(float thickness)
	{
		if (m_params.thickness == thickness)
			return;
		m_params.thickness = thickness;
		updateBox();
		invalidateMesh();
	}

	float getThickness() const {
		return m_params.thickness;
	}

	bool isCameraInsideCloud() const { return m_camera_inside_cloud; }

	const video::SColor getColor() const { return m_color.toSColor(); }

	bool getGrid(int x, int y)
	{
		int index = x + y * m_cloud_radius_i * 2;
		if (index < 0 || index >= m_grid.size())
			return false;
		return m_grid[index];
	}

	v2f getCloudOffset() const { return m_noise_position; }

private:
	void updateBox()
	{
		float height_bs    = m_params.height    * BS;
		float thickness_bs = m_params.thickness * BS;
		m_box = aabb3f(-BS * 1000000.0f, height_bs, -BS * 1000000.0f,
				BS * 1000000.0f, height_bs + thickness_bs, BS * 1000000.0f);
	}

	void updateMesh();
	void invalidateMesh()
	{
		m_mesh_valid = false;
	}

	bool gridFilled(int x, int y) const;

	// Are the clouds 3D?
	inline bool is3D() const {
		return m_enable_3d && m_params.thickness >= 0.01f;
	}

	video::SMaterial m_material;
	irr_ptr<scene::SMeshBuffer> m_meshbuffer;
	// Value of m_origin at the time the mesh was last updated
	v2f m_mesh_origin;
	// Value of center_of_drawing_in_noise_i at the time the mesh was last updated
	v2s16 m_last_noise_center;
	// Was the mesh ever generated?
	bool m_mesh_valid = false;

	aabb3f m_box;
	v2f m_origin;
	u16 m_cloud_radius_i;
	u32 m_seed;
	v3f m_camera_pos;
	v2f m_noise_position;

	v3s16 m_camera_offset;
	bool m_camera_inside_cloud = false;

	bool m_enable_shaders, m_enable_3d;
	video::SColorf m_color = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	CloudParams m_params;

	std::vector<bool> m_grid;

};
