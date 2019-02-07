/*
Minetest
Copyright (C) 2013 sapier

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

#include "guiEngine.h"

#include <IGUIStaticText.h>
#include <ICameraSceneNode.h>
#include "client/renderingengine.h"
#include "scripting_mainmenu.h"
#include "util/numeric.h"
#include "config.h"
#include "version.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "guiMainMenu.h"
#include "sound.h"
#include "client/sound_openal.h"
#include "client/clouds.h"
#include "httpfetch.h"
#include "log.h"
#include "client/fontengine.h"
#include "client/guiscalingfilter.h"
#include "irrlicht_changes/static_text.h"

#ifdef __ANDROID__
#include "client/tile.h"
#include <GLES/gl.h>
#endif


/******************************************************************************/
void TextDestGuiEngine::gotText(const StringMap &fields)
{
	m_engine->getScriptIface()->handleMainMenuButtons(fields);
}

/******************************************************************************/
void TextDestGuiEngine::gotText(const std::wstring &text)
{
	m_engine->getScriptIface()->handleMainMenuEvent(wide_to_utf8(text));
}

/******************************************************************************/
MenuTextureSource::~MenuTextureSource()
{
	for (const std::string &texture_to_delete : m_to_delete) {
		const char *tname = texture_to_delete.c_str();
		video::ITexture *texture = m_driver->getTexture(tname);
		m_driver->removeTexture(texture);
	}
}

/******************************************************************************/
video::ITexture *MenuTextureSource::getTexture(const std::string &name, u32 *id)
{
	if (id)
		*id = 0;

	if (name.empty())
		return NULL;

	m_to_delete.insert(name);

#ifdef __ANDROID__
	video::ITexture *retval = m_driver->findTexture(name.c_str());
	if (retval)
		return retval;

	video::IImage *image = m_driver->createImageFromFile(name.c_str());
	if (!image)
		return NULL;

	image = Align2Npot2(image, m_driver);
	retval = m_driver->addTexture(name.c_str(), image);
	image->drop();
	return retval;
#else
	return m_driver->getTexture(name.c_str());
#endif
}

/******************************************************************************/
/** MenuMusicFetcher                                                          */
/******************************************************************************/
void MenuMusicFetcher::fetchSounds(const std::string &name,
			std::set<std::string> &dst_paths,
			std::set<std::string> &dst_datas)
{
	if(m_fetched.count(name))
		return;
	m_fetched.insert(name);
	std::string base;
	base = porting::path_share + DIR_DELIM + "sounds";
	dst_paths.insert(base + DIR_DELIM + name + ".ogg");
	int i;
	for(i=0; i<10; i++)
		dst_paths.insert(base + DIR_DELIM + name + "."+itos(i)+".ogg");
	base = porting::path_user + DIR_DELIM + "sounds";
	dst_paths.insert(base + DIR_DELIM + name + ".ogg");
	for(i=0; i<10; i++)
		dst_paths.insert(base + DIR_DELIM + name + "."+itos(i)+".ogg");
}

/******************************************************************************/
/** GUIEngine                                                                 */
/******************************************************************************/
GUIEngine::GUIEngine(JoystickController *joystick,
		gui::IGUIElement *parent,
		IMenuManager *menumgr,
		MainMenuData *data,
		bool &kill) :
	m_parent(parent),
	m_menumanager(menumgr),
	m_smgr(RenderingEngine::get_scene_manager()),
	m_data(data),
	m_kill(kill)
{
	//initialize texture pointers
	for (image_definition &texture : m_textures) {
		texture.texture = NULL;
	}
	// is deleted by guiformspec!
	m_buttonhandler = new TextDestGuiEngine(this);

	//create texture source
	m_texture_source = new MenuTextureSource(RenderingEngine::get_video_driver());

	//create soundmanager
	MenuMusicFetcher soundfetcher;
#if USE_SOUND
	if (g_settings->getBool("enable_sound"))
		m_sound_manager = createOpenALSoundManager(g_sound_manager_singleton.get(), &soundfetcher);
#endif
	if(!m_sound_manager)
		m_sound_manager = &dummySoundManager;

	//create topleft header
	m_toplefttext = L"";

	core::rect<s32> rect(0, 0, g_fontengine->getTextWidth(m_toplefttext.c_str()),
		g_fontengine->getTextHeight());
	rect += v2s32(4, 0);

	m_irr_toplefttext = gui::StaticText::add(RenderingEngine::get_gui_env(),
			m_toplefttext, rect, false, true, 0, -1);

	//create formspecsource
	m_formspecgui = new FormspecFormSource("");

	/* Create menu */
	m_menu = new GUIFormSpecMenu(joystick,
			m_parent,
			-1,
			m_menumanager,
			NULL /* &client */,
			m_texture_source,
			m_formspecgui,
			m_buttonhandler,
			"",
			false);

	m_menu->allowClose(false);
	m_menu->lockSize(true,v2u32(800,600));

	// Initialize scripting

	infostream << "GUIEngine: Initializing Lua" << std::endl;

	m_script = new MainMenuScripting(this);

	try {
		m_script->setMainMenuData(&m_data->script_data);
		m_data->script_data.errormessage = "";

		if (!loadMainMenuScript()) {
			errorstream << "No future without main menu!" << std::endl;
			abort();
		}

		run();
	} catch (LuaError &e) {
		errorstream << "Main menu error: " << e.what() << std::endl;
		m_data->script_data.errormessage = e.what();
	}

	m_menu->quitMenu();
	m_menu->drop();
	m_menu = NULL;
}

