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
#include "clouds.h"
#include "util/numeric.h"
#include "guiscalingfilter.h"
#include "hud.h"
#include "camera.h"
#include "minimap.h"
#include "clientmap.h"
#include "renderingengine.h"
#include "inputhandler.h"
#include "gettext.h"

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__) && !defined(SERVER)
#define XORG_USED
#endif
#ifdef XORG_USED
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

RenderingEngine *RenderingEngine::s_singleton = nullptr;

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
	params.PrivateData = porting::app_global;
	params.OGLES2ShaderPath = std::string(
			porting::path_user + DIR_DELIM + "media" + DIR_DELIM + "Shaders" +
			DIR_DELIM).c_str();
#endif

	m_device = createDeviceEx(params);
	s_singleton = this;
}

RenderingEngine::~RenderingEngine()
{
	m_device->drop();
	s_singleton = nullptr;
}

v2u32 RenderingEngine::getWindowSize() const
{
	return m_device->getVideoDriver()->getScreenSize();
}

void RenderingEngine::setResizable(bool resize)
{
	m_device->setResizable(resize);
}

video::IVideoDriver *RenderingEngine::getVideoDriver()
{
	return m_device->getVideoDriver();
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

void RenderingEngine::setXorgClassHint(
		const video::SExposedVideoData &video_data, const std::string &name)
{
#ifdef XORG_USED
	if (video_data.OpenGLLinux.X11Display == NULL)
		return;

	XClassHint *classhint = XAllocClassHint();
	classhint->res_name = (char *)name.c_str();
	classhint->res_class = (char *)name.c_str();

	XSetClassHint((Display *)video_data.OpenGLLinux.X11Display,
			video_data.OpenGLLinux.X11Window, classhint);
	XFree(classhint);
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
	const video::SExposedVideoData exposedData =
			m_device->getVideoDriver()->getExposedVideoData();
	HWND hWnd; // Window handle

	switch (m_device->getVideoDriver()->getDriverType()) {
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

	video::IVideoDriver *v_driver = m_device->getVideoDriver();

	video::IImageLoader *image_loader = NULL;
	u32 cnt = v_driver->getImageLoaderCount();
	for (u32 i = 0; i < cnt; i++) {
		if (v_driver->getImageLoader(i)->isALoadableFileExtension(
				    icon_file.c_str())) {
			image_loader = v_driver->getImageLoader(i);
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

	const video::SExposedVideoData &video_data = v_driver->getExposedVideoData();

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

void RenderingEngine::_draw_scene(Camera *camera, Client *client, LocalPlayer *player,
		Hud *hud, Minimap *mapper, gui::IGUIEnvironment *guienv,
		const v2u32 &screensize, const video::SColor &skycolor, bool show_hud,
		bool show_minimap)
{
	bool draw_wield_tool =
			(show_hud && (player->hud_flags & HUD_FLAG_WIELDITEM_VISIBLE) &&
					camera->getCameraMode() < CAMERA_MODE_THIRD);

	bool draw_crosshair = ((player->hud_flags & HUD_FLAG_CROSSHAIR_VISIBLE) &&
			       (camera->getCameraMode() != CAMERA_MODE_THIRD_FRONT));

#ifdef HAVE_TOUCHSCREENGUI
	try {
		draw_crosshair = !g_settings->getBool("touchtarget");
	} catch (SettingNotFoundException) {
	}
#endif

	const std::string &draw_mode = g_settings->get("3d_mode");

	if (draw_mode == "anaglyph") {
		draw_anaglyph_3d_mode(
				camera, show_hud, hud, draw_wield_tool, client, guienv);
		draw_crosshair = false;
	} else if (draw_mode == "interlaced") {
		draw_interlaced_3d_mode(camera, show_hud, hud, screensize,
				draw_wield_tool, client, guienv, skycolor);
		draw_crosshair = false;
	} else if (draw_mode == "sidebyside") {
		draw_sidebyside_3d_mode(camera, show_hud, hud, screensize,
				draw_wield_tool, client, guienv, skycolor);
		show_hud = false;
	} else if (draw_mode == "topbottom") {
		draw_top_bottom_3d_mode(camera, show_hud, hud, screensize,
				draw_wield_tool, client, guienv, skycolor);
		show_hud = false;
	} else if (draw_mode == "pageflip") {
		draw_pageflip_3d_mode(camera, show_hud, hud, screensize, draw_wield_tool,
				client, guienv, skycolor);
		draw_crosshair = false;
		show_hud = false;
	} else {
		draw_plain(camera, show_hud, hud, screensize, draw_wield_tool, client,
				guienv, skycolor);
	}

	/*
		Post effects
	*/
	client->getEnv().getClientMap().renderPostFx(camera->getCameraMode());

	// TODO how to make those 3d too
	if (show_hud) {
		if (draw_crosshair)
			hud->drawCrosshair();

		hud->drawHotbar(client->getPlayerItem());
		hud->drawLuaElements(camera->getOffset());
		camera->drawNametags();

		if (mapper && show_minimap)
			mapper->drawMinimap();
	}

	guienv->drawAll();
}

void RenderingEngine::draw_anaglyph_3d_mode(Camera *camera, bool show_hud, Hud *hud,
		bool draw_wield_tool, Client *client, gui::IGUIEnvironment *guienv)
{

	/* preserve old setup*/
	irr::core::vector3df oldPosition = camera->getCameraNode()->getPosition();
	irr::core::vector3df oldTarget = camera->getCameraNode()->getTarget();

	irr::core::matrix4 startMatrix =
			camera->getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint =
			(camera->getCameraNode()->getTarget() -
					camera->getCameraNode()->getAbsolutePosition())
					.setLength(1) +
			camera->getCameraNode()->getAbsolutePosition();

	// Left eye...
	irr::core::vector3df leftEye;
	irr::core::matrix4 leftMove;
	leftMove.setTranslation(irr::core::vector3df(
			-g_settings->getFloat("3d_paralax_strength"), 0.0f, 0.0f));
	leftEye = (startMatrix * leftMove).getTranslation();

	// clear the depth buffer, and color
	getVideoDriver()->beginScene(true, true, irr::video::SColor(0, 200, 200, 255));
	getVideoDriver()->getOverrideMaterial().Material.ColorMask = irr::video::ECP_RED;
	getVideoDriver()->getOverrideMaterial().EnableFlags = irr::video::EMF_COLOR_MASK;
	getVideoDriver()->getOverrideMaterial().EnablePasses =
			irr::scene::ESNRP_SKY_BOX + irr::scene::ESNRP_SOLID +
			irr::scene::ESNRP_TRANSPARENT +
			irr::scene::ESNRP_TRANSPARENT_EFFECT + irr::scene::ESNRP_SHADOW;
	camera->getCameraNode()->setPosition(leftEye);
	camera->getCameraNode()->setTarget(focusPoint);
	get_scene_manager()->drawAll();
	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool)
			camera->drawWieldedTool(&leftMove);
	}

	guienv->drawAll();

	// Right eye...
	irr::core::vector3df rightEye;
	irr::core::matrix4 rightMove;
	rightMove.setTranslation(irr::core::vector3df(
			g_settings->getFloat("3d_paralax_strength"), 0.0f, 0.0f));
	rightEye = (startMatrix * rightMove).getTranslation();

	// clear the depth buffer
	getVideoDriver()->clearZBuffer();
	getVideoDriver()->getOverrideMaterial().Material.ColorMask =
			irr::video::ECP_GREEN + irr::video::ECP_BLUE;
	getVideoDriver()->getOverrideMaterial().EnableFlags = irr::video::EMF_COLOR_MASK;
	getVideoDriver()->getOverrideMaterial().EnablePasses =
			irr::scene::ESNRP_SKY_BOX + irr::scene::ESNRP_SOLID +
			irr::scene::ESNRP_TRANSPARENT +
			irr::scene::ESNRP_TRANSPARENT_EFFECT + irr::scene::ESNRP_SHADOW;
	camera->getCameraNode()->setPosition(rightEye);
	camera->getCameraNode()->setTarget(focusPoint);
	get_scene_manager()->drawAll();
	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool)
			camera->drawWieldedTool(&rightMove);
	}

	guienv->drawAll();

	getVideoDriver()->getOverrideMaterial().Material.ColorMask = irr::video::ECP_ALL;
	getVideoDriver()->getOverrideMaterial().EnableFlags = 0;
	getVideoDriver()->getOverrideMaterial().EnablePasses = 0;
	camera->getCameraNode()->setPosition(oldPosition);
	camera->getCameraNode()->setTarget(oldTarget);
}

void RenderingEngine::init_texture(
		const v2u32 &screensize, video::ITexture **texture, const char *name)
{
	if (*texture) {
		getVideoDriver()->removeTexture(*texture);
	}
	*texture = getVideoDriver()->addRenderTargetTexture(
			core::dimension2d<u32>(screensize.X, screensize.Y), name,
			irr::video::ECF_A8R8G8B8);
}

video::ITexture *RenderingEngine::draw_image(const v2u32 &screensize, parallax_sign psign,
		const irr::core::matrix4 &startMatrix,
		const irr::core::vector3df &focusPoint, bool show_hud, Camera *camera,
		Hud *hud, bool draw_wield_tool, Client *client,
		gui::IGUIEnvironment *guienv, const video::SColor &skycolor)
{
	static video::ITexture *images[2] = {NULL, NULL};
	static v2u32 last_screensize = v2u32(0, 0);

	video::ITexture *image = NULL;

	if (screensize != last_screensize) {
		init_texture(screensize, &images[1], "mt_drawimage_img1");
		init_texture(screensize, &images[0], "mt_drawimage_img2");
		last_screensize = screensize;
	}

	if (psign == RIGHT)
		image = images[1];
	else
		image = images[0];

	getVideoDriver()->setRenderTarget(image, true, true,
			irr::video::SColor(255, skycolor.getRed(), skycolor.getGreen(),
					skycolor.getBlue()));

	irr::core::vector3df eye_pos;
	irr::core::matrix4 movement;
	movement.setTranslation(irr::core::vector3df(
			(int)psign * g_settings->getFloat("3d_paralax_strength"), 0.0f,
			0.0f));
	eye_pos = (startMatrix * movement).getTranslation();

	// clear the depth buffer
	getVideoDriver()->clearZBuffer();
	camera->getCameraNode()->setPosition(eye_pos);
	camera->getCameraNode()->setTarget(focusPoint);
	get_scene_manager()->drawAll();

	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool)
			camera->drawWieldedTool(&movement);
	}

	guienv->drawAll();

	/* switch back to real renderer */
	getVideoDriver()->setRenderTarget(0, true, true,
			irr::video::SColor(0, skycolor.getRed(), skycolor.getGreen(),
					skycolor.getBlue()));

	return image;
}

