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
#include "scripting_mainmenu.h"
#include "util/numeric.h"
#include "config.h"
#include "version.h"
#include "porting.h"
#include "filesys.h"
#include "main.h"
#include "settings.h"
#include "guiMainMenu.h"
#include "sound.h"
#include "sound_openal.h"
#include "clouds.h"
#include "httpfetch.h"
#include "log.h"
#ifdef __ANDROID__
#include "tile.h"
#include <GLES/gl.h>
#endif


/******************************************************************************/
/** TextDestGuiEngine                                                         */
/******************************************************************************/
TextDestGuiEngine::TextDestGuiEngine(GUIEngine* engine)
{
	m_engine = engine;
}

/******************************************************************************/
void TextDestGuiEngine::gotText(std::map<std::string, std::string> fields)
{
	m_engine->getScriptIface()->handleMainMenuButtons(fields);
}

/******************************************************************************/
void TextDestGuiEngine::gotText(std::wstring text)
{
	m_engine->getScriptIface()->handleMainMenuEvent(wide_to_narrow(text));
}

/******************************************************************************/
/** MenuTextureSource                                                         */
/******************************************************************************/
MenuTextureSource::MenuTextureSource(video::IVideoDriver *driver)
{
	m_driver = driver;
}

/******************************************************************************/
MenuTextureSource::~MenuTextureSource()
{
	for (std::set<std::string>::iterator it = m_to_delete.begin();
			it != m_to_delete.end(); ++it) {
		const char *tname = (*it).c_str();
		video::ITexture *texture = m_driver->getTexture(tname);
		m_driver->removeTexture(texture);
	}
}

