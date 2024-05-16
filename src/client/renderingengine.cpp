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

#include <optional>
#include <irrlicht.h>
#include "fontengine.h"
#include "client.h"
#include "clouds.h"
#include "util/numeric.h"
#include "guiscalingfilter.h"
#include "localplayer.h"
#include "client/hud.h"
#include "camera.h"
#include "minimap.h"
#include "clientmap.h"
#include "renderingengine.h"
#include "render/core.h"
#include "render/factory.h"
#include "inputhandler.h"
#include "gettext.h"
#include "filesys.h"
#include "../gui/guiSkin.h"
#include "irrlicht_changes/static_text.h"
#include "irr_ptr.h"

RenderingEngine *RenderingEngine::s_singleton = nullptr;
const video::SColor RenderingEngine::MENU_SKY_COLOR = video::SColor(255, 140, 186, 250);
const float RenderingEngine::BASE_BLOOM_STRENGTH = 1.0f;

/* Helper classes */

void FpsControl::reset()
{
	last_time = porting::getTimeUs();
}

void FpsControl::limit(IrrlichtDevice *device, f32 *dtime, bool assume_paused)
{
	const float fps_limit = (device->isWindowFocused() && !assume_paused)
			? g_settings->getFloat("fps_max")
			: g_settings->getFloat("fps_max_unfocused");
	const u64 frametime_min = 1000000.0f / std::max(fps_limit, 1.0f);

	u64 time = porting::getTimeUs();

	if (time > last_time) // Make sure time hasn't overflowed
		busy_time = time - last_time;
	else
		busy_time = 0;

	if (busy_time < frametime_min) {
		sleep_time = frametime_min - busy_time;
		if (sleep_time > 0)
			sleep_us(sleep_time);
	} else {
		sleep_time = 0;
	}

	// Read the timer again to accurately determine how long we actually slept,
	// rather than calculating it by adding sleep_time to time.
	time = porting::getTimeUs();

	if (time > last_time) // Make sure last_time hasn't overflowed
		*dtime = (time - last_time) / 1000000.0f;
	else
		*dtime = 0;

	last_time = time;
}

class FogShaderConstantSetter : public IShaderConstantSetter
{
	CachedPixelShaderSetting<float, 4> m_fog_color{"fogColor"};
	CachedPixelShaderSetting<float> m_fog_distance{"fogDistance"};
	CachedPixelShaderSetting<float> m_fog_shading_parameter{"fogShadingParameter"};

public:
	void onSetConstants(video::IMaterialRendererServices *services) override
	{
		auto *driver = services->getVideoDriver();
		assert(driver);

		video::SColor fog_color(0);
		video::E_FOG_TYPE fog_type = video::EFT_FOG_LINEAR;
		f32 fog_start = 0;
		f32 fog_end = 0;
		f32 fog_density = 0;
		bool fog_pixelfog = false;
		bool fog_rangefog = false;
		driver->getFog(fog_color, fog_type, fog_start, fog_end, fog_density,
				fog_pixelfog, fog_rangefog);

		video::SColorf fog_colorf(fog_color);
		m_fog_color.set(fog_colorf, services);

		m_fog_distance.set(&fog_end, services);

		float parameter = 0;
		if (fog_end > 0)
			parameter = 1.0f / (1.0f - fog_start / fog_end);
		m_fog_shading_parameter.set(&parameter, services);
	}
};

IShaderConstantSetter *FogShaderConstantSetterFactory::create()
{
	return new FogShaderConstantSetter();
}

/* Other helpers */

static gui::GUISkin *createSkin(gui::IGUIEnvironment *environment,
		gui::EGUI_SKIN_TYPE type, video::IVideoDriver *driver)
{
	gui::GUISkin *skin = new gui::GUISkin(type, driver);

	gui::IGUIFont *builtinfont = environment->getBuiltInFont();
	gui::IGUIFontBitmap *bitfont = nullptr;
	if (builtinfont && builtinfont->getType() == gui::EGFT_BITMAP)
		bitfont = (gui::IGUIFontBitmap*)builtinfont;

	gui::IGUISpriteBank *bank = 0;
	skin->setFont(builtinfont);

	if (bitfont)
		bank = bitfont->getSpriteBank();

	skin->setSpriteBank(bank);

	return skin;
}

