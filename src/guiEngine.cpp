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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "irrlicht.h"

#include "porting.h"
#include "filesys.h"
#include "main.h"
#include "settings.h"
#include "guiMainMenu.h"

#include "guiEngine.h"

#if USE_CURL
#include <curl/curl.h>
#endif

/******************************************************************************/
int menuscript_ErrorHandler(lua_State *L) {
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
	lua_pop(L, 1);
	return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
	lua_pop(L, 2);
	return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

/******************************************************************************/
TextDestGuiEngine::TextDestGuiEngine(GUIEngine* engine)
{
	m_engine = engine;
}

/******************************************************************************/
void TextDestGuiEngine::gotText(std::map<std::string, std::string> fields)
{
	m_engine->handleButtons(fields);
}

/******************************************************************************/
void TextDestGuiEngine::gotText(std::wstring text)
{
	m_engine->handleEvent(wide_to_narrow(text));
}

/******************************************************************************/
GUIEngine::GUIEngine(	irr::IrrlichtDevice* dev,
						gui::IGUIElement* parent,
						IMenuManager *menumgr,
						scene::ISceneManager* smgr,
						MainMenuData* data) :
	m_device(dev),
	m_parent(parent),
	m_menumanager(menumgr),
	m_smgr(smgr),
	m_data(data),
	m_formspecgui(0),
	m_buttonhandler(0),
	m_menu(0),
	m_startgame(false),
	m_engineluastack(0),
	m_luaerrorhandler(-1),
	m_scriptdir(""),
	m_irr_toplefttext(0),
	m_clouds_enabled(true),
	m_cloud()
{
	//initialize texture pointers
	for (unsigned int i = 0; i < TEX_LAYER_MAX; i++) {
		m_textures[i] = 0;
	}
	// is deleted by guiformspec!
	m_buttonhandler = new TextDestGuiEngine(this);

	//create luastack
	m_engineluastack = luaL_newstate();

	//load basic lua modules
	luaL_openlibs(m_engineluastack);

	//init
	guiLuaApi::initialize(m_engineluastack,this);

	//push errorstring
	if (m_data->errormessage != "")
	{
		lua_getglobal(m_engineluastack, "gamedata");
		int gamedata_idx = lua_gettop(m_engineluastack);
		lua_pushstring(m_engineluastack, "errormessage");
		lua_pushstring(m_engineluastack,m_data->errormessage.c_str());
		lua_settable(m_engineluastack, gamedata_idx);
		m_data->errormessage = "";
	}

	//create topleft header
	core::rect<s32> rect(0, 0, 500, 40);
	rect += v2s32(4, 0);
	std::string t = "Minetest " VERSION_STRING;

	m_irr_toplefttext =
		m_device->getGUIEnvironment()->addStaticText(narrow_to_wide(t).c_str(),
		rect,false,true,0,-1);

	//create formspecsource
	m_formspecgui = new FormspecFormSource("",&m_formspecgui);

	/* Create menu */
	m_menu =
		new GUIFormSpecMenu(	m_device,
								m_parent,
								-1,
								m_menumanager,
								0 /* &client */,
								0 /* gamedef */);

	m_menu->allowClose(false);
	m_menu->lockSize(true,v2u32(800,600));
	m_menu->setFormSource(m_formspecgui);
	m_menu->setTextDest(m_buttonhandler);
	m_menu->useGettext(true);

	std::string builtin_helpers
		= porting::path_share + DIR_DELIM + "builtin"
			+ DIR_DELIM + "mainmenu_helper.lua";

	if (!runScript(builtin_helpers)) {
		errorstream
			<< "GUIEngine::GUIEngine unable to load builtin helper script"
			<< std::endl;
		return;
	}

	std::string menuscript = "";
	if (g_settings->exists("main_menu_script"))
		menuscript = g_settings->get("main_menu_script");
	std::string builtin_menuscript =
			porting::path_share + DIR_DELIM + "builtin"
				+ DIR_DELIM + "mainmenu.lua";

	lua_pushcfunction(m_engineluastack, menuscript_ErrorHandler);
	m_luaerrorhandler = lua_gettop(m_engineluastack);

	m_scriptdir = menuscript.substr(0,menuscript.find_last_of(DIR_DELIM)-1);
	if((menuscript == "") || (!runScript(menuscript))) {
		infostream
			<< "GUIEngine::GUIEngine execution of custom menu failed!"
			<< std::endl
			<< "\tfalling back to builtin menu"
			<< std::endl;
		m_scriptdir = fs::RemoveRelativePathComponents(porting::path_share + DIR_DELIM + "builtin"+ DIR_DELIM);
		if(!runScript(builtin_menuscript)) {
			errorstream
				<< "GUIEngine::GUIEngine unable to load builtin menu"
				<< std::endl;
			assert("no future without mainmenu" == 0);
		}
	}

	run();

	m_menumanager->deletingMenu(m_menu);
	m_menu->drop();
	m_menu = 0;
}

/******************************************************************************/
bool GUIEngine::runScript(std::string script) {

	int ret = 	luaL_loadfile(m_engineluastack, script.c_str()) ||
				lua_pcall(m_engineluastack, 0, 0, m_luaerrorhandler);
	if(ret){
		errorstream<<"========== ERROR FROM LUA WHILE CREATING MAIN MENU ==========="<<std::endl;
		errorstream<<"Failed to load and run script from "<<std::endl;
		errorstream<<script<<":"<<std::endl;
		errorstream<<std::endl;
		errorstream<<lua_tostring(m_engineluastack, -1)<<std::endl;
		errorstream<<std::endl;
		errorstream<<"=================== END OF ERROR FROM LUA ===================="<<std::endl;
		lua_pop(m_engineluastack, 1); // Pop error message from stack
		lua_pop(m_engineluastack, 1); // Pop the error handler from stack
		return false;
	}
	return true;
}

/******************************************************************************/
void GUIEngine::run()
{

	// Always create clouds because they may or may not be
	// needed based on the game selected
	video::IVideoDriver* driver = m_device->getVideoDriver();

	cloudInit();

	while(m_device->run() && (!m_startgame)) {
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
	}

	m_menu->quitMenu();
}

/******************************************************************************/
void GUIEngine::handleEvent(std::string text)
{
	lua_getglobal(m_engineluastack, "engine");

	lua_getfield(m_engineluastack, -1, "event_handler");

	if(lua_isnil(m_engineluastack, -1))
		return;

	luaL_checktype(m_engineluastack, -1, LUA_TFUNCTION);

	lua_pushstring(m_engineluastack, text.c_str());

	if(lua_pcall(m_engineluastack, 1, 0, m_luaerrorhandler))
		scriptError("error: %s", lua_tostring(m_engineluastack, -1));
}

/******************************************************************************/
void GUIEngine::handleButtons(std::map<std::string, std::string> fields)
{
	lua_getglobal(m_engineluastack, "engine");

	lua_getfield(m_engineluastack, -1, "button_handler");

	if(lua_isnil(m_engineluastack, -1))
		return;

	luaL_checktype(m_engineluastack, -1, LUA_TFUNCTION);

	lua_newtable(m_engineluastack);
	for(std::map<std::string, std::string>::const_iterator
		i = fields.begin(); i != fields.end(); i++){
		const std::string &name = i->first;
		const std::string &value = i->second;
		lua_pushstring(m_engineluastack, name.c_str());
		lua_pushlstring(m_engineluastack, value.c_str(), value.size());
		lua_settable(m_engineluastack, -3);
	}

	if(lua_pcall(m_engineluastack, 1, 0, m_luaerrorhandler))
		scriptError("error: %s", lua_tostring(m_engineluastack, -1));
}

/******************************************************************************/
GUIEngine::~GUIEngine()
{
	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver != 0);
	
	//TODO: clean up m_menu here

	lua_close(m_engineluastack);

	m_irr_toplefttext->setText(L"");

	//initialize texture pointers
	for (unsigned int i = 0; i < TEX_LAYER_MAX; i++) {
		if (m_textures[i] != 0)
			driver->removeTexture(m_textures[i]);
	}
	
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

	video::ITexture* texture = m_textures[TEX_LAYER_BACKGROUND];

	/* If no texture, draw background of solid color */
	if(!texture){
		video::SColor color(255,80,58,37);
		core::rect<s32> rect(0, 0, screensize.X, screensize.Y);
		driver->draw2DRectangle(color, rect, NULL);
		return;
	}

	/* Draw background texture */
	v2u32 sourcesize = texture->getSize();
	driver->draw2DImage(texture,
		core::rect<s32>(0, 0, screensize.X, screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/******************************************************************************/
void GUIEngine::drawOverlay(video::IVideoDriver* driver)
{
	v2u32 screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_OVERLAY];

	/* If no texture, draw background of solid color */
	if(!texture)
		return;

	/* Draw background texture */
	v2u32 sourcesize = texture->getSize();
	driver->draw2DImage(texture,
		core::rect<s32>(0, 0, screensize.X, screensize.Y),
		core::rect<s32>(0, 0, sourcesize.X, sourcesize.Y),
		NULL, NULL, true);
}

/******************************************************************************/
void GUIEngine::drawHeader(video::IVideoDriver* driver)
{
	core::dimension2d<u32> screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_HEADER];

	/* If no texture, draw nothing */
	if(!texture)
		return;

	f32 mult = (((f32)screensize.Width / 2)) /
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
		core::dimension2di(texture->getSize())),
		NULL, NULL, true);
	}
}