video::ITexture *RenderingEngine::draw_hud(const v2u32 &screensize, bool show_hud,
		Hud *hud, Client *client, bool draw_crosshair,
		const video::SColor &skycolor, gui::IGUIEnvironment *guienv,
		Camera *camera)
{
	static video::ITexture *image = nullptr;
	init_texture(screensize, &image, "mt_drawimage_hud");
	getVideoDriver()->setRenderTarget(
			image, true, true, irr::video::SColor(255, 0, 0, 0));

	if (show_hud) {
		if (draw_crosshair)
			hud->drawCrosshair();
		hud->drawHotbar(client->getPlayerItem());
		hud->drawLuaElements(camera->getOffset());
		camera->drawNametags();
		guienv->drawAll();
	}

	getVideoDriver()->setRenderTarget(0, true, true,
			irr::video::SColor(0, skycolor.getRed(), skycolor.getGreen(),
					skycolor.getBlue()));

	return image;
}

void RenderingEngine::draw_interlaced_3d_mode(Camera *camera, bool show_hud, Hud *hud,
		const v2u32 &screensize, bool draw_wield_tool, Client *client,
		gui::IGUIEnvironment *guienv, const video::SColor &skycolor)
{
	/* save current info */
	irr::core::vector3df oldPosition = camera->getCameraNode()->getPosition();
	irr::core::vector3df oldTarget = camera->getCameraNode()->getTarget();
	irr::core::matrix4 startMatrix =
			camera->getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint =
			(camera->getCameraNode()->getTarget() -
					camera->getCameraNode()->getAbsolutePosition())
					.setLength(1) +
			camera->getCameraNode()->getAbsolutePosition();

	/* create left view */
	video::ITexture *left_image = draw_image(screensize, LEFT, startMatrix,
			focusPoint, show_hud, camera, hud, draw_wield_tool, client,
			guienv, skycolor);

	// Right eye...
	irr::core::vector3df rightEye;
	irr::core::matrix4 rightMove;
	rightMove.setTranslation(irr::core::vector3df(
			g_settings->getFloat("3d_paralax_strength"), 0.0f, 0.0f));
	rightEye = (startMatrix * rightMove).getTranslation();

	// clear the depth buffer
	getVideoDriver()->clearZBuffer();
	camera->getCameraNode()->setPosition(rightEye);
	camera->getCameraNode()->setTarget(focusPoint);
	get_scene_manager()->drawAll();

	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool)
			camera->drawWieldedTool(&rightMove);
	}
	guienv->drawAll();

	for (unsigned int i = 0; i < screensize.Y; i += 2) {
#if (IRRLICHT_VERSION_MAJOR >= 1) && (IRRLICHT_VERSION_MINOR >= 8)
		getVideoDriver()->draw2DImage(left_image,
				irr::core::position2d<s32>(0, i),
#else
		getVideoDriver()->draw2DImage(left_image,
				irr::core::position2d<s32>(0, screensize.Y - i),
#endif
				irr::core::rect<s32>(0, i, screensize.X, i + 1), 0,
				irr::video::SColor(255, 255, 255, 255), false);
	}

	/* cleanup */
	camera->getCameraNode()->setPosition(oldPosition);
	camera->getCameraNode()->setTarget(oldTarget);
}

