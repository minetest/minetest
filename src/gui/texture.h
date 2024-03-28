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

#include "irr_ptr.h"
#include "irrlichttypes_extrabloated.h"

namespace ui
{
	using d2s32 = core::dimension2di;
	using d2u32 = core::dimension2du;
	using d2f32 = core::dimension2df;

	using rs32 = core::recti;
	using rf32 = core::rectf;

	const video::SColor BLANK = 0x00000000;
	const video::SColor BLACK = 0xFF000000;
	const video::SColor WHITE = 0xFFFFFFFF;

	template<typename T>
	core::vector2d<T> clamp_vec(core::vector2d<T> vec)
	{
		if (vec.X < 0)
			vec.X = 0;
		if (vec.Y < 0)
			vec.Y = 0;
		return vec;
	}

	template<typename T>
	core::rect<T> clamp_rect(core::rect<T> rect)
	{
		if (rect.getWidth() < 0)
			rect.LowerRightCorner.X = rect.UpperLeftCorner.X;
		if (rect.getHeight() < 0)
			rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y;
		return rect;
	}

	template<typename T>
	core::rect<T> clip_rect(core::rect<T> first, const core::rect<T> &second)
	{
		first.clipAgainst(second);
		return first;
	}

	template<typename T>
	T &dim_at(core::dimension2d<T> &dim, size_t index)
	{
		return index == 0 ? dim.Width : dim.Height;
	}

	template<typename T>
	const T &dim_at(const core::dimension2d<T> &dim, size_t index)
	{
		return index == 0 ? dim.Width : dim.Height;
	}

	class Texture
	{
	private:
		irr_ptr<video::ITexture> m_texture = nullptr;

	public:
		// These functions must surround any drawing calls to Texture instances
		// to ensure proper tracking of render targets. Raw Irrlicht draw calls
		// should be used with caution to avoid messing up render targets.
		static void begin();
		static void end();

		static Texture screen;

		Texture() = default;
		Texture(video::ITexture *texture);
		Texture(const d2s32 &size);

		~Texture();

		d2s32 getSize() const;

		s32 getWidth() const { return getSize().Width; }
		s32 getHeight() const { return getSize().Height; }

		bool isTexture() const
		{
			return m_texture.get() != nullptr;
		}

		void drawPixel(
			const v2s32 &pos,
			video::SColor color,
			const rs32 *clip = nullptr);

		void drawRect(
			const rs32 &rect,
			video::SColor color,
			const rs32 *clip = nullptr);

		void drawTexture(
			const v2s32 &pos,
			const Texture &texture,
			const rs32 *src = nullptr,
			const rs32 *clip = nullptr,
			video::SColor tint = WHITE);

		void drawTexture(
			const rs32 &rect,
			const Texture &texture,
			const rs32 *src = nullptr,
			const rs32 *clip = nullptr,
			video::SColor tint = WHITE);

		void drawFill(video::SColor color);

		friend bool operator==(const Texture &left, const Texture &right)
		{
			return left.m_texture == right.m_texture;
		}

		friend bool operator!=(const Texture &left, const Texture &right)
		{
			return !(left == right);
		}
	};

	class Canvas
	{
	private:
		Texture &m_texture;

		float m_scale;

		rs32 m_clip;
		rs32 *m_clip_ptr;

	public:
		Canvas(Texture &texture, float scale, const rf32 *clip = nullptr);

		Canvas(Canvas &canvas, const rf32 *clip = nullptr) :
			Canvas(canvas.getTexture(), canvas.getScale(), clip)
		{}

		Texture &getTexture() { return m_texture; }
		const Texture &getTexture() const { return m_texture; }

		float getScale() const { return m_scale; }

		void drawRect(
			const rf32 &rect,
			video::SColor color);

		void drawTexture(
			const v2f32 &pos,
			const Texture &texture,
			const rf32 &src = rf32(0.0f, 0.0f, 1.0f, 1.0f),
			video::SColor tint = WHITE);

		void drawTexture(
			const rf32 &rect,
			const Texture &texture,
			const rf32 &src = rf32(0.0f, 0.0f, 1.0f, 1.0f),
			video::SColor tint = WHITE);

	private:
		rs32 getDrawRect(const rf32 &rect) const;
	};
}
