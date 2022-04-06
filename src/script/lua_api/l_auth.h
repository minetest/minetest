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
#include "lua_api/l_internal.h"

class AuthDatabase;
struct AuthEntry;

class ModApiAuth : public ModApiBase
{
private:
	// auth_read(name)
	ENTRY_POINT_DECL(l_auth_read);

	// auth_save(table)
	ENTRY_POINT_DECL(l_auth_save);

	// auth_create(table)
	ENTRY_POINT_DECL(l_auth_create);

	// auth_delete(name)
	ENTRY_POINT_DECL(l_auth_delete);

	// auth_list_names()
	ENTRY_POINT_DECL(l_auth_list_names);

	// auth_reload()
	ENTRY_POINT_DECL(l_auth_reload);

	// helper for auth* methods
	static AuthDatabase *getAuthDb(lua_State *L);
	static void pushAuthEntry(lua_State *L, const AuthEntry &authEntry);

public:
	static void Initialize(lua_State *L, int top);
};
