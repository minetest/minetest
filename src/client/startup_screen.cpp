/*
Minetest
Copyright (C) 2020 Pierre-Yves Rollo <dev@pyrollo.com>

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

#include <IGUIStaticText.h>
#include "client/camera.h"
#include "client/clouds.h"
#include "client/fontengine.h"
#include "client/guiscalingfilter.h"
#include "client/renderingengine.h"
#include "client/startup_screen.h"
#include "gui/guiEngine.h" // For MenuTextureSource
#include "gettext.h"
#include "version.h"
#include "filesys.h"

/**********************************************************************/
StartupScreen::StartupScreen():
	m_driver(RenderingEngine::get_video_driver())
{
	// Create specific texture source
	m_tsrc = new MenuTextureSource(m_driver);

	// Initialize colors
	setColor(Color::BACKGROUND, video::SColor(255, 80, 58, 37));
	setColor(Color::SKY,        video::SColor(255, 140, 186, 250));
	setColor(Color::CLOUDS,     video::SColor(255, 240, 240, 255));
	setColor(Color::MESSAGE,    video::SColor(255, 240, 240, 255));

	// Initialize texture pointers
	for (TextureDefinition &texture : m_textures)
		texture.texture = nullptr;

	// Default textures for progress bar
	setTexture(Texture::PROGRESS_FG, "textures/base/pack/progress_bar.png");
	setTexture(Texture::PROGRESS_BG, "textures/base/pack/progress_bar_bg.png");

	m_last_time = RenderingEngine::get_timer_time();

	// Get last recorded screen size. It will be updated, if needed, next step
	m_screensize = v2u32(
		g_settings->getU16("screen_w"),
		g_settings->getU16("screen_h"));

	setBackgroundType(Background::SKY);
}

/**********************************************************************/
StartupScreen::~StartupScreen()
{
	// Shutdown scenes
	shutdownClouds();

	// Clean up texture pointers
	for (TextureDefinition &texture : m_textures) {
		if (texture.texture)
			RenderingEngine::get_video_driver()->removeTexture(texture.texture);
	}

	// Delete related objects
	delete m_tsrc;
}

/**********************************************************************/
void StartupScreen::setBackgroundType(Background type)
{
	if (m_background_type == type)
		return;

	// Shutdown old background
	if (m_background_type == Background::SKY) {
		// Dont shut down clouds, avoid discontinuity if Lua turns it off and on again
		// shutdownClouds();
	}

	m_background_type = type;

	// Start new background
	if (m_background_type == Background::SKY) {
		startupClouds();
	}
}


/**********************************************************************/
bool StartupScreen::setTexture(Texture type,
		const std::string &texturepath,
		bool tile_image,
		unsigned int minsize)
{
	int itype = static_cast<int>(type);
	if (m_textures[itype].texture) {
		m_driver->removeTexture(m_textures[itype].texture);
		m_textures[itype].texture = NULL;
	}

	if (texturepath.empty() || !fs::PathExists(texturepath)) {
		return false;
	}

	m_textures[itype].texture = m_driver->getTexture(texturepath.c_str());
	m_textures[itype].tile    = tile_image;
	m_textures[itype].minsize = minsize;

	if (!m_textures[itype].texture) {
		return false;
	}

	return true;
}

/**********************************************************************/
void StartupScreen::setMessage(const wchar_t *msg, int percent)
{
	m_message = std::wstring(msg);
	m_percent = percent;
	step(false);
}

/**********************************************************************/
void StartupScreen::setMessage(const char *msg, int percent)
{
	const wchar_t *wmsg = wgettext(msg);
	setMessage(wmsg, percent);
	delete[] wmsg;
}

/**********************************************************************/
void StartupScreen::startupClouds()
{
	if (!m_cloudsmgr) {
		m_cloudsmgr = RenderingEngine::get_scene_manager()
				->createNewSceneManager();
	}

	if (!m_clouds) {
		m_clouds = new Clouds(m_cloudsmgr, -1, rand());
		m_clouds->setHeight(100.0f);
		m_clouds->update(v3f(0, 0, 0), getColor(Color::CLOUDS));
		scene::ICameraSceneNode* camera;
		camera = m_cloudsmgr->addCameraSceneNode(NULL, v3f(0, 0, 0),
				v3f(0, 60, 100));
		camera->setFarValue(10000);
		m_driver->setFog(getColor(Color::SKY), video::EFT_FOG_LINEAR,
				50.0f, 100.0f, 0, true, false);
	}
}

