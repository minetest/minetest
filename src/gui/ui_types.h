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

#include "debug.h"
#include "exceptions.h"
#include "irrlichttypes_extrabloated.h"

namespace ui
{
	template<typename T>
	using Vec = core::vector2d<T>;
	template<typename T>
	using Dim = core::dimension2d<T>;
	template<typename T>
	using Rect = core::rect<T>;

	using Color = video::SColor;

	const Color BLANK = 0x00000000;
	const Color WHITE = 0xFFFFFFFF;
	const Color BLACK = 0xFF000000;

	template<typename T>
	T &left(Rect<T> &rect) { return rect.UpperLeftCorner.X; }
	template<typename T>
	const T &left(const Rect<T> &rect) { return rect.UpperLeftCorner.X; }

	template<typename T>
	T &top(Rect<T> &rect) { return rect.UpperLeftCorner.Y; }
	template<typename T>
	const T &top(const Rect<T> &rect) { return rect.UpperLeftCorner.Y; }

	template<typename T>
	T &right(Rect<T> &rect) { return rect.LowerRightCorner.X; }
	template<typename T>
	const T &right(const Rect<T> &rect) { return rect.LowerRightCorner.X; }

	template<typename T>
	T &bottom(Rect<T> &rect) { return rect.LowerRightCorner.Y; }
	template<typename T>
	const T &bottom(const Rect<T> &rect) { return rect.LowerRightCorner.Y; }

	template<typename T>
	Rect<T> add_rect_edges(const Rect<T> &rect, const Rect<T> &edges)
	{
		return Rect<T>(
			rect.UpperLeftCorner - edges.UpperLeftCorner,
			rect.LowerRightCorner + edges.LowerRightCorner
		);
	}

	template<typename T>
	Rect<T> sub_rect_edges(const Rect<T> &rect, const Rect<T> &edges)
	{
		return Rect<T>(
			rect.UpperLeftCorner + edges.UpperLeftCorner,
			rect.LowerRightCorner - edges.LowerRightCorner
		);
	}

	template<typename T>
	Rect<T> add_edges(const Rect<T> &lhs, const Rect<T> &rhs)
	{
		return Rect<T>(
			lhs.UpperLeftCorner + rhs.UpperLeftCorner,
			lhs.LowerRightCorner + rhs.LowerRightCorner
		);
	}

	template<typename T>
	Rect<T> sub_edges(const Rect<T> &lhs, const Rect<T> &rhs)
	{
		return Rect<T>(
			lhs.UpperLeftCorner - rhs.UpperLeftCorner,
			lhs.LowerRightCorner - rhs.LowerRightCorner
		);
	}

	template<typename T>
	Dim<T> edges_dim(const Rect<T> &edges)
	{
		return Dim<T>(
			left(edges) + right(edges),
			top(edges) + bottom(edges)
		);
	}

	template<typename T>
	const T &dim_at(const Dim<T> &dim, size_t index)
	{
		sanity_check(index < 2);
		return index == 1 ? dim.Height : dim.Width;
	}

	template<typename T>
	T &dim_at(Dim<T> &dim, size_t index)
	{
		sanity_check(index < 2);
		return index == 1 ? dim.Height : dim.Width;
	}

	template<typename T>
	Dim<T> dim_min(const Dim<T> &first, const Dim<T> &second)
	{
		return Dim<T>(
			std::min(first.Width, second.Width),
			std::min(first.Height, second.Height)
		);
	}

	template<typename T>
	Dim<T> dim_max(const Dim<T> &first, const Dim<T> &second)
	{
		return Dim<T>(
			std::max(first.Width, second.Width),
			std::max(first.Height, second.Height)
		);
	}

	struct UiException : BaseException
	{
		UiException(const std::string &message) : BaseException(message) {}
	};

	constexpr const char *NO_ID = "";

	constexpr const char *MAIN_ID_CHARS =
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-";
	constexpr const char *VIRT_ID_CHARS =
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-@";
}
