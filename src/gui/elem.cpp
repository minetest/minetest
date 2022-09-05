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

#include "elem.h"

#include "env.h"
#include "manager.h"

namespace ui
{
	void Elem::draw(const Rect<float> &parent_rect)
	{
		if (!m_visible) {
			return;
		}

		Vec<float> draw_pos(
			m_pos.X - (m_anchor.X * m_size.Width),
			m_pos.Y - (m_anchor.Y * m_size.Height)
		);
		m_draw_rect = Rect<float>(draw_pos, m_size);

		Canvas canvas{Texture()};
		canvas.setScale(m_env.getPixelSize());
		canvas.setSubRect(parent_rect);
		canvas.setNoClip(m_noclip);

		onDraw(canvas);

		for (size_t i = 0; i < m_children.size(); i++) {
			m_env.getElem(m_children[i]).draw(m_draw_rect);
		}
	}

	void Elem::onDraw(Canvas &canvas)
	{
		m_background.drawTo(canvas, m_draw_rect);

		Rect<float> total_rect = add_rect_edges(m_draw_rect, m_border);

		Rect<float> outer(
			std::min(left(total_rect), left(m_draw_rect)),
			std::min(top(total_rect), top(m_draw_rect)),
			std::max(right(total_rect), right(m_draw_rect)),
			std::max(bottom(total_rect), bottom(m_draw_rect))
		);
		Rect<float> inner(
			std::max(left(total_rect), left(m_draw_rect)),
			std::max(top(total_rect), top(m_draw_rect)),
			std::min(right(total_rect), right(m_draw_rect)),
			std::min(bottom(total_rect), bottom(m_draw_rect))
		);

		m_slices[SLICE_TOP_LEFT].drawTo(canvas,
				{left(outer), top(outer), left(inner), top(inner)});
		m_slices[SLICE_TOP].drawTo(canvas,
				{left(inner), top(outer), right(inner), top(inner)});
		m_slices[SLICE_TOP_RIGHT].drawTo(canvas,
				{right(inner), top(outer), right(outer), top(inner)});

		m_slices[SLICE_LEFT].drawTo(canvas,
				{left(outer), top(inner), left(inner), bottom(inner)});
		m_slices[SLICE_CENTER].drawTo(canvas,
				{left(inner), top(inner), right(inner), bottom(inner)});
		m_slices[SLICE_RIGHT].drawTo(canvas,
				{right(inner), top(inner), right(outer), bottom(inner)});

		m_slices[SLICE_BOTTOM_LEFT].drawTo(canvas,
				{left(outer), bottom(inner), left(inner), bottom(outer)});
		m_slices[SLICE_BOTTOM].drawTo(canvas,
				{left(inner), bottom(inner), right(inner), bottom(outer)});
		m_slices[SLICE_BOTTOM_RIGHT].drawTo(canvas,
				{right(inner), bottom(inner), right(outer), bottom(outer)});
	}

	void Elem::applyJson(const JsonReader &json)
	{
		if (auto val = json["state"]) {
			applyState(val);
		}
		if (auto val = json["style"]) {
			applyStyle(val);
		}
	}

	void Elem::applyState(const JsonReader &json)
	{
		json["children"].readVector<std::string>(m_children,
				JSON_BIND_TO(toId, MAIN_ID_CHARS, false));
		json["classes"].readSet<std::string>(m_classes,
				JSON_BIND_TO(toId, MAIN_ID_CHARS, false));
	}

	void Elem::resetStyle()
	{
		m_pos = Vec<float>(0.5f);
		m_size = Vec<float>(1.0f);
		m_anchor = Vec<float>(0.5f);

		m_min_size = Vec<float>();
		m_max_size = Vec<float>(Lim<float>::high());

		m_padding = Rect<float>();
		m_border = Rect<float>();
		m_margin = Rect<float>();

		m_visible = true;
		m_noclip = false;

		m_background.resetStyle();
		for (size_t i = 0; i < m_slices.size(); i++) {
			m_slices[i].resetStyle();
		}
	}

	void Elem::applyStyle(const JsonReader &json)
	{
		json["pos"].readVec(m_pos);
		json["size"].readDim(m_size);
		json["anchor"].readVec(m_anchor);

		json["min_size"].readDim(m_min_size);
		json["max_size"].readDim(m_max_size);

		json["padding"].readEdges(m_padding);
		json["border"].readEdges(m_border);
		json["margin"].readEdges(m_margin);

		json["visible"].readBool(m_visible);
		json["noclip"].readBool(m_noclip);

		if (auto val = json["background"]) {
			m_background.applyView(val);
		}

		json["slices"].readArray(m_slices, [](const JsonReader &val, Design &slice) {
			if (val) {
				slice.applyView(val);
			}
		});
	}
}
