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

#pragma once

#include "log.h"
#include "ui_types.h"

namespace ui
{
	/** Represents a texture-like surface, either an actual Irrlicht ITexture or the
	  * screen.
	  *
	  * Textures have a concept of "ownership": If a Texture created the underlying
	  * ITexture, a Texture will free that ITexture from the texture cache unless someone
	  * else takes ownership of it. However, if the ITexture came from somewhere else, the
	  * Texture will not free it. Note that the screen does not have or need ownership.
	  *
	  * Copied texture objects share the same underlying ITexture rather than copying it.
	  */
	class Texture
	{
	private:
		/** The underlying ITexture, or nullptr for the screen. */
		video::ITexture *m_texture;

		/** True iff this Texture is an owner of the underlying texture and will free it
		  * from the texture cache upon destruction if we hold the last reference to it.
		  *
		  * Textures we don't own can't be freed automatically because other code doesn't
		  * necessarily call `grab()` on textures consistently.
		  */
		bool m_owner;

	public:
		/** Creates a new texture that points to the screen. */
		Texture() :
			Texture(nullptr)
		{}

		/** Creates a new Texture from an existing ITexture. It will not have ownership
		  * for freeing it.
		  *
		  * \param texture The existing ITexture to use, or nullptr to use the screen.
		  */
		Texture(video::ITexture *texture) :
			m_texture(texture),
			m_owner(false)
		{
			if (m_texture != nullptr)
				m_texture->grab();
		}

		/** Creates a new texture of the specified size. It will be transparent by
		  * default. If texture creation fails or the driver doesn't support render
		  * textures, this Texture object will point to the screen instead.
		  *
		  * \param size The size of the texture to create.
		  */
		Texture(const Dim<s32> &size);

		Texture(const Texture &other) :
			m_texture(other.m_texture),
			m_owner(other.m_owner)
		{
			if (m_texture != nullptr)
				m_texture->grab();
		}

		Texture(Texture &&other) noexcept :
			Texture(nullptr)
		{
			swap(*this, other);
		}

		Texture &operator=(Texture other)
		{
			swap(*this, other);
			return *this;
		}

		friend void swap(Texture &first, Texture &second)
		{
			using std::swap;
			swap(first.m_owner, second.m_owner);
			swap(first.m_texture, second.m_texture);
		}

		~Texture();

		/** \return The size in pixels of this texture. */
		Dim<s32> getSize() const;

		/** \return The width of this texture in pixels. */
		s32 getWidth() const
		{
			return getSize().Width;
		}

		/** \return The height of this texture in pixels. */
		s32 getHeight() const
		{
			return getSize().Height;
		}

		/** \return True iff this is the screen. */
		bool isScreen() const
		{
			return m_texture == nullptr;
		}

		/** Draws a pixel of the specified color.
		  *
		  * \param pos   The position of the top left corner of the pixel.
		  * \param color The color of the pixel to draw. Alpha will be added.
		  * \param clip  If non-null, the pixel will not be drawn if it falls outside
		  *              these bounds.
		  */
		void drawPixel(const Vec<s32> &pos, Color color, const Rect<s32> *clip = nullptr);

		/** Draws a filled rectangle of the specified color.
		  *
		  * \param rect  The rectangle to draw.
		  * \param color The color to draw the rectangle. Alpha will be added.
		  * \param clip  If non-null, the rectangle will be clipped to these bounds.
		  */
		void drawRect(const Rect<s32> &rect, Color color, const Rect<s32> *clip = nullptr);

		/** Draws an unscaled texture at some position.
		  *
		  * \param pos     The position of the top left corner of the texture.
		  * \param texture The texture to draw at that position. Alpha will be added. It
		  *                may not be the screen or this texture.
		  * \param src     If non-null, this is the sub-rectangle of the texture that will
		  *                be drawn. Otherwise, the entire texture will be used.
		  * \param clip    If non-null, parts of the texture outside these bounds will not
		  *                be drawn.
		  * \param tint    The color to multiply the texture by. `WHITE` will result in
		  *                the texture being drawn normally.
		  */
		void drawTexture(
			const Vec<s32> &pos,
			const Texture &texture,
			const Rect<s32> *src = nullptr,
			const Rect<s32> *clip = nullptr,
			Color tint = WHITE);

		/** Draws a scaled texture at some position.
		  *
		  * \param rect The rectangle to draw the texture at. The texture will be scaled
		  *             to this rectangle.
		  * See the other `drawTexture()` overload for information on the other
		  * parameters.
		  */
		void drawTexture(
			const Rect<s32> &rect,
			const Texture &texture,
			const Rect<s32> *src = nullptr,
			const Rect<s32> *clip = nullptr,
			Color tint = WHITE);

		/** Fills the texture entirely with a single color. Unlike `drawRect()`, this does
		  * not add alpha, but replaces each pixel with the alpha verbatim.
		  *
		  * \param color The color to fill the texture with. Alpha is ignored if this is
		  *              the screen.
		  */
		void drawFill(Color color);

