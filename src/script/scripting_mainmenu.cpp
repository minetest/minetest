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
#include "log.h"
#include "filesys.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_base.h"
#include "lua_api/l_mainmenu.h"
#include "lua_api/l_util.h"
#include "lua_api/l_settings.h"

extern "C" {
#include "lualib.h"
	int luaopen_marshal(lua_State *L);
}
/******************************************************************************/
MainMenuScripting::MainMenuScripting(GUIEngine* guiengine)
{
	setGuiEngine(guiengine);

	//TODO add security

	luaL_openlibs(getStack());
	luaopen_marshal(getStack());

	SCRIPTAPI_PRECHECKHEADER

	lua_pushstring(L, DIR_DELIM);
	lua_setglobal(L, "DIR_DELIM");

	lua_newtable(L);
	lua_setglobal(L, "gamedata");

	lua_newtable(L);
	lua_setglobal(L, "engine");

	// Initialize our lua_api modules
	lua_getglobal(L, "engine");
	int top = lua_gettop(L);
	InitializeModApi(L, top);
	lua_pop(L, 1);

	infostream << "SCRIPTAPI: initialized mainmenu modules" << std::endl;
}

/******************************************************************************/
void MainMenuScripting::InitializeModApi(lua_State *L, int top)
{
	// Initialize mod api modules
	ModApiMainMenu::Initialize(L, top);
	ModApiUtil::Initialize(L, top);

	// Register reference classes (userdata)
	LuaSettings::Register(L);

	// Register functions to async environment
	ModApiMainMenu::InitializeAsync(m_AsyncEngine);
	ModApiUtil::InitializeAsync(m_AsyncEngine);

	// Initialize async environment
	//TODO possibly make number of async threads configurable
	m_AsyncEngine.Initialize(MAINMENU_NUMBER_OF_ASYNC_THREADS);
}

/******************************************************************************/
void MainMenuScripting::Step() {
	m_AsyncEngine.Step(getStack());
}

/******************************************************************************/
unsigned int MainMenuScripting::DoAsync(std::string serialized_fct,
		std::string serialized_params) {
	return m_AsyncEngine.doAsyncJob(serialized_fct,serialized_params);
}
