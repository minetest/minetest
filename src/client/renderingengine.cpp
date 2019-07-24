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

#include <IrrlichtDevice.h>
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
#include "../gui/guiSkin.h"

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__) && \
		!defined(SERVER) && !defined(__HAIKU__)
#define XORG_USED
#endif
#ifdef XORG_USED
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#endif

#if ENABLE_GLES
#include "filesys.h"
#endif

RenderingEngine *RenderingEngine::s_singleton = nullptr;


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


RenderingEngine::RenderingEngine(IEventReceiver *receiver)
{
	sanity_check(!s_singleton);

	// Resolution selection
	bool fullscreen = g_settings->getBool("fullscreen");
	u16 screen_w = g_settings->getU16("screen_w");
	u16 screen_h = g_settings->getU16("screen_h");

	// bpp, fsaa, vsync
	bool vsync = g_settings->getBool("vsync");
	u16 bits = g_settings->getU16("fullscreen_bpp");
	u16 fsaa = g_settings->getU16("fsaa");

	// stereo buffer required for pageflip stereo
	bool stereo_buffer = g_settings->get("3d_mode") == "pageflip";

	// Determine driver
	video::E_DRIVER_TYPE driverType = video::EDT_OPENGL;
	const std::string &driverstring = g_settings->get("video_driver");
	std::vector<video::E_DRIVER_TYPE> drivers =
			RenderingEngine::getSupportedVideoDrivers();
	u32 i;
	for (i = 0; i != drivers.size(); i++) {
		if (!strcasecmp(driverstring.c_str(),
				RenderingEngine::getVideoDriverName(drivers[i]))) {
			driverType = drivers[i];
			break;
		}
	}
	if (i == drivers.size()) {
		errorstream << "Invalid video_driver specified; "
			       "defaulting to opengl"
			    << std::endl;
	}

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType = driverType;
	params.WindowSize = core::dimension2d<u32>(screen_w, screen_h);
	params.Bits = bits;
	params.AntiAlias = fsaa;
	params.Fullscreen = fullscreen;
	params.Stencilbuffer = false;
	params.Stereobuffer = stereo_buffer;
	params.Vsync = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");
	params.ZBufferBits = 24;
#ifdef __ANDROID__
	// clang-format off
	params.PrivateData = porting::app_global;
	params.OGLES2ShaderPath = std::string(porting::path_user + DIR_DELIM + "media" +
		DIR_DELIM + "Shaders" + DIR_DELIM).c_str();
	// clang-format on
#elif ENABLE_GLES
	// there is no standardized path for these on desktop
	std::string rel_path = std::string("client") + DIR_DELIM
			+ "shaders" + DIR_DELIM + "Irrlicht";
	params.OGLES2ShaderPath = (porting::path_share + DIR_DELIM + rel_path + DIR_DELIM).c_str();
#endif

	m_device = createDeviceEx(params);
	driver = m_device->getVideoDriver();

	s_singleton = this;

	auto skin = createSkin(m_device->getGUIEnvironment(),
			gui::EGST_WINDOWS_METALLIC, driver);
	m_device->getGUIEnvironment()->setSkin(skin);
	skin->drop();
}

RenderingEngine::~RenderingEngine()
{
	core.reset();
	m_device->drop();
	s_singleton = nullptr;
}

v2u32 RenderingEngine::getWindowSize() const
{
	if (core)
		return core->getVirtualSize();
	return m_device->getVideoDriver()->getScreenSize();
}

void RenderingEngine::setResizable(bool resize)
{
	m_device->setResizable(resize);
}

