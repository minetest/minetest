/*
Minetest
Copyright (C) 2024 y5nw <y5nw@protonmail.com>

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

class ModApiAsync : public ModApiBase
{
public:
	static void Initialize(lua_State *L, int top);
private:
	// do_async_callback(func, params, mod_origin)
	static int l_do_async_callback(lua_State *L);
	// cancel_async_callback(id)
	static int l_cancel_async_callback(lua_State *L);
	// get_async_threading_capacity()
	static int l_get_async_threading_capacity(lua_State *L);
};
