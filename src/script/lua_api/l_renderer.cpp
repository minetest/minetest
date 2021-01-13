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
#include <string>
#include "client/fontengine.h"
#include "client/guiscalingfilter.h"
#include "client/hud.h"
#include "client/renderingengine.h"
#include "gettext.h"
#include "l_renderer.h"
#include "l_internal.h"
#include "log.h"
#include "script/common/c_converter.h"
#include "settings.h"
#include "util/string.h"
#if USE_FREETYPE
#include "irrlicht_changes/CGUITTFont.h"
#endif

ModApiRenderer s_renderer;

gui::IGUIFont *ModApiRenderer::get_font(lua_State *L, int index)
{
	FontSpec spec(FONT_SIZE_UNSPECIFIED, FM_Standard, false, false);

	if (is_yes(gettext("needs_fallback_font"))) {
		spec.mode = FM_Fallback;
	} else if (lua_istable(L, index)) {
		lua_getfield(L, index, "font");
		std::string font = readParam<std::string>(L, -1, "");
		if (font == "mono")
			spec.mode = FM_Mono;
		lua_pop(L, 1);
	}

	if (lua_istable(L, index)) {
		lua_getfield(L, index, "bold");
		if (lua_toboolean(L, -1))
			spec.bold = true;
		lua_pop(L, 1);

		lua_getfield(L, index, "italic");
		if (lua_toboolean(L, -1))
			spec.italic = true;
		lua_pop(L, 1);

		lua_getfield(L, index, "size");
		if (lua_isnumber(L, -1)) {
			spec.size = std::min(std::max((int)lua_tointeger(L, -1), 1), 999);
		} else {
			if (spec.mode == FM_Standard)
				spec.size = g_settings->getU16("font_size");
			else if (spec.mode == FM_Mono)
				spec.size = g_settings->getU16("mono_font_size");
			else
				spec.size = g_settings->getU16("fallback_font_size");
		}
		lua_pop(L, 1);
	}

	return g_fontengine->getFont(spec);
}

// get_text_size(text[, font]) -> size
int ModApiRenderer::l_get_text_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::wstring text = utf8_to_wide(
		unescape_enriched(readParam<std::string>(L, 1)));
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\n' || text[i] == L'\0')
			text[i] = L' ';
	}

	gui::IGUIFont *font = get_font(L, 2);

	core::dimension2du size = font->getDimension(text.c_str());
	size.Height += font->getKerningHeight();
	push_v2s32(L, v2s32(size.Width, size.Height));

	return 1;
}

// get_font_size(font) -> size
int ModApiRenderer::l_get_font_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (is_yes(gettext("needs_fallback_font"))) {
		lua_pushinteger(L, g_settings->getU16("fallback_font_size"));
	} else {
		if (lua_isstring(L, 1) && readParam<std::string>(L, 1) == "mono")
			lua_pushinteger(L, g_settings->getU16("mono_font_size"));
		else
			lua_pushinteger(L, g_settings->getU16("font_size"));
	}

	return 1;
}

// get_driver_info() -> features
int ModApiRenderer::l_get_driver_info(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	lua_createtable(L, 0, 2);

	video::E_DRIVER_TYPE driver_type = driver->getDriverType();
	lua_pushstring(L, RenderingEngine::getVideoDriverName(driver_type));
	lua_setfield(L, -2, "name");

	lua_pushstring(L, RenderingEngine::getVideoDriverFriendlyName(driver_type));
	lua_setfield(L, -2, "friendly_name");

	lua_pushboolean(L, driver->queryFeature(video::EVDF_RENDER_TO_TARGET));
	lua_setfield(L, -2, "render_textures");

	lua_pushboolean(L, driver->queryFeature(video::EVDF_TEXTURE_NPOT));
	lua_setfield(L, -2, "npot_textures");

	lua_pushboolean(L, driver->queryFeature(video::EVDF_TEXTURE_NSQUARE));
	lua_setfield(L, -2, "non_square_textures");

	lua_pushboolean(L, driver_type == video::EDT_OPENGL);
	lua_setfield(L, -2, "shaders");

	core::dimension2du max_size = driver->getMaxTextureSize();
	push_v2f(L, v2f(max_size.Width == 0 ? HUGE_VAL : max_size.Width,
			max_size.Height == 0 ? HUGE_VAL : max_size.Height));
	lua_setfield(L, -2, "max_texture_size");

	return 1;
}

void ModApiRenderer::Initialize(lua_State *L, int top)
{
	API_FCT(get_text_size);
	API_FCT(get_font_size);
	API_FCT(get_driver_info);

	LuaTexture::create_window(L);
	lua_setfield(L, top, "window");
};