bool RenderingEngine::print_video_modes()
{
	IrrlichtDevice *nulldevice;

	bool vsync = g_settings->getBool("vsync");
	u16 fsaa = g_settings->getU16("fsaa");
	MyEventReceiver *receiver = new MyEventReceiver();

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType = video::EDT_NULL;
	params.WindowSize = core::dimension2d<u32>(640, 480);
	params.Bits = 24;
	params.AntiAlias = fsaa;
	params.Fullscreen = false;
	params.Stencilbuffer = false;
	params.Vsync = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");

	nulldevice = createDeviceEx(params);

	if (!nulldevice) {
		delete receiver;
		return false;
	}

	std::cout << _("Available video modes (WxHxD):") << std::endl;

	video::IVideoModeList *videomode_list = nulldevice->getVideoModeList();

	if (videomode_list != NULL) {
		s32 videomode_count = videomode_list->getVideoModeCount();
		core::dimension2d<u32> videomode_res;
		s32 videomode_depth;
		for (s32 i = 0; i < videomode_count; ++i) {
			videomode_res = videomode_list->getVideoModeResolution(i);
			videomode_depth = videomode_list->getVideoModeDepth(i);
			std::cout << videomode_res.Width << "x" << videomode_res.Height
				  << "x" << videomode_depth << std::endl;
		}

		std::cout << _("Active video mode (WxHxD):") << std::endl;
		videomode_res = videomode_list->getDesktopResolution();
		videomode_depth = videomode_list->getDesktopDepth();
		std::cout << videomode_res.Width << "x" << videomode_res.Height << "x"
			  << videomode_depth << std::endl;
	}

	nulldevice->drop();
	delete receiver;

	return videomode_list != NULL;
}

bool RenderingEngine::setupTopLevelWindow(const std::string &name)
{
	// FIXME: It would make more sense for there to be a switch of some
	// sort here that would call the correct toplevel setup methods for
	// the environment Minetest is running in but for now not deviating
	// from the original pattern.

	/* Setting Xorg properties for the top level window */
	setupTopLevelXorgWindow(name);
	/* Done with Xorg properties */

	/* Setting general properties for the top level window */
	verbosestream << "Client: Configuring general top level"
		<< " window properties"
		<< std::endl;

	bool result = setWindowIcon();

	verbosestream << "Client: Finished configuring general top level"
		<< " window properties"
		<< std::endl;
	/* Done with general properties */

	// FIXME: setWindowIcon returns a bool result but it is unused.
	// For now continue to return this result.
	return result;
}

void RenderingEngine::setupTopLevelXorgWindow(const std::string &name)
{
#ifdef XORG_USED
	const video::SExposedVideoData exposedData = driver->getExposedVideoData();

	Display *x11_dpl = reinterpret_cast<Display *>(exposedData.OpenGLLinux.X11Display);
	if (x11_dpl == NULL) {
		warningstream << "Client: Could not find X11 Display in ExposedVideoData"
			<< std::endl;
		return;
	}

	verbosestream << "Client: Configuring Xorg specific top level"
		<< " window properties"
		<< std::endl;


	Window x11_win = reinterpret_cast<Window>(exposedData.OpenGLLinux.X11Window);

	// Set application name and class hints. For now name and class are the same.
	XClassHint *classhint = XAllocClassHint();
	classhint->res_name = const_cast<char *>(name.c_str());
	classhint->res_class = const_cast<char *>(name.c_str());

	XSetClassHint(x11_dpl, x11_win, classhint);
	XFree(classhint);

	// FIXME: In the future WMNormalHints should be set ... e.g see the
	// gtk/gdk code (gdk/x11/gdksurface-x11.c) for the setup_top_level
	// method. But for now (as it would require some significant changes)
	// leave the code as is.

	// The following is borrowed from the above gdk source for setting top
	// level windows. The source indicates and the Xlib docs suggest that
	// this will set the WM_CLIENT_MACHINE and WM_LOCAL_NAME. This will not
	// set the WM_CLIENT_MACHINE to a Fully Qualified Domain Name (FQDN) which is
	// required by the Extended Window Manager Hints (EWMH) spec when setting
	// the _NET_WM_PID (see further down) but running Minetest in an env
	// where the window manager is on another machine from Minetest (therefore
	// making the PID useless) is not expected to be a problem. Further
	// more, using gtk/gdk as the model it would seem that not using a FQDN is
	// not an issue for modern Xorg window managers.

	verbosestream << "Client: Setting Xorg window manager Properties"
		<< std::endl;

	XSetWMProperties (x11_dpl, x11_win, NULL, NULL, NULL, 0, NULL, NULL, NULL);

	// Set the _NET_WM_PID window property according to the EWMH spec. _NET_WM_PID
	// (in conjunction with WM_CLIENT_MACHINE) can be used by window managers to
	// force a shutdown of an application if it doesn't respond to the destroy
	// window message.

	verbosestream << "Client: Setting Xorg _NET_WM_PID extened window manager property"
		<< std::endl;

	Atom NET_WM_PID = XInternAtom(x11_dpl, "_NET_WM_PID", false);

	pid_t pid = getpid();
	infostream << "Client: PID is '" << static_cast<long>(pid) << "'"
		<< std::endl;

	XChangeProperty(x11_dpl, x11_win, NET_WM_PID,
			XA_CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&pid),1);

	// Set the WM_CLIENT_LEADER window property here. Minetest has only one
	// window and that window will always be the leader.

	verbosestream << "Client: Setting Xorg WM_CLIENT_LEADER property"
		<< std::endl;

	Atom WM_CLIENT_LEADER = XInternAtom(x11_dpl, "WM_CLIENT_LEADER", false);

	XChangeProperty (x11_dpl, x11_win, WM_CLIENT_LEADER,
		XA_WINDOW, 32, PropModeReplace,
		reinterpret_cast<unsigned char *>(&x11_win), 1);

	verbosestream << "Client: Finished configuring Xorg specific top level"
		<< " window properties"
		<< std::endl;
