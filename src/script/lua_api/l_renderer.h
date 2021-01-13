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

class LuaTexture;

class ModApiRenderer : public ModApiBase
{
private:
	friend class LuaTexture;

	bool in_callback = false;
	video::ITexture *current_target = nullptr;

	static int l_get_text_size(lua_State *L);
	static int l_get_font_size(lua_State *L);

	static int l_get_driver_info(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);

	//! Sets the render target and associated information. A texture of `nullptr` sets
	//! the screen as the target. Should be used unless the render target will be reset
	//! immediately after use, in which case driver->setRenderTarget should be used.
	static bool set_render_target(video::IVideoDriver *driver, video::ITexture *texture,
			bool clear = false, video::SColor color = 0x0);

	//! Functions for starting and ending when it is allowed to draw to textures
	//! or the window.
	static void start_callback();
	static void end_callback();

	//! Create an IGUIFont from a FontSpec.
	static gui::IGUIFont *get_font(lua_State *L, int index);
};

class LuaTexture : public ModApiBase
{
private:
	friend class ModApiRenderer;

	static const char class_name[];
	static const luaL_Reg methods[];

	video::ITexture *texture;
	bool is_ref;
	bool is_target = false;

	//! Checks if the texture is writable and sets it as target if it is.
	static bool set_as_target(LuaTexture *t, video::IVideoDriver *driver,
			bool clear = false, video::SColor color = 0x0);

	static int gc_object(lua_State *L);

	static int l_is_readable(lua_State *L);
	static int l_is_writable(lua_State *L);
	static int l_get_size(lua_State *L);

	static int l_fill(lua_State *L);
	static int l_draw_pixel(lua_State *L);
	static int l_draw_rect(lua_State *L);
	static int l_draw_texture(lua_State *L);
	static int l_draw_text(lua_State *L);
	static int l_draw_item(lua_State *L);

	static int l_new(lua_State *L);
	static int l_copy(lua_State *L);
	static int l_ref(lua_State *L);

public:
	//! Create `minetest.window` and leave it at the top of the stack
	static int create_window(lua_State *L);

	//! Read a LuaTexture from the Lua stack
	static LuaTexture *read_object(lua_State *L, int index, bool allow_write_only);
	//! Read a LuaTexture that can be drawn to from the Lua stack
	static LuaTexture *read_writable_object(lua_State *L, int index, bool allow_write_only);
	//! Read any type of texture from the Lua stack
	static video::ITexture *read_texture(lua_State *L, int index, bool allow_write_only);

	//! Returns the dimensions of the texture/window
	core::vector2di getSize();

	static void Register(lua_State *L);
};