bool ModApiRenderer::set_render_target(video::IVideoDriver *driver,
		video::ITexture *texture, bool clear, video::SColor color)
{
	if (texture != s_renderer.current_target) {
		if (texture == nullptr) {
			driver->setRenderTarget(video::ERT_FRAME_BUFFER, clear, clear, color);
			s_renderer.current_target = nullptr;

			// Irrlicht (annoyingly) doesn't use setViewPort in setRenderTarget, so
			// getViewPort returns an incorrect result, messing up the actual viewport
			// if viewports are changed temporarily (like in drawItemStack). So, we have
			// to change the viewport manually so getViewPort will reflect the actual
			// value.
			core::dimension2du size = driver->getCurrentRenderTargetSize();
			driver->setViewPort(core::recti(0, 0, size.Width, size.Height));
		} else {
			if (driver->queryFeature(video::EVDF_RENDER_TO_TARGET) &&
					driver->setRenderTarget(texture, clear, clear, color)) {
				s_renderer.current_target = texture;

				core::dimension2du size = driver->getCurrentRenderTargetSize();
				driver->setViewPort(core::recti(0, 0, size.Width, size.Height));
			} else {
				errorstream << "Unable to set texture as render target" << std::endl;
				return true;
			}
		}
	}
	return false;
}

void ModApiRenderer::start_callback()
{
	s_renderer.in_callback = true;
}

void ModApiRenderer::end_callback()
{
	set_render_target(RenderingEngine::get_video_driver(), nullptr);

	s_renderer.in_callback = false;
}

bool LuaTexture::set_as_target(LuaTexture *t, video::IVideoDriver *driver,
		bool clear, video::SColor color)
{
	if (!s_renderer.in_callback)
		throw LuaError("Attempt to draw to texture outside of a drawing callback");
	if (t->is_ref)
		throw LuaError("Attempt to write to a texture reference");

	return ModApiRenderer::set_render_target(driver, t->texture, clear, color);
}

// is_readable() -> bool
int LuaTexture::l_is_readable(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	lua_pushboolean(L, read_object(L, 1, true)->texture != nullptr);
	return 1;
}

// is_writable() -> bool
int LuaTexture::l_is_writable(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	lua_pushboolean(L, !read_object(L, 1, true)->is_ref);
	return 1;
}

// get_size() -> size
int LuaTexture::l_get_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = read_object(L, 1, true);

	core::vector2di size = t->getSize();
	push_v2s32(L, v2s32(size.X, size.Y));

	return 1;
}

// fill(self, color)
int LuaTexture::l_fill(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);

	// Read before `set_render_target` so it can be passed to it
	video::SColor color(0x0);
	read_color(L, 2, &color);

	// setRenderTarget is the only way to fill a texture with a pixel using color
	// replacement instead of adding. We change the target to the window before
	// setting it to the actual texture because Irrlicht ignores setRenderTarget
	// if the texture is already set as the target.
	if (ModApiRenderer::set_render_target(driver, nullptr) ||
			set_as_target(t, driver, true, color))
		return 0;

	// The window requires special treatment since it can have no alpha.
	if (t->texture == nullptr) {
		color.setAlpha(0xFF);
		driver->draw2DRectangle(color, core::recti(core::vector2di(0, 0), t->getSize()));
	}

	return 0;
}

// draw_pixel(self, pos, color[, clip_rect])
int LuaTexture::l_draw_pixel(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::vector2di pos = read_rel_pos(L, 2, t->getSize());

	video::SColor color(0x0);
	read_color(L, 3, &color);

	if (lua_istable(L, 4)) {
		core::recti clip_rect = read_rel_rect(L, 4, t->getSize());
		if (pos.X < clip_rect.UpperLeftCorner.X ||
				pos.Y < clip_rect.UpperLeftCorner.Y ||
				pos.X >= clip_rect.LowerRightCorner.X ||
				pos.Y >= clip_rect.LowerRightCorner.Y)
			return 0; // Do not draw since the pixel is clipped
	}

	driver->drawPixel(pos.X, pos.Y, color);

	return 0;
}

// draw_rect(self, rect, color[, clip_rect])
int LuaTexture::l_draw_rect(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect = read_rel_rect(L, 2, t->getSize());

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_rel_rect(L, 4, t->getSize());
		clip_ptr = &clip_rect;
	}

	// Draw with a gradient if it is an _array_ of four colors since a table with keys
	// might be a ColorSpec in table form
	if (lua_istable(L, 3) && lua_objlen(L, 3) == 4) {
		video::SColor colors[4] = {0x0, 0x0, 0x0, 0x0};
		for (size_t i = 0; i < 4; i++) {
			lua_rawgeti(L, 3, i + 1);
			read_color(L, -1, &colors[i]);
			lua_pop(L, 1);
		}
		driver->draw2DRectangle(rect, colors[0], colors[1], colors[2], colors[3], clip_ptr);
	} else {
		video::SColor color(0x0);
		read_color(L, 3, &color);
		driver->draw2DRectangle(color, rect, clip_ptr);
	}

	return 0;
}

