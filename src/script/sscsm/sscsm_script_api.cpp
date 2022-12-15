#include "sscsm_script_api.h"
#include "sscsm_message.h"
#include "sscsm_script_process.h"
#include "cmake_config.h"
#include "irr_v3d.h"
#include "nodedef.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#if USE_LUAJIT
#include "luajit.h"
#else
#include "bit.h"
#endif
}

void sscsm_load_libraries(lua_State *L)
{
	lua_pushcfunction(L, luaopen_base);
	lua_pushliteral(L, "");
	lua_call(L, 1, 0);
	lua_pushcfunction(L, luaopen_table);
	lua_pushliteral(L, LUA_TABLIBNAME);
	lua_call(L, 1, 0);
	lua_pushcfunction(L, luaopen_string);
	lua_pushliteral(L, LUA_STRLIBNAME);
	lua_call(L, 1, 0);
	lua_pushcfunction(L, luaopen_math);
	lua_pushliteral(L, LUA_MATHLIBNAME);
	lua_call(L, 1, 0);
	lua_pushcfunction(L, luaopen_bit);
	lua_pushliteral(L, LUA_BITLIBNAME);
	lua_call(L, 1, 0);

	// core
	lua_newtable(L);

	// core.get_node
	lua_pushcfunction(L, [](lua_State *L) -> int {
		v3s16 pos;
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_getfield(L, 1, "x");
		pos.X = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 1, "y");
		pos.Y = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, 1, "z");
		pos.Z = lua_tointeger(L, -1);
		lua_pop(L, 1);

		sscsm_send_msg_ex(g_sscsm_to_controller, SSCSMMsgType::S2C_GET_NODE,
				sizeof(v3s16), &pos);
		SSCSMRecvMsg msg = sscsm_recv_msg_ex(g_sscsm_from_controller);
		if (msg.type != SSCSMMsgType::C2S_GET_NODE || msg.data.size() < sizeof(MapNode)) {
			assert(false);
			return 0;
		}
		MapNode n;
		memcpy(&n, msg.data.data(), sizeof(MapNode));
		lua_createtable(L, 0, 3);
		lua_pushstring(L, g_sscsm_nodedef->get(n).name.c_str());
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, n.getParam1());
		lua_setfield(L, -2, "param1");
		lua_pushinteger(L, n.getParam2());
		lua_setfield(L, -2, "param2");
		return 1;
	});
	lua_setfield(L, -2, "get_node");

	lua_setglobal(L, "core");
}

void sscsm_load_mods(lua_State *L)
{
	luaL_dostring(L, "print('mods loaded', core.get_node({x=0,y=0,z=0}).name)");
	sscsm_send_msg_ex(g_sscsm_to_controller, SSCSMMsgType::S2C_DONE, 0, nullptr);
}

void sscsm_run_step(lua_State *L, float dtime)
{
	luaL_dostring(L, "print('step', core.get_node({x=0,y=0,z=0}).name)");
	sscsm_send_msg_ex(g_sscsm_to_controller, SSCSMMsgType::S2C_DONE, 0, nullptr);
}
