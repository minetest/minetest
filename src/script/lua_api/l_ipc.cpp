// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "lua_api/l_ipc.h"
#include "lua_api/l_internal.h"
#include "common/c_packer.h"
#include "server.h"
#include "debug.h"

typedef std::shared_lock<std::shared_mutex> SharedReadLock;
typedef std::unique_lock<std::shared_mutex> SharedWriteLock;

int ModApiIPC::l_ipc_get(lua_State *L)
{
	auto *store = getGameDef(L)->getModIPCStore();

	auto key = readParam<std::string>(L, 1);

	{
		SharedReadLock autolock(store->mutex);
		auto it = store->map.find(key);
		if (it == store->map.end())
			lua_pushnil(L);
		else
			script_unpack(L, it->second.get());
	}
	return 1;
}

int ModApiIPC::l_ipc_set(lua_State *L)
{
	auto *store = getGameDef(L)->getModIPCStore();

	auto key = readParam<std::string>(L, 1);

	luaL_checkany(L, 2);
	std::unique_ptr<PackedValue> pv;
	if (!lua_isnil(L, 2)) {
		pv.reset(script_pack(L, 2));
		if (pv->contains_userdata)
			throw LuaError("Userdata not allowed");
	}

	{
		SharedWriteLock autolock(store->mutex);
		if (pv)
			store->map[key] = std::move(pv);
		else
			store->map.erase(key); // delete the map value for nil
	}
	return 0;
}

/*
 * Implementation note:
 * Iterating over the IPC table is intentionally not supported.
 * Mods should know what they have set.
 * This has the nice side effect that mods are able to use a randomly generated key
 * if they really *really* want to avoid other code touching their data. 
 */

void ModApiIPC::Initialize(lua_State *L, int top)
{
	FATAL_ERROR_IF(!getGameDef(L)->getModIPCStore(), "ModIPCStore missing from gamedef");

	API_FCT(ipc_get);
	API_FCT(ipc_set);
}
