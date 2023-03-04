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

class ModChannel;

class ModApiChannels : public ModApiBase
{
private:
	// mod_channel_join(name)
	static int l_mod_channel_join(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};

class ModChannelRef : public ModApiBase
{
public:
	ModChannelRef(const std::string &modchannel);
	~ModChannelRef() = default;

	static void Register(lua_State *L);
	static void create(lua_State *L, const std::string &channel);

	// leave()
	static int l_leave(lua_State *L);

	// send(message)
	static int l_send_all(lua_State *L);

	// is_writeable()
	static int l_is_writeable(lua_State *L);

	static const char className[];

private:
	// garbage collector
	static int gc_object(lua_State *L);

	static ModChannel *getobject(lua_State *L, ModChannelRef *ref);

	std::string m_modchannel_name;

	static const luaL_Reg methods[];
};