/******************************************************************************/
void GUIEngine::drawFooter(video::IVideoDriver* driver)
{
	core::dimension2d<u32> screensize = driver->getScreenSize();

	video::ITexture* texture = m_textures[TEX_LAYER_FOOTER];

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
			core::dimension2di(texture->getSize())),
			NULL, NULL, true);
	}
}

/******************************************************************************/
bool GUIEngine::setTexture(texture_layer layer,std::string texturepath) {

	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver != 0);

	if (m_textures[layer] != 0)
	{
		driver->removeTexture(m_textures[layer]);
		m_textures[layer] = 0;
	}

	if ((texturepath == "") || !fs::PathExists(texturepath))
		return false;

	m_textures[layer] = driver->getTexture(texturepath.c_str());

	if (m_textures[layer] == 0) return false;

	return true;
}

/******************************************************************************/
#if USE_CURL
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	FILE* targetfile = (FILE*) userp;
	fwrite(contents,size,nmemb,targetfile);
	return size * nmemb;
}
#endif
bool GUIEngine::downloadFile(std::string url,std::string target) {
#if USE_CURL
	//download file via curl
	CURL *curl;

	curl = curl_easy_init();

	if (curl)
	{
		CURLcode res;
		bool retval = true;

		FILE* targetfile = fopen(target.c_str(),"wb");

		if (targetfile) {
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, targetfile);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				errorstream << "File at url \"" << url
					<<"\" not found (" << curl_easy_strerror(res) << ")" <<std::endl;
				retval = false;
			}
			fclose(targetfile);
		}
		else {
			retval = false;
		}

		curl_easy_cleanup(curl);
		return retval;
	}
#endif
	return false;
}

/******************************************************************************/
void GUIEngine::scriptError(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char buf[10000];
	vsnprintf(buf, 10000, fmt, argp);
	va_end(argp);
	errorstream<<"MAINMENU ERROR: "<<buf;
}

/******************************************************************************/
void GUIEngine::setTopleftText(std::string append) {
	std::string toset = "Minetest " VERSION_STRING;

	if (append != "") {
		toset += " / ";
		toset += append;
	}

	m_irr_toplefttext->setText(narrow_to_wide(toset).c_str());
}
