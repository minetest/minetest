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
#include "client/renderingengine.h"
#include "gettext.h"
#include "hud.h"
#include "l_drawer.h"
#include "l_internal.h"
#include "log.h"
#include "script/common/c_converter.h"
#include "settings.h"
#include "util/string.h"
#if USE_FREETYPE
#include "irrlicht_changes/CGUITTFont.h"
#endif

ModApiDrawer s_drawer;

gui::IGUIFont *ModApiDrawer::get_font(lua_State *L, int index)
{
	FontSpec spec(FONT_SIZE_UNSPECIFIED, FM_Standard, false, false);

	if (is_yes(gettext("needs_fallback_font"))) {
		spec.mode = FM_Fallback;
	} else {
		lua_getfield(L, index, "font");
		std::string font = readParam<std::string>(L, -1, "");
		if (font == "mono")
			spec.mode = FM_Mono;
		lua_pop(L, 1);
	}

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

	return g_fontengine->getFont(spec);
}

bool ModApiDrawer::set_to_normal_target(bool clear, video::SColor color)
{
	s_drawer.current_target = nullptr;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (s_drawer.normal_target == nullptr) {
		driver->setRenderTarget(video::ERT_FRAME_BUFFER, clear, clear, color);
	} else if (!driver->queryFeature(video::EVDF_RENDER_TO_TARGET) ||
			!driver->setRenderTarget(s_drawer.normal_target->texture, clear, clear, color)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		return true;
	}

	return false;
}

void ModApiDrawer::reset()
{
	RenderingEngine::get_video_driver()->setRenderTarget(
		video::ERT_FRAME_BUFFER, false, false);
	s_drawer.current_target = nullptr;
	s_drawer.normal_target = nullptr;
}

// get_text_size(text[, style]) -> size
int ModApiDrawer::l_get_text_size(lua_State *L)
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
int ModApiDrawer::l_get_font_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (is_yes(gettext("needs_fallback_font"))) {
		lua_pushinteger(L, g_settings->getU16("fallback_font_size"));
	} else {
		std::string font = readParam<std::string>(L, 1);
		if (font == "mono")
			lua_pushinteger(L, g_settings->getU16("mono_font_size"));
		else
			lua_pushinteger(L, g_settings->getU16("font_size"));
	}

	return 1;
}

// get_driver_info() -> features
int ModApiDrawer::l_get_driver_info(lua_State *L)
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
	push_v2s32(L, v2s32(max_size.Width, max_size.Height));
	lua_setfield(L, -2, "max_texture_size");

	return 1;
}

// set_render_target(texture)
int ModApiDrawer::l_set_render_target(lua_State *L)
{
	if (s_drawer.normal_target != nullptr) {
		// First destroy old texture
		s_drawer.normal_target->is_target = false;
		s_drawer.normal_target->try_destroy();
	}

	LuaTexture *t = LuaTexture::read_writable_object(L, 1, true);
	if (t->texture == nullptr) {
		s_drawer.normal_target = nullptr;
	} else {
		s_drawer.normal_target = t;
		s_drawer.normal_target->is_target = true;
	}

	ModApiDrawer::set_to_normal_target();

	return 0;
}

// draw_render_target(texture)
int ModApiDrawer::l_draw_render_target(lua_State *L)
{
	if (s_drawer.normal_target == nullptr)
		return 0;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	driver->setRenderTarget(video::ERT_FRAME_BUFFER, false, false);
	driver->draw2DImage(s_drawer.normal_target->texture, core::vector2di(0, 0));
	ModApiDrawer::set_to_normal_target();

	return 0;
}

