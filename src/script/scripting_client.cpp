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

#include "scripting_client.h"
#include "client.h"
#include "log.h"
#include "settings.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_areastore.h"
#include "lua_api/l_base.h"
#include "lua_api/l_craft.h"
#include "lua_api/l_env.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "lua_api/l_mapgen.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_object.h"
#include "lua_api/l_particles.h"
#include "lua_api/l_rollback.h"
#include "lua_api/l_client.h"
#include "lua_api/l_util.h"
#include "lua_api/l_vmanip.h"
#include "lua_api/l_settings.h"
#include "lua_api/l_http.h"

extern "C" {
#include "lualib.h"
}

ClientScripting::ClientScripting(Client* client)
{
	setClient(client);
	setServer(NULL);

	// setEnv(env) is called by ScriptApiEnv::initializeEnvironment()
	// once the environment has been created

	SCRIPTAPI_PRECHECKHEADER

	initializeSecurity();

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
	lua_pushstring(L, "client");
	lua_setglobal(L, "INIT");

	infostream << "SCRIPTAPI: Initialized client modules" << std::endl;
}

void ClientScripting::InitializeModApi(lua_State *L, int top)
{
	
	// Initialize mod api modules
	ModApiCraft::Initialize(L, top);
	ModApiEnvMod::Initialize(L, top);
	ModApiInventory::Initialize(L, top);
	ModApiItemMod::Initialize(L, top);
	ModApiMapgen::Initialize(L, top);
	ModApiParticles::Initialize(L, top);
	ModApiRollback::Initialize(L, top);
	ModApiClient::Initialize(L, top);
	ModApiUtil::Initialize(L, top);
	ModApiHttp::Initialize(L, top);

	// Register reference classes (userdata)
	InvRef::Register(L);
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
	ObjectRef::Register(L);
	LuaSettings::Register(L);
}

int ClientScripting::count(const std::string filename)
{
	return m_files.count(filename);
}

std::string& ClientScripting::operator[] (const std::string filename)
{
	std::string modname = filename.substr(0, filename.find(MEDIA_MOD_SEPERATOR));
	// Add modname if it is not already in m_mods
	if (std::find(m_mods.begin(), m_mods.end(), modname) == m_mods.end()) {
		m_mods.push_back(modname);
	}
	return m_files[filename];
}

bool ClientScripting::contains(const std::string filename)
{
	return m_files.count(filename);
}

void ClientScripting::getModNames(std::vector<std::string> &modlist)
{
	std::vector<std::string>::iterator it;
	for (it = m_mods.begin(); it != m_mods.end(); ++it)
		modlist.push_back(*it);
}

void ClientScripting::loadScript(const std::string &path)
{	
	verbosestream << "Running script from " << path << std::endl;

	lua_State *L = getStack();

	int error_handler = PUSH_ERROR_HANDLER(L);

	bool ok;
	if (contains(path)) {
		std::string fileContents = m_files[path];
		ok = !luaL_loadbuffer(L, fileContents.c_str(), fileContents.length(), NULL);
	} else {
		ok = ScriptApiSecurity::safeLoadFile(L, path.c_str());
	}
	ok = ok && !lua_pcall(L, 0, 0, error_handler);
	if (!ok) {
		std::string error_msg = lua_tostring(L, -1);
		lua_pop(L, 2); // Pop error message and error handler
		throw ModError("Failed to load and run script from " +
				path + ":\n" + error_msg);
	}
	lua_pop(L, 1); // Pop error handler
}

void ClientScripting::loadMods()
{
	for(std::vector<std::string>::iterator it = m_mods.begin(); it != m_mods.end(); ++it) {
		std::string mod_name = *it;
		std::string mod_path = mod_name + MEDIA_MOD_SEPERATOR"init.lua";
		if (contains(mod_path)) loadScript(mod_path);
	}
}
