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

#include "design.h"
#include "drawable.h"
#include "json_reader.h"
#include "ui_types.h"

#include <array>
#include <string>
#include <unordered_set>
#include <vector>

namespace ui
{
	enum Slice
	{
		SLICE_TOP_LEFT,
		SLICE_TOP,
		SLICE_TOP_RIGHT,
		SLICE_LEFT,
		SLICE_CENTER,
		SLICE_RIGHT,
		SLICE_BOTTOM_LEFT,
		SLICE_BOTTOM,
		SLICE_BOTTOM_RIGHT,

		SLICE_LEN,
	};

	class Env;

	class Elem
	{
	private:
		Env &m_env;
		std::string m_id; // Computed; cross reference to ID in Env

		std::vector<std::string> m_children;
		std::string m_parent; // Computed

		std::unordered_set<std::string> m_classes;

		Vec<float> m_pos;
		Dim<float> m_size;
		Vec<float> m_anchor;

		Dim<float> m_min_size;
		Dim<float> m_max_size;

		Rect<float> m_padding;
		Rect<float> m_border;
		Rect<float> m_margin;

		Dim<float> m_abs_min_size; // Computed by sizer
		Rect<float> m_draw_rect; // Computed by sizer

		bool m_visible;
		bool m_noclip;

		Design m_background;
		std::array<Design, SLICE_LEN> m_slices;

	public:
		Elem(Env &env, std::string id) :
			m_env(env),
			m_id(std::move(id))
		{
			resetStyle();
		}

		Elem(const Elem &) = delete;
		Elem &operator=(const Elem &) = delete;

		virtual ~Elem() = default;

		Env &getEnv()
		{
			return m_env;
		}

		const Env &getEnv() const
		{
			return m_env;
		}

		const std::string &getId() const
		{
			return m_id;
		}

		const std::vector<std::string> &getChildren() const
		{
			return m_children;
		}

		const std::string &getParent() const
		{
			return m_parent;
		}

		void draw(const Rect<float> &parent_rect);
		virtual void onDraw(Canvas &canvas);

		void applyJson(const JsonReader &json);
		virtual void applyState(const JsonReader &json);
		virtual void resetStyle();
		virtual void applyStyle(const JsonReader &json);

	private:
		// Begin Env friend interface
		friend class Env;

		void setParent(std::string parent)
		{
			m_parent = std::move(parent);
		}
		// End Env friend interface
	};
}