#endif
}


bool RenderingEngine::setWindowIcon()
{
#if defined(XORG_USED)
#if RUN_IN_PLACE
	return setXorgWindowIconFromPath(
			porting::path_share + "/misc/" PROJECT_NAME "-xorg-icon-128.png");
#else
	// We have semi-support for reading in-place data if we are
	// compiled with RUN_IN_PLACE. Don't break with this and
	// also try the path_share location.
	return setXorgWindowIconFromPath(
			       ICON_DIR "/hicolor/128x128/apps/" PROJECT_NAME ".png") ||
	       setXorgWindowIconFromPath(porting::path_share + "/misc/" PROJECT_NAME
							       "-xorg-icon-128.png");
#endif
#elif defined(_WIN32)
	const video::SExposedVideoData exposedData = driver->getExposedVideoData();
	HWND hWnd; // Window handle

	switch (driver->getDriverType()) {
	case video::EDT_DIRECT3D8:
		hWnd = reinterpret_cast<HWND>(exposedData.D3D8.HWnd);
		break;
	case video::EDT_DIRECT3D9:
		hWnd = reinterpret_cast<HWND>(exposedData.D3D9.HWnd);
		break;
	case video::EDT_OPENGL:
		hWnd = reinterpret_cast<HWND>(exposedData.OpenGLWin32.HWnd);
		break;
	default:
		return false;
	}

	// Load the ICON from resource file
	const HICON hicon = LoadIcon(GetModuleHandle(NULL),
			MAKEINTRESOURCE(130) // The ID of the ICON defined in
					     // winresource.rc
	);

	if (hicon) {
		SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hicon));
		SendMessage(hWnd, WM_SETICON, ICON_SMALL,
				reinterpret_cast<LPARAM>(hicon));
		return true;
	}
	return false;
#else
	return false;
#endif
}

