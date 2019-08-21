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
#include <array>
#include "camera.h"
#include "irrlichttypes_extrabloated.h"

#pragma once

#define SKY_MATERIAL_COUNT 12

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
	
	void setSunVisible(bool sun_visible) { m_sun_visible = sun_visible; }
	void setSunTexture(std::string sun_texture, 
		std::string sun_tonemap, ITextureSource *tsrc)
	{
		// Ignore matching textures (with modifiers) entirely,
		// but lets at least update the tonemap before hand.
		m_sun_tonemap_name = sun_tonemap;
		m_sun_tonemap = tsrc->isKnownSourceImage(m_sun_tonemap_name) ?
			tsrc->getTexture(m_sun_tonemap_name) : NULL;
		if (m_sun_tonemap)
			m_materials[3].Lighting = true;
		else
			m_materials[3].Lighting = false;
		
		if (m_sun_name == sun_texture)
			return;
		m_sun_name = sun_texture;

		if (sun_texture != "") {
			// We want to ensure the texture exists first.
			m_sun_texture = tsrc->getTextureForMesh(m_sun_name);

			if (m_sun_texture) {
				m_materials[3] = m_materials[0];
				m_materials[3].setTexture(0, m_sun_texture);
				m_materials[3].MaterialType = video::
					EMT_TRANSPARENT_ALPHA_CHANNEL;
				// Settings to enable or disable texture filtering
				// on a per-mod basis.
				m_materials[3].setFlag(video::E_MATERIAL_FLAG(0x100), m_sun_billinear);
				m_materials[3].setFlag(video::E_MATERIAL_FLAG(0x200), false);
				m_materials[3].setFlag(video::E_MATERIAL_FLAG(0x400), m_sun_antiso);
			}
		} else {
			m_sun_texture = nullptr;
		}
	}
	void setSunYaw(f32 sun_yaw) { m_sun_rotation = sun_yaw; }
	void setSunScale(f32 sun_scale) { m_sun_scale = sun_scale; }
	void setSunriseVisible(bool glow_visible) { m_sunglow_visible = glow_visible; }
	void setSunriseTexture(std::string sunglow_texture, ITextureSource* tsrc)
	{
		// Ignore matching textures (with modifiers) entirely.
		if (m_sunrise_name == sunglow_texture)
			return;

		m_sunrise_name = sunglow_texture;

		if (sunglow_texture != "") {
			// We want to ensure the texture exists first.
			//m_sunrise_texture = tsrc->getTextureForMesh(m_sunrise_name);
			m_materials[2].setTexture(0, tsrc->getTextureForMesh(m_sunrise_name));
		} else {
			//m_sunrise_texture = tsrc->getTextureForMesh("sunrisebg.png");
			m_materials[2].setTexture(0, tsrc->getTextureForMesh("sunrisebg.png"));
		}
	}

	void setMoonVisible(bool moon_visible) { m_moon_visible = moon_visible; }
	void setMoonTexture(std::string moon_texture, 
		std::string moon_tonemap, ITextureSource *tsrc)
	{
		// Ignore matching textures (with modifiers) entirely,
		// but lets at least update the tonemap before hand.
		m_moon_tonemap_name = moon_tonemap;
		m_moon_tonemap = tsrc->isKnownSourceImage(m_moon_tonemap_name) ?
			tsrc->getTexture(m_moon_tonemap_name) : NULL;
		if (m_moon_tonemap)
			m_materials[4].Lighting = true;
		else
			m_materials[4].Lighting = false;

		if (m_moon_name == moon_texture)
			return;
		m_moon_name = moon_texture;

		if (moon_texture != "") {
			// We want to ensure the texture exists first.
			m_moon_texture = tsrc->getTextureForMesh(m_moon_name);
			
			if (m_moon_texture) {
				m_materials[4] = m_materials[0];
				m_materials[4].setTexture(0, m_moon_texture);
				m_materials[4].MaterialType = video::
					EMT_TRANSPARENT_ALPHA_CHANNEL;
				// Settings to enable or disable texture filtering
				// on a per-mod basis.
				m_materials[4].setFlag(video::E_MATERIAL_FLAG(0x100), m_moon_billinear);
				m_materials[4].setFlag(video::E_MATERIAL_FLAG(0x200), false);
				m_materials[4].setFlag(video::E_MATERIAL_FLAG(0x400), m_moon_antiso);
			}
		} else {
			m_moon_texture = nullptr;
		}
	}
	void setMoonYaw(f32 moon_yaw) { m_moon_rotation = moon_yaw; }
	void setMoonScale(f32 moon_scale) { m_moon_scale = moon_scale; }

	void setStarsVisible(bool stars_visible) { m_stars_visible = stars_visible; }
	void setStarCount(u16 star_count)
	{
		// Ignore changing star count if the new value is identical
		if (m_star_count == star_count)
			return;

		m_star_count = star_count;
		m_stars.clear();
		// Rebuild the stars surrounding the camera
		for (u16 i=0; i < star_count; i++) {
			v3f star = v3f(
				myrand_range(-10000, 10000),
				myrand_range(-10000, 10000),
				myrand_range(-10000, 10000)
			);

			star.normalize();
			m_stars.emplace_back(star);
		}
	}
	void setStarColor(video::SColor star_color) { m_starcolor = star_color; }
	void setStarYaw(f32 star_yaw) { m_star_rotation = star_yaw; }
	void setStarScale(f32 star_scale) { m_star_scale = star_scale; }

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
	void updateSkyColors(video::SColor bg_day, video::SColor bg_dawn,
		video::SColor bg_night, video::SColor sky_day,
		video::SColor sky_dawn, video::SColor sky_night,
		video::SColor bg_indoor) {

		// Bottom half of the skybox
		m_bgcolor_day_f = bg_day;
		m_bgcolor_dawn_f = bg_dawn;
		m_bgcolor_night_f = bg_night;
		// Top half of the skybox
		m_skycolor_day_f = sky_day;
		m_skycolor_dawn_f = sky_dawn;
		m_skycolor_night_f = sky_night;
		// Indoors
		m_bgcolor_indoor_f = bg_indoor;
	}
	void updateHorizonTint(video::SColor sun_tint, video::SColor moon_tint,
		std::string use_sun_tint) {
		// Change sun and moon tinting:
		m_pointcolor_sun_f = sun_tint;
		m_pointcolor_moon_f = moon_tint;
		// Faster than comparing strings every rendering frame
		if (use_sun_tint == "default")
			m_default_tint = true;
		else if (use_sun_tint == "custom")
			m_default_tint = false;
		else
			m_default_tint = true;
	}
	void setInClouds(bool clouds) { m_in_clouds = clouds; }
	void clearSkyboxTextures() { m_skybox_textures.clear(); }
	void addTextureToSkybox(std::string texture, int material_id,
		ITextureSource *tsrc) {
		// Keep a list of texture names handy.
		m_skybox_textures.emplace_back(texture);
		video::ITexture *result = tsrc->getTextureForMesh(texture);
		m_materials[material_id+5] = m_materials[0];
		m_materials[material_id+5].setTexture(0, result);
		m_materials[material_id+5].MaterialType = video::EMT_SOLID;
	}
	
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
	bool m_in_clouds = true; // Prevent duplicating bools to remember old values

	video::SColorf m_bgcolor_bright_f = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	video::SColorf m_skycolor_bright_f = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	video::SColorf m_cloudcolor_bright_f = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f);
	video::SColor m_bgcolor;
	video::SColor m_skycolor;
	video::SColorf m_cloudcolor_f;
	
	// Horizon colors
	//video::SColorf m_bgcolor_day_f = video::SColor(255, 255, 0, 255);
	video::SColorf m_bgcolor_day_f = video::SColor(255, 155, 193, 240);
	video::SColorf m_bgcolor_indoor_f = video::SColor(255, 100, 100, 100);
	video::SColorf m_bgcolor_dawn_f = video::SColor(255, 186, 193, 240);
	video::SColorf m_bgcolor_night_f = video::SColor(255, 64, 144, 255);
	// Sky colors
	video::SColorf m_skycolor_day_f = video::SColor(255, 140, 186, 250);
	video::SColorf m_skycolor_dawn_f = video::SColor(255, 180, 186, 250);
	video::SColorf m_skycolor_night_f = video::SColor(255, 0, 107, 255);

	// pure white: becomes "diffuse light component" for clouds
	video::SColorf m_cloudcolor_day_f = video::SColorf(1, 1, 1, 1);
	// dawn-factoring version of pure white (note: R is above 1.0)
	video::SColorf m_cloudcolor_dawn_f = video::SColorf(
		255.0f/240.0f,
		223.0f/240.0f,
		191.0f/255.0f
	);

	bool m_default_tint = true;
	
	bool m_sun_visible = true;
	bool m_sunglow_visible = true;
	std::string m_sun_name = "sun.png";
	std::string m_sun_tonemap_name = "sun_tonemap.png";
	std::string m_sunrise_name = "sunrisebg.png";
	video::SColorf m_pointcolor_sun_f;
	float m_sun_scale = 1;
	f32 m_sun_rotation = 90;
	bool m_sun_antiso = true;
	bool m_sun_billinear = true;

	bool m_moon_visible = true;
	std::string m_moon_name = "moon.png";
	std::string m_moon_tonemap_name = "moon_tonemap.png";
	video::SColorf m_pointcolor_moon_f;
	float m_moon_scale = 1;
	f32 m_moon_rotation = -90;
	bool m_moon_antiso = true;
	bool m_moon_billinear = true;

	bool m_stars_visible = true;
	std::vector<v3f> m_stars;
	float m_star_scale = 1;
	video::SColor m_starcolor = video::SColor(105, 235, 235, 255);
	f32 m_star_rotation = 0;
	u32 m_star_count = 0;

	video::ITexture *m_sun_texture;
	video::ITexture *m_sunrise_texture;
	video::ITexture *m_moon_texture;
	video::ITexture *m_sun_tonemap;
	video::ITexture *m_moon_tonemap;

	std::vector<std::string> m_skybox_textures;
	video::ITexture *m_skybox_top;
	video::ITexture *m_skybox_bottom;
	video::ITexture *m_skybox_left;
	video::ITexture *m_skybox_right;
	video::ITexture *m_skybox_front;
	video::ITexture *m_skybox_back;

	void draw_sun(video::IVideoDriver *driver, float sunsize, const video::SColor &suncolor,
		const video::SColor &suncolor2, float wicked_time_of_day);
	void draw_moon(video::IVideoDriver *driver, float moonsize, const video::SColor &mooncolor,
		const video::SColor &mooncolor2, float wicked_time_of_day);
	void draw_sky_body(std::array<video::S3DVertex, 4> &vertices,
		float pos_1, float pos_2, const video::SColor &c);
	void draw_stars(video::IVideoDriver *driver, float wicked_time_of_day);
	void place_sky_body(std::array<video::S3DVertex, 4> &vertices,
		float horizon_position,	float day_position);
};
