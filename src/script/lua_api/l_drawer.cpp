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
#include "script/common/c_converter.h"
#include "settings.h"
#include "util/string.h"
#if USE_FREETYPE
#include "irrlicht_changes/CGUITTFont.h"
#endif

ModApiDrawer s_drawer;

// get_window_size() -> {x = width, y = height}
int ModApiDrawer::l_get_window_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	core::dimension2d<u32> size = RenderingEngine::get_video_driver()->getScreenSize();
	push_v2f(L, v2f(size.Width, size.Height));

	return 1;
}

// draw_rect(rect, color[, clip_rect])
int ModApiDrawer::l_draw_rect(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_drawer.in_callback)
		return 0;

	core::recti rect = read_recti(L, 1);

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 3)) {
		clip_rect = read_recti(L, 3);
		clip_ptr = &clip_rect;
	}

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	// Draw with a gradient if it is an _array_ of four colors as a table with
	// keys might be a ColorSpec in table form
	if (lua_istable(L, 2) && lua_objlen(L, 2) == 4) {
		video::SColor colors[4] = {0x0, 0x0, 0x0, 0x0};
		for (size_t i = 0; i < 4; i++) {
			lua_rawgeti(L, 2, i + 1);
			read_color(L, -1, &colors[i]);
			lua_pop(L, 1);
		}
		driver->draw2DRectangle(rect, colors[0], colors[1], colors[3], colors[2],
			clip_ptr);
	} else {
		video::SColor color(0x0);
		read_color(L, 2, &color);
		driver->draw2DRectangle(color, rect, clip_ptr);
	}

	return 0;
}

// draw_texture(rect, texture[, clip_rect][, middle_rect][, from_rect][, recolor])
int ModApiDrawer::l_draw_texture(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_drawer.in_callback)
		return 0;

	core::recti rect = read_recti(L, 1);
	video::ITexture *texture = getClient(L)->tsrc()->getTexture(
		readParam<std::string>(L, 2));

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 3)) {
		clip_rect = read_recti(L, 3);
		clip_ptr = &clip_rect;
	}

	core::recti from_rect;
	core::dimension2d<u32> size = texture->getOriginalSize();
	if (lua_istable(L, 5)) {
		from_rect = read_recti(L, 5);

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
	if (!lua_isnil(L, 6))
		read_color(L, 6, &color);
	const video::SColor colors[] = {color, color, color, color};

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (lua_istable(L, 4)) {
		core::recti middle_rect = read_recti(L, 4);

		if (middle_rect.LowerRightCorner.X < middle_rect.UpperLeftCorner.X)
			middle_rect.LowerRightCorner.X += from_rect.getWidth();
		if (middle_rect.LowerRightCorner.Y < middle_rect.UpperLeftCorner.Y)
			middle_rect.LowerRightCorner.Y += from_rect.getHeight();

		draw2DImage9Slice(driver, texture, rect, from_rect, middle_rect,
			clip_ptr, colors);
	} else {
		draw2DImageFilterScaled(driver, texture, rect, from_rect,
			clip_ptr, colors, true);
	}

	return 0;
}

// get_texture_size(texture) -> {x = width, y = height}
int ModApiDrawer::l_get_texture_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	core::dimension2d<u32> size = getClient(L)->tsrc()->
		getTexture(readParam<std::string>(L, 2))->getOriginalSize();
	push_v2f(L, v2f(size.Width, size.Height));

	return 1;
}

// Create an IGUIFont from a font styling table.
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

// draw_text(pos, text[, clip_rect][, style])
// style = {color, font, size, bold, italic, underline, strike}
int ModApiDrawer::l_draw_text(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_drawer.in_callback)
		return 0;

	core::recti rect;
	rect.UpperLeftCorner = read_v2s32(L, 1);

	std::wstring text = utf8_to_wide(
		unescape_enriched(readParam<std::string>(L, 2)));
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == L'\n' || text[i] == L'\0')
			text[i] = L' ';
	}

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 3)) {
		clip_rect = read_recti(L, 3);
		clip_ptr = &clip_rect;
	}

	gui::IGUIFont *font = get_font(L, 4);

	lua_getfield(L, 4, "color");
	video::SColor color(0xFFFFFFFF);
	read_color(L, -1, &color);
	lua_pop(L, 1);

	lua_getfield(L, 4, "underline");
	bool underline = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 4, "strike");
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

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

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