/******************************************************************************/
bool GUIEngine::loadMainMenuScript()
{
	// Set main menu path (for core.get_mainmenu_path())
	m_scriptdir = g_settings->get("main_menu_path");
	if (m_scriptdir.empty()) {
		m_scriptdir = porting::path_share + DIR_DELIM + "builtin" + DIR_DELIM + "mainmenu";
	}

	// Load builtin (which will load the main menu script)
	std::string script = porting::path_share + DIR_DELIM "builtin" + DIR_DELIM "init.lua";
	try {
		m_script->loadScript(script);
		// Menu script loaded
		return true;
	} catch (const ModError &e) {
		errorstream << "GUIEngine: execution of menu script failed: "
			<< e.what() << std::endl;
	}

	return false;
}

/******************************************************************************/
void GUIEngine::run()
{
	// Always create clouds because they may or may not be
	// needed based on the game selected
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	cloudInit();

	unsigned int text_height = g_fontengine->getTextHeight();

	irr::core::dimension2d<u32> previous_screen_size(g_settings->getU16("screen_w"),
		g_settings->getU16("screen_h"));

	static const video::SColor sky_color(255, 140, 186, 250);

	// Reset fog color
	{
		video::SColor fog_color;
		video::E_FOG_TYPE fog_type = video::EFT_FOG_LINEAR;
		f32 fog_start = 0;
		f32 fog_end = 0;
		f32 fog_density = 0;
		bool fog_pixelfog = false;
		bool fog_rangefog = false;
		driver->getFog(fog_color, fog_type, fog_start, fog_end, fog_density,
				fog_pixelfog, fog_rangefog);

		driver->setFog(sky_color, fog_type, fog_start, fog_end, fog_density,
				fog_pixelfog, fog_rangefog);
	}

	while (RenderingEngine::run() && (!m_startgame) && (!m_kill)) {

		const irr::core::dimension2d<u32> &current_screen_size =
			RenderingEngine::get_video_driver()->getScreenSize();
		// Verify if window size has changed and save it if it's the case
		// Ensure evaluating settings->getBool after verifying screensize
		// First condition is cheaper
		if (previous_screen_size != current_screen_size &&
				current_screen_size != irr::core::dimension2d<u32>(0,0) &&
				g_settings->getBool("autosave_screensize")) {
			g_settings->setU16("screen_w", current_screen_size.Width);
			g_settings->setU16("screen_h", current_screen_size.Height);
			previous_screen_size = current_screen_size;
		}

		//check if we need to update the "upper left corner"-text
		if (text_height != g_fontengine->getTextHeight()) {
			updateTopLeftTextSize();
			text_height = g_fontengine->getTextHeight();
		}

		driver->beginScene(true, true, sky_color);

		if (m_clouds_enabled)
		{
			cloudPreProcess();
			drawOverlay(driver);
		}
		else
			drawBackground(driver);

		drawHeader(driver);
		drawFooter(driver);

		RenderingEngine::get_gui_env()->drawAll();

		driver->endScene();

		if (m_clouds_enabled)
			cloudPostProcess();
		else
			sleep_ms(25);

		m_script->step();

#ifdef __ANDROID__
		m_menu->getAndroidUIInput();
#endif
	}
}

