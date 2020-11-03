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

#include <vector>
#include "client/client.h"
#include "client/tile.h"
#include "irrlichttypes_extrabloated.h"
#include "lua_api/l_base.h"

class ModApiDrawer : public ModApiBase
{
private:
	bool in_callback;
	std::vector<video::ITexture *> renderers;
	scene::SMeshBuffer effects_mesh;

	static int l_get_window_size(lua_State *L);

	static int l_draw_rect(lua_State *L);
	static int l_draw_texture(lua_State *L);
	static int l_get_texture_size(lua_State *L);

	static gui::IGUIFont *get_font(lua_State *L, int index);
	static int l_draw_text(lua_State *L);
	static int l_get_text_size(lua_State *L);
	static int l_get_font_size(lua_State *L);

	static int l_draw_item(lua_State *L);

	static int l_start_effect(lua_State *L);
	static int l_draw_effect(lua_State *L);
	static int l_effects_supported(lua_State *L);
	static int l_TEMP_TRANSFORM(lua_State *L);

public:
	ModApiDrawer();

	static void Initialize(lua_State *L, int top);

	static void start_callback();
	static void end_callback();
};