// draw_texture(self, rect, texture[, clip_rect][, source_rect][, middle_rect][, recolor]
//     [, no_alpha])
int LuaTexture::l_draw_texture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect = read_rel_rect(L, 2, t->getSize());

	video::ITexture *texture = read_texture(L, 3, false);
	if (texture == t->texture)
		throw LuaError("Attempt to draw texture to itself");

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_rel_rect(L, 4, t->getSize());
		clip_ptr = &clip_rect;
	}

	core::recti from_rect;
	core::dimension2du usize = texture->getOriginalSize();
	core::vector2di size(usize.Width, usize.Height);
	if (lua_istable(L, 5))
		from_rect = read_rel_rect(L, 5, size);
	else
		from_rect = core::recti(core::vector2di(0, 0), size);

	video::SColor color(0xFFFFFFFF);
	if (!lua_isnil(L, 7))
		read_color(L, 7, &color);
	const video::SColor colors[] = {color, color, color, color};

	bool use_alpha = !lua_toboolean(L, 8);

	if (lua_istable(L, 6)) {
		draw2DImage9Slice(driver, texture, rect, from_rect,
				read_rel_rect(L, 6, from_rect.getSize()), clip_ptr, colors, use_alpha);
	} else {
		driver->draw2DImage(texture, rect, from_rect, clip_ptr, colors, use_alpha);
	}

	return 0;
}

// draw_text(self, pos, text[, clip_rect][, font])
int LuaTexture::l_draw_text(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect;
	rect.UpperLeftCorner = read_rel_pos(L, 2, t->getSize());

	std::wstring text = utf8_to_wide(
		unescape_enriched(readParam<std::string>(L, 3)));
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\n' || text[i] == L'\0')
			text[i] = L' ';
	}

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_rel_rect(L, 4, t->getSize());
		clip_ptr = &clip_rect;
	}

	gui::IGUIFont *font = ModApiRenderer::get_font(L, 5);

	video::SColor color(0xFFFFFFFF);
	bool underline = false;
	bool strike = false;
	if (lua_istable(L, 5)) {
		lua_getfield(L, 5, "color");
		read_color(L, -1, &color);
		lua_pop(L, 1);

		lua_getfield(L, 5, "underline");
		underline = lua_toboolean(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 5, "strike");
		strike = lua_toboolean(L, -1);
		lua_pop(L, 1);
	}

	font->draw(text.c_str(), rect, color, false, false, clip_ptr);

	if (!underline && !strike)
		return 0;

	core::dimension2du size = font->getDimension(text.c_str());
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

		driver->draw2DRectangle(color, line_rect, clip_ptr);
	}

	if (strike) {
		s32 line_pos = rect.UpperLeftCorner.Y + size.Height * 6 / 11;
		core::recti line_rect(
			rect.UpperLeftCorner.X,
			line_pos - (baseline >> 3) - 1,
			rect.LowerRightCorner.X,
			line_pos + (baseline >> 3)
		);

		driver->draw2DRectangle(color, line_rect, clip_ptr);
	}

	return 0;
}

// draw_item(self, rect, item[, clip_rect][, angle])
int LuaTexture::l_draw_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect = read_rel_rect(L, 2, t->getSize());

	Client *client = getClient(L);
	ItemStack item;
	item.deSerialize(readParam<std::string>(L, 3), client->idef());
	item.count = 1; // No item count; drawn manually to be configurable

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_rel_rect(L, 4, t->getSize());
		clip_ptr = &clip_rect;
	}

	v3s16 angle;
	if (lua_istable(L, 5))
		angle = read_v3s16(L, 5);

	drawItemStack(RenderingEngine::get_video_driver(), g_fontengine->getFont(), item,
		rect, clip_ptr, client, IT_ROT_NONE, angle, v3s16(0, 0, 0));

	return 0;
}

// new(size) -> texture
int LuaTexture::l_new(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_renderer.in_callback)
		throw LuaError("Attempt to create a texture outside of a drawing callback");

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!driver->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		lua_pushnil(L);
		return 1;
	}

	LuaTexture *t = new LuaTexture;
	t->is_ref = false;

	core::vector2di size = read_v2s32(L, 1);

	t->texture = driver->addRenderTargetTexture(
		core::dimension2du(size.X, size.Y), "", video::ECF_A8R8G8B8);
	if (t->texture == nullptr) {
		errorstream << "Unable to create custom texture" << std::endl;
		delete t;
		lua_pushnil(L);
		return 1;
	}

	// Clear texture
	driver->setRenderTarget(t->texture, true, true);
	driver->setRenderTarget(nullptr, false, false);

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
	return 1;
}

