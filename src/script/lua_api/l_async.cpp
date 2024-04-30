/*
Minetest
Copyright (C) 2024 y5nw <y5nw@outlook.com>

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

#include "lua_api/l_internal.h"
#include "lua_api/l_async.h"
#include "cpp_api/s_async.h"

// garbage collector
int LuaAsyncJob::gc_object(lua_State *L)
{
	LuaAsyncJob *o = *(LuaAsyncJob **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// get_id() -> id
int LuaAsyncJob::l_get_id(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaAsyncJob *o = checkObject<LuaAsyncJob>(L, 1);
	lua_pushinteger(L, o->get_id());
	return 1;
}

static std::string get_serialized_function(lua_State *L, int index)
{
	luaL_checktype(L, index, LUA_TFUNCTION);
	call_string_dump(L, index);
	size_t func_length;
	const char *serialized_func_raw = lua_tolstring(L, -1, &func_length);
	return std::string(serialized_func_raw, func_length);
}

int LuaAsyncJob::create(lua_State *L, const int id)
{
	NO_MAP_LOCK_REQUIRED;
	LuaAsyncJob *o = new LuaAsyncJob(id);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

void LuaAsyncJob::Register(lua_State *L)
{
	static const luaL_Reg metamethods[] = {
		{"__gc", gc_object},
		{0, 0}
	};

	registerClass(L, className, methods, metamethods);

	// Expose __index to be complemented by Lua.
	lua_getglobal(L, "core");
	luaL_getmetatable(L, className);
	lua_getfield(L, -1, "__index");
	lua_setfield(L, -3, "async_job_methods");
}

const char LuaAsyncJob::className[] = "AsyncJob";
const luaL_Reg LuaAsyncJob::methods[] = {
	luamethod(LuaAsyncJob, get_id),
	{0, 0}
};


// do_async_callback(func, params, mod_origin)
int ModApiAsync::l_do_async_callback(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ScriptApiAsync *script = getScriptApi<ScriptApiAsync>(L);

	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_checktype(L, 3, LUA_TSTRING);

	auto serialized_func = get_serialized_function(L, 1);
	PackedValue *param = script_pack(L, 2);
	std::string mod_origin = readParam<std::string>(L, 3);

	u32 jobId = script->queueAsync(
		std::move(serialized_func),
		param, mod_origin);

	lua_settop(L, 0);
	LuaAsyncJob::create(L, jobId);
	return 1;
}

// cancel_async_callback(id)
int ModApiAsync::l_cancel_async_callback(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ScriptApiAsync *script = getScriptApi<ScriptApiAsync>(L);
	int id = luaL_checkinteger(L, 1);
	lua_settop(L, 0);
	lua_pushboolean(L, script->cancelAsync(id));
	return 1;
}

// get_async_capacity()
int ModApiAsync::l_get_async_threading_capacity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ScriptApiAsync *script = getScriptApi<ScriptApiAsync>(L);
	lua_settop(L, 0);
	lua_pushinteger(L, script->getThreadingCapacity());
	return 1;
}

void ModApiAsync::Initialize(lua_State *L, int top)
{
	API_FCT(do_async_callback);
	API_FCT(cancel_async_callback);
	API_FCT(get_async_threading_capacity);
}
