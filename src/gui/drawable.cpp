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

#include "drawable.h"

#include "client/renderingengine.h"
#include "exceptions.h"

namespace ui
{
	static video::IVideoDriver *driver() {
		return RenderingEngine::get_video_driver();
	}

	/** The current render target. We store this as bookkeeping in `set_as_target()` so we
	  * don't change render targets more often than necessary. It is only valid between
	  * `start_drawing()` and `end_drawing()`; otherwise, it's nullptr.
	  */
	video::ITexture *s_current_target = nullptr;

	/** Sets a new render target, possibly clearing it to a new color. It will do nothing
	  * (including clearing the texture) if the provided target is the current target. If
	  * the target couldn't be set for some reason, an error is logged and the current
	  * render target is set to the screen.
	  *
	  * \param target The new texture (or nullptr for screen) to set as the render target.
	  * \param clear  Iff true, the texture will be cleared to a new color. Depth and
	  *               stencil buffers will also be cleared.
	  * \param color  The color to clear the texture to if `clear` is true.
	  * \param force  Ignores the current state of `s_current_target` and sets the target
	  *               anyway.
	  */
	static void set_as_target(video::ITexture *target, bool clear = false,
			Color color = BLANK, bool force = false)
	{
		if (s_current_target == target && !force) {
			return;
		}

		u16 to_clear = clear ? video::ECBF_ALL : video::ECBF_NONE;
		if (driver()->setRenderTarget(target, to_clear, color)) {
			s_current_target = target;
		} else {
			errorstream << "Unable to set render target" << std::endl;

			// That failed for some reason; the driver probably doesn't support
			// render textures. So, set it back to the screen. If *this* call
			// errors, there's nothing we can do.
			driver()->setRenderTarget(nullptr, to_clear, color);
			s_current_target = nullptr;
		}
	}

	Texture::Texture(const Dim<s32> &size) :
		m_owner(true)
	{
		if (driver()->queryFeature(video::EVDF_RENDER_TO_TARGET)) {
			m_texture = driver()->addRenderTargetTexture(core::dimension2du(size));
		} else {
			m_texture = nullptr;
		}

		if (m_texture != nullptr) {
			// The default contents of a texture are actually unspecified,
			// although it's usually transparent. So, we explicitly fill it.
			drawFill(BLANK);
			m_texture->grab();
		}
	}

	Texture::~Texture()
	{
		// We don't have to do anything to the screen.
		if (m_texture == nullptr) {
			return;
		}

		m_texture->drop();

		// If we own the texture and the reference count is one, only Irrlicht
		// still holds a reference to the texture, so we can remove it from the
		// driver.
		if (m_owner && m_texture->getReferenceCount() == 1) {
			driver()->removeTexture(m_texture);
		}
	}

	Dim<s32> Texture::getSize() const
	{
		if (isScreen()) {
			return Dim<s32>(driver()->getScreenSize());
		}
		return Dim<s32>(m_texture->getOriginalSize());
	}

	void Texture::drawPixel(const Vec<s32> &pos, Color color, const Rect<s32> *clip)
	{
		// We have to manually clip. `Rect::isPointInside()` doesn't do the
		// trick because a pixel has a width of one, so the lower right corner
		// must be exclusive.
		if (clip != nullptr && (
				pos.X < left(*clip) ||
				pos.Y < top(*clip) ||
				pos.X >= right(*clip) ||
				pos.Y >= bottom(*clip))) {
			return;
		}

		set_as_target(m_texture);
		driver()->drawPixel(pos.X, pos.Y, color);
	}

	void Texture::drawRect(const Rect<s32> &rect, Color color, const Rect<s32> *clip)
	{
		set_as_target(m_texture);
		driver()->draw2DRectangle(color, rect, clip);
	}

	void Texture::drawTexture(
		const Vec<s32> &pos,
		const Texture &texture,
		const Rect<s32> *src,
		const Rect<s32> *clip,
		Color tint)
	{
		Dim<s32> size = (src != nullptr) ? src->getSize() : texture.getSize();
		drawTexture(Rect<s32>(pos, size), texture, src, clip, tint);
	}

