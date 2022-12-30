/*
Minetest
Copyright (C) 2022 sfan5 <sfan5@live.de>

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

#include "scripting_mapgen.h"
#include "emerge.h"
#include "server.h"
#include "filesys.h"
#include "settings.h"
#include "cpp_api/s_internal.h"
#include "common/c_packer.h"
#include "lua_api/l_areastore.h"
#include "lua_api/l_base.h"
#include "lua_api/l_craft.h"
#include "lua_api/l_env.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_mapgen.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_server.h"
#include "lua_api/l_util.h"
#include "lua_api/l_vmanip.h"
#include "lua_api/l_settings.h"

extern "C" {
#include <lualib.h>
}

MapgenScripting::MapgenScripting(Server *server):
		ScriptApiBase(ScriptingType::Mapgen)
{
	setGameDef(server);

	SCRIPTAPI_PRECHECKHEADER

	if (g_settings->getBool("secure.enable_security"))
		initializeSecurity();

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	InitializeModApi(L, top);

	auto *data = ModApiBase::getServer(L)->m_lua_globals_data.get();
	assert(data);
	script_unpack(L, data);
	lua_setfield(L, top, "transferred_globals");

	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "mapgen");
	lua_setglobal(L, "INIT");

	try {
		loadMod(Server::getBuiltinLuaPath() + DIR_DELIM + "init.lua",
			BUILTIN_MOD_NAME);
		checkSetByBuiltin();
	} catch (const ModError &e) {
		errorstream << "Execution of mapgen base environment failed: "
			<< e.what() << std::endl;
		FATAL_ERROR("Execution of mapgen base environment failed");
	}
}

void MapgenScripting::InitializeModApi(lua_State *L, int top)
{
	// Register reference classes (userdata)
	InvRef::Register(L);
	ItemStackMetaRef::Register(L);
	LuaAreaStore::Register(L);
	LuaItemStack::Register(L);
	LuaPerlinNoise::Register(L);
	LuaPerlinNoiseMap::Register(L);
	LuaPseudoRandom::Register(L);
	LuaPcgRandom::Register(L);
	LuaSecureRandom::Register(L);
	LuaVoxelManip::Register(L);
	NodeMetaRef::Register(L);
	NodeTimerRef::Register(L);
	LuaSettings::Register(L);

	// Initialize mod api modules
	ModApiCraft::InitializeAsync(L, top);
	//ModApiEnv: should have Lua replacements
	ModApiItemMod::InitializeAsync(L, top);
	//ModApiMapgen: we should have part of these
	ModApiServer::InitializeAsync(L, top);
	ModApiUtil::InitializeAsync(L, top);
}