/**********************************************************************/
void StartupScreen::shutdownClouds()
{
	if (m_clouds)
		m_clouds->drop();
	if (m_cloudsmgr)
		m_cloudsmgr->drop();
	m_clouds = nullptr;
	m_cloudsmgr = nullptr;
}

/**********************************************************************/
void StartupScreen::drawClouds(float dtime)
{
	m_clouds->step(dtime * 3);
	m_clouds->render();
	m_cloudsmgr->drawAll();
}

/**********************************************************************/
void StartupScreen::drawBackgroundTexture()
{
	TextureDefinition txtDef = getTexture(Texture::BACKGROUND);

	video::ITexture* texture = txtDef.texture;

	if(!texture)
		return;

	v2u32 sourcesize = texture->getOriginalSize();

	if (txtDef.tile) {
		/* Draw tiled background texture */
		v2u32 tilesize(
				MYMAX(sourcesize.X, txtDef.minsize),
				MYMAX(sourcesize.Y, txtDef.minsize));
		for (unsigned int x = 0; x < m_screensize.X; x += tilesize.X )
		{
			for (unsigned int y = 0; y < m_screensize.Y; y += tilesize.Y )
			{
				draw2DImageFilterScaled(m_driver, texture,
					core::rect<s32>(x, y, x+tilesize.X, y+tilesize.Y),
					core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
					NULL, NULL, true);
			}
		}
	} else {
		/* Draw stretched background texture */
		draw2DImageFilterScaled(m_driver, texture,
			core::rect<s32>(0, 0, m_screensize.X, m_screensize.Y),
			core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
			NULL, NULL, true);
	}
}