		/** \return True iff the two textures have the same underlying texture. */
		friend bool operator==(const Texture &left, const Texture &right)
		{
			return left.m_texture == right.m_texture;
		}

		/** \return True iff the two textures do not have the same underlying texture. */
		friend bool operator!=(const Texture &left, const Texture &right)
		{
			return !(left == right);
		}
	};

	/** A drawable durface that works as an abstraction layer around a subset of a texture.
	  * It effectively pretends that a certain sub-rectangle of a texture is the entire
	  * texture, with all operations automatically drawing relative to the top right of
	  * the sub-rectangle and clipping to that sub-rectangle.
	  *
	  * Canvases additionally support scaling and offsets built in. All coordinates passed
	  * in are scaled by the scale factor and have the drawing offset added to them. As
	  * such, all coordinates are specified in floating point multiples of that scaling
	  * factor, and are rounded to the nearest pixel when drawing. Hence, a factor of 2.0
	  * and a position of 1.5 would result in 3 pixels.
	  *
	  * All drawing operations still have clipping rectangles. Those rectangles are first
	  * clipped to the sub-rectangle, and that new rectangle is used for the clipping
	  * operations. Clipping can additionally be turned off with `setNoClip()` so drawing
	  * operations may affect the entire underlying texture while still drawing relative
	  * to the sub-rectangle.
	  *
	  * No clipping is done on the sub-rectangle against the texture since that doesn't
	  * play well with the screen, which can resize. Additionally, drawing operations
	  * already gracefully handle drawing out of bounds.
	  */
	class Canvas
	{
	private:
		/** The underlying texture that is actually drawn to. */
		Texture m_texture;

		/** The scale factor of the canvas, i.e. what all dimensions will be multiplied by
		  * when drawing.
		  */
		float m_scale;

		/** The sub-rectangle of the underlying texture that all drawing draws relative to
		  * and clips to. It is multiplied by the scale factor.
		  */
		Rect<float> m_sub_rect;

		/** The shift that will be added to each position in drawing. It is multiplied by
		  * the scale factor.
		  */
		Vec<float> m_shift;

		/** Iff true, drawing operations will not clip to `m_sub_rect`. */
		bool m_noclip = false;

	public:
		/** Creates a new canvas that draws to the entire area of a texture. It will have
		  * a scale of zero and an empty sub-rectangle, so these must be configured before
		  * anything will be drawn.
		  *
		  * \param texture The underlying texture to draw to.
		  */
		Canvas(Texture texture) :
			m_texture(std::move(texture))
		{}

		Texture &getTexture()
		{
			return m_texture;
		}

		const Texture &getTexture() const
		{
			return m_texture;
		}

		void setTexture(Texture texture)
		{
			m_texture = std::move(texture);
		}

		float getScale() const
		{
			return m_scale;
		}

		void setScale(float scale)
		{
			m_scale = scale;
		}

		const Rect<float> &getSubRect() const
		{
			return m_sub_rect;
		}

		void setSubRect(const Rect<float> &sub_rect)
		{
			m_sub_rect = sub_rect;
		}

		const Vec<float> &getShift() const
		{
			return m_shift;
		}

		void setShift(const Vec<float> &shift)
		{
			m_shift = shift;
		}

		bool getNoClip() const
		{
			return m_noclip;
		}

		void setNoClip(bool noclip)
		{
			m_noclip = noclip;
		}

		void drawPixel(
			const Vec<float> &pos,
			Color color,
			const Rect<float> *clip = nullptr);

		void drawRect(
			const Rect<float> &rect,
			Color color,
			const Rect<float> *clip = nullptr);

		void drawTexture(
			const Vec<float> &pos,
			const Texture &texture,
			const Rect<s32> *src = nullptr,
			const Rect<float> *clip = nullptr,
			Color tint = WHITE);

		void drawTexture(
			const Rect<float> &rect,
			const Texture &texture,
			const Rect<s32> *src = nullptr,
			const Rect<float> *clip = nullptr,
			Color tint = WHITE);

	private:
		Vec<s32> getDrawPos(const Vec<float> &pos) const;

		Dim<s32> getDrawDim(const Dim<float> &dim) const;

		Rect<s32> getDrawRect(const Rect<float> &rect) const;

		Dim<float> getCoordDim(const Dim<s32> &dim) const;

		/** Given a possibly nullptr clipping rectangle from some drawing operation,
		  * returns a new rectangle that is the clipping rectangle clipped against the
		  * sub-rectangle, taking into account no-clipping.
		  *
		  * \param The possibly nullptr clipping rectangle from the drawing operation.
		  * \return The drawing rectangle to clip against for this drawing operation.
		  */
		Rect<s32> getClipRect(const Rect<float> *clip) const;
	};

	/** This should be called before performing any drawing with any Texture for this
	  * frame. Drawing to Textures must not be interspersed with native Irrlicht or
	  * OpenGL drawing.
	  */
	void start_drawing();

	/** This should be called after all drawing with Textures is done for this frame.
	  * Drawing with Irrlicht and OpenGL calls may be done again, and the current render
	  * target will be set to the screen.
	  */
	void end_drawing();
}