ModApiDrawer::ModApiDrawer()
{
	// Prepare effects mesh for transformations
	effects_mesh.Vertices.set_used(4);
	effects_mesh.Indices.set_used(6);

	video::SColor white(255, 255, 255, 255);
	v3f normal(0.f, 0.f, 1.f);

	effects_mesh.Vertices[0] =
		video::S3DVertex(v3f(-1.f, -1.f, 0.f), normal, white, v2f(0.f, 1.f));
	effects_mesh.Vertices[1] =
		video::S3DVertex(v3f(-1.f,  1.f, 0.f), normal, white, v2f(0.f, 0.f));
	effects_mesh.Vertices[2] =
		video::S3DVertex(v3f( 1.f,  1.f, 0.f), normal, white, v2f(1.f, 0.f));
	effects_mesh.Vertices[3] =
		video::S3DVertex(v3f( 1.f, -1.f, 0.f), normal, white, v2f(1.f, 1.f));

	effects_mesh.Indices[0] = 0;
	effects_mesh.Indices[1] = 1;
	effects_mesh.Indices[2] = 2;
	effects_mesh.Indices[3] = 2;
	effects_mesh.Indices[4] = 3;
	effects_mesh.Indices[5] = 0;

	video::SMaterial &material = effects_mesh.getMaterial();
	material.Lighting = false;
	material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
}

void ModApiDrawer::Initialize(lua_State *L, int top)
{
	API_FCT(get_text_size);
	API_FCT(get_font_size);
	API_FCT(get_driver_info);
	API_FCT(set_render_target);
	API_FCT(draw_render_target);

	LuaTexture::create_window(L);
	lua_setfield(L, top, "window");
};

void ModApiDrawer::start_callback(bool begin_scene)
{
	if (begin_scene)
		RenderingEngine::get_video_driver()->beginScene(false, false);

	s_drawer.in_callback = true;
}

void ModApiDrawer::end_callback(bool end_scene)
{
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (end_scene)
		driver->endScene();
	set_to_normal_target();

	s_drawer.in_callback = false;
}

bool LuaTexture::set_as_target(LuaTexture *t, video::IVideoDriver *driver)
{
	if (!s_drawer.in_callback)
		return true;

	if (t->is_ref)
		throw LuaError("Attempt to write to texture reference");

	if (t->texture != s_drawer.current_target) {
		if (t->texture == nullptr) {
			ModApiDrawer::set_to_normal_target();
		} else {
			if (driver->queryFeature(video::EVDF_RENDER_TO_TARGET) &&
					driver->setRenderTarget(t->texture, false, false)) {
				s_drawer.current_target = t->texture;
			} else {
				errorstream << "Unable to set texture as render target" << std::endl;
				return true;
			}
		}
	}
	return false;
}

// is_ref() -> bool
int LuaTexture::l_is_ref(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	lua_pushboolean(L, read_object(L, 1, true)->is_ref);
	return 1;
}

// get_original_size() -> size
int LuaTexture::l_get_original_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = read_object(L, 1, true);

	core::dimension2du size;
	if (t->texture == nullptr)
		size = RenderingEngine::get_video_driver()->getScreenSize();
	else
		size = t->texture->getOriginalSize();
	push_v2s32(L, v2s32(size.Width, size.Height));

	return 1;
}

// get_size() -> size
int LuaTexture::l_get_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = read_object(L, 1, true);

	core::dimension2du size;
	if (t->texture == nullptr)
		size = RenderingEngine::get_video_driver()->getScreenSize();
	else
		size = t->texture->getSize();
	push_v2s32(L, v2s32(size.Width, size.Height));

	return 1;
}

// fill(self, color)
int LuaTexture::l_fill(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	// Do not use set_as_target as it will be done manually
	if (!s_drawer.in_callback)
		return 0;

	if (t->is_ref)
		throw LuaError("Attempt to write to texture reference");

	video::SColor color(0x0);
	read_color(L, 2, &color);

	// There seems to be no other certain way to fill a texture with a certain color
	// than by using driver->setRenderTexture with clearBackBuffer = true.
	if (t->texture == nullptr) {
		if (s_drawer.normal_target == nullptr)
			color.setAlpha(0xFF); // Window can't have transparency
		ModApiDrawer::set_to_normal_target(true, color);
	} else {
		if (driver->queryFeature(video::EVDF_RENDER_TO_TARGET) &&
				driver->setRenderTarget(t->texture, true, true, color)) {
			s_drawer.current_target = t->texture;
		} else {
			errorstream << "Unable to set texture as render target" << std::endl;
			return true;
		}
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

	core::vector2di pos = read_v2s32(L, 2);

	video::SColor color(0x0);
	read_color(L, 3, &color);

	if (lua_istable(L, 4) && !read_recti(L, 4).isPointInside(pos))
		return 0;

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

	core::recti rect = read_recti(L, 2);

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_recti(L, 4);
		clip_ptr = &clip_rect;
	}

	// Draw with a gradient if it is an _array_ of four colors as a table with keys
	// might be a ColorSpec in table form
	if (lua_istable(L, 3) && lua_objlen(L, 3) == 4) {
		video::SColor colors[4] = {0x0, 0x0, 0x0, 0x0};
		for (size_t i = 0; i < 4; i++) {
			lua_rawgeti(L, 3, i + 1);
			read_color(L, -1, &colors[i]);
			lua_pop(L, 1);
		}
		driver->draw2DRectangle(rect, colors[0], colors[1], colors[3], colors[2],
			clip_ptr);
	} else {
		video::SColor color(0x0);
		read_color(L, 3, &color);
		driver->draw2DRectangle(color, rect, clip_ptr);
	}

	return 0;
}

