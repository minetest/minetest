/*
Minetest
Copyright (C) 2023 DS
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

#pragma once

#include "lua_api/l_base.h"
#include "util/basic_macros.h"

using sound_handle_t = int;

class ModApiClientSound : public ModApiBase
{
private:
	// sound_play(spec, parameters)
	static int l_sound_play(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};

class ClientSoundHandle final : public ModApiBase
{
private:
	sound_handle_t m_handle;

	static const char className[];
	static const luaL_Reg methods[];

	ClientSoundHandle(sound_handle_t handle) : m_handle(handle) {}

	DISABLE_CLASS_COPY(ClientSoundHandle)

	static ClientSoundHandle *checkobject(lua_State *L, int narg);

	static int gc_object(lua_State *L);

	// :stop()
	static int l_stop(lua_State *L);

	// :fade(step, gain)
	static int l_fade(lua_State *L);

public:
	~ClientSoundHandle() = default;

	static void create(lua_State *L, sound_handle_t handle);
	static void Register(lua_State *L);
};
