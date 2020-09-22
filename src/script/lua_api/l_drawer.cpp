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

#include <algorithm>
#include <cstdlib>
#include <string>
#include "client/fontengine.h"
#include "client/guiscalingfilter.h"
#include "l_drawer.h"
#include "l_internal.h"
#include "script/common/c_converter.h"
#include "settings.h"
#include "util/string.h"

#if USE_FREETYPE
#include "irrlicht_changes/CGUITTFont.h"
#endif

// get_window_size(self) -> {x = width, y = height}
int LuaScreenDrawer::l_get_window_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	core::dimension2d<u32> size = drawer->driver->getScreenSize();
	push_v2f(L, v2f(size.Width, size.Height));

	return 1;
}

// draw_rect(self, rect, color[, clip_rect])
int LuaScreenDrawer::l_draw_rect(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	core::recti rect = read_recti(L, 2);
	video::SColor color(0);
	read_color(L, 3, &color);

	if (lua_istable(L, 4)) {
		core::recti clip_rect = read_recti(L, 4);
		drawer->driver->draw2DRectangle(color, rect, &clip_rect);
	} else {
		drawer->driver->draw2DRectangle(color, rect);
	}

	return 0;
}

// draw_image(self, rect, texture[, clip_rect][, middle_rect][, from_rect][, recolor])
int LuaScreenDrawer::l_draw_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	core::recti rect = read_recti(L, 2);
	video::ITexture *texture = drawer->tsrc->getTexture(readParam<std::string>(L, 3));

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_recti(L, 4);
		clip_ptr = &clip_rect;
	}

	core::recti from_rect;
	core::dimension2d<u32> size = texture->getOriginalSize();
	if (lua_istable(L, 6)) {
		from_rect = read_recti(L, 6);

		// `-x` is interpreted as `w - x`. Also account for `read_recti` adding `w` to `x`.
		if (from_rect.LowerRightCorner.X < from_rect.UpperLeftCorner.X)
			from_rect.LowerRightCorner.X += (s32)size.Width - from_rect.UpperLeftCorner.X;
		if (from_rect.LowerRightCorner.Y < from_rect.UpperLeftCorner.Y)
			from_rect.LowerRightCorner.Y += (s32)size.Height - from_rect.UpperLeftCorner.Y;
	} else {
		from_rect = core::rect<s32>(core::position2d<s32>(0, 0),
			core::dimension2di(size.Width, size.Height));
	}

	video::SColor color(255, 255, 255, 255);
	if (!lua_isnil(L, 7))
		read_color(L, 7, &color);
	const video::SColor colors[] = {color, color, color, color};

	if (lua_istable(L, 5)) {
		core::recti middle_rect = read_recti(L, 5);

		if (middle_rect.LowerRightCorner.X < middle_rect.UpperLeftCorner.X)
			middle_rect.LowerRightCorner.X += (s32)size.Width - middle_rect.UpperLeftCorner.X;
		if (middle_rect.LowerRightCorner.Y < middle_rect.UpperLeftCorner.Y)
			middle_rect.LowerRightCorner.Y += (s32)size.Height - middle_rect.UpperLeftCorner.Y;

		draw2DImage9Slice(drawer->driver, texture, rect, from_rect, middle_rect,
			clip_ptr, colors);
	} else {
		draw2DImageFilterScaled(drawer->driver, texture, rect, from_rect,
			clip_ptr, colors, true);
	}

	return 0;
}

// get_image_size(self, texture) -> {x = width, y = height}
int LuaScreenDrawer::l_get_image_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	core::dimension2d<u32> size =
		drawer->tsrc->getTexture(readParam<std::string>(L, 2))->getOriginalSize();
	push_v2f(L, v2f(size.Width, size.Height));

	return 1;
}

// Create an IGUIFont. The font style table should be at the top of the stack.
gui::IGUIFont *LuaScreenDrawer::get_font(lua_State *L)
{
	FontSpec spec(FONT_SIZE_UNSPECIFIED, FM_Standard, false, false);

	lua_getfield(L, -1, "font");
	std::string font = readParam<std::string>(L, -1);
	if (font == "mono")
		spec.mode = FM_Mono;
	lua_pop(L, 1);

	lua_getfield(L, -1, "bold");
	if (lua_toboolean(L, -1))
		spec.bold = true;
	lua_pop(L, 1);

	lua_getfield(L, -1, "italic");
	if (lua_toboolean(L, -1))
		spec.italic = true;
	lua_pop(L, 1);

	lua_getfield(L, -1, "size");
	if (lua_isnumber(L, -1)) {
		spec.size = std::min(std::max((int)lua_tointeger(L, -1), 1), 999);
	} else {
		if (spec.mode == FM_Mono)
			spec.size = g_settings->getU16("mono_font_size");
		else
			spec.size = g_settings->getU16("font_size");
	}
	lua_pop(L, 1);

	return g_fontengine->getFont(spec);
}

