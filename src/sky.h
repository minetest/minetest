/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "camera.h"

#ifndef SKY_HEADER
#define SKY_HEADER

#define SKY_MATERIAL_COUNT 5
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
			float direct_brightness, bool sunlight_seen, CameraMode cam_mode,
			float yaw, float pitch);
	
	float getBrightness(){ return m_brightness; }

	video::SColor getBgColor(){
		return m_visible ? m_bgcolor : m_fallback_bg_color;
	}
	video::SColor getSkyColor(){
		return m_visible ? m_skycolor : m_fallback_bg_color;
	}
	
	bool getCloudsVisible(){ return m_clouds_visible && m_visible; }
	video::SColorf getCloudColor(){ return m_cloudcolor_f; }

	void setVisible(bool visible){ m_visible = visible; }
	void setFallbackBgColor(const video::SColor &fallback_bg_color){
		m_fallback_bg_color = fallback_bg_color;
	}

private:
	core::aabbox3d<f32> Box;
	video::SMaterial m_materials[SKY_MATERIAL_COUNT];

	// How much sun & moon transition should affect horizon color
	float m_horizon_blend()
	{
		if (!m_sunlight_seen)
			return 0;
		float x = m_time_of_day >= 0.5 ? (1 - m_time_of_day) * 2 : m_time_of_day * 2;

		if (x <= 0.3)
			return 0;
		if (x <= 0.4) // when the sun and moon are aligned
			return (x - 0.3) * 10;
		if (x <= 0.5)
			return (0.5 - x) * 10;
		return 0;
	}

	// Mix two colors by a given amount
	video::SColor m_mix_scolor(video::SColor col1, video::SColor col2, f32 factor)
	{
		video::SColor result = video::SColor(
			col1.getAlpha() * (1 - factor) + col2.getAlpha() * factor,
			col1.getRed() * (1 - factor) + col2.getRed() * factor,
			col1.getGreen() * (1 - factor) + col2.getGreen() * factor,
			col1.getBlue() * (1 - factor) + col2.getBlue() * factor);
		return result;
	}
	video::SColorf m_mix_scolorf(video::SColorf col1, video::SColorf col2, f32 factor)
	{
		video::SColorf result = video::SColorf(
			col1.r * (1 - factor) + col2.r * factor,
			col1.g * (1 - factor) + col2.g * factor,
			col1.b * (1 - factor) + col2.b * factor,
			col1.a * (1 - factor) + col2.a * factor);
		return result;
	}

	bool m_visible;
	video::SColor m_fallback_bg_color; // Used when m_visible=false
	bool m_first_update;
	float m_time_of_day;
	float m_time_brightness;
	bool m_sunlight_seen;
	float m_brightness;
	float m_cloud_brightness;
	bool m_clouds_visible;
	bool m_directional_colored_fog;
	video::SColorf m_bgcolor_bright_f;
	video::SColorf m_skycolor_bright_f;
	video::SColorf m_cloudcolor_bright_f;
	video::SColor m_bgcolor;
	video::SColor m_skycolor;
	video::SColorf m_cloudcolor_f;
	v3f m_stars[SKY_STAR_COUNT];
	video::S3DVertex m_star_vertices[SKY_STAR_COUNT*4];
	video::ITexture* m_sun_texture;
	video::ITexture* m_moon_texture;
	video::ITexture* m_sun_tonemap;
	video::ITexture* m_moon_tonemap;
};

#endif