// copy(texture) -> texture
int LuaTexture::l_copy(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_renderer.in_callback)
		throw LuaError("Attempt to create a texture outside of a drawing callback");

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!driver->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		lua_pushnil(L);
		return 1;
	}

	LuaTexture *t = new LuaTexture;
	t->is_ref = false;

	video::ITexture *to_copy = read_texture(L, 1, false);

	t->texture = driver->addRenderTargetTexture(
		to_copy->getOriginalSize(), "", video::ECF_A8R8G8B8);
	if (t->texture == nullptr) {
		errorstream << "Unable to create render target texture" << std::endl;
		delete t;
		lua_pushnil(L);
		return 1;
	}
	if (!driver->setRenderTarget(t->texture, true, true)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		delete t;
		lua_pushnil(L);
		return 1;
	}
	driver->draw2DImage(to_copy, core::vector2di(0, 0));
	driver->setRenderTarget(nullptr, false, false);

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
	return 1;
}

// ref(texture) -> texture
int LuaTexture::l_ref(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!driver->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		lua_pushnil(L);
		return 1;
	}

	LuaTexture *t = new LuaTexture;
	t->is_ref = true;

	t->texture = read_texture(L, 1, true);

	if (t->texture != nullptr) {
		// The texture being referenced might become invalid if the actual texture is
		// garbage collected and the texture removed with driver->removeTexture. Grabbing
		// it will keep it valid.
		t->texture->grab();
	}

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
	return 1;
}

int LuaTexture::create_window(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = new LuaTexture;
	t->texture = nullptr;
	t->is_ref = false;

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
	return 1;
}

LuaTexture *LuaTexture::read_object(lua_State *L, int index, bool allow_write_only)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = *(LuaTexture **)luaL_checkudata(L, index, class_name);
	if (!allow_write_only && t->texture == nullptr)
		throw LuaError("Attempt to read from a write-only texture");

	return t;
}

LuaTexture *LuaTexture::read_writable_object(lua_State *L, int index, bool allow_write_only)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = *(LuaTexture **)luaL_checkudata(L, index, class_name);
	if (!allow_write_only && t->texture == nullptr)
		throw LuaError("Attempt to read from a write-only texture");
	if (t->is_ref)
		throw LuaError("Attempt to write to a texture reference");

	return t;
}

video::ITexture *LuaTexture::read_texture(lua_State *L, int index, bool allow_write_only)
{
	NO_MAP_LOCK_REQUIRED;

	if (lua_isstring(L, index)) {
		return getClient(L)->tsrc()->getTexture(readParam<std::string>(L, index));
	} else {
		video::ITexture *texture = (*(LuaTexture **)luaL_checkudata(
			L, index, class_name))->texture;
		if (!allow_write_only && texture == nullptr)
			throw LuaError("Attempt to read from a write-only texture");
		return texture;
	}
}

core::vector2di LuaTexture::getSize()
{
	core::dimension2du size;
	if (this->texture == nullptr)
		size = RenderingEngine::get_video_driver()->getScreenSize();
	else
		size = this->texture->getOriginalSize();

	return core::vector2di(size.Width, size.Height);
}

int LuaTexture::gc_object(lua_State *L)
{
	LuaTexture *t = *(LuaTexture **)lua_touserdata(L, 1);
	if (t->texture != nullptr) {
		// Only drop reference textures, and only remove non-reference textures
		if (t->is_ref) {
			t->texture->drop();
		} else {
			video::IVideoDriver *driver = RenderingEngine::get_video_driver();
			if (t->is_target)
				ModApiRenderer::set_render_target(driver, nullptr);
			driver->removeTexture(t->texture);
		}
	}
	delete t;
	return 0;
}

void LuaTexture::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, class_name);
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

	lua_createtable(L, 0, 4);
	int top = lua_gettop(L);
	API_FCT(new);
	API_FCT(copy);
	API_FCT(ref);
	lua_setglobal(L, class_name);
}

const char LuaTexture::class_name[] = "Texture";
const luaL_Reg LuaTexture::methods[] =
{
	luamethod(LuaTexture, is_readable),
	luamethod(LuaTexture, is_writable),
	luamethod(LuaTexture, get_size),
	luamethod(LuaTexture, fill),
	luamethod(LuaTexture, draw_pixel),
	luamethod(LuaTexture, draw_rect),
	luamethod(LuaTexture, draw_texture),
	luamethod(LuaTexture, draw_text),
	luamethod(LuaTexture, draw_item),
	{0, 0}
};
