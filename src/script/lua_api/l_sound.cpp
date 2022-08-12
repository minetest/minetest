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

#include "l_sound.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "gui/guiEngine.h"


int ModApiSound::l_sound_play(lua_State *L)
{
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	spec.loop = readParam<bool>(L, 2);

	s32 handle = getGuiEngine(L)->playSound(spec);

	lua_pushinteger(L, handle);

	return 1;
}

int ModApiSound::l_sound_stop(lua_State *L)
{
	u32 handle = luaL_checkinteger(L, 1);

	getGuiEngine(L)->stopSound(handle);

	return 1;
}

void ModApiSound::Initialize(lua_State *L, int top)
{
	API_FCT(sound_play);
	API_FCT(sound_stop);
}