void RenderingEngine::draw_sidebyside_3d_mode(Camera *camera, bool show_hud, Hud *hud,
		const v2u32 &screensize, bool draw_wield_tool, Client *client,
		gui::IGUIEnvironment *guienv, const video::SColor &skycolor)
{
	/* save current info */
	irr::core::vector3df oldPosition = camera->getCameraNode()->getPosition();
	irr::core::vector3df oldTarget = camera->getCameraNode()->getTarget();
	irr::core::matrix4 startMatrix =
			camera->getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint =
			(camera->getCameraNode()->getTarget() -
					camera->getCameraNode()->getAbsolutePosition())
					.setLength(1) +
			camera->getCameraNode()->getAbsolutePosition();

	/* create left view */
	video::ITexture *left_image = draw_image(screensize, LEFT, startMatrix,
			focusPoint, show_hud, camera, hud, draw_wield_tool, client,
			guienv, skycolor);

	/* create right view */
	video::ITexture *right_image = draw_image(screensize, RIGHT, startMatrix,
			focusPoint, show_hud, camera, hud, draw_wield_tool, client,
			guienv, skycolor);

	/* create hud overlay */
	video::ITexture *hudtexture = draw_hud(screensize, show_hud, hud, client, false,
			skycolor, guienv, camera);
	getVideoDriver()->makeColorKeyTexture(
			hudtexture, irr::video::SColor(255, 0, 0, 0));
	// makeColorKeyTexture mirrors texture so we do it twice to get it right again
	getVideoDriver()->makeColorKeyTexture(
			hudtexture, irr::video::SColor(255, 0, 0, 0));

	draw2DImageFilterScaled(getVideoDriver(), left_image,
			irr::core::rect<s32>(0, 0, screensize.X / 2, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			false);

	draw2DImageFilterScaled(getVideoDriver(), hudtexture,
			irr::core::rect<s32>(0, 0, screensize.X / 2, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);

	draw2DImageFilterScaled(getVideoDriver(), right_image,
			irr::core::rect<s32>(
					screensize.X / 2, 0, screensize.X, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			false);

	draw2DImageFilterScaled(getVideoDriver(), hudtexture,
			irr::core::rect<s32>(
					screensize.X / 2, 0, screensize.X, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);

	left_image = nullptr;
	right_image = nullptr;

	/* cleanup */
	camera->getCameraNode()->setPosition(oldPosition);
	camera->getCameraNode()->setTarget(oldTarget);
}

void RenderingEngine::draw_top_bottom_3d_mode(Camera *camera, bool show_hud, Hud *hud,
		const v2u32 &screensize, bool draw_wield_tool, Client *client,
		gui::IGUIEnvironment *guienv, const video::SColor &skycolor)
{
	/* save current info */
	irr::core::vector3df oldPosition = camera->getCameraNode()->getPosition();
	irr::core::vector3df oldTarget = camera->getCameraNode()->getTarget();
	irr::core::matrix4 startMatrix =
			camera->getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint =
			(camera->getCameraNode()->getTarget() -
					camera->getCameraNode()->getAbsolutePosition())
					.setLength(1) +
			camera->getCameraNode()->getAbsolutePosition();

	/* create left view */
	video::ITexture *left_image = draw_image(screensize, LEFT, startMatrix,
			focusPoint, show_hud, camera, hud, draw_wield_tool, client,
			guienv, skycolor);

	/* create right view */
	video::ITexture *right_image = draw_image(screensize, RIGHT, startMatrix,
			focusPoint, show_hud, camera, hud, draw_wield_tool, client,
			guienv, skycolor);

	/* create hud overlay */
	video::ITexture *hudtexture = draw_hud(screensize, show_hud, hud, client, false,
			skycolor, guienv, camera);
	getVideoDriver()->makeColorKeyTexture(
			hudtexture, irr::video::SColor(255, 0, 0, 0));
	// makeColorKeyTexture mirrors texture so we do it twice to get it right again
	getVideoDriver()->makeColorKeyTexture(
			hudtexture, irr::video::SColor(255, 0, 0, 0));

	draw2DImageFilterScaled(getVideoDriver(), left_image,
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y / 2),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			false);

	draw2DImageFilterScaled(getVideoDriver(), hudtexture,
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y / 2),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);

	draw2DImageFilterScaled(getVideoDriver(), right_image,
			irr::core::rect<s32>(
					0, screensize.Y / 2, screensize.X, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			false);

	draw2DImageFilterScaled(getVideoDriver(), hudtexture,
			irr::core::rect<s32>(
					0, screensize.Y / 2, screensize.X, screensize.Y),
			irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);

	left_image = NULL;
	right_image = NULL;

	/* cleanup */
	camera->getCameraNode()->setPosition(oldPosition);
	camera->getCameraNode()->setTarget(oldTarget);
}

void RenderingEngine::draw_pageflip_3d_mode(Camera *camera, bool show_hud, Hud *hud,
		const v2u32 &screensize, bool draw_wield_tool, Client *client,
		gui::IGUIEnvironment *guienv, const video::SColor &skycolor)
{
#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR > 8
	errorstream << "Pageflip 3D mode is not supported"
		    << " with your Irrlicht version!" << std::endl;
#else
	/* preserve old setup*/
	irr::core::vector3df oldPosition = camera->getCameraNode()->getPosition();
	irr::core::vector3df oldTarget = camera->getCameraNode()->getTarget();

	irr::core::matrix4 startMatrix =
			camera->getCameraNode()->getAbsoluteTransformation();
	irr::core::vector3df focusPoint =
			(camera->getCameraNode()->getTarget() -
					camera->getCameraNode()->getAbsolutePosition())
					.setLength(1) +
			camera->getCameraNode()->getAbsolutePosition();

	// Left eye...
	getVideoDriver()->setRenderTarget(irr::video::ERT_STEREO_LEFT_BUFFER);

	irr::core::vector3df leftEye;
	irr::core::matrix4 leftMove;
	leftMove.setTranslation(irr::core::vector3df(
			-g_settings->getFloat("3d_paralax_strength"), 0.0f, 0.0f));
	leftEye = (startMatrix * leftMove).getTranslation();

	// clear the depth buffer, and color
	getVideoDriver()->beginScene(true, true, irr::video::SColor(200, 200, 200, 255));
	camera->getCameraNode()->setPosition(leftEye);
	camera->getCameraNode()->setTarget(focusPoint);
	get_scene_manager()->drawAll();
	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool)
			camera->drawWieldedTool(&leftMove);
		hud->drawHotbar(client->getPlayerItem());
		hud->drawLuaElements(camera->getOffset());
		camera->drawNametags();
	}

	guienv->drawAll();

	// Right eye...
	getVideoDriver()->setRenderTarget(irr::video::ERT_STEREO_RIGHT_BUFFER);

	irr::core::vector3df rightEye;
	irr::core::matrix4 rightMove;
	rightMove.setTranslation(irr::core::vector3df(
			g_settings->getFloat("3d_paralax_strength"), 0.0f, 0.0f));
	rightEye = (startMatrix * rightMove).getTranslation();

	// clear the depth buffer, and color
	getVideoDriver()->beginScene(true, true, irr::video::SColor(200, 200, 200, 255));
	camera->getCameraNode()->setPosition(rightEye);
	camera->getCameraNode()->setTarget(focusPoint);
	get_scene_manager()->drawAll();
	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool)
			camera->drawWieldedTool(&rightMove);
		hud->drawHotbar(client->getPlayerItem());
		hud->drawLuaElements(camera->getOffset());
		camera->drawNametags();
	}

	guienv->drawAll();

	camera->getCameraNode()->setPosition(oldPosition);
	camera->getCameraNode()->setTarget(oldTarget);
