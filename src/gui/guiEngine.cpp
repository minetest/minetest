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
#include "client/renderingengine.h"
#include "client/startup_screen.h"
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
#include "httpfetch.h"
#include "log.h"
#include "client/fontengine.h"
#include "client/guiscalingfilter.h"
#include "irrlicht_changes/static_text.h"

#if ENABLE_GLES
#include "client/tile.h"
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

#if ENABLE_GLES
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
		StartupScreen *startup_screen,
		gui::IGUIElement *parent,
		IMenuManager *menumgr,
		MainMenuData *data,
		bool &kill) :
	m_startup_screen(startup_screen),
	m_parent(parent),
	m_menumanager(menumgr),
	m_smgr(RenderingEngine::get_scene_manager()),
	m_data(data),
	m_kill(kill)
{
	// is deleted by guiformspec!
	m_buttonhandler = new TextDestGuiEngine(this);

	//create texture source
	m_texture_source = new MenuTextureSource(RenderingEngine::get_video_driver());

	//create soundmanager
	MenuMusicFetcher soundfetcher;
#if USE_SOUND
	if (g_settings->getBool("enable_sound") && g_sound_manager_singleton.get())
		m_sound_manager = createOpenALSoundManager(g_sound_manager_singleton.get(), &soundfetcher);
#endif
	if (!m_sound_manager)
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
	unsigned int text_height = g_fontengine->getTextHeight();

	while (RenderingEngine::run() && (!m_startgame) && (!m_kill)) {

		//check if we need to update the "upper left corner"-text
		if (text_height != g_fontengine->getTextHeight()) {
			updateTopLeftTextSize();
			text_height = g_fontengine->getTextHeight();
		}

		m_startup_screen->step(true);
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

	delete m_texture_source;
}

/******************************************************************************/
void GUIEngine::setFormspecPrepend(const std::string &fs)
{
	if (m_menu) {
		m_menu->setFormspecPrepend(fs);
	}
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
s32 GUIEngine::playSound(const SimpleSoundSpec &spec, bool looped)
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
