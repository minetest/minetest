/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "csm_scripting.h"
#include "csm_gamedef.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_csm.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_util.h"

CSMScripting::CSMScripting(CSMGameDef *gamedef):
		ScriptApiBase(ScriptingType::CSM)
{
	setGameDef(gamedef);

	SCRIPTAPI_PRECHECKHEADER

	initializeSecurityCSM();

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	// Initialize our lua_api modules
	InitializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "csm");
	lua_setglobal(L, "INIT");
}

void CSMScripting::InitializeModApi(lua_State *L, int top)
{
	ModApiCSM::Initialize(L, top);
	ModApiItemMod::InitializeCSM(L, top);
	ModApiUtil::InitializeCSM(L, top);
	LuaItemStack::Register(L);
	ItemStackMetaRef::Register(L);
}