/**********************************************************************/
void StartupScreen::drawOverlay()
{
	video::ITexture* texture = getTexture(Texture::OVERLAY).texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	/* Draw background texture */
	v2u32 sourcesize = texture->getOriginalSize();
	draw2DImageFilterScaled(m_driver, texture,
		core::rect<s32>(0, 0, m_screensize.X, m_screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/**********************************************************************/
void StartupScreen::drawHeader()
{
	video::ITexture* texture = getTexture(Texture::HEADER).texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	f32 mult = (((f32)m_screensize.X / 2.0)) /
			((f32)texture->getOriginalSize().Width);

	v2s32 splashsize(((f32)texture->getOriginalSize().Width) * mult,
			((f32)texture->getOriginalSize().Height) * mult);

	// Don't draw the header if there isn't enough room
	s32 free_space = (((s32)m_screensize.Y)-320)/2;

	if (free_space > splashsize.Y) {
		core::rect<s32> splashrect(0, 0, splashsize.X, splashsize.Y);
		splashrect += v2s32((m_screensize.X/2)-(splashsize.X/2),
				((free_space/2)-splashsize.Y/2)+10);

	draw2DImageFilterScaled(m_driver, texture, splashrect,
		core::rect<s32>(core::position2d<s32>(0,0),
		core::dimension2di(texture->getOriginalSize())),
		NULL, NULL, true);
	}
}

/**********************************************************************/
void StartupScreen::drawFooter()
{
	video::ITexture* texture = getTexture(Texture::FOOTER).texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	f32 mult = (((f32)m_screensize.X)) /
			((f32)texture->getOriginalSize().Width);

	v2s32 footersize(((f32)texture->getOriginalSize().Width) * mult,
			((f32)texture->getOriginalSize().Height) * mult);

	// Don't draw the footer if there isn't enough room
	s32 free_space = (((s32)m_screensize.Y)-320)/2;

	if (free_space > footersize.Y) {
		core::rect<s32> rect(0,0,footersize.X,footersize.Y);
		rect += v2s32(m_screensize.X/2,m_screensize.Y-footersize.Y);
		rect -= v2s32(footersize.X/2, 0);

		draw2DImageFilterScaled(m_driver, texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			NULL, NULL, true);
	}
}

/**********************************************************************/
void StartupScreen::drawProgressBar()
{
	// Default textrect: whole screen, font drawing will center text
	core::rect<s32> textrect(0, 0, m_screensize.X, m_screensize.Y);

	// draw progress bar
	if ((m_percent >= 0) && (m_percent <= 100)) {
		video::ITexture *progress_img =
				getTexture(Texture::PROGRESS_FG).texture;
		video::ITexture *progress_img_bg =
				getTexture(Texture::PROGRESS_BG).texture;

		if (progress_img && progress_img_bg) {
#ifndef __ANDROID__
			const core::dimension2d<u32> &img_size =
					progress_img_bg->getSize();
			u32 imgW = rangelim(img_size.Width, 200, 600);
			u32 imgH = rangelim(img_size.Height, 24, 72);
#else
			const core::dimension2d<u32> img_size(256, 48);
			float imgRatio = (float)img_size.Height / img_size.Width;
			u32 imgW = m_screensize.X / 2.2f;
			u32 imgH = floor(imgW * imgRatio);
#endif
			v2s32 img_pos((m_screensize.X - imgW) / 2,
					(m_screensize.Y - imgH) / 2);

			// Textrect updated so text is clipped into progress bar
			textrect = core::rect<s32>(
				img_pos.X, img_pos.Y,
				img_pos.X + imgW,
				img_pos.Y + imgH);

			draw2DImageFilterScaled(m_driver, progress_img_bg, textrect,
					core::rect<s32>(0, 0, img_size.Width, img_size.Height),
					0, 0, true);

			draw2DImageFilterScaled(m_driver, progress_img,
					core::rect<s32>(img_pos.X, img_pos.Y,
							img_pos.X + (m_percent * imgW) / 100,
							img_pos.Y + imgH),
					core::rect<s32>(0, 0,
							(m_percent * img_size.Width) / 100,
							img_size.Height),
					0, 0, true);
		}
	}

	// Draw message
	if (m_message != L"") {
		g_fontengine->getFont()->draw(m_message.c_str(), textrect,
			getColor(Color::MESSAGE), true, true);
	}
}

/**********************************************************************/
void StartupScreen::drawAll(float dtime)
{
	switch (m_background_type) {
		case Background::COLOR:
			m_driver->beginScene(true, true, getColor(Color::BACKGROUND));
			break;
		case Background::TEXTURE:
			m_driver->beginScene(true, true, getColor(Color::BACKGROUND));
			drawBackgroundTexture();
			break;
		case Background::SKY:
			m_driver->beginScene(true, true, getColor(Color::SKY));
			drawClouds(dtime);
			drawOverlay(); // ??
			break;
	}
	if (m_percent >= 0 || m_message != L"")
		drawProgressBar();

	drawHeader();
	drawFooter();

	RenderingEngine::get_gui_env()->drawAll();

	m_driver->endScene();
}

/**********************************************************************/
void StartupScreen::setWindowsCaption()
{
	const wchar_t *text = wgettext("Main Menu");
	RenderingEngine::get_raw_device()->setWindowCaption((
				utf8_to_wide(PROJECT_NAME_C) +
				L" " + utf8_to_wide(g_version_hash) +
				L" [" + text + L"]"
			).c_str());
	delete[] text;
}

/**********************************************************************/
void StartupScreen::step(bool limitFps)
{
	// Compute dtime
	float dtime;

	u32 time = RenderingEngine::get_timer_time();

	if(time > m_last_time)
		dtime = (time - m_last_time) / 1000.0;
	else
		dtime = 0;

	m_last_time = time;

	setWindowsCaption();

	// Verify if window size has changed and save it if it's the case
	// Ensure evaluating settings->getBool after verifying screensize
	// First condition is cheaper
	v2u32 screensize = m_driver->getScreenSize();

	if (m_screensize != screensize && screensize != v2u32(0,0)) {
		m_screensize = screensize;
		if (g_settings->getBool("autosave_screensize")) {
			g_settings->setU16("screen_w", m_screensize.X);
			g_settings->setU16("screen_h", m_screensize.Y);
		}
	}

	// Perform all graphic updates
	drawAll(dtime);

	// Limit FPS
	float fps_max = g_settings->getFloat("pause_fps_max");

	if (limitFps && fps_max > 0) {
		// Time of frame without fps limit
		u32 busytime_u32;

		// not using getRealTime is necessary for wine
		busytime_u32 = RenderingEngine::get_timer_time() - m_last_time;

		// FPS limiter
		u32 frametime_min = 1000./fps_max;

		if (busytime_u32 < frametime_min) {
			u32 sleeptime = frametime_min - busytime_u32;
			RenderingEngine::get_raw_device()->sleep(sleeptime);
		}
	}
}

