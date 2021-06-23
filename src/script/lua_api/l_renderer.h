/*
Minetest
Copyright (C) 2021 Vincent Robinson (v-rob) <robinsonvincent89@gmail.com>

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
#include "client/fontengine.h"
#include "client/tile.h"
#include "irrlichttypes_extrabloated.h"
#include "lua_api/l_base.h"

class LuaTexture;

class ModApiRenderer : public ModApiBase
{
private:
	friend class LuaTexture;

	bool in_callback = false;
	video::ITexture *current_target = nullptr;

	static int l_get_driver_info(lua_State *L);
	static int l_get_optimal_texture_size(lua_State *L);

public:
	//! Sets the render target and associated information. A texture of `nullptr` sets
	//! the screen as the target. Should be used unless the render target will be set and
	//! reset in the same function, in which case setRenderTarget is better.
	static bool set_render_target(video::IVideoDriver *driver, video::ITexture *texture,
			bool clear = false, video::SColor color = 0x0);

	//! Functions for starting and ending when it is allowed to draw to textures
	//! or the window.
	static void start_callback();
	static void end_callback();

	static void Initialize(lua_State *L, int top);
};

class LuaTexture : public ModApiBase
{
private:
	static const char class_name[];
	static const luaL_Reg methods[];

	video::ITexture *texture;
	bool is_ref;
	bool is_target = false;

	//! Checks if the texture is writable and sets it as target if it is.
	static bool set_as_target(LuaTexture *t, video::IVideoDriver *driver,
			bool clear = false, video::SColor color = 0x0);

	static int l_is_readable(lua_State *L);
	static int l_is_writable(lua_State *L);
	static int l_get_size(lua_State *L);

	static int l_fill(lua_State *L);
	static int l_draw_pixel(lua_State *L);
	static int l_draw_rect(lua_State *L);
	static int l_draw_texture(lua_State *L);
	static int l_draw_text(lua_State *L);

	static int l_new(lua_State *L);
	static int l_copy(lua_State *L);
	static int l_ref(lua_State *L);

	static int gc_object(lua_State *L);

public:
	//! Create `minetest.window` and leave it at the top of the stack
	static int create_window(lua_State *L);

	//! Read a LuaTexture from the Lua stack
	static LuaTexture *read_object(lua_State *L, int index, bool allow_write_only);
	//! Read a LuaTexture that can be drawn to from the Lua stack
	static LuaTexture *read_writable_object(lua_State *L, int index, bool allow_write_only);
	//! Read an in-game texture string from the Lua stack
	static video::ITexture *read_in_game_texture(lua_State *L, int index);
	//! Read any type of texture from the Lua stack
	static video::ITexture *read_texture(lua_State *L, int index, bool allow_write_only);

	//! Returns the dimensions of this texture
	core::vector2di getSize();
	//! Returns the stored texture itself
	video::ITexture *getTexture() { return texture; };

	static void Register(lua_State *L);
};

class LuaFont : public ModApiBase
{
private:
	static const char class_name[];
	static const luaL_Reg methods[];

	gui::IGUIFont *font;

	int size;
	FontMode face;
	bool bold;
	bool italic;

	static int l_get_metrics(lua_State *L);
	static int l_get_text_width(lua_State *L);
	static int l_to_table(lua_State *L);

	static int l_create(lua_State *L);

	static int gc_object(lua_State *L);

public:
	//! Read a Font from the Lua stack
	static LuaFont *read_object(lua_State *L, int index);
	//! Read a font from the Lua stack, returning the default font if nil.
	static gui::IGUIFont *read_font(lua_State *L, int index);

	//! Returns the stored font itself
	gui::IGUIFont *getFont() { return font; };

	static void Register(lua_State *L);
};