/******************************************************************************/
GUIEngine::~GUIEngine()
{
	if (m_sound_manager != &dummySoundManager){
		delete m_sound_manager;
		m_sound_manager = NULL;
	}

	infostream<<"GUIEngine: Deinitializing scripting"<<std::endl;
	delete m_script;

	m_irr_toplefttext->setText(L"");

	//clean up texture pointers
	for (image_definition &texture : m_textures) {
		if (texture.texture)
			RenderingEngine::get_video_driver()->removeTexture(texture.texture);
	}

	delete m_texture_source;

	if (m_cloud.clouds)
		m_cloud.clouds->drop();
}

/******************************************************************************/
void GUIEngine::cloudInit()
{
	m_cloud.clouds = new Clouds(m_smgr, -1, rand());
	m_cloud.clouds->setHeight(100.0f);
	m_cloud.clouds->update(v3f(0, 0, 0), video::SColor(255,240,240,255));

	m_cloud.camera = m_smgr->addCameraSceneNode(0,
				v3f(0,0,0), v3f(0, 60, 100));
	m_cloud.camera->setFarValue(10000);

	m_cloud.lasttime = RenderingEngine::get_timer_time();
}

/******************************************************************************/
void GUIEngine::cloudPreProcess()
{
	u32 time = RenderingEngine::get_timer_time();

	if(time > m_cloud.lasttime)
		m_cloud.dtime = (time - m_cloud.lasttime) / 1000.0;
	else
		m_cloud.dtime = 0;

	m_cloud.lasttime = time;

	m_cloud.clouds->step(m_cloud.dtime*3);
	m_cloud.clouds->render();
	m_smgr->drawAll();
}

/******************************************************************************/
void GUIEngine::cloudPostProcess()
{
	float fps_max = g_settings->getFloat("pause_fps_max");
	// Time of frame without fps limit
	u32 busytime_u32;

	// not using getRealTime is necessary for wine
	u32 time = RenderingEngine::get_timer_time();
	if(time > m_cloud.lasttime)
		busytime_u32 = time - m_cloud.lasttime;
	else
		busytime_u32 = 0;

	// FPS limiter
	u32 frametime_min = 1000./fps_max;

	if (busytime_u32 < frametime_min) {
		u32 sleeptime = frametime_min - busytime_u32;
		RenderingEngine::get_raw_device()->sleep(sleeptime);
	}
}

