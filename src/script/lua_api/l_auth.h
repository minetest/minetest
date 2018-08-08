/*
Minetest
Copyright (C) 2018 bendeutsch, Ben Deutsch <ben@bendeutsch.de>

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

class AuthDatabase;
struct AuthEntry;

class ModApiAuth : public ModApiBase
{
private:
	// auth_read(name)
	static int l_auth_read(lua_State *L);

	// auth_save(table)
	static int l_auth_save(lua_State *L);

	// auth_create(table)
	static int l_auth_create(lua_State *L);

	// auth_delete(name)
	static int l_auth_delete(lua_State *L);

	// auth_list_names()
	static int l_auth_list_names(lua_State *L);

	// auth_reload()
	static int l_auth_reload(lua_State *L);

	// helper for auth* methods
	static AuthDatabase *getAuthDb(lua_State *L);
	static void pushAuthEntry(lua_State *L, const AuthEntry &authEntry);

public:
	static void Initialize(lua_State *L, int top);
};