static std::optional<video::E_DRIVER_TYPE> chooseVideoDriver()
{
	auto &&configured_name = g_settings->get("video_driver");
	if (configured_name.empty())
		return std::nullopt;

	auto &&drivers = RenderingEngine::getSupportedVideoDrivers();
	for (auto driver: drivers) {
		auto &&info = RenderingEngine::getVideoDriverInfo(driver);
		if (!strcasecmp(configured_name.c_str(), info.name.c_str()))
			return driver;
	}

	errorstream << "Invalid video_driver specified: " << configured_name << std::endl;
	return std::nullopt;
}

static inline auto getVideoDriverName(video::E_DRIVER_TYPE driver)
{
	return RenderingEngine::getVideoDriverInfo(driver).friendly_name;
}

static irr::IrrlichtDevice *createDevice(SIrrlichtCreationParameters params, std::optional<video::E_DRIVER_TYPE> requested_driver)
{
	if (requested_driver) {
		params.DriverType = *requested_driver;
		verbosestream << "Trying video driver " << getVideoDriverName(params.DriverType) << std::endl;
		if (auto *device = createDeviceEx(params))
			return device;
		errorstream << "Failed to initialize the " << getVideoDriverName(params.DriverType) << " video driver" << std::endl;
	}
	sanity_check(requested_driver != video::EDT_NULL);

	// try to find any working video driver
	for (auto fallback_driver: RenderingEngine::getSupportedVideoDrivers()) {
		if (fallback_driver == video::EDT_NULL || fallback_driver == requested_driver)
			continue;
		params.DriverType = fallback_driver;
		verbosestream << "Trying video driver " << getVideoDriverName(params.DriverType) << std::endl;
		if (auto *device = createDeviceEx(params))
			return device;
	}

	throw std::runtime_error("Could not initialize the device with any supported video driver");
}

/* RenderingEngine class */

RenderingEngine::RenderingEngine(IEventReceiver *receiver)
{
	sanity_check(!s_singleton);

	// Resolution selection
	bool fullscreen = g_settings->getBool("fullscreen");
#ifdef __ANDROID__
	u16 screen_w = 0, screen_h = 0;
	bool window_maximized = false;
#else
	u16 screen_w = std::max<u16>(g_settings->getU16("screen_w"), 1);
	u16 screen_h = std::max<u16>(g_settings->getU16("screen_h"), 1);
	bool window_maximized = g_settings->getBool("window_maximized");
#endif

	// bpp, fsaa, vsync
	bool vsync = g_settings->getBool("vsync");
	bool enable_fsaa = g_settings->get("antialiasing") == "fsaa";
	u16 fsaa = enable_fsaa ? MYMAX(2, g_settings->getU16("fsaa")) : 0;

	// Determine driver
	auto driverType = chooseVideoDriver();

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	if (tracestream)
		params.LoggingLevel = irr::ELL_DEBUG;
	params.WindowSize = core::dimension2d<u32>(screen_w, screen_h);
	params.AntiAlias = fsaa;
	params.Fullscreen = fullscreen;
	params.WindowMaximized = window_maximized;
	params.WindowResizable = 1; // 1 means always (required for window_maximized)
	params.Stencilbuffer = false;
	params.Vsync = vsync;
	params.EventReceiver = receiver;
	params.DriverDebug = g_settings->getBool("opengl_debug");

	// there is no standardized path for these on desktop
	std::string rel_path = std::string("client") + DIR_DELIM
			+ "shaders" + DIR_DELIM + "Irrlicht";
	params.OGLES2ShaderPath = (porting::path_share + DIR_DELIM + rel_path + DIR_DELIM).c_str();

	m_device = createDevice(params, driverType);
	driver = m_device->getVideoDriver();
	verbosestream << "Using the " << getVideoDriverName(driver->getDriverType()) << " video driver" << std::endl;

	// This changes the minimum allowed number of vertices in a VBO. Default is 500.
	driver->setMinHardwareBufferVertexCount(4);

	s_singleton = this;

	auto skin = createSkin(m_device->getGUIEnvironment(),
			gui::EGST_WINDOWS_METALLIC, driver);
	m_device->getGUIEnvironment()->setSkin(skin);
	skin->drop();
}