bool RenderingEngine::setXorgWindowIconFromPath(const std::string &icon_file)
{
#ifdef XORG_USED

	video::IImageLoader *image_loader = NULL;
	u32 cnt = driver->getImageLoaderCount();
	for (u32 i = 0; i < cnt; i++) {
		if (driver->getImageLoader(i)->isALoadableFileExtension(
				    icon_file.c_str())) {
			image_loader = driver->getImageLoader(i);
			break;
		}
	}

	if (!image_loader) {
		warningstream << "Could not find image loader for file '" << icon_file
			      << "'" << std::endl;
		return false;
	}

	io::IReadFile *icon_f =
			m_device->getFileSystem()->createAndOpenFile(icon_file.c_str());

	if (!icon_f) {
		warningstream << "Could not load icon file '" << icon_file << "'"
			      << std::endl;
		return false;
	}

	video::IImage *img = image_loader->loadImage(icon_f);

	if (!img) {
		warningstream << "Could not load icon file '" << icon_file << "'"
			      << std::endl;
		icon_f->drop();
		return false;
	}

	u32 height = img->getDimension().Height;
	u32 width = img->getDimension().Width;

	size_t icon_buffer_len = 2 + height * width;
	long *icon_buffer = new long[icon_buffer_len];

	icon_buffer[0] = width;
	icon_buffer[1] = height;

	for (u32 x = 0; x < width; x++) {
		for (u32 y = 0; y < height; y++) {
			video::SColor col = img->getPixel(x, y);
			long pixel_val = 0;
			pixel_val |= (u8)col.getAlpha() << 24;
			pixel_val |= (u8)col.getRed() << 16;
			pixel_val |= (u8)col.getGreen() << 8;
			pixel_val |= (u8)col.getBlue();
			icon_buffer[2 + x + y * width] = pixel_val;
		}
	}

	img->drop();
	icon_f->drop();

	const video::SExposedVideoData &video_data = driver->getExposedVideoData();

	Display *x11_dpl = (Display *)video_data.OpenGLLinux.X11Display;

	if (x11_dpl == NULL) {
		warningstream << "Could not find x11 display for setting its icon."
			      << std::endl;
		delete[] icon_buffer;
		return false;
	}

	Window x11_win = (Window)video_data.OpenGLLinux.X11Window;

	Atom net_wm_icon = XInternAtom(x11_dpl, "_NET_WM_ICON", False);
	Atom cardinal = XInternAtom(x11_dpl, "CARDINAL", False);
	XChangeProperty(x11_dpl, x11_win, net_wm_icon, cardinal, 32, PropModeReplace,
			(const unsigned char *)icon_buffer, icon_buffer_len);

	delete[] icon_buffer;

#endif
	return true;
}

