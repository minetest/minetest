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

#include <unordered_map>
#include "client/client.h"
#include "client/tile.h"
#include "irrlichttypes_extrabloated.h"
#include "lua_api/l_base.h"

class LuaTexture;

class ModApiDrawer : public ModApiBase
{
private:
	friend class LuaTexture;

	bool in_callback = false;
	video::ITexture *current_target = nullptr;
	LuaTexture *normal_target = nullptr;

	scene::SMeshBuffer effects_mesh;

	static int l_get_text_size(lua_State *L);
	static int l_get_font_size(lua_State *L);

	static int l_get_driver_info(lua_State *L);

	static int l_set_render_target(lua_State *L);
	static int l_draw_render_target(lua_State *L);

public:
	ModApiDrawer();

	static void Initialize(lua_State *L, int top);

	// Functions for starting and ending times where it is allowed to draw to textures
	// or the screen. Only begin/end the scene if it has not already begun!
	static void start_callback(bool begin_scene);
	static void end_callback(bool end_scene);

	// Reset render textures and suchlike on shutdown
	static void reset();

	// Set the render target to the default render target. Returns true on failure
	static bool set_to_normal_target(bool clear = false, video::SColor = 0x0);

	// Create an IGUIFont from a font styling table.
	static gui::IGUIFont *get_font(lua_State *L, int index);
};

class LuaTexture : public ModApiBase
{
private:
	friend class ModApiDrawer;

	static const luaL_Reg methods[];

	video::ITexture *texture;
	bool is_ref;
	bool is_bound = false;
	bool is_target = false;
	bool is_dead = false;

	static int gc_object(lua_State *L);

	static bool set_as_target(LuaTexture *t, video::IVideoDriver *driver);

	static int l_is_ref(lua_State *L);
	static int l_get_original_size(lua_State *L);
	static int l_get_size(lua_State *L);

	static int l_fill(lua_State *L);
	static int l_draw_pixel(lua_State *L);
	static int l_draw_rect(lua_State *L);
	static int l_draw_texture(lua_State *L);
	static int l_draw_text(lua_State *L);
	static int l_draw_item(lua_State *L);
	static int l_draw_transformed_texture(lua_State *L);

	static int l_bind_ingame(lua_State *L);

	static int l_new(lua_State *L);
	static int l_copy(lua_State *L);
	static int l_ref(lua_State *L);

public:
	static int create_window(lua_State *L);

	// Read a LuaTexture from the Lua stack
	static LuaTexture *read_object(lua_State *L, int index, bool allow_window);
	// Read a LuaTexture that can be drawn to from the Lua stack
	static LuaTexture *read_writable_object(lua_State *L, int index, bool allow_window);
	// Read any type of texture from the Lua stack
	static video::ITexture *read_texture(lua_State *L, int index, bool allow_window);

	// Tries to destroy the texture. If it is still in use, it will not be destroyed
	// and should be destroyed by the thing using it instead.
	void try_destroy();

	static void Register(lua_State *L);
};