RenderingEngine::~RenderingEngine()
{
	sanity_check(s_singleton == this);

	core.reset();
	m_device->closeDevice();
	m_device->drop();
	s_singleton = nullptr;
}

v2u32 RenderingEngine::_getWindowSize() const
{
	if (core)
		return core->getVirtualSize();
	return m_device->getVideoDriver()->getScreenSize();
}

void RenderingEngine::setResizable(bool resize)
{
	m_device->setResizable(resize);
}

void RenderingEngine::removeMesh(const scene::IMesh* mesh)
{
	m_device->getSceneManager()->getMeshCache()->removeMesh(mesh);
}

void RenderingEngine::cleanupMeshCache()
{
	auto mesh_cache = m_device->getSceneManager()->getMeshCache();
	mesh_cache->clear();
}

bool RenderingEngine::setupTopLevelWindow()
{
	return setWindowIcon();
}

bool RenderingEngine::setWindowIcon()
{
	irr_ptr<video::IImage> img(driver->createImageFromFile(
			(porting::path_share + "/textures/base/pack/logo.png").c_str()));
	if (!img) {
		warningstream << "Could not load icon file." << std::endl;
		return false;
	}

	return m_device->setWindowIcon(img.get());
}

/*
	Draws a screen with a single text on it.
	Text will be removed when the screen is drawn the next time.
	Additionally, a progressbar can be drawn when percent is set between 0 and 100.
*/
void RenderingEngine::draw_load_screen(const std::wstring &text,
		gui::IGUIEnvironment *guienv, ITextureSource *tsrc, float dtime,
		int percent, bool sky)
{
	v2u32 screensize = getWindowSize();

	v2s32 textsize(g_fontengine->getTextWidth(text), g_fontengine->getLineHeight());
	v2s32 center(screensize.X / 2, screensize.Y / 2);
	core::rect<s32> textrect(center - textsize / 2, center + textsize / 2);

	gui::IGUIStaticText *guitext =
			gui::StaticText::add(guienv, text, textrect, false, false);
	guitext->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);

	auto *driver = get_video_driver();

	if (sky) {
		driver->beginScene(true, true, RenderingEngine::MENU_SKY_COLOR);
		if (g_settings->getBool("menu_clouds")) {
			g_menuclouds->step(dtime * 3);
			g_menucloudsmgr->drawAll();
		}
	} else {
		driver->beginScene(true, true, video::SColor(255, 0, 0, 0));
	}

	// draw progress bar
	if ((percent >= 0) && (percent <= 100)) {
		video::ITexture *progress_img = tsrc->getTexture("progress_bar.png");
		video::ITexture *progress_img_bg =
				tsrc->getTexture("progress_bar_bg.png");

		if (progress_img && progress_img_bg) {
#ifndef __ANDROID__
			const core::dimension2d<u32> &img_size =
					progress_img_bg->getSize();
			float density = g_settings->getFloat("gui_scaling", 0.5f, 20.0f) *
					getDisplayDensity();
			u32 imgW = rangelim(img_size.Width, 200, 600) * density;
			u32 imgH = rangelim(img_size.Height, 24, 72) * density;
#else
			const core::dimension2d<u32> img_size(256, 48);
			float imgRatio = (float)img_size.Height / img_size.Width;
			u32 imgW = screensize.X / 2.2f;
			u32 imgH = floor(imgW * imgRatio);
#endif
			v2s32 img_pos((screensize.X - imgW) / 2,
					(screensize.Y - imgH) / 2);

			draw2DImageFilterScaled(get_video_driver(), progress_img_bg,
					core::rect<s32>(img_pos.X, img_pos.Y,
							img_pos.X + imgW,
							img_pos.Y + imgH),
					core::rect<s32>(0, 0, img_size.Width,
							img_size.Height),
					0, 0, true);

			draw2DImageFilterScaled(get_video_driver(), progress_img,
					core::rect<s32>(img_pos.X, img_pos.Y,
							img_pos.X + (percent * imgW) / 100,
							img_pos.Y + imgH),
					core::rect<s32>(0, 0,
							(percent * img_size.Width) / 100,
							img_size.Height),
					0, 0, true);
		}
	}

	guienv->drawAll();
	driver->endScene();
	guitext->remove();
}

