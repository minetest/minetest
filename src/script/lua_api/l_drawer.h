/*
Minetest
Copyright (C) 2020 Vincent Robinson (v-rob) <robinsonvincent89@gmail.com>

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

#include "client/client.h"
#include "client/tile.h"
#include "irrlichttypes_extrabloated.h"
#include "lua_api/l_base.h"

//! Lua interface for drawing to and getting attributes about the screen
class LuaScreenDrawer : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	ISimpleTextureSource *tsrc;
	Client *client;

	static int gc_object(lua_State *L);

	static int l_get_window_size(lua_State *L);

	static int l_draw_rect(lua_State *L);
	static int l_draw_image(lua_State *L);
	static int l_get_image_size(lua_State *L);

	static gui::IGUIFont *get_font(lua_State *L);
	static int l_draw_text(lua_State *L);
	static int l_get_text_size(lua_State *L);
	static int l_get_font_size(lua_State *L);

	static int l_draw_item(lua_State *L);

public:
	//! Creates the object and leaves it at the top of the stack
	static int create_object(lua_State *L, ISimpleTextureSource *tsrc,
		Client *client);

	static LuaScreenDrawer *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};
