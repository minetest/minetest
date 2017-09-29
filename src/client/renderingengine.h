/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <vector>
#include <string>
#include "irrlichttypes_extrabloated.h"
#include "debug.h"

class ITextureSource;
class Camera;
class Client;
class LocalPlayer;
class Hud;
class Minimap;

class RenderingEngine
{
public:
	RenderingEngine(IEventReceiver *eventReceiver);
	~RenderingEngine();

	v2u32 getWindowSize() const;
	void setResizable(bool resize);

	video::IVideoDriver *getVideoDriver();

	static const char *getVideoDriverName(irr::video::E_DRIVER_TYPE type);
	static const char *getVideoDriverFriendlyName(irr::video::E_DRIVER_TYPE type);
	static float getDisplayDensity();
	static v2u32 getDisplaySize();

	static void setXorgClassHint(const video::SExposedVideoData &video_data,
			const std::string &name);
	bool setWindowIcon();
	bool setXorgWindowIconFromPath(const std::string &icon_file);
	static bool print_video_modes();

	static RenderingEngine *get_instance() { return s_singleton; }

	static io::IFileSystem *get_filesystem()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getFileSystem();
	}

	static video::IVideoDriver *get_video_driver()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getVideoDriver();
	}

	static scene::IMeshCache *get_mesh_cache()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getSceneManager()->getMeshCache();
	}

	static scene::ISceneManager *get_scene_manager()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getSceneManager();
	}

	static irr::IrrlichtDevice *get_raw_device()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device;
	}

	static u32 get_timer_time()
	{
		sanity_check(s_singleton && s_singleton->m_device &&
				s_singleton->m_device->getTimer());
		return s_singleton->m_device->getTimer()->getTime();
	}

	static gui::IGUIEnvironment *get_gui_env()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getGUIEnvironment();
	}

	inline static void draw_load_screen(const std::wstring &text,
			gui::IGUIEnvironment *guienv, ITextureSource *tsrc,
			float dtime = 0, int percent = 0, bool clouds = true)
	{
		s_singleton->_draw_load_screen(
				text, guienv, tsrc, dtime, percent, clouds);
	}

	inline static void draw_scene(Camera *camera, Client *client, LocalPlayer *player,
			Hud *hud, Minimap *mapper, gui::IGUIEnvironment *guienv,
			const v2u32 &screensize, const video::SColor &skycolor,
			bool show_hud, bool show_minimap)
	{
		s_singleton->_draw_scene(camera, client, player, hud, mapper, guienv,
				screensize, skycolor, show_hud, show_minimap);
	}

	static bool run()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->run();
	}

	static std::vector<core::vector3d<u32>> getSupportedVideoModes();
	static std::vector<irr::video::E_DRIVER_TYPE> getSupportedVideoDrivers();

private:
	enum parallax_sign
	{
		LEFT = -1,
		RIGHT = 1,
		EYECOUNT = 2
	};

	void _draw_load_screen(const std::wstring &text, gui::IGUIEnvironment *guienv,
			ITextureSource *tsrc, float dtime = 0, int percent = 0,
			bool clouds = true);

	void _draw_scene(Camera *camera, Client *client, LocalPlayer *player, Hud *hud,
			Minimap *mapper, gui::IGUIEnvironment *guienv,
			const v2u32 &screensize, const video::SColor &skycolor,
			bool show_hud, bool show_minimap);

	void draw_anaglyph_3d_mode(Camera *camera, bool show_hud, Hud *hud,
			bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv);

	void draw_interlaced_3d_mode(Camera *camera, bool show_hud, Hud *hud,
			const v2u32 &screensize, bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv, const video::SColor &skycolor);

	void draw_sidebyside_3d_mode(Camera *camera, bool show_hud, Hud *hud,
			const v2u32 &screensize, bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv, const video::SColor &skycolor);

	void draw_top_bottom_3d_mode(Camera *camera, bool show_hud, Hud *hud,
			const v2u32 &screensize, bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv, const video::SColor &skycolor);

	void draw_pageflip_3d_mode(Camera *camera, bool show_hud, Hud *hud,
			const v2u32 &screensize, bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv, const video::SColor &skycolor);

	void draw_plain(Camera *camera, bool show_hud, Hud *hud, const v2u32 &screensize,
			bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv, const video::SColor &skycolor);

	void init_texture(const v2u32 &screensize, video::ITexture **texture,
			const char *name);

	video::ITexture *draw_image(const v2u32 &screensize, parallax_sign psign,
			const irr::core::matrix4 &startMatrix,
			const irr::core::vector3df &focusPoint, bool show_hud,
			Camera *camera, Hud *hud, bool draw_wield_tool, Client *client,
			gui::IGUIEnvironment *guienv, const video::SColor &skycolor);

	video::ITexture *draw_hud(const v2u32 &screensize, bool show_hud, Hud *hud,
			Client *client, bool draw_crosshair,
			const video::SColor &skycolor, gui::IGUIEnvironment *guienv,
			Camera *camera);

	irr::IrrlichtDevice *m_device = nullptr;
	static RenderingEngine *s_singleton;
};
