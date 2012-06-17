/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes_extrabloated.h"
#include <ISceneNode.h>

#ifndef SKY_HEADER
#define SKY_HEADER

#define SKY_MATERIAL_COUNT 3
#define SKY_STAR_COUNT 200

// Skybox, rendered with zbuffer turned off, before all other nodes.
class Sky : public scene::ISceneNode
{
public:
	//! constructor
	Sky(scene::ISceneNode* parent, scene::ISceneManager* mgr, s32 id);

	virtual void OnRegisterSceneNode();

	//! renders the node.
	virtual void render();

	virtual const core::aabbox3d<f32>& getBoundingBox() const;

	// Used by Irrlicht for optimizing rendering
	virtual video::SMaterial& getMaterial(u32 i)
	{ return m_materials[i]; }

	// Used by Irrlicht for optimizing rendering
	virtual u32 getMaterialCount() const
	{ return SKY_MATERIAL_COUNT; }

	void update(float m_time_of_day, float time_brightness,
			float direct_brightness, bool sunlight_seen);
	
	float getBrightness(){ return m_brightness; }
	video::SColor getBgColor(){ return m_bgcolor; }
	video::SColor getSkyColor(){ return m_skycolor; }
	
	bool getCloudsVisible(){ return m_clouds_visible; }
	video::SColorf getCloudColor(){ return m_cloudcolor_f; }

private:
	core::aabbox3d<f32> Box;
	video::SMaterial m_materials[SKY_MATERIAL_COUNT];
	
	bool m_first_update;
	float m_time_of_day;
	float m_time_brightness;
	bool m_sunlight_seen;
	float m_brightness;
	float m_cloud_brightness;
	bool m_clouds_visible;
	video::SColorf m_bgcolor_bright_f;
	video::SColorf m_skycolor_bright_f;
	video::SColorf m_cloudcolor_bright_f;
	video::SColor m_bgcolor;
	video::SColor m_skycolor;
	video::SColorf m_cloudcolor_f;
	v3f m_stars[SKY_STAR_COUNT];
	u16 m_star_indices[SKY_STAR_COUNT*4];
	video::S3DVertex m_star_vertices[SKY_STAR_COUNT*4];
};

#endif