// draw_texture(self, rect, texture[, clip_rect][, middle_rect][, from_rect][, recolor]
//     [, use_alpha])
int LuaTexture::l_draw_texture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect = read_recti(L, 2);
	video::ITexture *texture = LuaTexture::read_texture(L, 3, false);

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_recti(L, 4);
		clip_ptr = &clip_rect;
	}

	core::recti from_rect;
	core::dimension2du size = texture->getOriginalSize();
	if (lua_istable(L, 6)) {
		from_rect = read_recti(L, 6);

		// `-x` is interpreted as `w - x`.
		if (from_rect.LowerRightCorner.X < from_rect.UpperLeftCorner.X)
			from_rect.LowerRightCorner.X += (s32)size.Width;
		if (from_rect.LowerRightCorner.Y < from_rect.UpperLeftCorner.Y)
			from_rect.LowerRightCorner.Y += (s32)size.Height;
	} else {
		from_rect = core::rect<s32>(core::position2d<s32>(0, 0),
			core::dimension2di(size.Width, size.Height));
	}

	video::SColor color(0xFFFFFFFF);
	if (!lua_isnil(L, 7))
		read_color(L, 7, &color);
	const video::SColor colors[] = {color, color, color, color};

	bool use_alpha = true;
	if (lua_type(L, 8) == LUA_TBOOLEAN)
		use_alpha = lua_toboolean(L, 8);

	if (lua_istable(L, 5)) {
		core::recti middle_rect = read_recti(L, 5);

		if (middle_rect.LowerRightCorner.X < middle_rect.UpperLeftCorner.X)
			middle_rect.LowerRightCorner.X += from_rect.getWidth();
		if (middle_rect.LowerRightCorner.Y < middle_rect.UpperLeftCorner.Y)
			middle_rect.LowerRightCorner.Y += from_rect.getHeight();

		draw2DImage9Slice(driver, texture, rect, from_rect, middle_rect,
			clip_ptr, colors, use_alpha);
	} else {
		draw2DImageFilterScaled(driver, texture, rect, from_rect,
			clip_ptr, colors, use_alpha);
	}

	return 0;
}

// draw_text(self, pos, text[, clip_rect][, style])
// style = {color, font, size, bold, italic, underline, strike}
int LuaTexture::l_draw_text(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect;
	rect.UpperLeftCorner = read_v2s32(L, 2);

	std::wstring text = utf8_to_wide(
		unescape_enriched(readParam<std::string>(L, 3)));
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\n' || text[i] == L'\0')
			text[i] = L' ';
	}

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_recti(L, 4);
		clip_ptr = &clip_rect;
	}

	gui::IGUIFont *font = ModApiDrawer::get_font(L, 5);

	lua_getfield(L, 5, "color");
	video::SColor color(0xFFFFFFFF);
	read_color(L, -1, &color);
	lua_pop(L, 1);

	lua_getfield(L, 5, "underline");
	bool underline = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 5, "strike");
	bool strike = lua_toboolean(L, -1);
	lua_pop(L, 1);

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

	core::recti rect = read_recti(L, 2);

	Client *client = getClient(L);
	IItemDefManager *idef = client->idef();
	ItemStack item;
	item.deSerialize(readParam<std::string>(L, 3), idef);

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 4)) {
		clip_rect = read_recti(L, 4);
		clip_ptr = &clip_rect;
	}

	v3s16 angle;
	if (lua_istable(L, 5))
		angle = read_v3s16(L, 5);

	drawItemStack(RenderingEngine::get_video_driver(), g_fontengine->getFont(), item,
		rect, clip_ptr, client, IT_ROT_NONE, angle, v3s16());

	return 0;
}

