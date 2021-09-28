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

#pragma once

#include "cpp_api/s_base.h"
#include "cpp_api/s_mainmenu.h"
#include "cpp_api/s_async.h"

/*****************************************************************************/
/* Scripting <-> Main Menu Interface                                         */
/*****************************************************************************/

class MainMenuScripting
		: virtual public ScriptApiBase,
		  public ScriptApiMainMenu
{
public:
	MainMenuScripting(GUIEngine* guiengine);

	// Global step handler to pass back async events
	void step();

	// Pass async events from engine to async threads
	u32 queueAsync(std::string &&serialized_func,
		std::string &&serialized_param);

private:
	void initializeModApi(lua_State *L, int top);
	static void registerLuaClasses(lua_State *L, int top);

	AsyncEngine asyncEngine;
};
