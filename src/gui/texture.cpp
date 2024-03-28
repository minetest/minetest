/*
Minetest
Copyright (C) 2022 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include "gui/texture.h"

#include "client/renderingengine.h"
#include "debug.h"
#include "log.h"

namespace ui
{
	static video::IVideoDriver *driver() {
		return RenderingEngine::get_video_driver();
	}

	// The current render target. We keep track of this in set_as_target() so
	// as to only change render targets when necessary. It is only valid
	// between start_drawing() and end_drawing(); otherwise, it's nullptr.
	video::ITexture *s_current_target = nullptr;

	static void force_screen_target()
	{
		// Force-set the render target to the screen, regardless of the current
		// value of s_current_target.
		driver()->setRenderTarget(nullptr, false, 0);
		s_current_target = nullptr;
	}

	static void set_as_target(video::ITexture *target, bool clear = false,
			video::SColor color = BLANK)
	{
		// Don't change the render target if it's already set.
		if (s_current_target == target) {
			return;
		}

		// If we want to clear it to a certain color, then we go ahead and
		// clear the depth buffer as well.
		u16 to_clear = clear ? video::ECBF_ALL : video::ECBF_NONE;

		if (driver()->setRenderTarget(target, to_clear, color)) {
			// The call succeeded, so update the current target variable.
			s_current_target = target;
		} else {
			// The call failed, so we probably don't support render targets.
			errorstream << "Unable to set render target" << std::endl;
			force_screen_target();
		}
	}

	void Texture::begin()
	{
		// Force set the target since we don't know what the target was before,
		// so s_current_target could possibly be invalid.
		force_screen_target();
	}

	void Texture::end()
	{
		set_as_target(nullptr);
	}

	Texture Texture::screen = {};

	Texture::Texture(const d2s32 &size)
	{
		if (driver()->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
			m_texture.grab(driver()->addRenderTargetTexture(d2u32(size)));
			if (!isTexture()) {
				errorstream << "Unable to create render target" << std::endl;
			}

			// The default contents of a texture are appear to be unspecified,
			// even though some it's usually transparent by default. So, we
			// explicitly make it transparent.
			drawFill(BLANK);
		}
	}

	Texture::Texture(video::ITexture *texture)
	{
		m_texture.grab(texture);
	}

	Texture::~Texture()
	{
		// If the reference count is one, only Irrlicht still holds a reference
		// to the texture, so we can remove it from the driver.
		if (isTexture() && m_texture.get()->getReferenceCount() == 1) {
			driver()->removeTexture(m_texture.get());
		}
	}

	d2s32 Texture::getSize() const
	{
		if (isTexture()) {
			return d2s32(m_texture->getOriginalSize());
		}
		return d2s32(driver()->getScreenSize());
	}

	void Texture::drawPixel(
		const v2s32 &pos,
		video::SColor color,
		const rs32 *clip)
	{
		drawRect(rs32(pos, d2s32(0, 0)), color, clip);
	}

	void Texture::drawRect(
		const rs32 &rect,
		video::SColor color,
		const rs32 *clip)
	{
		set_as_target(m_texture.get());
		driver()->draw2DRectangle(color, rect, clip);
	}

	void Texture::drawTexture(
		const v2s32 &pos,
		const Texture &texture,
		const rs32 *src,
		const rs32 *clip,
		video::SColor tint)
	{
		d2s32 size = (src != nullptr) ? src->getSize() : texture.getSize();
		drawTexture(rs32(pos, size), texture, src, clip, tint);
	}

	void Texture::drawTexture(
		const rs32 &rect,
		const Texture &texture,
		const rs32 *src,
		const rs32 *clip,
		video::SColor tint)
	{
		if (!texture.isTexture()) {
			errorstream << "Can't draw the screen to a texture" << std::endl;
			return;
		}
		if (m_texture.get() == texture.m_texture.get()) {
			errorstream << "Can't draw a texture to itself" << std::endl;
			return;
		}

		set_as_target(m_texture.get());

		// If we don't have a source rectangle, make it encompass the entire
		// texture.
		rs32 texture_rect(texture.getSize());
		if (src == nullptr) {
			src = &texture_rect;
		}

		// All the corners should have the same tint.
		video::SColor tints[] = {tint, tint, tint, tint};

		driver()->draw2DImage(
			texture.m_texture.get(), rect, *src, clip, tints, true);
	}

	void Texture::drawFill(video::SColor color)
	{
		if (isTexture()) {
			/* There's no normal way to fill a texture with a color; drawing a
			 * rect will add alpha, not replace it. So, use setRenderTarget()
			 * to clear it instead. Irrlicht will ignore the call if this
			 * texture is already the current render target, so we set it to
			 * the screen first before attempting to clear the texture.
			 */
			set_as_target(nullptr);
			set_as_target(m_texture.get(), true, color);
		} else {
			// The screen can't have transparency, so make the color opaque and
			// draw a rectangle across the entire screen.
			color.setAlpha(0xFF);
			driver()->draw2DRectangle(color, rs32(getSize()));
		}
	}

	Canvas::Canvas(Texture &texture, float scale, const rf32 *clip) :
		m_texture(texture),
		m_scale(scale)
	{
		if (clip == nullptr) {
			m_clip_ptr = nullptr;
		} else {
			m_clip = rs32(
				clip->UpperLeftCorner.X * m_scale,
				clip->UpperLeftCorner.Y * m_scale,
				clip->LowerRightCorner.X * m_scale,
				clip->LowerRightCorner.Y * m_scale
			);
			m_clip_ptr = &m_clip;
		}
	}

	void Canvas::drawRect(
		const rf32 &rect,
		video::SColor color)
	{
		m_texture.drawRect(getDrawRect(rect), color, m_clip_ptr);
	}

	void Canvas::drawTexture(
		const v2f32 &pos,
		const Texture &texture,
		const rf32 &src,
		video::SColor tint)
	{
		drawTexture(rf32(pos, d2f32(texture.getSize())), texture, src, tint);
	}

	void Canvas::drawTexture(
		const rf32 &rect,
		const Texture &texture,
		const rf32 &src,
		video::SColor tint)
	{
		rs32 draw_src(
			src.UpperLeftCorner.X * texture.getWidth(),
			src.UpperLeftCorner.Y * texture.getHeight(),
			src.LowerRightCorner.X * texture.getWidth(),
			src.LowerRightCorner.Y * texture.getHeight()
		);
		m_texture.drawTexture(
			getDrawRect(rect), texture, &draw_src, m_clip_ptr, tint);
	}

	rs32 Canvas::getDrawRect(const rf32 &rect) const
	{
		return rs32(
			rect.UpperLeftCorner.X * m_scale,
			rect.UpperLeftCorner.Y * m_scale,
			rect.LowerRightCorner.X * m_scale,
			rect.LowerRightCorner.Y * m_scale
		);
	}
}
