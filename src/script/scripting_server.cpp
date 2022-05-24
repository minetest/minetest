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

#include "scripting_server.h"
#include "server.h"
#include "log.h"
#include "settings.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_areastore.h"
#include "lua_api/l_auth.h"
#include "lua_api/l_base.h"
#include "lua_api/l_craft.h"
#include "lua_api/l_env.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_mapgen.h"
#include "lua_api/l_modchannels.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_object.h"
#include "lua_api/l_playermeta.h"
#include "lua_api/l_particles.h"
#include "lua_api/l_rollback.h"
#include "lua_api/l_server.h"
#include "lua_api/l_util.h"
#include "lua_api/l_vmanip.h"
#include "lua_api/l_settings.h"
#include "lua_api/l_http.h"
#include "lua_api/l_storage.h"

extern "C" {
#include <lualib.h>
}

ServerScripting::ServerScripting(Server* server):
		ScriptApiBase(ScriptingType::Server),
		asyncEngine(server)
{
	setGameDef(server);

	// setEnv(env) is called by ScriptApiEnv::initializeEnvironment()
	// once the environment has been created

	SCRIPTAPI_PRECHECKHEADER

	if (g_settings->getBool("secure.enable_security")) {
		initializeSecurity();
	} else {
		warningstream << "\\!/ Mod security should never be disabled, as it allows any mod to "
				<< "access the host machine."
				<< "Mods should use minetest.request_insecure_environment() instead \\!/" << std::endl;
	}

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");

	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");

	// Initialize our lua_api modules
	InitializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "game");
	lua_setglobal(L, "INIT");

	infostream << "SCRIPTAPI: Initialized game modules" << std::endl;
}

void ServerScripting::initAsync()
{
	// Save globals to transfer
	{
		lua_State *L = getStack();
		lua_getglobal(L, "core");
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "get_globals_to_transfer");
		lua_call(L, 0, 1);
		auto *data = script_pack(L, -1);
		assert(!data->contains_userdata);
		getServer()->m_async_globals_data.reset(data);
		lua_pushnil(L);
		lua_setfield(L, -3, "get_globals_to_transfer"); // unset function too
		lua_pop(L, 2); // pop 'core', return value
	}

	infostream << "SCRIPTAPI: Initializing async engine" << std::endl;
	asyncEngine.registerStateInitializer(InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiUtil::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiCraft::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiItemMod::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiServer::InitializeAsync);
	// not added: ModApiMapgen is a minefield for thread safety
	// not added: ModApiHttp async api can't really work together with our jobs
	// not added: ModApiStorage is probably not thread safe(?)

	asyncEngine.initialize(0);
}

void ServerScripting::stepAsync()
{
	asyncEngine.step(getStack());
}

u32 ServerScripting::queueAsync(std::string &&serialized_func,
	PackedValue *param, const std::string &mod_origin)
{
	return asyncEngine.queueAsyncJob(std::move(serialized_func),
			param, mod_origin);
}

void ServerScripting::InitializeModApi(lua_State *L, int top)
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
	LuaRaycast::Register(L);
	LuaSecureRandom::Register(L);
	LuaVoxelManip::Register(L);
	NodeMetaRef::Register(L);
	NodeTimerRef::Register(L);
	ObjectRef::Register(L);
	PlayerMetaRef::Register(L);
	LuaSettings::Register(L);
	StorageRef::Register(L);
	ModChannelRef::Register(L);

	// Initialize mod api modules
	ModApiAuth::Initialize(L, top);
	ModApiCraft::Initialize(L, top);
	ModApiEnvMod::Initialize(L, top);
	ModApiInventory::Initialize(L, top);
	ModApiItemMod::Initialize(L, top);
	ModApiMapgen::Initialize(L, top);
	ModApiParticles::Initialize(L, top);
	ModApiRollback::Initialize(L, top);
	ModApiServer::Initialize(L, top);
	ModApiUtil::Initialize(L, top);
	ModApiHttp::Initialize(L, top);
	ModApiStorage::Initialize(L, top);
	ModApiChannels::Initialize(L, top);
}

void ServerScripting::InitializeAsync(lua_State *L, int top)
{
	// classes
	LuaItemStack::Register(L);
	LuaPerlinNoise::Register(L);
	LuaPerlinNoiseMap::Register(L);
	LuaPseudoRandom::Register(L);
	LuaPcgRandom::Register(L);
	LuaSecureRandom::Register(L);
	LuaVoxelManip::Register(L);
	LuaSettings::Register(L);

	// globals data
	lua_getglobal(L, "core");
	luaL_checktype(L, -1, LUA_TTABLE);
	auto *data = ModApiBase::getServer(L)->m_async_globals_data.get();
	script_unpack(L, data);
	lua_setfield(L, -2, "transferred_globals");
	lua_pop(L, 1); // pop 'core'
}
