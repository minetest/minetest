/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "scripting_mainmenu.h"
#include "content/mods.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_base.h"
#include "lua_api/l_http.h"
#include "lua_api/l_mainmenu.h"
#include "lua_api/l_sound.h"
#include "lua_api/l_util.h"
#include "lua_api/l_settings.h"
#include "log.h"

extern "C" {
#include "lualib.h"
}
#define MAINMENU_NUM_ASYNC_THREADS 4


MainMenuScripting::MainMenuScripting(GUIEngine* guiengine):
		ScriptApiBase(ScriptingType::MainMenu)
{
	setGuiEngine(guiengine);

	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	lua_newtable(L);
	lua_setglobal(L, "gamedata");

	// Initialize our lua_api modules
	initializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "mainmenu");
	lua_setglobal(L, "INIT");

	infostream << "SCRIPTAPI: Initialized main menu modules" << std::endl;
}

/******************************************************************************/
void MainMenuScripting::initializeModApi(lua_State *L, int top)
{
	registerLuaClasses(L, top);

	// Initialize mod API modules
	ModApiMainMenu::Initialize(L, top);
	ModApiUtil::Initialize(L, top);
	ModApiSound::Initialize(L, top);
	ModApiHttp::Initialize(L, top);

	asyncEngine.registerStateInitializer(registerLuaClasses);
	asyncEngine.registerStateInitializer(ModApiMainMenu::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiUtil::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiHttp::InitializeAsync);

	// Initialize async environment
	//TODO possibly make number of async threads configurable
	asyncEngine.initialize(MAINMENU_NUM_ASYNC_THREADS);
}

/******************************************************************************/
void MainMenuScripting::registerLuaClasses(lua_State *L, int top)
{
	LuaSettings::Register(L);
}

/******************************************************************************/
void MainMenuScripting::step()
{
	asyncEngine.step(getStack());
}

/******************************************************************************/
u32 MainMenuScripting::queueAsync(std::string &&serialized_func,
		std::string &&serialized_param)
{
	return asyncEngine.queueAsyncJob(std::move(serialized_func), std::move(serialized_param));
}