// draw_text(self, pos, text[, clip_rect][, style])
// style = {color, font, size, bold, italic, underline, strike}
int LuaScreenDrawer::l_draw_text(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	core::recti rect;
	rect.UpperLeftCorner = read_v2s32(L, 2);

	std::wstring text = utf8_to_wide(
		unescape_enriched(readParam<std::string>(L, 3)));
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\0')
			text[i] = L' ';
	}

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_recti(L, 4);
		clip_ptr = &clip_rect;
	}

	gui::IGUIFont *font = get_font(L);

	lua_getfield(L, -1, "color");
	video::SColor color(0xFFFFFFFF);
	read_color(L, -1, &color);
	lua_pop(L, 1);

	lua_getfield(L, -1, "underline");
	bool underline = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "strike");
	bool strike = lua_toboolean(L, -1);
	lua_pop(L, 1);

	font->draw(text.c_str(), rect, color, false, false, clip_ptr);

	if (!underline && !strike)
		return 0;

	core::dimension2d<u32> size = font->getDimension(text.c_str());
	rect.LowerRightCorner = rect.UpperLeftCorner +
		core::vector2di(size.Width, size.Height);

	s32 baseline = 0;
#if USE_FREETYPE
	if (font->getType() == gui::EGFT_CUSTOM)
		baseline = size.Height - 1 - ((gui::CGUITTFont *)font)->getAscender() / 64;
#endif

	if (underline) {
		s32 line_pos = rect.LowerRightCorner.Y - (baseline >> 1);
		core::recti line_rect(
			rect.UpperLeftCorner.X,
			line_pos - (baseline >> 3) - 1,
			rect.LowerRightCorner.X,
			line_pos + (baseline >> 3)
		);

		drawer->driver->draw2DRectangle(color, line_rect, clip_ptr);
	}

	if (strike) {
		s32 line_pos = rect.UpperLeftCorner.Y + size.Height * 6 / 11;
		core::recti line_rect(
			rect.UpperLeftCorner.X,
			line_pos - (baseline >> 3) - 1,
			rect.LowerRightCorner.X,
			line_pos + (baseline >> 3)
		);

		drawer->driver->draw2DRectangle(color, line_rect, clip_ptr);
	}

	return 0;
}

// get_text_size(self, text[, style]) -> {x = width, y = height}
int LuaScreenDrawer::l_get_text_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	std::wstring text = utf8_to_wide(
		unescape_enriched(readParam<std::string>(L, 2)));
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\0')
			text[i] = L' ';
	}

	gui::IGUIFont *font = get_font(L);

	core::dimension2d<u32> size = font->getDimension(text.c_str());
	size.Height += font->getKerningHeight();
	push_v2f(L, v2f(size.Width, size.Height));

	return 1;
}

// get_font_size(self, font) -> size
int LuaScreenDrawer::l_get_font_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaScreenDrawer *drawer = checkobject(L, 1);
	if (drawer == nullptr)
		return 0;

	std::string font = readParam<std::string>(L, 2);

	// We'll assume that the fallback font has about the same size as the others
	// because there's no way to check if it will be used.
	if (font == "mono")
		lua_pushinteger(L, g_settings->getU16("mono_font_size"));
	else
		lua_pushinteger(L, g_settings->getU16("font_size"));

	return 1;
}

int LuaScreenDrawer::create_object(lua_State *L, video::IVideoDriver *driver,
	ISimpleTextureSource *tsrc)
{
	LuaScreenDrawer *drawer = new LuaScreenDrawer;

	drawer->driver = driver;
	drawer->tsrc = tsrc;

	*(void **)(lua_newuserdata(L, sizeof(void *))) = drawer;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

LuaScreenDrawer *LuaScreenDrawer::checkobject(lua_State *L, int narg)
{
	NO_MAP_LOCK_REQUIRED;

	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *drawer = luaL_checkudata(L, narg, className);
	if (drawer == nullptr)
		luaL_typerror(L, narg, className);
	return *(LuaScreenDrawer **)drawer;
}

int LuaScreenDrawer::gc_object(lua_State *L)
{
	LuaScreenDrawer *drawer = *(LuaScreenDrawer **)(lua_touserdata(L, 1));
	delete drawer;
	return 0;
}

void LuaScreenDrawer::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);
}

const char LuaScreenDrawer::className[] = "ScreenDrawer";
const luaL_Reg LuaScreenDrawer::methods[] =
{
	luamethod(LuaScreenDrawer, get_window_size),
	luamethod(LuaScreenDrawer, draw_rect),
	luamethod(LuaScreenDrawer, draw_image),
	luamethod(LuaScreenDrawer, get_image_size),
	luamethod(LuaScreenDrawer, draw_text),
	luamethod(LuaScreenDrawer, get_text_size),
	luamethod(LuaScreenDrawer, get_font_size),
	{ 0, 0 }
};