// draw_transformed_texture(self, rect, texture, transform)
int LuaTexture::l_draw_transformed_texture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	LuaTexture *t = read_writable_object(L, 1, true);
	if (set_as_target(t, driver))
		return 0;

	core::recti rect = read_recti(L, 2);
	video::ITexture *texture = LuaTexture::read_texture(L, 3, false);

	f32 m_data[16];
	for (size_t i = 0; i < 16; i++) {
		lua_rawgeti(L, 4, i + 1);
		m_data[i] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	core::matrix4 transform;
	transform.setM(m_data);

	core::rect<s32> old_viewport = driver->getViewPort();
	core::matrix4 old_proj_mat = driver->getTransform(video::ETS_PROJECTION);
	core::matrix4 old_view_mat = driver->getTransform(video::ETS_VIEW);

	driver->setViewPort(rect);
	driver->setTransform(video::ETS_PROJECTION, core::IdentityMatrix);
	driver->setTransform(video::ETS_VIEW, core::IdentityMatrix);
	driver->setTransform(video::ETS_WORLD, transform);

	video::SMaterial &material = s_drawer.effects_mesh.getMaterial();
	material.TextureLayer[0].Texture = texture;
	driver->setMaterial(material);
	driver->drawMeshBuffer(&s_drawer.effects_mesh);

	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	driver->setTransform(video::ETS_VIEW, old_view_mat);
	driver->setTransform(video::ETS_PROJECTION, old_proj_mat);

	driver->setViewPort(old_viewport);

	return 0;
}

// bind_ingame(self, name) -> success
int LuaTexture::l_bind_ingame(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = read_object(L, 1, false);

	std::string name = readParam<std::string>(L, 2);

	const char *ends[] = {".ct", nullptr};
	if (removeStringEnd(name, ends).empty() ||
			!string_allowed(name, TEXTURENAME_ALLOWED_CHARS)) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (getClient(L)->getWritableTextureSource()->insertRawTexture(name, t->texture)) {
		lua_pushboolean(L, false);
		return 1;
	}

	t->is_bound = true;
	lua_pushboolean(L, true);

	return 1;
}

// new(size) -> texture
int LuaTexture::l_new(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!driver->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		lua_pushnil(L);
		return 1;
	}

	LuaTexture *t = new LuaTexture;
	t->is_ref = false;

	core::vector2di size = read_v2s32(L, 1);

	/* if (size.Width == 0 || size.Height == 0 ||
			((!driver->queryFeature(video::EVDF_TEXTURE_NPOT) ||
			!driver->queryFeature(EVDF_TEXTURE_NSQUARE)) &&
			(size.Width != size.Height || (size.Width & (size.Width - 1)) == 0)))
		throw LuaError("Invalid texture size"); */

	t->texture = driver->addRenderTargetTexture(
		core::dimension2du(size.X, size.Y), "", video::ECF_A8R8G8B8);
	if (t->texture == nullptr) {
		errorstream << "Unable to create custom texture" << std::endl;
		delete t;
		lua_pushnil(L);
		return 1;
	}
	driver->setRenderTarget(t->texture, true, true);
	driver->setRenderTarget(nullptr, false, false);

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, "Texture");
	lua_setmetatable(L, -2);
	return 1;
}

