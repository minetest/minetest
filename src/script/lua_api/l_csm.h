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

#pragma once

#include "lua_api/l_base.h"
#include "itemdef.h"
#include "tool.h"

class ModApiCSM : public ModApiBase
{
private:
	// get_current_modname()
	static int l_get_current_modname(lua_State *L);

	// get_modpath(modname)
	static int l_get_modpath(lua_State *L);

	// print(text)
	static int l_print(lua_State *L);

	// get_last_run_mod(n)
	static int l_get_last_run_mod(lua_State *L);

	// set_last_run_mod(modname)
	static int l_set_last_run_mod(lua_State *L);

	// get_node(pos)
	static int l_get_node(lua_State *L);

	// get_node_or_nil(pos)
	static int l_get_node_or_nil(lua_State *L);

	// set_node(pos, node)
	static int l_set_node(lua_State *L);

	// add_node(pos, node)
	static int l_add_node(lua_State *L);

	// swap_node(pos, node)
	static int l_swap_node(lua_State *L);

	// get_meta(pos)
	static int l_get_meta(lua_State *L);

	// get_item_def(itemstring)
	static int l_get_item_def(lua_State *L);

	// get_node_def(nodename)
	static int l_get_node_def(lua_State *L);

	// get_builtin_path()
	static int l_get_builtin_path(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