// get_text_size(text[, style]) -> {x = width, y = height}
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

	core::dimension2d<u32> size = font->getDimension(text.c_str());
	size.Height += font->getKerningHeight();
	push_v2f(L, v2f(size.Width, size.Height));

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

// draw_item(rect, item[, clip_rect][, angle])
int ModApiDrawer::l_draw_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_drawer.in_callback)
		return 0;

	core::recti rect = read_recti(L, 1);

	Client *client = getClient(L);
	IItemDefManager *idef = client->idef();
	ItemStack item;
	item.deSerialize(readParam<std::string>(L, 2), idef);

	core::recti clip_rect;
	core::recti *clip_ptr = nullptr;
	if (lua_istable(L, 3)) {
		clip_rect = read_recti(L, 3);
		clip_ptr = &clip_rect;
	}

	v3s16 angle;
	if (lua_istable(L, 4))
		angle = read_v3s16(L, 4);

	drawItemStack(RenderingEngine::get_video_driver(), g_fontengine->getFont(), item,
		rect, clip_ptr, client, IT_ROT_NONE, angle, v3s16());

	return 0;
}

// start_effect()
int ModApiDrawer::l_start_effect(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!s_drawer.in_callback || !driver->queryFeature(video::EVDF_RENDER_TO_TARGET))
		return 0;

	video::ITexture *render = driver->addRenderTargetTexture(driver->getScreenSize());
	s_drawer.renderers.push_back(render);
	driver->setRenderTarget(render, true, false);

	return 0;
}

// draw_effect([recolor])
int ModApiDrawer::l_draw_effect(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!s_drawer.in_callback || !driver->queryFeature(video::EVDF_RENDER_TO_TARGET) ||
			s_drawer.renderers.size() == 0)
		return 0;

	video::ITexture *rendered = s_drawer.renderers.back();
	s_drawer.renderers.pop_back();

	if (s_drawer.renderers.size() == 0)
		driver->setRenderTarget(video::ERT_FRAME_BUFFER, false, false);
	else
		driver->setRenderTarget(s_drawer.renderers.back(), false, false);

	core::recti rect(core::vector2di(), driver->getScreenSize());

	video::SColor color(0xFFFFFFFF);
	if (!lua_isnil(L, 1))
		read_color(L, 1, &color);
	const video::SColor colors[] = {color, color, color, color};

	draw2DImageFilterScaled(driver, rendered, rect, rect, nullptr, colors, true);

	return 0;
}

// TEMP_TRANSFORM(rect, texture, transform)
int ModApiDrawer::l_TEMP_TRANSFORM(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!s_drawer.in_callback)
		return 0;

	core::recti rect = read_recti(L, 1);
	video::ITexture *texture = getClient(L)->tsrc()->getTexture(
		readParam<std::string>(L, 2));

	f32 m_data[16];
	for (size_t i = 0; i < 16; i++) {
		lua_rawgeti(L, 3, i + 1);
		m_data[i] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	core::matrix4 transform;
	transform.setM(m_data);

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

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
}

// effects_supported() -> bool
int ModApiDrawer::l_effects_supported(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	lua_pushboolean(L, RenderingEngine::get_video_driver()->
		queryFeature(video::EVDF_RENDER_TO_TARGET));
	return 1;
}

ModApiDrawer::ModApiDrawer()
{
	in_callback = false;

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
	API_FCT(get_window_size);
	API_FCT(draw_rect);
	API_FCT(draw_texture);
	API_FCT(get_texture_size);
	API_FCT(draw_text);
	API_FCT(get_text_size);
	API_FCT(get_font_size);
	API_FCT(draw_item);
	API_FCT(start_effect);
	API_FCT(draw_effect);
	API_FCT(effects_supported);
	API_FCT(TEMP_TRANSFORM);
};

void ModApiDrawer::start_callback()
{
	s_drawer.in_callback = true;
}

void ModApiDrawer::end_callback()
{
	s_drawer.in_callback = false;
	RenderingEngine::get_video_driver()->setRenderTarget(
		video::ERT_FRAME_BUFFER, false, false);
	s_drawer.renderers.clear();
}
