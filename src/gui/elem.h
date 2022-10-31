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
#include "util/string.h"

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

	enum LayoutDir
	{
		LAYOUT_DIR_HORIZ,
		LAYOUT_DIR_VERT
	};

	constexpr EnumNameMap<LayoutDir> LAYOUT_DIR_NAME_MAP = {
		{"horiz", LAYOUT_DIR_HORIZ},
		{"vert", LAYOUT_DIR_VERT}
	};

	enum Spacing
	{
		SPACING_BEFORE,
		SPACING_BETWEEN,
		SPACING_AFTER,

		SPACING_LEN
	};

	enum class ExtraSpace
	{
		OUTSIDE,
		AROUND,
		BETWEEN
	};

	constexpr EnumNameMap<ExtraSpace> EXTRA_SPACE_NAME_MAP = {
		{"outside", ExtraSpace::OUTSIDE},
		{"around", ExtraSpace::AROUND},
		{"between", ExtraSpace::BETWEEN}
	};

	struct LayoutItem
	{
		std::unordered_set<size_t> elems;
		float weight = 0.0f;

		float size; // Computed

		void applyLayout(const JsonReader &json);
	};

	struct LayoutAxis
	{
		LayoutDir dir = LAYOUT_DIR_HORIZ;

		std::vector<LayoutItem> items;

		std::array<float, SPACING_LEN> spacing = {0.0f, 0.0f, 0.0f};
		ExtraSpace extra_space = ExtraSpace::OUTSIDE;
		float align = 0.0f;

		float min_size; // Computed
		float total_weight; // Computed

		void applyLayout(const JsonReader &json);
	};

	class Elem
	{
	private:
		// Universal properties
		Env &m_env;
		std::string m_id; // Computed; cross reference to ID in Env

		// State properties
		std::vector<std::string> m_children;
		std::string m_parent; // Computed

		std::unordered_set<std::string> m_classes;

		// Layout properties
		Dim<float> m_min_size;

		Vec<float> m_scale;
		Vec<float> m_align;

		Rect<float> m_padding;
		Rect<float> m_border;
		Rect<float> m_margin;

		float m_aspect_ratio;

		std::vector<LayoutAxis> m_axes;

		// Computed layout properties
		Dim<float> m_content_size;

		Rect<float> m_clip_rect;
		Rect<float> m_draw_rect;

		std::array<bool, 2> m_clip_constrained;

		// View properties
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

		void draw();
		virtual void onDraw(Canvas &canvas);

		void applyJson(const JsonReader &json);
		virtual void applyState(const JsonReader &json);

		void resetStyle()
		{
			resetLayout();
			resetView();
		}

		void resetLayout();
		void applyLayout(const JsonReader &json);

		void resetView();
		void applyView(const JsonReader &json);

	private:
		Rect<float> getDrawEdges() const;
		Rect<float> getAllEdges() const;

		Dim<float> getMinDrawSize() const;
		Dim<float> getMinMarginSize() const;

		// Begin Env friend interface
		friend class Env;

		void setParent(std::string parent)
		{
			m_parent = std::move(parent);
		}

		void layout();
		// End Env friend interface

		void layoutContentSize();
		void layoutDrawRect();
		void layoutClipRects();
	};
}