#endif
}

// returns (size / coef), rounded upwards
inline int scaledown(int coef, int size)
{
	return (size + coef - 1) / coef;
}

void RenderingEngine::draw_plain(Camera *camera, bool show_hud, Hud *hud,
		const v2u32 &screensize, bool draw_wield_tool, Client *client,
		gui::IGUIEnvironment *guienv, const video::SColor &skycolor)
{
	// Undersampling-specific stuff
	static video::ITexture *image = NULL;
	static v2u32 last_pixelated_size = v2u32(0, 0);
	static thread_local int undersampling = g_settings->getU16("undersampling");
	v2u32 pixelated_size;
	v2u32 dest_size;
	if (undersampling > 0) {
		pixelated_size = v2u32(scaledown(undersampling, screensize.X),
				scaledown(undersampling, screensize.Y));
		dest_size = v2u32(undersampling * pixelated_size.X,
				undersampling * pixelated_size.Y);
		if (pixelated_size != last_pixelated_size) {
			init_texture(pixelated_size, &image, "mt_drawimage_img1");
			last_pixelated_size = pixelated_size;
		}
		getVideoDriver()->setRenderTarget(image, true, true, skycolor);
	}

	// Render
	get_scene_manager()->drawAll();
	getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (show_hud) {
		hud->drawSelectionMesh();
		if (draw_wield_tool) {
			camera->drawWieldedTool();
		}
	}

	// Upscale lowres render
	if (undersampling > 0) {
		getVideoDriver()->setRenderTarget(0, true, true);
		getVideoDriver()->draw2DImage(image,
				irr::core::rect<s32>(0, 0, dest_size.X, dest_size.Y),
				irr::core::rect<s32>(0, 0, pixelated_size.X,
						pixelated_size.Y));
	}
}

const char *RenderingEngine::getVideoDriverName(irr::video::E_DRIVER_TYPE type)
{
	static const char *driver_ids[] = {
			"null", "software", "burningsvideo", "direct3d8", "direct3d9",
			"opengl", "ogles1", "ogles2",
	};

	return driver_ids[type];
}

const char *RenderingEngine::getVideoDriverFriendlyName(irr::video::E_DRIVER_TYPE type)
{
	static const char *driver_names[] = {
			"NULL Driver", "Software Renderer", "Burning's Video",
			"Direct3D 8", "Direct3D 9", "OpenGL", "OpenGL ES1", "OpenGL ES2",
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
			float dpi_height = floor(
					DisplayHeight(x11display, 0) /
							(DisplayHeightMM(x11display, 0) *
									0.039370) +
					0.5);
			float dpi_width = floor(
					DisplayWidth(x11display, 0) /
							(DisplayWidthMM(x11display, 0) *
									0.039370) +
					0.5);

			XCloseDisplay(x11display);

			return std::max(dpi_height, dpi_width) / 96.0;
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
#endif // __ANDROID__