std::vector<video::E_DRIVER_TYPE> RenderingEngine::getSupportedVideoDrivers()
{
	// Only check these drivers. We do not support software and D3D in any capacity.
	// Order by preference (best first)
	static const video::E_DRIVER_TYPE glDrivers[] = {
		video::EDT_OPENGL,
		video::EDT_OPENGL3,
		video::EDT_OGLES2,
		video::EDT_OGLES1,
		video::EDT_NULL,
	};
	std::vector<video::E_DRIVER_TYPE> drivers;

	for (video::E_DRIVER_TYPE driver: glDrivers) {
		if (IrrlichtDevice::isDriverSupported(driver))
			drivers.push_back(driver);
	}

	return drivers;
}

void RenderingEngine::initialize(Client *client, Hud *hud)
{
	const std::string &draw_mode = g_settings->get("3d_mode");
	core.reset(createRenderingCore(draw_mode, m_device, client, hud));
}

void RenderingEngine::finalize()
{
	core.reset();
}

void RenderingEngine::draw_scene(video::SColor skycolor, bool show_hud,
		bool draw_wield_tool, bool draw_crosshair)
{
	core->draw(skycolor, show_hud, draw_wield_tool, draw_crosshair);
}

const VideoDriverInfo &RenderingEngine::getVideoDriverInfo(irr::video::E_DRIVER_TYPE type)
{
	static const std::unordered_map<int, VideoDriverInfo> driver_info_map = {
		{(int)video::EDT_NULL,   {"null",   "NULL Driver"}},
		{(int)video::EDT_OPENGL, {"opengl", "OpenGL"}},
		{(int)video::EDT_OPENGL3, {"opengl3", "OpenGL 3+"}},
		{(int)video::EDT_OGLES1, {"ogles1", "OpenGL ES1"}},
		{(int)video::EDT_OGLES2, {"ogles2", "OpenGL ES2"}},
	};
	return driver_info_map.at((int)type);
}

float RenderingEngine::getDisplayDensity()
{
#ifndef __ANDROID__
	static float cached_display_density = [&] {
		float dpi = get_raw_device()->getDisplayDensity();
		// fall back to manually specified dpi
		if (dpi == 0.0f)
			dpi = g_settings->getFloat("screen_dpi");
		return dpi / 96.0f;
	}();
	return std::max(cached_display_density * g_settings->getFloat("display_density_factor"), 0.5f);

#else // __ANDROID__
	return porting::getDisplayDensity();
#endif // __ANDROID__
}

void RenderingEngine::autosaveScreensizeAndCo(
		const irr::core::dimension2d<u32> initial_screen_size,
		const bool initial_window_maximized)
{
	if (!g_settings->getBool("autosave_screensize"))
		return;

	// Note: If the screensize or similar hasn't changed (i.e. it's the same as
	// the setting was when minetest started, as given by the initial_* parameters),
	// we do not want to save the thing. This allows users to also manually change
	// the settings.

	// Screen size
	const irr::core::dimension2d<u32> current_screen_size =
		RenderingEngine::get_video_driver()->getScreenSize();
	// Don't replace good value with (0, 0)
	if (current_screen_size != irr::core::dimension2d<u32>(0, 0) &&
			current_screen_size != initial_screen_size) {
		g_settings->setU16("screen_w", current_screen_size.Width);
		g_settings->setU16("screen_h", current_screen_size.Height);
	}

	// Window maximized
	const bool is_window_maximized = RenderingEngine::get_raw_device()
			->isWindowMaximized();
	if (is_window_maximized != initial_window_maximized)
		g_settings->setBool("window_maximized", is_window_maximized);
}
