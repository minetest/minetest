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

#include <ISceneNode.h>
#include "camera.h"
#include "irrlichttypes_extrabloated.h"

#pragma once

#define SKY_MATERIAL_COUNT 5
#define SKY_STAR_COUNT 200

class ITextureSource;

// Skybox, rendered with zbuffer turned off, before all other nodes.
class Sky : public scene::ISceneNode
{
public:
	//! constructor
	Sky(s32 id, ITextureSource *tsrc);

	virtual void OnRegisterSceneNode();

	//! renders the node.
	virtual void render();

	virtual const aabb3f &getBoundingBox() const { return m_box; }

	// Used by Irrlicht for optimizing rendering
	virtual video::SMaterial &getMaterial(u32 i) { return m_materials[i]; }

	// Used by Irrlicht for optimizing rendering
	virtual u32 getMaterialCount() const { return SKY_MATERIAL_COUNT; }

	void update(float m_time_of_day, float time_brightness, float direct_brightness,
			bool sunlight_seen, CameraMode cam_mode, float yaw, float pitch);

	float getBrightness() { return m_brightness; }	

	const video::SColor &getBgColor() const
	{
		return m_visible ? m_bgcolor : m_fallback_bg_color;
	}

	const video::SColor &getSkyColor() const
	{
		return m_visible ? m_skycolor : m_fallback_bg_color;
	}

	bool getCloudsVisible() const { return m_clouds_visible && m_clouds_enabled; }
	const video::SColorf &getCloudColor() const { return m_cloudcolor_f; }

	void setVisible(bool visible) { m_visible = visible; }
	// Set only from set_sky API
	void setCloudsEnabled(bool clouds_enabled) { m_clouds_enabled = clouds_enabled; }
	void setFallbackBgColor(const video::SColor &fallback_bg_color)
	{
		m_fallback_bg_color = fallback_bg_color;
	}
	void overrideColors(const video::SColor &bgcolor, const video::SColor &skycolor)
	{
		m_bgcolor = bgcolor;
		m_skycolor = skycolor;
	}
	void setBodiesVisible(bool visible) { m_bodies_visible = visible; }
	void setSunVisible(bool sun_visible) { m_sun_visible = sun_visible; }
	void setSunGlowVisible(bool sun_glow) { m_sun_glow = sun_glow; }
	void setMoonVisible(bool moon_visible) { m_moon_visible = moon_visible; }
	void setStarsVisible(bool stars_visible) { m_stars_visible = stars_visible; }
	void setMeshVisible(bool dynamic_visible) { m_dyanmic_visible = dynamic_visible; }
	void setStarCount(u16 star_count) { m_star_count = star_count; }
	void setCustomFog(bool use_fog) { m_use_fog = use_fog; }
	void setSunYaw(f32 sun_yaw) { m_sun_yaw = sun_yaw; }
	void setSunTilt(f32 sun_tilt) { m_sun_tilt = sun_tilt; }
	void setSunTexture(std::string sun_texture) { m_sun_name = sun_texture; }
	void setMoonYaw(f32 moon_yaw) { m_moon_yaw = moon_yaw; }
	void setMoonTilt(f32 moon_tilt) { m_moon_tilt = moon_tilt; }
	void setMoonTexture(std::string moon_texture) { m_moon_name = moon_texture; }
	void setStarYaw(f32 star_yaw) { m_star_yaw = star_yaw; }
	void setStarTilt(f32 star_tilt) { m_star_tilt = star_tilt; }


private:
	aabb3f m_box;
	video::SMaterial m_materials[SKY_MATERIAL_COUNT];

	// How much sun & moon transition should affect horizon color
	float m_horizon_blend()
	{
		if (!m_sunlight_seen)
			return 0;
		float x = m_time_of_day >= 0.5 ? (1 - m_time_of_day) * 2
					       : m_time_of_day * 2;

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
		video::SColorf result =
				video::SColorf(col1.r * (1 - factor) + col2.r * factor,
						col1.g * (1 - factor) + col2.g * factor,
						col1.b * (1 - factor) + col2.b * factor,
						col1.a * (1 - factor) + col2.a * factor);
		return result;
	}

	bool m_visible = true;
	// Used when m_visible=false
	video::SColor m_fallback_bg_color = video::SColor(255, 255, 255, 255);
	bool m_first_update = true;
	float m_time_of_day;
	float m_time_brightness;
	bool m_sunlight_seen;
	float m_brightness = 0.5f;
	float m_cloud_brightness = 0.5f;
	bool m_clouds_visible; // Whether clouds are disabled due to player underground
	bool m_clouds_enabled = true; // Initialised to true, reset only by set_sky API
	bool m_directional_colored_fog;
	bool m_bodies_visible = true; // sun, moon, stars (disables the next three bools)
	bool m_dyanmic_visible = true; // control rendering the mesh skybox visible
	bool m_sun_visible = true; // render the sun
	bool m_sun_glow = true; // render the sunrise/set texture
	bool m_moon_visible = true; // render the moon
	bool m_stars_visible = true; // render the stars
	bool m_use_fog = false; // use only if you're using the combined skybox.

	video::SColorf m_bgcolor_bright_f = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	video::SColorf m_skycolor_bright_f = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	video::SColorf m_cloudcolor_bright_f = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	video::SColor m_bgcolor;
	video::SColor m_skycolor;
	video::SColorf m_cloudcolor_f;
	v3f m_stars[SKY_STAR_COUNT];
	video::ITexture *m_sun_texture;
	video::ITexture *m_moon_texture;
	video::ITexture *m_sun_tonemap;
	video::ITexture *m_moon_tonemap;
	std::string m_sun_name = "sun.png";
	std::string m_moon_name = "moon.png";
	f32 m_sun_yaw = 90;
	f32 m_sun_tilt = 0;
	f32 m_moon_yaw = -90;
	f32 m_moon_tilt = 0;
	f32 m_star_yaw = 90;
	f32 m_star_tilt = 0;
	u16 m_star_count = 200;
};