	void Texture::drawTexture(
		const Rect<s32> &rect,
		const Texture &texture,
		const Rect<s32> *src,
		const Rect<s32> *clip,
		Color tint)
	{
		if (texture.isScreen()) {
			errorstream << "Can't read from screen for drawing." << std::endl;
			return;
		}
		if (*this == texture) {
			errorstream << "Can't draw texture to itself" << std::endl;
			return;
		}

		set_as_target(m_texture);

		// If we don't have a source rectangle, make it encompass the entire
		// texture.
		Rect<s32> texture_rect(texture.getSize());
		if (src == nullptr) {
			src = &texture_rect;
		}

		video::SColor tints[] = {tint, tint, tint, tint};

		driver()->draw2DImage(texture.m_texture, rect, *src, clip, tints, true);
	}

	void Texture::drawFill(Color color)
	{
		if (isScreen()) {
			// The screen can't have transparency, so make the color opaque and
			// draw a rectangle across the entire screen.
			color.setAlpha(0xFF);
			driver()->draw2DRectangle(color, Rect<s32>(getSize()));
		} else {
			/* There's no normal way to fill a texture with a color; drawing a
			 * rect will add alpha, not replace it. So, use `setRenderTarget()`
			 * to clear it instead. Irrlicht will ignore the call if this
			 * texture is already the current render target, so set it to the
			 * screen first before attempting to clear the texture.
			 */
			set_as_target(nullptr);
			set_as_target(m_texture, true, color);
		}
	}

	void Canvas::drawPixel(const Vec<float> &pos, Color color, const Rect<float> *clip)
	{
		drawRect(Rect<float>(pos, Dim<float>(1.0f, 1.0f)), color, clip);
	}

	void Canvas::drawRect(const Rect<float> &rect, Color color, const Rect<float> *clip)
	{
		Rect<s32> clipped = getClipRect(clip);
		m_texture.drawRect(getDrawRect(rect), color, &clipped);
	}

	void Canvas::drawTexture(
		const Vec<float> &pos,
		const Texture &texture,
		const Rect<s32> *src,
		const Rect<float> *clip,
		Color tint)
	{
		Dim<s32> size = (src != nullptr) ? src->getSize() : texture.getSize();
		drawTexture(Rect<float>(pos, getCoordDim(size)), texture, src, clip, tint);
	}

	void Canvas::drawTexture(
		const Rect<float> &rect,
		const Texture &texture,
		const Rect<s32> *src,
		const Rect<float> *clip,
		Color tint)
	{
		Rect<s32> clipped = getClipRect(clip);
		m_texture.drawTexture(getDrawRect(rect), texture, src, &clipped, tint);
	}

	Vec<s32> Canvas::getDrawPos(const Vec<float> &pos) const
	{
		return Vec<s32>(
			(pos.X * m_scale) + m_shift.X + left(m_sub_rect),
			(pos.Y * m_scale) + m_shift.Y + top(m_sub_rect)
		);
	}

	Dim<s32> Canvas::getDrawDim(const Dim<float> &dim) const
	{
		return Dim<s32>(dim.Width * m_scale, dim.Height * m_scale);
	}

	Rect<s32> Canvas::getDrawRect(const Rect<float> &rect) const
	{
		return Rect<s32>(getDrawPos(rect.UpperLeftCorner), getDrawDim(rect.getSize()));
	}

	Dim<float> Canvas::getCoordDim(const Dim<s32> &dim) const
	{
		return Dim<float>(dim.Width / m_scale, dim.Height / m_scale);
	}

	Rect<s32> Canvas::getClipRect(const Rect<float> *clip) const
	{
		Rect<float> clipped(getCoordDim(m_texture.getSize()));

		if (clip != nullptr) {
			clipped.clipAgainst(*clip);
		}
		if (!m_noclip) {
			clipped.clipAgainst(m_sub_rect);
		}

		return getDrawRect(clipped);
	}

	void start_drawing()
	{
		// Force set the target since we don't know what the target was before,
		// so `s_current_target` could possibly be invalid.
		set_as_target(nullptr, false, BLANK, true);
	}

	void end_drawing()
	{
		set_as_target(nullptr);
	}
}
