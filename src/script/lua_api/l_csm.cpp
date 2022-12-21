/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "l_csm.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "cpp_api/s_base.h"
#include "csm/csm_message.h"
#include "csm/csm_script_ipc.h"
#include "itemdef.h"
#include "nodedef.h"

// get_current_modname()
int ModApiCSM::l_get_current_modname(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	return 1;
}

// get_modpath(modname)
int ModApiCSM::l_get_modpath(lua_State *L)
{
	std::string modname = readParam<std::string>(L, 1);
	// CSMs use a virtual filesystem, see ModVFS::scanModSubfolder()
	std::string path = modname + ":";
	lua_pushstring(L, path.c_str());
	return 1;
}

// get_last_run_mod()
int ModApiCSM::l_get_last_run_mod(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	std::string current_mod = readParam<std::string>(L, -1, "");
	if (current_mod.empty()) {
		lua_pop(L, 1);
		lua_pushstring(L, getScriptApiBase(L)->getOrigin().c_str());
	}
	return 1;
}

// set_last_run_mod(modname)
int ModApiCSM::l_set_last_run_mod(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	const char *mod = lua_tostring(L, 1);
	getScriptApiBase(L)->setOriginDirect(mod);
	lua_pushboolean(L, true);
	return 1;
}

// print(text)
int ModApiCSM::l_print(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string text = luaL_checkstring(L, 1);
	rawstream << text << std::endl;
	return 0;
}

// get_node(pos)
int ModApiCSM::l_get_node(lua_State *L)
{
	CSMS2CGetNode send;
	send.pos = read_v3s16(L, 1);
	CSM_IPC(exchange(send));
	CSMC2SGetNode recv;
	sanity_check(csm_recv_size() >= sizeof(recv));
	memcpy(&recv, csm_recv_data(), sizeof(recv));
	pushnode(L, recv.n);
	return 1;
}

// get_node_or_nil(pos)
int ModApiCSM::l_get_node_or_nil(lua_State *L)
{
	CSMS2CGetNode send;
	send.pos = read_v3s16(L, 1);
	CSM_IPC(exchange(send));
	CSMC2SGetNode recv;
	sanity_check(csm_recv_size() >= sizeof(recv));
	memcpy(&recv, csm_recv_data(), sizeof(recv));
	if (recv.pos_ok) {
		pushnode(L, recv.n);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// get_item_def(itemstring)
int ModApiCSM::l_get_item_def(lua_State *L)
{
	IGameDef *gdef = getGameDef(L);
	assert(gdef);

	IItemDefManager *idef = gdef->idef();
	assert(idef);

	if (!lua_isstring(L, 1))
		return 0;

	std::string name = readParam<std::string>(L, 1);
	if (!idef->isKnown(name))
		return 0;
	const ItemDefinition &def = idef->get(name);

	push_item_definition_full(L, def);

	return 1;
}

// get_node_def(nodename)
int ModApiCSM::l_get_node_def(lua_State *L)
{
	IGameDef *gdef = getGameDef(L);
	assert(gdef);

	const NodeDefManager *ndef = gdef->ndef();
	assert(ndef);

	if (!lua_isstring(L, 1))
		return 0;

	std::string name = readParam<std::string>(L, 1);
	const ContentFeatures &cf = ndef->get(ndef->getId(name));
	if (cf.name != name) // Unknown node. | name = <whatever>, cf.name = ignore
		return 0;

	push_content_features(L, cf);

	return 1;
}

// get_builtin_path()
int ModApiCSM::l_get_builtin_path(lua_State *L)
{
	lua_pushstring(L, BUILTIN_MOD_NAME ":");
	return 1;
}

void ModApiCSM::Initialize(lua_State *L, int top)
{
	API_FCT(get_current_modname);
	API_FCT(get_modpath);
	API_FCT(print);
	API_FCT(set_last_run_mod);
	API_FCT(get_last_run_mod);
	API_FCT(get_node);
	API_FCT(get_node_or_nil);
	API_FCT(get_item_def);
	API_FCT(get_node_def);
	API_FCT(get_builtin_path);
}