/******************************************************************************/
video::ITexture* MenuTextureSource::getTexture(const std::string &name, u32 *id)
{
	if(id)
		*id = 0;
	if(name.empty())
		return NULL;
	m_to_delete.insert(name);

#ifdef __ANDROID__
	video::IImage *image = m_driver->createImageFromFile(name.c_str());
	if (image) {
		image = Align2Npot2(image, m_driver);
		video::ITexture* retval = m_driver->addTexture(name.c_str(), image);
		image->drop();
		return retval;
	}
#endif
	return m_driver->getTexture(name.c_str());
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
GUIEngine::GUIEngine(	irr::IrrlichtDevice* dev,
						gui::IGUIElement* parent,
						IMenuManager *menumgr,
						scene::ISceneManager* smgr,
						MainMenuData* data,
						bool& kill) :
	m_device(dev),
	m_parent(parent),
	m_menumanager(menumgr),
	m_smgr(smgr),
	m_data(data),
	m_texture_source(NULL),
	m_sound_manager(NULL),
	m_formspecgui(0),
	m_buttonhandler(0),
	m_menu(0),
	m_kill(kill),
	m_startgame(false),
	m_script(0),
	m_scriptdir(""),
	m_irr_toplefttext(0),
	m_clouds_enabled(true),
	m_cloud()
{
	//initialize texture pointers
	for (unsigned int i = 0; i < TEX_LAYER_MAX; i++) {
		m_textures[i].texture = NULL;
	}
	// is deleted by guiformspec!
	m_buttonhandler = new TextDestGuiEngine(this);

	//create texture source
	m_texture_source = new MenuTextureSource(m_device->getVideoDriver());

	//create soundmanager
	MenuMusicFetcher soundfetcher;
#if USE_SOUND
	m_sound_manager = createOpenALSoundManager(&soundfetcher);
#endif
	if(!m_sound_manager)
		m_sound_manager = &dummySoundManager;

	//create topleft header
	core::rect<s32> rect(0, 0, 500, 20);
	rect += v2s32(4, 0);
	std::string t = std::string("Minetest ") + minetest_version_hash;

	m_irr_toplefttext =
		m_device->getGUIEnvironment()->addStaticText(narrow_to_wide(t).c_str(),
		rect,false,true,0,-1);

	//create formspecsource
	m_formspecgui = new FormspecFormSource("");

	/* Create menu */
	m_menu = new GUIFormSpecMenu(m_device,
			m_parent,
			-1,
			m_menumanager,
			NULL /* &client */,
			NULL /* gamedef */,
			m_texture_source,
			m_formspecgui,
			m_buttonhandler,
			NULL);

	m_menu->allowClose(false);
	m_menu->lockSize(true,v2u32(800,600));

	// Initialize scripting

	infostream << "GUIEngine: Initializing Lua" << std::endl;

	m_script = new MainMenuScripting(this);

	try {
		if (m_data->errormessage != "") {
			m_script->setMainMenuErrorMessage(m_data->errormessage);
			m_data->errormessage = "";
		}

		if (!loadMainMenuScript()) {
			errorstream << "No future without mainmenu" << std::endl;
			abort();
		}

		run();
	}
	catch(LuaError &e) {
		errorstream << "MAINMENU ERROR: " << e.what() << std::endl;
		m_data->errormessage = e.what();
	}

	m_menu->quitMenu();
	m_menu->drop();
	m_menu = NULL;
}

/******************************************************************************/
bool GUIEngine::loadMainMenuScript()
{
	// Try custom menu script (main_menu_path)

	m_scriptdir = g_settings->get("main_menu_path");
	if (m_scriptdir.empty()) {
		m_scriptdir = porting::path_share + DIR_DELIM "builtin" + DIR_DELIM "mainmenu";
	}

	std::string script = porting::path_share + DIR_DELIM "builtin" + DIR_DELIM "init.lua";
	if (m_script->loadScript(script)) {
		// Menu script loaded
		return true;
	} else {
		infostream
			<< "GUIEngine: execution of menu script in: \""
			<< m_scriptdir << "\" failed!" << std::endl;
	}

	return false;
}

/******************************************************************************/
void GUIEngine::run()
{
	// Always create clouds because they may or may not be
	// needed based on the game selected
	video::IVideoDriver* driver = m_device->getVideoDriver();

	cloudInit();

	while(m_device->run() && (!m_startgame) && (!m_kill)) {
		driver->beginScene(true, true, video::SColor(255,140,186,250));

		if (m_clouds_enabled)
		{
			cloudPreProcess();
			drawOverlay(driver);
		}
		else
			drawBackground(driver);

		drawHeader(driver);
		drawFooter(driver);

		m_device->getGUIEnvironment()->drawAll();

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
	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver != 0);

	if(m_sound_manager != &dummySoundManager){
		delete m_sound_manager;
		m_sound_manager = NULL;
	}

	infostream<<"GUIEngine: Deinitializing scripting"<<std::endl;
	delete m_script;

	m_irr_toplefttext->setText(L"");

	//clean up texture pointers
	for (unsigned int i = 0; i < TEX_LAYER_MAX; i++) {
		if (m_textures[i].texture != NULL)
			driver->removeTexture(m_textures[i].texture);
	}

	delete m_texture_source;

	if (m_cloud.clouds)
		m_cloud.clouds->drop();
}

/******************************************************************************/
void GUIEngine::cloudInit()
{
	m_cloud.clouds = new Clouds(m_smgr->getRootSceneNode(),
			m_smgr, -1, rand(), 100);
	m_cloud.clouds->update(v2f(0, 0), video::SColor(255,200,200,255));

	m_cloud.camera = m_smgr->addCameraSceneNode(0,
				v3f(0,0,0), v3f(0, 60, 100));
	m_cloud.camera->setFarValue(10000);

	m_cloud.lasttime = m_device->getTimer()->getTime();
}

/******************************************************************************/
void GUIEngine::cloudPreProcess()
{
	u32 time = m_device->getTimer()->getTime();

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
	float fps_max = g_settings->getFloat("fps_max");
	// Time of frame without fps limit
	float busytime;
	u32 busytime_u32;
	// not using getRealTime is necessary for wine
	u32 time = m_device->getTimer()->getTime();
	if(time > m_cloud.lasttime)
		busytime_u32 = time - m_cloud.lasttime;
	else
		busytime_u32 = 0;
	busytime = busytime_u32 / 1000.0;

	// FPS limiter
	u32 frametime_min = 1000./fps_max;

	if(busytime_u32 < frametime_min) {
		u32 sleeptime = frametime_min - busytime_u32;
		m_device->sleep(sleeptime);
	}
}

/******************************************************************************/
void GUIEngine::drawBackground(video::IVideoDriver* driver)
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
				driver->draw2DImage(texture,
					core::rect<s32>(x, y, x+tilesize.X, y+tilesize.Y),
					core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
					NULL, NULL, true);
			}
		}
		return;
	}

	/* Draw background texture */
	driver->draw2DImage(texture,
		core::rect<s32>(0, 0, screensize.X, screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/******************************************************************************/
void GUIEngine::drawOverlay(video::IVideoDriver* driver)
{
	v2u32 screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_OVERLAY].texture;

	/* If no texture, draw background of solid color */
	if(!texture)
		return;

	/* Draw background texture */
	v2u32 sourcesize = texture->getOriginalSize();
	driver->draw2DImage(texture,
		core::rect<s32>(0, 0, screensize.X, screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/******************************************************************************/
void GUIEngine::drawHeader(video::IVideoDriver* driver)
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

	// Don't draw the header is there isn't enough room
	s32 free_space = (((s32)screensize.Height)-320)/2;

	if (free_space > splashsize.Y) {
		core::rect<s32> splashrect(0, 0, splashsize.X, splashsize.Y);
		splashrect += v2s32((screensize.Width/2)-(splashsize.X/2),
				((free_space/2)-splashsize.Y/2)+10);

	video::SColor bgcolor(255,50,50,50);

	driver->draw2DImage(texture, splashrect,
		core::rect<s32>(core::position2d<s32>(0,0),
		core::dimension2di(texture->getOriginalSize())),
		NULL, NULL, true);
	}
}

/******************************************************************************/
void GUIEngine::drawFooter(video::IVideoDriver* driver)
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

		driver->draw2DImage(texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			NULL, NULL, true);
	}
}

/******************************************************************************/
bool GUIEngine::setTexture(texture_layer layer, std::string texturepath,
		bool tile_image, unsigned int minsize)
{
	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver != 0);

	if (m_textures[layer].texture != NULL)
	{
		driver->removeTexture(m_textures[layer].texture);
		m_textures[layer].texture = NULL;
	}

	if ((texturepath == "") || !fs::PathExists(texturepath))
	{
		return false;
	}

	m_textures[layer].texture = driver->getTexture(texturepath.c_str());
	m_textures[layer].tile    = tile_image;
	m_textures[layer].minsize = minsize;

	if (m_textures[layer].texture == NULL)
	{
		return false;
	}

	return true;
}

/******************************************************************************/
bool GUIEngine::downloadFile(std::string url, std::string target)
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
		return false;
	}
	target_file << fetch_result.data;

	return true;
#else
	return false;
#endif
}

/******************************************************************************/
void GUIEngine::setTopleftText(std::string append)
{
	std::string toset = std::string("Minetest ") + minetest_version_hash;

	if (append != "") {
		toset += " / ";
		toset += append;
	}

	m_irr_toplefttext->setText(narrow_to_wide(toset).c_str());
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
unsigned int GUIEngine::queueAsync(std::string serialized_func,
		std::string serialized_params)
{
	return m_script->queueAsync(serialized_func, serialized_params);
}