/******************************************************************************/
void GUIEngine::drawBackground(video::IVideoDriver *driver)
{
	v2u32 screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_BACKGROUND].texture;

	/* If no texture, draw background of solid color */
	if(!texture){
		video::SColor color(255,80,58,37);
		core::rect<s32> rect(0, 0, screensize.X, screensize.Y);
		driver->draw2DRectangle(color, rect, NULL);
		return;
	}

	v2u32 sourcesize = texture->getOriginalSize();

	if (m_textures[TEX_LAYER_BACKGROUND].tile)
	{
		v2u32 tilesize(
				MYMAX(sourcesize.X,m_textures[TEX_LAYER_BACKGROUND].minsize),
				MYMAX(sourcesize.Y,m_textures[TEX_LAYER_BACKGROUND].minsize));
		for (unsigned int x = 0; x < screensize.X; x += tilesize.X )
		{
			for (unsigned int y = 0; y < screensize.Y; y += tilesize.Y )
			{
				draw2DImageFilterScaled(driver, texture,
					core::rect<s32>(x, y, x+tilesize.X, y+tilesize.Y),
					core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
					NULL, NULL, true);
			}
		}
		return;
	}

	/* Draw background texture */
	draw2DImageFilterScaled(driver, texture,
		core::rect<s32>(0, 0, screensize.X, screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/******************************************************************************/
void GUIEngine::drawOverlay(video::IVideoDriver *driver)
{
	v2u32 screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_OVERLAY].texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	/* Draw background texture */
	v2u32 sourcesize = texture->getOriginalSize();
	draw2DImageFilterScaled(driver, texture,
		core::rect<s32>(0, 0, screensize.X, screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/******************************************************************************/
void GUIEngine::drawHeader(video::IVideoDriver *driver)
{
	core::dimension2d<u32> screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_HEADER].texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	f32 mult = (((f32)screensize.Width / 2.0)) /
			((f32)texture->getOriginalSize().Width);

	v2s32 splashsize(((f32)texture->getOriginalSize().Width) * mult,
			((f32)texture->getOriginalSize().Height) * mult);

	// Don't draw the header if there isn't enough room
	s32 free_space = (((s32)screensize.Height)-320)/2;

	if (free_space > splashsize.Y) {
		core::rect<s32> splashrect(0, 0, splashsize.X, splashsize.Y);
		splashrect += v2s32((screensize.Width/2)-(splashsize.X/2),
				((free_space/2)-splashsize.Y/2)+10);

	video::SColor bgcolor(255,50,50,50);

	draw2DImageFilterScaled(driver, texture, splashrect,
		core::rect<s32>(core::position2d<s32>(0,0),
		core::dimension2di(texture->getOriginalSize())),
		NULL, NULL, true);
	}
}

/******************************************************************************/
void GUIEngine::drawFooter(video::IVideoDriver *driver)
{
	core::dimension2d<u32> screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_FOOTER].texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	f32 mult = (((f32)screensize.Width)) /
			((f32)texture->getOriginalSize().Width);

	v2s32 footersize(((f32)texture->getOriginalSize().Width) * mult,
			((f32)texture->getOriginalSize().Height) * mult);

	// Don't draw the footer if there isn't enough room
	s32 free_space = (((s32)screensize.Height)-320)/2;

	if (free_space > footersize.Y) {
		core::rect<s32> rect(0,0,footersize.X,footersize.Y);
		rect += v2s32(screensize.Width/2,screensize.Height-footersize.Y);
		rect -= v2s32(footersize.X/2, 0);

		draw2DImageFilterScaled(driver, texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			NULL, NULL, true);
	}
}

/******************************************************************************/
bool GUIEngine::setTexture(texture_layer layer, std::string texturepath,
		bool tile_image, unsigned int minsize)
{
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	if (m_textures[layer].texture) {
		driver->removeTexture(m_textures[layer].texture);
		m_textures[layer].texture = NULL;
	}

	if (texturepath.empty() || !fs::PathExists(texturepath)) {
		return false;
	}

	m_textures[layer].texture = driver->getTexture(texturepath.c_str());
	m_textures[layer].tile    = tile_image;
	m_textures[layer].minsize = minsize;

	if (!m_textures[layer].texture) {
		return false;
	}

	return true;
}

/******************************************************************************/
bool GUIEngine::downloadFile(const std::string &url, const std::string &target)
{
#if USE_CURL
	std::ofstream target_file(target.c_str(), std::ios::out | std::ios::binary);
	if (!target_file.good()) {
		return false;
	}

	HTTPFetchRequest fetch_request;
	HTTPFetchResult fetch_result;
	fetch_request.url = url;
	fetch_request.caller = HTTPFETCH_SYNC;
	fetch_request.timeout = g_settings->getS32("curl_file_download_timeout");
	httpfetch_sync(fetch_request, fetch_result);

	if (!fetch_result.succeeded) {
		target_file.close();
		fs::DeleteSingleFileOrEmptyDirectory(target);
		return false;
	}
	target_file << fetch_result.data;

	return true;
#else
	return false;
#endif
}

/******************************************************************************/
void GUIEngine::setTopleftText(const std::string &text)
{
	m_toplefttext = translate_string(utf8_to_wide(text));

	updateTopLeftTextSize();
}

/******************************************************************************/
void GUIEngine::updateTopLeftTextSize()
{
	core::rect<s32> rect(0, 0, g_fontengine->getTextWidth(m_toplefttext.c_str()),
		g_fontengine->getTextHeight());
	rect += v2s32(4, 0);

	m_irr_toplefttext->remove();
	m_irr_toplefttext = gui::StaticText::add(RenderingEngine::get_gui_env(),
			m_toplefttext, rect, false, true, 0, -1);
}

/******************************************************************************/
s32 GUIEngine::playSound(SimpleSoundSpec spec, bool looped)
{
	s32 handle = m_sound_manager->playSound(spec, looped);
	return handle;
}

/******************************************************************************/
void GUIEngine::stopSound(s32 handle)
{
	m_sound_manager->stopSound(handle);
}

/******************************************************************************/
unsigned int GUIEngine::queueAsync(const std::string &serialized_func,
		const std::string &serialized_params)
{
	return m_script->queueAsync(serialized_func, serialized_params);
}

