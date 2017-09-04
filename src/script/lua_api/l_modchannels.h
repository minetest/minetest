/*
Minetest
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

#pragma once

#include "lua_api/l_base.h"
#include "config.h"

class ModApiChannels : public ModApiBase
{
private:
	// mod_channel_join(name)
	static int l_mod_channel_join(lua_State *L);

	// mod_channel_leave(name)
	static int l_mod_channel_leave(lua_State *L);

	// mod_channel_send(name, message)
	static int l_mod_channel_send(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