/*
	Draws a screen with a single text on it.
	Text will be removed when the screen is drawn the next time.
	Additionally, a progressbar can be drawn when percent is set between 0 and 100.
*/
void RenderingEngine::_draw_load_screen(const std::wstring &text,
		gui::IGUIEnvironment *guienv, ITextureSource *tsrc, float dtime,
		int percent, bool clouds)
{
	v2u32 screensize = RenderingEngine::get_instance()->getWindowSize();

	v2s32 textsize(g_fontengine->getTextWidth(text), g_fontengine->getLineHeight());
	v2s32 center(screensize.X / 2, screensize.Y / 2);
	core::rect<s32> textrect(center - textsize / 2, center + textsize / 2);

	gui::IGUIStaticText *guitext =
			guienv->addStaticText(text.c_str(), textrect, false, false);
	guitext->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);

	bool cloud_menu_background = clouds && g_settings->getBool("menu_clouds");
	if (cloud_menu_background) {
		g_menuclouds->step(dtime * 3);
		g_menuclouds->render();
		get_video_driver()->beginScene(
				true, true, video::SColor(255, 140, 186, 250));
		g_menucloudsmgr->drawAll();
	} else
		get_video_driver()->beginScene(true, true, video::SColor(255, 0, 0, 0));

	// draw progress bar
	if ((percent >= 0) && (percent <= 100)) {
		video::ITexture *progress_img = tsrc->getTexture("progress_bar.png");
		video::ITexture *progress_img_bg =
				tsrc->getTexture("progress_bar_bg.png");

		if (progress_img && progress_img_bg) {
#ifndef __ANDROID__
			const core::dimension2d<u32> &img_size =
					progress_img_bg->getSize();
			u32 imgW = rangelim(img_size.Width, 200, 600);
			u32 imgH = rangelim(img_size.Height, 24, 72);
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
	get_video_driver()->endScene();
	guitext->remove();
}

/*
	Draws the menu scene including (optional) cloud background.
*/
void RenderingEngine::_draw_menu_scene(gui::IGUIEnvironment *guienv,
		float dtime, bool clouds)
{
	bool cloud_menu_background = clouds && g_settings->getBool("menu_clouds");
	if (cloud_menu_background) {
		g_menuclouds->step(dtime * 3);
		g_menuclouds->render();
		get_video_driver()->beginScene(
				true, true, video::SColor(255, 140, 186, 250));
		g_menucloudsmgr->drawAll();
	} else
		get_video_driver()->beginScene(true, true, video::SColor(255, 0, 0, 0));

	guienv->drawAll();
	get_video_driver()->endScene();
}

std::vector<core::vector3d<u32>> RenderingEngine::getSupportedVideoModes()
{
	IrrlichtDevice *nulldevice = createDevice(video::EDT_NULL);
	sanity_check(nulldevice);

	std::vector<core::vector3d<u32>> mlist;
	video::IVideoModeList *modelist = nulldevice->getVideoModeList();

	s32 num_modes = modelist->getVideoModeCount();
	for (s32 i = 0; i != num_modes; i++) {
		core::dimension2d<u32> mode_res = modelist->getVideoModeResolution(i);
		u32 mode_depth = (u32)modelist->getVideoModeDepth(i);
		mlist.emplace_back(mode_res.Width, mode_res.Height, mode_depth);
	}

	nulldevice->drop();
	return mlist;
}

std::vector<irr::video::E_DRIVER_TYPE> RenderingEngine::getSupportedVideoDrivers()
{
	std::vector<irr::video::E_DRIVER_TYPE> drivers;

	for (int i = 0; i != irr::video::EDT_COUNT; i++) {
		if (irr::IrrlichtDevice::isDriverSupported((irr::video::E_DRIVER_TYPE)i))
			drivers.push_back((irr::video::E_DRIVER_TYPE)i);
	}

	return drivers;
}

void RenderingEngine::_initialize(Client *client, Hud *hud)
{
	const std::string &draw_mode = g_settings->get("3d_mode");
	core.reset(createRenderingCore(draw_mode, m_device, client, hud));
	core->initialize();
}

void RenderingEngine::_finalize()
{
	core.reset();
}

void RenderingEngine::_draw_scene(video::SColor skycolor, bool show_hud,
		bool show_minimap, bool draw_wield_tool, bool draw_crosshair)
{
	core->draw(skycolor, show_hud, show_minimap, draw_wield_tool, draw_crosshair);
}

const char *RenderingEngine::getVideoDriverName(irr::video::E_DRIVER_TYPE type)
{
	static const char *driver_ids[] = {
			"null",
			"software",
			"burningsvideo",
			"direct3d8",
			"direct3d9",
			"opengl",
			"ogles1",
			"ogles2",
	};

	return driver_ids[type];
}

const char *RenderingEngine::getVideoDriverFriendlyName(irr::video::E_DRIVER_TYPE type)
{
	static const char *driver_names[] = {
			"NULL Driver",
			"Software Renderer",
			"Burning's Video",
			"Direct3D 8",
			"Direct3D 9",
			"OpenGL",
			"OpenGL ES1",
			"OpenGL ES2",
	};

	return driver_names[type];
}

#ifndef __ANDROID__
#ifdef XORG_USED

static float calcDisplayDensity()
{
	const char *current_display = getenv("DISPLAY");

	if (current_display != NULL) {
		Display *x11display = XOpenDisplay(current_display);

		if (x11display != NULL) {
			/* try x direct */
			int dh = DisplayHeight(x11display, 0);
			int dw = DisplayWidth(x11display, 0);
			int dh_mm = DisplayHeightMM(x11display, 0);
			int dw_mm = DisplayWidthMM(x11display, 0);
			XCloseDisplay(x11display);

			if (dh_mm != 0 && dw_mm != 0) {
				float dpi_height = floor(dh / (dh_mm * 0.039370) + 0.5);
				float dpi_width = floor(dw / (dw_mm * 0.039370) + 0.5);
				return std::max(dpi_height, dpi_width) / 96.0;
			}
		}
	}

	/* return manually specified dpi */
	return g_settings->getFloat("screen_dpi") / 96.0;
}

float RenderingEngine::getDisplayDensity()
{
	static float cached_display_density = calcDisplayDensity();
	return cached_display_density;
}

#else  // XORG_USED
float RenderingEngine::getDisplayDensity()
{
	return g_settings->getFloat("screen_dpi") / 96.0;
}
#endif // XORG_USED

v2u32 RenderingEngine::getDisplaySize()
{
	IrrlichtDevice *nulldevice = createDevice(video::EDT_NULL);

	core::dimension2d<u32> deskres =
			nulldevice->getVideoModeList()->getDesktopResolution();
	nulldevice->drop();

	return deskres;
}

#else // __ANDROID__
float RenderingEngine::getDisplayDensity()
{
	return porting::getDisplayDensity();
}

v2u32 RenderingEngine::getDisplaySize()
{
	return porting::getDisplaySize();
}
#endif // __ANDROID__
