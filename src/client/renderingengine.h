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
#include <memory>
#include <string>
#include "irrlichttypes_extrabloated.h"
#include "debug.h"

class ITextureSource;
class Camera;
class Client;
class LocalPlayer;
class Hud;
class Minimap;

class RenderingCore;

class RenderingEngine
{
public:
	RenderingEngine(IEventReceiver *eventReceiver);
	~RenderingEngine();

	v2u32 getWindowSize() const;
	void setResizable(bool resize);

	video::IVideoDriver *getVideoDriver() { return driver; }

	static const char *getVideoDriverName(irr::video::E_DRIVER_TYPE type);
	static const char *getVideoDriverFriendlyName(irr::video::E_DRIVER_TYPE type);
	static float getDisplayDensity();
	static v2u32 getDisplaySize();

	bool setupTopLevelWindow(const std::string &name);
	void setupTopLevelXorgWindow(const std::string &name);
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

	inline static void draw_menu_scene(
			gui::IGUIEnvironment *guienv, float dtime, bool clouds)
	{
		s_singleton->_draw_menu_scene(guienv, dtime, clouds);
	}

	inline static void draw_scene(video::SColor skycolor, bool show_hud,
			bool show_minimap, bool draw_wield_tool, bool draw_crosshair)
	{
		s_singleton->_draw_scene(skycolor, show_hud, show_minimap,
				draw_wield_tool, draw_crosshair);
	}

	inline static void initialize(Client *client, Hud *hud)
	{
		s_singleton->_initialize(client, hud);
	}

	inline static void finalize() { s_singleton->_finalize(); }

	static bool run()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->run();
	}

	static std::vector<core::vector3d<u32>> getSupportedVideoModes();
	static std::vector<irr::video::E_DRIVER_TYPE> getSupportedVideoDrivers();

private:
	void _draw_load_screen(const std::wstring &text, gui::IGUIEnvironment *guienv,
			ITextureSource *tsrc, float dtime = 0, int percent = 0,
			bool clouds = true);

	void _draw_menu_scene(gui::IGUIEnvironment *guienv, float dtime = 0,
			bool clouds = true);

	void _draw_scene(video::SColor skycolor, bool show_hud, bool show_minimap,
			bool draw_wield_tool, bool draw_crosshair);

	void _initialize(Client *client, Hud *hud);

	void _finalize();

	std::unique_ptr<RenderingCore> core;
	irr::IrrlichtDevice *m_device = nullptr;
	irr::video::IVideoDriver *driver;
	static RenderingEngine *s_singleton;
};
