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
#ifndef GAMEGLOBALSHADERCONSTANTSETTER_HPP_
#define GAMEGLOBALSHADERCONSTANTSETTER_HPP_

#include <vector>

#include "client/client.h"
#include "client/minimap.h"
#include "client/shader.h"
#include "client/sky.h"
#include "irrlichttypes.h"

// Before 1.8 there isn't a "integer interface", only float
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
typedef f32 SamplerLayer_t;
#else
typedef s32 SamplerLayer_t;
#endif

class GameGlobalShaderConstantSetter : public IShaderConstantSetter
{
	Sky *m_sky;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	bool m_fog_enabled;
	CachedPixelShaderSetting<float, 4> m_sky_bg_color;
	CachedPixelShaderSetting<float> m_fog_distance;
	CachedVertexShaderSetting<float> m_animation_timer_vertex;
	CachedPixelShaderSetting<float> m_animation_timer_pixel;
	CachedPixelShaderSetting<float, 3> m_day_light;
	CachedPixelShaderSetting<float, 3> m_eye_position_pixel;
	CachedVertexShaderSetting<float, 3> m_eye_position_vertex;
	CachedPixelShaderSetting<float, 3> m_minimap_yaw;
	CachedPixelShaderSetting<SamplerLayer_t> m_base_texture;
	CachedPixelShaderSetting<SamplerLayer_t> m_normal_texture;
	CachedPixelShaderSetting<SamplerLayer_t> m_texture_flags;
	Client *m_client;

public:
	void onSettingsChange(const std::string &name)
	{
		if (name == "enable_fog")
			m_fog_enabled = g_settings->getBool("enable_fog");
	}

	static void settingsCallback(const std::string &name, void *userdata)
	{
		reinterpret_cast<GameGlobalShaderConstantSetter*>(userdata)->onSettingsChange(name);
	}

	void setSky(Sky *sky) { m_sky = sky; }

	GameGlobalShaderConstantSetter(Sky *sky, bool *force_fog_off,
			f32 *fog_range, Client *client) :
		m_sky(sky),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_sky_bg_color("skyBgColor"),
		m_fog_distance("fogDistance"),
		m_animation_timer_vertex("animationTimer"),
		m_animation_timer_pixel("animationTimer"),
		m_day_light("dayLight"),
		m_eye_position_pixel("eyePosition"),
		m_eye_position_vertex("eyePosition"),
		m_minimap_yaw("yawVec"),
		m_base_texture("baseTexture"),
		m_normal_texture("normalTexture"),
		m_texture_flags("textureFlags"),
		m_client(client)
	{
		g_settings->registerChangedCallback("enable_fog", settingsCallback, this);
		m_fog_enabled = g_settings->getBool("enable_fog");
	}

	~GameGlobalShaderConstantSetter()
	{
		g_settings->deregisterChangedCallback("enable_fog", settingsCallback, this);
	}

	virtual void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel)
	{
		if (!is_highlevel)
			return;

		// Background color
		video::SColor bgcolor = m_sky->getBgColor();
		video::SColorf bgcolorf(bgcolor);
		float bgcolorfa[4] = {
			bgcolorf.r,
			bgcolorf.g,
			bgcolorf.b,
			bgcolorf.a,
		};
		m_sky_bg_color.set(bgcolorfa, services);

		// Fog distance
		float fog_distance = 10000 * BS;

		if (m_fog_enabled && !*m_force_fog_off)
			fog_distance = *m_fog_range;

		m_fog_distance.set(&fog_distance, services);

		u32 daynight_ratio = (float)m_client->getEnv().getDayNightRatio();
		video::SColorf sunlight;
		get_sunlight_color(&sunlight, daynight_ratio);
		float dnc[3] = {
			sunlight.r,
			sunlight.g,
			sunlight.b };
		m_day_light.set(dnc, services);

		u32 animation_timer = porting::getTimeMs() % 100000;
		float animation_timer_f = (float)animation_timer / 100000.f;
		m_animation_timer_vertex.set(&animation_timer_f, services);
		m_animation_timer_pixel.set(&animation_timer_f, services);

		float eye_position_array[3];
		v3f epos = m_client->getEnv().getLocalPlayer()->getEyePosition();
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
		eye_position_array[0] = epos.X;
		eye_position_array[1] = epos.Y;
		eye_position_array[2] = epos.Z;
#else
		epos.getAs3Values(eye_position_array);
#endif
		m_eye_position_pixel.set(eye_position_array, services);
		m_eye_position_vertex.set(eye_position_array, services);

		if (m_client->getMinimap()) {
			float minimap_yaw_array[3];
			v3f minimap_yaw = m_client->getMinimap()->getYawVec();
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
			minimap_yaw_array[0] = minimap_yaw.X;
			minimap_yaw_array[1] = minimap_yaw.Y;
			minimap_yaw_array[2] = minimap_yaw.Z;
#else
			minimap_yaw.getAs3Values(minimap_yaw_array);
#endif
			m_minimap_yaw.set(minimap_yaw_array, services);
		}

		SamplerLayer_t base_tex = 0,
				normal_tex = 1,
				flags_tex = 2;
		m_base_texture.set(&base_tex, services);
		m_normal_texture.set(&normal_tex, services);
		m_texture_flags.set(&flags_tex, services);
	}
};

class GameGlobalShaderConstantSetterFactory : public IShaderConstantSetterFactory
{
	Sky *m_sky;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	Client *m_client;
	std::vector<GameGlobalShaderConstantSetter *> created_nosky;
public:
	GameGlobalShaderConstantSetterFactory(bool *force_fog_off,
			f32 *fog_range, Client *client) :
		m_sky(NULL),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_client(client)
	{}

	void setSky(Sky *sky) {
		m_sky = sky;
		for (GameGlobalShaderConstantSetter *ggscs : created_nosky) {
			ggscs->setSky(m_sky);
		}
		created_nosky.clear();
	}

	virtual IShaderConstantSetter* create()
	{
		GameGlobalShaderConstantSetter *scs = new GameGlobalShaderConstantSetter(
				m_sky, m_force_fog_off, m_fog_range, m_client);
		if (!m_sky)
			created_nosky.push_back(scs);
		return scs;
	}
};

#endif // GAMEGLOBALSHADERCONSTANTSETTER_HPP_