// copy(texture) -> texture
int LuaTexture::l_copy(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!driver->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
		errorstream << "Unable to set texture as render target" << std::endl;
		lua_pushnil(L);
		return 1;
	}

	LuaTexture *t = new LuaTexture;
	t->is_ref = false;

	video::ITexture *to_copy = LuaTexture::read_texture(L, 1, true);

	bool needs_removal = false;
	if (to_copy == nullptr) {
		// The window needs special treatment. Since it is not a real texture, a
		// texture must be created from a screenshot. Has caveats, but it's better
		// than nothing.
		video::IImage *temp = driver->createScreenShot(video::ECF_A8R8G8B8);
		to_copy = driver->addTexture("", temp);
		if (to_copy == nullptr) {
			errorstream << "Unable to create texture" << std::endl;
			delete t;
			lua_pushnil(L);
			return 1;
		}
		temp->drop();
		needs_removal = true;
	}

	// The seemingly most reliable way to copy a texture is drawing with a
	// render texture. Cross normal/render texture copying with lock() could
	// potentially have incompatibilities.
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

	if (needs_removal)
		driver->removeTexture(to_copy);

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, "Texture");
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

	t->texture = LuaTexture::read_texture(L, 1, true);

	if (t->texture != nullptr) {
		// The texture being referenced might become invalid if the actual texture is
		// garbage collected and the texture removed with driver->removeTexture. Grabbing
		// it will keep it valid.
		t->texture->grab();
	}

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, "Texture");
	lua_setmetatable(L, -2);
	return 1;
}

int LuaTexture::create_window(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = new LuaTexture;
	t->texture = nullptr;
	t->is_ref = false;
	t->is_bound = false;

	*(void **)lua_newuserdata(L, sizeof(void *)) = t;
	luaL_getmetatable(L, "Texture");
	lua_setmetatable(L, -2);
	return 1;
}

LuaTexture *LuaTexture::read_object(lua_State *L, int index, bool allow_window)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = *(LuaTexture **)luaL_checkudata(L, index, "Texture");
	if (!allow_window && t->texture == nullptr)
		throw LuaError("Invalid use of minetest.window as normal texture");

	return t;
}

LuaTexture *LuaTexture::read_writable_object(lua_State *L, int index, bool allow_window)
{
	NO_MAP_LOCK_REQUIRED;

	LuaTexture *t = *(LuaTexture **)luaL_checkudata(L, index, "Texture");
	if (!allow_window && t->texture == nullptr)
		throw LuaError("Invalid use of minetest.window as normal texture");
	if (t->is_ref)
		throw LuaError("Invalid attempt to write to a texture reference");

	return t;
}

video::ITexture *LuaTexture::read_texture(lua_State *L, int index, bool allow_window)
{
	NO_MAP_LOCK_REQUIRED;

	if (lua_isstring(L, index)) {
		return getClient(L)->tsrc()->getTexture(readParam<std::string>(L, index));
	} else {
		video::ITexture *texture = (*(LuaTexture **)luaL_checkudata(
			L, index, "Texture"))->texture;
		if (!allow_window && texture == nullptr)
			throw LuaError("Invalid use of minetest.window as normal texture");
		return texture;
	}
}

void LuaTexture::try_destroy()
{
	if (is_dead && texture != nullptr) {
		// Only drop reference textures, and only remove non-reference non-bound textures
		if (is_ref)
			texture->drop();
		else if (!is_bound && !is_target)
			RenderingEngine::get_video_driver()->removeTexture(texture);
	}
}

int LuaTexture::gc_object(lua_State *L)
{
	LuaTexture *t = *(LuaTexture **)lua_touserdata(L, 1);
	t->is_dead = true;
	t->try_destroy();
	delete t;
	return 0;
}

void LuaTexture::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, "Texture");
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
	lua_setglobal(L, "Texture");
}

const luaL_Reg LuaTexture::methods[] =
{
	luamethod(LuaTexture, is_ref),
	luamethod(LuaTexture, get_original_size),
	luamethod(LuaTexture, get_size),
	luamethod(LuaTexture, fill),
	luamethod(LuaTexture, draw_pixel),
	luamethod(LuaTexture, draw_rect),
	luamethod(LuaTexture, draw_texture),
	luamethod(LuaTexture, draw_text),
	luamethod(LuaTexture, draw_item),
	luamethod(LuaTexture, draw_transformed_texture),
	luamethod(LuaTexture, bind_ingame),
	{0, 0}
};
